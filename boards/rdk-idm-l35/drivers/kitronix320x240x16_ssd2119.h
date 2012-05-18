//*****************************************************************************
//
// kitronix320x240x16_ssd2119.h - Prototypes for the Kitronix K350QVG-V1-F
//                               display driver with an SSD2119 controller.
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
// This is part of revision 8555 of the RDK-IDM-L35 Firmware Package.
//
//*****************************************************************************

#ifndef __KITRONIX320X240X16_SSD2119_H__
#define __KITRONIX320X240X16_SSD2119_H__

//*****************************************************************************
//
// Prototypes for the globals exported by this driver.
//
//*****************************************************************************
extern void Kitronix320x240x16_SSD2119Init(void);
extern void Kitronix320x240x16_SSD2119BacklightOn(unsigned char ucBrightness);
extern void Kitronix320x240x16_SSD2119BacklightOff(void);
extern const tDisplay g_sKitronix320x240x16_SSD2119;

#endif // __KITRONIX320X240X16_SSD2119_H__
