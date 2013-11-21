//*****************************************************************************
//
// blox_screen.c - Screen handling functions used by the Blox game.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "drivers/touch.h"
#include "third_party/blox/blox.h"
#include "third_party/blox/screen.h"
#include "blox_screen.h"
#include "utils/ustdlib.h"
#include "images.h"

#define USE_IMAGE_BACKGROUND

//*****************************************************************************
//
// The array of colored tiles used to draw each shape.  This array is indexed
// by the value storedin the board or g_psCurScreen array to determine what
// image to use to draw the individual blocks on the screen.
//
//*****************************************************************************
const unsigned char *g_ppucBlockImages[MAX_BLOCK_COLORS] =
{
    0,
    g_pucYellowTile12x12,
    g_pucRedTile12x12,
    g_pucBlueTile12x12,
    g_pucGreenTile12x12,
    g_pucMagentaTile12x12,
    g_pucCyanTile12x12,
    g_pucPinkTile12x12
};

//*****************************************************************************
//
// The game screen contents as currently displayed.
//
//*****************************************************************************
static cell g_psCurScreen[B_SIZE];

//*****************************************************************************
//
// The game score as currently displayed.
//
//*****************************************************************************
static int g_iCurScore = -1;

//*****************************************************************************
//
// Are we currently showing a message above the game area?
//
//*****************************************************************************
static tBoolean g_bMsgShown;

//*****************************************************************************
//
// Did the last game update the high score?
//
//*****************************************************************************
static tBoolean g_bNewHighScore;

//*****************************************************************************
//
// Have we just erased the whole screen?  This flag is used to perform a quick
// background refresh the first time the game area is painted.
//
//*****************************************************************************
static tBoolean g_bScreenCleared;

//*****************************************************************************
//
// The highest score obtained since the system booted.
//
//*****************************************************************************
int g_iHighScore = 0;

//*****************************************************************************
//
// A pointer to the message we are showing above the game area.
//
//*****************************************************************************
static char *g_pcMessage;

//*****************************************************************************
//
// The graphics context used to update the game play area.
//
//*****************************************************************************
tContext g_sScreenContext;

//*****************************************************************************
//
// Paint the canvas widget containing the next shape to be played.
//
//*****************************************************************************
void
OnNextPiecePaint(tWidget *pWidget, tContext *pContext)
{
    tRectangle sRect;
    unsigned long ulLoop;
    int iRow, iCol, iOff, iDelta, iXOff, iYOff;

    //
    // Clear the widget background.
    //
    GrContextForegroundSet(pContext, BACKGROUND_COLOR);
    GrRectFill(pContext, &pWidget->sPosition);

    //
    // Although no game piece occupies more than 2 colums in its starting
    // configuration, the definitions actually span 3 colums.  Check for cases
    // that are defined using the second and third columns and move these one
    // block to the left to keep everything in the same 2 columns of the
    // display.  We also use this loop to set the X and Y offset required to
    // center the piece in the canvas widget.
    //
    iDelta = 1;
    iXOff = 0;
    iYOff = GAME_BLOCK_SIZE / 2;

    for(ulLoop = 0; ulLoop < 3; ulLoop++)
    {
        //
        // Does this offset extend into the third column?
        //
        if(nextshape->off[ulLoop] > 2)
        {
            //
            // Yes - we need to move the shape one cell left to keep it
            // within the bounds of the widget.
            //
            iDelta = 0;
            break;
        }

        //
        // If we find an offset of 2, this means we are dealing with the
        // 4-in-a-line shape so we need to muck with the X and Y offset to
        // ensure it is centered in the widget.  Yes - this is a somewhat
        // tacky inference to make but, hey, it works.
        //
        if(nextshape->off[ulLoop] == 2)
        {
            iXOff = -(GAME_BLOCK_SIZE / 2);
            iYOff = 0;
        }
    }

    //
    // Now draw the game piece.  A block at offset (0,0) is always assumed.
    //
    iOff = 0;
    for(ulLoop = 0; ulLoop < 4; ulLoop++)
    {
        //
        // What is the row and column offset for this block in the shape?  This
        // is somewhat awkward since the blocks comprising each shape are
        // defined in terms of index offsets within the board array rather
        // than (x, y) coordinates.
        //
        if(iOff <= -(B_COLS - 1))
        {
            iRow = -1;
            iCol = (iOff + B_COLS);
        }
        else if(iOff >= (B_COLS - 1))
        {
            iRow = 1;
            iCol = (iOff - B_COLS);
        }
        else
        {
            iRow = 0;
            iCol = iOff;
        }

        sRect.sXMin = pWidget->sPosition.sXMin + iXOff +
                      (GAME_BLOCK_SIZE * (iRow + iDelta));
        sRect.sYMin = pWidget->sPosition.sYMin + iYOff +
                      (GAME_BLOCK_SIZE * (iCol + 1));
        sRect.sXMax = sRect.sXMin + (GAME_BLOCK_SIZE - 1);
        sRect.sYMax = sRect.sYMin + (GAME_BLOCK_SIZE - 1);

        //
        // Draw this block of the shape.  We use either an image if one is
        // defined or black otherwise.
        //
        if(g_ppucBlockImages[nextshape->color])
        {
            GrImageDraw(pContext, g_ppucBlockImages[nextshape->color],
                        (long)sRect.sXMin, (long)sRect.sYMin);
        }
        else
        {
            GrContextForegroundSet(pContext, BACKGROUND_COLOR);
            GrRectFill(pContext, &sRect);
        }

        //
        // Get the offset of the next block.
        //
        if(ulLoop < 3)
        {
            iOff = nextshape->off[ulLoop];
        }
    }
}

//*****************************************************************************
//
// Draw a string with a white drop shadow at a give position on the display.
//
//*****************************************************************************
void
CenteredStringWithShadow(const tFont *pFont, char *pcString, short sX, short sY,
                         tBoolean bOpaque)
{
    GrContextFontSet(&g_sScreenContext, pFont);
    GrContextForegroundSet(&g_sScreenContext, SHADOW_COLOR);
    GrStringDrawCentered(&g_sScreenContext, pcString, -1, sX+1, sY+1, bOpaque);
    GrContextForegroundSet(&g_sScreenContext, HIGHLIGHT_COLOR);
    GrStringDrawCentered(&g_sScreenContext, pcString, -1, sX, sY, false);
}

//*****************************************************************************
//
// Paint the main area of the display while the game is not running.
//
//*****************************************************************************
void
OnStopAreaPaint(tWidget *pWidget, tContext *pContext)
{
    short sX, sY;
    char pcHighScore[32];

    //
    // Determine the position for the top line of text.
    //
    sX = GAME_AREA_LEFT + (GAME_AREA_WIDTH / 2);
    sY = GAME_AREA_TOP + 20;

    //
    // What we display depends upon the current game state.
    //
    switch(g_eGameState)
    {
        //
        // We are waiting for someone to press the "Start" button so just show
        // some very basic instructions.
        //
        case BLOX_WAITING:
        {
            //
            // Clear the canvas.
            //
            GrContextForegroundSet(pContext, BACKGROUND_COLOR);
            GrRectFill(pContext, &(pWidget->sPosition));

            //
            // Display some instructions
            //
            GrContextForegroundSet(pContext, TEXT_COLOR);
            GrContextFontSet(pContext, g_pFontCmss16);
            GrStringDrawCentered(pContext,
                                 "Guide the falling blocks so that they fit",
                                 -1, sX, sY, false);
            GrStringDrawCentered(pContext,
                                 "without leaving gaps. Full rows will be",
                                 -1, sX, sY + 15, false);
            GrStringDrawCentered(pContext,
                                "removed. Earn higher scores by dropping",
                                -1, sX, sY + 30, false);
            GrStringDrawCentered(pContext,
                                 "blocks from a greater height.",
                                 -1, sX, sY + 45, false);
            break;
        }

        //
        // The "Start" button has been pressed.  Show a countdown.
        //
        case BLOX_STARTING:
        {
            //
            // Clear the canvas.
            //
            GrContextForegroundSet(pContext, BACKGROUND_COLOR);
            GrRectFill(pContext, &(pWidget->sPosition));

            //
            // Draw the heading text.
            //
            CenteredStringWithShadow(g_pFontCmss20, "GAME STARTING IN",
                                     sX, sY + 26, false);

            //
            // Draw the countdown time.
            //
            UpdateCountdown(g_ulCountdown);

            break;
        }

        //
        // The game has just ended.  Show the high score.
        //
        case BLOX_GAME_OVER:
        {
            //
            // Write "GAME OVER" with a white shadow.
            //
            CenteredStringWithShadow(g_pFontCmss20, " GAME_OVER ", sX, sY, true);

            //
            // Format the high score string and display it.
            //
            usnprintf(pcHighScore, 32, " %s %d ",
                      g_bNewHighScore ? " NEW HIGH SCORE! " : "High Score",
                      g_iHighScore);
            CenteredStringWithShadow(g_pFontCmss18, pcHighScore, sX, sY + 30,
                                     true);
            break;
        }

        default:
            break;
    }

}

//*****************************************************************************
//
// Paint the blocks in the game play area.
//
//*****************************************************************************
void
OnGameAreaPaint(tWidget *pWidget, tContext *pContext)
{
    static const struct shape *psLastShape;
    static tBoolean bLastMsgShown = false;
    unsigned long ulIndex, ulRow, ulCol;
    tRectangle rectBlock;

    //
    // Update the score if necessary.
    //
    if (score != g_iCurScore)
    {
        usnprintf(g_pcScore, MAX_SCORE_LEN, "  %d  ", score);
        WidgetPaint((tWidget *)&g_sScore);
        g_iCurScore = score;
    }

    //
    // Draw a preview of the next shape if it is different
    //
    if (nextshape != psLastShape)
    {
        WidgetPaint((tWidget *)&g_sNextPiece);
        psLastShape = nextshape;
    }

    //
    // Has the screen just been cleared? If so, we fill the whole background
    // in a single operation rather than doing it cell-by-cell.
    //
    if(g_bScreenCleared)
    {
        //
        // Draw the logo into the background of the game area.
        //
        GrImageDraw(&g_sScreenContext, g_pucTILogo240x120,
                    GAME_AREA_LEFT, GAME_AREA_TOP);
    }

    //
    // Loop through the rows (which are actually columns in our implementation
    // since we've rotated the board).
    //
    for (ulRow = D_FIRST; ulRow < (D_LAST - 1); ulRow++)
    {
        //
        // Get the index of the first block in this row.
        //
        ulIndex = ulRow * B_COLS;

        //
        // Loop through all the displayed colums (rows in our implementation)
        //
        for (ulCol = 1; ulCol < (B_COLS - 1); ulCol++)
        {
            //
            // If the block on the screen is different from the one we have been
            // asked to draw, draw it.  We also force a redraw of everything if
            // we had a message displayed last time we repainted but it has
            // since been removed.
            //
            if ((board[ulIndex + ulCol] != g_psCurScreen[ulIndex + ulCol]) ||
               (bLastMsgShown && !g_bMsgShown))
            {
                //
                // Calculate the coordinates of this block.
                //
                rectBlock.sXMin = GAME_AREA_LEFT +
                                  ((ulRow - D_FIRST) * GAME_BLOCK_SIZE);
                rectBlock.sYMin = GAME_AREA_TOP +
                                  ((ulCol - 1) * GAME_BLOCK_SIZE);
                rectBlock.sXMax = rectBlock.sXMin + (GAME_BLOCK_SIZE - 1);
                rectBlock.sYMax = rectBlock.sYMin + (GAME_BLOCK_SIZE - 1);

                //
                // Remember what we are about to draw into the current cell.
                //
                g_psCurScreen[ulIndex + ulCol] = board[ulIndex + ulCol];

                //
                // Draw this block of the shape.  We use either an image if one
                // is defined or black otherwise.
                //
                if(g_ppucBlockImages[board[ulIndex + ulCol]])
                {
                    //
                    // If we just cleared the whole background, there is no
                    // need to reblit the background blocks.
                    //
                    if(!g_bScreenCleared)
                    {
                        GrImageDraw(&g_sScreenContext,
                                    g_ppucBlockImages[board[ulIndex + ulCol]],
                                    (long)rectBlock.sXMin,
                                    (long)rectBlock.sYMin);
                    }
                }
                else
                {
                    tRectangle sRect;

                    //
                    // Draw the block from the background image.  First save
                    // the existing clipping region.
                    //
                    sRect = g_sScreenContext.sClipRegion;

                    //
                    // Set the new clipping region to the block we are about
                    // to draw.
                    //
                    GrContextClipRegionSet(&g_sScreenContext, &rectBlock);

                    //
                    // Draw the background image into the clipping region.
                    //
                    GrImageDraw(&g_sScreenContext, g_pucTILogo240x120,
                                GAME_AREA_LEFT, GAME_AREA_TOP);

                    //
                    // Restore the previous clip region.
                    //
                    GrContextClipRegionSet(&g_sScreenContext, &sRect);
                }
            }
        }
    }

    //
    // If there is a message to show, draw it on top of the game area.
    //
    if(g_bMsgShown)
    {
        //
        // Determine the center of the game area.
        //
        rectBlock.sXMin = GAME_AREA_LEFT +
                          (GAME_BLOCK_SIZE * ((D_LAST - 1) - D_FIRST)) / 2;
        rectBlock.sYMin = GAME_AREA_TOP +
                          ((GAME_BLOCK_SIZE * (B_COLS - 2)) / 2);

        //
        // Render the required message centered in the game area.
        //
        GrContextFontSet(&g_sScreenContext, g_pFontCmss20b);
        GrContextForegroundSet(&g_sScreenContext, MESSAGE_COLOR);
        GrContextBackgroundSet(&g_sScreenContext, BACKGROUND_COLOR);
        GrStringDrawCentered(&g_sScreenContext, g_pcMessage, -1,
                             rectBlock.sXMin, rectBlock.sYMin, true);
    }

    //
    // Remember whether we displayed the message or not.
    //
    bLastMsgShown = g_bMsgShown;

    //
    // Clear the flag that we use to indicate the first redraw in a new game.
    //
    g_bScreenCleared = false;
}

//*****************************************************************************
//
// Update the high score if necessary then repaint the display to show the
// "Game Over" information.
//
//*****************************************************************************
void
GameOver(int iLastScore)
{
    //
    // See if the high score needs to be updated.
    //
    g_bNewHighScore = (iLastScore > g_iHighScore) ? true : false;
    g_iHighScore = g_bNewHighScore ? iLastScore : g_iHighScore;

    //
    // Update the score if necessary.
    //
    if (iLastScore != g_iCurScore)
    {
        usnprintf(g_pcScore, MAX_SCORE_LEN, "  %d  ", iLastScore);
        WidgetPaint((tWidget *)&g_sScore);
        g_iCurScore = iLastScore;
    }

    //
    // Update the display.
    //
    WidgetPaint((tWidget *)&g_sStoppedCanvas);
}

//*****************************************************************************
//
// Update the countdown text on the display prior to starting a new game.
//
//*****************************************************************************
void
UpdateCountdown(unsigned long ulCountdown)
{
    short sX, sY;
    char pcCountdown[4];

    //
    // Determine the position for the countdown text.
    //
    sX = GAME_AREA_LEFT + (GAME_AREA_WIDTH / 2);
    sY = GAME_AREA_TOP + 70;

    //
    // Render the countdown string.
    //
    usnprintf(pcCountdown, 4, " %d ", ulCountdown);

    //
    // Display it on the screen.
    //
    CenteredStringWithShadow(g_pFontCmss32b, pcCountdown, sX, sY, true);
}

//*****************************************************************************
//
// Initialize the game's block area in preparation for play.
//
//*****************************************************************************
void
scr_init(void)
{
    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sScreenContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Clear out the current screen buffer, filling it with an invalid
    // value to force a complete redraw on the first paint.
    //
    memset((char *)g_psCurScreen, -1, sizeof(g_psCurScreen));

    //
    // Set the flag indicating that the screen has just been cleared.
    //
    g_bScreenCleared = true;

    return;
}

//*****************************************************************************
//
// Display or remove a message string from the center of the game block area.
//
//*****************************************************************************
void
scr_msg(char *pcMsg, int iShow)
{
    //
    // Show or remove the message?
    //
    g_bMsgShown = iShow ? true : false;

    //
    // If showing a message, keep track of the string we are displaying.
    //
    if(iShow)
    {
        g_pcMessage = pcMsg;
    }

    //
    // Update the display.  This is done asynchronously rather than immediately
    // since adding and removing messages is not time critical.
    //
    WidgetPaint((tWidget *)&g_sGameCanvas);
}

//*****************************************************************************
//
// Update the game area of the display during play.
//
//*****************************************************************************
void
scr_update(void)
{
    OnGameAreaPaint((tWidget *)&g_sGameCanvas, &g_sScreenContext);
}

//*****************************************************************************
//
// Clear the data structure representing the screen.
//
//*****************************************************************************
void
scr_clear()
{
    memset((char *)g_psCurScreen, 0, sizeof(g_psCurScreen));
}
