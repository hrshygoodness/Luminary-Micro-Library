//*****************************************************************************
//
// usbdescriptors.h - The usb descriptor information for this project.
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
// This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#ifndef __USBDESCRIPTORS_H__
#define __USBDESCRIPTORS_H__

//*****************************************************************************
//
// Pre-declarations of the handlers.
//
//*****************************************************************************
extern void HandleHIDDescriptor(unsigned long ulIndex,
                                tUSBRequest *pUSBRequest);
extern void HandleRequests(unsigned long ulIndex, tUSBRequest *pUSBRequest);
extern void ConfigurationChange(unsigned long ulIndex, unsigned long ulInfo);
extern void EP1Handler(unsigned long ulIndex, unsigned long ulStatus);
extern void HandleReset(unsigned long ulIndex);
extern void HandleDisconnect(unsigned long ulIndex);

extern unsigned long const g_ulReportDescriptorSize;
extern unsigned char const g_pucReportDescriptor[];
extern tDeviceInfo g_sMouseDeviceInfo;

#endif // __USBDESCRIPTORS_H__
