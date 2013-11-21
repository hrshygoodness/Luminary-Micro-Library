//*****************************************************************************
//
// sdram.h - Functions related to initialisation and management of SDRAM.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#ifndef _SDRAM_H_
#define _SDRAM_H_

//*****************************************************************************
//
// Public function prototypes.
//
//*****************************************************************************
extern tBoolean SDRAMInit(unsigned long ulEPIDivider, unsigned long ulConfig,
                          unsigned long ulRefresh);
extern void *ExtRAMAlloc(unsigned long ulSize);
extern void ExtRAMFree(void *pvBlock);
extern unsigned long ExtRAMMaxFree(unsigned long *pulTotalFree);

//*****************************************************************************
//
// Functions in this driver were renamed to maintain compatibility with similar
// functions in the dk-lm3s9b96 release.  The following labels are included to
// aid backwards compatibility.
//
//*****************************************************************************
#ifndef DEPRECATED
#define SDRAMAlloc(a) ExtRAMAlloc(a)
#define SDRAMFree(a) ExtRAMFree(a)
#define SDRAMMaxFree(a) ExtRAMMaxFree(a)
#endif
#endif // _SDRAM_H_
