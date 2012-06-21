//*****************************************************************************
//
// widgets.c -  Widget definitions for the user interfaces of the
//              Bluetooth SPP example applications.
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

#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/container.h"
#include "grlib/imgbutton.h"

#include "drivers/kitronix320x240x16_ssd2119_8bit.h"

#include "widgets.h"
#include "images.h"
#include "graphics.h"

//*****************************************************************************
//
// The red color in the TI logo.
//
//*****************************************************************************
#define ClrTIRed 0x00ed1c24

//*****************************************************************************
//
// The heading containing the logo banner image (main screen)
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, 0, &g_psMainPanel,
       &g_sKitronix320x240x16_SSD2119, 60, 0, 194, 20, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucBannerImage, 0);

//*****************************************************************************
//*****************************************************************************
//
// Widgets for the display shown while waiting for the watch to send us a
// packet.
//
//*****************************************************************************
//*****************************************************************************
Canvas(g_sMainImage, &g_psMainPanel, &g_sLMSymbol, 0,
       &g_sKitronix320x240x16_SSD2119, 113, 75, 184, 62,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucBluetopiaImage, 0);

Canvas(g_sLMSymbol, &g_psMainPanel, &g_sMainPanelText, 0,
       &g_sKitronix320x240x16_SSD2119, 10, 55, 100, 100,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_IMG),
       ClrBlack, 0, 0, 0, 0, g_pucTISymbol_80x75, 0);

//*****************************************************************************
//
// String indicating the purpose of the demo
//
//*****************************************************************************
char g_pcMainPanel[MAX_MAIN_PANEL_STRING_LEN];
Canvas(g_sMainPanelText, &g_psMainPanel, &g_sMainStatus, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 192, 320, 30, CANVAS_STYLE_TEXT,
       ClrBlack, ClrWhite, ClrWhite, g_pFontCmss16,
       g_pcMainPanel, 0, 0);

//*****************************************************************************
//
// Canvas used to display the latest status.
//
//*****************************************************************************
char g_pcStatus[MAX_STATUS_STRING_LEN];
Canvas(g_sMainStatus, &g_psMainPanel, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 45, 218, 230, 22,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_OUTLINE),
       ClrBlack, ClrWhite, ClrWhite, g_pFontCmss14, g_pcStatus, 0, 0);

//*****************************************************************************
//*****************************************************************************
//
// Widgets for the display shown when in Accelerameter mode.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// String indicating that we are receiving accelerometer data.
//
//*****************************************************************************
Canvas(g_sAccMode, &g_psAccelPanel, 0, &g_sXTitle,
       &g_sKitronix320x240x16_SSD2119, 108, 50, 212, 20, CANVAS_STYLE_TEXT,
       ClrBlack, ClrWhite, ClrWhite, g_pFontCmss16,
       "Accelerometer Graph", 0, 0);

//*****************************************************************************
//
// The canvas widgets containing text titles.
//
//*****************************************************************************
Canvas(g_sXTitle, &g_sAccMode, &g_sYTitle, 0,
       &g_sKitronix320x240x16_SSD2119, 41, 77, 14, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, ClrWhite, ClrWhite,
       g_pFontCmss16, "X:", 0, 0);
Canvas(g_sYTitle, &g_sAccMode, &g_sIndicators, 0,
       &g_sKitronix320x240x16_SSD2119, 41, 92, 14, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, ClrWhite, ClrWhite,
       g_pFontCmss16, "Y:", 0, 0);

//*****************************************************************************
//
// An invisible container widget used to make it easier to repaint only the
// indicator widgets.
//
//*****************************************************************************
Container(g_sIndicators, &g_sAccMode, &g_sCalibrateBtn, g_psAccFields,
          &g_sKitronix320x240x16_SSD2119, 45, 50, 50, 78,
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The canvas widgets used to show raw accelerometer values for each axis.
//
//*****************************************************************************
char g_pcAccStrings[2][MAX_DATA_STRING_LEN];
tCanvasWidget g_psAccFields[2] =
{
    //
    // The X and Y indicators here may appear to be reversed compared to the
    // commenting in the source code for the eZ430.  This ensures that the
    // accelerometer readings for left-right movement of the watch appears
    // as X readings and those for forwards-backwards movement appear as Y
    // readings (which appears intuitive to me at least).
    //
    CanvasStruct(&g_sIndicators, g_psAccFields + 1, 0,
                 &g_sKitronix320x240x16_SSD2119, 53, 77, 30, 20,
                 (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
                 CANVAS_STYLE_TEXT_RIGHT), ClrBlack, 0, ClrWhite,
                 g_pFontCmss14, g_pcAccStrings[1], 0, 0),
    CanvasStruct(&g_sIndicators, &g_sDrawingCanvas, 0,
                 &g_sKitronix320x240x16_SSD2119, 53, 92, 30, 20,
                 (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
                 CANVAS_STYLE_TEXT_RIGHT), ClrBlack, 0, ClrWhite,
                 g_pFontCmss14, g_pcAccStrings[0], 0, 0),
};

Canvas(g_sDrawingCanvas, &g_sIndicators, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 108, 70, 204, 140,
       CANVAS_STYLE_APP_DRAWN, ClrBlack, ClrWhite, 0, 0, 0, 0,
       OnPaintAccelCanvas);

ImageButton(g_sCalibrateBtn, &g_psAccelPanel, &g_sClearBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 12, 125, 90, 22,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "Calibrate",
            g_pucRedButtonUp90x22Image, g_pucRedButtonDown90x22Image, 0, 1, 1,
            0, 0, OnCalibrateButtonPress);

ImageButton(g_sClearBtn, &g_psAccelPanel, 0, 0,
            &g_sKitronix320x240x16_SSD2119, 12, 150, 90, 22,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "Clear",
            g_pucRedButtonUp90x22Image, g_pucRedButtonDown90x22Image, 0, 1, 1,
            0, 0, OnClearButtonPress);

//*****************************************************************************
//
// The canvas widget acting as background for the main screen.
//
//*****************************************************************************
tCanvasWidget g_psMainPanel =
    CanvasStruct(&g_sHeading, 0, &g_sMainImage,
                 &g_sKitronix320x240x16_SSD2119, 0, 50, 320, (240 - 73),
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The canvas widget acting as background for the Accelerameter screen.
//
//*****************************************************************************
tCanvasWidget g_psAccelPanel =
    CanvasStruct(&g_sHeading, 0, &g_sAccMode,
                 &g_sKitronix320x240x16_SSD2119, 0, 50, 320, (240 - 73),
                 CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);
