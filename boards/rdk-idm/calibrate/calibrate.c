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
// This is part of revision 8555 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/ustdlib.h"
#include "drivers/formike240x320x16_ili9320.h"
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
//! described by Carlos E. Vidales in the June 2002 issue of Embedded Systems
//! Design.  It can be found on-line at
//! <a href="http://www.embedded.com/story/OEG20020529S0046">
//! http://www.embedded.com/story/OEG20020529S0046</a>.
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.  This
//! feature is available in versions of LM Flash Programmer with build numbers
//! greater than 560.
//
//*****************************************************************************
//
// The graphics context used by the application.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// A global flag used to indicate if a remote firmware update request has been
// received.
//
//*****************************************************************************
static volatile tBoolean g_bFirmwareUpdate = false;

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
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
}

//*****************************************************************************
//
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the bootloader to be entered the next time the user enters a
// command on the console.
//
//*****************************************************************************
void SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// Transfer control to the bootloader to wait for an ethernet-based firmware
// update to occur.
//
//*****************************************************************************
void
UpdateFirmware(void)
{
    //
    // Tell the user what's up
    //
    GrStringDraw(&g_sContext, "Updating firmware...", -1, 0, 290, true);

    //
    // Transfer control to the bootloader.
    //
    SoftwareUpdateBegin();
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
    unsigned long ulUser0, ulUser1, ulLastIPAddr, ulIPAddr;
    unsigned char pucMACAddr[6];

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
    // Enable SysTick to provide a periodic interrupt.  This is used to provide
    // a tick for the TCP/IP stack.
    //
    SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Get the MAC address from the user registers in NV ram.
    //
    FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV ram into a MAC address
    // array.
    //
    pucMACAddr[0] = ulUser0 & 0xff;
    pucMACAddr[1] = (ulUser0 >> 8) & 0xff;
    pucMACAddr[2] = (ulUser0 >> 16) & 0xff;
    pucMACAddr[3] = ulUser1 & 0xff;
    pucMACAddr[4] = (ulUser1 >> 8) & 0xff;
    pucMACAddr[5] = (ulUser1 >> 16) & 0xff;

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM calibrate");

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Initialize the display driver.
    //
    Formike240x320x16_ILI9320Init();

    //
    // Turn on the backlight.
    //
    Formike240x320x16_ILI9320BacklightOn();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike240x320x16_ILI9320);

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
    pplPoints[0][1] = GrContextDpyHeightGet(&g_sContext) / 10;
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
        // lifted or a firmware update request is received.
        //
        while(!g_bFirmwareUpdate)
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
        // Was a firmware update requested during the time we were
        // sampling?
        //
        if(g_bFirmwareUpdate)
        {
            UpdateFirmware();
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
    // Display the IP and MAC addresses.
    //
    GrStringDraw(&g_sContext, "IP: Not Assigned              ", -1, 0, 240,
                 true);
    usprintf(pcBuffer,
             "MAC: %02x-%02x-%02x-%02x-%02x-%02x",
             pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
             pucMACAddr[4], pucMACAddr[5]);
    GrStringDraw(&g_sContext, pcBuffer, -1, 0, 260, true);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&g_sContext);

    //
    // Clear our current IP address to force a display update when we are
    // assigned one (or when we read the one that has already been assigned)
    //
    ulLastIPAddr = 0;

    //
    // The calibration is complete.  Loop forever unless a remote firmware
    // update request is received.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // What is our current IP address?
        //
        ulIPAddr = lwIPLocalIPAddrGet();

        //
        // If it changed, update the display.
        //
        if(ulIPAddr != ulLastIPAddr)
        {
            ulLastIPAddr = ulIPAddr;

            usprintf(pcBuffer, "IP: %d.%d.%d.%d          ",
                     ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                     (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
            GrStringDraw(&g_sContext, pcBuffer, -1, 0, 240, true);
        }
    }

    //
    // The previous loop only exits if a firmware update request is received
    // so go ahead and process this.
    //
    UpdateFirmware();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
