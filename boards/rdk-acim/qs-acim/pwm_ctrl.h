//*****************************************************************************
//
// pwm_ctrl.h - Prototypes for the PWM control routines.
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

#ifndef __PWM_CTRL_H__
#define __PWM_CTRL_H__

//*****************************************************************************
//
// Prototypes for the exported variables and functions.
//
//*****************************************************************************
extern unsigned long g_ulPWMFrequency;
extern void PWMSetMinPulseWidth(void);
extern void PWMSetDeadBand(void);
extern void PWMSetFrequency(void);
extern void PWM0IntHandler(void);
extern unsigned long PWMGetPeriodCount(void);
extern void PWMReducePeriodCount(unsigned long ulCount);
extern void PWMSetDutyCycle(unsigned long ulDutyCycleU,
                            unsigned long ulDutyCycleV,
                            unsigned long ulDutyCycleW);
extern void PWMOutputPrecharge(void);
extern void PWMOutputOn(void);
extern void PWMOutputOff(void);
extern void PWMOutputDCBrake(unsigned long ulVoltage);
extern void PWMSetUpdateRate(unsigned char ucUpdateRate);
extern void GPIOBIntHandler(void);
extern void PWMInit(void);

#endif // __PWM_CTRL_H__
