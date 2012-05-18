//*****************************************************************************
//
// gui_widgets.h - Prototypes and definitions related to the widget set used in
//                 the user interface of the qs-checkout application.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __GUI_WIDGETS_H__
#define __GUI_WIDGETS_H__

#ifndef GUI_WIDGETS_FUNCS_ONLY
//*****************************************************************************
//
// Color definitions for the GUI.
//
//*****************************************************************************
#define CLR_BACKGROUND ClrBlack
#define CLR_BOX        ClrDarkBlue
#define CLR_OUTLINE    ClrWhite
#define CLR_TEXT       ClrSilver
#define CLR_PRESENT    ClrWhite
#define CLR_ABSENT     ClrGray
#define CLR_BUTTON     ClrDarkRed
#define CLR_PRESSED    ClrRed

//*****************************************************************************
//
// Variables exported by this module.
//
//*****************************************************************************
extern tCanvasWidget g_sIPAddr;
extern tCanvasWidget g_sHeading;
extern tCanvasWidget g_sIPAddr;
extern tCanvasWidget g_sTouchPos;
extern tCanvasWidget g_sSDCard1;
extern tCanvasWidget g_sSDCard2;
extern tCanvasWidget g_sMousePos;
extern tCanvasWidget g_sModeString;
extern tCanvasWidget g_sMouseBtn1;
extern tCanvasWidget g_sMouseBtn2;
extern tCanvasWidget g_sMouseBtn3;
extern tSliderWidget g_sThumbwheelSlider;
extern tCanvasWidget g_sPotPos;
extern tContainerWidget g_sImageScreen;
extern tContainerWidget g_sAudioScreen;
extern tContainerWidget g_sHomeScreen;

//*****************************************************************************
//
// Storage for the string showing the current mouse position.
//
//*****************************************************************************
#define MAX_MOUSE_POS_LEN 16
extern char g_pcMousePos[];

//*****************************************************************************
//
// An array of strings describing the three modes that the USB mouse can be
// in - "None", "Host" or "Device".
//
//*****************************************************************************
extern char *g_pcMouseModes[];

//*****************************************************************************
//
// The index of the screen which is currently hooked to WIDGET_ROOT and, hence,
// displayed.
//
//*****************************************************************************
extern unsigned long g_ulCurrentScreen;

//*****************************************************************************
//
// Indices of the various user interface screens defined in g_psScreens.
//
//*****************************************************************************
#define HOME_SCREEN  0
#define IO_SCREEN    1
#define DEMO_SCREEN  2
#define IMAGE_SCREEN 3
#define AUDIO_SCREEN 4

//*****************************************************************************
//
// Friendly labels containing the indices of the various strings in the
// g_pcMouseModes array.
//
//*****************************************************************************
#define MOUSE_MODE_STR_NONE   0
#define MOUSE_MODE_STR_HOST   1
#define MOUSE_MODE_STR_DEVICE 2

//*****************************************************************************
//
// Widget message handler functions.
//
//*****************************************************************************
extern void OnBtnHome(tWidget *pWidget);
extern void OnBtnShowIOScreen(tWidget *pWidget);
extern void OnBtnShowDemoScreen(tWidget *pWidget);
extern void OnBtnShowImageScreen(tWidget *pWidget);
extern void OnCheckLED(tWidget *pWidget, unsigned long bSelected);

#endif // GUI_WIDGETS_FUNCS_ONLY

//*****************************************************************************
//
// Functions exported by this module.
//
//*****************************************************************************
extern int PrintfStatus(char *pcFormat, ...);
extern void ShowUIScreen(unsigned long ulIndex);

#endif // __GUI_WIDGETS_H__
