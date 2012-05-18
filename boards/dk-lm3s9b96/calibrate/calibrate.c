//*****************************************************************************
//
// calibrate.c - Calibration routine for the touch screen driver.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Calibration for the Touch Screen (calibrate)</h1>
//!
//! The raw sample interface of the touch screen driver is used to compute the
//! calibration matrix required to convert raw samples into screen X/Y
//! positions.  The produced calibration matrix can be inserted into the touch
//! screen driver to map the raw samples into screen coordinates.
//!
//! The touch screen calibration is performed according to the algorithm
//! described by Carlos E. Videles in the June 2002 issue of Embedded Systems
//! Design.  It can be found online at
//! <a href="http://www.embedded.com/story/OEG20020529S0046">
//! http://www.embedded.com/story/OEG20020529S0046</a>.
//!
//
//*****************************************************************************
//
// The graphics context used by the application.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// Performs calibration of the touch screen.
//
//*****************************************************************************
int
main(void)
{
    long lIdx, lX1, lY1, lX2, lY2, lCount;
    long pplPoints[3][4];
    char pcBuffer[32];
    tRectangle sRect;

    //
    // Enable the PLL and clock the part at 50 MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 23;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontCm20);
    GrStringDrawCentered(&g_sContext, "calibrate", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 11, 0);

    //
    // Print the instructions across the middle of the screen in white with a
    // 20 point small-caps font.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrContextFontSet(&g_sContext, g_pFontCmsc20);
    GrStringDraw(&g_sContext, "Touch the box", -1, 0,
                 (GrContextDpyHeightGet(&g_sContext) / 2) - 10, 0);

    //
    // Set the points used for calibration based on the size of the screen.
    //
    pplPoints[0][0] = GrContextDpyWidthGet(&g_sContext) / 10;
    pplPoints[0][1] = (GrContextDpyHeightGet(&g_sContext) * 2)/ 10;
    pplPoints[1][0] = GrContextDpyWidthGet(&g_sContext) / 2;
    pplPoints[1][1] = (GrContextDpyHeightGet(&g_sContext) * 9) / 10;
    pplPoints[2][0] = (GrContextDpyWidthGet(&g_sContext) * 9) / 10;
    pplPoints[2][1] = GrContextDpyHeightGet(&g_sContext) / 2;

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Loop through the calibration points.
    //
    for(lIdx = 0; lIdx < 3; lIdx++)
    {
        //
        // Fill a white box around the calibration point.
        //
        GrContextForegroundSet(&g_sContext, ClrWhite);
        sRect.sXMin = pplPoints[lIdx][0] - 5;
        sRect.sYMin = pplPoints[lIdx][1] - 5;
        sRect.sXMax = pplPoints[lIdx][0] + 5;
        sRect.sYMax = pplPoints[lIdx][1] + 5;
        GrRectFill(&g_sContext, &sRect);

        //
        // Flush any cached drawing operations.
        //
        GrFlush(&g_sContext);

        //
        // Initialize the raw sample accumulators and the sample count.
        //
        lX1 = 0;
        lY1 = 0;
        lCount = -5;

        //
        // Loop forever.  This loop is explicitly broken out of when the pen is
        // lifted.
        //
        while(1)
        {
            //
            // Grab the current raw touch screen position.
            //
            lX2 = g_sTouchX;
            lY2 = g_sTouchY;

            //
            // See if the pen is up or down.
            //
            if((lX2 < g_sTouchMin) || (lY2 < g_sTouchMin))
            {
                //
                // The pen is up, so see if any samples have been accumulated.
                //
                if(lCount > 0)
                {
                    //
                    // The pen has just been lifted from the screen, so break
                    // out of the controlling while loop.
                    //
                    break;
                }

                //
                // Reset the accumulators and sample count.
                //
                lX1 = 0;
                lY1 = 0;
                lCount = -5;

                //
                // Grab the next sample.
                //
                continue;
            }

            //
            // Increment the count of samples.
            //
            lCount++;

            //
            // If the sample count is greater than zero, add this sample to the
            // accumulators.
            //
            if(lCount > 0)
            {
                lX1 += lX2;
                lY1 += lY2;
            }
        }

        //
        // Save the averaged raw ADC reading for this calibration point.
        //
        pplPoints[lIdx][2] = lX1 / lCount;
        pplPoints[lIdx][3] = lY1 / lCount;

        //
        // Erase the box around this calibration point.
        //
        GrContextForegroundSet(&g_sContext, ClrBlack);
        GrRectFill(&g_sContext, &sRect);
    }

    //
    // Clear the screen.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 24;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrRectFill(&g_sContext, &sRect);

    //
    // Indicate that the calibration data is being displayed.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrStringDraw(&g_sContext, "Calibration data:", -1, 0, 40, 0);

    //
    // Compute and display the M0 calibration value.
    //
    usprintf(pcBuffer, "M0 = %d",
             (((pplPoints[0][0] - pplPoints[2][0]) *
               (pplPoints[1][3] - pplPoints[2][3])) -
              ((pplPoints[1][0] - pplPoints[2][0]) *
               (pplPoints[0][3] - pplPoints[2][3]))));
    GrStringDraw(&g_sContext, pcBuffer, -1, 0, 80, 0);

    //
    // Compute and display the M1 calibration value.
    //
    usprintf(pcBuffer, "M1 = %d",
             (((pplPoints[0][2] - pplPoints[2][2]) *
               (pplPoints[1][0] - pplPoints[2][0])) -
              ((pplPoints[0][0] - pplPoints[2][0]) *
               (pplPoints[1][2] - pplPoints[2][2]))));
    GrStringDraw(&g_sContext, pcBuffer, -1, 0, 100, 0);

    //
    // Compute and display the M2 calibration value.
    //
    usprintf(pcBuffer, "M2 = %d",
             ((((pplPoints[2][2] * pplPoints[1][0]) -
                (pplPoints[1][2] * pplPoints[2][0])) * pplPoints[0][3]) +
              (((pplPoints[0][2] * pplPoints[2][0]) -
                (pplPoints[2][2] * pplPoints[0][0])) * pplPoints[1][3]) +
              (((pplPoints[1][2] * pplPoints[0][0]) -
                (pplPoints[0][2] * pplPoints[1][0])) * pplPoints[2][3])));
    GrStringDraw(&g_sContext, pcBuffer, -1, 0, 120, 0);

    //
    // Compute and display the M3 calibration value.
    //
    usprintf(pcBuffer, "M3 = %d",
             (((pplPoints[0][1] - pplPoints[2][1]) *
               (pplPoints[1][3] - pplPoints[2][3])) -
              ((pplPoints[1][1] - pplPoints[2][1]) *
               (pplPoints[0][3] - pplPoints[2][3]))));
    GrStringDraw(&g_sContext, pcBuffer, -1, 0, 140, 0);

    //
    // Compute and display the M4 calibration value.
    //
    usprintf(pcBuffer, "M4 = %d",
             (((pplPoints[0][2] - pplPoints[2][2]) *
               (pplPoints[1][1] - pplPoints[2][1])) -
              ((pplPoints[0][1] - pplPoints[2][1]) *
               (pplPoints[1][2] - pplPoints[2][2]))));
    GrStringDraw(&g_sContext, pcBuffer, -1, 0, 160, 0);

    //
    // Compute and display the M5 calibration value.
    //
    usprintf(pcBuffer, "M5 = %d",
             ((((pplPoints[2][2] * pplPoints[1][1]) -
                (pplPoints[1][2] * pplPoints[2][1])) * pplPoints[0][3]) +
              (((pplPoints[0][2] * pplPoints[2][1]) -
                (pplPoints[2][2] * pplPoints[0][1])) * pplPoints[1][3]) +
              (((pplPoints[1][2] * pplPoints[0][1]) -
                (pplPoints[0][2] * pplPoints[1][1])) * pplPoints[2][3])));
    GrStringDraw(&g_sContext, pcBuffer, -1, 0, 180, 0);

    //
    // Compute and display the M6 calibration value.
    //
    usprintf(pcBuffer, "M6 = %d",
             (((pplPoints[0][2] - pplPoints[2][2]) *
               (pplPoints[1][3] - pplPoints[2][3])) -
              ((pplPoints[1][2] - pplPoints[2][2]) *
               (pplPoints[0][3] - pplPoints[2][3]))));
    GrStringDraw(&g_sContext, pcBuffer, -1, 0, 200, 0);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&g_sContext);

    //
    // The calibration is complete.  Sit around and wait for a reset.
    //
    while(1)
    {
    }
}
