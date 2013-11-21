//*****************************************************************************
//
// calibrate.c - Calibration routine for the touch screen driver.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/locator.h"

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
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.
//
//*****************************************************************************

//*****************************************************************************
//
// The graphics context used by the application.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

//*****************************************************************************
//
// The display coordinates at which the IP address will be shown.
//
//*****************************************************************************
#define IP_ADDR_LEFT 200
#define IP_ADDR_TOP  230

//*****************************************************************************
//
// The display coordinates at which the MAC address will be shown.
//
//*****************************************************************************
#define MAC_ADDR_LEFT 30
#define MAC_ADDR_TOP  230

//*****************************************************************************
//
// The size of buffer used to render the Ethernet MAC address string.
//
//*****************************************************************************
#define SIZE_MAC_ADDR_BUFFER 32

//*****************************************************************************
//
// A signal used to tell the main loop to transfer control to the boot loader
// so that a firmware update can be performed over Ethernet.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// This function is called by the swupdate module whenever it receives a
// signal indicating that a remote firmware update request is being made.  This
// notification occurs in the context of the Ethernet interrupt handler so it
// is vital that you DO NOT transfer control to the boot loader directly from
// this function (since the boot loader does not like being entered in interrupt
// context).
//
//*****************************************************************************
void
SoftwareUpdateRequestCallback(void)
{
    //
    // Set the flag that tells the main task to transfer control to the boot
    // loader.
    //
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  We use this to provide the
// required timer call to the lwIP stack.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
}

//*****************************************************************************
//
// Initialize the Ethernet hardware and lwIP TCP/IP stack and set up to listen
// for remote firmware update requests.
//
//*****************************************************************************
unsigned long
TCPIPStackInit(void)
{
    char pcMACAddrString [SIZE_MAC_ADDR_BUFFER];
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACAddr[6];

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Configure the Ethernet LEDs on PF2 and PF3.
    //  LED0        Bit 3   Output
    //  LED1        Bit 2   Output
    //
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Get the MAC address from the UART0 and UART1 registers in NV ram.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);

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
    // Format this address into a string and display it.
    //
    usnprintf(pcMACAddrString, SIZE_MAC_ADDR_BUFFER,
              "MAC: %02X-%02X-%02X-%02X-%02X-%02X",
              pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
              pucMACAddr[4], pucMACAddr[5]);
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);
    GrStringDraw(&g_sContext, pcMACAddrString, -1, MAC_ADDR_LEFT,
                 MAC_ADDR_TOP, true);

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM-SBC calibrate");

    //
    // Start monitoring for the special packet that tells us a software
    // download is being requested.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Return our initial IP address.  This is 0 for now since we have not
    // had one assigned yet.
    //
    return(0);
}

//*****************************************************************************
//
// Check to see if the IP address has changed and, if so, update the
// display.
//
//*****************************************************************************
unsigned long IPAddressChangeCheck(unsigned long ulCurrentIP)
{
    unsigned long ulIPAddr;
    char pcIPAddrString[24];

    //
    // What is our current IP address?
    //
    ulIPAddr = lwIPLocalIPAddrGet();

    //
    // Has the IP address changed?
    //
    if(ulIPAddr != ulCurrentIP)
    {
        //
        // Yes - the address changed so update the display.
        //
        usprintf(pcIPAddrString, "IP: %d.%d.%d.%d",
                 ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                 (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
        GrContextFontSet(&g_sContext, g_pFontFixed6x8);
        GrStringDraw(&g_sContext, pcIPAddrString, -1, IP_ADDR_LEFT,
                     IP_ADDR_TOP, true);
    }

    //
    // Return our current IP address.
    //
    return(ulIPAddr);
}

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
    unsigned long ulLastIPAddr;

    //
    // Enable the PLL and clock the part at 50 MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout correctly for the IDM-SBC board.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Turn on the display backlight at full brightness.
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
    // Initialize the Ethernet hardware and lwIP TCP/IP stack.
    //
    ulLastIPAddr = TCPIPStackInit();

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
        // lifted or a software upgrade request is received.
        //
        while(!g_bFirmwareUpdate)
        {
            //
            // See if we have an IP address assigned and update the
            // display if necessary.
            //
            ulLastIPAddr = IPAddressChangeCheck(ulLastIPAddr);

            //
            // Grab the current raw touch screen position.
            //
            lX2 = g_sTouchX;
            lY2 = g_sTouchY;

            //
            // See if the pen is up or down.
            //
            if((lX2 < TOUCH_MIN) || (lY2 < TOUCH_MIN))
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
        // If we got a firmware update request, drop out immediately.
        //
        if(g_bFirmwareUpdate)
        {
            break;
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
    // Reset the colors and font.
    //
    GrContextFontSet(&g_sContext, g_pFontCmsc20);
    GrContextBackgroundSet(&g_sContext, ClrBlack);

    //
    // Clear the screen.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 24;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = MAC_ADDR_TOP - 1;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);

    //
    // Draw in white from here.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);

    //
    // Only do the calculation if we exited the loop normally.  If a firmware
    // update request was received, skip this.
    //
    if(!g_bFirmwareUpdate)
    {
        //
        // Indicate that the calibration data is being displayed.
        //
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
    }
    else
    {
        GrStringDrawCentered(&g_sContext, "Firmware Update...", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2,
                             GrContextDpyHeightGet(&g_sContext) / 2, false);
    }

    //
    // Now that everything is complete, transfer control to the boot loader
    // to allow a firmware update to take place.  We do this even if we did
    // not receive an explicit indication that an update is pending.  Note that
    // this function call will not return.
    //
    SoftwareUpdateBegin();
}
