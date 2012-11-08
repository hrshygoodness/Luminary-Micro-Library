//*****************************************************************************
//
// luminary_guids.h - GUIDs associated with Luminary Micro USB examples
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
#ifndef _LUMINARY_GUIDS_
#define _LUMINARY_GUIDS_

//
// Vendor and Product IDs for the generic bulk device.
//
#define BULK_VID 0x1cbe
#define BULK_PID 0x0003

//
// Stellaris Bulk Device Class GUID
// {F5450C06-EB58-420e-8F98-A76C5D4AFB18}
//
DEFINE_GUID(GUID_DEVCLASS_STELLARIS_BULK,
0xF5450C06, 0xEB5B, 0x420E, 0x8F, 0x98, 0xA7, 0x6C, 0x5D, 0x4A, 0xFB, 0x18);

//
// Stellaris Bulk Device Interface GUID
// {6E45736A-2B1B-4078-B772-B3AF2B6FDE1C}
//
DEFINE_GUID(GUID_DEVINTERFACE_STELLARIS_BULK,
0x6E45736A, 0x2B1B, 0x4078, 0xB7, 0x72, 0xb3, 0xAF, 0x2B, 0x6F, 0xDE, 0x1C);

#endif