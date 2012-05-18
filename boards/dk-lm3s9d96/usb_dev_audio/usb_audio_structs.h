//*****************************************************************************
//
// usb_audio_structs.h - Data structures defining the audio USB device.
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
// This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#ifndef _USB_AUDIO_STRUCTS_H_
#define _USB_AUDIO_STRUCTS_H_

//*****************************************************************************
//
// These are the Volume control basics that are provided back to the USB host
// controller and are based on the Texas Instruments TLV320AIC23B device.
//
//*****************************************************************************
#define VOLUME_MAX              ((short)0x0C00)  // +12db
#define VOLUME_MIN              ((short)0xDC80)  // -34.5db
#define VOLUME_STEP             ((short)0x0180)  // 1.5db

extern const tUSBDAudioDevice g_sAudioDevice;

#endif
