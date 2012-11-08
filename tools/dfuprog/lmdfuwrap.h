//*****************************************************************************
//
// lmdfuwrap.h : A thin wrapper over the lmdfu.dll library allowing it to be
//               loaded dynamically rather than statically linking to its .lib
//               file.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifndef __LMDFUWRAP_H__
#define __LMDFUWRAP_H__

//*****************************************************************************
//
// Wrapper function prototypes.
//
//*****************************************************************************
tLMDFUErr __stdcall _LMDFUInit(void);
tLMDFUErr __stdcall _LMDFUDeviceOpen(int iDeviceIndex,
                                    tLMDFUDeviceInfo *psDevInfo,
                                    tLMDFUHandle *phHandle);
tLMDFUErr __stdcall _LMDFUDeviceClose(tLMDFUHandle hHandle, bool bReset);
tLMDFUErr __stdcall _LMDFUDeviceStringGet(tLMDFUHandle hHandle,
                                         unsigned char ucStringIndex,
                                         unsigned short usLanguageID,
                                         char *pcString,
                                         unsigned short *pusStringLen);
tLMDFUErr __stdcall _LMDFUDeviceASCIIStringGet(tLMDFUHandle hHandle,
                                              unsigned char ucStringIndex,
                                              char *pcString,
                                              unsigned short *pusStringLen);
tLMDFUErr __stdcall _LMDFUParamsGet(tLMDFUHandle hHandle,
                                   tLMDFUParams *psParams);
tLMDFUErr __stdcall _LMDFUIsValidImage(tLMDFUHandle hHandle,
                                      unsigned char *pcDFUImage,
                                      unsigned long ulImageLen,
                                      bool *pbStellarisFormat);
tLMDFUErr __stdcall _LMDFUDownload(tLMDFUHandle hHandle,
                                  unsigned char *pcDFUImage,
                                  unsigned long ulImageLen, bool bVerify,
                                  bool bIgnoreIDs, HWND hwndNotify);
tLMDFUErr __stdcall _LMDFUDownloadBin(tLMDFUHandle hHandle,
                                     unsigned char *pcBinaryImage,
                                     unsigned long ulImageLen,
                                     unsigned long ulStartAddr,
                                     bool bVerify, HWND hwndNotify);
tLMDFUErr __stdcall _LMDFUErase(tLMDFUHandle hHandle, unsigned long ulStartAddr,
                               unsigned long ulEraseLen, bool bVerify,
                               HWND hwndNotify);
tLMDFUErr __stdcall _LMDFUBlankCheck(tLMDFUHandle hHandle,
                                    unsigned long ulStartAddr,
                                    unsigned long ulLen);
tLMDFUErr __stdcall _LMDFUUpload(tLMDFUHandle hHandle, unsigned char *pcBuffer,
                                unsigned long ulStartAddr,
                                unsigned long ulImageLen, bool bRaw,
                                HWND hwndNotify);
tLMDFUErr __stdcall _LMDFUStatusGet(tLMDFUHandle hHandle, tDFUStatus *pStatus);
tLMDFUErr __stdcall _LMDFUModeSwitch(tLMDFUHandle hHandle);
char * __stdcall _LMDFUErrorStringGet(tLMDFUErr eError);

#endif
