//*****************************************************************************
//
// sinemod.h - Prototypes for the sine wave modulation routine.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#ifndef __SINEMOD_H__
#define __SINEMOD_H__

//*****************************************************************************
//
// Prototype for the sine wave modulation routine.
//
//*****************************************************************************
extern void SineModulate(unsigned long ulAngle, unsigned long ulAmplitude,
                         unsigned long *pulDutyCycles);

#endif // __SINEMOD_H__
