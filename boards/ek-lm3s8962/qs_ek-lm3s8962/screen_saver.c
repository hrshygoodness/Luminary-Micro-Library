//*****************************************************************************
//
// screen_saver.c - A screen saver for the OSRAM OLED display.
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
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"
#include "audio.h"
#include "can_net.h"
#include "globals.h"
#include "random.h"
#include "screen_saver.h"

//*****************************************************************************
//
// These arrays contain the starting and ending points of the lines on the
// display; this history buffer lists the lines from oldest to youngest.
//
//*****************************************************************************
static unsigned char g_pucScreenLinesX1[30];
static unsigned char g_pucScreenLinesY1[30];
static unsigned char g_pucScreenLinesX2[30];
static unsigned char g_pucScreenLinesY2[30];

//*****************************************************************************
//
// These variables contain the direction and rate of movement of the endpoints
// of the lines being drawn on the display.
//
//*****************************************************************************
static char g_cScreenDeltaX1;
static char g_cScreenDeltaY1;
static char g_cScreenDeltaX2;
static char g_cScreenDeltaY2;

//*****************************************************************************
//
// Draws a line in the local frame buffer using Bresneham's line drawing
// algorithm.
//
//*****************************************************************************
static void
ScreenSaverLine(long lX1, long lY1, long lX2, long lY2, unsigned long ulLevel)
{
    long lError, lDeltaX, lDeltaY, lYStep;
    tBoolean bSteep;

    //
    // Determine if the line is steep.  A steep line has more motion in the Y
    // direction than the X direction.
    //
    if(((lY2 > lY1) ? (lY2 - lY1) : (lY1 - lY2)) >
       ((lX2 > lX1) ? (lX2 - lX1) : (lX1 - lX2)))
    {
        bSteep = true;
    }
    else
    {
        bSteep = false;
    }

    //
    // If the line is steep, then swap the X and Y coordinates.
    //
    if(bSteep)
    {
        lError = lX1;
        lX1 = lY1;
        lY1 = lError;
        lError = lX2;
        lX2 = lY2;
        lY2 = lError;
    }

    //
    // If the starting X coordinate is larger than the ending X coordinate,
    // tehn swap the start and end coordinates.
    //
    if(lX1 > lX2)
    {
        lError = lX1;
        lX1 = lX2;
        lX2 = lError;
        lError = lY1;
        lY1 = lY2;
        lY2 = lError;
    }

    //
    // Compute the difference between the start and end coordinates in each
    // axis.
    //
    lDeltaX = lX2 - lX1;
    lDeltaY = (lY2 > lY1) ? (lY2 - lY1) : (lY1 - lY2);

    //
    // Initialize the error term to negative half the X delta.
    //
    lError = -lDeltaX / 2;

    //
    // Determine the direction to step in the Y axis when required.
    //
    if(lY1 < lY2)
    {
        lYStep = 1;
    }
    else
    {
        lYStep = -1;
    }

    //
    // Loop through all the points along the X axis of the line.
    //
    for(; lX1 <= lX2; lX1++)
    {
        //
        // See if this is a steep line.
        //
        if(bSteep)
        {
            //
            // Plot this point of the line, swapping the X and Y coordinates.
            //
            if(lY1 & 1)
            {
                g_pucFrame[(lX1 * 64) + (lY1 / 2)] =
                    ((g_pucFrame[(lX1 * 64) + (lY1 / 2)] & 0xf0) |
                     (ulLevel & 0xf));
            }
            else
            {
                g_pucFrame[(lX1 * 64) + (lY1 / 2)] =
                    ((g_pucFrame[(lX1 * 64) + (lY1 / 2)] & 0x0f) |
                     ((ulLevel & 0xf) << 4));
            }
        }
        else
        {
            //
            // Plot this point of the line, using the coordinates as is.
            //
            if(lX1 & 1)
            {
                g_pucFrame[(lY1 * 64) + (lX1 / 2)] =
                    ((g_pucFrame[(lY1 * 64) + (lX1 / 2)] & 0xf0) |
                     (ulLevel & 0xf));
            }
            else
            {
                g_pucFrame[(lY1 * 64) + (lX1 / 2)] =
                    ((g_pucFrame[(lY1 * 64) + (lX1 / 2)] & 0x0f) |
                     ((ulLevel & 0xf) << 4));
            }
        }

        //
        // Increment the error term by the Y delta.
        //
        lError += lDeltaY;

        //
        // See if the error term is now greater than zero.
        //
        if(lError > 0)
        {
            //
            // Take a step in the Y axis.
            //
            lY1 += lYStep;

            //
            // Decrement the error term by the X delta.
            //
            lError -= lDeltaX;
        }
    }
}

//*****************************************************************************
//
// A screen saver to avoid damage to the OLED display (it has similar
// characteristics to a CRT with respect to image burn-in).  This implements a
// Qix-style chasing line that bounces about the display.
//
//*****************************************************************************
void
ScreenSaver(void)
{
    unsigned long ulCount, ulLoop;

    //
    // Clear out the line history so that any lines from a previous run of the
    // screen saver are not used again.
    //
    for(ulLoop = 0; ulLoop < 29; ulLoop++)
    {
        g_pucScreenLinesX1[ulLoop] = 0;
        g_pucScreenLinesY1[ulLoop] = 0;
        g_pucScreenLinesX2[ulLoop] = 0;
        g_pucScreenLinesY2[ulLoop] = 0;
    }

    //
    // Choose random starting points for the first line.
    //
    g_pucScreenLinesX1[29] = RandomNumber() >> 25;
    g_pucScreenLinesY1[29] = RandomNumber() >> 26;
    g_pucScreenLinesX2[29] = RandomNumber() >> 25;
    g_pucScreenLinesY2[29] = RandomNumber() >> 26;

    //
    // Choose a random direction for each endpoint.  Make sure that the
    // endpoint does not have a zero direction vector.
    //
    g_cScreenDeltaX1 = 4 - (RandomNumber() >> 29);
    if(g_cScreenDeltaX1 < 1)
    {
        g_cScreenDeltaX1--;
    }
    g_cScreenDeltaY1 = 4 - (RandomNumber() >> 29);
    if(g_cScreenDeltaY1 < 1)
    {
        g_cScreenDeltaY1--;
    }
    g_cScreenDeltaX2 = 4 - (RandomNumber() >> 29);
    if(g_cScreenDeltaX2 < 1)
    {
        g_cScreenDeltaX2--;
    }
    g_cScreenDeltaY2 = 4 - (RandomNumber() >> 29);
    if(g_cScreenDeltaY2 < 1)
    {
        g_cScreenDeltaY2--;
    }

    //
    // Loop through the number of updates to the graphical screen saver to be
    // done before the display is turned off.
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
            // Return to the caller, ending the screen saver.
            //
            return;
        }

        //
        // Shift the lines down one entry in the history buffer.
        //
        for(ulLoop = 1; ulLoop < 30; ulLoop++)
        {
            g_pucScreenLinesX1[ulLoop - 1] = g_pucScreenLinesX1[ulLoop];
            g_pucScreenLinesY1[ulLoop - 1] = g_pucScreenLinesY1[ulLoop];
            g_pucScreenLinesX2[ulLoop - 1] = g_pucScreenLinesX2[ulLoop];
            g_pucScreenLinesY2[ulLoop - 1] = g_pucScreenLinesY2[ulLoop];
        }

        //
        // Update the starting X coordinate of the youngest line.  If the edge
        // of the display has been reached, then choose a new travel speed in
        // the opposite direction.
        //
        g_pucScreenLinesX1[29] = g_pucScreenLinesX1[28] + g_cScreenDeltaX1;
        if(g_pucScreenLinesX1[29] > 127)
        {
            if(g_pucScreenLinesX1[29] > 191)
            {
                g_pucScreenLinesX1[29] = 0;
                g_cScreenDeltaX1 = (RandomNumber() >> 30) + 1;
            }
            else
            {
                g_pucScreenLinesX1[29] = 127;
                g_cScreenDeltaX1 = (char)-1 - (RandomNumber() >> 30);
            }
        }

        //
        // Update the starting Y coordinate of the youngest line.  If the edge
        // of the display has been reached, then choose a new travel speed in
        // the opposite direction.
        //
        g_pucScreenLinesY1[29] = g_pucScreenLinesY1[28] + g_cScreenDeltaY1;
        if(g_pucScreenLinesY1[29] > 95)
        {
            if(g_pucScreenLinesY1[29] > 191)
            {
                g_pucScreenLinesY1[29] = 0;
                g_cScreenDeltaY1 = (RandomNumber() >> 30) + 1;
            }
            else
            {
                g_pucScreenLinesY1[29] = 95;
                g_cScreenDeltaY1 = (char)-1 - (RandomNumber() >> 30);
            }
        }

        //
        // Update the ending X coordinate of the youngest line.  If the edge of
        // the display has been reached, then choose a new travel speed in the
        // opposite direction.
        //
        g_pucScreenLinesX2[29] = g_pucScreenLinesX2[28] + g_cScreenDeltaX2;
        if(g_pucScreenLinesX2[29] > 127)
        {
            if(g_pucScreenLinesX2[29] > 191)
            {
                g_pucScreenLinesX2[29] = 0;
                g_cScreenDeltaX2 = (RandomNumber() >> 30) + 1;
            }
            else
            {
                g_pucScreenLinesX2[29] = 127;
                g_cScreenDeltaX2 = (char)-1 - (RandomNumber() >> 30);
            }
        }

        //
        // Update the ending Y coordinate of the youngest line.  If the edge of
        // the display has been reached, then choose a new travel speed in the
        // opposite direction.
        //
        g_pucScreenLinesY2[29] = g_pucScreenLinesY2[28] + g_cScreenDeltaY2;
        if(g_pucScreenLinesY2[29] > 95)
        {
            if(g_pucScreenLinesY2[29] > 191)
            {
                g_pucScreenLinesY2[29] = 0;
                g_cScreenDeltaY2 = (RandomNumber() >> 30) + 1;
            }
            else
            {
                g_pucScreenLinesY2[29] = 95;
                g_cScreenDeltaY2 = (char)-1 - (RandomNumber() >> 30);
            }
        }

        //
        // Clear the local frame buffer.
        //
        for(ulLoop = 0; ulLoop < 6144; ulLoop += 4)
        {
            *((unsigned long *)(g_pucFrame + ulLoop)) = 0;
        }

        //
        // Loop through the lines in the history buffer.
        //
        for(ulLoop = 0; ulLoop < 30; ulLoop++)
        {
            //
            // Draw this line if it "exists".  If both end points are at 0,0
            // then the line is assumed to not exist (i.e. the line history at
            // the very start).  There is a tiny likelyhood that the two
            // endpoints will converge on 0,0 at the same time in which case
            // there will be a small anomaly (though it would be extremely
            // difficult to visually discern that it occurred).
            //
            if(g_pucScreenLinesX1[ulLoop] || g_pucScreenLinesY1[ulLoop] ||
               g_pucScreenLinesX2[ulLoop] || g_pucScreenLinesY2[ulLoop])
            {
                ScreenSaverLine(g_pucScreenLinesX1[ulLoop],
                                g_pucScreenLinesY1[ulLoop],
                                g_pucScreenLinesX2[ulLoop],
                                g_pucScreenLinesY2[ulLoop], (ulLoop / 2) + 1);
            }
        }

        //
        // Copy the local frame buffer to the display.
        //
        RIT128x96x4ImageDraw(g_pucFrame, 0, 0, 128, 96);
    }

    //
    // Clear the display and turn it off.
    //
    RIT128x96x4Clear();
    RIT128x96x4DisplayOff();

    //
    // Turn off the music.
    //
    AudioOff();

    //
    // Configure PWM0 to generate a 10 KHz signal for driving the user LED.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT | PWM_OUT_1_BIT, false);
    PWMGenDisable(PWM0_BASE, PWM_GEN_0);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
                    (PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC |
                     PWM_GEN_MODE_DBG_RUN));
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, g_ulSystemClock / 80000);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 2);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);

    //
    // Configure the user LED pin for hardware control, i.e. the PWM output
    // from the PWM.
    //
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);

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
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0,
                             ((((g_ulSystemClock / 80000) - 4) *
                               ulCount) / 16) + 2);
        }

        if(ulCount == 8)
        {
            //
            // Turn on the LED on the CAN Device board.
            //
            CANUpdateTargetLED(1, false);
        }

        //
        // Turn off the user LED in eight gradual steps.
        //
        if((ulCount > 32) && (ulCount <= 48))
        {
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0,
                             ((((g_ulSystemClock / 80000) - 4) *
                               (48 - ulCount)) / 16) + 2);
        }

        if(ulCount == 40)
        {
            //
            // Turn off the LED on the CAN Device board.
            //
            CANUpdateTargetLED(0, false);
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
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);

    //
    // Re-enable the music.
    //
    AudioOn();

    //
    // Turn on the display.
    //
    RIT128x96x4DisplayOn();
}
