//*****************************************************************************
//
// flashstore.c - Data logger module to handle storage in flash.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/flash.h"
#include "utils/ustdlib.h"
#include "qs-logger.h"
#include "usbstick.h"
#include "flashstore.h"

//*****************************************************************************
//
// This module manages the storage of data logger data into flash memory.
//
//*****************************************************************************

//*****************************************************************************
//
// Define the beginning and end of the flash storage area.  You must make sure
// that this area is well clear of any space occupied by the application
// binary, and that this space is not used for any other purpose.
// The start and end addresses must be 1K aligned.  The end address is
// exclusive - it is 1 value greater than the last valid location used for
// storage.
//
//*****************************************************************************
#define FLASH_STORE_START_ADDR 0x20000
#define FLASH_STORE_END_ADDR 0x40000

//*****************************************************************************
//
// The next address in flash, that will be used for storing a data record.
//
//*****************************************************************************
static unsigned long g_ulStoreAddr;

//*****************************************************************************
//
// A buffer used to assemble a complete record of data prior to storing it
// in the flash.
//
//*****************************************************************************
static unsigned long g_ulRecordBuf[32];

//*****************************************************************************
//
// Initializes the flash storage. This is a stub because there is nothing
// special to do.
//
//*****************************************************************************
void
FlashStoreInit(void)
{
}

//*****************************************************************************
//
// Saves data records that are stored in the flash to an externally connected
// USB memory storage device (USB stick).
// The flash memory is scanned for the presence of store data records.  When
// records are found they are written in CSV format to the USB stick.  This
// function assumes a non-corrupted storage area, and that any records, once
// found, are contiguous with all stored records.  It will find the oldest
// record and start with that when storing.
//
//*****************************************************************************
int
FlashStoreSave(void)
{
    unsigned long ulAddr;
    unsigned long ulOldestRecord = FLASH_STORE_START_ADDR;
    unsigned long ulOldestSeconds = 0xFFFFFFFF;
    tLogRecord *pRecord;

    //
    // Show a message to the user.
    //
    SetStatusText("SAVE", "SCANNING", "FLASH", 0);

    //
    // Start at beginning of flash storage area
    //
    ulAddr = FLASH_STORE_START_ADDR;

    //
    // Search all of flash area checking every stored record.
    //
    while(ulAddr < FLASH_STORE_END_ADDR)
    {
        //
        // If a record signature is found, check for oldest record, then
        // increment to the next record
        //
        if((HWREG(ulAddr) & 0xFFFFFF00) == 0x53554100)
        {
            //
            // Get a pointer to the data record (account for flash header word)
            //
            pRecord = (tLogRecord *)(ulAddr + 4);

            //
            // If the seconds in this record are older than any found so far
            // then save the seconds value, and the address of this record
            //
            if(pRecord->ulSeconds < ulOldestSeconds)
            {
                ulOldestSeconds = pRecord->ulSeconds;
                ulOldestRecord = ulAddr;
            }

            //
            // Advance the address to the next record.
            //
            ulAddr += HWREG(ulAddr) & 0xFF;
        }

        //
        // Otherwise a record was not found so just advance to the next
        // location in flash
        //
        else
        {
            ulAddr += 4;
        }
    }

    //
    // If no "oldest" seconds was found, then there is no valid data stored
    //
    if(ulOldestSeconds == 0xFFFFFFFF)
    {
        SetStatusText("SAVE", "NO RECORDS", "FOUND", "PRESS <");
        return(1);
    }

    //
    // Open the output file on the USB stick.  It will return NULL if there
    // was any problem.
    //
    if(!USBStickOpenLogFile(0))
    {
        SetStatusText("SAVE", 0, "USB ERROR", "PRESS <");
        return(1);
    }

    //
    // Notify user we are saving data to USB
    //
    SetStatusText("SAVE", "SAVING", "TO USB", 0);

    //
    // Start reading records from flash, start at the address of the oldest
    // record, as found above.  We scan through records, assuming the flash
    // store is not corrupted.  Continue scanning until a blank space is
    // found which should indicate the end of recorded data, or until we
    // have read all the records.
    //
    ulAddr = ulOldestRecord;
    while(HWREG(ulAddr) != 0xFFFFFFFF)
    {
        unsigned long ulCount;
        unsigned long ulPartialCount;

        //
        // If a record signature is found (which it should be), extract the
        // record data and send it to USB stick.
        //
        if((HWREG(ulAddr) & 0xFFFFFF00) == 0x53554100)
        {
            //
            // Get byte count for this record
            //
            ulCount = HWREG(ulAddr) & 0xFF;

            //
            // Adjust the count and the address to remove the flash header
            //
            ulCount -= 4;
            ulAddr += 4;

            //
            // Adjust for memory wrap
            //
            if(ulAddr >= FLASH_STORE_END_ADDR)
            {
                ulAddr = FLASH_STORE_START_ADDR;
            }

            //
            // If the contents of this record go past the end of the memory
            // storage area, then perform a partial copy first.
            //
            ulPartialCount = 0;
            if((ulAddr + ulCount) >= FLASH_STORE_END_ADDR)
            {
                //
                // Find how many bytes are left on this page
                //
                ulPartialCount = FLASH_STORE_END_ADDR - ulAddr;

                //
                // Copy the portion until the end of memory store, adjust
                // remaining count and address
                //
                memcpy(g_ulRecordBuf, (void *)ulAddr, ulPartialCount);
                ulCount -= ulPartialCount;
                ulAddr = FLASH_STORE_START_ADDR;
            }

            //
            // Copy entire record (or remaining part of record if memory wrap)
            // into record buffer
            //
            memcpy(&g_ulRecordBuf[ulPartialCount / 4], (void *)ulAddr,
                   ulCount);

            //
            // Update address pointer to next record
            //
            ulAddr += ulCount;

            //
            // Now we have an entire data logger record copied from flash
            // storage into a local (contiguous) memory buffer.  Pass it
            // to the USB file writing function to write the record to the
            // USB stick.
            //
            USBStickWriteRecord((tLogRecord *)g_ulRecordBuf);
        }

        //
        // This should not happen, but it means we ended up in a non-blank
        // location that is not the start of a record.  In this case just
        // advance through memory until either a blank location or another
        // record is found.
        //
        else
        {
            //
            // Increment to next word in flash, adjust for memory wrap.
            //
            ulAddr += 4;
            if(ulAddr >= FLASH_STORE_END_ADDR)
            {
                ulAddr = FLASH_STORE_START_ADDR;
            }
        }
    }

    //
    // Close the USB stick file so that any buffers will be flushed.
    //
    USBStickCloseFile();

    //
    // Inform user that save is complete.
    //
    SetStatusText("SAVE", "USB SAVE", "COMPLETE", "PRESS <");

    //
    // Return success
    //
    return(0);
}

//*****************************************************************************
//
// This is called at the start of logging to prepare space in flash for
// storage of logged data.  It searches for the first blank area in the
// flash storage to be used for storing records.
//
// If a starting address is specified then the search is skipped and it goes
// directly to the new address.  If the starting address is 0, then it performs
// the search.
//
//*****************************************************************************
int
FlashStoreOpenLogFile(unsigned long ulStartAddr)
{
    unsigned long ulAddr;

    //
    // If a valid starting address is specified, then just use that and skip
    // the search below.
    //
    if((ulStartAddr >= FLASH_STORE_START_ADDR) &&
       (ulStartAddr < FLASH_STORE_END_ADDR))
    {
        g_ulStoreAddr = ulStartAddr;
        return(0);
    }

    //
    // Start at beginning of flash storage area
    //
    ulAddr = FLASH_STORE_START_ADDR;

    //
    // Search until a blank is found or the end of flash storage area
    //
    while((HWREG(ulAddr) != 0xFFFFFFFF) && (ulAddr < FLASH_STORE_END_ADDR))
    {
        //
        // If a record signature is found, then increment to the next record
        //
        if((HWREG(ulAddr) & 0xFFFFFF00) == 0x53554100)
        {
            ulAddr += HWREG(ulAddr) & 0xFF;
        }

        //
        // Otherwise just advance to the next location in flash
        //
        else
        {
            ulAddr += 4;
        }
    }

    //
    // If we are at the end of flash that means no blank area was found.
    // So reset to the beginning and erase the first page.
    //
    if(ulAddr >= FLASH_STORE_END_ADDR)
    {
        ulAddr = FLASH_STORE_START_ADDR;
        FlashErase(ulAddr);
    }

    //
    // When we reach here we either found a blank location, or made a new
    // blank location by erasing the first page.
    // To keep things simple we are making an assumption that the flash store
    // is not corrupted and that the first blank location implies the start
    // of a blank area suitable for storing data records.
    //
    g_ulStoreAddr = ulAddr;

    //
    // Return success indication to caller
    //
    return(0);
}

//*****************************************************************************
//
// This is called each time there is a new data record to log to the flash
// storage area.  A simple algorithm is used which rotates programming
// data log records through an area of flash.  It is assumed that the current
// page is blank.  Records are stored on the current page until a page
// boundary is crossed.  If the page boundary is crossed and the new page
// is not blank (testing only the first location), then the new page is
// erased.  Finally the entire record is programmed into flash and the
// storage pointers are updated.
//
// While storing and when crossing to a new page, if the flash page is not
// blank it is erased.  So this algorithm overwrites old data.
//
// The data is stored in flash as a record, with a flash header prepended,
// and with the record length padded to be a multiple of 4 bytes.  The flash
// header is a 3-byte magic number and one byte of record length.
//
//*****************************************************************************
int
FlashStoreWriteRecord(tLogRecord *pRecord)
{
    unsigned long ulIdx;
    unsigned long ulItemCount;
    unsigned long *pulRecord;

    //
    // Check the arguments
    //
    ASSERT(pRecord);
    if(!pRecord)
    {
        return(1);
    }

    //
    // Determine how many channels are to be logged
    //
    ulIdx = pRecord->usItemMask;
    ulItemCount = 0;
    while(ulIdx)
    {
        if(ulIdx & 1)
        {
            ulItemCount++;
        }
        ulIdx >>= 1;
    }

    //
    // Add 16-bit count equivalent of record header, time stamp, and
    // selected items mask.  This is the total number of 16 bit words
    // of the record.
    //
    ulItemCount += 6;

    //
    // Convert the count to bytes, be sure to pad to 32-bit alignment.
    //
    ulItemCount = ((ulItemCount * 2) + 3) & ~3;

    //
    // Create the flash record header, which is a 3-byte signature and a
    // one byte count of bytes in the record.  Save it at the beginning
    // of the write buffer.
    //
    ulIdx = 0x53554100 | (ulItemCount & 0xFF);
    g_ulRecordBuf[0] = ulIdx;

    //
    // Copy the rest of the record to the buffer, and get a pointer to
    // the buffer.
    //
    memcpy(&g_ulRecordBuf[1], pRecord, ulItemCount - 4);
    pulRecord = g_ulRecordBuf;

    //
    // Check to see if the record is going to cross a page boundary.
    //
    if(((g_ulStoreAddr & 0x3FF) + ulItemCount) > 0x3FF)
    {
        //
        // Find number of bytes remaining on this page
        //
        ulIdx = 0x400 - (g_ulStoreAddr & 0x3FF);

        //
        // Program part of the record on the space remaining on the current
        // page
        //
        FlashProgram(pulRecord, g_ulStoreAddr, ulIdx);

        //
        // Increment the store address by the amount just written, which
        // should make the new store address be at the beginning of the next
        // flash page.
        //
        g_ulStoreAddr += ulIdx;

        //
        // Adjust the remaining bytes to program, and the pointer to the
        // remainder of the record data.
        //
        ulItemCount -= ulIdx;
        pulRecord = &g_ulRecordBuf[ulIdx / 4];

        //
        // Check to see if the new page is past the end of store and adjust
        //
        if(g_ulStoreAddr  >= FLASH_STORE_END_ADDR)
        {
            g_ulStoreAddr = FLASH_STORE_START_ADDR;
        }

        //
        // If new page is not blank, then erase it
        //
        if(HWREG(g_ulStoreAddr) != 0xFFFFFFFF)
        {
            FlashErase(g_ulStoreAddr);
        }
    }

    //
    // Now program the remaining part of the record (if we crossed a page
    // boundary above) or the full record to the current location in flash
    //
    FlashProgram(pulRecord, g_ulStoreAddr, ulItemCount);

    //
    // Increment the storage address to the next location.
    //
    g_ulStoreAddr += ulItemCount;

    //
    // Return success indication to caller.
    //
    return(0);
}

//*****************************************************************************
//
// Return the current address being used for storing records.
//
//*****************************************************************************
unsigned long
FlashStoreGetAddr(void)
{
    return(g_ulStoreAddr);
}

//*****************************************************************************
//
// Erase the data storage area of flash.
//
//*****************************************************************************
void
FlashStoreErase(void)
{
    unsigned long ulAddr;

    //
    // Inform user we are erasing
    //
    SetStatusText("ERASE", 0, "ERASING", 0);

    //
    // Loop through entire storage area and erase each page.
    //
    for(ulAddr = FLASH_STORE_START_ADDR; ulAddr < FLASH_STORE_END_ADDR;
        ulAddr += 0x400)
    {
        FlashErase(ulAddr);
    }

    //
    // Inform user the erase is done.
    //
    SetStatusText("SAVE", "ERASE", "COMPLETE", "PRESS <");
}

//*****************************************************************************
//
// Determine if the flash block that contains the address is blank.
//
//*****************************************************************************
static int
IsBlockFree(unsigned long ulBaseAddr)
{
    unsigned long ulAddr;

    //
    // Make sure we start at the beginning of a 1K block
    //
    ulBaseAddr &= ~0x3FF;

    //
    // Loop through every address in this block and test if it is blank.
    //
    for(ulAddr = 0; ulAddr < 0x400; ulAddr += 4)
    {
        if(HWREG(ulBaseAddr + ulAddr) != 0xFFFFFFFF)
        {
            //
            // Found a non-blank location, so return indication that block
            // is not free.
            //
            return(0);
        }
    }

    //
    // If we made it to here then every location in this block is erased,
    // so return indication that the block is free.
    //
    return(1);
}

//*****************************************************************************
//
// Report to the user the amount of free space and used space in the data
// storage area.
//
//*****************************************************************************
void
FlashStoreReport(void)
{
    unsigned long ulAddr;
    unsigned long ulFreeBlocks = 0;
    unsigned long ulUsedBlocks = 0;
    static char cBufFree[16];
    static char cBufUsed[16];

    //
    // Loop through each block of the storage area and count how many blocks
    // are free and non-free.
    //
    for(ulAddr = FLASH_STORE_START_ADDR; ulAddr < FLASH_STORE_END_ADDR;
        ulAddr += 0x400)
    {
        if(IsBlockFree(ulAddr))
        {
            ulFreeBlocks++;
        }
        else
        {
            ulUsedBlocks++;
        }
    }

    //
    // Report the result to the user via a status display screen.
    //
    usnprintf(cBufFree, sizeof(cBufFree), "FREE: %3uK", ulFreeBlocks);
    usnprintf(cBufUsed, sizeof(cBufUsed), "USED: %3uK", ulUsedBlocks);
    SetStatusText("FREE FLASH", cBufFree, cBufUsed, "PRESS <");
}
