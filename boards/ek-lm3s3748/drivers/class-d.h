//*****************************************************************************
//
// class-d.h - Prototypes for the Class-D amplifier driver.
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

#ifndef __CLASS_D_H__
#define __CLASS_D_H__

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void ClassDPWMHandler(void);
extern void ClassDInit(unsigned long ulPWMClock);
extern void ClassDPlayPCM(const unsigned char *pucBuffer,
                          unsigned long ulLength);
extern void ClassDPlayADPCM(const unsigned char *pucBuffer,
                            unsigned long ulLength);
extern tBoolean ClassDBusy(void);
extern void ClassDStop(void);
extern void ClassDVolumeSet(unsigned long ulVolume);
extern void ClassDVolumeUp(unsigned long ulVolume);
extern void ClassDVolumeDown(unsigned long ulVolume);

#endif // __CLASS_D_H__
