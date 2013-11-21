//*****************************************************************************
//
// sound.h - Prototypes for the sound driver.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9107 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __SOUND_H__
#define __SOUND_H__

//*****************************************************************************
//
// The frequencies of the piano keys, for convenience when constructing a song.
// Note that the minimum frequency that can be produced is processor clock
// divided by 65536; at 50 MHz this equates to 763 Hz.  Lower audio frequencies
// are available if the processor clock is lowered, at the cost of lower
// processor performance (most noticable in the screen update speed).
//
//*****************************************************************************
#define SILENCE                 40000
#define A0                      28
#define AS0                     29
#define B0                      31
#define C1                      33
#define CS1                     35
#define D1                      37
#define DS1                     39
#define E1                      41
#define F1                      44
#define FS1                     46
#define G1                      49
#define GS1                     52
#define A1                      55
#define AS1                     58
#define B1                      62
#define C2                      65
#define CS2                     69
#define D2                      73
#define DS2                     78
#define E2                      82
#define F2                      87
#define FS2                     92
#define G2                      98
#define GS2                     104
#define A2                      110
#define AS2                     117
#define B2                      123
#define C3                      131
#define CS3                     139
#define D3                      147
#define DS3                     156
#define E3                      165
#define F3                      175
#define FS3                     185
#define G3                      196
#define GS3                     208
#define A3                      220
#define AS3                     233
#define B3                      247
#define C4                      262
#define CS4                     277
#define D4                      294
#define DS4                     311
#define E4                      330
#define F4                      349
#define FS4                     370
#define G4                      392
#define GS4                     415
#define A4                      440
#define AS4                     466
#define B4                      494
#define C5                      523
#define CS5                     554
#define D5                      587
#define DS5                     622
#define E5                      659
#define F5                      698
#define FS5                     740
#define G5                      784
#define GS5                     831
#define A5                      880
#define AS5                     932
#define B5                      988
#define C6                      1047
#define CS6                     1109
#define D6                      1175
#define DS6                     1245
#define E6                      1319
#define F6                      1397
#define FS6                     1480
#define G6                      1568
#define GS6                     1661
#define A6                      1760
#define AS6                     1865
#define B6                      1976
#define C7                      2093
#define CS7                     2217
#define D7                      2349
#define DS7                     2489
#define E7                      2637
#define F7                      2794
#define FS7                     2960
#define G7                      3136
#define GS7                     3322
#define A7                      3520
#define AS7                     3729
#define B7                      3951
#define C8                      4186

//*****************************************************************************
//
// Enables using uDMA to fill the I2S fifo.
//
//*****************************************************************************
#define BUFFER_EVENT_FREE       0x00000001
#define BUFFER_EVENT_FULL       0x00000002

typedef void (* tBufferCallback)(void *pvBuffer, unsigned long ulEvent);

//*****************************************************************************
//
// Prototypes for the music and sound effect functions.
//
//*****************************************************************************
extern void SoundInit(unsigned long ulEnableReceive);
extern void SoundIntHandler(void);
extern void SoundPlay(const unsigned short *pusSong, unsigned long ulLength);
extern void SoundVolumeDown(unsigned long ulPercent);
extern unsigned char SoundVolumeGet(void);
extern void SoundVolumeSet(unsigned long ulPercent);
extern void SoundVolumeUp(unsigned long ulPercent);
extern void SoundSetFormat(unsigned long ulSampleRate,
                           unsigned short usBitsPerSample,
                           unsigned short usChannels);
extern unsigned long SoundSampleRateGet(void);
extern unsigned long SoundBufferPlay(const void *pvData, unsigned long ulLength,
                                     tBufferCallback pfnCallback);
extern unsigned long SoundBufferRead(void *pvData, unsigned long ulLength,
                                     tBufferCallback pfnCallback);

#endif // __SOUND_H__
