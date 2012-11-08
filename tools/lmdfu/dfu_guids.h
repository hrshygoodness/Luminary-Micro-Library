//*****************************************************************************
//
// dfu_guids.h - GUIDs associated with TI Stellaris DFU device.
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
#ifndef _DFU_GUIDS_
#define _DFU_GUIDS_

//
// Vendor and Product IDs for the Device Firmware Upgrade device.
//
#define DFU_VID 0x1cbe
#define DFU_PID 0x00ff

//
// Stellaris DFU Device Class GUID
// {BDF4A2F9-552C-437b-892D-7CF600016A3F}
//
DEFINE_GUID(GUID_DEVCLASS_STELLARIS_DFU,
0xBDF4A2F9, 0x552C, 0x437b, 0x89, 0x2D, 0x7C, 0xF6, 0x00, 0x01, 0x6A, 0x3F);

//
// Stellaris DFU Runtime Device Class GIUD
// {0D42186B-31A8-4800-B875-1A5525A407B9}
//
DEFINE_GUID(GUID_DEVCLASS_STELLARIS_RUNTIME_DFU,
0xd42186b, 0x31a8, 0x4800, 0xb8, 0x75, 0x1a, 0x55, 0x25, 0xa4, 0x7, 0xb9);

//
// Stellaris DFU Device Interface GUID
// {D17C772B-AF45-4041-9979-AAFE96BF6398}
//
DEFINE_GUID(GUID_DEVINTERFACE_STELLARIS_DFU,
0xd17c772b, 0xaf45, 0x4041, 0x99, 0x79, 0xaa, 0xfe, 0x96, 0xbf, 0x63, 0x98);

#endif
