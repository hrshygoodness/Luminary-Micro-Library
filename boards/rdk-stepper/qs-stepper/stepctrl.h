//*****************************************************************************
//
// stepctrl.h - Prototypes and definitions for the stepper control module
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __STEPCTRL_H__
#define __STEPCTRL_H__

//*****************************************************************************
//
// Prototypes for the exported variables and functions.
//
//*****************************************************************************
extern unsigned short g_usFixedOnTime;
extern unsigned short g_usBlankOffTime;
extern unsigned long g_ulPwmPeriod;
extern unsigned long g_ulPeakCurrentRaw[2];

void StepCtrlInit(void);
void StepCtrlOpenPWMMode(unsigned long ulPeriod);
void StepCtrlClosedPWMMode(unsigned long ulPeriod);
void StepCtrlChopMode(void);
void StepCtrlChopSlow(unsigned long ulWinding, long lSetting);
void StepCtrlChopFast(unsigned long ulWinding, long lSetting);
void StepCtrlOpenPwmSlow(unsigned long ulWinding, long lSetting);
void StepCtrlOpenPwmFast(unsigned long ulWinding, long lSetting);
void StepCtrlClosedPwmSlow(unsigned long ulWinding, long lSetting);
void StepCtrlClosedPwmFast(unsigned long ulWinding, long lSetting);
void StepCtrlTimerAIntHandler(void);
void StepCtrlTimerBIntHandler(void);
void StepCtrlADCAIntHandler(void);
void StepCtrlADCBIntHandler(void);

#endif
