//*****************************************************************************
//
// ui_ethernet.h - Prototypes for the simple TCP/IP control interface.
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
// This is part of revision 6852 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#ifndef __UI_ETHERNET_H__
#define __UI_ETHERNET_H__

//*****************************************************************************
//
// Functions and data exported by the ethernet user interface.
//
//*****************************************************************************
extern volatile unsigned long g_ulEthernetTimer;
extern volatile unsigned long g_ulEthernetTXCount;
extern volatile unsigned long g_ulEthernetRXCount;
extern unsigned char g_ucBoardID;
extern volatile unsigned long g_ulConnectionTimeoutParameter;
extern void UIEthernetSendRealTimeData(void);
extern void UIEthernetInit(tBoolean bUseDHCP);
extern void UIEthernetTick(unsigned long ulTickMS);
extern unsigned long UIEthernetGetIPAddress(void);

#endif // __UI_ETHERNET_H__
