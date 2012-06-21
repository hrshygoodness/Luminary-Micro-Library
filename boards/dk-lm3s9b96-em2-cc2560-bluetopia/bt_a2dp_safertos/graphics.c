//*****************************************************************************
//
// graphics.c - Stellaris Graphics Library (grlib) handling module for
//              the Bluetooth A2DP example application.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2560-BLUETOPIA Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/container.h"
#include "grlib/imgbutton.h"
#include "drivers/set_pinout.h"
#include "drivers/touch.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "utils/ustdlib.h"
#include "widgets.h"
#include "graphics.h"

//*****************************************************************************
//
// The error Callback function that is called when a button press is
// detected.
//
//*****************************************************************************
static void (*g_sButtonPressCallback)(unsigned int);

//*****************************************************************************
//
// This function is called to process the widget message queue.
//
//*****************************************************************************
void
ProcessGraphics(void)
{
    //
    // Process Widget Message Queue.
    //
    WidgetMessageQueueProcess();
}

//*****************************************************************************
//
// Updates the status string on the display.
//
//*****************************************************************************
void
UpdateStatusBox(const char *pcString)
{
    //
    // If a non-null pointer is passed, copy the string to the status
    // string buffer.
    //
    if(pcString)
    {
        usnprintf(g_pcStatus, MAX_STATUS_STRING_LEN, "%s", pcString);
        g_pcStatus[MAX_STATUS_STRING_LEN - 1] = '\0';
    }

    //
    // Otherwise just set the status to the empty string.
    //
    else
    {
        g_pcStatus[0] = '\0';
    }

    //
    // Update the status string on the display.
    //
    WidgetPaint((tWidget *)&g_sMainStatus);
}

//*****************************************************************************
//
// Button handler for all buttons button.  This function determines the
// button that was pressed and calls the registered callback function with
// the correct information.
//
//*****************************************************************************
void
OnButtonPress(tWidget *pWidget)
{
    unsigned int uiButtonPress = 0;

    //
    // First, determine the button that was pressed.
    //
    if(pWidget == (tWidget *)&g_sPlayBtn)
    {
        uiButtonPress = BUTTON_PRESS_PLAY;
    }
    else if(pWidget == (tWidget *)&g_sPauseBtn)
    {
        uiButtonPress = BUTTON_PRESS_PAUSE;
    }
    else if(pWidget == (tWidget *)&g_sNextBtn)
    {
        uiButtonPress = BUTTON_PRESS_NEXT;
    }
    else if(pWidget == (tWidget *)&g_sBackBtn)
    {
        uiButtonPress = BUTTON_PRESS_BACK;
    }

    //
    // If a button was pressed AND a callback was registered, go ahead
    // and make the callback.
    //
    if(uiButtonPress && g_sButtonPressCallback)
    {
        (*g_sButtonPressCallback)(uiButtonPress);
    }
}

//*****************************************************************************
//
// This function is called to configure the graphics and display a TI
// Image as well as initialize the status bar.
//
//*****************************************************************************
void
InitializeGraphics(void (*sButtonPressCallback)(unsigned int))
{
    //
    // Set the graphics driver to indicate that a EM2 board is used.
    //
    g_eDaughterType = DAUGHTER_EM2;

    //
    // Note the Button Press Callback function.
    //
    g_sButtonPressCallback = sButtonPressCallback;

    //
    // Construct the string telling everyone what this demo is
    //
    usnprintf(g_pcMainPanel, MAX_MAIN_PANEL_STRING_LEN, "Bluetooth A2DP Demo");

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Initialize the status string.  We will let the main application
    // handle the value of this box.
    //
    UpdateStatusBox("");

    //
    //
    // Paint the widget tree to make sure they all appear on the display. We
    // do this here to ensure that the display shows something.
    //
    WidgetPaint(WIDGET_ROOT);
    WidgetMessageQueueProcess();
}

