//*****************************************************************************
//
// lmusbdll.h : Main header file for the Stellaris USB interface DLL.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifdef __cplusplus

#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

//
// Functions exported by this DLL.
//
extern "C" {
#endif

//****************************************************************************
//
// A handle returned by InitializeDevice().
//
//****************************************************************************
typedef void *LMUSB_HANDLE;

//****************************************************************************
//
// Flags used in constructing the ucRequestType parameter to Endpoint0Transfer().
//
//****************************************************************************
#define REQUEST_TRANSFER_IN             0x80
#define REQUEST_TRANSFER_OUT            0x00

#define REQUEST_TYPE_STANDARD           0x00
#define REQUEST_TYPE_CLASS              0x20
#define REQUEST_TYPE_VENDOR             0x40

#define REQUEST_RECIPIENT_DEVICE        0x00
#define REQUEST_RECIPIENT_INTERFACE     0x01
#define REQUEST_RECIPIENT_ENDPOINT      0x02
#define REQUEST_RECIPIENT_OTHER         0x03

//****************************************************************************
//
// Prototypes for functions exported by the DLL.
//
//****************************************************************************
LMUSB_HANDLE __stdcall InitializeDevice(unsigned short usVID,
                                        unsigned short usPID,
                                        LPGUID lpGUID,
                                        BOOL *pbDriverInstalled);
LMUSB_HANDLE __stdcall InitializeDeviceByIndex(unsigned short usVID,
                                               unsigned short usPID,
                                               LPGUID lpGUID,
                                               DWORD dwIndex,
                                               BOOL bOpenDataEndpoints,
                                               BOOL *pbDriverInstalled);
BOOL __stdcall TerminateDevice(LMUSB_HANDLE hHandle);
BOOL __stdcall WriteUSBPacket(LMUSB_HANDLE hHandle,
                              unsigned char *pcBuffer,
                              unsigned long ulSize,
                              unsigned long *pulWritten);
DWORD __stdcall ReadUSBPacket(LMUSB_HANDLE hHandle,
                             unsigned char *pcBuffer,
                             unsigned long ulSize,
                             unsigned long *pulRead,
                             unsigned long ulTimeoutMs,
                             HANDLE hBreak);
BOOL __stdcall Endpoint0Transfer(LMUSB_HANDLE hHandle, UCHAR ucRequestType,
                                 UCHAR ucRequest, USHORT usValue,
                                 USHORT usIndex, USHORT usLength,
                                 PUCHAR pucBuffer, PUSHORT pusCount);

//****************************************************************************
//
// Typedefs for each of the exported functions.  This helps if applications
// want to link the DLL dynamically using LoadLibrary rather than linking
// directly to the lib file.
//
//****************************************************************************
typedef LMUSB_HANDLE (__stdcall *tInitializeDevice)(unsigned short usVID,
                                                    unsigned short usPID,
                                                    LPGUID lpGUID,
                                                    BOOL *pbDriverInstalled);
typedef BOOL (__stdcall *tTerminateDevice)(LMUSB_HANDLE hHandle);
typedef BOOL (__stdcall *tWriteUSBPacket)(LMUSB_HANDLE hHandle,
                                          unsigned char *pcBuffer,
                                          unsigned long ulSize,
                                          unsigned long *pulWritten);
typedef DWORD (__stdcall *tReadUSBPacket)(LMUSB_HANDLE hHandle,
                                          unsigned char *pcBuffer,
                                          unsigned long ulSize,
                                          unsigned long *pulRead,
                                          unsigned long ulTimeoutMs,
                                          HANDLE hBreak);
typedef DWORD (__stdcall *tEndpoint0Transfer)(LMUSB_HANDLE hHandle,
                                              UCHAR ucRequestType,
                                              UCHAR ucRequest, USHORT usValue,
                                              USHORT usIndex, USHORT usLength,
                                              PUCHAR pucBuffer,
                                              PUSHORT pusCount);
#ifdef __cplusplus
}
#endif

