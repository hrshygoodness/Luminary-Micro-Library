//*****************************************************************************
//
// dev_list.c - Displays the "Device List" panel.
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
// This is part of revision 10636 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "utils/ustdlib.h"
#include "shared/can_proto.h"
#include "bdc-ui.h"
#include "can_comm.h"
#include "dev_list.h"
#include "menu.h"
#include "rit128x96x4.h"

//*****************************************************************************
//
// The widgets that make up the "Device List" panel.
//
//*****************************************************************************
static tCanvasWidget g_psDevListWidgets[] =
{
    //
    // The first row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 20, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "1", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 20, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "2", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 20, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "3", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 20, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "4", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 20, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "5", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 20, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "6", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 20, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "7", 0, 0),

    //
    // The second row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 28, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "8", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 28, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "9", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 28, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "10", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 28, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "11", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 28, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "12", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 28, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "13", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 28, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "14", 0, 0),

    //
    // The third row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 36, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "15", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 36, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "16", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 36, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "17", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 36, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "18", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 36, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "19", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 36, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "20", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 36, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "21", 0, 0),

    //
    // The fourth row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 44, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "22", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 44, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "23", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 44, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "24", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 44, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "25", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 44, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "26", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 44, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "27", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 44, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "28", 0, 0),

    //
    // The fifth row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 52, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "29", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 52, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "30", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 52, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "31", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 52, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "32", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 52, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "33", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 52, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "34", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 52, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "35", 0, 0),

    //
    // The sixth row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 60, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "36", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 60, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "37", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 60, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "38", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 60, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "39", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 60, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "40", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 60, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "41", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 60, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "42", 0, 0),

    //
    // The seventh row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 68, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "43", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 68, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "44", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 68, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "45", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 68, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "46", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 68, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "47", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 68, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "48", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 68, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "49", 0, 0),

    //
    // The eighth row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 76, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "50", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 76, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "51", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 76, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "52", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 76, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "53", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 76, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "54", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 76, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "55", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 76, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "56", 0, 0),

    //
    // The ninth row of device numbers.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 84, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "57", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 19, 84, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "58", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 37, 84, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "59", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 55, 84, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "60", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 73, 84, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "61", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 91, 84, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "62", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 109, 84, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrNotPresent,
                 g_pFontFixed6x8, "63", 0, 0),

    //
    // The header.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 "Device List", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 12, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0),
};

//*****************************************************************************
//
// The number of widgets in the "Device List" panel.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psDevListWidgets) /                 \
                                 sizeof(g_psDevListWidgets[0]))

//*****************************************************************************
//
// A widget to indicate that a device ID is being assigned.
//
//*****************************************************************************
static char g_pcBuffer[16];
static Canvas(g_sAssignWidget, 0, 0, 0, &g_sRIT128x96x4Display, 12, 46, 104,
              12, CANVAS_STYLE_OUTLINE | CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT,
              ClrBlack, ClrSelected, ClrWhite, g_pFontFixed6x8, g_pcBuffer, 0,
              0);

//*****************************************************************************
//
// Displays the "Device List" panel.  The returned value is the ID of the panel
// to be displayed instead of the "Device List" panel.
//
//*****************************************************************************
unsigned long
DisplayDevList(void)
{
    unsigned long ulPosX, ulPosY, ulIdx;

    //
    // Disable the widget fill for all the widgets except the one for device
    // ID 1.
    //
    CanvasFillOn(g_psDevListWidgets);
    for(ulIdx = 1; ulIdx < 64; ulIdx++)
    {
        CanvasFillOff(g_psDevListWidgets + ulIdx);
    }

    //
    // Add the "Device List" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psDevListWidgets + ulIdx));
    }

    //
    // Set the default cursor position to device ID 1.
    //
    ulPosX = 0;
    ulPosY = 1;

    //
    // Loop forever.  This loop will be explicitly exited when the proper
    // condition is detected.
    //
    while(1)
    {
        //
        // Enumerate the devices on the CAN bus.
        //
        CANEnumerate();

        //
        // Delay for 100ms while the bus is being enumerated.
        //
        for(ulIdx = 0; ulIdx < 100; ulIdx++)
        {
            HWREGBITW(&g_ulFlags, FLAG_TICK) = 0;
            while(HWREGBITW(&g_ulFlags, FLAG_TICK) == 0)
            {
            }
        }

        //
        // See if a serial download has begun.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 1)
        {
            //
            // Remove the "Device List" panel widgets.
            //
            for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
            {
                WidgetRemove((tWidget *)(g_psDevListWidgets + ulIdx));
            }

            //
            // Return the ID of the update panel.
            //
            return(PANEL_UPDATE);
        }

        //
        // See if the up button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the top of the
            // screen.
            //
            if(ulPosY != 0)
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psDevListWidgets + (ulPosY * 7) + ulPosX - 7);

                //
                // Decrement the cursor row.
                //
                ulPosY--;

                //
                // Enable the widget fill for the newly selected widget.
                //
                if(ulPosY == 0)
                {
                    CanvasFillOn(g_psDevListWidgets + 63);
                }
                else
                {
                    CanvasFillOn(g_psDevListWidgets + (ulPosY * 7) + ulPosX -
                                 7);
                }
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
            // screen.
            //
            if(ulPosY != 9)
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                if(ulPosY == 0)
                {
                    CanvasFillOff(g_psDevListWidgets + 63);
                }
                else
                {
                    CanvasFillOff(g_psDevListWidgets + (ulPosY * 7) + ulPosX -
                                  7);
                }

                //
                // Increment the cursor row.
                //
                ulPosY++;

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psDevListWidgets + (ulPosY * 7) + ulPosX - 7);
            }

            //
            // Clear the press flag for the down button.
            //
            HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
        }

        //
        // See if the left button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not on the top row of the screen
            // and if it is not already at the left edge of the screen.
            //
            if((ulPosX != 0) && (ulPosY != 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psDevListWidgets + (ulPosY * 7) + ulPosX - 7);

                //
                // Decrement the cursor column.
                //
                ulPosX--;

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psDevListWidgets + (ulPosY * 7) + ulPosX - 7);
            }

            //
            // Clear the press flag for the left button.
            //
            HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
        }

        //
        // See if the right button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not on the top row of the screen
            // and if it is not already at the right edge of the screen.
            //
            if((ulPosX != 6) && (ulPosY != 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psDevListWidgets + (ulPosY * 7) + ulPosX - 7);

                //
                // Increment the cursor column.
                //
                ulPosX++;

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psDevListWidgets + (ulPosY * 7) + ulPosX - 7);
            }

            //
            // Clear the press flag for the right button.
            //
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
        }

        //
        // Loop through the 63 possible device IDs and set the text color of
        // the corresponding widget based on that ID's presence or absence on
        // the bus.
        //
        for(ulIdx = 1; ulIdx < 64; ulIdx++)
        {
            if(g_pulStatusEnumeration[ulIdx / 32] & (1 << (ulIdx % 32)))
            {
                CanvasTextColorSet(g_psDevListWidgets + ulIdx - 1, ClrWhite);
            }
            else
            {
                CanvasTextColorSet(g_psDevListWidgets + ulIdx - 1,
                                   ClrNotPresent);
            }
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
            // See if the cursor is on the top row of the screen.
            //
            if(ulPosY == 0)
            {
                //
                // Display the menu.
                //
                ulIdx = DisplayMenu(PANEL_DEV_LIST);

                //
                // See if another panel was selected.
                //
                if(ulIdx != PANEL_DEV_LIST)
                {
                    //
                    // Remove the "Device List" panel widgets.
                    //
                    for(ulPosX = 0; ulPosX < NUM_WIDGETS; ulPosX++)
                    {
                        WidgetRemove((tWidget *)(g_psDevListWidgets + ulPosX));
                    }

                    //
                    // Return the ID of the newly selected panel.
                    //
                    return(ulIdx);
                }

                //
                // Since the "Device List" panel was selected from the menu,
                // move the cursor down one row.
                //
                CanvasFillOff(g_psDevListWidgets + 63);
                ulPosY++;
                CanvasFillOn(g_psDevListWidgets + (ulPosY * 7) + ulPosX - 7);
            }
            else
            {
                //
                // Indicate that the current ID is begin assigned.
                //
                usnprintf(g_pcBuffer, sizeof(g_pcBuffer), "Assigning %d...",
                          (ulPosY * 7) + ulPosX - 6);
                WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sAssignWidget);

                //
                // Update the display.
                //
                DisplayFlush();

                //
                // Perform a CAN device ID assignment.
                //
                CANAssign((ulPosY * 7) + ulPosX - 6);

                //
                // Delay for 5 seconds while the ID assignment takes place.
                //
                for(ulIdx = 0; ulIdx < 5000; ulIdx++)
                {
                    HWREGBITW(&g_ulFlags, FLAG_TICK) = 0;
                    while(HWREGBITW(&g_ulFlags, FLAG_TICK) == 0)
                    {
                    }
                }

                //
                // Remove the assignment indicator widget.
                //
                WidgetRemove((tWidget *)&g_sAssignWidget);

                //
                // Clear any button presses that may have occurred during the
                // ID assignment.
                //
                HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) = 0;
            }
        }

        //
        // Update the display.
        //
        DisplayFlush();
    }
}
