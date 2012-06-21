//*****************************************************************************
//
// graphics.c - Stellaris Graphics Library (grlib) handling module for
//              the Bluetooth SPP example application.
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

//*****************************************************************************
//
// Accelerometer drawing information.
//
//*****************************************************************************
typedef struct
{
    short sAccel[2];
    short sAccelOffset[2];
} tAccelInfo;

//*****************************************************************************
//
// Variables used to control the accelerometer scribble drawing.
//
//*****************************************************************************
static tAccelInfo g_sAccelInfo;
static tBoolean g_bClearAccelCanvas = true;
static short g_sXPosAccel;
static short g_sYPosAccel;

//*****************************************************************************
//
// Variables used to denote the currently displayed screen.
//
//*****************************************************************************
static tCanvasWidget *g_sCurrentScreen;

//*****************************************************************************
//
// Calculate the coordinates of a point within the rectangle provided which
// represents the latest acceleration reading from the eZ430-Chronos whose
// data we are currently displaying.
//
// The raw acceleration data is in the range [-128, 127].
//
//*****************************************************************************
static void
CalculateAccelPoint(tRectangle *pRect, long *plNewX, long *plNewY)
{
    tRectangle sRect;
    long lRawX, lRawY, lXCenter, lYCenter;

    //
    // Get the raw X and Y acceleration values adjusted for any offset that
    // has been set.  We reverse these since this offers a more realistic
    // scribbling experience on the display.
    //
    lRawX = (long)(g_sAccelInfo.sAccel[1] -
                   g_sAccelInfo.sAccelOffset[1]);
    lRawY = (long)(g_sAccelInfo.sAccel[0] -
                   g_sAccelInfo.sAccelOffset[0]);

    //
    // Get the rectangle corresponding to the drawing canvas and adjust it
    // to represent the interior drawing area.
    //
    sRect = *pRect;
    sRect.sXMax--;
    sRect.sYMax--;
    sRect.sXMin++;
    sRect.sYMin++;

    //
    // Determine the coordinates of the center of the drawing area.
    //
    lXCenter = (long)((pRect->sXMax + pRect->sXMin) / 2);
    lYCenter = (long)((pRect->sYMax + pRect->sYMin) / 2);

    //
    // Translate the values such that their origin is at the center of the
    // drawing area.  Although the accelerometer range is [-128, 127] and
    // our drawing area is likely to be smaller than this, we don't scale the
    // values since normal tilting of the watch doesn't generate particularly
    // large swings in the measured acceleration.  Instead we use the unscaled
    // value and clip to the edges of the drawing area.
    //
    lRawX += lXCenter;
    lRawY += lYCenter;

    //
    // Clip them to the display area.
    //
    lRawX = (lRawX < sRect.sXMin) ? sRect.sXMin : lRawX;
    lRawX = (lRawX >= sRect.sXMax) ? sRect.sXMax : lRawX;
    lRawY = (lRawY < sRect.sYMin) ? sRect.sYMin : lRawY;
    lRawY = (lRawY >= sRect.sYMax) ? sRect.sYMax : lRawY;

    //
    // Translate the X and Y values such that they are centered about the
    // midpoint of the drawing area.
    //
    *plNewX = lRawX;
    *plNewY = lRawY;
}

//*****************************************************************************
//
// Cause a repaint of the provided widget and its children.
// This function may be called to update the display at any time.
//
//*****************************************************************************
static void
UpdateDisplay(tWidget *psWidget)
{
    //
    // Actually perform the repaint.
    //
    WidgetPaint(psWidget);
}

//*****************************************************************************
//
// Update the display widgets showing the current accelerometer readings.
//
//*****************************************************************************
static void
UpdateAccelDisplay(void)
{
    unsigned long ulLoop;

    for(ulLoop = 0; ulLoop < 2; ulLoop++)
    {
        //
        // Write the value as a decimal number into the relevant buffer.  We
        // apply the offset here assuming it has been set by the user.
        //
        usprintf(g_pcAccStrings[ulLoop], "%d",
                 (g_sAccelInfo.sAccel[ulLoop] -
                  g_sAccelInfo.sAccelOffset[ulLoop]));
    }

    //
    // Tell the widget library to repaint all the indicator fields if the
    // display is currently showing them.  This will also update the canvas
    // used to show the accelerometer "scribble" since it is also a child of
    // g_sIndicators.
    //
    UpdateDisplay((tWidget *)&g_sIndicators);
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
// This function is called to configure the graphics and display a TI
// Image as well as initialize the status bar.
//
//*****************************************************************************
void
InitializeGraphics(void)
{
    //
    // Set the graphics driver to indicate that a EM2 board is used.
    //
    g_eDaughterType = DAUGHTER_EM2;

    //
    // Construct the string telling everyone what this demo is
    //
    usnprintf(g_pcMainPanel, MAX_MAIN_PANEL_STRING_LEN,
              "Bluetooth BlueMSP430 Demo");

    //
    // Note the current Screen as the Main Screen.
    //
    g_sCurrentScreen = &g_psMainPanel;

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
// Updates the screen to the main (default) screen.
//
//*****************************************************************************
void
SwitchToMainScreen(void)
{
    if(g_sCurrentScreen != &g_psMainPanel)
    {
        //
        // Remove the widgets from the current panel.
        //
        WidgetRemove((tWidget *)g_sCurrentScreen);

        //
        // Note the new Screen.
        //
        g_sCurrentScreen = &g_psMainPanel;

        //
        // Add the new panel's widgets into the active tree.
        //
        WidgetAdd((tWidget *)&g_sHeading, (tWidget *)g_sCurrentScreen);

        //
        // Repaint the screen to show the new widgets.
        //
        WidgetPaint((tWidget *)g_sCurrentScreen);
    }
}

//*****************************************************************************
//
// Updates the screen to the connected (accelerometer data) screen.
//
//*****************************************************************************
void
SwitchToAccelScreen(void)
{
    if(g_sCurrentScreen != &g_psAccelPanel)
    {
        //
        // Remove the widgets from the current panel.
        //
        WidgetRemove((tWidget *)g_sCurrentScreen);

        //
        // Note the new Screen.
        //
        g_sCurrentScreen = &g_psAccelPanel;

        //
        // Clear the acceleration scribble control.
        //
        g_bClearAccelCanvas = true;

        //
        // Add the new panel's widgets into the active tree.
        //
        WidgetAdd((tWidget *)&g_sHeading, (tWidget *)g_sCurrentScreen);

        //
        // Repaint the screen to show the new widgets.
        //
        WidgetPaint((tWidget *)g_sCurrentScreen);
    }
}

//*****************************************************************************
//
// Process an X-Y pair of accelerometer data, filtering it and updating the
// value to the screen.
//
//*****************************************************************************
void
ProcessAccelData(short sXData, short sYData)
{
    if(g_sCurrentScreen == &g_psAccelPanel)
    {
        //
        // Update the acceleration stored.  We use a simple filter to
        // smooth the accelerometer readings.
        //
#ifndef USE_UNFILTERED_ACCEL_VALUES
        g_sAccelInfo.sAccel[0] = ((g_sAccelInfo.sAccel[0] * 3) / 4) +
                                 ((sXData) / 4);
        g_sAccelInfo.sAccel[1] = ((g_sAccelInfo.sAccel[1] * 3) / 4) +
                                 ((sYData) / 4);
#else
        g_sAccelInfo.sAccel[0] = sXData;
        g_sAccelInfo.sAccel[1] = sYData;
#endif

        //
        // Update the display using the new acceleration values.
        //
        UpdateAccelDisplay();
   }
}

//*****************************************************************************
//
// Paint handlers for the canvas widget we use to display accelerometer values.
// This control merely draws lines between points corresponding to each raw
// (x,y) accelerometer reading, changing the color according to the Z value.
//
// Note that this is merely an indication of the acceleration reading and does
// not perform any rigorous mathematics to convert acceleration to position.
// Using the raw values allows us to generate a reasonably good "scribble"
// effect by tilting the device left or right, back or forward.
//
//*****************************************************************************
void
OnPaintAccelCanvas(tWidget *pWidget, tContext *pContext)
{
    tRectangle sRect;
    tCanvasWidget *pCanvas;
    unsigned long ulColor;
    long lNewX, lNewY;

    //
    // Get a pointer to the canvas structure.
    //
    pCanvas = (tCanvasWidget *)pWidget;

    //
    // Have we been asked to initialize the canvas?  If so, draw the border,
    // clear the main drawing area and reset the drawing position and color.
    //
    if(g_bClearAccelCanvas)
    {
        //
        // Remember that we've cleared the control.
        //
        g_bClearAccelCanvas = false;

        //
        // Determine the bounding rectangle of the canvas.
        //
        sRect = pWidget->sPosition;

        //
        // Outline the area in the required color.
        //
        GrContextForegroundSet(pContext, pCanvas->ulOutlineColor);
        GrRectDraw(pContext, &sRect);

        //
        // Adjust the rectangle to represent only the inner drawing area.
        //
        sRect.sXMin++;
        sRect.sXMax--;
        sRect.sYMin++;
        sRect.sYMax--;

        //
        // Clear the drawing surface.
        //
        GrContextForegroundSet(pContext, pCanvas->ulFillColor);
        GrRectFill(pContext, &sRect);

        //
        // Reset our drawing position.
        //
        g_sXPosAccel = (sRect.sXMax + sRect.sXMin) / 2;
        g_sYPosAccel = (sRect.sYMax + sRect.sYMin) / 2;
    }
    else
    {
        //
        // We've not been told to initialize the whole canvas so merely draw
        // a line between the last point we plotted and the latest acceleration
        // value read.

        //
        // Add blue to shift this to an orange or yellow value.  This prevents
        // the drawing from being black when no Z acceleration is registered.
        //
        ulColor = 0xFF00;

        //
        // Get the X and Y coordinates representing the new accelerometer
        // reading
        //
        CalculateAccelPoint(&pWidget->sPosition, &lNewX, &lNewY);

        //
        // Now draw a line using this color from the last point we plotted to
        // the new one.
        //
        GrContextForegroundSet(pContext, ulColor);
        GrLineDraw(pContext, (long)g_sXPosAccel, (long)g_sYPosAccel,
                   lNewX, lNewY);

        //
        // Remember the new drawing position.
        //
        g_sXPosAccel = (short)lNewX;
        g_sYPosAccel = (short)lNewY;
    }
}

//*****************************************************************************
//
// Button handler for the "Calibrate" button.  This uses the current readings
// from the accelerometer to set the zero point for future measurement.
//
//*****************************************************************************
void
OnCalibrateButtonPress(tWidget *pWidget)
{
    unsigned long ulLoop;

    //
    // Copy the current accelerometer readings into the offset fields to use
    // as the origin when reading future values.
    //
    for(ulLoop = 0; ulLoop < 2; ulLoop++)
    {
        g_sAccelInfo.sAccelOffset[ulLoop] = g_sAccelInfo.sAccel[ulLoop];
    }

    //
    // Update the display and clear the current scribble.
    //
    g_bClearAccelCanvas = true;
    UpdateAccelDisplay();
}

//*****************************************************************************
//
// Button handler for the "Clear" button.  This clears the area of the screen
// which is drawn on by moving the eZ430-Chronos watch while in accelerometer
// mode.
//
//*****************************************************************************
void
OnClearButtonPress(tWidget *pWidget)
{
    //
    // Tell the acceleration scribble canvas to clear itself.
    //
    g_bClearAccelCanvas = true;
    WidgetPaint((tWidget *)&g_sDrawingCanvas);
}
