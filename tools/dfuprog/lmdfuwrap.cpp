//*****************************************************************************
//
// lmdfuwrap.cpp : A thin wrapper over the lmdfu.dll library allowing it to be
//                 loaded dynamically rather than statically linking to its .lib
//                 file.
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

#include <windows.h>
#include "lmdfu.h"
#include "lmdfuwrap.h"

//*****************************************************************************
//
// DLL module handle.
//
//*****************************************************************************
HINSTANCE hLMUSB = (HINSTANCE)NULL;

//*****************************************************************************
//
// Function pointers for each DLL entry point.
//
//*****************************************************************************
tLMDFUInit pfnLMDFUInit = NULL;
tLMDFUDeviceOpen pfnLMDFUDeviceOpen = NULL;
tLMDFUDeviceClose pfnLMDFUDeviceClose = NULL;
tLMDFUDeviceStringGet pfnLMDFUDeviceStringGet = NULL;
tLMDFUDeviceASCIIStringGet pfnLMDFUDeviceASCIIStringGet = NULL;
tLMDFUParamsGet pfnLMDFUParamsGet = NULL;
tLMDFUIsValidImage pfnLMDFUIsValidImage = NULL;
tLMDFUDownload pfnLMDFUDownload = NULL;
tLMDFUDownloadBin pfnLMDFUDownloadBin = NULL;
tLMDFUErase pfnLMDFUErase = NULL;
tLMDFUBlankCheck pfnLMDFUBlankCheck = NULL;
tLMDFUUpload pfnLMDFUUpload = NULL;
tLMDFUStatusGet pfnLMDFUStatusGet = NULL;
tLMDFUErrorStringGet pfnLMDFUErrorStringGet = NULL;
tLMDFUModeSwitch pfnLMDFUModeSwitch = NULL;

tLMDFUErr __stdcall _LMDFUInit(void)
{
    //
    // Try to load the USB DLL.
    //
    hLMUSB = LoadLibrary(TEXT("lmdfu.dll"));

    //
    // Did we find it?
    //
    if(hLMUSB)
    {
        //
        // Yes - query all the entry point addresses.
        //
        pfnLMDFUInit = (tLMDFUInit)GetProcAddress(hLMUSB, "LMDFUInit");
        pfnLMDFUDeviceOpen = (tLMDFUDeviceOpen)GetProcAddress(hLMUSB, "LMDFUDeviceOpen");
        pfnLMDFUDeviceClose = (tLMDFUDeviceClose)GetProcAddress(hLMUSB, "LMDFUDeviceClose");
        pfnLMDFUDeviceStringGet = (tLMDFUDeviceStringGet)GetProcAddress(hLMUSB, "LMDFUDeviceStringGet");
        pfnLMDFUDeviceASCIIStringGet = (tLMDFUDeviceASCIIStringGet)GetProcAddress(hLMUSB, "LMDFUDeviceASCIIStringGet");
        pfnLMDFUParamsGet = (tLMDFUParamsGet)GetProcAddress(hLMUSB, "LMDFUParamsGet");
        pfnLMDFUIsValidImage = (tLMDFUIsValidImage)GetProcAddress(hLMUSB, "LMDFUIsValidImage");
        pfnLMDFUDownload = (tLMDFUDownload)GetProcAddress(hLMUSB, "LMDFUDownload");
        pfnLMDFUDownloadBin = (tLMDFUDownloadBin)GetProcAddress(hLMUSB, "LMDFUDownloadBin");
        pfnLMDFUErase = (tLMDFUErase)GetProcAddress(hLMUSB, "LMDFUErase");
        pfnLMDFUBlankCheck = (tLMDFUBlankCheck)GetProcAddress(hLMUSB, "LMDFUBlankCheck");
        pfnLMDFUUpload = (tLMDFUUpload)GetProcAddress(hLMUSB, "LMDFUUpload");
        pfnLMDFUStatusGet = (tLMDFUStatusGet)GetProcAddress(hLMUSB, "LMDFUStatusGet");
        pfnLMDFUErrorStringGet = (tLMDFUErrorStringGet)GetProcAddress(hLMUSB, "LMDFUErrorStringGet");
        pfnLMDFUModeSwitch = (tLMDFUModeSwitch)GetProcAddress(hLMUSB, "LMDFUModeSwitch");

        //
        // Make sure we actually queried all the expected entry points.
        //
        if(!pfnLMDFUInit || !pfnLMDFUDeviceOpen ||  !pfnLMDFUDeviceClose ||
           !pfnLMDFUDeviceStringGet || !pfnLMDFUDeviceASCIIStringGet ||
           !pfnLMDFUParamsGet || !pfnLMDFUIsValidImage || !pfnLMDFUDownload ||
           !pfnLMDFUDownloadBin || !pfnLMDFUErase || !pfnLMDFUBlankCheck ||
           !pfnLMDFUUpload || !pfnLMDFUStatusGet || !pfnLMDFUErrorStringGet ||
           !pfnLMDFUModeSwitch)
        {
            //
            // We failed to query at least one entry point.  Return
            // DFU_ERR_INVALID_ADDR to signal to the caller that the driver
            // is likely the wrong version.
            //
            return(DFU_ERR_INVALID_ADDR);
        }
        else
        {
            //
            // We got all the expected function pointers so call the library
            // init function and return it's response.
            //
            return(pfnLMDFUInit());
        }
    }
    else
    {
        //
        // The DLL could not be found.  This tends to suggest that the driver
        // is not installed.  Return an appropriate error code to the caller
        // so that they can provide helpful information to the user.
        //
        return(DFU_ERR_NOT_FOUND);
    }
}

tLMDFUErr __stdcall _LMDFUDeviceOpen(int iDeviceIndex,
                                    tLMDFUDeviceInfo *psDevInfo,
                                    tLMDFUHandle *phHandle)
{
    if(pfnLMDFUDeviceOpen)
    {
        return(pfnLMDFUDeviceOpen(iDeviceIndex, psDevInfo, phHandle));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUDeviceClose(tLMDFUHandle hHandle, bool bReset)
{
    if(pfnLMDFUDeviceClose)
    {
        return(pfnLMDFUDeviceClose(hHandle, bReset));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUDeviceStringGet(tLMDFUHandle hHandle,
                                         unsigned char ucStringIndex,
                                         unsigned short usLanguageID,
                                         char *pcString,
                                         unsigned short *pusStringLen)
{
    if(pfnLMDFUDeviceStringGet)
    {
        return(pfnLMDFUDeviceStringGet(hHandle, ucStringIndex, usLanguageID,
                                       pcString, pusStringLen));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUDeviceASCIIStringGet(tLMDFUHandle hHandle,
                                              unsigned char ucStringIndex,
                                              char *pcString,
                                              unsigned short *pusStringLen)
{
    if(pfnLMDFUDeviceASCIIStringGet)
    {
        return(pfnLMDFUDeviceASCIIStringGet(hHandle, ucStringIndex, pcString,
                                            pusStringLen));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUParamsGet(tLMDFUHandle hHandle,
                                   tLMDFUParams *psParams)
{
    if(pfnLMDFUParamsGet)
    {
        return(pfnLMDFUParamsGet(hHandle, psParams));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUIsValidImage(tLMDFUHandle hHandle,
                                      unsigned char *pcDFUImage,
                                      unsigned long ulImageLen,
                                      bool *pbStellarisFormat)
{
    if(pfnLMDFUIsValidImage)
    {
        return(pfnLMDFUIsValidImage(hHandle, pcDFUImage, ulImageLen,
                                    pbStellarisFormat));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUDownload(tLMDFUHandle hHandle,
                                  unsigned char *pcDFUImage,
                                  unsigned long ulImageLen, bool bVerify,
                                  bool bIgnoreIDs, HWND hwndNotify)
{
    if(pfnLMDFUDownload)
    {
        return(pfnLMDFUDownload(hHandle, pcDFUImage, ulImageLen, bVerify,
                                bIgnoreIDs, hwndNotify));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUDownloadBin(tLMDFUHandle hHandle,
                                     unsigned char *pcBinaryImage,
                                     unsigned long ulImageLen,
                                     unsigned long ulStartAddr,
                                     bool bVerify, HWND hwndNotify)
{
    if(pfnLMDFUDownloadBin)
    {
        return(pfnLMDFUDownloadBin(hHandle, pcBinaryImage, ulImageLen,
                                   ulStartAddr, bVerify, hwndNotify));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUErase(tLMDFUHandle hHandle, unsigned long ulStartAddr,
                               unsigned long ulEraseLen, bool bVerify,
                               HWND hwndNotify)
{
    if(pfnLMDFUErase)
    {
        return(pfnLMDFUErase(hHandle, ulStartAddr, ulEraseLen, bVerify,
                             hwndNotify));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUBlankCheck(tLMDFUHandle hHandle,
                                    unsigned long ulStartAddr,
                                    unsigned long ulLen)
{
    if(pfnLMDFUBlankCheck)
    {
        return(pfnLMDFUBlankCheck(hHandle, ulStartAddr, ulLen));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUUpload(tLMDFUHandle hHandle, unsigned char *pcBuffer,
                                unsigned long ulStartAddr,
                                unsigned long ulImageLen, bool bRaw,
                                HWND hwndNotify)
{
    if(pfnLMDFUUpload)
    {
        return(pfnLMDFUUpload(hHandle, pcBuffer, ulStartAddr, ulImageLen,
                              bRaw, hwndNotify));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUStatusGet(tLMDFUHandle hHandle, tDFUStatus *pStatus)
{
    if(pfnLMDFUStatusGet)
    {
        return(pfnLMDFUStatusGet(hHandle, pStatus));
    }

    return(DFU_ERR_HANDLE);
}

tLMDFUErr __stdcall _LMDFUModeSwitch(tLMDFUHandle hHandle)
{
    if(pfnLMDFUStatusGet)
    {
        return(pfnLMDFUModeSwitch(hHandle));
    }

    return(DFU_ERR_HANDLE);
}

char * __stdcall _LMDFUErrorStringGet(tLMDFUErr eError)
{
    if(pfnLMDFUErrorStringGet)
    {
        return(pfnLMDFUErrorStringGet(eError));
    }

    return("Driver not installed");
}
