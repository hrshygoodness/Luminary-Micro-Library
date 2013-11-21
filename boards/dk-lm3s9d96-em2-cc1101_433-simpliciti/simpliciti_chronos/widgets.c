//*****************************************************************************
//
// widgets.c -  Widget definitions for the user interfaces of the
//              simpliciti_chronos example applications.
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
// This is part of revision 10636 of the DK-LM3S9D96-EM2-CC1101_433-SIMPLICITI Firmware Package.
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
#include "simpliciti_chronos.h"

//*****************************************************************************
//
// The red color in the TI logo.
//
//*****************************************************************************
#define ClrTIRed 0x00ed1c24

//*****************************************************************************
//*****************************************************************************
//
// Widgets common to all display panels.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// The heading containing the logo banner image.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sMainStatus, g_psPanels + PANEL_WAITING,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 50, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucBannerImage, 0);

//*****************************************************************************
//
// Canvas used to display the latest status.
//
//*****************************************************************************

char g_pcStatus[MAX_STATUS_STRING_LEN];
Canvas(g_sMainStatus, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 218, STATUS_FULL_WIDTH, 22,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_OUTLINE),
       ClrBlack, ClrWhite, ClrWhite, g_pFontCmss14, g_pcStatus, 0, 0);

//*****************************************************************************
//
// Button used to cycle between the connected devices.  This is a child of the
// device status to allow us to add and remove it without affecting other
// widgets.  Note that it is not linked by default since we start off with
// no connected devices.
//
//*****************************************************************************
ImageButton(g_sChangeButton, &g_sMainStatus, 0, 0,
            &g_sKitronix320x240x16_SSD2119, 230, 218, 90, 22,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "Change",
            g_pucRedButtonUp90x22Image, g_pucRedButtonDown90x22Image, 0, 1, 1,
            0, 0, OnChangeButtonPress);

//*****************************************************************************
//*****************************************************************************
//
// Widgets for the display shown while waiting for the watch to send us a
// packet.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// String indicating that we are waiting for communication from a watch.
//
//*****************************************************************************

Canvas(g_sChronosWaitingImage, g_psPanels + PANEL_WAITING, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 94, 56, 132, 134, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucEZ430ChronosImage, 0);

char g_pcWaiting[MAX_WAITING_STRING_LEN];
Canvas(g_sWaiting, g_psPanels + PANEL_WAITING, &g_sChronosWaitingImage, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 192, 320, 30, CANVAS_STYLE_TEXT,
       ClrBlack, ClrWhite, ClrWhite, g_pFontCmss16,
       g_pcWaiting, 0, 0);

//*****************************************************************************
//*****************************************************************************
//
// Widgets for the display shown when the watch is in RF Tilr Control (ACC) mode.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// String indicating that we are receiving accelerometer data from the watch.
//
//*****************************************************************************
Canvas(g_sAccMode, g_psPanels + PANEL_ACC, 0, &g_sXTitle,
       &g_sKitronix320x240x16_SSD2119, 108, 50, 212, 20, CANVAS_STYLE_TEXT,
       ClrBlack, ClrWhite, ClrWhite, g_pFontCmss16,
       "Tilt Control Mode (ACC)", 0, 0);

//*****************************************************************************
//
// The canvas widgets containing text titles.
//
//*****************************************************************************
Canvas(g_sXTitle, &g_sAccMode, &g_sYTitle, 0,
       &g_sKitronix320x240x16_SSD2119, 30, 59, 14, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, ClrWhite, ClrWhite,
       g_pFontCmss16, "X:", 0, 0);
Canvas(g_sYTitle, &g_sAccMode, &g_sZTitle, 0,
       &g_sKitronix320x240x16_SSD2119, 30, 80, 14, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, ClrWhite, ClrWhite,
       g_pFontCmss16, "Y:", 0, 0);
Canvas(g_sZTitle, &g_sAccMode, &g_sIndicators, 0,
       &g_sKitronix320x240x16_SSD2119, 30, 101, 14, 20,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT, ClrBlack, ClrWhite, ClrWhite,
       g_pFontCmss16, "Z:", 0, 0);

//*****************************************************************************
//
// An invisible container widget used to make it easier to repaint only the
// indicator widgets.
//
//*****************************************************************************
Container(g_sIndicators, &g_sAccMode, &g_sBtnAccStar, g_psAccFields,
          &g_sKitronix320x240x16_SSD2119, 45, 50, 50, 78,
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The canvas widgets used to show raw accelerometer values for each axis.
//
//*****************************************************************************
char g_pcAccStrings[3][MAX_DATA_STRING_LEN];
tCanvasWidget g_psAccFields[3] =
{
    //
    // The X and Y indicators here may appear to be reversed compared to the
    // commenting in the source code for the eZ430.  This ensures that the
    // accelerometer readings for left-right movement of the watch appears
    // as X readings and those for forwards-backwards movement appear as Y
    // readings (which appears intuitive to me at least).
    //
    CanvasStruct(&g_sIndicators, g_psAccFields + 1, 0,
                 &g_sKitronix320x240x16_SSD2119, 45, 59, 30, 20,
                 (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
                 CANVAS_STYLE_TEXT_RIGHT), ClrBlack, 0, ClrWhite,
                 g_pFontCmss14, g_pcAccStrings[1], 0, 0),
    CanvasStruct(&g_sIndicators, g_psAccFields + 2, 0,
                 &g_sKitronix320x240x16_SSD2119, 45, 80, 30, 20,
                 (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
                 CANVAS_STYLE_TEXT_RIGHT), ClrBlack, 0, ClrWhite,
                 g_pFontCmss14, g_pcAccStrings[0], 0, 0),
    CanvasStruct(&g_sIndicators, &g_sDrawingCanvas, 0,
                 &g_sKitronix320x240x16_SSD2119, 45, 101, 30, 20,
                 (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
                 CANVAS_STYLE_TEXT_RIGHT), ClrBlack, 0, ClrWhite,
                 g_pFontCmss14, g_pcAccStrings[2], 0, 0),
};

Canvas(g_sDrawingCanvas, &g_sIndicators, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 108, 70, 204, 140,
       CANVAS_STYLE_APP_DRAWN, ClrBlack, ClrWhite, 0, 0, 0, 0,
       OnPaintAccelCanvas);

Canvas(g_sBtnAccStar, g_psPanels + PANEL_ACC, &g_sBtnAccNum, 0,
       &g_sKitronix320x240x16_SSD2119, 10, 130, 30, 30, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucGreyStar30x30Image, 0);

Canvas(g_sBtnAccNum, g_psPanels + PANEL_ACC, &g_sBtnAccUp, 0,
       &g_sKitronix320x240x16_SSD2119, 42, 130, 30, 30, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucGreyNum30x30Image, 0);

Canvas(g_sBtnAccUp, g_psPanels + PANEL_ACC, &g_sCalibrateBtn, 0,
       &g_sKitronix320x240x16_SSD2119, 74, 130, 30, 30, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucGreyCarat30x30Image, 0);

ImageButton(g_sCalibrateBtn, g_psPanels + PANEL_ACC, &g_sClearBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 12, 162, 90, 22,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "Calibrate",
            g_pucRedButtonUp90x22Image, g_pucRedButtonDown90x22Image, 0, 1, 1,
            0, 0, OnCalibrateButtonPress);

ImageButton(g_sClearBtn, g_psPanels + PANEL_ACC, 0, 0,
            &g_sKitronix320x240x16_SSD2119, 12, 187, 90, 22,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "Clear",
            g_pucRedButtonUp90x22Image, g_pucRedButtonDown90x22Image, 0, 1, 1,
            0, 0, OnClearButtonPress);

//*****************************************************************************
//*****************************************************************************
//
// Widgets for the display shown when the watch is in button (Ppt) mode.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// String indicating that we are waiting for communication from a watch.
//
//*****************************************************************************
Canvas(g_sPptMode, g_psPanels + PANEL_PPT, &g_sChronosImage, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 192, 320, 23, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_VCENTER | CANVAS_STYLE_TEXT_HCENTER),
       ClrBlack, ClrWhite, ClrWhite, g_pFontCmss16,
       "PowerPoint Control (PPt)", 0, 0);

Canvas(g_sChronosImage, g_psPanels + PANEL_PPT, &g_sBtnPptStar, 0,
       &g_sKitronix320x240x16_SSD2119, 94, 56, 132, 134, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucEZ430ChronosImage, 0);

Canvas(g_sBtnPptStar, g_psPanels + PANEL_PPT, &g_sBtnPptNum, 0,
       &g_sKitronix320x240x16_SSD2119, 50, 70, 30, 30, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucGreyStar30x30Image, 0);

Canvas(g_sBtnPptNum, g_psPanels + PANEL_PPT, &g_sBtnPptUp, 0,
       &g_sKitronix320x240x16_SSD2119, 50, 136, 30, 30, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucGreyNum30x30Image, 0);

Canvas(g_sBtnPptUp, g_psPanels + PANEL_PPT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 240, 70, 30, 30, CANVAS_STYLE_IMG,
       0, 0, 0, 0, 0, g_pucGreyCarat30x30Image, 0);

//*****************************************************************************
//*****************************************************************************
//
// Widgets for the display shown when the watch is in button (Ppt) mode.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// String indicating that the watch is in sync mode.
//
//*****************************************************************************
Canvas(g_sSyncMode, g_psPanels + PANEL_SYNC, &g_sHours, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 192, 320, 23, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_VCENTER | CANVAS_STYLE_TEXT_HCENTER),
       ClrBlack, ClrWhite, ClrWhite, g_pFontCmss16,  "Sync Mode", 0, 0);

char g_pcHours[3];
Canvas(g_sHours, g_psPanels + PANEL_SYNC, &g_sMinutes, &g_sAmPm,
       &g_sKitronix320x240x16_SSD2119, 30, 60, 40, 30, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_RIGHT | CANVAS_STYLE_FILL),
       ClrBlack, 0, ClrSilver, g_pFontCmss26b, g_pcHours, 0, 0);

char g_pcAmPm[3];
Canvas(g_sAmPm, &g_sHours, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 170, 60, 30, 30, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_LEFT | CANVAS_STYLE_FILL),
       ClrBlack, 0, ClrSilver, g_pFontCmss14b, g_pcAmPm, 0, 0);

char g_pcMinutes[3];
Canvas(g_sMinutes, g_psPanels + PANEL_SYNC, &g_sSeconds, 0,
       &g_sKitronix320x240x16_SSD2119, 80, 60, 40, 30, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_FILL),
       ClrBlack, 0, ClrSilver, g_pFontCmss26b, g_pcMinutes, 0, 0);

char g_pcSeconds[3];
Canvas(g_sSeconds, g_psPanels + PANEL_SYNC, &g_sColon1, 0,
       &g_sKitronix320x240x16_SSD2119, 130, 60, 40, 30, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_FILL),
       ClrBlack, 0, ClrSilver, g_pFontCmss26b, g_pcSeconds, 0, 0);

Canvas(g_sColon1, g_psPanels + PANEL_SYNC, &g_sColon2, 0,
       &g_sKitronix320x240x16_SSD2119, 70, 60, 10, 30, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER),
       ClrBlack, 0, ClrSilver, g_pFontCmss26b, ":", 0, 0);

Canvas(g_sColon2, g_psPanels + PANEL_SYNC, &g_sDate, 0,
       &g_sKitronix320x240x16_SSD2119, 120, 60, 10, 30, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER),
       ClrBlack, 0, ClrSilver, g_pFontCmss26b, ":", 0, 0);

char g_pcDate[MAX_DATE_LEN];
Canvas(g_sDate, g_psPanels + PANEL_SYNC, &g_sYear, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 90, 200, 30, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_FILL),
       ClrBlack, 0, ClrSilver, g_pFontCmss26b, g_pcDate, 0, 0);

char g_pcYear[6];
Canvas(g_sYear, g_psPanels + PANEL_SYNC, &g_sAlarm, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 120, 200, 30, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_FILL),
       ClrBlack, 0, ClrSilver, g_pFontCmss26b, g_pcYear, 0, 0);

char g_pcAlarmTime[10];
Canvas(g_sAlarmTime, &g_sAlarm, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 204, 72, 102, 20, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_FILL),
       ClrMidnightBlue, 0, ClrSilver, g_pFontCmss14, g_pcAlarmTime, 0, 0);

Container(g_sAlarm, g_psPanels + PANEL_SYNC, &g_sTemperature, &g_sAlarmTime,
          &g_sKitronix320x240x16_SSD2119, 200, 60, 110, 36,
          (CTR_STYLE_FILL | CTR_STYLE_OUTLINE | CTR_STYLE_TEXT),
          ClrMidnightBlue, ClrWhite, ClrWhite, g_pFontCmss14b, "Alarm");

char g_pcTemperature[8];
Canvas(g_sTemperatureValue, &g_sTemperature, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 204, 112, 102, 20, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_FILL),
       ClrMidnightBlue, 0, ClrSilver, g_pFontCmss14, g_pcTemperature, 0, 0);

Container(g_sTemperature, g_psPanels + PANEL_SYNC, &g_sAltitude,
          &g_sTemperatureValue, &g_sKitronix320x240x16_SSD2119,
          200, 100, 110, 36, (CTR_STYLE_FILL | CTR_STYLE_OUTLINE |
          CTR_STYLE_TEXT), ClrMidnightBlue, ClrWhite, ClrWhite,
          g_pFontCmss14b, "Temperature");

char g_pcAltitude[8];
Canvas(g_sAltitudeValue, &g_sAltitude, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 204, 152, 102, 20, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_FILL),
       ClrMidnightBlue, 0, ClrSilver, g_pFontCmss14, g_pcAltitude, 0, 0);

Container(g_sAltitude, g_psPanels + PANEL_SYNC, &g_sFormat, &g_sAltitudeValue,
          &g_sKitronix320x240x16_SSD2119, 200, 140, 110, 36,
          (CTR_STYLE_FILL | CTR_STYLE_OUTLINE | CTR_STYLE_TEXT),
          ClrMidnightBlue, ClrWhite, ClrWhite, g_pFontCmss14b, "Altitude");

Canvas(g_sFormat, g_psPanels + PANEL_SYNC, &g_sFormatBtn, 0,
       &g_sKitronix320x240x16_SSD2119, 20, 164, 80, 20, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_HCENTER | CANVAS_STYLE_FILL),
       ClrBlack, 0, ClrSilver, g_pFontCmss14, "Set Format", 0, 0);

ImageButton(g_sFormatBtn, g_psPanels + PANEL_SYNC, 0, 0,
            &g_sKitronix320x240x16_SSD2119, 100, 162, 90, 22,
            IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY,
            ClrWhite, ClrWhite, ClrTIRed, g_pFontCmss14, "Imperial",
            g_pucRedButtonUp90x22Image, g_pucRedButtonDown90x22Image, 0, 1, 1,
            0, 0, OnFormatButtonPress);

//*****************************************************************************
//
// The canvas widgets acting as background to each of the screens.
//
//*****************************************************************************
tCanvasWidget g_psPanels[] =
{
    CanvasStruct(&g_sHeading, 0, &g_sWaiting,
                 &g_sKitronix320x240x16_SSD2119, 0, 50, 320, (240 - 73),
                 CANVAS_STYLE_FILL , ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(&g_sHeading, 0, &g_sAccMode,
                 &g_sKitronix320x240x16_SSD2119, 0, 50, 320, (240 - 73),
                 CANVAS_STYLE_FILL , ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(&g_sHeading, 0, &g_sPptMode,
                 &g_sKitronix320x240x16_SSD2119, 0, 50, 320, (240 - 73),
                 CANVAS_STYLE_FILL , ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(&g_sHeading, 0, &g_sSyncMode,
                 &g_sKitronix320x240x16_SSD2119, 0, 50, 320, (240 - 73),
                 CANVAS_STYLE_FILL , ClrBlack, 0, 0, 0, 0, 0, 0),
};
