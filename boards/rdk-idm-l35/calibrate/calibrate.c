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
// This is part of revision 9453 of the RDK-IDM-L35 Firmware Package.
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
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119.h"
#include "drivers/touch.h"

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
//! This application supports remote software update over serial using the
//! LM Flash Programmer application.  The application transfers control to the
//! boot loader whenever it completes to allow a new image to be downloaded if
//! required.  The LMFlash serial data rate must be set to 115200bps and the
//! "Program Address Offset" to 0x800.
//!
//! UART0, which is connected to the 6 pin header on the underside of the
//! IDM-L35 RDK board (J8), is configured for 115200bps, and 8-n-1 mode.  The
//! USB-to-serial cable supplied with the IDM-L35 RDK may be used to connect
//! this TTL-level UART to the host PC to allow firmware update.
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
// Passes control to the bootloader and initiates a remote software update
// over the serial connection.
//
// This function passes control to the bootloader and initiates an update of
// the main application firmware image via UART0.
//
// \return Never returns.
//
//*****************************************************************************
void
JumpToBootLoader(void)
{
    //
    // Disable all processor interrupts.  Instead of disabling them
    // one at a time (and possibly missing an interrupt if new sources
    // are added), a direct write to NVIC is done to disable all
    // peripheral interrupts.
    //
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;

    //
    // We need to make sure that UART0 and its associated GPIO port are
    // enabled before we pass control to the boot loader.  The boot loader
    // does not enable or configure these peripherals for us if we enter it
    // via its SVC vector.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Set GPIO A0 and A1 as UART.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115200, n, 8, 1
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_WLEN_8));

    //
    // Enable the UART operation.
    //
    UARTEnable(UART0_BASE);

    //
    // Return control to the boot loader.  This is a call to the SVC
    // handler in the boot loader.
    //
    (*((void (*)(void))(*(unsigned long *)0x2c)))();
}

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
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Enable the PLL and clock the part at 50 MHz.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Turn on the backlight.
    //
    Kitronix320x240x16_SSD2119BacklightOn(255);

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
            if((lX2 < 100) || (lY2 < 100))
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
    // The calibration is complete. Jump into the boot loader and wait for a
    // firmware update.
    //
    JumpToBootLoader();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
