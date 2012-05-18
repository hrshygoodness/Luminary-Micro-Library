//*****************************************************************************
//
// wavfile.h - This file supports reading audio data from a .wav file and
// reading the file format.
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
// This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#ifndef WAVEFILE_H_
#define WAVEFILE_H_

//*****************************************************************************
//
// The wav file header information.
//
//*****************************************************************************
typedef struct
{
    //
    // Sample rate in bytes per second.
    //
    unsigned long ulSampleRate;

    //
    // The average byte rate for the wav file.
    //
    unsigned long ulAvgByteRate;

    //
    // The size of the wav data in the file.
    //
    unsigned long ulDataSize;

    //
    // The number of bits per sample.
    //
    unsigned short usBitsPerSample;

    //
    // The wav file format.
    //
    unsigned short usFormat;

    //
    // The number of audio channels.
    //
    unsigned short usNumChannels;
}
tWavHeader;

//*****************************************************************************
//
// The structure used to hold the wav file state.
//
//*****************************************************************************
typedef struct
{
    //
    // The wav files header information
    //
    tWavHeader sWavHeader;

    //
    // The file information for the current file.
    //
    FIL sFile;

    //
    // Current state flags, a combination of the WAV_FLAG_* values.
    //
    unsigned long ulFlags;
} tWavFile;

void WavGetFormat(tWavFile *psWavData, tWavHeader *pWaveHeader);
int WavOpen(const char *pcFileName, tWavFile *psWavData);
void WavClose(tWavFile *psWavData);
unsigned short WavRead(tWavFile *psWavData, unsigned char *pucBuffer,
                        unsigned long ulSize);

#endif
