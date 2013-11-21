//
// lmusbwrap.h : Header file for the lmusbdll wrapper layer
//

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

typedef void *LMUSB_HANDLE;

BOOL __stdcall LoadLMUSBLibrary(BOOL *pbDriverInstalled);
LMUSB_HANDLE __stdcall _InitializeDevice(unsigned short usVID,
                                         unsigned short usPID,
                                         LPGUID lpGUID,
                                         BOOL *pbDriverInstalled);
BOOL __stdcall _TerminateDevice(LMUSB_HANDLE hHandle);
BOOL __stdcall _WriteUSBPacket(LMUSB_HANDLE hHandle,
                               unsigned char *pcBuffer,
                               unsigned long ulSize,
                               unsigned long *pulWritten);
DWORD __stdcall _ReadUSBPacket(LMUSB_HANDLE hHandle,
                              unsigned char *pcBuffer,
                              unsigned long ulSize,
                              unsigned long *pulRead,
                              unsigned long ulTimeoutMs,
                              HANDLE hBreak);

#ifdef __cplusplus
}
#endif

