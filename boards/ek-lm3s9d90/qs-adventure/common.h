//*****************************************************************************
//
// common.h - Prototypes for the various source modules for the game.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM3S9D90 Firmware Package.
//
//*****************************************************************************

#ifndef __COMMON_H__
#define __COMMON_H__

//*****************************************************************************
//
// The following defines describe the communication interface being used to
// play the game.
//
//*****************************************************************************
#define GAME_IF_NONE            0
#define GAME_IF_ENET            1
#define GAME_IF_USB             2

//*****************************************************************************
//
// These global variables are provided by qs-adventure.c.
//
//*****************************************************************************
extern const unsigned char g_pucErrorMessage[53];
extern unsigned long g_ulTime;
extern volatile unsigned long g_ulGameIF;
extern unsigned long g_ulRestart;

//*****************************************************************************
//
// These functions are provided by enet_if.c.
//
//*****************************************************************************
extern void EnetIFTick(unsigned long ulMS);
extern void EnetIFInit(void);
extern unsigned char EnetIFRead(void);
extern void EnetIFWrite(unsigned char ucChar);
extern void EnetIFClose(void);

//*****************************************************************************
//
// These functions are provided by usb_if.c.
//
//*****************************************************************************
extern void USBIFInit(void);
extern unsigned char USBIFRead(void);
extern void USBIFWrite(unsigned char ucChar);

#endif // __COMMON_H__
