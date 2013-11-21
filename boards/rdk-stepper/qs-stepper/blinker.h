//*****************************************************************************
//
// blinker.h - Prototypes for the LED blinking functions.
//
// Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#ifndef __BLINKER_H__
#define __BLINKER_H__

//*****************************************************************************
//
// Prototypes for the exported functions.
//
//*****************************************************************************
void BlinkInit(unsigned long ulIdx, unsigned long ulPort, unsigned long ulPin);
void BlinkStart(unsigned long ulIdx, unsigned long ulOn,
                unsigned long ulOff, unsigned long ulRepeat);
void BlinkUpdate(unsigned long ulIdx, unsigned long ulOn, unsigned long ulOff);
void BlinkHandler(void);

#endif
