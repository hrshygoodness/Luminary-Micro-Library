//*****************************************************************************
//
// usb_mouse.h - Header file for USB mouse functions.
//
// Copyright (c) 2009-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef _USB_MOUSE_H_
#define _USB_MOUSE_H_

//*****************************************************************************
//
// Bit masks used to indicate to callers which parameters have changed, allowing
// them to make appropriate updates to the user interface.
//
//*****************************************************************************
#define MOUSE_FLAG_CONNECTION    0x01
#define MOUSE_FLAG_POSITION      0x02
#define MOUSE_FLAG_BUTTONS       0x04
#define MSC_FLAG_CONNECTION      0x08

//*****************************************************************************
//
// Bit masks used to indicate the button states.  These flags are returned by
// USBMouseButtonsGet().  A set bit indicates that the button is pressed.
//
//*****************************************************************************
#define MOUSE_BTN_1             0x01
#define MOUSE_BTN_2             0x02
#define MOUSE_BTN_3             0x04

//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************
extern tBoolean USBFuncsInit(unsigned long ulScreenWidth,
                             unsigned long ulScreenHeight);
extern unsigned long USBFuncsProcess(void);
extern void USBMouseHostPositionGet(short *psX, short *psY);
extern unsigned long USBMouseHostButtonsGet(void);
extern tBoolean USBMouseIsConnected(tBoolean *pbDeviceMode);
extern tBoolean USBMSCIsConnected(void);
extern void USBMouseTouchHandler(unsigned long ulMessage, long lX, long lY);
extern unsigned long USBDeviceMouseCallback(void *pvCBData,
                                            unsigned long ulEvent,
                                            unsigned long ulMsgData,
                                            void *pvMsgData);
#endif // _USB_MOUSE_H_
