//*****************************************************************************
//
// stepseq.h - Prototypes and definitions for the step sequencing module.
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
// This is part of revision 9453 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#ifndef __STEPSEQ_H__
#define __STEPSEQ_H__

//*****************************************************************************
//
// Prototypes for the exported variables and functions.
//
//*****************************************************************************
extern volatile long g_lCurrentPos;
extern unsigned char g_ucMotorStatus;
extern unsigned short g_usPwmFreq;
extern unsigned long g_ulStepTime;

extern void StepSeqHandler(void);
extern void StepSeqMove(long lNewPosition, unsigned short usSpeed,
                        unsigned short usAccel, unsigned short usDecel);
extern void StepSeqInit(void);
extern void StepSeqShutdown(void);
extern void StepSeqStop(void);
extern void StepSeqCurrent(unsigned short usDrive, unsigned short usHold,
                           unsigned long ulMax);
extern void StepSeqDecayMode(unsigned char ucDecayMode);
extern void StepSeqControlMode(unsigned char ucControlMode);
extern void StepSeqStepMode(unsigned char ucStepMode);

#endif
