//*****************************************************************************
//
// scope_guids.h - GUIDs associated with Luminary Micro Oscilloscope
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
#ifndef _SCOPE_GUIDS_
#define _SCOPE_GUIDS_

// Vendor and Product IDs for the oscilloscope device
#define SCOPE_VID 0x1cbe
#define SCOPE_PID 0x0004

// Luminary Micro Oscilloscope Class GUID
DEFINE_GUID(GUID_DEVCLASS_LUMINARY_SCOPE,
0xa4ee4387, 0x1e4d, 0x4d95, 0xa2, 0xd4, 0xf0, 0xab, 0xd2, 0xb9, 0xda, 0xbd);

// Luminary Micro Oscilloscope Interface GUID
DEFINE_GUID(GUID_DEVINTERFACE_LUMINARY_SCOPE,
0x817ed455, 0xbe9f, 0x45a6, 0xa7, 0x78, 0x1c, 0x15, 0xb6, 0x6, 0x56, 0x5b);

#endif
