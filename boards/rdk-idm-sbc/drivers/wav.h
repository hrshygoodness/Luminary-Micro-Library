//*****************************************************************************
//
// wav.h - Public prototypes and definitions for the wav audio playback driver.
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
// This is part of revision 8555 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************
#ifndef __WAV_H__
#define __WAV_H__

//*****************************************************************************
//
//! \addtogroup wav_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The header information parsed from a ``.wav'' file during a call to the
//! function WaveOpen.
//
//*****************************************************************************
typedef struct
{
    //
    //! Sample rate in bytes per second.
    //
    unsigned long ulSampleRate;

    //
    //! The average byte rate for the wav file.
    //
    unsigned long ulAvgByteRate;

    //
    //! The size of the wav data in the file.
    //
    unsigned long ulDataSize;

    //
    //! The number of bits per sample.
    //
    unsigned short usBitsPerSample;

    //
    //! The wav file format.
    //
    unsigned short usFormat;

    //
    //! The number of audio channels.
    //
    unsigned short usNumChannels;
}
tWaveHeader;

//*****************************************************************************
//
//! Possible return codes from the WaveOpen function.
//
//*****************************************************************************
typedef enum
{
    //
    //! The wav data was parsed successfully.
    //
    WAVE_OK = 0,

    //
    //! The RIFF information in the wav data is not supported.
    //
    WAVE_INVALID_RIFF,

    //
    //! The chunk size specified in the wav data is not supported.
    //
    WAVE_INVALID_CHUNK,

    //
    //! The format of the wav data is not supported.
    //
    WAVE_INVALID_FORMAT
}
tWaveReturnCode;

tWaveReturnCode WaveOpen(unsigned long *pulAddress, tWaveHeader *pWaveHeader);
void WavePlayStart(tWaveHeader *pWaveHeader);
tBoolean WavePlayContinue(tWaveHeader *pWaveHeader);
void WaveStop(void);
void WaveGetTime(tWaveHeader *pWaveHeader, char *pcTime, unsigned long ulSize);
tBoolean WavePlaybackStatus(void);

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
#endif // __WAV_H__
