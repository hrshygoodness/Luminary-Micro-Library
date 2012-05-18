//*****************************************************************************
//
// buttons.h - Prototypes for the on-board push button handling functions.
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
// This is part of revision 8555 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#ifndef __BUTTONS_H__
#define __BUTTONS_H__

//*****************************************************************************
//
// Defines for the hardware resources used by the pushbuttons.
//
//*****************************************************************************
#define BUTTONS_GPIO_PERIPH     SYSCTL_PERIPH_GPIOB
#define BUTTONS_GPIO_BASE       GPIO_PORTB_BASE

#define NUM_BUTTONS             5
#define UP_BUTTON               GPIO_PIN_3
#define DOWN_BUTTON             GPIO_PIN_4
#define LEFT_BUTTON             GPIO_PIN_5
#define RIGHT_BUTTON            GPIO_PIN_6
#define SELECT_BUTTON           GPIO_PIN_7

#define ALL_BUTTONS             (LEFT_BUTTON | RIGHT_BUTTON | UP_BUTTON | \
                                 DOWN_BUTTON | SELECT_BUTTON)

//*****************************************************************************
//
// Useful macros for detecting button events.
//
//*****************************************************************************
#define BUTTON_PRESSED(button, buttons, changed)            \
        (((button) & (changed)) && !((button) & (buttons)))

#define BUTTON_RELEASED(button, buttons, changed)          \
        (((button) & (changed)) && ((button) & (buttons)))

#define BUTTON_REPEAT(button, repeats) \
        ((button) & (repeats))

//*****************************************************************************
//
// Functions exported from buttons.c
//
//*****************************************************************************
extern void ButtonsInit(void);
extern unsigned char ButtonsPoll(unsigned char *pucDelta,
                                 unsigned char *pucRepeat);
extern void ButtonsSetAutoRepeat(unsigned char ucButtonIDs,
                                 unsigned char ucInitialTicks,
                                 unsigned char ucRepeatTicks);

#endif // __BUTTONS_H__
