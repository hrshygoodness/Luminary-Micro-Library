//*****************************************************************************
//
// extram.h - Functions related to initialisation and management of external
//            RAM.
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

#ifndef __EXTRAM_H__
#define __EXTRAM_H__

//*****************************************************************************
//
// Public function prototypes.
//
//*****************************************************************************
extern tBoolean SDRAMInit(unsigned long ulEPIDivider, unsigned long ulConfig,
                          unsigned long ulRefresh);
extern tBoolean ExtRAMHeapInit(void);
extern void *ExtRAMAlloc(unsigned long ulSize);
extern void ExtRAMFree(void *pvBlock);
extern unsigned long ExtRAMMaxFree(unsigned long *pulTotalFree);

//*****************************************************************************
//
// With the addition of external RAM on other daughter boards in addition to the
// basic SDRAM daughter, this driver and various function it contains were
// renamed.  The previous functions are deprecated but defined here to aid
// backwards compatibility.
//
//*****************************************************************************
#ifndef DEPRECATED
#define SRAMHeapInit(a) ExtRAMHeapInit(a)
#define SDRAMAlloc(a) ExtRAMAlloc(a)
#define SDRAMFree(a) ExtRAMFree(a)
#define SDRAMMaxFree(a) ExtRAMMaxFree(a)
#endif

#endif // __EXTRAM_H__
