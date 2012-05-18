//*****************************************************************************
//
// file.h - Public function prototypes and globals related to file handling
//          in the RDK-IDM-SBC checkout application.
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
// This is part of revision 8555 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#ifndef __FILE_H__
#define __FILE_H__

extern void FileTickHandler(void);
extern tBoolean FileInit(void);
extern tBoolean FileCatToUART(char *pcFilename);
extern tBoolean FileLsToUART(char *pcDir);
extern tBoolean FileIsDrivePresent(unsigned char ucDriveNum);
extern unsigned long FileCountJPEGFiles(void);
extern tBoolean FileGetJPEGFileInfo(unsigned long ulIndex, char **ppcFilename,
                                    unsigned long *pulLen,
                                    unsigned char **ppucData);
extern unsigned long FileSDRAMImageSizeGet(void);
extern tBoolean FileIsSDRAMImagePresent(void);

#endif // __FILE_H__
