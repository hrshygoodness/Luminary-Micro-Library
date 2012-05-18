//****************************************************************************
//
// usb_structs.h - Data structures defining the composite HID mouse and CDC
// serial USB device.
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM3S9B90 Firmware Package.
//
//****************************************************************************

#ifndef __USB_STRUCTS_H__
#define __USB_STRUCTS_H__

//****************************************************************************
//
// Globals used by both classes.
//
//****************************************************************************
extern volatile unsigned long g_ulFlags;
extern volatile unsigned long g_ulSysTickCount;

//****************************************************************************
//
// The flags used by this application for the g_ulFlags value.
//
//****************************************************************************
#define FLAG_MOVE_UPDATE       0
#define FLAG_CONNECTED         1
#define FLAG_LED_ACTIVITY      2
#define FLAG_MOVE_MOUSE        3
#define FLAG_COMMAND_RECEIVED  4

//****************************************************************************
//
// The size of the transmit and receive buffers used for the redirected UART.
// This number should be a power of 2 for best performance.  256 is chosen
// pretty much at random though the buffer should be at least twice the size
// of a maximum-sized USB packet.
//
//****************************************************************************
#define UART_BUFFER_SIZE 256

extern void MouseInit(void);
extern void MouseMain(void);

extern void SerialInit(void);
extern void SerialMain(void);

//****************************************************************************
//
// CDC device callback function prototypes.
//
//****************************************************************************
extern unsigned long RxHandler(void *pvCBData, unsigned long ulEvent,
                        unsigned long ulMsgValue, void *pvMsgData);
extern unsigned long TxHandler(void *pvCBData, unsigned long ulEvent,
                        unsigned long ulMsgValue, void *pvMsgData);
extern unsigned long EventHandler(void *pvCBData, unsigned long ulEvent,
                             unsigned long ulMsgValue, void *pvMsgData);
extern unsigned long MouseHandler(void *pvCBData, unsigned long ulEvent,
                                  unsigned long ulMsgData, void *pvMsgData);
extern unsigned long SerialHandler(void *pvCBData, unsigned long ulEvent,
                                   unsigned long ulMsgValue, void *pvMsgData);

#endif
