//*****************************************************************************
//
// menu.h - Prototypes for the system menu.
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

#ifndef __MENU_H__
#define __MENU_H__

//*****************************************************************************
//
// The IDs for the various panels in the system.
//
//*****************************************************************************
#define PANEL_VOLTAGE           0
#define PANEL_VCOMP             1
#define PANEL_CURRENT           2
#define PANEL_SPEED             3
#define PANEL_POSITION          4
#define PANEL_CONFIGURATION     5
#define PANEL_DEV_LIST          6
#define PANEL_UPDATE            7
#define PANEL_HELP              8
#define PANEL_ABOUT             9
#define NUM_PANELS              10

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern unsigned long DisplayMenu(unsigned long ulPanel);

#endif // __MENU_H__
