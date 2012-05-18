//*****************************************************************************
//
// fatwrapper.h - Prototypes for the FAT font wrapper module.
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
// This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#ifndef __FATWRAPPER_H__
#define __FATWRAPPER_H__

//*****************************************************************************
//
// Exported globals.
//
//*****************************************************************************
extern const tFontAccessFuncs g_sFATFontAccessFuncs;

//*****************************************************************************
//
// Public function prototypes.
//
//*****************************************************************************
extern void FATWrapperSysTickHandler(void);
extern tBoolean FATFontWrapperInit(void);
extern unsigned char *FATFontWrapperLoad(char *pcFilename);
extern void FATFontWrapperUnload(unsigned char *pucFontId);

#endif
