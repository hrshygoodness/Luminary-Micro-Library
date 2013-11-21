//*****************************************************************************
//
// audio.h - Prototypes for the music/sound generation functions.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#ifndef __AUDIO_H__
#define __AUDIO_H__

//*****************************************************************************
//
// The frequencies of the piano keys, for convenience when constructing a song.
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
// Prototypes for the music and sound effect fuctions.
//
//*****************************************************************************
extern void AudioVolumeUp(unsigned long ulPercent);
extern void AudioVolumeDown(unsigned long ulPercent);
extern unsigned char AudioVolumeGet(void);
extern void AudioVolume(unsigned long ulPercent);
extern void AudioHandler(void);
extern void AudioOff(void);
extern void AudioOn(void);
extern void AudioPlaySong(const unsigned short *pusSong,
                          unsigned long ulLength);
extern void AudioPlaySound(const unsigned short *pusSound,
                           unsigned long ulLength);

#endif // __AUDIO_H__
