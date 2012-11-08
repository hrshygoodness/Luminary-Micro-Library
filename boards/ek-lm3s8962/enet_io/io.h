//*****************************************************************************
//
// io.h - Prototypes for I/O routines for the enet_io example.
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
// This is part of revision 9453 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#ifndef __IO_H__
#define __IO_H__

#ifdef __cplusplus
extern "C"
{
#endif

void io_init(void);
void io_set_led(tBoolean bOn);
void io_set_pwm(tBoolean bOn);
void io_pwm_freq(unsigned long ulFreq);
void io_pwm_dutycycle(unsigned long ulDutyCycle);
unsigned long io_get_pwmfreq(void);
unsigned long io_get_pwmdutycycle(void);
void io_get_ledstate(char * pcBuf, int iBufLen);
int io_is_led_on(void);
void io_get_pwmstate(char * pcBuf, int iBufLen);
int io_is_pwm_on(void);

#ifdef __cplusplus
}
#endif

#endif // __IO_H__
