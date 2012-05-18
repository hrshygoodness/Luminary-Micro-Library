//*****************************************************************************
//
// usb_boot_demo1.c - First USB boot loader example.
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
// This is part of revision 8555 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "drivers/formike128x128x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Boot Loader Demo 1 (usb_boot_demo1)</h1>
//!
//! An example to demonstrate the use of the flash-based USB boot loader.  At
//! startup, the application displays a message then branches to the USB boot
//! loader to await the start of an update.  The boot loader presents a
//! Device Firmware Upgrade interface to the host allowing new applications
//! to be downloaded to flash via USB.
//!
//! The usb_boot_demo2 application can be used along with this application to
//! easily demonstrate that the boot loader is actually updating the on-chip
//! flash.
//!
//! The application dfuwrap, found in the boards directory, can be used to
//! prepare binary images for download to a particular position in device
//! flash.  This application adds a Stellairs-specific prefix and a DFU standard
//! suffix to the binary. A sample Windows command line application, dfuprog,
//! is also provided which allows either binary images or DFU-wrapped files to
//! be downloaded to the board or uploaded from it.
//!
//! The usb_boot_demo1 and usb_boot_demo2 applications are essentially
//! identical to boot_demo1 and boot_demo2 with the exception that they are
//! linked to run at address 0x1800 rather than 0x0.  This is due to the fact
//! that the USB boot loader is not currently included in the Stellaris ROM
//! and therefore has to be stored in the bottom few KB of flash with the
//! main application stored above it.
//
//*****************************************************************************

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
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
// Demonstrate the use of the boot loader.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;

    //
    // Set the clocking to run from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_8MHZ);

    //
    // Initialize the display driver.
    //
    Formike128x128x16Init();

    //
    // Turn on the backlight.
    //
    Formike128x128x16BacklightOn();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike128x128x16);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 14;
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
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "usb_boot_demo1", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Indicate what is happening.
    //
    GrStringDrawCentered(&g_sContext, "The boot loader is", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 24, 0);
    GrStringDrawCentered(&g_sContext, "now running and", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 32, 0);
    GrStringDrawCentered(&g_sContext, "awaiting an update", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 40, 0);
    GrStringDrawCentered(&g_sContext, "over USB.", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 48, 0);

    //
    // Call the boot loader so that it will listen for an update via USB.
    //
    (*((void (*)(void))(*(unsigned long *)0x2c)))();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
