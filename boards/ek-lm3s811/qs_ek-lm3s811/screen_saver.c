//*****************************************************************************
//
// screen_saver.c - A screen saver for the OLED display.
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
// This is part of revision 9453 of the EK-LM3S811 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "drivers/display96x16x1.h"
#include "globals.h"
#include "random.h"
#include "screen_saver.h"

//*****************************************************************************
//
// A screen saver to avoid damage to the OLED display (it has similar
// characteristics to a CRT with respect to image burn-in).  This implements
// John Conway's "Game of Life" (from the April 1970 issue of Scientific
// American).
//
//*****************************************************************************
void
ScreenSaver(void)
{
    unsigned long ulCount, ulLoop, ulData1, ulData2, ulData3, ulData, ulBit;
    unsigned long ulValue;

    //
    // Loop through the number of updates to the graphical screen saver to be
    // done before the display is turned off.
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
            // Return to the caller, ending the screen saver.
            //
            return;
        }

        //
        // See how far it is into a ten second interval.
        //
        ulData = ulCount % (10 * CLOCK_RATE / 4);

        //
        // See if this is one of the first 32 updates of a ten second interval.
        //
        if(ulData < 32)
        {
            //
            // Fill the field with random data to seed the game.  Each update,
            // three columns are filled.
            //
            ulData *= 3;

            //
            // Get a random number.
            //
            ulData1 = RandomNumber();

            //
            // Loop through three columns of the display.
            //
            for(ulLoop = 0; ulLoop < 3; ulLoop++)
            {
                //
                // Fill this column with random data.
                //
                g_pucFrame[ulData + ulLoop] = ulData1 >> 24;
                ulData1 = NEXT_RAND(ulData1);
                g_pucFrame[ulData + ulLoop + 96] = ulData1 >> 24;
                ulData1 = NEXT_RAND(ulData1);
            }

            //
            // Display the updated screen saver image on the display.
            //
            Display96x16x1ImageDraw(g_pucFrame, 0, 0, 96, 2);

            //
            // Wait for the next update request.
            //
            continue;
        }

        //
        // Only update every fourth update request so that things don't proceed
        // too quickly.
        //
        if((ulCount & 0x00000003) != 0)
        {
            //
            // Wait for the next update request.
            //
            continue;
        }

        //
        // Copy the local frame buffer to the background image buffer.
        //
        for(ulLoop = 0; ulLoop < 192; ulLoop += 4)
        {
            *(unsigned long *)(g_pucBackground + ulLoop) =
                *(unsigned long *)(g_pucFrame + ulLoop);
        }

        //
        // Loop over the columns of the display.
        //
        for(ulLoop = 0; ulLoop < 96; ulLoop++)
        {
            //
            // See if this is the first column of the display.
            //
            if(ulLoop == 0)
            {
                //
                // Get the first two columns of the display.
                //
                ulData1 = 0;
                ulData2 = g_pucBackground[0] | (g_pucBackground[96] << 8);
                ulData3 = g_pucBackground[1] | (g_pucBackground[97] << 8);
            }

            //
            // See if this is the last column of the display.
            //
            else if(ulLoop == 95)
            {
                //
                // Get the last two columns of the display.
                //
                ulData1 = g_pucBackground[94] | (g_pucBackground[190] << 8);
                ulData2 = g_pucBackground[95] | (g_pucBackground[191] << 8);
                ulData3 = 0;
            }

            //
            // Otherwise, this is somewhere in the middle of the display.
            //
            else
            {
                //
                // Get the current column, plus the two adjacent columns.
                //
                ulData1 = (g_pucBackground[ulLoop - 1] |
                           (g_pucBackground[ulLoop + 95] << 8));
                ulData2 = (g_pucBackground[ulLoop] |
                           (g_pucBackground[ulLoop + 96] << 8));
                ulData3 = (g_pucBackground[ulLoop + 1] |
                           (g_pucBackground[ulLoop + 97] << 8));
            }

            //
            // Set the new column data to zero.
            //
            ulData = 0;

            //
            // Loop over the scan lines of this column.
            //
            for(ulBit = 0; ulBit < 16; ulBit++)
            {
                //
                // See if this is the first scan line.
                //
                if(ulBit == 0)
                {
                    //
                    // Count the number of organisms in the adjacent cells.
                    //
                    ulValue = (((ulData1 & 0x0001) ? 1 : 0) +
                               ((ulData1 & 0x0002) ? 1 : 0) +
                               ((ulData2 & 0x0002) ? 1 : 0) +
                               ((ulData3 & 0x0001) ? 1 : 0) +
                               ((ulData3 & 0x0002) ? 1 : 0));
                }

                //
                // See if this is the last scan line.
                //
                else if(ulBit == 15)
                {
                    //
                    // Count the number of organisms in the adjacent cells.
                    //
                    ulValue = (((ulData1 & 0x4000) ? 1 : 0) +
                               ((ulData1 & 0x8000) ? 1 : 0) +
                               ((ulData2 & 0x4000) ? 1 : 0) +
                               ((ulData3 & 0x4000) ? 1 : 0) +
                               ((ulData3 & 0x8000) ? 1 : 0));
                }

                //
                // Otherwise, this is somewhere in the middle of the display.
                //
                else
                {
                    //
                    // Count the number of organisms in the adjacent cells.
                    //
                    ulValue = (((ulData1 & (1 << (ulBit - 1))) ? 1 : 0) +
                               ((ulData1 & (1 << ulBit)) ? 1 : 0) +
                               ((ulData1 & (1 << (ulBit + 1))) ? 1 : 0) +
                               ((ulData2 & (1 << (ulBit - 1))) ? 1 : 0) +
                               ((ulData2 & (1 << (ulBit + 1))) ? 1 : 0) +
                               ((ulData3 & (1 << (ulBit - 1))) ? 1 : 0) +
                               ((ulData3 & (1 << ulBit)) ? 1 : 0) +
                               ((ulData3 & (1 << (ulBit + 1))) ? 1 : 0));
                }

                //
                // Survival; if this cell contains an organism and has two or
                // three neighbors, then it survives.  With zero or one
                // neighbor, it dies of boredom.  With four or more neighbors,
                // it dies of overcrowding.
                //
                if((ulData2 & (1 << ulBit)) && (ulValue > 1) && (ulValue < 4))
                {
                    ulData |= (1 << ulBit);
                }

                //
                // Birth; if this cell does not contain an organism and has
                // three neighbors, then it generates a new organism.
                //
                if(!(ulData & (1 << ulBit)) && (ulValue == 3))
                {
                    ulData |= (1 << ulBit);
                }
            }

            //
            // Put the newly generated column of data into the local frame
            // buffer.
            //
            g_pucFrame[ulLoop] = ulData & 0xff;
            g_pucFrame[ulLoop + 96] = ulData >> 8;
        }

        //
        // Display the updated screen saver image on the display.
        //
        Display96x16x1ImageDraw(g_pucFrame, 0, 0, 96, 2);
    }

    //
    // Clear the display and turn it off.
    //
    Display96x16x1Clear();
    Display96x16x1DisplayOff();

    //
    // Configure the user LED pin for hardware control, i.e. the PWM output
    // from the timer.
    //
    TimerMatchSet(TIMER0_BASE, TIMER_B, 0);
    GPIOPinTypeTimer(GPIO_PORTC_BASE, USER_LED);

    //
    // Loop forever.
    //
    for(ulCount = 0; ; ulCount = (ulCount + 1) & 63)
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
        // Turn on the user LED in sixteen gradual steps.
        //
        if((ulCount > 0) && (ulCount <= 16))
        {
            TimerMatchSet(TIMER0_BASE, TIMER_B,
                          ((ulCount * SysCtlClockGet()) / 160000) - 2);
        }

        //
        // Turn off the user LED in eight gradual steps.
        //
        if((ulCount > 32) && (ulCount <= 48))
        {
            TimerMatchSet(TIMER0_BASE, TIMER_B,
                          (((48 - ulCount) * SysCtlClockGet()) / 160000) - 2);
        }

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
            // Break out of the loop.
            //
            break;
        }
    }

    //
    // Turn off the user LED.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, USER_LED);
    GPIOPinWrite(GPIO_PORTC_BASE, USER_LED, 0);

    //
    // Turn on the display.
    //
    Display96x16x1DisplayOn();
}
