//*****************************************************************************
//
// mpu_fault.c - MPU example.
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
// This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/mpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/set_pinout.h"

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
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

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
// 2000.0000 - 2000.8000 - rgn 1: read-write, RAM
// 2000.8000 - 2000.A000 - rgn 2: read-only, RAM (disabled sub-rgn 4 of rgn 1)
// 2000.A000 - 2000.FFFF - rgn 1: read-write, RAM
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
    tRectangle sRect;
    unsigned int bFail = 0;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context and find the middle X coordinate.
    //
    GrContextInit(&g_sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 23;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontCm20);
    GrStringDrawCentered(&g_sContext, "mpu-fault", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 11, 0);
    GrContextFontSet(&g_sContext, g_pFontCmss22b);

    //
    // Configure an executable, read-only MPU region for flash.  It is a 16 KB
    // region with the last 2 KB disabled to result in a 14 KB executable
    // region.  This region is needed so that the program can execute from
    // flash.
    //
    ROM_MPURegionSet(0, FLASH_BASE,
                     MPU_RGN_SIZE_16K | MPU_RGN_PERM_EXEC |
                     MPU_RGN_PERM_PRV_RO_USR_RO | MPU_SUB_RGN_DISABLE_7 |
                     MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for RAM.  It is a 64 KB region.  There
    // is a 8 KB sub-region in the middle that is disabled in order to open up
    // a hole in which different permissions can be applied.
    //
    ROM_MPURegionSet(1, SRAM_BASE,
                     MPU_RGN_SIZE_64K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_SUB_RGN_DISABLE_4 |
                     MPU_RGN_ENABLE);

    //
    // Configure a read-only MPU region for the 8 KB of RAM that is disabled in
    // the previous region.  This region is used for demonstrating read-only
    // permissions.
    //
    ROM_MPURegionSet(2, SRAM_BASE + 0x8000,
                     MPU_RGN_SIZE_2K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RO_USR_RO | MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for peripherals.  The region is 512 KB
    // total size, with several sub-regions disabled to prevent access to areas
    // where there are no peripherals.  This region is needed because the
    // program needs access to some peripherals.
    //
    ROM_MPURegionSet(3, 0x40000000,
                     MPU_RGN_SIZE_512K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_SUB_RGN_DISABLE_1 |
                     MPU_SUB_RGN_DISABLE_6 | MPU_SUB_RGN_DISABLE_7 |
                     MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for access to the NVIC.  The region is
    // 4 KB in size.  This region is needed because NVIC registers are needed
    // in order to control the MPU.
    //
    ROM_MPURegionSet(4, NVIC_BASE,
                     MPU_RGN_SIZE_4K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_RGN_ENABLE);

    //
    // Configure a read-write MPU region for the top 32KB of RAM.
    //
    ROM_MPURegionSet(5, SRAM_BASE + (64 * 1024),
                     MPU_RGN_SIZE_32K | MPU_RGN_PERM_NOEXEC |
                     MPU_RGN_PERM_PRV_RW_USR_RW | MPU_RGN_ENABLE);

    //
    // Need to clear the NVIC fault status register to make sure there is no
    // status hanging around from a previous program.
    //
    g_ulFaultStatus = HWREG(NVIC_FAULT_STAT);
    HWREG(NVIC_FAULT_STAT) = g_ulFaultStatus;

    //
    // Enable the MPU fault.
    //
    ROM_IntEnable(FAULT_MPU);

    //
    // Enable the MPU.  This will begin to enforce the memory protection
    // regions.  The MPU is configured so that when in the hard fault or NMI
    // exceptions, a default map will be used.  Neither of these should occur
    // in this example program.
    //
    ROM_MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to write to the flash.  This should cause a protection fault due
    // to the fact that this region is read-only.
    //
    GrStringDraw(&g_sContext, "Check flash write", -1, 0, 60, 0);
    g_ulMPUFaultCount = 0;
    HWREG(0x100) = 0x12345678;

    //
    // Verify that the fault occurred, at the expected address.
    //
    if((g_ulMPUFaultCount == 1) && (g_ulFaultStatus == 0x82) &&
       (g_ulMMAR == 0x100))
    {
        GrStringDraw(&g_sContext, " OK", -1, 200, 60, 0);
    }
    else
    {
        bFail = 1;
        GrStringDraw(&g_sContext, "NOK", -1, 200, 60, 0);
    }

    //
    // The MPU was disabled when the previous fault occurred, so it needs to be
    // re-enabled.
    //
    ROM_MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to read from the disabled section of flash, the upper 2 KB of
    // the 16 KB region.
    //
    GrStringDraw(&g_sContext, "Check flash read", -1, 0, 85, 0);
    g_ulMPUFaultCount = 0;
    g_ulValue = HWREG(0x3820);

    //
    // Verify that the fault occurred, at the expected address.
    //
    if((g_ulMPUFaultCount == 1) && (g_ulFaultStatus == 0x82) &&
       (g_ulMMAR == 0x3820))
    {
        GrStringDraw(&g_sContext, " OK", -1, 200, 85, 0);
    }
    else
    {
        bFail = 1;
        GrStringDraw(&g_sContext, "NOK", -1, 200, 85, 0);
    }

    //
    // The MPU was disabled when the previous fault occurred, so it needs to be
    // re-enabled.
    //
    ROM_MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to read from the read-only area of RAM, the middle 8 KB of the
    // 64 KB region.
    //
    GrStringDraw(&g_sContext, "Check RAM read", -1, 0, 110, 0);
    g_ulMPUFaultCount = 0;
    g_ulValue = HWREG(0x20008440);

    //
    // Verify that the RAM read did not cause a fault.
    //
    if(g_ulMPUFaultCount == 0)
    {
        GrStringDraw(&g_sContext, " OK", -1, 200, 110, 0);
    }
    else
    {
        bFail = 1;
        GrStringDraw(&g_sContext, "NOK", -1, 200, 110, 0);
    }

    //
    // The MPU should not have been disabled since the last access was not
    // supposed to cause a fault.  But if it did cause a fault, then the MPU
    // will be disabled, so re-enable it here anyway, just in case.
    //
    ROM_MPUEnable(MPU_CONFIG_HARDFLT_NMI);

    //
    // Attempt to write to the read-only area of RAM, the middle 8 KB of the
    // 64 KB region.
    //
    GrStringDraw(&g_sContext, "Check RAM write", -1, 0, 135, 0);
    g_ulMPUFaultCount = 0;
    HWREG(0x20008460) = 0xabcdef00;

    //
    // Verify that the RAM write caused a fault.
    //
    if((g_ulMPUFaultCount == 1) && (g_ulFaultStatus == 0x82) &&
       (g_ulMMAR == 0x20008460))
    {
        GrStringDraw(&g_sContext, " OK", -1, 200, 135, 0);
    }
    else
    {
        bFail = 1;
        GrStringDraw(&g_sContext, "NOK", -1, 200, 135, 0);
    }

    //
    // Display the results of the example program.
    //
    if(bFail)
    {
        GrStringDrawCentered(&g_sContext, "Failure!", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 220, 0);
   }
    else
    {
        GrStringDrawCentered(&g_sContext, "Success!", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 220, 0);
    }

    //
    // Disable the MPU, so there are no lingering side effects if another
    // program is run.
    //
    ROM_MPUDisable();

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
