//*****************************************************************************
//
// menu.c - Displays the system menu.
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
// This is part of revision 8555 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "bdc-ui.h"
#include "menu.h"
#include "rit128x96x4.h"

//*****************************************************************************
//
// The widgets that make up the system menu.
//
//*****************************************************************************
static tCanvasWidget g_psMenuWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "Voltage Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 8, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "VComp Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 16, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "Current Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 24, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "Speed Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 32, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "Position Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 40, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "Configuration", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 48, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "Device List", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 56, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "Firmware Update", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 64, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "Help", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 72, 128, 8,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "About", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 80, 128, 4,
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 81, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the system menu.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psMenuWidgets) /                    \
                                 sizeof(g_psMenuWidgets[0]))

//*****************************************************************************
//
// Displays the system menu.  The returned value is the ID of the panel to be
// displayed.
//
//*****************************************************************************
unsigned long
DisplayMenu(unsigned long ulPanel)
{
    unsigned long ulIdx;

    //
    // Disable the widget fill from all the widgets except the one for current
    // panel.
    //
    for(ulIdx = 0; ulIdx < NUM_PANELS; ulIdx++)
    {
        CanvasFillColorSet(g_psMenuWidgets + ulIdx, ClrBlack);
    }
    CanvasFillColorSet(g_psMenuWidgets + ulPanel, ClrSelected);

    //
    // Add the system menu widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psMenuWidgets + ulIdx));
    }

    //
    // Loop forever.  This loop will be explicitly exited when the proper
    // condition is detected.
    //
    while(1)
    {
        //
        // Update the display.
        //
        DisplayFlush();

        //
        // Wait until the up, down, or select button is pressed, or a serial
        // download begins.
        //
        while((HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 0))
        {
        }

        //
        // See if a serial download has begun.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 1)
        {
            //
            // Set the panel to the update panel.
            //
            ulPanel = PANEL_UPDATE;

            //
            // Break out of the infinite loop.
            //
            break;
        }

        //
        // See if the up button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the top of the
            // system menu.
            //
            if(ulPanel != 0)
            {
                //
                // Disable the widget fill from the currently selected widget.
                //
                CanvasFillColorSet(g_psMenuWidgets + ulPanel, ClrBlack);

                //
                // Decrement the system menu entry.
                //
                ulPanel--;

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillColorSet(g_psMenuWidgets + ulPanel, ClrSelected);
            }

            //
            // Clear the press flag for the up button.
            //
            HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) = 0;
        }

        //
        // See if the down button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the bottom of the
            // system menu.
            //
            if(ulPanel != (NUM_PANELS - 1))
            {
                //
                // Disable the widget fill from the currently selected widget.
                //
                CanvasFillColorSet(g_psMenuWidgets + ulPanel, ClrBlack);

                //
                // Increment the system menu entry.
                //
                ulPanel++;

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillColorSet(g_psMenuWidgets + ulPanel, ClrSelected);
            }

            //
            // Clear the press flag for the down button.
            //
            HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
        }

        //
        // See if the select button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) == 1)
        {
            //
            // Clear the press flag for the select button.
            //
            HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) = 0;

            //
            // Break out of the infinite loop.
            //
            break;
        }
    }

    //
    // Remove the system menu widgets.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetRemove((tWidget *)(g_psMenuWidgets + ulIdx));
    }

    //
    // Clear the press flag for the left and right buttons.
    //
    HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
    HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) = 0;
    HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) = 0;
    HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) = 0;
    HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
    HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) = 0;
    HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) = 0;
    HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) = 0;

    //
    // Return the ID of the selected menu item.
    //
    return(ulPanel);
}
