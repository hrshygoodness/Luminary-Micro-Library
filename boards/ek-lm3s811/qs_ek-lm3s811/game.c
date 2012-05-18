//*****************************************************************************
//
// game.c - A "fly through the tunnel and shoot things" game.
//
// Copyright (c) 2006-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM3S811 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/uart.h"
#include "drivers/display96x16x1.h"
#include "game.h"
#include "globals.h"
#include "random.h"

//*****************************************************************************
//
// A bitmap for the "Press Button To Play" screen.  The bitmap is as follows:
//
//     xxx.........................xxx........x...x...............xxxxx.......
//     x..x........................x..x.......x...x.................x.........
//     x..x.x.xx..xx...xxx..xxx....x..x.x..x.xxx.xxx..xx..xxx.......x..xx.....
//     xxx..xx...x..x.x....x.......xxx..x..x..x...x..x..x.x..x......x.x..x....
//     x....x....xxxx..xx...xx.....x..x.x..x..x...x..x..x.x..x......x.x..x....
//     x....x....x.......x....x....x..x.x..x..x...x..x..x.x..x......x.x..x....
//     x....x.....xxx.xxx..xxx.....xxx...xxx...x...x..xx..x..x......x..xx.....
//     .......................................................................
//
// Continued...
//
//    xxx..x..........
//    x..x.x..........
//    x..x.x..xx..x..x
//    xxx..x.x..x.x..x
//    x....x.x..x.x..x
//    x....x.x..x..xxx
//    x....x..xxx....x
//    ............xxx.
//
//*****************************************************************************
static const unsigned char g_pucPlay[87] =
{
    0x7f, 0x09, 0x09, 0x06, 0x00, 0x7c, 0x08, 0x04, 0x04, 0x00, 0x38, 0x54,
    0x54, 0x58, 0x00, 0x48, 0x54, 0x54, 0x24, 0x00, 0x48, 0x54, 0x54, 0x24,
    0x00, 0x00, 0x00, 0x00, 0x7f, 0x49, 0x49, 0x36, 0x00, 0x3c, 0x40, 0x40,
    0x7c, 0x00, 0x04, 0x3f, 0x44, 0x00, 0x04, 0x3f, 0x44, 0x00, 0x38, 0x44,
    0x44, 0x38, 0x00, 0x7c, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x01, 0x7f, 0x01, 0x39, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, 0x7f,
    0x09, 0x09, 0x06, 0x00, 0x7f, 0x00, 0x38, 0x44, 0x44, 0x78, 0x00, 0x9c,
    0xa0, 0xa0, 0x7c
};

//*****************************************************************************
//
// A bitmap for the space ship.  The bitmap is as follows:
//
//     x....
//     xxx..
//     xxxxx
//
//*****************************************************************************
static const unsigned char g_pucShip[5] =
{
    0x07, 0x06, 0x06, 0x04, 0x04
};

//*****************************************************************************
//
// A bitmap for mine type one.  The bitmap is as follows:
//
//     .x.
//     xxx
//     .x.
//
//*****************************************************************************
static const unsigned char g_pucMine1[3] =
{
    0x02, 0x07, 0x02
};

//*****************************************************************************
//
// A bitmap for mine type two.  The bitmap is as follows:
//
//     x..x
//     .xx.
//     .xx.
//     x..x
//
//*****************************************************************************
static const unsigned char g_pucMine2[4] =
{
    0x09, 0x06, 0x06, 0x09
};

//*****************************************************************************
//
// A bitmap for the first stage of an explosion.  The bitmap is as follows:
//
//     x
//
//*****************************************************************************
static const unsigned char g_pucExplosion1[1] =
{
    0x01
};

//*****************************************************************************
//
// A bitmap for the second stage of an explosion.  The bitmap is as follows:
//
//     x.x
//     .x.
//     x.x
//
//*****************************************************************************
static const unsigned char g_pucExplosion2[3] =
{
    0x05, 0x02, 0x05
};

//*****************************************************************************
//
// A bitmap for the third stage of an explosion.  The bitmap is as follows:
//
//     x...x
//     .x.x.
//     ..x..
//     .x.x.
//     x...x
//
//*****************************************************************************
static const unsigned char g_pucExplosion3[5] =
{
    0x11, 0x0a, 0x04, 0x0a, 0x11
};

//*****************************************************************************
//
// A bitmap for the fourth stage of an explosion.  The bitmap is as follows:
//
//     x..x..x
//     .x.x.x.
//     ..x.x..
//     xx.x.xx
//     ..x.x..
//     .x.x.x.
//     x..x..x
//
//*****************************************************************************
static const unsigned char g_pucExplosion4[7] =
{
    0x49, 0x2a, 0x14, 0x6b, 0x14, 0x2a, 0x49
};

//*****************************************************************************
//
// This array contains the sequence of explosion images, along with the width
// of each one.
//
//*****************************************************************************
static const struct
{
    const unsigned char *pucImage;
    unsigned char ucAdjust;
    unsigned char ucWidth;
}
g_psExplosion[4] =
{
    { g_pucExplosion1, 0, 1 },
    { g_pucExplosion2, 1, 3 },
    { g_pucExplosion3, 2, 5 },
    { g_pucExplosion4, 3, 7 }
};

//*****************************************************************************
//
// Storage for the background image of the tunnel.  This is copied to the local
// frame buffer and then the other elements are overlaid upon it.
//
//*****************************************************************************
unsigned char g_pucBackground[192];

//*****************************************************************************
//
// The offsets from the top and bottom scan lines of the display to the wall of
// the tunnel.  The first element is the offset from the top scan line and the
// second element is the offfset from the bottom scan line.
//
//*****************************************************************************
static unsigned char g_pucOffset[2];

//*****************************************************************************
//
// An array of mines currently visible on the display.  Up to five mines can be
// displayed, and each has three variables associated with it: the type (in
// index zero), the horizontal position (in index one), and the vertical
// position (in index two).  If all three variables are negative one, then that
// mine does not exist.
//
//*****************************************************************************
static char g_pcMines[5][3];

//*****************************************************************************
//
// The location of the missile, if it has been fired.  The first entry contains
// the horizontal position and the second entry contains the vertical position.
// If both are negative one, then the missile has not been fired.
//
//*****************************************************************************
static char g_pcMissile[2];

//*****************************************************************************
//
// An array of explosions currently active on the display.  Up to five
// explosions can be displayed (the fifth being dedicated to the ship
// explosion), and each has three variables associated with it: the explosion
// step number (in index zero), the horizontal position (in index one), and the
// vertical position (in index two).  If the step number is negative one, then
// that explosion is not active.
//
//*****************************************************************************
static char g_pcExplosions[5][3];

//*****************************************************************************
//
// The point accumulated during the game.  One point is added for each time the
// display is scrolled to the left (i.e. the ship travels one step through the
// tunnel) and twenty-five points are added for each obstacle that is shot.
//
//*****************************************************************************
static unsigned long g_ulScore;

//*****************************************************************************
//
// Scroll the tunnel image one column to the left and add a new column of
// tunnel on the right side of the display.
//
//*****************************************************************************
static void
UpdateBackground(unsigned long ulGap)
{
    unsigned long ulCount, ulIdx;

    //
    // Loop through the array of mines.
    //
    for(ulIdx = 0; ulIdx < 5; ulIdx++)
    {
        //
        // Skip this mine if it is disabled.
        //
        if((g_pcMines[ulIdx][0] == (char)-1) &&
           (g_pcMines[ulIdx][1] == (char)-1) &&
           (g_pcMines[ulIdx][2] == (char)-1))
        {
            continue;
        }

        //
        // Stop searching if this mine is near or on the right side of the
        // display.
        //
        if(g_pcMines[ulIdx][1] > 91)
        {
            break;
        }
    }

    //
    // Get a random number based on the collected entropy.
    //
    ulCount = RandomNumber();

    //
    // If the top part of the tunnel is not at the top of the display, then
    // move it up 18.75% of the time.
    //
    if((ulCount < 0x30000000) && (g_pucOffset[0] != 0))
    {
        g_pucOffset[0]--;
    }

    //
    // If the top part of the tunnel is not too close to the bottom part of the
    // tunnel, and there is no mine on the right side of the display or the top
    // part of the tunnel is far enough away from the mine, then move it down
    // 18.75% of the time.
    //
    if((ulCount > 0xd0000000) && ((g_pucOffset[0] + ulGap) < g_pucOffset[1]) &&
       ((ulIdx == 5) || ((g_pcMines[ulIdx][2] - g_pucOffset[0]) > 1)))
    {
        g_pucOffset[0]++;
    }

    //
    // Get a new pseudo random number based on the original random number (no
    // new entropy will have been collected, so it will return the exact same
    // random number, which isn't so random).
    //
    ulCount = NEXT_RAND(ulCount);

    //
    // If the bottom part of the tunnel is not too close to the top part of the
    // tunnel, and there is no mine on the right side of the display or the
    // bottom part of the tunnel is far enough away from the mine, then move it
    // up 18.75% of the time.
    //
    if((ulCount < 0x30000000) && ((g_pucOffset[1] - ulGap) > g_pucOffset[0]) &&
       ((ulIdx == 5) || ((g_pucOffset[1] - g_pcMines[ulIdx][2]) > 5)))
    {
        g_pucOffset[1]--;
    }

    //
    // If the bottom part of the tunnel is not at the bottom of the display,
    // then move it down 18.75% of the time.
    //
    if((ulCount > 0xd0000000) && (g_pucOffset[1] != 16))
    {
        g_pucOffset[1]++;
    }

    //
    // Move the background image one column to the left.
    //
    for(ulCount = 0; ulCount < 192; ulCount++)
    {
        g_pucBackground[ulCount] = g_pucBackground[ulCount + 1];
    }

    //
    // Generate a new column on the right side of the background image.
    //
    g_pucBackground[95] = 0xff >> (8 - g_pucOffset[0]);
    g_pucBackground[191] = 0xff << (g_pucOffset[1] - 8);

    //
    // Copy the background image to the local frame buffer.
    //
    for(ulCount = 0; ulCount < 192; ulCount += 4)
    {
        *(unsigned long *)(g_pucFrame + ulCount) =
            *(unsigned long *)(g_pucBackground + ulCount);
    }
}

//*****************************************************************************
//
// Draws a image on the local frame buffer.
//
//*****************************************************************************
static void
DrawImage(const unsigned char *pucImage, long lX, long lY,
          unsigned long ulWidth)
{
    unsigned long ulIdx;

    //
    // Loop through the columns of this mine.
    //
    for(ulIdx = 0; ulIdx < ulWidth; ulIdx++)
    {
        //
        // See if this column is on the display.
        //
        if(((lX + (long)ulIdx) >= 0) && ((lX + ulIdx) < 96))
        {
            //
            // See if this mine is in the upper or lower row.
            //
            if(lY < 8)
            {
                //
                // Add this mine to the first row of the local frame buffer.
                // Part of the mine image may be in the second row as well, so
                // possibly add it there as well.
                //
                g_pucFrame[lX + ulIdx] |= pucImage[ulIdx] << lY;
                g_pucFrame[lX + ulIdx + 96] |= pucImage[ulIdx] >> (8 - lY);
            }
            else
            {
                //
                // Add this mine to the second row of the local frame buffer.
                //
                g_pucFrame[lX + ulIdx + 96] |= pucImage[ulIdx] << (lY - 8);
            }
        }
    }
}

//*****************************************************************************
//
// Update the mines in the tunnel.
//
//*****************************************************************************
static void
UpdateMines(void)
{
    unsigned long ulCount, ulIdx, ulMax;

    //
    // The maximum horizontal position of any mine found.
    //
    ulMax = 0;

    //
    // Loop through the five possible mines.
    //
    for(ulCount = 0; ulCount < 5; ulCount++)
    {
        //
        // Skip this mine if it does not exist.
        //
        if((g_pcMines[ulCount][0] == (char)-1) &&
           (g_pcMines[ulCount][1] == (char)-1) &&
           (g_pcMines[ulCount][2] == (char)-1))
        {
            continue;
        }

        //
        // Move the mine one step to the left (i.e. keep it in the same place
        // within the tunnel).
        //
        g_pcMines[ulCount][1]--;

        //
        // If the mine is too far off the left edge of the display then disable
        // it.
        //
        if(g_pcMines[ulCount][1] == (char)-4)
        {
            g_pcMines[ulCount][0] = (char)-1;
            g_pcMines[ulCount][1] = (char)-1;
            g_pcMines[ulCount][2] = (char)-1;
            continue;
        }

        //
        // See if this mine is further to the right than any other mine that
        // has been encountered thus far.
        //
        if((g_pcMines[ulCount][1] + 5) > ulMax)
        {
            //
            // Set the new maximal mine position.
            //
            ulMax = g_pcMines[ulCount][1] + 5;
        }

        //
        // See which type of mine this is.
        //
        if(g_pcMines[ulCount][0] == 0)
        {
            //
            // Draw mine type one on the local frame buffer.
            //
            DrawImage(g_pucMine1, g_pcMines[ulCount][1], g_pcMines[ulCount][2],
                      3);
        }
        else
        {
            //
            // Draw mine type two on the local frame buffer.
            //
            DrawImage(g_pucMine2, g_pcMines[ulCount][1], g_pcMines[ulCount][2],
                      4);
        }
    }

    //
    // If there are mines too close to the right side of the display then do
    // not place any new mines.
    //
    if(ulMax > 85)
    {
        return;
    }

    //
    // Get a random number.
    //
    ulIdx = RandomNumber();

    //
    // Only place new mines occasionally.
    //
    if(ulIdx >= 0x0c000000)
    {
        return;
    }

    //
    // Try to find an unused mine entry.
    //
    for(ulCount = 0; ulCount < 5; ulCount++)
    {
        if((g_pcMines[ulCount][0] == (char)-1) &&
           (g_pcMines[ulCount][1] == (char)-1) &&
           (g_pcMines[ulCount][2] == (char)-1))
        {
            break;
        }
    }

    //
    // If all five mines are already in use, then a new mine can not be added.
    //
    if(ulCount == 5)
    {
        return;
    }

    //
    // Choose a random mine type.
    //
    ulIdx = NEXT_RAND(ulIdx);
    g_pcMines[ulCount][0] = ulIdx >> 31;

    //
    // The mine starts at the right edge of the display.
    //
    g_pcMines[ulCount][1] = 95;

    //
    // Choose a random vertical position within the tunnel for the mine.
    //
    ulIdx = NEXT_RAND(ulIdx);
    g_pcMines[ulCount][2] = (g_pucOffset[0] + 1 +
                             (((g_pucOffset[1] - g_pucOffset[0] - 1 - 3 -
                                g_pcMines[ulCount][0]) *
                               (ulIdx >> 16)) >> 16));

    //
    // See which type of mine was choosen.
    //
    if(g_pcMines[ulCount][0] == 0)
    {
        //
        // Draw mine type one on the local frame buffer.
        //
        DrawImage(g_pucMine1, g_pcMines[ulCount][1], g_pcMines[ulCount][2], 3);
    }
    else
    {
        //
        // Draw mine type two on the local frame buffer.
        //
        DrawImage(g_pucMine2, g_pcMines[ulCount][1], g_pcMines[ulCount][2], 4);
    }
}

//*****************************************************************************
//
// Move the missile further to the right, checking for impacts.
//
//*****************************************************************************
static void
UpdateMissile(tBoolean bFire, tBoolean bDead)
{
    unsigned long ulBit, ulX;

    //
    // Set the X position to zero to indicate that no impact has been detected.
    //
    ulX = 0;

    //
    // See if a missile is currently in flight.
    //
    if((g_pcMissile[0] == (char)-1) && (g_pcMissile[1] == (char)-1))
    {
        //
        // No missile is in flight, so see if one should be fired.
        //
        if(bFire && !bDead)
        {
            //
            // Set the position of a newly fired missile
            //
            g_pcMissile[0] = 8;
            g_pcMissile[1] = (((1023 - g_ulWheel) * 14) / 1024) + 2;
        }
        else
        {
            //
            // No missile is in flight, and no missile is being fired, so do
            // nothing.
            //
            return;
        }
    }

    //
    // Move the missile to the right.
    //
    g_pcMissile[0] += 2;

    //
    // See if the missile has moved off the display.
    //
    if(g_pcMissile[0] == 96)
    {
        //
        // The missile is no longer on the display, so remove it from the
        // display.
        //
        g_pcMissile[0] = (char)-1;
        g_pcMissile[1] = (char)-1;

        //
        // Return without doing anything else.
        //
        return;
    }

    //
    // See if the missile is on the first row or the second row.
    //
    if(g_pcMissile[1] < 8)
    {
        //
        // Compute the bit that contains the missile.
        //
        ulBit = 1 << g_pcMissile[1];

        //
        // Draw the left most column of the missile and check for an impact.
        //
        g_pucFrame[g_pcMissile[0] + 0] ^= ulBit;
        if((g_pucFrame[g_pcMissile[0] + 0] & ulBit) != ulBit)
        {
            g_pucFrame[g_pcMissile[0] + 0] |= ulBit;
            ulX = g_pcMissile[0];
        }

        //
        // Draw the middle column of the missile and check for an impact.
        //
        g_pucFrame[g_pcMissile[0] + 1] ^= ulBit;
        if((g_pucFrame[g_pcMissile[0] + 1] & ulBit) != ulBit)
        {
            g_pucFrame[g_pcMissile[0] + 1] |= ulBit;
            if(ulX == 0)
            {
                ulX = g_pcMissile[0] + 1;
            }
        }

        //
        // Draw the right column of the missile and check for an impact.  The
        // right column may be off the display, so bypass the check in that
        // case.
        //
        if(g_pcMissile[0] != 94)
        {
            g_pucFrame[g_pcMissile[0] + 2] ^= ulBit;
            if((g_pucFrame[g_pcMissile[0] + 2] & ulBit) != ulBit)
            {
                g_pucFrame[g_pcMissile[0] + 2] |= ulBit;
                if(ulX == 0)
                {
                    ulX = g_pcMissile[0] + 2;
                }
            }
        }
    }
    else
    {
        //
        // Compute the bit that contains the missile.
        //
        ulBit = 1 << (g_pcMissile[1] - 8);

        //
        // Draw the left most column of the missile and check for an impact.
        //
        g_pucFrame[g_pcMissile[0] + 96] ^= ulBit;
        if((g_pucFrame[g_pcMissile[0] + 96] & ulBit) != ulBit)
        {
            g_pucFrame[g_pcMissile[0] + 96] |= ulBit;
            ulX = g_pcMissile[0];
        }

        //
        // Draw the middle column of the missile and check for an impact.
        //
        g_pucFrame[g_pcMissile[0] + 97] ^= ulBit;
        if((g_pucFrame[g_pcMissile[0] + 97] & ulBit) != ulBit)
        {
            g_pucFrame[g_pcMissile[0] + 97] |= ulBit;
            if(ulX == 0)
            {
                ulX = g_pcMissile[0] + 1;
            }
        }

        //
        // Draw the right column of the missile and check for an impact.  The
        // right column may be off the display, so bypass the check in that
        // case.
        //
        if(g_pcMissile[0] != 94)
        {
            g_pucFrame[g_pcMissile[0] + 98] ^= ulBit;
            if((g_pucFrame[g_pcMissile[0] + 98] & ulBit) != ulBit)
            {
                g_pucFrame[g_pcMissile[0] + 98] |= ulBit;
                if(ulX == 0)
                {
                    ulX = g_pcMissile[0] + 2;
                }
            }
        }
    }

    //
    // See if the missile hit something.
    //
    if(ulX != 0)
    {
        //
        // Loop through the mines.
        //
        for(ulBit = 0; ulBit < 5; ulBit++)
        {
            //
            // See if the missile hit this mine.
            //
            if((g_pcMines[ulBit][0] != (char)-1) &&
               (g_pcMines[ulBit][1] <= ulX) &&
               ((g_pcMines[ulBit][1] + g_pcMines[ulBit][0] + 2) >= ulX) &&
               (g_pcMines[ulBit][2] <= g_pcMissile[1]) &&
               ((g_pcMines[ulBit][2] + g_pcMines[ulBit][0] + 2) >=
                g_pcMissile[1]))
            {
                //
                // This mine was struck, so remove it from the display.
                //
                g_pcMines[ulBit][0] = (char)-1;
                g_pcMines[ulBit][1] = (char)-1;
                g_pcMines[ulBit][2] = (char)-1;

                //
                // Increase the player's score by 25 if they are not dead.
                //
                if(!bDead)
                {
                    g_ulScore += 25;
                }

                //
                // Stop looking through the mines.
                //
                break;
            }
        }

        //
        // Find an empty entry in the explosion list.
        //
        for(ulBit = 0; ulBit < 4; ulBit++)
        {
            if(g_pcExplosions[ulBit][0] == (char)-1)
            {
                break;
            }
        }

        //
        // See if an empty entry was found.
        //
        if(ulBit != 5)
        {
            //
            // Start an explosion at the point of impact.
            //
            g_pcExplosions[ulBit][0] = 0;
            g_pcExplosions[ulBit][1] = ulX;
            g_pcExplosions[ulBit][2] = g_pcMissile[1];
        }

        //
        // Remove the missile from the display.
        //
        g_pcMissile[0] = (char)-1;
        g_pcMissile[1] = (char)-1;
    }
}

//*****************************************************************************
//
// Draw the player's ship, checking for collisions.
//
//*****************************************************************************
static tBoolean
DrawShip(void)
{
    unsigned long ulPos, ulCount, ulBits;
    tBoolean bBoom;

    //
    // Convert the wheel position to the position of the ship on the display.
    //
    ulPos = ((1023 - g_ulWheel) * 14) / 1024;

    //
    // Assume no collisions until one is found.
    //
    bBoom = false;

    //
    // Loop through the five columns of the ship image.
    //
    for(ulCount = 0; ulCount < 5; ulCount++)
    {
        //
        // See if the ship image starts in the first or second row of the
        // display.
        //
        if(ulPos < 8)
        {
            //
            // Get the scan lines that should be set for the first row of the
            // display.
            //
            ulBits = (g_pucShip[ulCount] << ulPos) & 0xff;
            if(ulBits)
            {
                //
                // Set the scan lines in the local frame buffer and check for
                // an impact.
                //
                g_pucFrame[ulCount + 4] ^= ulBits;
                if((g_pucFrame[ulCount + 4] & ulBits) != ulBits)
                {
                    g_pucFrame[ulCount + 4] |= ulBits;
                    bBoom = true;
                }
            }

            //
            // Get the scan lines that should be set for the second row of the
            // display.
            //
            ulBits = g_pucShip[ulCount] >> (8 - ulPos);
            if(ulBits)
            {
                //
                // Set the scan lines in the local frame buffer and check for
                // an impact.
                //
                g_pucFrame[ulCount + 100] ^= ulBits;
                if((g_pucFrame[ulCount + 100] & ulBits) != ulBits)
                {
                    g_pucFrame[ulCount + 100] |= ulBits;
                    bBoom = true;
                }
            }
        }
        else
        {
            //
            // Get the scan lines that should be set for the second row of the
            // display.
            //
            ulBits = g_pucShip[ulCount] << (ulPos - 8);

            //
            // Set the scan lines in the local frame buffer and check for an
            // impact.
            //
            g_pucFrame[ulCount + 100] ^= ulBits;
            if((g_pucFrame[ulCount + 100] & ulBits) != ulBits)
            {
                g_pucFrame[ulCount + 100] |= ulBits;
                bBoom = true;
            }
        }
    }

    //
    // See if an impact occurred.
    //
    if(bBoom)
    {
        //
        // Start the ship explosion.
        //
        g_pcExplosions[4][0] = 0;
        g_pcExplosions[4][1] = 6;
        g_pcExplosions[4][2] = ulPos + 1;

        //
        // Indicate that the ship has exploded.
        //
        return(true);
    }
    else
    {
        //
        // The ship survived, so increment the score by one.
        //
        g_ulScore++;

        //
        // Indicate that the ship is still alive.
        //
        return(false);
    }
}

//*****************************************************************************
//
// Draws any active explosions.
//
//*****************************************************************************
static void
DrawExplosions(void)
{
    unsigned long ulCount, ulIdx;

    //
    // Loop through the explosion list.
    //
    for(ulCount = 0; ulCount < 5; ulCount++)
    {
        //
        // Skip this entry if it is not in use.
        //
        if(g_pcExplosions[ulCount][0] == (char)-1)
        {
            continue;
        }

        //
        // Get the index to the explosion type to display.
        //
        ulIdx = g_pcExplosions[ulCount][0] >> 2;

        //
        // For all except the last explosion (i.e. the ship explosion), move
        // the explosion to the left to match the movement of the tunnel.
        //
        if(ulCount != 4)
        {
            g_pcExplosions[ulCount][1]--;
        }

        //
        // Draw the explosion image into the local frame buffer.
        //
        DrawImage(g_psExplosion[ulIdx].pucImage,
                  g_pcExplosions[ulCount][1] - g_psExplosion[ulIdx].ucAdjust,
                  g_pcExplosions[ulCount][2] - g_psExplosion[ulIdx].ucAdjust,
                  g_psExplosion[ulIdx].ucWidth);

        //
        // Increment the explosion type counter.
        //
        g_pcExplosions[ulCount][0]++;

        //
        // If the explosion has completed, then remove it from the list.
        //
        if(g_pcExplosions[ulCount][0] == 16)
        {
            g_pcExplosions[ulCount][0] = (char)-1;
        }
    }
}

//*****************************************************************************
//
// The main screen of the game which waits for the user to press the button to
// begin the game.  If the button is not pressed soon enough, the screen saver
// will be called instead.
//
//*****************************************************************************
tBoolean
MainScreen(void)
{
    unsigned long ulCount, ulIdx;

    //
    // Set the top and bottom cave positions to the top and bottom of the
    // display.
    //
    g_pucOffset[0] = 0;
    g_pucOffset[1] = 16;

    //
    // Clear out the background buffer.
    //
    for(ulIdx = 0; ulIdx < 192; ulIdx += 4)
    {
        *(unsigned long *)(g_pucBackground + ulIdx) = 0;
    }

    //
    // Loop through the number of updates to the main screen to be done before
    // the screen saver is called instead.
    //
    for(ulCount = 0; ulCount < (2 * 60 * CLOCK_RATE / 4); ulCount++)
    {
        //
        // Wait until an update has been requested.
        //
        while(HWREGBITW(&g_ulFlags, FLAG_UPDATE) == 0)
        {
        }

        //
        // Clear the update request flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_UPDATE) = 0;

        //
        // See if the button has been pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS))
        {
            //
            // Clear the button press flag.
            //
            HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS) = 0;

            //
            // Return to the caller, indicating that the game should be played.
            //
            return(true);
        }

        //
        // Update the tunnel background, making sure that there are at least
        // thirteen scan lines between the top and bottom walls (providing room
        // for the "Press Button To Play" text).
        //
        UpdateBackground(13);

        //
        // Display the "Press Button To Play" text for sixteen frames every
        // sixteen frames, causing it to flash with a 50% duty cycle.
        //
        if(ulCount & 16)
        {
            //
            // Loop over the columns of the "Press Button To Play" image.
            //
            for(ulIdx = 0; ulIdx < sizeof(g_pucPlay); ulIdx++)
            {
                //
                // Copy this column of the image to the middle of the local
                // frame buffer.
                //
                g_pucFrame[ulIdx + 4] |= g_pucPlay[ulIdx] << 4;
                g_pucFrame[ulIdx + 100] |= g_pucPlay[ulIdx] >> 4;
            }
        }

        //
        // Display the updated image on the display.
        //
        Display96x16x1ImageDraw(g_pucFrame, 0, 0, 96, 2);
    }

    //
    // The button was not pressed so the screen saver should be invoked.
    //
    return(false);
}

//*****************************************************************************
//
// Plays the game.
//
//*****************************************************************************
void
PlayGame(void)
{
    unsigned long ulIdx;
    tBoolean bFire, bDead;
    char pcScore[6];

    //
    // Initialize the top and bottom wall of the tunnel to the top and bottom
    // of the display.
    //
    g_pucOffset[0] = 0;
    g_pucOffset[1] = 16;

    //
    // Turn off all the mines.
    //
    for(ulIdx = 0; ulIdx < 5; ulIdx++)
    {
        g_pcMines[ulIdx][0] = (char)-1;
        g_pcMines[ulIdx][1] = (char)-1;
        g_pcMines[ulIdx][2] = (char)-1;
    }

    //
    // The missile has not been fired.
    //
    g_pcMissile[0] = (char)-1;
    g_pcMissile[1] = (char)-1;

    //
    // Turn off all the explosions.
    //
    for(ulIdx = 0; ulIdx < 5; ulIdx++)
    {
        g_pcExplosions[ulIdx][0] = (char)-1;
    }

    //
    // Reset the score to zero.
    //
    g_ulScore = 0;

    //
    // The player is not dead yet.
    //
    bDead = false;

    //
    // Clear out the background buffer.
    //
    for(ulIdx = 0; ulIdx < 192; ulIdx += 4)
    {
        *(unsigned long *)(g_pucBackground + ulIdx) = 0;
    }

    //
    // Loop until the game is over.
    //
    while(1)
    {
        //
        // Wait until an update has been requested.
        //
        while(HWREGBITW(&g_ulFlags, FLAG_UPDATE) == 0)
        {
        }

        //
        // Clear the update request flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_UPDATE) = 0;

        //
        // See if the button has been pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS) == 1)
        {
            //
            // Clear the button press flag.
            //
            HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS) = 0;

            //
            // Indicate that the missile should be fired if possible.
            //
            bFire = true;
        }
        else
        {
            //
            // Indicate that the missile should not be fired.
            //
            bFire = false;
        }

        //
        // Update the tunnel.  The tunnel gets smaller as the score goes up.
        //
        UpdateBackground(13 - (g_ulScore / 2000));

        //
        // Update the position of the mines.
        //
        UpdateMines();

        //
        // Update the position of the missile, possibly firing it.
        //
        UpdateMissile(bFire, bDead);

        //
        // See if the player is dead.
        //
        if(!bDead)
        {
            //
            // Draw the ship on the display.
            //
            bDead = DrawShip();

            //
            // See if the ship just hit somthing.
            //
            if(bDead)
            {
                //
                // Set the delay counter to zero.
                //
                ulIdx = 0;
            }
        }

        //
        // Draw the active explosions.
        //
        DrawExplosions();

        //
        // Display the updated image on the display.
        //
        Display96x16x1ImageDraw(g_pucFrame, 0, 0, 96, 2);

        //
        // Write the current score to the UART.
        //
        UARTCharPut(UART0_BASE, '\r');
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 10000) % 10));
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 1000) % 10));
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 100) % 10));
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 10) % 10));
        UARTCharPut(UART0_BASE, '0' + (g_ulScore % 10));

        //
        // Check to see if the player is dead and the ship explosion has
        // completed.
        //
        if((g_pcExplosions[4][0] == (char)-1) && bDead)
        {
            //
            // Increment the delay counter.
            //
            ulIdx++;

            //
            // If a second has passed, then stop updating the tunnel and leave
            // the game.
            //
            if(ulIdx == (CLOCK_RATE / 4))
            {
                break;
            }
        }
    }

    //
    // Wait until an update has been requested.
    //
    while(HWREGBITW(&g_ulFlags, FLAG_UPDATE) == 0)
    {
    }

    //
    // Clear the update request flag.
    //
    HWREGBITW(&g_ulFlags, FLAG_UPDATE) = 0;

    //
    // Clear the display.
    //
    Display96x16x1Clear();

    //
    // Display the user's score.
    //
    Display96x16x1StringDraw("Score: ", 12, 1);
    pcScore[0] = '0' + ((g_ulScore / 10000) % 10);
    pcScore[1] = '0' + ((g_ulScore / 1000) % 10);
    pcScore[2] = '0' + ((g_ulScore / 100) % 10);
    pcScore[3] = '0' + ((g_ulScore / 10) % 10);
    pcScore[4] = '0' + (g_ulScore % 10);
    pcScore[5] = '\0';
    Display96x16x1StringDraw(pcScore, 54, 1);

    //
    // Loop for five seconds.
    //
    for(ulIdx = 0; ulIdx < (5 * CLOCK_RATE / 4); ulIdx++)
    {
        //
        // At the start of every second, draw "Game Over" on the display.
        //
        if((ulIdx % (CLOCK_RATE / 4)) == 0)
        {
            Display96x16x1StringDraw("Game Over", 21, 0);
        }

        //
        // At the half way point of every second, clear the "Game Over" from
        // the display.
        //
        if((ulIdx % (CLOCK_RATE / 4)) == (CLOCK_RATE / 4 / 2))
        {
            Display96x16x1StringDraw("         ", 21, 0);
        }

        //
        // Wait until an update has been requested.
        //
        while(HWREGBITW(&g_ulFlags, FLAG_UPDATE) == 0)
        {
        }

        //
        // Clear the update request flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_UPDATE) = 0;
    }
}
