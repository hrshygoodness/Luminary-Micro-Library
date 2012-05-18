//*****************************************************************************
//
// usb_mousedfu_structs.h - Data structures defining the mouse and DFU USB
//                          device.
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
// This is part of revision 8555 of the EK-LM3S9B92 Firmware Package.
//
//*****************************************************************************

#ifndef _USB_MOUSEDFU_STRUCTS_H_
#define _USB_MOUSEDFU_STRUCTS_H_

extern unsigned long MouseHandler(void *pvCBData,
                                  unsigned long ulEvent,
                                  unsigned long ulMsgData,
                                  void *pvMsgData);
extern unsigned long DFUDetachCallback(void *pvCBData,
                                       unsigned long ulEvent,
                                       unsigned long ulMsgData,
                                       void *pvMsgData);

extern const tUSBDHIDMouseDevice g_sMouseDevice;
extern const tUSBDDFUDevice g_sDFUDevice;
extern tUSBDCompositeDevice g_sCompDevice;

#define DESCRIPTOR_BUFFER_SIZE (COMPOSITE_DDFU_SIZE + COMPOSITE_DHID_SIZE)
extern unsigned char g_pcDescriptorBuffer[];

#endif // _USB_MOUSEDFU_STRUCTS_H_
