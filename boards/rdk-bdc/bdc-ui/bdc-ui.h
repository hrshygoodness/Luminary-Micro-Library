//*****************************************************************************
//
// bdc-ui.h - Prototypes for the brushed DC motor user interface.
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
// This is part of revision 8555 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#ifndef __BDC_UI_H__
#define __BDC_UI_H__

//*****************************************************************************
//
// The bit positions of the flags in g_ulFlags.
//
//*****************************************************************************
#define FLAG_TICK               0
#define FLAG_UP_PRESSED         1
#define FLAG_DOWN_PRESSED       2
#define FLAG_LEFT_PRESSED       3
#define FLAG_LEFT_ACCEL1        4
#define FLAG_LEFT_ACCEL2        5
#define FLAG_LEFT_ACCEL3        6
#define FLAG_RIGHT_PRESSED      7
#define FLAG_RIGHT_ACCEL1       8
#define FLAG_RIGHT_ACCEL2       9
#define FLAG_RIGHT_ACCEL3       10
#define FLAG_SELECT_PRESSED     11
#define FLAG_SERIAL_BOOTLOADER  12

//*****************************************************************************
//
// Some common "colors" used in the user interface.
//
//*****************************************************************************
#define ClrNotPresent           0x00222222
#define ClrSelected             0x00666666

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
extern volatile unsigned long g_ulFlags;
extern volatile unsigned long g_ulTickCount;
extern void DisplayFlush(void);

#endif // __BDC_UI_H__
