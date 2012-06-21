//*****************************************************************************
//
// widgets.h - Variables exported from widgets.c in the Bluetooth A2DP
//             example application.
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

#ifndef __WIDGETS_H__
#define __WIDGETS_H__

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_psMainPanel;
extern tCanvasWidget g_sHeading;
extern tCanvasWidget g_sMainImage;
extern tCanvasWidget g_sLMSymbol;
extern tCanvasWidget g_sMainPanelText;
extern tCanvasWidget g_sMainStatus;
extern tImageButtonWidget g_sPlayBtn;
extern tImageButtonWidget g_sPauseBtn;
extern tImageButtonWidget g_sNextBtn;
extern tImageButtonWidget g_sBackBtn;

//*****************************************************************************
//
// Maximum lengths of various string buffers.
//
//*****************************************************************************
#define MAX_STATUS_STRING_LEN     36
#define MAX_MAIN_PANEL_STRING_LEN 64

//*****************************************************************************
//
// Buffers used to hold various status strings.
//
//*****************************************************************************
extern char g_pcStatus[MAX_STATUS_STRING_LEN];
extern char g_pcMainPanel[MAX_MAIN_PANEL_STRING_LEN];

#endif // __WIDGETS_H__
