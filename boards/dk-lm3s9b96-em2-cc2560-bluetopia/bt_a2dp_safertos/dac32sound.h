//*****************************************************************************
//
// dac32sound.h - Macro Definitions to abstract the processor and compiler
//                specific formats and functions
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2560-BLUETOPIA Firmware Package.
//
//*****************************************************************************

#ifndef __DAC32SOUND_H__
#define __DAC32SOUND_H__

#include "inc/hw_memmap.h"

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
// The following defines the platform specific I2C parameters.
//
//*****************************************************************************
#define I2C_ERROR_CODE          (0xFFFF)
#define I2C_BASE_ADDRESS        I2C0_MASTER_BASE

//*****************************************************************************
//
// The following defines the starting Page used to access the DAC registers.
//
//*****************************************************************************
#define DEFAULT_CURRENT_PAGE    0

//*****************************************************************************
//
// The following defines the I2C address where the DAC is located.
//
//*****************************************************************************
#define I2C_DAC_ADDR            0x1A

//*****************************************************************************
//
// The following defines the volume level that the Headset Audio will be set at
// powerup.
//
//*****************************************************************************
#define DEFAULT_POWERUP_VOLUME  90

//*****************************************************************************
//
// The following are the API Function Prototypes for this module
//
//*****************************************************************************
extern void DAC32SoundInit(void);
extern void SoundSetFormat(unsigned long ulSampleRate);
extern void SoundVolumeSet(unsigned int uiPercent);
extern unsigned int SoundVolumeGet(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif   //__DAC32SOUND_H__

