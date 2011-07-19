//*****************************************************************************
//
// tftp_qs - TFTP server support functions for the Tempest DK quickstart
//           application.  This file supports GET and PUT requests for binary
//           file system images stored in external flash memories.
//
// Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "drivers/extflash.h"
#include "drivers/ssiflash.h"
#include "drivers/set_pinout.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "utils/tftp.h"
#include "utils/fswrapper.h"
#include "tftp_qs.h"

//*****************************************************************************
//
// Determines the size of any image currently in the SSI flash device.
//
//*****************************************************************************
static unsigned long
TFTP9B96GetEEPROMImageSize(void)
{
    unsigned long pulHeader[2];
    unsigned long ulLength;

    //
    // Read the first 8 bytes of the SSI device content.
    //
    ulLength = SSIFlashRead(0, 2 * sizeof(unsigned long),
                            (unsigned char *)pulHeader);
    if(ulLength == (2 * sizeof(unsigned long)))
    {
        //
        // We read the device.  Now check to see if there appears to be a file
        // system header there.
        //
        if(pulHeader[0] == (unsigned long)FILE_SYSTEM_MARKER)
        {
            //
            // The file system marker is there so the length should follow it.
            //
            return(pulHeader[1]);
        }
        else
        {
            //
            // In this case, the file system marker is not present so we just
            // return 0 to indicate that there is no image in the device.  This
            // makes the assumption that we will only ever write position-
            // independent file system images generated using makefsfile.  This
            // is valid given the example applications we are including but you
            // may like to change this if you use the SSI flash to store
            // anything else.
            //
            return(0);
        }
    }
    else
    {
        //
        // We can't read from the device so return 0 to indicate an error.
        //
        return(0);
    }
}

//*****************************************************************************
//
// Writes a block of data to the SSI flash device.  The position of the block
// is determined by the block number (ulBlockNum) and offset (ulDataRemaining)
// fields of the psTFTP structure passed and the total length of the data to
// write is in ulDataLength.
//
// The assumption is made here that the SSI flash sector size is an integer
// multiple of TFTP_BLOCK_SIZE (512).  This is valid for the devices populates
// on the dk-lm3s9b96 board but, if you use this driver in another design,
// take care to verify that this assumption still holds.
//
//*****************************************************************************
static tTFTPError
TFTP9B96PutEEPROM(tTFTPConnection *psTFTP)
{
    unsigned long ulOffset, ulSize;
    tBoolean bRetcode;

    //
    // Where does this block of data get written to?  We calculate this knowing
    // the block number that we are given (TFTP blocks are fixed sized) and
    // the offset into the block as provided in the ulDataRemaining field.
    //
    ulOffset = ((psTFTP->ulBlockNum - 1) * TFTP_BLOCK_SIZE) +
               psTFTP->ulDataRemaining;

    //
    // Does this offset start on a new flash sector boundary. If so, we need to
    // erase a sector of the flash.
    //
    if(!(ulOffset % SSIFlashSectorSizeGet()))
    {
        //
        // The start address is at the start of a block so we need to erase
        // the block.
        //
        bRetcode = SSIFlashSectorErase(ulOffset, true);
        if(!bRetcode)
        {
            //
            // Oops - we can't erase the block.  Report an error.
            //
            psTFTP->pcErrorString = "Flash erase failure.";
            return(TFTP_ERR_NOT_DEFINED);
        }
    }

    //
    // Program the new data.
    //
    ulSize = SSIFlashWrite(ulOffset, psTFTP->ulDataLength, psTFTP->pucData);

    //
    // Did all go as planned?
    //
    if(ulSize == psTFTP->ulDataLength)
    {
        //
        // Everything is fine.
        //
        return(TFTP_OK);
    }
    else
    {
        //
        // Oops - we can't erase the block.  Report an error.
        //
        psTFTP->pcErrorString = "Flash write failure.";
        return(TFTP_ERR_NOT_DEFINED);
    }
}

//*****************************************************************************
//
// Read a block of data from the SSI flash device.
//
//*****************************************************************************
static tTFTPError
TFTP9B96GetEEPROM(tTFTPConnection *psTFTP)
{
    unsigned long ulOffset, ulLength;

    //
    // Where does this block of data get read from?  This is calculated from
    // the requested TFTP block number.
    //
    ulOffset = ((psTFTP->ulBlockNum - 1) * TFTP_BLOCK_SIZE);

    //
    // Read the data from the flash device into the supplied buffer.
    //
    ulLength = SSIFlashRead(ulOffset, psTFTP->ulDataLength,
                            psTFTP->pucData);

    //
    // Did we read the expected number of bytes?
    //
    if(ulLength == psTFTP->ulDataLength)
    {
        //
        // Yes - tell the caller everything went fine.
        //
        return(TFTP_OK);
    }
    else
    {
        //
        // We read a different number of bytes from the requested count.  Tell
        // the caller we have a problem.
        //
        psTFTP->pcErrorString = "SSI flash read error.";
        return(TFTP_ERR_NOT_DEFINED);
    }
}

//*****************************************************************************
//
// Determines the size of any image currently in the external flash.
//
//*****************************************************************************
static unsigned long
TFTP9B96GetExtFlashImageSize(void)
{
    unsigned char *pcLoop;

    //
    // We support a single, binary image in the external flash.  First we check
    // for a file system image marker and, if we find it, read the length from
    // the header.
    //
    if(*((unsigned long *)EXT_FLASH_BASE) == (unsigned long)FILE_SYSTEM_MARKER)
    {
        //
        // Yes - there is a file there. How big is it?
        //
        return(*((unsigned long *)(EXT_FLASH_BASE + 4)));
    }
    else
    {
        //
        // There is something other than a file system image in flash.  Walk
        // backwards through the flash to find the last non-0xFF byte and
        // assume that everything between there and the start of flash is the
        // image.  This is not foolproof since an image that ends in one or
        // more 0xFFs will not end correctly.  If you wrote it back, though,
        // you would get the correct behaviour so I guess this is OK after all.
        //
        pcLoop = (unsigned char *)(EXT_FLASH_BASE + ExtFlashChipSizeGet() - 1);
        while((pcLoop >= (unsigned char *)EXT_FLASH_BASE) && (*pcLoop == 0xFF))
        {
            pcLoop--;
        }

        //
        // The image size is the difference between the current pointer and
        // the start of flash + 1.
        //
        return((unsigned long)(pcLoop + 1) - EXT_FLASH_BASE);
    }
}

//*****************************************************************************
//
// Writes incoming TFTP PUT data packets to external flash.  This
// implementation assumes that the target flash device has sectors of no less
// than TFTP_BLOCK_SIZE (512) bytes and that the sectors are all multiples of
// 512 bytes in size.  It will break if these assumptions are not correct.
//
//*****************************************************************************
static tTFTPError
TFTP9B96PutExtFlash(tTFTPConnection *psTFTP)
{
    unsigned long ulOffset, ulStart, ulBlockStart, ulSize;
    tBoolean bRetcode;

    //
    // Where does this block of data get written to?  We calculate this knowing
    // the block number that we are given (TFTP blocks are fixed sized) and
    // the offset into the block as provided in the ulDataRemaining field.
    //
    ulOffset = ((psTFTP->ulBlockNum - 1) * TFTP_BLOCK_SIZE) +
               psTFTP->ulDataRemaining;

    //
    // Does this offset start on a new flash block boundary. If so, we need to
    // erase a block of the flash.
    //
    ulStart = EXT_FLASH_BASE + ulOffset;
    ExtFlashBlockSizeGet(ulStart, &ulBlockStart);

    //
    // Is this packet being written at the start of a flash block?
    //
    if(ulStart == ulBlockStart)
    {
        //
        // The start address is at the start of a block so we need to erase
        // the block.
        //
        bRetcode = ExtFlashBlockErase(ulBlockStart, true);
        if(!bRetcode)
        {
            //
            // Oops - we can't erase the block.  Report an error.
            //
            psTFTP->pcErrorString = "Flash erase failure.";
            return(TFTP_ERR_NOT_DEFINED);
        }
    }

    //
    // Program the new data.
    //
    ulSize = ExtFlashWrite(ulStart, psTFTP->ulDataLength, psTFTP->pucData);

    //
    // Did all go as planned?
    //
    if(ulSize == psTFTP->ulDataLength)
    {
        //
        // Everything is fine.
        //
        return(TFTP_OK);
    }
    else
    {
        //
        // Oops - we can't erase the block.  Report an error.
        //
        psTFTP->pcErrorString = "Flash write failure.";
        return(TFTP_ERR_NOT_DEFINED);
    }
}

//*****************************************************************************
//
// Reads data for a TFTP GET data packets from external flash.
//
//*****************************************************************************
static tTFTPError
TFTP9B96GetExtFlash(tTFTPConnection *psTFTP)
{
    unsigned long ulOffset;

    UARTprintf("Get block %d, %d\n", psTFTP->ulBlockNum, psTFTP->ulDataLength);

    //
    // Where does this block of data get read from?  This is calculated from
    // the requested TFTP block number.
    //
    ulOffset = ((psTFTP->ulBlockNum - 1) * TFTP_BLOCK_SIZE);

    //
    // Copy the data into the supplied buffer.
    //
    memcpy(psTFTP->pucData, (unsigned char *)(EXT_FLASH_BASE + ulOffset),
           psTFTP->ulDataLength);

    //
    // Tell the caller everything went fine.
    //
    return(TFTP_OK);
}

//*****************************************************************************
//
// Signals that the TFTP connection is being closed down.
//
//*****************************************************************************
static void
TFTP9B96Close(tTFTPConnection *psTFTP)
{
    //
    // Nothing to do here currently.
    //
}

//*****************************************************************************
//
// Checks incoming TFTP request to determine if we want to handle it.
//
// \param psTFTP points to the TFTP connection instance structure for this
// request.
// \param bGet is \b true of the request is a GET (read) or \b false if it is
// a PUT (write).
// \param pucFileName points to a NULL terminated string containing the
// requested filename.
// \param tTFTPMode indicates the requested transfer mode, ASCII or binary.
//
// This function is called by the TFTP server whenever a new request is
// received.  It checks the passed filename to determine whether it is valid.
// If the filename is valid, the function sets fields \e ulDataRemaining, \e
// pfnGetData and \e pfnClose in the \e psTFTP structure for a GET request or
// fields \e pfnPutData, \e pfnClosebefore returning
// \e TFTP_OK to continue processing the request.
//
// This implementation supports requests for "eeprom" which will read or write
// the image stored in the serial flash device and "extflash" which will
// access an image stored in the flash provided by the Flash/SRAM/LCD daughter
// board if this is installed.
//
// \return Returns \e TFTP_OK if the request should be processed or any other
// TFTP error code otherwise.  In cases where \e TFTP_OK is not returned, the
// field psTFTP->pcErrorString may be set with an ASCII error string which
// will be returned to the TFTP client.
//
//*****************************************************************************
static tTFTPError
TFTP9B96Request(tTFTPConnection *psTFTP, tBoolean bGet, char *pucFileName,
                tTFTPMode eMode)
{
    UARTprintf("Incoming TFTP request %s %s.\n", bGet ? "GET" : "PUT",
               pucFileName);

    //
    // Are we being asked to write the EEPROM (SSI flash) image?
    //
    if(!ustrnicmp(pucFileName, "eeprom", 7))
    {
        //
        // Set the appropriate callback functions depending upon the
        // type of request received.
        //
        psTFTP->pfnClose = TFTP9B96Close;

        //
        // Is this a GET or a PUT request?
        //
        if(bGet)
        {
            //
            // GET request - fill in the image size and the data transfer
            // function pointer.
            //
            psTFTP->pfnGetData = TFTP9B96GetEEPROM;
            psTFTP->ulDataRemaining = TFTP9B96GetEEPROMImageSize();
        }
        else
        {
            //
            // PUT request - fill in the data transfer function pointer.
            //
            psTFTP->pfnPutData = TFTP9B96PutEEPROM;
        }
    }

    //
    // Are we being asked to write to the external flash device?
    //
    else if (!ustrnicmp(pucFileName, "extflash", 9))
    {
        //
        // Yes - is the SRAM/Flash daughter board installed?
        //
        if(g_eDaughterType == DAUGHTER_SRAM_FLASH)
        {
            //
            // The daughter board is present so go ahead and fill in the
            // appropriate fields in the instance data.
            //
            psTFTP->pfnClose = TFTP9B96Close;

            //
            // Is this a GET or PUT request?
            //
            if(bGet)
            {
                //
                // GET request - fill in the image size and the data transfer
                // function pointer.
                //
                psTFTP->pfnGetData = TFTP9B96GetExtFlash;
                psTFTP->ulDataRemaining = TFTP9B96GetExtFlashImageSize();
            }
            else
            {
                //
                // PUT request - fill in the data transfer function pointer.
                //
                psTFTP->pfnPutData = TFTP9B96PutExtFlash;
            }
        }
        else
        {
            //
            // The external flash is not present.
            //
            psTFTP->pcErrorString = "File not found.";
            return(TFTP_FILE_NOT_FOUND);
        }
    }

    else
    {
        //
        // The filename is not valid.
        //
        psTFTP->pcErrorString = "File not found.";
        return(TFTP_FILE_NOT_FOUND);
    }

    //
    // If we get here, all is well and the transfer can continue.
    //
    return(TFTP_OK);
}

//*****************************************************************************
//
// Initializes the TFTP server supporting the DK-LM3S9B96 board.
//
// This function initializes the lwIP TFTP server and starts listening for
// incoming requests from clients.  It must be called after PinoutSet(),
// after the network stack is initialized and after SSIFlashInit().
//
// \return None.
//
//*****************************************************************************
void TFTPQSInit(void)
{
    //
    // Initialize the TFTP module and pass it our board-specific GET and PUT
    // request handler function pointer.
    //
    TFTPInit(TFTP9B96Request);
}

