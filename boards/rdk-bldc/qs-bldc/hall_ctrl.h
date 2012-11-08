//*****************************************************************************
//
// hall_ctrl.h - Prototypes for the routines to determine the speed of the
//               motor using Hall Sensors.
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

#ifndef __HALL_CTRL_H__
#define __HALL_CTRL_H__

//*****************************************************************************
//
// Prototypes for the exported variables and functions.
//
//*****************************************************************************
extern unsigned long g_ulHallRotorSpeed;
extern unsigned long g_ulHallValue;
extern void GPIOBIntHandler(void);
extern void HallTickHandler(void);
extern void HallInit(void);
extern void HallConfigure(void);

#endif // __HALL_CTRL_H__
