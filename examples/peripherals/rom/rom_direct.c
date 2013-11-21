//*****************************************************************************
//
// rom_direct.c - Example demonstrating direct ROM calls.
//
// Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
// 
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the  
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

//
// The following macro is needed so that the ROM calls get correctly
// mapped at compiler time to the part in use.  There are several valid
// choices for the TARGET_IS_ macro.  Consult the Stellaris Peripheral
// Driver Library User's Guide chapter on ROM for a list of the valid choices.
// This macro can also be defined on the compiler command line, instead of
// defined in the source file.
// TODO: define the correct TARGET_IS_ macro for your device.
//
#define TARGET_IS_DUSTDEVIL_RA0

//
// The order of the following header files is important.  The header rom.h
// defines the available ROM functions based on the above macro.
//
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"

//*****************************************************************************
//
//! \addtogroup rom_examples_list
//! <h1>Direct ROM Function Calls (rom_direct)</h1>
//!
//! This example shows how to directly call a ROM based driver library
//! function using the \b ROM_ prefix on the driver library function name.
//! When you call a ROM function in this way, it will only work on a part
//! with ROM, and you will have to change it to work with a non-ROM part.
//
//*****************************************************************************

//*****************************************************************************
//
// Set up the system clock using the ROM based function.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run directly from the PLL at 20 MHz.
    // The ROM_ prefix means that this will be coded as a direct call
    // to the function in ROM.  If your part does not have this function
    // in ROM then this will result in an error.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Return with no errors.
    //
    return(0);
}

