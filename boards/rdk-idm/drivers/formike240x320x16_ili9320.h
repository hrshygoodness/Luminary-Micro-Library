//*****************************************************************************
//
// formike240x320x16_ili9320.h - Prototypes for the Formike Electronic
//                               KWH028Q02-F03 display with an ILI9320
//                               controller and KWH028Q02-F05 display with an
//                               ILI9325 controller.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#ifndef __FORMIKE240X320X16_ILI9320_H__
#define __FORMIKE240X320X16_ILI9320_H__

//*****************************************************************************
//
// Prototypes for the globals exported by this driver.
//
//*****************************************************************************
extern void Formike240x320x16_ILI9320Init(void);
extern void Formike240x320x16_ILI9320BacklightOn(void);
extern unsigned short Formike240x320x16_ILI9320ControllerIdGet(void);
extern void Formike240x320x16_ILI9320BacklightOff(void);
extern const tDisplay g_sFormike240x320x16_ILI9320;

#endif // __FORMIKE240X320X16_ILI9320_H__
