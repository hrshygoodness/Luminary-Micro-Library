//*****************************************************************************
//
// widgets.c -  Widget definitions for the user interfaces of the
//              Bluetooth A2DP example applications.
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
// The heading containing the logo banner image.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, 0, &g_psMainPanel,
       &g_sKitronix320x240x16_SSD2119, 60, 0, 194, 20, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucBannerImage, 0);

//*****************************************************************************
//
// Widgets for the main display.
//
//*****************************************************************************
Canvas(g_sMainImage, &g_psMainPanel, &g_sLMSymbol, 0,
       &g_sKitronix320x240x16_SSD2119, 113, 45, 184, 62,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucBluetopiaImage, 0);

Canvas(g_sLMSymbol, &g_psMainPanel, &g_sBackBtn, 0,
       &g_sKitronix320x240x16_SSD2119, 10, 25, 100, 100,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_IMG),
       ClrBlack, 0, 0, 0, 0, g_pucTISymbol_80x75, 0);

ImageButton(g_sBackBtn, &g_psMainPanel, &g_sPauseBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 15, 125, 60, 60,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "",
            g_pucBackButtonUp60x60, g_pucBackButtonDown60x60, 0, 1, 1,
            0, 0, OnButtonPress);

ImageButton(g_sPauseBtn, &g_psMainPanel, &g_sPlayBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 90, 125, 60, 60,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "",
            g_pucPauseButtonUp60x60Image, g_pucPauseButtonDown60x60Image, 0, 1, 1,
            0, 0, OnButtonPress);

ImageButton(g_sPlayBtn, &g_psMainPanel, &g_sNextBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 165, 125, 60, 60,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "",
            g_pucPlayButtonUp60x60Image, g_pucPlayButtonDown60x60Image, 0, 1, 1,
            0, 0, OnButtonPress);

ImageButton(g_sNextBtn, &g_psMainPanel, &g_sMainPanelText, 0,
            &g_sKitronix320x240x16_SSD2119, 243, 125, 60, 60,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "",
            g_pucNextButtonUp60x60, g_pucNextButtonDown60x60, 0, 1, 1,
            0, 0, OnButtonPress);

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
//
// The canvas widget acting as background for the main screen.
//
//*****************************************************************************
tCanvasWidget g_psMainPanel =
   CanvasStruct(&g_sHeading, 0, &g_sMainImage,
                &g_sKitronix320x240x16_SSD2119, 0, 50, 320, (240 - 73),
                CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);
