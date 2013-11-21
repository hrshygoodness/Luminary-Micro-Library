//*****************************************************************************
//
// game.c - A "wander through a maze and shoot things" game.
//
// Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/uart.h"
#include "drivers/rit128x96x4.h"
#include "audio.h"
#include "can_net.h"
#include "enet.h"
#include "game.h"
#include "globals.h"
#include "images.h"
#include "random.h"
#include "sounds.h"

//*****************************************************************************
//
// The points accumulated during the game.
//
//*****************************************************************************
unsigned long g_ulScore;

//*****************************************************************************
//
// The position of the player within the maze.  This is specified in pixel
// coordinates, where each cell of the maze is 12x12 pixels.  Therefore, the
// range of the X coordinate is 0 to 1523, and the range of the Y coordinate is
// 0 to 1127.  Each coordinate must be an even number to avoid having to shift
// the image data in the X direction; the Y coordinate is also required to be
// even so that movement along each axis is at the same rate.
//
//*****************************************************************************
unsigned short g_usPlayerX;
unsigned short g_usPlayerY;

//*****************************************************************************
//
// The position of the monsters within the maze.  The positions are specified
// the same as the player position.  A monster position of 0,0 indicates a dead
// monster (since it is not possible for a monster to be at that position since
// it is a wall).
//
//*****************************************************************************
unsigned short g_pusMonsterX[100];
unsigned short g_pusMonsterY[100];

//*****************************************************************************
//
// The monster animation count, indicating the index into the animation
// sequence for each monster.
//
//*****************************************************************************
static unsigned char g_pucMonsterCount[100];

//*****************************************************************************
//
// The position of the bullets within the maze.  The positions are specified
// the same as the player position.  A bullet position of 0,0 indicates an
// unfired bullet (since it is not possible for a bullet to be at that position
// since it is a wall).
//
//*****************************************************************************
static unsigned short g_pusBulletX[4];
static unsigned short g_pusBulletY[4];

//*****************************************************************************
//
// The direction of travel for each of the active bullets.
//
//*****************************************************************************
static unsigned char g_pucBulletDir[4];

//*****************************************************************************
//
// The position of the explosions within the maze.  The positions are specified
// the same as the player position.  An explosion position of 0,0 indicates the
// explosion is not active (since it is not possible for an explosion to be at
// that position since it is a wall).
//
//*****************************************************************************
static unsigned short g_pusExplosionX[4];
static unsigned short g_pusExplosionY[4];

//*****************************************************************************
//
// The explosion animation count, indicating the index into the animation
// sequence for each explosion.
//
//*****************************************************************************
static unsigned char g_pucExplosionCount[4];

//*****************************************************************************
//
// The direction the player is facing.  When the lower nibble is non-zero, this
// indicates the direction that the player is walking.  If the lower nibble is
// zero, then the upper nibble indicates the direction that the player is
// facing.
//
//*****************************************************************************
static unsigned char g_ucDirection = 0;

//*****************************************************************************
//
// The player animation count, indicating the index into the animation sequence
// for the current direction that the player is facing and/or moving.
//
//*****************************************************************************
static unsigned char g_ucCount = 0;

//*****************************************************************************
//
// An array that contains a grid describing the walls and corridors of the
// maze.  Each entry contains the index of the glyph that is drawn in that cell
// of the maze.
//
//*****************************************************************************
char g_ppcMaze[94][127];

//*****************************************************************************
//
// An array containing the left grouping of cells, used to keep track of the
// cells that are connected via the current or previous row(s) of the maze.
//
//*****************************************************************************
static char g_pcLeft[43];

//*****************************************************************************
//
// An array containing the right grouping of cells, used to keep track of the
// cells that are connected via the current or previous row(s) of the maze.
//
//*****************************************************************************
static char g_pcRight[43];

//*****************************************************************************
//
// This function uses Eller's maze generation algorithm to generate a "perfect"
// maze.  A perfect maze is one in which there are no loops and no isolations
// (i.e. any point in the maze can be reached from any other point, and there
// is only one path between any two points in the maze).
//
//*****************************************************************************
static void
GenerateMaze(void)
{
    int iX, iY, iTemp;

    //
    // Choose a new random seed.
    //
    RandomSeed();

    //
    // Clear out the entire maze.
    //
    for(iY = 0; iY < 94; iY++)
    {
        for(iX = 0; iX < 127; iX++)
        {
            g_ppcMaze[iY][iX] = 0;
        }
    }

    //
    // Place walls along the top and bottom of the maze.
    //
    for(iX = 0; iX < 127; iX++)
    {
        g_ppcMaze[0][iX] = 1;
        g_ppcMaze[93][iX] = 1;
    }

    //
    // Place walls along the left and right of the maze.
    //
    for(iY = 0; iY < 94; iY++)
    {
        g_ppcMaze[iY][0] = 1;
        g_ppcMaze[iY][126] = 1;
    }

    //
    // Initialize the cell row data structure.
    //
    for(iX = 0; iX <= 42; iX++)
    {
        g_pcLeft[iX] = iX;
        g_pcRight[iX] = iX;
    }
    g_pcLeft[0] = 1;

    //
    // Loop through the rows of the maze.
    //
    for(iY = 1; iY < 31; iY++)
    {
        //
        // Loop through the cells of this row of the maze.
        //
        for(iX = 42; iX; iX--)
        {
            //
            // See if this cell can be connected to the cell to the right, and
            // if so, if it should be (i.e. based on randomness).
            //
            iTemp = g_pcLeft[iX - 1];
            if((iX != iTemp) && (RandomNumber() > (6 << 27)))
            {
                //
                // Update the row data structure to indicate that this cell is
                // connected to the cell to the right.
                //
                g_pcRight[iTemp] = g_pcRight[iX];
                g_pcLeft[(int)g_pcRight[iX]] = iTemp;
                g_pcRight[iX] = iX - 1;
                g_pcLeft[iX - 1] = iX;
            }
            else
            {
                //
                // This cell is not connected to the cell to the right, so
                // place a wall between the two cells.
                //
                g_ppcMaze[(iY * 3) - 0][(43 - iX) * 3] = 1;
                g_ppcMaze[(iY * 3) - 1][(43 - iX) * 3] = 1;
                g_ppcMaze[(iY * 3) - 2][(43 - iX) * 3] = 1;
                g_ppcMaze[(iY * 3) - 3][(43 - iX) * 3] = 1;
            }

            //
            // See if this cell can be connected to the cell below it, and if
            // so, if it should be (i.e. based on randomness).
            //
            iTemp = g_pcLeft[iX];
            if((iX != iTemp) && (RandomNumber() > (6 << 27)))
            {
                //
                // Update the row data structure to indicate that this cell is
                // connected to the cell below it.
                //
                g_pcRight[iTemp] = g_pcRight[iX];
                g_pcLeft[(int)g_pcRight[iX]] = iTemp;
                g_pcLeft[iX] = iX;
                g_pcRight[iX] = iX;

                //
                // Place a wall below this cell.
                //
                g_ppcMaze[iY * 3][((43 - iX) * 3) - 0] = 1;
                g_ppcMaze[iY * 3][((43 - iX) * 3) - 1] = 1;
                g_ppcMaze[iY * 3][((43 - iX) * 3) - 2] = 1;
                g_ppcMaze[iY * 3][((43 - iX) * 3) - 3] = 1;
            }
        }
    }

    //
    // Loop through the cells of the last row of the maze.
    //
    for(iX = 42; iX; iX--)
    {
        //
        // See if this cell can be connected to the cell to the right, and if
        // so, if it should be (i.e. if it is required in order to maintain the
        // connectedness of the maze or based on randomness).
        //
        iTemp = g_pcLeft[iX - 1];
        if((iX != iTemp) &&
           ((iX == g_pcRight[iX]) || (RandomNumber() < (6 << 27))))
        {
            //
            // Update the row data structure to indicate that this cell is
            // connected to the cell to the right.
            //
            g_pcRight[iTemp] = g_pcRight[iX];
            g_pcLeft[(int)g_pcRight[iX]] = iTemp;
            g_pcRight[iX] = iX - 1;
            g_pcLeft[iX - 1] = iX;
        }
        else
        {
            //
            // This cell is not connected to the cell to the right, so place a
            // wall between the two cells.
            //
            g_ppcMaze[(iY * 3) - 0][(43 - iX) * 3] = 1;
            g_ppcMaze[(iY * 3) - 1][(43 - iX) * 3] = 1;
            g_ppcMaze[(iY * 3) - 2][(43 - iX) * 3] = 1;
            g_ppcMaze[(iY * 3) - 3][(43 - iX) * 3] = 1;
        }

        //
        // Update the row data structure to indicate that this cell is not
        // connected to the cell below it (since this is the last row).
        //
        iTemp = g_pcLeft[iX];
        g_pcRight[iTemp] = g_pcRight[iX];
        g_pcLeft[(int)g_pcRight[iX]] = iTemp;
        g_pcLeft[iX] = iX;
        g_pcRight[iX] = iX;
    }

    //
    // Choose a random corner to remove from the maze to form the exit.
    //
    switch(RandomNumber() >> 30)
    {
        //
        // Put the exit in the upper left corner.
        //
        case 0:
        {
            //
            // Remove the wall from the top left corner of the maze.
            //
            if(RandomNumber() >> 31)
            {
                g_ppcMaze[0][1] = 0;
                g_ppcMaze[0][2] = 0;
            }
            else
            {
                g_ppcMaze[1][0] = 0;
                g_ppcMaze[2][0] = 0;
            }

            //
            // Done adding the exit.
            //
            break;
        }

        //
        // Put the exit in the upper right corner.
        //
        case 1:
        {
            //
            // Remove the wall from the top right corner of the maze.
            //
            if(RandomNumber() >> 31)
            {
                g_ppcMaze[1][126] = 0;
                g_ppcMaze[2][126] = 0;
            }
            else
            {
                g_ppcMaze[0][124] = 0;
                g_ppcMaze[0][125] = 0;
            }

            //
            // Done adding the exit.
            //
            break;
        }

        //
        // Put the exit in the lower right corner.
        //
        case 2:
        {
            //
            // Remove the wall from the bottom right corner of the maze.
            //
            if(RandomNumber() >> 31)
            {
                g_ppcMaze[93][124] = 0;
                g_ppcMaze[93][125] = 0;
            }
            else
            {
                g_ppcMaze[91][126] = 0;
                g_ppcMaze[92][126] = 0;
            }

            //
            // Done adding the exit.
            //
            break;
        }

        //
        // Put the exit in the lower left corner.
        //
        case 3:
        default:
        {
            //
            // Remove the wall from the bottom left corner of the maze.
            //
            if(RandomNumber() >> 31)
            {
                g_ppcMaze[91][0] = 0;
                g_ppcMaze[92][0] = 0;
            }
            else
            {
                g_ppcMaze[93][1] = 0;
                g_ppcMaze[93][2] = 0;
            }

            //
            // Done adding the exit.
            //
            break;
        }
    }

    //
    // The maze is now constructed, but the proper wall types need to be
    // selected.  Loop through the rows of the maze.
    //
    for(iY = 0; iY < 94; iY++)
    {
        //
        // Loop through the columns of the maze.
        //
        for(iX = 0; iX < 127; iX++)
        {
            //
            // Skip this cell if it does not contain a wall.
            //
            if(g_ppcMaze[iY][iX] == 0)
            {
                continue;
            }

            //
            // The cell type starts as zero until the adjacent wall segments
            // are found.  The wall graphics are arranged such that the zero
            // bit indicates a wall exists above this cell, the one bit
            // indicates a wall exists below this cell, the two bit indicates a
            // wall exists to the right of this cell, and the three bit
            // indicates a wall exists to the left of this cell.
            //
            iTemp = 0;

            //
            // See if there is a wall segment above this cell, and set the bit
            // mask if so.
            //
            if((iY != 0) && (g_ppcMaze[iY - 1][iX] != 0))
            {
                iTemp |= 1;
            }

            //
            // See if there is a wall segment below this cell, and set the bit
            // mask if so.
            //
            if((iY != 93) && (g_ppcMaze[iY + 1][iX] != 0))
            {
                iTemp |= 2;
            }

            //
            // See if there is a wall segment to the right of this cell, and
            // set the bit mask if so.
            //
            if((iX != 126) && (g_ppcMaze[iY][iX + 1] != 0))
            {
                iTemp |= 4;
            }

            //
            // See if there is a wall segment to the left of this cell, and set
            // the bit mask if so.
            //
            if((iX != 0) && (g_ppcMaze[iY][iX - 1] != 0))
            {
                iTemp |= 8;
            }

            //
            // Replace this cell of the maze with the appropriate value so that
            // the wall is drawn correctly.
            //
            g_ppcMaze[iY][iX] = iTemp;
        }
    }
}

//*****************************************************************************
//
// Draws the portion of the maze centered around the player.
//
//*****************************************************************************
static void
DrawMaze(void)
{
    long lXPos, lYPos, lX1, lY1, lX2, lY2, lXCell, lYCell, lXTemp;

    //
    // Find the upper left corner of the display based on the position of the
    // player.
    //
    lXPos = (long)g_usPlayerX - (64 - 6);
    lYPos = (long)g_usPlayerY - (47 - 6);

    //
    // Get the cell of the maze for the upper left corner of the display.
    //
    lXCell = lXPos / 12;
    lYCell = lYPos / 12;

    //
    // Get the screen position of the maze cell that is at the upper left of
    // the display.  Typically, these coordinates will be negative.
    //
    lXPos = (lXCell * 12) - lXPos;
    lYPos = (lYCell * 12) - lYPos;

    //
    // Loop over the maze cells that are visible (or partially visible) on the
    // vertical extent of the display.
    //
    for(lY1 = lYPos; lY1 < 94; lY1 += 12, lYCell++)
    {
        //
        // Do not draw this cell if it is not part of the maze.
        //
        if((lYCell < 0) || (lYCell > 93))
        {
            continue;
        }

        //
        // Get the starting maze cell for the left side of the display.
        //
        lXTemp = lXCell;

        //
        // Loop over the maze cells that are visible (or partially visible) on
        // the horizontal extent of the display.
        //
        for(lX1 = lXPos; lX1 < 128; lX1 += 12, lXTemp++)
        {
            //
            // Do not draw this column if it is not part of the maze.
            //
            if((lXTemp < 0) || (lXTemp > 126))
            {
                continue;
            }

            //
            // Loop over the scan lines of this maze cell that are visible on
            // the display.
            //
            for(lY2 = (lY1 < 0) ? -lY1 : 0; (lY2 < 12) && ((lY1 + lY2) < 94);
                lY2++)
            {
                //
                // Loop over the columns of this maze cell that are visible on
                // the display.
                //
                for(lX2 = (lX1 < 0) ? -lX1 : 0;
                    (lX2 < 12) && ((lX1 + lX2) < 128); lX2 += 2)
                {
                    //
                    // Copy the graphics data for this cell of the maze into
                    // the local frame buffer.
                    //
                    g_pucFrame[((lY1 + lY2) * 64) + ((lX1 + lX2) / 2)] =
                        g_ppucSprites[(unsigned long)g_ppcMaze[lYCell][lXTemp]]
                                     [(lY2 * 6) + (lX2 / 2)];
                }
            }
        }
    }
}

//*****************************************************************************
//
// Draws the entire maze on the display, providing a cheat mode.
//
//*****************************************************************************
static void
DrawCheat(void)
{
    long lX, lY;

    //
    // Loop over the 64 scan lines of the display.
    //
    for(lY = 0; lY < 94; lY++)
    {
        //
        // Loop over the 128 columns of the display.
        //
        for(lX = 0; lX < 127; lX += 2)
        {
            //
            // See if this is the last column pair of the display.
            //
            if(lX == 126)
            {
                //
                // Display the last column of the maze.
                //
                g_pucFrame[(lY * 64) + (lX / 2)] =
                    g_ppcMaze[lY][lX] ? 0xf0 : 0x00;
            }
            else
            {
                //
                // Display these two columns of the maze.
                //
                g_pucFrame[(lY * 64) + (lX / 2)] =
                    ((g_ppcMaze[lY][lX] ? 0xf0 : 0x00) |
                     (g_ppcMaze[lY][lX + 1] ? 0x0f : 0x00));
            }
        }
    }

    //
    // Loop over the seven rows surrounding the position of the player.
    //
    for(lY = (g_usPlayerY / 12) - 3; lY < (g_usPlayerY / 12) + 4; lY++)
    {
        //
        // If this row is not part of the maze then skip this row.
        //
        if(lY < 0)
        {
            continue;
        }

        //
        // Loop over the eleven columns surrounding the position of the player.
        //
        for(lX = (g_usPlayerX / 12) - 5; lX < (g_usPlayerX / 12) + 6; lX++)
        {
            //
            // If this column is not part of the maze then skip this column.
            //
            if(lX < 0)
            {
                continue;
            }

            //
            // See if this is an even or odd column of the display.
            //
            if(lX & 1)
            {
                //
                // This is an odd pixel, so if this pixel of the display is off
                // (i.e. there is no wall at this position) then make it grey.
                //
                if((g_pucFrame[(lY * 64) + (lX / 2)] & 0x0f) == 0)
                {
                    g_pucFrame[(lY * 64) + (lX / 2)] |= 0x06;
                }
            }
            else
            {
                //
                // This is an even pixel, so if this pixel of the dispaly is
                // off (i.e. there is no wall at this position) then make it
                // grey.
                //
                if((g_pucFrame[(lY * 64) + (lX / 2)] & 0xf0) == 0)
                {
                    g_pucFrame[(lY * 64) + (lX / 2)] |= 0x60;
                }
            }
        }
    }
}

//*****************************************************************************
//
// Moves the monsters towards the player, regardless of how far they are from
// the player within the maze (they can't move through the walls of course).
//
//*****************************************************************************
static void
MoveMonsters(void)
{
    unsigned long ulLoop, ulIdx;
    tBoolean bDown, bRight;
    long lX, lY;

    //
    // Loop through all the monsters.
    //
    for(ulLoop = 0; ulLoop < 100; ulLoop++)
    {
        //
        // Skip this monster if it is dead.
        //
        if((g_pusMonsterX[ulLoop] == 0) && (g_pusMonsterY[ulLoop] == 0))
        {
            continue;
        }

        //
        // Determine the distance between the player and the monster in both
        // coordinates.
        //
        lX = g_usPlayerX - g_pusMonsterX[ulLoop];
        lY = g_usPlayerY - g_pusMonsterY[ulLoop];

        //
        // See if the X coordinate is less than zero.
        //
        if(lX < 0)
        {
            //
            // The monster should move to the left.
            //
            bRight = false;

            //
            // Make the X coordinate be positive.
            //
            lX = -lX;
        }
        else
        {
            //
            // The monster should move to the right.
            //
            bRight = true;
        }

        //
        // See if the Y coordinate is less than zero.
        //
        if(lY < 0)
        {
            //
            // The monster should move up.
            //
            bDown = false;

            //
            // Make the Y coordinate be positive.
            //
            lY = -lY;
        }
        else
        {
            //
            // The monster should move down.
            //
            bDown = true;
        }

        //
        // Try two times to move the monster.  The first attempt will be along
        // the axis that the monster is furthest from the player, and the
        // second attempt will be along he shorter axis.
        //
        for(ulIdx = 0; ulIdx < 2; ulIdx++)
        {
            //
            // See if the monster is further away on the X or Y axis.
            //
            if(lX > lY)
            {
                //
                // The monster is further away on the X axis.  See if it should
                // move to the left or right.
                //
                if(bRight && (lX != 0))
                {
                    //
                    // See if there is a wall blocking the monster's movement
                    // to the right.
                    //
                    if((g_ppcMaze[g_pusMonsterY[ulLoop] / 12]
                                 [(g_pusMonsterX[ulLoop] + 12) / 12] == 0) &&
                       (g_ppcMaze[(g_pusMonsterY[ulLoop] + 11) / 12]
                                 [(g_pusMonsterX[ulLoop] + 12) / 12] == 0))
                    {
                        //
                        // There is no wall obstructing the monster, so move it
                        // to the right.
                        //
                        g_pusMonsterX[ulLoop] += 2;

                        //
                        // The monster has been moved, so do not attempt to
                        // move it further.
                        //
                        break;
                    }
                }
                else if(lX != 0)
                {
                    //
                    // See if there is a wall blocking the monster's movement
                    // to the left.
                    //
                    if((g_ppcMaze[g_pusMonsterY[ulLoop] / 12]
                                 [(g_pusMonsterX[ulLoop] - 2) / 12] == 0) &&
                       (g_ppcMaze[(g_pusMonsterY[ulLoop] + 11) / 12]
                                 [(g_pusMonsterX[ulLoop] - 2) / 12] == 0))
                    {
                        //
                        // There is no wall obstructing the monster, so move it
                        // to the left.
                        //
                        g_pusMonsterX[ulLoop] -= 2;

                        //
                        // The monster has been moved, so do not attempt to
                        // move it further.
                        //
                        break;
                    }
                }

                //
                // Make the X axis difference be zero so that the next move
                // attempt will be along the Y axis.
                //
                lX = 0;
            }
            else
            {
                //
                // The monster is further away on the Y axis.  See if it should
                // be moved up or down.
                //
                if(bDown && (lY != 0))
                {
                    //
                    // See if there is a wall blocking the monster's movement
                    // down.
                    //
                    if((g_ppcMaze[(g_pusMonsterY[ulLoop] + 12) / 12]
                                 [g_pusMonsterX[ulLoop] / 12] == 0) &&
                       (g_ppcMaze[(g_pusMonsterY[ulLoop] + 12) / 12]
                                 [(g_pusMonsterX[ulLoop] + 11) / 12] == 0))
                    {
                        //
                        // There is no wall obstructing the monster, so move it
                        // down.
                        //
                        g_pusMonsterY[ulLoop] += 2;

                        //
                        // The monster has been moved, so do not attempt to
                        // move it further.
                        //
                        break;
                    }
                }
                else if(lY != 0)
                {
                    //
                    // See if there is a wall blocking the monster's movement
                    // up.
                    //
                    if((g_ppcMaze[(g_pusMonsterY[ulLoop] - 2) / 12]
                                 [g_pusMonsterX[ulLoop] / 12] == 0) &&
                       (g_ppcMaze[(g_pusMonsterY[ulLoop] - 2) / 12]
                                 [(g_pusMonsterX[ulLoop] + 11) / 12] == 0))
                    {
                        //
                        // There is no wall obstructing the monster, so move it
                        // up.
                        //
                        g_pusMonsterY[ulLoop] -= 2;

                        //
                        // The monster has been moved, so do not attempt to
                        // move it further.
                        //
                        break;
                    }
                }

                //
                // Make the Y axis difference be zero so that the next move
                // attempt will be along the X axis.
                //
                lY = 0;
            }
        }
    }
}

//*****************************************************************************
//
// Draws the visible monsters onto the display.
//
//*****************************************************************************
static void
DrawMonsters(void)
{
    long lMonsterX, lMonsterY, lX, lY;
    unsigned char ucData1, ucData2;
    unsigned long ulLoop, ulIdx;

    //
    // Loop through all the monsters.
    //
    for(ulLoop = 0; ulLoop < 100; ulLoop++)
    {
        //
        // Skip this monster if it is dead.
        //
        if((g_pusMonsterX[ulLoop] == 0) && (g_pusMonsterY[ulLoop] == 0))
        {
            continue;
        }

        //
        // Increment the monster animation count.
        //
        g_pucMonsterCount[ulLoop]++;

        //
        // Find the index into the animation sequence based on the current
        // animation count.
        //
        for(ulIdx = 0; ulIdx < sizeof(g_pucMonster); ulIdx += 2)
        {
            if(g_pucMonster[ulIdx] > g_pucMonsterCount[ulLoop])
            {
                break;
            }
        }

        //
        // See if the animation sequence has been completed.
        //
        if(ulIdx == sizeof(g_pucMonster))
        {
            //
            // Reset the animation count.
            //
            g_pucMonsterCount[ulLoop] = 0;

            //
            // Use the first image in the animation sequence.
            //
            ulIdx = g_pucMonster[1];
        }
        else
        {
            //
            // Otherwise, use the most recent image in the animation sequence.
            //
            ulIdx = g_pucMonster[ulIdx - 1];
        }

        //
        // Find the position of this monster on the display.
        //
        lMonsterX = g_pusMonsterX[ulLoop] - (g_usPlayerX - (64 - 6));
        lMonsterY = g_pusMonsterY[ulLoop] - (g_usPlayerY - (47 - 6));

        //
        // Skip this monster if it is not on the display.
        //
        if((lMonsterX < -12) || (lMonsterX > 128) ||
           (lMonsterY < -12) || (lMonsterY > 94))
        {
            continue;
        }

        //
        // Loop through the scan lines of this monster.
        //
        for(lY = (lMonsterY < 0) ? -lMonsterY : 0;
            (lY < 12) && ((lMonsterY + lY) < 94); lY++)
        {
            //
            // Loop through the columns of this monster.
            //
            for(lX = (lMonsterX < 0) ? -lMonsterX : 0;
                (lX < 12) && ((lMonsterX + lX) < 128); lX++)
            {
                //
                // Copy the sprite data into the local frame buffer.
                //
                ucData1 = g_pucFrame[((lMonsterY + lY) * 64) +
                                     ((lMonsterX + lX) / 2)];
                ucData2 = g_ppucSprites[ulIdx][(lY * 6) + (lX / 2)];
                if((ucData2 & 0xf0) != 0x00)
                {
                    ucData1 &= 0x0f;
                }
                if((ucData2 & 0x0f) != 0x00)
                {
                    ucData1 &= 0xf0;
                }
                g_pucFrame[((lMonsterY + lY) * 64) + ((lMonsterX + lX) / 2)] =
                    ucData1 | ucData2;
            }
        }
    }
}

//*****************************************************************************
//
// Adds a new explosion.
//
//*****************************************************************************
static void
AddExplosion(unsigned long ulX, unsigned long ulY)
{
    unsigned long ulLoop;

    //
    // Loop through all the explosions looking for an inactive explosion.
    //
    for(ulLoop = 0; ulLoop < 4; ulLoop++)
    {
        if((g_pusExplosionX[ulLoop] == 0) && (g_pusExplosionY[ulLoop] == 0))
        {
            break;
        }
    }

    //
    // See if an inactive explosion was found.
    //
    if(ulLoop != 4)
    {
        //
        // Add the new explosion.
        //
        g_pusExplosionX[ulLoop] = ulX;
        g_pusExplosionY[ulLoop] = ulY;

        //
        // Start the explosion animation from the beginning.
        //
        g_pucExplosionCount[ulLoop] = 0;
    }
}

//*****************************************************************************
//
// Sets the direction that the player's bullet will be fired.
//
//*****************************************************************************
static void
SetBulletDir(unsigned long ulBullet)
{
    unsigned long ulLoop, ulDist, ulMin, ulIdx;
    long lX, lY;

    //
    // The bullet will be fired towards the nearest monster.  Loop through the
    // monsters to find the one that is closest.
    //
    for(ulLoop = 0, ulMin = 0xffffffff, ulIdx = 0; ulLoop < 100; ulLoop++)
    {
        //
        // Do not consider this monster if it is already dead.
        //
        if((g_pusMonsterX[ulLoop] == 0) && (g_pusMonsterY[ulLoop] == 0))
        {
            continue;
        }

        //
        // Get the distance along the X and Y axis to this monster.
        //
        lX = (long)g_pusMonsterX[ulLoop] - (long)g_usPlayerX;
        lY = (long)g_pusMonsterY[ulLoop] - (long)g_usPlayerY;

        //
        // Get the diagonal distance to this monster.  The square root is
        // omitted to speed computation (and since the actual distance is not
        // required).
        //
        ulDist = (lX * lX) + (lY * lY);

        //
        // See if this monster is closer than the previously found closest
        // monster.
        //
        if(ulDist < ulMin)
        {
            //
            // This is now the closest monster found.
            //
            ulMin = ulDist;
            ulIdx = ulLoop;
        }
    }

    //
    // See if a monster was found.
    //
    if(ulMin == 0xffffffff)
    {
        //
        // There are no monsters left in the maze, so fire in the direction
        // that the player is facing.
        //
        g_pucBulletDir[ulBullet] = ((g_ucDirection & 0x0f) |
                                    (g_ucDirection >> 4));
        if(g_pucBulletDir[ulBullet] == 0)
        {
            g_pucBulletDir[ulBullet] = 2;
        }
    }
    else
    {
        //
        // Get the distance along the X and Y axis to the monster.
        //
        lX = (long)g_pusMonsterX[ulIdx] - (long)g_usPlayerX;
        lY = (long)g_pusMonsterY[ulIdx] - (long)g_usPlayerY;

        //
        // See if the monster is closer along the X or Y axis.  The square is
        // used to make both values be positive (as opposed to doing an
        // absolute value).  This is actually faster than an absolute value
        // since multiply is single cycle whereas an absolute value will
        // require a compare and conditional execution.
        //
        if((lX * lX) > (lY * lY))
        {
            //
            // The monster is closer along the Y axis, so fire along the X
            // axis.  Determine if the monster is to the left or right.
            //
            if(lX < 0)
            {
                //
                // The monster is to the left, so fire to the left.
                //
                g_pucBulletDir[ulBullet] = 4;
            }
            else
            {
                //
                // The monster is to the right, so fire to the right.
                //
                g_pucBulletDir[ulBullet] = 8;
            }
        }
        else
        {
            //
            // The monster is closer along the X axis, so fire along the Y
            // axis.  Determine if the monster is above or below.
            //
            if(lY < 0)
            {
                //
                // The monster is above, so fire up.
                //
                g_pucBulletDir[ulBullet] = 1;
            }
            else
            {
                //
                // The monster is below, so fire down.
                //
                g_pucBulletDir[ulBullet] = 2;
            }
        }
    }
}

//*****************************************************************************
//
// Draws the bullets onto the display.
//
//*****************************************************************************
static void
DrawBullets(tBoolean bFire)
{
    unsigned long ulLoop, ulIdx;
    tBoolean bHit;
    long lX, lY;

    //
    // See if a new bullet should be fired.
    //
    if(bFire)
    {
        //
        // Loop through the bullets looking for one that hasn't been fired.
        //
        for(ulLoop = 0; ulLoop < 4; ulLoop++)
        {
            if((g_pusBulletX[ulLoop] == 0) && (g_pusBulletY[ulLoop] == 0))
            {
                break;
            }
        }

        //
        // See if an unfired bullet was found.
        //
        if(ulLoop < 4)
        {
            //
            // Play the bullet firing sound.
            //
            AudioPlaySound(g_pusFireEffect, sizeof(g_pusFireEffect) / 2);

            //
            // Send a message to flash the LED on the CAN device board.
            //
            CANUpdateTargetLED(1, true);

            //
            // Set the direction for this bullet.
            //
            SetBulletDir(ulLoop);

            //
            // Set the initial position of the bullet based on the direction.
            //
            switch(g_pucBulletDir[ulLoop])
            {
                //
                // The bullet is being fired up.
                //
                case 1:
                {
                    //
                    // Set the position of the bullet at the top of the player.
                    //
                    g_pusBulletX[ulLoop] = g_usPlayerX + 4;
                    g_pusBulletY[ulLoop] = g_usPlayerY;

                    //
                    // This bullet has been positioned.
                    //
                    break;
                }

                //
                // The bullet is being fired down.
                //
                case 2:
                {
                    //
                    // Set the position of the bullet at the bottom of the
                    // player.
                    //
                    g_pusBulletX[ulLoop] = g_usPlayerX + 4;
                    g_pusBulletY[ulLoop] = g_usPlayerY + 8;

                    //
                    // This bullet has been positioned.
                    //
                    break;
                }

                //
                // The bullet is being fired left.
                //
                case 4:
                {
                    //
                    // Set the position of the bullet at the left of the
                    // player.
                    //
                    g_pusBulletX[ulLoop] = g_usPlayerX;
                    g_pusBulletY[ulLoop] = g_usPlayerY + 6;

                    //
                    // This bullet has been positioned.
                    //
                    break;
                }

                //
                // The bullet is being fired right.
                //
                case 8:
                {
                    //
                    // Set the position of the bullet at the right of the
                    // player.
                    //
                    g_pusBulletX[ulLoop] = g_usPlayerX + 8;
                    g_pusBulletY[ulLoop] = g_usPlayerY + 6;

                    //
                    // This bullet has been positioned.
                    //
                    break;
                }
            }
        }
    }

    //
    // Loop through the four bullets.
    //
    for(ulLoop = 0; ulLoop < 4; ulLoop++)
    {
        //
        // Skip this bullet if it has not been fired.
        //
        if((g_pusBulletX[ulLoop] == 0) && (g_pusBulletY[ulLoop] == 0))
        {
            continue;
        }

        //
        // Determine the direction this bullet was fired.
        //
        switch(g_pucBulletDir[ulLoop])
        {
            //
            // This bullet was fired up.
            //
            case 1:
            {
                //
                // Move the bullet up.
                //
                g_pusBulletY[ulLoop] -= 4;

                //
                // See if the bullet has struck a wall.
                //
                if((g_pusBulletY[ulLoop] < 1128) &&
                   (g_ppcMaze[g_pusBulletY[ulLoop] / 12]
                             [g_pusBulletX[ulLoop] / 12] != 0))
                {
                    //
                    // Add an explosion at the point the bullet struck the
                    // wall.
                    //
                    AddExplosion(g_pusBulletX[ulLoop] - 6,
                                 ((g_pusBulletY[ulLoop] / 12) * 12) + 6);

                    //
                    // Play the sound of the wall being struck.
                    //
                    AudioPlaySound(g_pusWallEffect,
                                   sizeof(g_pusWallEffect) / 2);

                    //
                    // The bullet is now unfired.
                    //
                    g_pusBulletX[ulLoop] = 0;
                    g_pusBulletY[ulLoop] = 0;
                }

                //
                // Done moving this bullet.
                //
                break;
            }

            //
            // This bullet was fired down.
            //
            case 2:
            {
                //
                // Move the bullet down.
                //
                g_pusBulletY[ulLoop] += 4;

                //
                // See if the bullet has struck a wall.
                //
                if((g_pusBulletY[ulLoop] < 1125) &&
                   (g_ppcMaze[(g_pusBulletY[ulLoop] + 3) / 12]
                             [g_pusBulletX[ulLoop] / 12] != 0))
                {
                    //
                    // Add an explosion at the point the bullet struck the
                    // wall.
                    //
                    AddExplosion(g_pusBulletX[ulLoop] - 6,
                                 (((g_pusBulletY[ulLoop] - 4) / 12) * 12) + 6);

                    //
                    // Play the sound of the wall being struck.
                    //
                    AudioPlaySound(g_pusWallEffect,
                                   sizeof(g_pusWallEffect) / 2);

                    //
                    // The bullet is now unfired.
                    //
                    g_pusBulletX[ulLoop] = 0;
                    g_pusBulletY[ulLoop] = 0;
                }

                //
                // Done moving this bullet.
                //
                break;
            }

            //
            // This bullet was fired left.
            //
            case 4:
            {
                //
                // Move the bullet left.
                //
                g_pusBulletX[ulLoop] -= 4;

                //
                // See if the bullet has struck a wall.
                //
                if((g_pusBulletX[ulLoop] < 1524) &&
                   (g_ppcMaze[g_pusBulletY[ulLoop] / 12]
                             [g_pusBulletX[ulLoop] / 12] != 0))
                {
                    //
                    // Add an explosion at the point the bullet struck the
                    // wall.
                    //
                    AddExplosion(((g_pusBulletX[ulLoop] / 12) * 12) + 6,
                                 g_pusBulletY[ulLoop] - 6);

                    //
                    // Play the sound of the wall being struck.
                    //
                    AudioPlaySound(g_pusWallEffect,
                                   sizeof(g_pusWallEffect) / 2);

                    //
                    // The bullet is now unfired.
                    //
                    g_pusBulletX[ulLoop] = 0;
                    g_pusBulletY[ulLoop] = 0;
                }

                //
                // Done moving this bullet.
                //
                break;
            }

            //
            // This bullet was fired right.
            //
            case 8:
            {
                //
                // Move the bullet right.
                //
                g_pusBulletX[ulLoop] += 4;

                //
                // See if the bullet has struck a wall.
                //
                if((g_pusBulletX[ulLoop] < 1521) &&
                   (g_ppcMaze[g_pusBulletY[ulLoop] / 12]
                             [(g_pusBulletX[ulLoop] + 3) / 12] != 0))
                {
                    //
                    // Add an explosion at the point the bullet struck the
                    // wall.
                    //
                    AddExplosion((((g_pusBulletX[ulLoop] - 4) / 12) * 12) + 6,
                                 g_pusBulletY[ulLoop] - 6);

                    //
                    // Play the sound of the wall being struck.
                    //
                    AudioPlaySound(g_pusWallEffect,
                                   sizeof(g_pusWallEffect) / 2);

                    //
                    // The bullet is now unfired.
                    //
                    g_pusBulletX[ulLoop] = 0;
                    g_pusBulletY[ulLoop] = 0;
                }

                //
                // Done moving this bullet.
                //
                break;
            }
        }

        //
        // If the bullet is no longer within the maze (i.e. it was fired
        // through the maze exit) then remove it from the display and make it
        // be unfired.
        //
        if((g_pusBulletX[ulLoop] > 1524) || (g_pusBulletY[ulLoop] > 1128))
        {
            g_pusBulletX[ulLoop] = 0;
            g_pusBulletY[ulLoop] = 0;
        }

        //
        // See if this bullet needs to be drawn.
        //
        if((g_pusBulletX[ulLoop] != 0) || (g_pusBulletY[ulLoop] != 0))
        {
            //
            // Until a collision is found, assume there is no collision.
            //
            bHit = false;

            //
            // Get the position on the display where the bullet should be
            // drawn.
            //
            lX = (long)g_pusBulletX[ulLoop] - (long)g_usPlayerX + (64 - 6);
            lY = (long)g_pusBulletY[ulLoop] - (long)g_usPlayerY + (47 - 6);

            //
            // See if the bullet is completely off the display.
            //
            if((lX < -3) || (lX > 127) || (lY < -3) || (lY > 93))
            {
                //
                // Make this bullet be unfired since it is no longer on the
                // display.
                //
                g_pusBulletX[ulLoop] = 0;
                g_pusBulletY[ulLoop] = 0;
            }

            //
            // Otherwise, see if the bullet is fired vertically.
            //
            else if((g_pucBulletDir[ulLoop] & 0x03) != 0)
            {
                //
                // Loop through the four rows of the bullet.
                //
                for(ulIdx = (lY < 0) ? -lY : 0, lY = (lY < 0) ? 0 : lY;
                    (ulIdx < 4) && (lY < 94); ulIdx++, lY++)
                {
                    //
                    // See if this pixel is already set.
                    //
                    if((g_pucFrame[(lY * 64) + (lX / 2)] & 0x0f) != 0)
                    {
                        //
                        // Indicate that a collision has occurred.
                        //
                        bHit = true;
                    }

                    //
                    // Draw this pixel on the display.
                    //
                    g_pucFrame[(lY * 64) + (lX / 2)] |= 0x0f;
                }
            }

            //
            // Otherwise, the bullet is fired horizontally.
            //
            else
            {
                //
                // Loop through the four columns of the bullet.
                //
                for(ulIdx = (lX < 0) ? -lX : 0, lX = (lX < 0) ? 0 : lX;
                    (ulIdx < 2) && (lX < 128); ulIdx++, lX += 2)
                {
                    //
                    // See if either of these pixels is already set.
                    //
                    if(g_pucFrame[(lY * 64) + (lX / 2)] != 0)
                    {
                        //
                        // Indicate that a collision has occurred.
                        //
                        bHit = true;
                    }

                    //
                    // Draw these pixel on the display.
                    //
                    g_pucFrame[(lY * 64) + (lX / 2)] |= 0xff;
                }
            }

            //
            // See if a collision has occurred.
            //
            if(bHit)
            {
                //
                // Play the sound effect for a monster dying.
                //
                AudioPlaySound(g_pusMonsterEffect,
                               sizeof(g_pusMonsterEffect) / 2);

                //
                // Add one hundred points to the score.
                //
                g_ulScore += 100;

                //
                // See if this bullet was travelling vertically or horizontally
                // and determine the adjustment made to the top and left edge
                // of the monster position to account for the fact that the
                // bullet is not a single point.
                //
                if((g_pucBulletDir[ulLoop] & 0x03) != 0)
                {
                    //
                    // The bullet was travelling vertically.  Set the monster
                    // position adjust accordingly.
                    //
                    lX = 0;
                    lY = -3;
                }
                else
                {
                    //
                    // The bullet was travelling horizontally.  Set the monster
                    // position adjust accordingly.
                    //
                    lX = -3;
                    lY = 0;
                }

                //
                // Loop over all the monsters.
                //
                for(ulIdx = 0; ulIdx < 100; ulIdx++)
                {
                    //
                    // Skip this monster if it is already dead.
                    //
                    if((g_pusMonsterX[ulIdx] == 0) &&
                       (g_pusMonsterY[ulIdx] == 0))
                    {
                        continue;
                    }

                    //
                    // See if the bullet hit this monster.
                    //
                    if(((g_pusMonsterX[ulIdx] + lX) < g_pusBulletX[ulLoop]) &&
                       ((g_pusMonsterX[ulIdx] + 12) > g_pusBulletX[ulLoop]) &&
                       ((g_pusMonsterY[ulIdx] + lY) < g_pusBulletY[ulLoop]) &&
                       ((g_pusMonsterY[ulIdx] + 12) > g_pusBulletY[ulLoop]))
                    {
                        //
                        // Add an explosion at the position of the monster.
                        //
                        AddExplosion(g_pusMonsterX[ulIdx],
                                     g_pusMonsterY[ulIdx]);

                        //
                        // Indicate that this monster is now dead.
                        //
                        g_pusMonsterX[ulIdx] = 0;
                        g_pusMonsterY[ulIdx] = 0;

                        //
                        // Stop looking for monsters.
                        //
                        break;
                    }
                }

                //
                // This bullet is no longer active.
                //
                g_pusBulletX[ulLoop] = 0;
                g_pusBulletY[ulLoop] = 0;
            }
        }
    }
}

//*****************************************************************************
//
// Draws the player in the middle of the display, handling the animation of the
// player based on its motion and direction.
//
//*****************************************************************************
static tBoolean
DrawPlayer(unsigned char ucDirection)
{
    unsigned long ulLoop, ulIdx, ulNum;
    const unsigned char *pucAnimation;
    unsigned char ucData1, ucData2;
    tBoolean bBoom;

    //
    // Assume no collisions until one is found.
    //
    bBoom = false;

    //
    // See if the player is moving in the same direction as the previous time.
    //
    if(ucDirection != (g_ucDirection & 0x0f))
    {
        //
        // The player is not moving the same as before, so see if the player is
        // moving or stopped.
        //
        if(ucDirection)
        {
            //
            // The player is moving, so save the direction of motion.
            //
            g_ucDirection = ucDirection;
        }
        else
        {
            //
            // The player is stopped, so save the previous direction of motion
            // (which determines the direction that the player faces when
            // stopped).
            //
            g_ucDirection <<= 4;
        }

        //
        // Reset the animation count to zero.
        //
        g_ucCount = 0;
    }
    else
    {
        //
        // The player is moving in the same direction as before, so increment
        // the animation count.
        //
        g_ucCount++;
    }

    //
    // Determine the animation to use based on the player's direction of
    // motion.
    //
    switch(g_ucDirection)
    {
        //
        // The player is moving up.
        //
        case 0x01:
        {
            //
            // Use the animation for the player walking up.
            //
            pucAnimation = g_pucPlayerWalkingUp;
            ulNum = sizeof(g_pucPlayerWalkingUp);

            //
            // Done handling this animation.
            //
            break;
        }

        //
        // The player is moving down.
        //
        case 0x02:
        {
            //
            // Use the animation for the player walking down.
            //
            pucAnimation = g_pucPlayerWalkingDown;
            ulNum = sizeof(g_pucPlayerWalkingDown);

            //
            // Done handling this animation.
            //
            break;
        }

        //
        // The player is moving left.
        //
        case 0x04:
        {
            //
            // Use the animation for the player walking left.
            //
            pucAnimation = g_pucPlayerWalkingLeft;
            ulNum = sizeof(g_pucPlayerWalkingLeft);

            //
            // Done handling this animation.
            //
            break;
        }

        //
        // The player is moving right.
        //
        case 0x08:
        {
            //
            // Use the animation for the player walking right.
            //
            pucAnimation = g_pucPlayerWalkingRight;
            ulNum = sizeof(g_pucPlayerWalkingRight);

            //
            // Done handling this animation.
            //
            break;
        }

        //
        // The player is dying.
        //
        case 0x0f:
        {
            //
            // Use the animation sequence for the player dying.
            //
            pucAnimation = g_pucPlayerDying;
            ulNum = sizeof(g_pucPlayerDying);

            //
            // Done handling this animation.
            //
            break;
        }

        //
        // The player is standing still but previously was moving up.
        //
        case 0x10:
        {
            //
            // Use the animation for the player standing while facing up.
            //
            pucAnimation = g_pucPlayerStandingUp;
            ulNum = sizeof(g_pucPlayerStandingUp);

            //
            // Done handling this animation.
            //
            break;
        }

        //
        // The player is standing still but previously was moving down.
        //
        default:
        case 0x20:
        {
            //
            // Use the animation for the player standing while facing down.
            //
            pucAnimation = g_pucPlayerStandingDown;
            ulNum = sizeof(g_pucPlayerStandingDown);

            //
            // Done handling this animation.
            //
            break;
        }

        //
        // The player is standing still but previously was moving left.
        //
        case 0x40:
        {
            //
            // Use the animation for the player standing while facing left.
            //
            pucAnimation = g_pucPlayerStandingLeft;
            ulNum = sizeof(g_pucPlayerStandingLeft);

            //
            // Done handling this animation.
            //
            break;
        }

        //
        // The player is standing still but previously was moving right.
        //
        case 0x80:
        {
            //
            // Use the animation for the player standing while facing right.
            //
            pucAnimation = g_pucPlayerStandingRight;
            ulNum = sizeof(g_pucPlayerStandingRight);

            //
            // Done handling this animation.
            //
            break;
        }
    }

    //
    // Find the index into the animation sequence based on the current
    // animation count.
    //
    for(ulLoop = 0; ulLoop < ulNum; ulLoop += 2)
    {
        if(pucAnimation[ulLoop] > g_ucCount)
        {
            break;
        }
    }

    //
    // See if the animation sequence has been completed.
    //
    if(ulLoop == ulNum)
    {
        //
        // Reset the animation count.
        //
        g_ucCount = 0;

        //
        // Use the first image in the animation sequence.
        //
        ulIdx = pucAnimation[1];
    }
    else
    {
        //
        // Otherwise, use the most recent image in the animation sequence.
        //
        ulIdx = pucAnimation[ulLoop - 1];
    }

    //
    // Loop over the bytes of the sprite for this sequence in the animation.
    //
    for(ulLoop = 0; ulLoop < 72; ulLoop++)
    {
        //
        // Copy the sprite data into the local frame buffer and check for a
        // collision.
        //
        ucData1 = g_pucFrame[(((ulLoop / 6) + 41) * 64) + (ulLoop % 6) + 29];
        ucData2 = g_ppucSprites[ulIdx][ulLoop];
        if((ucData2 & 0xf0) != 0)
        {
            if((ucData1 & 0xf0) != 0)
            {
                bBoom = true;
            }
            ucData1 &= 0x0f;
        }
        if((ucData2 & 0x0f) != 0)
        {
            if((ucData1 & 0x0f) != 0)
            {
                bBoom = true;
            }
            ucData1 &= 0xf0;
        }
        g_pucFrame[(((ulLoop / 6) + 41) * 64) + (ulLoop % 6) + 29] =
            ucData1 | ucData2;
    }

    //
    // Return an indication of the collision detection.
    //
    return(bBoom);
}

//*****************************************************************************
//
// Draws the active explosions onto the display.
//
//*****************************************************************************
static void
DrawExplosions(void)
{
    long lX, lY, lExplosionX, lExplosionY;
    unsigned char ucData1, ucData2;
    unsigned long ulLoop, ulIdx;

    //
    // Loop through all the explosions.
    //
    for(ulLoop = 0; ulLoop < 4; ulLoop++)
    {
        //
        // Skip this explosion if it is not active.
        //
        if((g_pusExplosionX[ulLoop] == 0) && (g_pusExplosionY[ulLoop] == 0))
        {
            continue;
        }

        //
        // Increment the explosion animation count.
        //
        g_pucExplosionCount[ulLoop]++;

        //
        // Find the index into the animation sequence based on the current
        // animation count.
        //
        for(ulIdx = 0; ulIdx < sizeof(g_pucExplosion); ulIdx += 2)
        {
            if(g_pucExplosion[ulIdx] > g_pucExplosionCount[ulLoop])
            {
                break;
            }
        }

        //
        // See if the animation sequence has been completed.
        //
        if(ulIdx == sizeof(g_pucExplosion))
        {
            //
            // This explosion is done, so disable it.
            //
            g_pusExplosionX[ulLoop] = 0;
            g_pusExplosionY[ulLoop] = 0;

            //
            // Go to the next explosion.
            //
            continue;
        }
        else
        {
            //
            // Otherwise, use the most recent image in the animation sequence.
            //
            ulIdx = g_pucExplosion[ulIdx - 1];
        }

        //
        // Find the position of this explosion on the display.
        //
        lExplosionX = g_pusExplosionX[ulLoop] - (g_usPlayerX - (64 - 6));
        lExplosionY = g_pusExplosionY[ulLoop] - (g_usPlayerY - (47 - 6));

        //
        // Loop through the scan lines of this explosion.
        //
        for(lY = (lExplosionY < 0) ? -lExplosionY : 0;
            (lY < 12) && ((lExplosionY + lY) < 94); lY++)
        {
            //
            // Loop through the columns of this explosion.
            //
            for(lX = (lExplosionX < 0) ? -lExplosionX : 0;
                (lX < 12) && ((lExplosionX + lX) < 128); lX++)
            {
                //
                // Copy the sprite data into the local frame buffer.
                //
                ucData1 = g_pucFrame[((lExplosionY + lY) * 64) +
                                     ((lExplosionX + lX) / 2)];
                ucData2 = g_ppucSprites[ulIdx][(lY * 6) + (lX / 2)];
                if((ucData2 & 0xf0) != 0x00)
                {
                    ucData1 &= 0x0f;
                }
                if((ucData2 & 0x0f) != 0x00)
                {
                    ucData1 &= 0xf0;
                }
                g_pucFrame[((lExplosionY + lY) * 64) +
                           ((lExplosionX + lX) / 2)] = ucData1 | ucData2;
            }
        }
    }
}

//*****************************************************************************
//
// Draws a single number to the local frame buffer, with an optional trailing
// dot.  The mask specifies the minimum number of digits to draw (i.e. a mask
// of 100 will always draw digits for the hundreds, tens, and ones digit even
// if not required).
//
//*****************************************************************************
static unsigned long
DrawNumber(unsigned long ulStart, unsigned long ulNumber, unsigned long ulMask,
           tBoolean bDot)
{
    unsigned long ulLoop, ulIdx, ulDigit;

    //
    // Loop through the three possible digits in this number.
    //
    for(ulIdx = 1000000000; ulIdx > 0; ulIdx /= 10)
    {
        //
        // Continue if this digit should not be drawn (i.e. the hundreds digit
        // for a two digit number).
        //
        if((ulNumber < ulIdx) && (ulIdx > ulMask))
        {
            continue;
        }

        //
        // Extract this digit from the number.
        //
        ulDigit = (ulNumber / ulIdx) % 10;

        //
        // Loop over the bytes of the image for this digit.
        //
        for(ulLoop = 0; ulLoop < 52; ulLoop++)
        {
            //
            // Copy this byte of the image to the local frame buffer.
            //
            g_pucFrame[ulStart + ((ulLoop / 4) * 64) + (ulLoop % 4)] =
                g_ppucDigits[ulDigit][ulLoop];
        }

        //
        // Skip past this digit in the local frame buffer.
        //
        ulStart += 4;
    }

    //
    // See if a trailing dot should be drawn.
    //
    if(bDot)
    {
        //
        // Loop over the bytes of the image for the dot.
        //
        for(ulLoop = 0; ulLoop < 26; ulLoop++)
        {
            //
            // Copy this byte of the dot image to the local frame buffer.
            //
            g_pucFrame[ulStart + ((ulLoop / 2) * 64) + (ulLoop % 2)] =
                g_pucDot[ulLoop];
        }

        //
        // Skip past the dot in the local frame buffer.
        //
        ulStart += 2;
    }

    //
    // Return the new frame buffer starting address so that further drawing can
    // occur after the number just drawn.
    //
    return(ulStart);
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
    unsigned long ulCount, ulLoop, ulAddr, ulStart, ulDigit;

    //
    // Generate a new maze.
    //
    GenerateMaze();

    //
    // Set the initial display position near the upper left of the maze.
    //
    g_usPlayerX = 90;
    g_usPlayerY = 90;

    //
    // Get rid of all the monsters.
    //
    for(ulLoop = 0; ulLoop < 100; ulLoop++)
    {
        g_pusMonsterX[ulLoop] = 0;
        g_pusMonsterY[ulLoop] = 0;
    }

    //
    // Loop through the number of updates to the main screen to be done before
    // the screen saver is called instead.
    //
    for(ulCount = 0; ulCount < (2 * 60 * 30); ulCount++)
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
        // See if the display position is along the top of the maze.
        //
        if((g_usPlayerY == 90) && (g_usPlayerX != (1524 - 102)))
        {
            //
            // Move the display position to the right.
            //
            g_usPlayerX += 2;
        }

        //
        // Otherwise, see if the display position is along the bottom of the
        // maze.
        //
        else if((g_usPlayerY == (1128 - 102)) && (g_usPlayerX != 90))
        {
            //
            // Move the display position to the left.
            //
            g_usPlayerX -= 2;
        }

        //
        // Otherwise, see if the display position is along the right of the
        // maze.
        //
        else if(g_usPlayerX == (1524 - 102))
        {
            //
            // Move the display position down.
            //
            g_usPlayerY += 2;
        }

        //
        // Otherwise, the display position is along the left of the maze.
        //
        else
        {
            //
            // Move the display position up.
            //
            g_usPlayerY -= 2;
        }

        //
        // Clear the local frame buffer.
        //
        for(ulLoop = 0; ulLoop < sizeof(g_pucFrame); ulLoop += 4)
        {
            *((unsigned long *)(g_pucFrame + ulLoop)) = 0;
        }

        //
        // Draw the maze.
        //
        DrawMaze();

        //
        // Display the "Press Button To Play" text for sixteen frames every
        // sixteen frames, causing it to flash with a 50% duty cycle.
        //
        if(ulCount & 16)
        {
            //
            // Loop over the bytes in the image.
            //
            for(ulLoop = 0; ulLoop < (64 * 16); ulLoop++)
            {
                //
                // Copy this byte from the image to the local frame buffer.
                //
                g_pucFrame[ulLoop + (39 * 64)] = g_pucPlay[ulLoop];
            }
        }

        //
        // Get the current IP address of the Ethernet interface.  This will be
        // zero when the IP address has not been assigned yet.
        //
        ulAddr = EnetGetIPAddr();

        //
        // See if the IP address has been assigned.
        //
        if(ulAddr == 0)
        {
            //
            // Display the "Acquiring Address..." text across the bottom of the
            // display.  Loop over the bytes in the image.
            //
            for(ulLoop = 0; ulLoop < (64 * 13); ulLoop++)
            {
                //
                // Copy this byte from the image to the local frame buffer.
                //
                g_pucFrame[ulLoop + (81 * 64)] = g_pucAcquiring[ulLoop];
            }
        }
        else
        {
            //
            // Get the width of the digits in the IP address.
            //
            ulDigit = ulAddr >> 24;
            ulStart = (ulDigit > 99) ? 24 : ((ulDigit > 9) ? 16 : 8);
            ulDigit = (ulAddr >> 16) & 0xff;
            ulStart += (ulDigit > 99) ? 24 : ((ulDigit > 9) ? 16 : 8);
            ulDigit = (ulAddr >> 8) & 0xff;
            ulStart += (ulDigit > 99) ? 24 : ((ulDigit > 9) ? 16 : 8);
            ulDigit = ulAddr & 0xff;
            ulStart += (ulDigit > 99) ? 24 : ((ulDigit > 9) ? 16 : 8);

            //
            // Compute the starting address in the local frame buffer of the
            // location for the "IP: <addr>" string.
            //
            ulStart = (81 * 64) + ((128 - (ulStart + 30)) / 4);

            //
            // Display the "IP:" text on the bottom of the display.  Loop over
            // the bytes in the image.
            //
            for(ulLoop = 0; ulLoop < (9 * 13); ulLoop++)
            {
                //
                // Copy this byte from the image to the local frame buffer.
                //
                g_pucFrame[ulStart + ((ulLoop / 9) * 64) + (ulLoop % 9)] =
                    g_pucIP[ulLoop];
            }

            //
            // Advance the frame buffer starting address.
            //
            ulStart += 9;

            //
            // Draw the first byte of the IP address, with a dot to separate it
            // from the next byte.
            //
            ulStart = DrawNumber(ulStart, ulAddr & 0xff, 1, true);

            //
            // Draw the second byte of the IP address, with a dot to separate
            // it from the next byte.
            //
            ulStart = DrawNumber(ulStart, (ulAddr >> 8) & 0xff, 1, true);

            //
            // Draw the third byte of the IP address, with a dot to separate it
            // from the next byte.
            //
            ulStart = DrawNumber(ulStart, (ulAddr >> 16) & 0xff, 1, true);

            //
            // Draw the fourth bytes of the IP address.
            //
            DrawNumber(ulStart, ulAddr >> 24, 1, false);
        }

        //
        // Display the updated image on the display.
        //
        RIT128x96x4ImageDraw(g_pucFrame, 0, 0, 128, 96);
    }

    //
    // Clear out the maze.
    //
    for(ulLoop = 0; ulLoop < 94; ulLoop++)
    {
        for(ulCount = 0; ulCount < 127; ulCount++)
        {
            g_ppcMaze[ulLoop][ulCount] = 0;
        }
    }

    //
    // Move the player position to the upper left corner.
    //
    g_usPlayerX = 0;
    g_usPlayerY = 0;

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
    tBoolean bAtExit, bDead, bStart, bMoveMonsters, bFire;
    unsigned long ulLoop, ulCount;

    //
    // A new maze needs to be generated.
    //
    bStart = true;

    //
    // The player is not dead yet.
    //
    bDead = false;

    //
    // The player is not at the exit yet.
    //
    bAtExit = false;

    //
    // The monsters should not be moved yet.
    //
    bMoveMonsters = false;

    //
    // The animation count defaults to zero.
    //
    ulCount = 0;

    //
    // Reset the score.
    //
    g_ulScore = 0;

    //
    // Play the start of game song.
    //
    AudioPlaySong(g_pusStartOfGame, sizeof(g_pusStartOfGame) / 2);

    //
    // Loop until the game is over.
    //
    while(1)
    {
        //
        // See if a new maze should be generated.
        //
        if(bStart)
        {
            //
            // Generate a new maze.
            //
            GenerateMaze();

            //
            // Set the initial player position in the middle of the maze.
            //
            g_usPlayerX = 738;
            g_usPlayerY = 558;

            //
            // The player is initially facing down.
            //
            g_ucDirection = 0;

            //
            // The player is not at the exit.
            //
            bAtExit = false;

            //
            // Choose random positions for the monsters.
            //
            for(ulLoop = 0; ulLoop < 100; ulLoop++)
            {
                //
                // Pick a random position for this monster until it is not too
                // cloose to the player's starting position.
                //
                do
                {
                    g_pusMonsterX[ulLoop] =
                        ((((RandomNumber() / 65536) * 42) / 65536) * 36) + 18;
                    g_pusMonsterY[ulLoop] =
                        ((((RandomNumber() / 65536) * 31) / 65536) * 36) + 18;
                }
                while((g_pusMonsterX[ulLoop] > (g_usPlayerX - 100)) &&
                      (g_pusMonsterX[ulLoop] < (g_usPlayerX + 100)) &&
                      (g_pusMonsterY[ulLoop] > (g_usPlayerY - 50)) &&
                      (g_pusMonsterY[ulLoop] < (g_usPlayerY + 50)));

                //
                // Pick a random offset into the monster animation for this
                // monster.
                //
                g_pucMonsterCount[ulLoop] =
                    (((RandomNumber() / 65536) *
                      g_pucMonster[sizeof(g_pucMonster) - 2]) / 65536);
            }

            //
            // The monsters should not be moved on the first update.
            //
            bMoveMonsters = false;

            //
            // Initially there are no bullets or explosions.
            //
            for(ulLoop = 0; ulLoop < 4; ulLoop++)
            {
                g_pusBulletX[ulLoop] = 0;
                g_pusBulletY[ulLoop] = 0;
                g_pusExplosionX[ulLoop] = 0;
                g_pusExplosionY[ulLoop] = 0;
            }

            //
            // A new maze no longer needs to be generated.
            //
            bStart = false;
        }

        //
        // Clear the local frame buffer.
        //
        for(ulLoop = 0; ulLoop < sizeof(g_pucFrame); ulLoop += 4)
        {
            *((unsigned long *)(g_pucFrame + ulLoop)) = 0;
        }

        //
        // See if the player is dead.
        //
        if(!bDead && !bAtExit)
        {
            //
            // See if all four direction buttons are pressed simultaneously.
            //
            if((g_ucSwitches & 0x0f) == 0)
            {
                //
                // All four direction buttons are pressed simultaneously, so
                // draw the entire maze as a cheat.
                //
                DrawCheat();
            }
            else
            {
                //
                // See if only the up button is pressed.
                //
                if((g_ucSwitches & 0x0f) == 0x0e)
                {
                    //
                    // See if there is a wall blocking the player's movement in
                    // the up direction.
                    //
                    if((g_ppcMaze[(g_usPlayerY - 2) / 12]
                                 [g_usPlayerX / 12] == 0) &&
                       (g_ppcMaze[(g_usPlayerY - 2) / 12]
                                 [(g_usPlayerX + 11) / 12] == 0))
                    {
                        //
                        // The player's movement is not blocked so move up by
                        // two positions.
                        //
                        g_usPlayerY -= 2;

                        //
                        // Increment the score since the player moved.
                        //
                        g_ulScore++;
                    }
                }

                //
                // See if only the down button is pressed.
                //
                if((g_ucSwitches & 0x0f) == 0x0d)
                {
                    //
                    // See if there is a wall blocking the player's movement in
                    // the down direction.
                    //
                    if((g_ppcMaze[(g_usPlayerY + 12) / 12]
                                 [g_usPlayerX / 12] == 0) &&
                       (g_ppcMaze[(g_usPlayerY + 12) / 12]
                                 [(g_usPlayerX + 11) / 12] == 0))
                    {
                        //
                        // The player's movement is not blocked so move down by
                        // two positions.
                        //
                        g_usPlayerY += 2;

                        //
                        // Increment the score since the player moved.
                        //
                        g_ulScore++;
                    }
                }

                //
                // See if only the left button is pressed.
                //
                if((g_ucSwitches & 0x0f) == 0x0b)
                {
                    //
                    // See if there is a wall blocking the player's movement in
                    // the left direction.
                    //
                    if((g_ppcMaze[g_usPlayerY / 12]
                                 [(g_usPlayerX - 2) / 12] == 0) &&
                       (g_ppcMaze[(g_usPlayerY + 11) / 12]
                                 [(g_usPlayerX - 2) / 12] == 0))
                    {
                        //
                        // The player's movement is not blocked so move left by
                        // two positions.
                        //
                        g_usPlayerX -= 2;

                        //
                        // Increment the score since the player moved.
                        //
                        g_ulScore++;
                    }
                }

                //
                // See if only the right button is pressed.
                //
                if((g_ucSwitches & 0x0f) == 0x07)
                {
                    //
                    // See if there is a wall blocking the player's movement in
                    // the right direction.
                    //
                    if((g_ppcMaze[g_usPlayerY / 12]
                                 [(g_usPlayerX + 12) / 12] == 0) &&
                       (g_ppcMaze[(g_usPlayerY + 11) / 12]
                                 [(g_usPlayerX + 12) / 12] == 0))
                    {
                        //
                        // The player's movement is not blocked so move right
                        // by two positions.
                        //
                        g_usPlayerX += 2;

                        //
                        // Increment the score since the player moved.
                        //
                        g_ulScore++;
                    }
                }

                //
                // See if the select button was pressed.
                //
                if(HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS))
                {
                    //
                    // Clear the button press indicator.
                    //
                    HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS) = 0;

                    //
                    // Request that a bullet be fired if possible.
                    //
                    bFire = true;
                }
                else
                {
                    //
                    // No bullet should be fired at this time.
                    //
                    bFire = false;
                }

                //
                // Move the monsters if requried.
                //
                if(bMoveMonsters)
                {
                    MoveMonsters();
                }
                bMoveMonsters = bMoveMonsters ? false : true;

                //
                // Draw the maze.
                //
                DrawMaze();

                //
                // Draw the monsters.
                //
                DrawMonsters();

                //
                // Draw the bullets.
                //
                DrawBullets(bFire);

                //
                // Draw the player.
                //
                bDead = DrawPlayer((g_ucSwitches & 0x0f) ^ 0x0f);

                //
                // Draw the explosions.
                //
                DrawExplosions();

                //
                // See if the player has reached the exit.
                //
                if((g_usPlayerX < 6) || (g_usPlayerX > 1506) ||
                   (g_usPlayerY < 6) || (g_usPlayerY > 1110))
                {
                    //
                    // The exit was reached, so add ten thousand to the score.
                    //
                    g_ulScore += 10000;

                    //
                    // Indicate that the player is at the exit and set the
                    // animation count for the exit.
                    //
                    bAtExit = true;
                    ulCount = 64;

                    //
                    // Play the completion of maze song.
                    //
                    AudioPlaySong(g_pusEndOfMaze, sizeof(g_pusEndOfMaze) / 2);
                }

                //
                // See if the player has died.
                //
                if(bDead)
                {
                    //
                    // Play the sound effect for the player dying.
                    //
                    AudioPlaySound(g_pusPlayerEffect,
                                   sizeof(g_pusPlayerEffect) / 2);

                    //
                    // Set the number of steps in the end of game sequence.
                    //
                    ulCount = 150;
                }
            }
        }

        //
        // Otherwise, see if the player is at the exit.
        //
        else if(bAtExit)
        {
            //
            // The player is at the exit, so flash the maze at the current
            // position.
            //
            if((ulCount % 16) > 8)
            {
                DrawMaze();
            }

            //
            // Decrement the animation count.
            //
            ulCount--;

            //
            // See if the animation count has reached zero.
            //
            if(ulCount == 0)
            {
                //
                // Request that a new maze be generated for the next update.
                //
                bStart = true;
            }
        }
        else
        {
            //
            // The player is dead, so draw the maze, monsters, and show the
            // player dying.
            //
            DrawMaze();
            DrawMonsters();
            DrawPlayer(0x0f);

            //
            // See if the player death sound effect has completed.
            //
            if(ulCount == 130)
            {
                //
                // Play the end of game song.
                //
                AudioPlaySong(g_pusEndOfGame, sizeof(g_pusEndOfGame) / 2);
            }

            //
            // Flash the "Game Over" text across the top of the display.
            //
            if((ulCount % 30) >= 15)
            {
                for(ulLoop = 0; ulLoop < 481; ulLoop++)
                {
                    g_pucFrame[(((ulLoop / 37) + 5) * 64) +
                               (ulLoop % 37) + 13] = g_pucGameOver[ulLoop];
                }
            }

            //
            // Display the score text across the bottom of the display.
            //
            for(ulLoop = 0; ulLoop < 286; ulLoop++)
            {
                g_pucFrame[(((ulLoop / 22) + 76) * 64) +
                           (ulLoop % 22) + 8] = g_pucScore[ulLoop];
            }
            for(ulLoop = 0; ulLoop < 13; ulLoop++)
            {
                g_pucFrame[((ulLoop + 76) * 64) + 30] = 0;
            }
            DrawNumber((76 * 64) + 31, g_ulScore % 1000000, 100000, false);

            //
            // Decrement the animation count.
            //
            ulCount--;
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
        // Display the updated image on the display.
        //
        RIT128x96x4ImageDraw(g_pucFrame, 0, 0, 128, 96);

        //
        // Write the current score to the UART.
        //
        UARTCharPut(UART0_BASE, '\r');
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 100000) % 10));
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 10000) % 10));
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 1000) % 10));
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 100) % 10));
        UARTCharPut(UART0_BASE, '0' + ((g_ulScore / 10) % 10));
        UARTCharPut(UART0_BASE, '0' + (g_ulScore % 10));

        //
        // See if the player is dead and the animation count has reached zero.
        //
        if(bDead && (ulCount == 0))
        {
            //
            // Exit the game.
            //
            break;
        }
    }

    //
    // Clear the button press indicator since it may still be set from the
    // player trying to fire bullets at the end of the game.
    //
    HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS) = 0;
}
