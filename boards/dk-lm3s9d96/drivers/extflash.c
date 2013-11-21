//*****************************************************************************
//
// extflash.c - Functions accessing the external flash on the SRAM/Flash
//              daughter board.
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

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/epi.h"
#include "extflash.h"

//*****************************************************************************
//
//! \addtogroup extflash_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Definitions of important bits in the flash status register.
//
//*****************************************************************************
#define FLASH_STATUS_DATA_POLL 0x80
#define FLASH_STATUS_ERROR     0x20

//*****************************************************************************
//
// Check to determine whether or not the flash appears to be present.  We do
// this by attempting to read the first 3 bytes of the CFI Query Identification
// String which should contain "QRY".  If we get this, we assume all is well,
// otherwise we assume the flash is not accessible.
//
//*****************************************************************************
static tBoolean
CheckFlashPresent(void)
{
    tBoolean bRetcode;

    //
    // Set autoselect mode
    //
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
    HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0x90;

    //
    // Set CFI query
    //
    HWREGB(EXT_FLASH_BASE + 0xAA) = 0x98;

    //
    // Check that the query string is returned correctly.
    //
    if((HWREGB(EXT_FLASH_BASE + 0x20) != 'Q') ||
       (HWREGB(EXT_FLASH_BASE + 0x22) != 'R') ||
       (HWREGB(EXT_FLASH_BASE + 0x24) != 'Y'))
    {
        //
        // We can't access the flash correctly - the query string was not
        // read as expected.
        //
        bRetcode = false;
    }
    else
    {
        //
        // We read the query string so tell the caller all is well.
        //
        bRetcode = true;
    }

    //
    // Return to read array mode.  We need to do this twice.  The first write
    // gets us back to autoselect mode and the second returns us to array
    // read.
    //
    HWREGB(EXT_FLASH_BASE) = 0xF0;
    HWREGB(EXT_FLASH_BASE) = 0xF0;

    //
    // Tell the caller whether or not the flash is accessible.
    //
    return(bRetcode);
}

//*****************************************************************************
//
//! Determines whether the external flash on the Flash/SRAM/LCD daughter board
//! is accessible.
//!
//! This function checks to ensure that the external flash is accessible by
//! reading back the "QRY" tag at the beginning of the CFI query block.  If
//! this is read correctly, \b true is returned indicating that the flash is
//! now accessible, otherwise \b false is returned.  It is assumed that the EPI
//! configuration has previously been set correctly using a call to
//! PinoutSet().
//!
//! On return, the flash device is in read array mode.
//!
//! \return Returns \b true if the flash is found and accessible or \b false
//! if the flash device is not found.
//
//*****************************************************************************
tBoolean
ExtFlashPresent(void)
{
    //
    // Make sure we can access the flash.
    //
    return(CheckFlashPresent());
}

//*****************************************************************************
//
//! Queries the total size of the attached flash device.
//!
//! This function read the flash CFI query block to determine the total size of
//! the device in bytes and returns this value to the caller.  It is assumed
//! that the EPI configuration has previously been set correctly using a call
//! to PinoutSet().
//!
//! \return Returns the total size of the attached flash device in bytes.
//
//*****************************************************************************
unsigned long
ExtFlashChipSizeGet(void)
{
    unsigned char ucSize;

    //
    // Set autoselect mode
    //
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
    HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0x90;

    //
    // Set CFI query
    //
    HWREGB(EXT_FLASH_BASE + 0xAA) = 0x98;

    //
    // Read the device size from the CFI block.  This is returned as a power
    // of two.
    //
    ucSize = HWREGB(EXT_FLASH_BASE + 0x4E);

    //
    // Switch back to read mode.  We need to do this twice.  The first write
    // gets us back to autoselect mode and the second returns to read array
    // mode.
    //
    HWREGB(EXT_FLASH_BASE) = 0xF0;
    HWREGB(EXT_FLASH_BASE) = 0xF0;

    //
    // Return the size of the device in bytes.
    //
    return(1 << ucSize);
}

//*****************************************************************************
//
//! Returns the size of a given flash block.
//!
//! \param ulAddress is the flash address whose block information is being
//! queried.
//! \param pulBlockAddr is storage for the returned block start address.
//!
//! This function determines the start address and size of the flash block
//! which contains the supplied address.  The block information is determined
//! by parsing the flash CFI query data.  If an invalid address is passed, the
//! block address and size are both returned as 0.
//!
//! It is assumed that the EPI configuration has previously been set correctly
//! using a call to PinoutSet().
//!
//! \return Returns the size of the flash block which contains \e ulAddress or
//! 0 if \e ulAddress is invalid.
//
//*****************************************************************************
unsigned long
ExtFlashBlockSizeGet(unsigned long ulAddress, unsigned long *pulBlockAddr)
{
    unsigned char *pcCFIQuery;
    unsigned char ucSize;
    unsigned short usNumBlocks;
    unsigned long ulBlockStart, ulBlockEnd, ulSizeBlock;
    unsigned long ulRetcode, ulBlockAddr;

    //
    // Modify the address to be relative to the start of the flash device.
    //
    ulAddress -= EXT_FLASH_BASE;

    //
    // Set return values to indicate an error for now.
    //
    ulRetcode = 0;
    ulBlockAddr = 0;

    //
    // Set autoselect mode
    //
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
    HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0x90;

    //
    // Set CFI query
    //
    HWREGB(EXT_FLASH_BASE + 0xAA) = 0x98;

    //
    // Get a pointer to the CFI query information.
    //
    pcCFIQuery = (unsigned char *)EXT_FLASH_BASE;

    //
    //
    // Read the device size from the CFI block.  This is returned as a power
    // of two.
    //
    ucSize = pcCFIQuery[0x4E];

    //
    // Is the address passed within the device?
    //
    if(ulAddress < (1 << ucSize))
    {
        //
        // The address appears to be valid.  Now walk the CFI regions to
        // determine which one it is within.
        //
        ulBlockStart = 0;
        for(ucSize = 0; ucSize < pcCFIQuery[0x58]; ucSize++)
        {
            //
            // How many erase blocks exist in this region?
            //
            usNumBlocks = (pcCFIQuery[0x5A + (8 * ucSize)] +
                          (pcCFIQuery[0x5C + (8 * ucSize)] << 8) + 1);

            //
            // How big is each block?
            //
            ulSizeBlock = (pcCFIQuery[0x5E + (8 * ucSize)] +
                          (pcCFIQuery[0x60 + (8 * ucSize)] << 8)) * 256;

            // Where does this region end?
            //
            ulBlockEnd = ulBlockStart + ((unsigned long)usNumBlocks *
                         ulSizeBlock);

            //
            // Does the passed address fall within this region?
            //
            if(ulAddress < ulBlockEnd)
            {
                //
                // Yes - determine where the block starts.
                //
                ulBlockAddr = EXT_FLASH_BASE + (ulBlockStart +
                              (((ulAddress - ulBlockStart) / ulSizeBlock) *
                              ulSizeBlock));

                //
                // Set up to return the block size.
                //
                ulRetcode = ulSizeBlock;

                //
                // Drop out of the loop now that we have found the required
                // block.
                //
                break;
            }

            //
            // Move on to the next region.
            //
            ulBlockStart = ulBlockEnd;
        }
    }

    //
    // Return to read array mode.  We need to do this twice.  The first write
    // gets us back to autoselect mode and the second returns us to array
    // read.
    //
    HWREGB(EXT_FLASH_BASE) = 0xF0;
    HWREGB(EXT_FLASH_BASE) = 0xF0;

    //
    // Return the block start address and size to the caller.
    //
    *pulBlockAddr = ulBlockAddr;
    return(ulRetcode);

}

//*****************************************************************************
//
//! Determines whether the last erase operation has completed.
//!
//! \param ulAddress is an address within the area of flash which was last
//! erased using a call to ExtFlashBlockErase() or any valid flash address if
//! ExtFlashChipErase() was last issued.
//! \param pbError is storage for the returned error indicator.
//!
//! This function determines whether the last flash erase operation has
//! completed.  The address passed in parameter \e ulAddress must correspond to
//! an address in the region which was last erased.  When the operation
//! completes, \b true is returned and the caller should check the value of
//! \e *pbError to determine whether or not the operation was successful. If
//! \e *pbError is \b true, an error was reported by the device and the flash
//! may not have been correctly erased.  If \e *pbError is \b false, the
//! operation completed successfully.
//!
//! It is assumed that the EPI configuration has previously been set correctly
//! using a call to PinoutSet().
//!
//! \return Returns \b true if the erase operation completed or \b false if it
//! is still ongoing.
//
//*****************************************************************************
tBoolean
ExtFlashEraseIsComplete(unsigned long ulAddress, tBoolean *pbError)
{
    //
    // If reading the location returns 0xFF, the erase must be complete.
    //
    if(HWREGB(ulAddress) == 0xFF)
    {
        *pbError = false;
        return(true);
    }
    else
    {
        //
        // The erase is not complete so we look at the value read to determine
        // whether an error occurred or not.
        //
        if(HWREGB(ulAddress) & FLASH_STATUS_ERROR)
        {
            //
            // An error seems to have been reported. Check once more as
            // indicated in the datasheet.
            if(HWREGB(ulAddress) == 0xFF)
            {
                //
                // False alarm - tell the caller the operation completed.
                //
                *pbError = false;
                return(true);
            }
            else
            {
                //
                // Looks as if the error was real so issue a Read/Reset to get
                // the chip back into read mode and clear the error condition.
                // then report the failure to the caller.
                //
                HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
                HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
                HWREGB(EXT_FLASH_BASE) = 0xF0;

                *pbError = true;
            }
        }
        else
        {
            *pbError = false;
        }

        //
        // If an error occurred, we return true so that any polling loop will
        // exit.  If no error occurred, the operation is not complete so we
        // return false.
        //
        return(*pbError);
    }
}

//*****************************************************************************
//
//! Erases a single block of the flash device.
//!
//! \param ulAddress is an address within the block to be erased.
//! \param bSync indicates whether to return immediately or poll until the
//! erase operation has completed.
//!
//! This function erases a single block of the flash device.  The block erased
//! is identified by parameter \e ulAddress.  If this is not a block start
//! address, the block containing \e ulAddress is erased.  Applications may
//! call ExtFlashBlockSizeGet() to determine the size and start address of
//! the block containing a particular flash address.
//!
//! If called with \e bAsync set to \b false, the function will poll until the
//! erase operation completes and only return at this point. If, however, \e
//! bAsync is \b true, the function will return immediately and the caller can
//! determine when the erase operation completes by polling function
//! ExtFlashEraseIsComplete(), passing it the same \e ulAddress parameter as
//! was passed to this function.
//!
//! It is assumed that the EPI configuration has previously been set correctly
//! using a call to PinoutSet().
//!
//! \note A block erase will typically take 0.8 seconds to complete and may
//! take up to 6 seconds in the worst case.
//!
//! \return Returns \b true to indicate success or \b false to indicate that
//! an error occurred.
//
//*****************************************************************************
tBoolean
ExtFlashBlockErase(unsigned long ulAddress, tBoolean bSync)
{
    tBoolean bError;

    //
    // Issue the command sequence to erase the block.
    //
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
    HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0x80;
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
    HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
    HWREGB(ulAddress) = 0x30;

    //
    // Have we been asked to block until the operation completes?
    //
    if(bSync)
    {
        //
        // We've been asked to wait.  Poll the status until we are told the
        // erase completed.
        //
        while(!ExtFlashEraseIsComplete(ulAddress, &bError))
        {
            //
            // Keep waiting...
            //
        }

        //
        // Return true if no error was reported in the erase operation.
        //
        return(!bError);
    }

    //
    // If we get this far, either we started the erase operation and are
    // returning immediately because bSync is true or the operation completed
    // successfully.
    //
    return(true);
}

//*****************************************************************************
//
//! Erases the entire flash device.
//!
//! \param bSync indicates whether to return immediately or poll until the
//! erase operation has completed.
//!
//! This function erases the entire flash device.  If called with \e bAsync set
//! to \b false, the function will poll until the erase operation completes and
//! only return at this point. If, however, \e bAsync is \b true, the function
//! will return immediately and the caller can determine when the erase
//! operation completes by polling function ExtFlashEraseIsComplete(), passing
//! \e EXT_FLASH_BASE as the \e ulAddress parameter.
//!
//! It is assumed that the EPI configuration has previously been set correctly
//! using a call to PinoutSet().
//!
//! \note A chip erase will typically take 80 seconds to complete and may
//! take up to 400 seconds in the worst case.
//!
//! \return Returns \b true to indicate success or \b false to indicate that
//! an error occurred.
//
//*****************************************************************************
tBoolean
ExtFlashChipErase(tBoolean bSync)
{
    tBoolean bError;

    //
    // Issue the command sequence to erase the entire chip.
    //
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
    HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0x80;
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
    HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
    HWREGB(EXT_FLASH_BASE + 0xAAA) = 0x10;

    //
    // Have we been asked to block until the operation completes?
    //
    if(bSync)
    {
        //
        // We've been asked to wait.  Poll the status until we are told the
        // erase completed.
        //
        while(!ExtFlashEraseIsComplete(EXT_FLASH_BASE, &bError))
        {
            //
            // Keep waiting...
            //
        }

        //
        // Return true if no error was reported in the erase operation.
        //
        return(!bError);
    }

    //
    // If we get this far, either we started the erase operation and are
    // returning immediately because bSync is true or the operation completed
    // successfully.
    //
    return(true);
}

//*****************************************************************************
//
//! Writes data to the flash device.
//!
//! \param ulAddress is the address that the data is to be written to.
//! \param ulLength is the number of bytes of data to write.
//! \param pucSrc points to the first byte of data to write.
//!
//! This function writes data to the flash device.  Callers must ensure that
//! the are of flash being written has previously been erased or, at least,
//! that writing the data will not require any bits which are already stored
//! as 0s in the flash to revert to 1s.  The function returns either when an
//! error is detected or all data has been successfully written to the device.
//!
//! It is assumed that the EPI configuration has previously been set correctly
//! using a call to PinoutSet().
//!
//! \note Programming data may take as long as 200 microseconds per byte.
//!
//! \return Returns the number of bytes successfully written.
//
//*****************************************************************************
unsigned long
ExtFlashWrite(unsigned long ulAddress, unsigned long ulLength,
              unsigned char *pucSrc)
{
    unsigned long ulLoop;

    //
    // Program each byte in turn.
    //
    for(ulLoop = 0; ulLoop < ulLength; ulLoop++)
    {
        //
        // Send the command to program this byte.
        //
        HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
        HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
        HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xA0;
        HWREGB(ulAddress) = *pucSrc;

        //
        // Wait for this byte to be programmed.
        //
        while(1)
        {
            if(HWREGB(ulAddress) == *pucSrc)
            {
                //
                // The flash reads back the same data we wrote to it so this
                // write operation has completed.
                //
                break;
            }
            else
            {
                //
                // The operation has not completed. Is an error reported?
                //
                if(HWREGB(ulAddress) & FLASH_STATUS_ERROR)
                {
                    //
                    // The error bit appears to be set but this may just be
                    // because the operation just completed. Check the location
                    // once more to be sure.
                    //
                    if(HWREGB(ulAddress) == *pucSrc)
                    {
                        //
                        // Yes - it completed.  Move on to the next byte.
                        //
                        break;
                    }
                    else
                    {
                        //
                        // An error was reported.  We clear the error and return
                        // early telling the caller how many bytes we actually
                        // programmed.
                        //
                        HWREGB(EXT_FLASH_BASE + 0xAAA) = 0xAA;
                        HWREGB(EXT_FLASH_BASE + 0x555) = 0x55;
                        HWREGB(EXT_FLASH_BASE) = 0xF0;

                        return(ulLoop);
                    }
                }
            }
        }
        //
        // Move on to the next location.
        //
        ulAddress++;
        pucSrc++;

    }

    //
    // If we get here, we successfully programmed everything we were given.
    //
    return(ulLength);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
