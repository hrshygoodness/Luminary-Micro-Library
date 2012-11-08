//*****************************************************************************
//
// about.c - Displays the "About" panel.
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
// This is part of revision 9453 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "about.h"
#include "bdc-ui.h"
#include "menu.h"
#include "rit128x96x4.h"
#include "splash.h"

//*****************************************************************************
//
// The widgets that make up the "About" panel.
//
//*****************************************************************************
static tCanvasWidget g_psAboutWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT | CANVAS_STYLE_FILL, ClrSelected, 0,
                 ClrWhite, g_pFontFixed6x8, "About", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 12, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 20, 128, 56,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucSplashImage, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 80, 128, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Brushed DC Motor", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 88, 128, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Reference Design Kit", 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the "About" panel.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psAboutWidgets) /                   \
                                 sizeof(g_psAboutWidgets[0]))

//*****************************************************************************
//
// Displays the "About" panel.  The returned value is the ID of the panel to be
// displayed instead of the "About" panel.
//
//*****************************************************************************
unsigned long
DisplayAbout(void)
{
    unsigned long ulIdx, ulPanel;

    //
    // Add the "About" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psAboutWidgets + ulIdx));
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
        // Wait until the select button is pressed or a serial download begins.
        //
        while((HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 0))
        {
        }

        //
        // Clear the press flags for the up, down, left, and right buttons.
        //
        HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) = 0;
        HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
        HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
        HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;

        //
        // See if a serial download has begun.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 1)
        {
            //
            // Remove the "About" panel widgets.
            //
            for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
            {
                WidgetRemove((tWidget *)(g_psAboutWidgets + ulIdx));
            }

            //
            // Return the ID of the update panel.
            //
            return(PANEL_UPDATE);
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
            // Display the menu.
            //
            ulPanel = DisplayMenu(PANEL_ABOUT);

            //
            // See if another panel was selected.
            //
            if(ulPanel != PANEL_ABOUT)
            {
                //
                // Remove the "About" panel widgets.
                //
                for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
                {
                    WidgetRemove((tWidget *)(g_psAboutWidgets + ulIdx));
                }

                //
                // Return the ID of the newly selected panel.
                //
                return(ulPanel);
            }
        }
    }
}
