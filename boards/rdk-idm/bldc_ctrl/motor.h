//*****************************************************************************
//
// motor.h - Prototypes for the BLDC motor control RDK Ethernet interface.
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

#ifndef __MOTOR_H__
#define __MOTOR_H__

//*****************************************************************************
//
// The states for the motor control Ethernet connection.
//
//*****************************************************************************
#define MOTOR_STATE_DISCON      0
#define MOTOR_STATE_CONNECTING  1
#define MOTOR_STATE_CONNECTED   2

//*****************************************************************************
//
// Prototypes for the functions for accessing the BLDC motor control RDK over
// the Ethernet.
//
//*****************************************************************************
extern unsigned long g_ulMotorState;
extern unsigned long g_ulMotorSpeed;
extern unsigned long g_ulTargetSpeed;
extern unsigned char g_pucMACAddr[];
extern void MotorInit(void);
extern void MotorConnect(void);
extern void MotorRun(void);
extern void MotorStop(void);
extern void MotorSpeedSet(unsigned long ulSpeed);

#endif // __MOTOR_H__
