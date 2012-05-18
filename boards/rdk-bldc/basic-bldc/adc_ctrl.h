//*****************************************************************************
//
// adc_ctrl.h - Prototypes for the ADC control routines.
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
// This is part of revision 8555 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#ifndef __ADC_CTRL_H__
#define __ADC_CTRL_H__

//*****************************************************************************
//
// Prototypes for the exported variables and functions.
//
//*****************************************************************************
extern short g_psPhaseCurrent[3];
extern unsigned long g_ulPhaseBEMFVoltage;
extern unsigned long g_ulMotorPower;
extern short g_sMotorCurrent;
extern unsigned long g_ulBEMFRotorSpeed;
extern unsigned long g_ulBEMFHallValue;
extern unsigned long g_ulBEMFNextHall;
extern unsigned long g_ulBusVoltage;
extern unsigned char g_ucAmbientTemp;
extern unsigned long g_ulLinearHallValue;
extern unsigned long g_ulLinearRotorSpeed;
extern void ADC0IntHandler(void);
extern void ADCInit(void);
extern void ADCConfigure(void);
extern void ADCTickHandler(void);
extern unsigned long ADCReadAnalog(void);

#endif // __ADC_CTRL_H__
