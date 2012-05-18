//*****************************************************************************
//
// sounds.h - Prototypes for the music/sound effect arrays.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM3S1968 Firmware Package.
//
//*****************************************************************************

#ifndef __SOUNDS_H__
#define __SOUNDS_H__

//*****************************************************************************
//
// Declarations for the song and sound effect arrays.
//
//*****************************************************************************
extern const unsigned char g_pucIntro[58000];
extern const unsigned char g_pucStartOfGame[9117];
extern const unsigned char g_pucEndOfMaze[5184];
extern const unsigned char g_pucEndOfGame[18862];
extern const unsigned char g_pucFireEffect[651];
extern const unsigned char g_pucWallEffect[1718];
extern const unsigned char g_pucMonsterEffect[1619];
extern const unsigned char g_pucPlayerEffect[2507];

#endif // __SOUNDS_H__
