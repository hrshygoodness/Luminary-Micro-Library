//****************************************************************************
//
// usb_dev_caudiohid.c - Global application definitions for the Audio HID
// composite device example.
//
// Copyright (c) 2010-2011 Texas Instruments Incorporated.  All rights reserved.
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
//****************************************************************************

#ifndef __USB_DEV_CAUDIOHID_H__
#define __USB_DEV_CAUDIOHID_H__

//****************************************************************************
//
// These defines are used to define the screen constraints to the application.
//
//****************************************************************************
#define DISPLAY_STATUS_TEXT_POSY                                             \
    ((GrContextDpyHeightGet(&g_sContext) - DISPLAY_BANNER_HEIGHT - 1 + 8))
#define DISPLAY_STATUS_MUTE_TEXT    36
#define DISPLAY_STATUS_MUTE_INSET   4
#define DISPLAY_BANNER_HEIGHT       24
#define DISPLAY_BANNER_BG           ClrDarkBlue
#define DISPLAY_TEXT_FG             ClrWhite
#define DISPLAY_TEXT_BG             ClrBlack
#define DISPLAY_MUTE_BG             ClrRed

//
// A volume update is pending.
//
#define FLAG_VOLUME_UPDATE      0

//
// A mute update is pending.
//
#define FLAG_MUTE_UPDATE        1

//
// The current state of the mute flag.
//
#define FLAG_MUTED              2

//
// A flag used to indicate whether or not we are currently connected to the
// USB host.
//
#define FLAG_CONNECTED          3

//
// A flag used to indicate if the device is suspended.
//
#define FLAG_SUSPENDED          4

//
// A flag used to indicate if the status area needs to be updated.
//
#define FLAG_STATUS_UPDATE      5

extern volatile unsigned long g_ulFlags;

extern tContext g_sContext;

void AudioInit(void);
void AudioMain(void);

void KeyboardInit(void);
void KeyboardMain(void);

void UpdateVolume(void);
void UpdateMute(void);

#endif
