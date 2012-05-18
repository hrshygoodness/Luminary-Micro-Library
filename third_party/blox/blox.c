/*    $NetBSD: blox.c,v 1.17 2004/01/27 20:30:30 jsm Exp $    */

/*-
 * Copyright (c) 1992, 1993
 *    The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek and Darren F. Provine.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *    @(#)blox.c    8.1 (Berkeley) 5/31/93
 */

/*
 * This version of the file renamed and modified by Texas Instruments.
 *
 */

/*
 * Blox (or however it is spelled).
 */

#include <stdlib.h>
#include <string.h>

#include "random.h"
#include "screen.h"
#include "blox.h"

cell    board[B_SIZE];        /* 1 => occupied, 0 => empty */

int    Rows, Cols;        /* current screen size */

const struct shape *curshape;
const struct shape *nextshape;

long    fallrate;        /* less than 1 million; smaller => faster */
long    elapsed;         /* milliseconds since last tick timeout */
int     score;           /* the obvious thing */
int     pos;             /* Position of the currently falling shape */
int     paused;          /* Set to 1 if the game is currently paused */
int     level = DEFAULTLEVEL;    /* The difficulty level */

static    void    elide(void);
static    void    setup_board(void);

#ifdef NO_BLOX_TITLE
static char pause_msg[] = "";
#else
static char pause_msg[] = "Game Paused";
#endif

/*
 * Set up the initial board.  The bottom display row is completely set,
 * along with another (hidden) row underneath that.  Also, the left and
 * right edges are set.
 */
static void
setup_board()
{
    int i;
    cell *p;

    p = board;
    for (i = B_SIZE; i; i--)
        *p++ = i <= (2 * B_COLS) || (i % B_COLS) < 2;
}

/*
 * Elide any full active rows.
 */
static void
elide()
{
    int i, j, base;
    cell *p;

    for (i = A_FIRST; i < A_LAST; i++) {
        base = i * B_COLS + 1;
        p = &board[base];
        for (j = B_COLS - 2; *p++ != 0;) {
            if (--j <= 0) {
                /* this row is to be elided */
                memset(&board[base], 0, B_COLS - 2);
                scr_update();
                while (--base != 0)
                    board[base + B_COLS] = board[base];
                scr_update();
                break;
            }
        }
    }
}


/*
 * Main game processing function.  This function must be called periodically
 * during game play.  lElapsed is the number of milliseconds that have
 * elapsed since the last time the function was called.  ulFlags indicates which
 * user interface buttons have been pressed since the last time the function
 * was called.
 *
 * Returns the 0 if the game is still in progress or 1 if the game has ended.
 *
 */
int
blox_timer(long lElapsed, unsigned long *pulFlags)
{
    /*
     * Handle any commands passed.
     */
    if (*pulFlags & BLOX_CMD_PAUSE)
    {
        /*
         * Toggle the pause state.
         */
        paused = 1 - paused;

        /*
         * Did we pause or resume the game?
         */
        if(paused)
        {
            /*
             * Pausing - show the pause message.
             */
            place(curshape, pos, 1);
            scr_update();
            scr_msg(pause_msg, 1);
            return(0);
        }
        else
        {
            /*
             * Resume - remove the pause message.
             */
            scr_msg(pause_msg, 0);
            place(curshape, pos, 0);
        }
    }

    if(!paused)
    {
        if (*pulFlags & BLOX_CMD_UP)
        {
            /* move up */
            if (fits_in(curshape, pos - 1))
                pos--;
        }

        if (*pulFlags & BLOX_CMD_ROTATE)
        {
            /* Rotate the shape */
            const struct shape *new = &shapes[curshape->rot];

            if (fits_in(new, pos))
                curshape = new;
        }

        if (*pulFlags & BLOX_CMD_DOWN)
        {
            /* move down */
            if (fits_in(curshape, pos + 1))
                pos++;
        }

        if (*pulFlags & BLOX_CMD_DROP)
        {
            /* move to bottom */
            while (fits_in(curshape, pos + B_COLS)) {
                pos += B_COLS;
                score++;
            }
        }

        /*
         * Update the millisecond counter.
         */
        elapsed += lElapsed ;

        if (elapsed >= (fallrate / 1024))
        {
            /*
             * Timeout.  Speed up and move down if possible.
             */
            faster();
            elapsed = 0;

            /*
             * Does the shape fit in the next row?
             */
            if (fits_in(curshape, pos + B_COLS))
            {
                /*
                 * Yes - move it down one row.
                 */
                pos += B_COLS;

                /*
                 * Tell the caller that we moved a block downwards.
                 */
                *pulFlags |= BLOX_STAT_DOWN;
            }
            else
            {
                /*
                 * The shape doesn't fit on the next row down so put the current
                 * shape `permanently', bump score, and elide any full rows.
                 */
                place(curshape, pos, 1);
                score++;
                elide();

                /*
                 * Choose a new shape.  If it does not fit,
                 * the game is over.
                 */
                curshape = nextshape;
                nextshape = randshape();
                pos = A_FIRST*B_COLS + (B_COLS/2)-1;

                if (!fits_in(curshape, pos))
                {
                    /*
                     * If we get here, the game is over - we can't fit a new shape
                     * at the top of the board.
                     */
                    *pulFlags |= BLOX_STAT_END;
                    return(1);
                }
                else
                {
                    /*
                     * Tell the caller that we are moving on to a new block.
                     */
                    *pulFlags |= BLOX_STAT_DROPPED;
                }
            }
        }

        /*
         * Redraw the screen
         */
        place(curshape, pos, 1);
        scr_update();
        place(curshape, pos, 0);
    }
    /*
     * If we get here, the game continues.
     */
    return(0);
}

/*
 * Initialize in preparation for playing the game.  This function must be
 * called during system initialization and to restart a game.
 *
 * ulRandomSeed is a number that will be used to seed the random number
 * generator used during the game.
 *
 * Returns 0 on success. Any other return value indicates a failure.
 *
 */
int
blox_init(unsigned long ulRandomSeed)
{
    scr_init();
    setup_board();

    /*
     * Add a bit more pseudo-randomness by reseeding the generator.
     */
    RandomAddEntropy(ulRandomSeed);
    RandomSeed();

    pos = A_FIRST*B_COLS + (B_COLS/2)-1;
    nextshape = randshape();
    curshape = randshape();

    fallrate = (1024 * 1024) / level;
    elapsed = 0;
    score = 0;

    paused = 0;

    return(0);
}
