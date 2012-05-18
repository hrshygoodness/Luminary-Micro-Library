//*****************************************************************************
//
// qs-checkout.h - Prototypes and definitions shared between the files
//                 comprising the qs-checkout example application.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __QS_CHECKOUT_H__
#define __QS_CHECKOUT_H__

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_TICK      (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// The redraw rate for the JPEG image viewer.  This is expressed in terms of
// system ticks.
//
//*****************************************************************************
#define JPEG_REDRAW_TIMEOUT 20

//*****************************************************************************
//
// Variables shared between application modules.
//
//*****************************************************************************
#define SIZE_TOUCH_COORD_BUFFER 10
extern char g_pcTouchCoordinates[];

#define SIZE_MAC_ADDR_BUFFER 18
extern char g_pucMACAddrString[];

#define SIZE_IP_ADDR_BUFFER 16
extern char g_pucIPAddrString[];

#define SIZE_THUMBWHEEL_BUFFER 8
extern char g_pucThumbwheelString[];

extern volatile unsigned long g_ulSysTickCount;

#endif // __QS_CHECKOUT_H__
