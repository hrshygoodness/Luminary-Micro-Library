//*****************************************************************************
//
// ssiflash.h - Header file for the Winbond Serial Flash driver for the
//              development boards.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __SSIFLASH_H__
#define __SSIFLASH_H__

//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************
extern tBoolean SSIFlashInit(void);
extern tBoolean SSIFlashIDGet(unsigned char *pucManufacturer,
                              unsigned char *pucDevice);
extern unsigned long SSIFlashChipSizeGet(void);
extern unsigned long SSIFlashSectorSizeGet(void);
extern unsigned long SSIFlashBlockSizeGet(void);
extern tBoolean SSIFlashIsBusy(void);
extern tBoolean SSIFlashSectorErase(unsigned long ulAddress, tBoolean bSync);
extern tBoolean SSIFlashBlockErase(unsigned long ulAddress, tBoolean bSync);
extern tBoolean SSIFlashChipErase(tBoolean bSync);
extern unsigned long SSIFlashRead(unsigned long ulAddress,
                                  unsigned long ulLength,
                                  unsigned char *pucDst);
extern unsigned long SSIFlashWrite(unsigned long ulAddress,
                                   unsigned long ulLength,
                                   unsigned char *pucSrc);

#endif // __SSIFLASH_H__
