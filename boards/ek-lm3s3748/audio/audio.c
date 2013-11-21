//*****************************************************************************
//
// audio.c - Audio playback example.
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
// This is part of revision 10636 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "drivers/buttons.h"
#include "drivers/class-d.h"
#include "drivers/formike128x128x16.h"
#include "adpcm.h"
#include "pcm.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Audio Playback (audio)</h1>
//!
//! This example application plays audio via the Class-D amplifier and speaker.
//! The same source audio clip is provided in both PCM and ADPCM format so that
//! the audio quality can be compared.
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
// Handles the SysTick timeout interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned char ucButtons, ucDelta, ucRepeat;

    //
    // Get the debounced button states.
    //
    ucButtons = ButtonsPoll(&ucDelta, &ucRepeat);

    //
    // See if the up button was just pressed.
    //
    if(BUTTON_PRESSED(UP_BUTTON, ucButtons, ucDelta) ||
       BUTTON_REPEAT(UP_BUTTON, ucRepeat))
    {
        //
        // Increase the volume of the playback.
        //
        ClassDVolumeUp(16);
    }

    //
    // See if the down button was just pressed.
    //
    if(BUTTON_PRESSED(DOWN_BUTTON, ucButtons, ucDelta) ||
       BUTTON_REPEAT(DOWN_BUTTON, ucRepeat))
    {
        //
        // Decrease the volume of the playback.
        //
        ClassDVolumeDown(16);
    }

    //
    // See if the left button was just pressed.
    //
    if(BUTTON_PRESSED(LEFT_BUTTON, ucButtons, ucDelta))
    {
        //
        // Start playback of the PCM stream.
        //
        ClassDPlayPCM(g_pucPCMData, sizeof(g_pucPCMData));
    }

    //
    // See if the right button was just pressed.
    //
    if(BUTTON_PRESSED(RIGHT_BUTTON, ucButtons, ucDelta))
    {
        //
        // Start playback of the ADPCM stream.
        //
        ClassDPlayADPCM(g_pucADPCMData, sizeof(g_pucADPCMData));
    }

    //
    // See if the select button was just pressed.
    //
    if(BUTTON_PRESSED(SELECT_BUTTON, ucButtons, ucDelta))
    {
        //
        // Stop playback.
        //
        ClassDStop();
    }
}

//*****************************************************************************
//
// This example demonstrates the use of bit-banding to set individual bits
// within a word of SRAM.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;

    //
    // Set the clocking to run from the PLL at 50MHz
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_8MHZ);
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    //
    // Initialize the display driver.
    //
    Formike128x128x16Init();

    //
    // Initialize the front panel button driver.
    //
    ButtonsInit();

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
    GrStringDrawCentered(&g_sContext, "audio", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Print out the directions.
    //
    GrStringDrawCentered(&g_sContext, "Audio Playback via", -1,
                         (sRect.sXMax / 2), 30, false);
    GrStringDrawCentered(&g_sContext, "Class-D Amplifier", -1,
                         (sRect.sXMax / 2), 40, false);
    GrStringDrawCentered(&g_sContext, "Press Up for Vol+", -1,
                         (sRect.sXMax / 2), 50, false);
    GrStringDrawCentered(&g_sContext, "Press Down for Vol-", -1,
                         (sRect.sXMax / 2), 60, false);
    GrStringDrawCentered(&g_sContext, "Press Left for PCM", -1,
                         (sRect.sXMax / 2), 70, false);
    GrStringDrawCentered(&g_sContext, "Press Right for ADPCM", -1,
                         (sRect.sXMax / 2), 80, false);
    GrStringDrawCentered(&g_sContext, "Press Select to stop", -1,
                         (sRect.sXMax / 2), 90, false);

    //
    // Initialize the Class-D amplifier driver.
    //
    ClassDInit(ROM_SysCtlClockGet());

    //
    // Start playback of the PCM stream.
    //
    ClassDPlayPCM(g_pucPCMData, sizeof(g_pucPCMData));

    //
    // Set up and enable SysTick.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 200);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
