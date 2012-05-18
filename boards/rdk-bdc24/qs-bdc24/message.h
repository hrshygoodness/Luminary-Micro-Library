//*****************************************************************************
//
// message.h - Prototypes for the message handling functions.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

//*****************************************************************************
//
// Prototypes for the message handling functions.
//
//*****************************************************************************
extern unsigned char g_pucResponse[12];
extern unsigned long g_ulResponseLength;
extern unsigned char g_ppucPStatMessages[4][8];
extern unsigned char g_pucPStatMessageLen[4];
extern unsigned long g_ulPStatFlags;
extern unsigned long MessageCommandHandler(unsigned long ulID,
                                           unsigned char *pucData,
                                           unsigned long ulMsgLen);
extern unsigned long MessageUpdateHandler(unsigned long ulID,
                                          unsigned char *pucData,
                                          unsigned long ulMsgLen);
extern void MessageTick(void);
extern void MessageButtonPress(void);

#endif // __MESSAGE_H__
