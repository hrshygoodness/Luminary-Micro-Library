//*****************************************************************************
//
// mpu_fault.c - MPU example.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM3S811 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_nvic.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/mpu.h"
#include "drivers/display96x16x1.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>MPU (mpu_fault)</h1>
//!
//! This example application demonstrates the use of the MPU to protect a
//! region of memory from access, and to generate a memory management fault
//! when there is an access violation.
//
//*****************************************************************************

//*****************************************************************************
//
// Variables to hold the state of the fault status when the fault occurs and
// the faulting address.
//
//*****************************************************************************
static volatile unsigned long g_ulMMAR;
static volatile unsigned long g_ulFaultStatus;

//*****************************************************************************
//
// A counter to track the number of times the fault handler has been entered.
//
//*****************************************************************************
static volatile unsigned long g_ulMPUFaultCount;

//*****************************************************************************
//
// A location for storing data read from various addresses.  Volatile forces
// the compiler to use it and not optimize the access away.
//
//*****************************************************************************
static volatile unsigned long g_ulValue;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// The exception handler for memory management faults, which are caused by MPU
// access violations.  This handler will verify the cause of the fault and
// clear the NVIC fault status register.
//
//*****************************************************************************
void
MPUFaultHandler(void)
{
    //
    // Preserve the value of the MMAR (the address causing the fault).
    // Preserve the fault status register value, then clear it.
    //
    g_ulMMAR = HWREG(NVIC_MM_ADDR);
    g_ulFaultStatus = HWREG(NVIC_FAULT_STAT);
    HWREG(NVIC_FAULT_STAT) = g_ulFaultStatus;

    //
    // Increment a counter to indicate the fault occurred.
    //
    g_ulMPUFaultCount++;

    //
    // Disable the MPU so that this handler can return and cause no more
    // faults.  The actual instruction that faulted will be re-executed.
    //
    MPUDisable();
}

//*****************************************************************************
//
// This example demonstrates how to configure MPU regions for different levels
// of memory protection.  The following memory map is set up:
//
// 0000.0000 - 0000.1C00 - rgn 0: executable read-only, flash
// 0000.1C00 - 0000.2000 - rgn 0: no access, flash (disabled sub-region 7)
// 2000.0000 - 2000.1000 - rgn 1: read-write, RAM
// 2000.1000 - 2000.1400 - rgn 2: read-only, RAM (disabled sub-rgn 4 of rgn 1)
// 2000.1400 - 2000.2000 - rgn 1: read-write, RAM
// 4000.0000 - 4001.0000 - rgn 3: read-write, peripherals
// 4001.0000 - 4002.0000 - rgn 3: no access (disabled sub-region 1)
// 4002.0000 - 4006.0000 - rgn 3: read-write, peripherals
// 4006.0000 - 4008.0000 - rgn 3: no access (disabled sub-region 6, 7)
// E000.E000 - E000.F000 - rgn 4: read-write, NVIC
//
// The example code will attempt to perform the following operations and check
// the faulting behavior:
//
// - write to flash                         (should fault)
// - read from the disabled area of flash   (should fault)
// - read from the read-only area of RAM    (should not fault)
// - write to the read-only section of RAM  (should fault)
//
//*****************************************************************************
int
main(void)
{
    unsigned int bFail = 0;

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Initialize the OLED display and write status.
    //
    Display96x16x1Init(false);
    Display96x16x1StringDraw("MPU example", 12, 0);

    //
    // Configure an executable, read-only MPU region for flash.  It is an 8 KB
    // region with the last 1 KB disabled to result in a 7 KB executable
    // region.  This region is needed so that the program can execute from
    // flash.
    //
    MPURegionSet(0, FLASH_BASE,
                 MPU_RGN_SIZE_8K |
                 MPU_RGN_PERM_EXEC |
                 MPU_RGN_PERM_PRV_RO_USR_RO |
                 MPU_SUB_RGN_DISABLE_7 |
                 MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for RAM.  It is an 8 KB region.  There
    // is a 1 KB sub-region in the middle that is disabled in order to open up
    // a hole in which different permissions can be applied.
    //
    MPURegionSet(1, SRAM_BASE,
                 MPU_RGN_SIZE_8K |
                 MPU_RGN_PERM_NOEXEC |
                 MPU_RGN_PERM_PRV_RW_USR_RW |
                 MPU_SUB_RGN_DISABLE_4 |
                 MPU_RGN_ENABLE);

    //
    // Configure a read-only MPU region for the 1 KB of RAM that is disabled in
    // the previous region.  This region is used for demonstrating read-only
    // permissions.
    //
    MPURegionSet(2, SRAM_BASE + 0x1000,
                 MPU_RGN_SIZE_1K |
                 MPU_RGN_PERM_NOEXEC |
                 MPU_RGN_PERM_PRV_RO_USR_RO |
                 MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for peripherals.  The region is 512 KB
    // total size, with several sub-regions disabled to prevent access to areas
    // where there are no peripherals.  This region is needed because the
    // program needs access to some peripherals.
    //
    MPURegionSet(3, 0x40000000,
                 MPU_RGN_SIZE_512K |
                 MPU_RGN_PERM_NOEXEC |
                 MPU_RGN_PERM_PRV_RW_USR_RW |
                 MPU_SUB_RGN_DISABLE_1 |
                 MPU_SUB_RGN_DISABLE_6 |
                 MPU_SUB_RGN_DISABLE_7 |
                 MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for access to the NVIC.  The region is
    // 4 KB in size.  This region is needed because NVIC registers are needed
    // in order to control the MPU.
    //
    MPURegionSet(4, NVIC_BASE,
                 MPU_RGN_SIZE_4K |
                 MPU_RGN_PERM_NOEXEC |
                 MPU_RGN_PERM_PRV_RW_USR_RW |
                 MPU_RGN_ENABLE);

    //
    // Need to clear the NVIC fault status register to make sure there is no
    // status hanging around from a previous program.
    //
    g_ulFaultStatus = HWREG(NVIC_FAULT_STAT);
    HWREG(NVIC_FAULT_STAT) = g_ulFaultStatus;

    //
    // Enable the MPU fault.
    //
    IntEnable(FAULT_MPU);

    //
    // Enable the MPU.  This will begin to enforce the memory protection
    // regions.  The MPU is configured so that when in the hard fault or NMI
    // exceptions, a default map will be used.  Neither of these should occur
    // in this example program.
    //
    MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to write to the flash.  This should cause a protection fault due
    // to the fact that this region is read-only.
    //
    g_ulMPUFaultCount = 0;
    HWREG(0x100) = 0x12345678;

    //
    // Verify that the fault occurred, at the expected address.
    //
    if(!((g_ulMPUFaultCount == 1) &&
         (g_ulFaultStatus == 0x82) &&
         (g_ulMMAR == 0x100)))
    {
        bFail = 1;
    }

    //
    // The MPU was disabled when the previous fault occurred, so it needs to be
    // re-enabled.
    //
    MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to read from the disabled section of flash, the upper 1 KB of
    // the 8 KB region.
    //
    g_ulMPUFaultCount = 0;
    g_ulValue = HWREG(0x1C10);

    //
    // Verify that the fault occurred, at the expected address.
    //
    if(!((g_ulMPUFaultCount == 1) &&
         (g_ulFaultStatus == 0x82) &&
         (g_ulMMAR == 0x1C10)))
    {
        bFail = 1;
    }

    //
    // The MPU was disabled when the previous fault occurred, so it needs to be
    // re-enabled.
    //
    MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to read from the read-only area of RAM, the middle 1 KB of the
    // 8 KB region.
    //
    g_ulMPUFaultCount = 0;
    g_ulValue = HWREG(0x20001040);

    //
    // Verify that the RAM read did not cause a fault.
    //
    if(g_ulMPUFaultCount != 0)
    {
        bFail = 1;
    }

    //
    // The MPU should not have been disabled since the last access was not
    // supposed to cause a fault.  But if it did cause a fault, then the MPU
    // will be disabled, so re-enable it here anyway, just in case.
    //
    MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to write to the read-only area of RAM, the middle 1 KB of the
    // 8 KB region.
    //
    g_ulMPUFaultCount = 0;
    HWREG(0x20001060) = 0xabcdef00;

    //
    // Verify that the RAM write caused a fault.
    //
    if(!((g_ulMPUFaultCount == 1) &&
         (g_ulFaultStatus == 0x82) &&
         (g_ulMMAR == 0x20001060)))
    {
        bFail = 1;
    }

    //
    // Display the results of the example program.
    //
    if(bFail)
    {
        Display96x16x1StringDraw("Failure!", 24, 1);
    }
    else
    {
        Display96x16x1StringDraw("Success!", 24, 1);
    }

    //
    // Disable the MPU, so there are no lingering side effects if another
    // program is run.
    //
    MPUDisable();

    //
    // Finished.
    //
    while(1)
    {
    }
}
