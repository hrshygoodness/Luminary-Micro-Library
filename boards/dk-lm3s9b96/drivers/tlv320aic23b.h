//*****************************************************************************
//
// tlv320aic23b.h - Prototypes and macros for the I2S controller.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __TLV320AIC23B_H__
#define __TLV320AIC23B_H__

//
// The values to use with the TLV320AIC23BLineInVolumeSet() function.
//
#define TLV_LINEIN_VC_MAX       0x1f
#define TLV_LINEIN_VC_MIN       0x00
#define TLV_LINEIN_VC_0DB       0x17
#define TLV_LINEIN_VC_MUTE      0x80

tBoolean TLV320AIC23BInit(void);
void TLV320AIC23BHeadPhoneVolumeSet(unsigned long ulVolume);
unsigned long TLV320AIC23BHeadPhoneVolumeGet(void);
void TLV320AIC23BLineInVolumeSet(unsigned char ucVolume);

#endif // __TLV320AIC23B_H__
