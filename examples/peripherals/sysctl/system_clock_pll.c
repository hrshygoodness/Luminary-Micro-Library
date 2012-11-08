//*****************************************************************************
//
// system_clock_pll.c - Example demonstrating setting the clock using the PLL.
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

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"

//*****************************************************************************
//
//! \addtogroup sysctl_examples_list
//! <h1>System Clock Configuration with PLL (system_clock_pll)</h1>
//!
//! This example shows how to set up the system clock to use the PLL.
//
//*****************************************************************************

//*****************************************************************************
//
// Set up the system clock using the PLL
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run directly from the PLL at 20 MHz.
    // The following code:
    // -sets the system clock divider to 10 (200 MHz PLL divide by 10 = 20 MHz)
    // -sets the system clock to use the PLL
    // -uses the main oscillator
    // -configures for use of 16 MHz crystal/oscillator input
    //
    SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Return with no errors.
    //
    return(0);
}

