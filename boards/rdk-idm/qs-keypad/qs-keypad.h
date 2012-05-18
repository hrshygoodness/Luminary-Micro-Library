//*****************************************************************************
//
// qs-keypad.h -
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
// This is part of revision 8555 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#ifndef __QS_KEYPAD_H__
#define __QS_KEYPAD_H__

//*****************************************************************************
//
// The number of SysTick interrupts per second.  This must evenly divide 1000
// to prevent accumulation of error in the software RTC and the lwIP timers,
// and must be 100 so that there is a tick every 10ms for use by the filesystem
// code.
//
//*****************************************************************************
#define TICKS_PER_SECOND        100

//*****************************************************************************
//
// The amount of idle time after which the keypad is cleared from the screen.
//
//*****************************************************************************
#define KEYPAD_TIMEOUT          (TICKS_PER_SECOND * 10)

//*****************************************************************************
//
// The amount of time the relay is opened after the correct code is entered.
//
//*****************************************************************************
#define RELAY_TIMEOUT           (TICKS_PER_SECOND * 5)

//*****************************************************************************
//
// Labels defining the various modes that the application can be in.  These are
// the values found in global variable g_ulMode.
//
//*****************************************************************************
#define MODE_LOCKED   0
#define MODE_KEYPAD   1
#define MODE_UNLOCKED 2
#define MODE_DEMO     3

//*****************************************************************************
//
// Prototypes for the variables and functions provided by the main application.
//
//*****************************************************************************
extern unsigned long g_ulAccessCode;
extern unsigned long g_ulTime;
extern unsigned long g_ulTimeCount;
extern unsigned long g_ulMode;
extern unsigned char g_pucLMIName[];

extern void SetAccessCode(unsigned long ulCode);
extern void GraphicsDemoShow(void);
extern void GraphicsDemoHide(void);

#endif // __QS_KEYPAD_H__
