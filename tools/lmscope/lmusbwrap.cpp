//*****************************************************************************
//
// lmusbwrap.cpp : A thin wrapper over the lmusbdll.dll library allowing it to
//                 be loaded dynamically rather than statically linking to
//                 its .lib file.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include "stdafx.h"
#include <windows.h>
#include "lmusbdll.h"
#include "lmusbwrap.h"

//*****************************************************************************
//
// DLL module handle.
//
//*****************************************************************************
HINSTANCE hLMUSBDLL = (HINSTANCE)NULL;

//*****************************************************************************
//
// Function pointers for each DLL entry point.
//
//*****************************************************************************

tInitializeDevice pfnInitializeDevice = NULL;
tTerminateDevice pfnTerminateDevice = NULL;
tWriteUSBPacket pfnWriteUSBPacket = NULL;
tReadUSBPacket pfnReadUSBPacket = NULL;

//***************************************************************************
//
// Load the device driver DLL (lmusbdll.dll), query the entry points used
// by the application and tell the caller whether or not we were successful.
//
// The return code will be TRUE on success or FALSE on failure.  In failure
// cases, the value of *pbDriverInstalled will differentiate between the
// cases of the driver not being installed at all (FALSE) or installed but
// the incorrect version (TRUE).
//
//***************************************************************************
BOOL __stdcall LoadLMUSBLibrary(BOOL *pbDriverInstalled)
{
    //
    // Try to load the USB DLL.
    //
    hLMUSBDLL = LoadLibrary(TEXT("lmusbdll.dll"));

    //
    // Did we find it?
    //
    if(hLMUSBDLL)
    {
        //
        // Yes - query all the entry point addresses.
        //
        pfnInitializeDevice = (tInitializeDevice)GetProcAddress(hLMUSBDLL,
                                                 "InitializeDevice");
        pfnTerminateDevice = (tTerminateDevice)GetProcAddress(hLMUSBDLL,
                                                 "TerminateDevice");
        pfnWriteUSBPacket = (tWriteUSBPacket)GetProcAddress(hLMUSBDLL,
                                                 "WriteUSBPacket");
        pfnReadUSBPacket = (tReadUSBPacket)GetProcAddress(hLMUSBDLL,
                                                 "ReadUSBPacket");

        //
        // Make sure we actually queried all the expected entry points.
        //
        if(!pfnInitializeDevice || !pfnTerminateDevice ||
           !pfnWriteUSBPacket || !pfnReadUSBPacket)
        {
            //
            // We failed to query at least one entry point but the driver must
            // be installed since we loaded the DLL itself.  This suggests that
            // the driver is likely the wrong version.  Return NULL to signal
            // to the caller that we can't continue.
            //
            *pbDriverInstalled = TRUE;
            return(FALSE);
        }
        else
        {
            //
            // We got all the expected function pointers so tell the caller
            // that all is well.
            //
            *pbDriverInstalled = TRUE;
            return(TRUE);
        }
    }
    else
    {
        //
        // The DLL could not be found.  This tends to suggest that the driver
        // is not installed.  Clear the "driver installed flag and return NULL
        // to tell the caller something went badly wrong.
        //
        *pbDriverInstalled = FALSE;
        return(FALSE);
    }

}

//***************************************************************************
//
// Initialize the oscilloscope device if it can be found.
//
//***************************************************************************
LMUSB_HANDLE __stdcall _InitializeDevice(unsigned short usVID,
                                         unsigned short usPID,
                                         LPGUID lpGUID,
                                         BOOL *pbDriverInstalled)
{
    //
    // Make sure we actually queried all the expected entry points.
    //
    if(pfnInitializeDevice)
    {
        //
        // We got all the expected function pointers so call the library
        // init function and return it's response.
        //
        return(pfnInitializeDevice(usVID, usPID, lpGUID, pbDriverInstalled));
    }

    //
    // If we get here, either LoadLMUSBLibrary has not been called or it
    // failed to load the required DLL.
    //
    *pbDriverInstalled = FALSE;
    return((LMUSB_HANDLE)NULL);
}

//***************************************************************************
//
// End use of the USB device.
//
//***************************************************************************
BOOL __stdcall _TerminateDevice(LMUSB_HANDLE hHandle)
{
    if(pfnTerminateDevice)
    {
        return(pfnTerminateDevice(hHandle));
    }

    //
    // The DLL isn't loaded so return FALSE to tell the caller we
    // had a problem.
    //
    return(FALSE);
}

//***************************************************************************
//
// Write a block of data to the USB device.
//
//***************************************************************************
BOOL __stdcall _WriteUSBPacket(LMUSB_HANDLE hHandle,
                               unsigned char *pcBuffer,
                               unsigned long ulSize,
                               unsigned long *pulWritten)
{
    if(pfnWriteUSBPacket)
    {
        return(pfnWriteUSBPacket(hHandle, pcBuffer, ulSize, pulWritten));
    }

    //
    // The DLL isn't loaded so return FALSE to tell the caller we
    // had a problem.
    //
    return(FALSE);
}

//***************************************************************************
//
// Write a block of data from the USB device.
//
//***************************************************************************
DWORD __stdcall _ReadUSBPacket(LMUSB_HANDLE hHandle,
                               unsigned char *pcBuffer,
                               unsigned long ulSize,
                               unsigned long *pulRead,
                               unsigned long ulTimeoutMs,
                               HANDLE hBreak)
{
    if(pfnReadUSBPacket)
    {
        return(pfnReadUSBPacket(hHandle, pcBuffer, ulSize, pulRead,
                                ulTimeoutMs, hBreak));
    }

    //
    // The DLL didn't load so tell the caller that the device doesn't exist.
    //
    *pulRead = 0;
    return(ERROR_DEV_NOT_EXIST);
}
