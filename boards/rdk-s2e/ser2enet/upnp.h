//*****************************************************************************
//
// upnp.h - Definitions for the UPnP interface.
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
// This is part of revision 8555 of the RDK-S2E Firmware Package.
//
//*****************************************************************************

#ifndef __UPNP_H__
#define __UPNP_H__

//*****************************************************************************
//
// The amount of time between sending out UPnP Advertisements.
//
//*****************************************************************************
#define UPNP_ADVERTISEMENT_INTERVAL     (10 * 1000)

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void UPnPInit(void);
extern void UPnPStart(void);
extern void UPnPStop(void);
extern void UPnPHandler(unsigned long ulTimeMS);

#endif // __UPnP_H__
