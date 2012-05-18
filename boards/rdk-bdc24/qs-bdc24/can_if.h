//*****************************************************************************
//
// can_if.h - Definitions and functions that are used to interact with the CAN
//            controller.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#ifndef __CAN_IF_H__
#define __CAN_IF_H__

//*****************************************************************************
//
// Prototypes for the CAN interface functions.
//
//*****************************************************************************
extern void CANIFInit(void);
extern void CANIFSetID(unsigned long ulID);
extern void CANIFEnumerate(void);
extern void CANIFPStatus(void);
extern void CANIFSendBridgeMessage(unsigned long ulID, unsigned char *pucData,
                                   unsigned long ulMsgLen);
extern void CANStatusWriteLECNoEvent(void);
extern unsigned long CANStatusRegGet(void);
extern unsigned long CANErrorRegGet(void);

#endif // __CAN_IF_H__
