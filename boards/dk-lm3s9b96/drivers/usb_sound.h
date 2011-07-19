//*****************************************************************************
//
// usb_sound.h - USB host audio handling header definitions.
//
// Copyright (c) 2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef USB_SOUND_H_
#define USB_SOUND_H_

//*****************************************************************************
//
// The following are defines for the ulEvent value that is returned with the
// tEventCallback function provided to the USBSoundInit() function.
//
//*****************************************************************************

//
// A USB audio device has been connected.
//
#define SOUND_EVENT_READY       0x00000001

//
// A USB device has been disconnected.
//
#define SOUND_EVENT_DISCONNECT  0x00000002

//
// An unknown device has been connected.
//
#define SOUND_EVENT_UNKNOWN_DEV 0x00000003

typedef void (* tUSBBufferCallback)(void *pvBuffer, unsigned long ulEvent);
typedef void (* tEventCallback)(unsigned long ulEvent, unsigned long ulParam);

extern void USBMain(unsigned long ulTicks);

extern void USBSoundInit(unsigned long ulEnableReceive,
                      tEventCallback pfnCallback);
extern void USBSoundVolumeSet(unsigned long ulPercent);
extern unsigned long USBSoundVolumeGet(unsigned long ulChannel);

extern unsigned long USBSoundOutputFormatGet(unsigned long ulSampleRate,
                                             unsigned long ulBits,
                                             unsigned long ulChannels);
extern unsigned long USBSoundOutputFormatSet(unsigned long ulSampleRate,
                                             unsigned long ulBits,
                                             unsigned long ulChannels);
extern unsigned long USBSoundInputFormatGet(unsigned long ulSampleRate,
                                            unsigned long ulBitsPerSample,
                                            unsigned long ulChannels);
extern unsigned long USBSoundInputFormatSet(unsigned long ulSampleRate,
                                            unsigned long ulBits,
                                            unsigned long ulChannels);

extern unsigned long USBSoundBufferOut(const void *pvData,
                                       unsigned long ulLength,
                                       tUSBBufferCallback pfnCallback);

extern unsigned long USBSoundBufferIn(const void *pvData,
                                      unsigned long ulLength,
                                      tUSBBufferCallback pfnCallback);

#endif
