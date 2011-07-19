//*****************************************************************************
//
// speed_sense.h - Prototypes for the routines to determine the speed of the
//                 motor (if an encoder is present).
//
// Copyright (c) 2007-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6852 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#ifndef __SPEED_SENSE_H__
#define __SPEED_SENSE_H__

//*****************************************************************************
//
// Prototypes for the exported variables and functions.
//
//*****************************************************************************
extern unsigned short g_usRotorFrequency;
extern void GPIOCIntHandler(void);
extern void QEIIntHandler(void);
extern void SpeedSenseInit(void);

#endif // __SPEED_SENSE_H__
