//*****************************************************************************
//
// simpliciti_chronos.h - Function prototypes and global variables exported
//                        from simpliciti_chronos.c.
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the DK-LM3S9B96-EM2-CC1101_915-SIMPLICITI Firmware Package.
//
//*****************************************************************************

#ifndef __SIMPLICITI_CHRONOS_H__
#define __SIMPLICITI_CHRONOS_H__

//*****************************************************************************
//
// The application name string.
//
//*****************************************************************************
extern const char g_pcFrequency[];

//*****************************************************************************
//
// Prototype of the various button handlers.
//
//*****************************************************************************
void OnChangeButtonPress(tWidget *pWidget);
void OnCalibrateButtonPress(tWidget *pWidget);
void OnClearButtonPress(tWidget *pWidget);
void OnFormatButtonPress(tWidget *pWidget);

//*****************************************************************************
//
// Prototypes for various display update functions.
//
//*****************************************************************************
void OnPaintAccelCanvas(tWidget *pWidget, tContext *pContext);


//*****************************************************************************
//
// Identifiers for the various panels that we display on the screen.  These
// must correspond to the respective index into the g_psPanels array.
//
//*****************************************************************************
#define PANEL_WAITING 0
#define PANEL_ACC     1
#define PANEL_PPT     2
#define PANEL_SYNC    3

#endif // __SIMPLICITI_CHRONOS_H__
