//*****************************************************************************
//
// rom_mapped.c - Example demonstrating mapped ROM calls.
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

//
// If the part has ROM, then the following macro is needed so that the ROM
// calls get correctly mapped at compiler time to the part in use.  There
// are several valid choices for the TARGET_IS_ macro.  Consult the Stellaris
// Peripheral Driver Library User's Guide chapter on ROM for a list of the
// valid choices.  This macro can also be defined on the compiler command line
// instead of defined in the source file.
// TODO: define the correct TARGET_IS_ macro for your device.
//
#define TARGET_IS_DUSTDEVIL_RA0

//
// The order of the following header files is important.  The header rom.h
// defines the available ROM functions based on the above macro.  The header
// rom_map.h then defines the MAP_ prefixed function calls as either ROM or
// library calls based on which ROM functions are available.
//
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"

//*****************************************************************************
//
//! \addtogroup rom_examples_list
//! <h1>Mapped ROM Function Calls (rom_mapped)</h1>
//!
//! This example shows how to map ROM function calls at compile time to use
//! a ROM function if available on the part, or a library call if the function
//! is not available in ROM.  This allows you to write code that can be used
//! on either a part with ROM or without ROM without needing to change the
//! code.  The mapping is performed at compile time and there is no performance
//! penalty for using the mapped method instead of the direct method.  Mapped
//! ROM functions are called with a \b MAP_ prefix on the driver library
//! function name.
//
//*****************************************************************************

//*****************************************************************************
//
// Set up the system clock using the mapped ROM function.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run directly from the PLL at 20 MHz.
    // The MAP_ prefix means that this will be coded as a ROM call if the
    // part supports this function in ROM.  Otherwise it will be coded by
    // the compiler as a library call.
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Return with no errors.
    //
    return(0);
}

