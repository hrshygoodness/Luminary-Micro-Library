//*****************************************************************************
//
// buttons.h - Prototypes for the on-board push button handling code.
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

#ifndef __BUTTONS_H__
#define __BUTTONS_H__

//*****************************************************************************
//
// The bit positions of the push buttons in the value returned by
// ButtonsTick().
//
//*****************************************************************************
#define BUTTON_UP               0x01
#define BUTTON_DOWN             0x02
#define BUTTON_LEFT             0x04
#define BUTTON_RIGHT            0x08
#define BUTTON_SELECT           0x10

//*****************************************************************************
//
// Prototypes for the button driver.
//
//*****************************************************************************
extern void ButtonsInit(void);
extern unsigned long ButtonsTick(void);

#endif // __BUTTONS_H__
