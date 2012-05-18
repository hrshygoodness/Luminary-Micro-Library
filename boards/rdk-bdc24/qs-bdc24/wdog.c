//*****************************************************************************
//
// wdog.c - Functions to handle the watchdog.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/watchdog.h"
#include "shared/can_proto.h"
#include "constants.h"
#include "controller.h"
#include "wdog.h"

//*****************************************************************************
//
// This function is called when the watchdog timer expires, which indicates
// that the input signal has been lost.
//
//*****************************************************************************
void
WatchdogIntHandler(void)
{
    //
    // Clear the watchdog interrupt.
    //
    ROM_WatchdogIntClear(WATCHDOG0_BASE);

    //
    // Indicate that there is no longer an input signal.
    //
    ControllerLinkLost(LINK_TYPE_NONE);
}

//*****************************************************************************
//
// This function prepares the watchdog timer to detect the lost of the input
// link.
//
//*****************************************************************************
void
WatchdogInit(void)
{
    //
    // Configure the watchdog timer to interrupt if it is not pet frequently
    // enough.
    //
    ROM_WatchdogReloadSet(WATCHDOG0_BASE, WATCHDOG_PERIOD);
    ROM_WatchdogStallEnable(WATCHDOG0_BASE);
    ROM_WatchdogEnable(WATCHDOG0_BASE);
    ROM_IntEnable(INT_WATCHDOG);
}
