//*****************************************************************************
//
// grlib_demo.c - Demonstration of the Stellaris Graphics Library.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "grlib/radiobutton.h"
#include "drivers/formike128x128x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Graphics Library Demonstration (grlib_demo)</h1>
//!
//! This application provides a demonstration of the capabilities of the
//! Stellaris Graphics Library.  The display will be configured to demonstrate
//! the available drawing primitives: lines, circles, rectangles, strings, and
//! images.
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
// The image of the TI logo.
//
//*****************************************************************************
const unsigned char g_pucLogo[] =
{
    IMAGE_FMT_4BPP_COMP,
    30, 0,
    30, 0,

    15,
    0x00, 0x00, 0x00,
    0x03, 0x02, 0x12,
    0x06, 0x05, 0x2b,
    0x0a, 0x08, 0x43,
    0x0d, 0x0a, 0x57,
    0x10, 0x0d, 0x69,
    0x12, 0x0e, 0x76,
    0x14, 0x10, 0x87,
    0x17, 0x12, 0x96,
    0x19, 0x14, 0xa6,
    0x1b, 0x15, 0xb1,
    0x1d, 0x17, 0xbe,
    0x1e, 0x18, 0xc8,
    0x21, 0x19, 0xd7,
    0x23, 0x1b, 0xe4,
    0x24, 0x1c, 0xed,

    0x84, 0x02, 0x79, 0x88, 0x8a, 0x50, 0x07, 0x00, 0x00, 0x08, 0xdf, 0xff,
    0xff, 0x80, 0x07, 0x00, 0x00, 0xbf, 0x90, 0x8a, 0x35, 0x30, 0x8f, 0xff,
    0xff, 0x70, 0x01, 0x31, 0xef, 0xa0, 0x8f, 0x89, 0x03, 0xff, 0x60, 0x17,
    0x90, 0x12, 0x33, 0x10, 0x17, 0xff, 0xff, 0xca, 0x13, 0x04, 0x98, 0x16,
    0xa9, 0x9a, 0x60, 0x16, 0xff, 0x18, 0x04, 0xfd, 0x1d, 0xff, 0xff, 0x90,
    0x16, 0xfc, 0x0b, 0x04, 0xf7, 0x2f, 0xff, 0xff, 0x80, 0x15, 0xfd, 0x84,
    0x08, 0x1e, 0xf5, 0x28, 0xbf, 0x8f, 0xf7, 0x00, 0x4f, 0x00, 0xf4, 0x00,
    0x6f, 0xff, 0x90, 0x00, 0x67, 0x66, 0x0a, 0x66, 0x66, 0xdf, 0xff, 0xa1,
    0xf2, 0x51, 0xe2, 0x00, 0x00, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6,
    0x00, 0x30, 0x9f, 0xb0, 0x34, 0xef, 0xff, 0xfc, 0x20, 0x42, 0x0b, 0x8b,
    0xff, 0xd0, 0xbf, 0x71, 0x42, 0x80, 0x22, 0x01, 0xbf, 0x0b, 0x82, 0xef,
    0x42, 0x42, 0x70, 0x22, 0x00, 0x1b, 0x0b, 0x42, 0xff, 0x35, 0x8c, 0x02,
    0x89, 0x13, 0x25, 0xff, 0x1a, 0x14, 0x00, 0xaf, 0x09, 0x04, 0xfe, 0x24,
    0x86, 0x04, 0x8f, 0x09, 0x60, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xc5, 0x8f,
    0xfb, 0x00, 0x00, 0x00, 0x00, 0x2f, 0xff, 0xfd, 0x73, 0x10, 0x00, 0x00,
    0x04, 0x07, 0xfc, 0x10, 0x09, 0xfc, 0x89, 0x5f, 0xfe, 0x40, 0x51, 0x59,
    0x00, 0x00, 0x21, 0x00, 0x01, 0xef, 0x06, 0x72, 0x22, 0x21, 0x9f, 0x92,
    0x93, 0x6a, 0x7f, 0x08, 0xff, 0xee, 0xee, 0xfa, 0x97, 0x00, 0x2f, 0xff,
    0x12, 0xff, 0xff, 0xd1, 0x8f, 0x00, 0x08, 0x89, 0x50, 0x94, 0x17, 0x00,
    0x02, 0x11, 0x20, 0x17, 0x00, 0x00, 0x61, 0x4f, 0x8f, 0x03, 0x05, 0xff,
    0xff, 0x50, 0x17, 0x8c, 0x01, 0x3a, 0xdd, 0x60, 0x8f, 0x01, 0x04, 0x88,
    0x70, 0x40, 0x17, 0x47, 0x77
};

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
// A simple demonstration of the features of the Stellaris Graphics Library.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIdx;
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
    GrStringDrawCentered(&g_sContext, "grlib_demo", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Draw a vertical sweep of lines from red to green.
    //
    for(ulIdx = 0; ulIdx <= 10; ulIdx++)
    {
        GrContextForegroundSet(&g_sContext,
                               (((((10 - ulIdx) * 255) / 10) << ClrRedShift) |
                                (((ulIdx * 255) / 10) << ClrGreenShift)));
        GrLineDraw(&g_sContext, 62, 70, 2, 70 - (5 * ulIdx));
    }

    //
    // Draw a horizontal sweep of lines from green to blue.
    //
    for(ulIdx = 1; ulIdx <= 10; ulIdx++)
    {
        GrContextForegroundSet(&g_sContext,
                               (((((10 - ulIdx) * 255) / 10) <<
                                 ClrGreenShift) |
                                (((ulIdx * 255) / 10) << ClrBlueShift)));
        GrLineDraw(&g_sContext, 62, 70, 2 + (ulIdx * 6), 20);
    }

    //
    // Draw a filled circle with an overlapping circle.
    //
    GrContextForegroundSet(&g_sContext, ClrBrown);
    GrCircleFill(&g_sContext, 88, 37, 17);
    GrContextForegroundSet(&g_sContext, ClrSkyBlue);
    GrCircleDraw(&g_sContext, 104, 45, 17);

    //
    // Draw a filled rectangle with an overlapping rectangle.
    //
    GrContextForegroundSet(&g_sContext, ClrSlateGray);
    sRect.sXMin = 4;
    sRect.sYMin = 84;
    sRect.sXMax = 42;
    sRect.sYMax = 104;
    GrRectFill(&g_sContext, &sRect);
    GrContextForegroundSet(&g_sContext, ClrSlateBlue);
    sRect.sXMin += 12;
    sRect.sYMin += 15;
    sRect.sXMax += 12;
    sRect.sYMax += 15;
    GrRectDraw(&g_sContext, &sRect);

    //
    // Draw a piece of text in fonts of increasing size.
    //
    GrContextForegroundSet(&g_sContext, ClrSilver);
    GrStringDraw(&g_sContext, "Strings", -1, 75, 114, 0);

    //
    // Draw an image.
    //
    GrImageDraw(&g_sContext, g_pucLogo, 80, 77);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&g_sContext);

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
