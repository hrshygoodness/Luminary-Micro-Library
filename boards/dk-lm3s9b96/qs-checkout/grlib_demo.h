//*****************************************************************************
//
// grlib_demo.h - Prototypes and definitions related to the widget set used in
//                the user interface of the qs-checkout application.
//
// Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __GRLIB_DEMO__
#define __GRLIB_DEMO__

//*****************************************************************************
//
// Functions and variables exported by this module.
//
//*****************************************************************************
extern void GraphicsDemoInit(void);

extern const unsigned short g_pusKeyClick[];
extern unsigned long g_ulKeyClickLen;

#endif // __GRLIB_DEMO__
