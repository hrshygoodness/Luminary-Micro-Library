//*****************************************************************************
//
// ui_serial.h - Prototypes for the simple UART control interface.
//
// Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#ifndef __UI_SERIAL_H__
#define __UI_SERIAL_H__

//*****************************************************************************
//
// Functions exported by the serial user interface.
//
//*****************************************************************************
extern void UART0IntHandler(void);
extern void UISerialSendRealTimeData(void);
extern void UISerialInit(void);

#endif // __UI_SERIAL_H__
