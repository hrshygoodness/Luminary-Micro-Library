//*****************************************************************************
//
// file.h - Public function prototypes and globals related to file handling
//!         in the qs-checkout application.
//
// Copyright (c) 2009-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************
#ifndef _FILE_H_
#define _FILE_H_

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
extern unsigned long g_ulMSCInstance;

//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************
extern void FileTickHandler(void);
extern tBoolean FileInit(void);
extern tBoolean FileCatToUART(char *pcFilename);
extern tBoolean FileLsToUART(char *pcDir);
extern tBoolean FileIsDrivePresent(unsigned char ucDriveNum);
extern tBoolean FileMountUSB(tBoolean bMount);
extern unsigned long FileCountJPEGFiles(void);
extern tBoolean FileGetJPEGFileInfo(unsigned long ulIndex, char **ppcFilename,
                                    unsigned long *pulLen,
                                    unsigned char **ppucData);
extern unsigned long FileExternalImageSizeGet(void);
extern tBoolean FileIsExternalImagePresent(void);
int Cmd_ls(int argc, char *argv[]);
int Cmd_pwd(int argc, char *argv[]);
int Cmd_cat(int argc, char *argv[]);
int Cmd_cd(int argc, char *argv[]);

#endif
