//*****************************************************************************
//
// sound.h - Definitions and prototypes for the EVALBOT sound driver.
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************
#ifndef __SOUND_H__
#define __SOUND_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Event definitions for the buffer callback function.
//
//*****************************************************************************
#define BUFFER_EVENT_FREE       0x00000001
#define BUFFER_EVENT_FULL       0x00000002

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void SoundInit(void);
extern void SoundIntHandler(void);
extern void SoundSetFormat(unsigned long ulSampleRate,
                           unsigned short usBitsPerSample,
                           unsigned short usChannels);
extern unsigned long SoundSampleRateGet(void);
extern void SoundBufferPlay(const void *pvData, unsigned long ulLength,
                            void (*pfnCallback)(void *pvBuffer,
                                                unsigned long ulEvent));
extern void SoundVolumeSet(unsigned long ulPercent);
extern unsigned char SoundVolumeGet(void);
extern void SoundVolumeDown(unsigned long ulPercent);
extern void SoundVolumeUp(unsigned long ulPercent);
extern void SoundClassDEn(void);
extern void SoundClassDDis(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SOUND_H__
