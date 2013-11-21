//*****************************************************************************
//
// widgets.h - Variables exported from widgets.c in the simpliciti_chronos
//             example application.
//
// Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the DK-LM3S9D96-EM2-CC1101_915-SIMPLICITI Firmware Package.
//
//*****************************************************************************

#ifndef __WIDGETS_H__
#define __WIDGETS_H__

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sHeading;
extern tCanvasWidget g_sBackground;
extern tCanvasWidget g_sXTitle;
extern tCanvasWidget g_sYTitle;
extern tCanvasWidget g_sZTitle;
extern tCanvasWidget g_sMainStatus;
extern tCanvasWidget g_sEraseChange;
extern tImageButtonWidget g_sChangeButton;
extern tCanvasWidget g_psPanels[];
extern tContainerWidget g_sIndicators;
extern tCanvasWidget g_psAccFields[3];
extern tCanvasWidget g_sDrawingCanvas;
extern tCanvasWidget g_sBtnAccStar;
extern tCanvasWidget g_sBtnAccNum;
extern tCanvasWidget g_sBtnAccUp;
extern tImageButtonWidget g_sCalibrateBtn;
extern tImageButtonWidget g_sClearBtn;
extern tCanvasWidget g_sChronosImage;
extern tCanvasWidget g_sBtnPptStar;
extern tCanvasWidget g_sBtnPptNum;
extern tCanvasWidget g_sBtnPptUp;
extern tCanvasWidget g_sHours;
extern tCanvasWidget g_sMinutes;
extern tCanvasWidget g_sSeconds;
extern tCanvasWidget g_sAmPm;
extern tCanvasWidget g_sAlarmTime;
extern tCanvasWidget g_sColon1;
extern tCanvasWidget g_sColon2;
extern tCanvasWidget g_sDate;
extern tCanvasWidget g_sYear;
extern tContainerWidget g_sTemperature;
extern tContainerWidget g_sAltitude;
extern tContainerWidget g_sAlarm;
extern tCanvasWidget g_sAltitudeValue;
extern tCanvasWidget g_sTemperatureValue;
extern tCanvasWidget g_sFormat;
extern tImageButtonWidget g_sFormatBtn;

//*****************************************************************************
//
// Maximum lengths of various string buffers.
//
//*****************************************************************************
#define MAX_STATUS_STRING_LEN 32
#define MAX_DATA_STRING_LEN 6
#define MAX_DATE_LEN 14
#define MAX_WAITING_STRING_LEN 64

//*****************************************************************************
//
// We change the width of the status string widget on the fly if more than
// one eZ430-Chronos attaches (to give us space to add the button used to cycle
// between the watches).
//
//*****************************************************************************
#define STATUS_FULL_WIDTH 320
#define STATUS_PART_WIDTH 230

//*****************************************************************************
//
// Buffers used to hold various status strings.
//
//*****************************************************************************
extern char g_pcAccStrings[3][MAX_DATA_STRING_LEN];
extern char g_pcStatus[MAX_STATUS_STRING_LEN];
extern char g_pcWaiting[];
extern char g_pcHours[];
extern char g_pcAmPm[];
extern char g_pcMinutes[];
extern char g_pcSeconds[];
extern char g_pcDate[];
extern char g_pcYear[];
extern char g_pcAlarmTime[];
extern char g_pcTemperature[];
extern char g_pcAltitude[];

#endif // __WIDGETS_H__
