//*****************************************************************************
//
// extram.c - Functions related to initialization and management of external
//            RAM.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_epi.h"
#include "driverlib/epi.h"
#include "third_party/bget/bget.h"
#include "extram.h"
#include "drivers/set_pinout.h"
#include "drivers/extflash.h"

//*****************************************************************************
//
//! \addtogroup extram_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The size of the SDRAM in bytes.
//
//*****************************************************************************
#define SDRAM_SIZE_BYTES 0x800000

//*****************************************************************************
//
// A pointer to the EPI memory aperture.
//
//*****************************************************************************
static volatile unsigned short *g_pusEPIMem;

//*****************************************************************************
//
// A marker to remind us whether or not we found external RAM.
//
//*****************************************************************************
static tBoolean g_bExtRAMPresent = false;

//*****************************************************************************
//
//! Initializes the SDRAM.
//!
//! \param ulEPIDivider is the EPI clock divider to use.
//! \param ulConfig is the SDRAM interface configuration.
//! \param ulRefresh is the refresh count in core clocks (0-2047).
//!
//! This function must be called prior to SDRAMAlloc() or SDRAMFree() and
//! after PinoutSet().  It configures the Stellaris microcontroller EPI
//! block for SDRAM access and initializes the SDRAM heap (if SDRAM is found).
//! The parameter \e ulConfig is the logical OR of several sets of choices:
//!
//! The processor core frequency must be specified with one of the following:
//!
//! - \b EPI_SDRAM_CORE_FREQ_0_15 - core clock is 0 MHz < clk <= 15 MHz
//! - \b EPI_SDRAM_CORE_FREQ_15_30 - core clock is 15 MHz < clk <= 30 MHz
//! - \b EPI_SDRAM_CORE_FREQ_30_50 - core clock is 30 MHz < clk <= 50 MHz
//! - \b EPI_SDRAM_CORE_FREQ_50_100 - core clock is 50 MHz < clk <= 100 MHz
//!
//! The low power mode is specified with one of the following:
//!
//! - \b EPI_SDRAM_LOW_POWER - enter low power, self-refresh state
//! - \b EPI_SDRAM_FULL_POWER - normal operating state
//!
//! The SDRAM device size is specified with one of the following:
//!
//! - \b EPI_SDRAM_SIZE_64MBIT - 64 Mbit device (8 MB)
//! - \b EPI_SDRAM_SIZE_128MBIT - 128 Mbit device (16 MB)
//! - \b EPI_SDRAM_SIZE_256MBIT - 256 Mbit device (32 MB)
//! - \b EPI_SDRAM_SIZE_512MBIT - 512 Mbit device (64 MB)
//!
//! The parameter \e ulRefresh sets the refresh counter in units of core
//! clock ticks.  It is an 11-bit value with a range of 0 - 2047 counts.
//!
//! \return Returns \b true on success of \b false if no SDRAM is found or
//! any other error occurs.
//
//*****************************************************************************
tBoolean
SDRAMInit(unsigned long ulEPIDivider, unsigned long ulConfig,
          unsigned long ulRefresh)
{
    //
    // If any daughter board is installed, return here indicating that SDRAM
    // is not present.
    //
    if(g_eDaughterType != DAUGHTER_NONE)
    {
        return(false);
    }

    //
    // Set the EPI divider.
    //
    EPIDividerSet(EPI0_BASE, ulEPIDivider);

    //
    // Select SDRAM mode.
    //
    EPIModeSet(EPI0_BASE, EPI_MODE_SDRAM);

    //
    // Configure SDRAM mode.
    //
    EPIConfigSDRAMSet(EPI0_BASE, ulConfig, ulRefresh);

    //
    // Set the address map.
    //
    EPIAddressMapSet(EPI0_BASE, EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6);

    //
    // Wait for the EPI initialization to complete.
    //
    while(HWREG(EPI0_BASE + EPI_O_STAT) &  EPI_STAT_INITSEQ)
    {
        //
        // Wait for SDRAM initialization to complete.
        //
    }

    //
    // Set the EPI mem pointer to the base of EPI mem
    //
    g_pusEPIMem = (unsigned short *)0x60000000;

    //
    // At this point, the SDRAM should be accessible.  We attempt a couple
    // of writes then read back the memory to see if it seems to be there.
    //
    g_pusEPIMem[0] = 0xABCD;
    g_pusEPIMem[1] = 0x5AA5;

    //
    // Read back the patterns we just wrote to make sure they are valid.  Note
    // that we declared g_pusEPIMem as volatile so the compiler should not
    // optimize these reads out of the image.
    //
    if((g_pusEPIMem[0] == 0xABCD) && (g_pusEPIMem[1] == 0x5AA5))
    {
        //
        // The memory appears to be there so remember that we found it.
        //
        g_bExtRAMPresent = true;

        //
        // Now set up the heap that SDRAMAlloc() and SDRAMFree() will use.
        //
        bpool((void *)g_pusEPIMem, SDRAM_SIZE_BYTES);
    }

    //
    // If we get this far, the SDRAM heap has been successfully initialized.
    //
    return(g_bExtRAMPresent);
}

//*****************************************************************************
//
//! Initializes any daughter-board SRAM as the external RAM heap.
//!
//! When an SRAM/Flash daughter board is installed, this function may be used
//! to configure the memory manager to use the SRAM on this board rather than
//! the SDRAM as its heap.  This allows software to call the ExtRAMAlloc() and
//! ExtRAMFree() functions to manage the SRAM on the daughter board.
//!
//! Function PinoutSet() must be called before this function.
//!
//! \return Returns \b true on success of \b false if no SRAM is found or
//! any other error occurs.
//
//*****************************************************************************
tBoolean
ExtRAMHeapInit(void)
{
    volatile unsigned char *pucTest;

    //
    // Is the correct daughter board installed?
    //
    if(g_eDaughterType == DAUGHTER_SRAM_FLASH)
    {
        //
        // Test that we can access the SRAM on the daughter board.
        //
        pucTest = (volatile unsigned char *)EXT_SRAM_BASE;

        pucTest[0] = 0xAA;
        pucTest[1] = 0x55;

        if((pucTest[0] == 0xAA) && (pucTest[1] == 0x55))
        {
            //
            // The memory appears to be there so remember that we found it.
            //
            g_bExtRAMPresent = true;

            //
            // Now set up the heap that ExtRAMAlloc() and ExtRAMFree() will use.
            //
            bpool((void *)pucTest, SRAM_MEM_SIZE);
        }
    }
    else
    {
        //
        // The SRAM/Flash daughter board is not currently installed.
        //
        return(false);
    }

    //
    // If we get here, all is well so pass this good news back to the caller.
    //
    return(true);
}

//*****************************************************************************
//
//! Allocates a block of memory from the SDRAM heap.
//!
//! \param ulSize is the size in bytes of the block of external RAM to allocate.
//!
//! This function allocates a block of external RAM and returns its pointer.  If
//! insufficient space exists to fulfill the request, a NULL pointer is returned.
//!
//! \return Returns a non-zero pointer on success or NULL if it is not possible
//! to allocate the required memory.
//
//*****************************************************************************
void *
ExtRAMAlloc(unsigned long ulSize)
{
    if(g_bExtRAMPresent)
    {
        return(bget(ulSize));
    }
    else
    {
        return((void *)0);
    }
}

//*****************************************************************************
//
//! Frees a block of memory in the SDRAM heap.
//!
//! \param pvBlock is the pointer to the block of SDRAM to free.
//!
//! This function frees a block of memory that had previously been allocated
//! using a call to ExtRAMAlloc();
//!
//! \return None.
//
//*****************************************************************************
void
ExtRAMFree(void *pvBlock)
{
    if(g_bExtRAMPresent)
    {
        brel(pvBlock);
    }
}

#if (defined INCLUDE_BGET_STATS) || (defined DOXYGEN)
//*****************************************************************************
//
//! Reports information on the current heap usage.
//!
//! \param pulTotalFree points to storage which will be written with the total
//! number of bytes unallocated in the heap.
//!
//! This function reports the total amount of memory free in the SDRAM heap and
//! the size of the largest available block.  It is included in the build only
//! if label INCLUDE_BGET_STATS is defined.
//!
//! \return Returns the size of the largest available free block in the SDRAM
//! heap.
//
//*****************************************************************************
unsigned long
ExtRAMMaxFree(unsigned long *pulTotalFree)
{
    bufsize sizeTotalFree, sizeTotalAlloc, sizeMaxFree;
    long lGet, lRel;

    if(g_bExtRAMPresent)
    {
        bstats(&sizeTotalAlloc, &sizeTotalFree, &sizeMaxFree, &lGet, &lRel);
        *pulTotalFree = (unsigned long)sizeTotalFree;
        return(sizeMaxFree);
    }
    else
    {
        *pulTotalFree = 0;
        return(0);
    }
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
