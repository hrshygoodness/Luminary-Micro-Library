//*****************************************************************************
//
// ssiflash.c - Driver for the Winbond Serial Flash on the development board.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9107 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "ssiflash.h"
#include "ssi_hw.h"

//*****************************************************************************
//
//! \addtogroup ssiflash_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The rate of the SSI clock and derived values.
//
//*****************************************************************************
#define SSI_CLK_RATE            10000000
#define SSI_CLKS_PER_MS         (SSI_CLK_RATE / 1000)
#define STATUS_READS_PER_MS     (SSI_CLKS_PER_MS / 16)

//*****************************************************************************
//
// Labels for the instructions supported by the Winbond part.
//
//*****************************************************************************
#define INSTR_WRITE_ENABLE      0x06
#define INSTR_WRITE_DISABLE     0x04
#define INSTR_READ_STATUS       0x05
#define INSTR_WRITE_STATUS      0x01
#define INSTR_READ_DATA         0x03
#define INSTR_FAST_READ         0x0B
#define INSTR_PAGE_PROGRAM      0x02
#define INSTR_BLOCK_ERASE       0xD8
#define INSTR_SECTOR_ERASE      0x20
#define INSTR_CHIP_ERASE        0xC7
#define INSTR_POWER_DOWN        0xB9
#define INSTR_POWER_UP          0xAB
#define INSTR_MAN_DEV_ID        0x90
#define INSTR_JEDEC_ID          0x9F

//*****************************************************************************
//
// Status register bit definitions
//
//*****************************************************************************
#define STATUS_BUSY                 0x01
#define STATUS_WRITE_ENABLE_LATCH   0x02
#define STATUS_BLOCK_PROTECT_0      0x04
#define STATUS_BLOCK_PROTECT_1      0x08
#define STATUS_BLOCK_PROTECT_2      0x10
#define STATUS_TOP_BOTTON_WP        0x20
#define STATUS_REGISTER_PROTECT     0x80

//*****************************************************************************
//
// Manufacturer and device IDs that we expect to see.
//
//*****************************************************************************
#define MANUFACTURER_WINBOND    0xEF
#define DEVICE_ID_W25X80A       0x13

//*****************************************************************************
//
// Block, sector, page and chip sizes for the supported device.  Some of the
// code here assumes (reasonably) that these are all powers of two.
//
//*****************************************************************************
#define W25X80A_BLOCK_SIZE      (64 * 1024)
#define W25X80A_SECTOR_SIZE     (4 * 1024)
#define W25X80A_PAGE_SIZE       256
#define W25X80A_CHIP_SIZE       (1024 * 1024)

//*****************************************************************************
//
// The number of times we query the device status waiting for it to be idle
// after various operations have taken place and during initialization.
//
//*****************************************************************************
#define MAX_BUSY_POLL_IDLE              100
#define MAX_BUSY_POLL_ERASE_SECTOR      (STATUS_READS_PER_MS * 250)
#define MAX_BUSY_POLL_ERASE_BLOCK       (STATUS_READS_PER_MS * 1000)
#define MAX_BUSY_POLL_ERASE_CHIP        (STATUS_READS_PER_MS * 10000)
#define MAX_BUSY_POLL_PROGRAM_PAGE      (STATUS_READS_PER_MS * 3)

//*****************************************************************************
//
// Reads the serial flash device status register.
//
// This function reads the serial flash status register and returns the value.
//
// \return Returns the current contents of the serial flash device status
// register.
//
//*****************************************************************************
static unsigned char
SSIFlashStatusGet(void)
{
    unsigned long ulStatus;

    //
    // Assert the chip select for the serial flash.
    //
    GPIOPinWrite(SFLASH_CS_BASE, SFLASH_CS_PIN, 0);

    //
    // Send the status register read instruction and read back a dummy byte.
    //
    SSIDataPut(SFLASH_SSI_BASE, INSTR_READ_STATUS);
    SSIDataGet(SFLASH_SSI_BASE, &ulStatus);

    //
    // Write a dummy byte then read back the actual status.
    //
    SSIDataPut(SFLASH_SSI_BASE, 0xFF);
    SSIDataGet(SFLASH_SSI_BASE, &ulStatus);

    //
    // Deassert the chip select for the serial flash.
    //
    GPIOPinWrite(SFLASH_CS_BASE, SFLASH_CS_PIN, SFLASH_CS_PIN);

    //
    // Return the status read.
    //
    return((unsigned char)(ulStatus & 0xFF));
}

//*****************************************************************************
//
// Empties the SSI receive FIFO of any data it may contain.
//
// \return None.
//
//*****************************************************************************
static void
SSIFlashRxFlush(void)
{
    unsigned long ulDummy;

    while(SSIDataGetNonBlocking(SFLASH_SSI_BASE, &ulDummy))
    {
        //
        // Spin until there is no more data to read.
        //
    }
}

//*****************************************************************************
//
// Write an instruction to the serial flash.
//
// \param ucInstruction is the instruction code that is to be sent.
// \param ucData is a pointer to optional instruction data that will be sent
// following the instruction code provided in \e ucInstruction.  This parameter
// is ignored if \e usLen is 0.
// \param usLen is the length of the optional instruction data.
//
// This function writes an instruction to the serial flash along with any
// provided instruction-specific data.  On return, the flash chip select
// remains asserted allowing an immediate call to SSIFlashInstructionRead().
// To finish an instruction and deassert the chip select, a call must be made
// to SSIFlashInstructionEnd() after this call.
//
// It is assumed that the caller has already determined that the serial flash
// is not busy and is able to accept a new instruction at this point.
//
//*****************************************************************************
static void
SSIFlashInstructionWrite(unsigned char ucInstruction, unsigned char *ucData,
                         unsigned short usLen)
{
    unsigned long ulLoop;
    unsigned long ulDummy;

    //
    // Throw away any data that may be sitting in the receive FIFO.
    //
    SSIFlashRxFlush();

    //
    // Deassert the select for the SD card (in case it was previously asserted).
    //
    GPIOPinWrite(SDCARD_CS_BASE, SDCARD_CS_PIN, SDCARD_CS_PIN);

    //
    // Assert the chip select for the serial flash.
    //
    GPIOPinWrite(SFLASH_CS_BASE, SFLASH_CS_PIN, 0);

    //
    // Send the instruction byte and receive a dummy byte to pace the
    // transaction.
    //
    SSIDataPut(SFLASH_SSI_BASE, ucInstruction);
    SSIDataGet(SFLASH_SSI_BASE, &ulDummy);

    //
    // Send any optional bytes.
    //
    for(ulLoop = 0; ulLoop < (unsigned long)usLen; ulLoop++)
    {
        SSIDataPut(SFLASH_SSI_BASE, ucData[ulLoop]);
        SSIDataGet(SFLASH_SSI_BASE, &ulDummy);
    }
}

//*****************************************************************************
//
// Write additional data following an instruction to the serial flash.
//
// \param ucData is a pointer to data that will be sent to the device.
// \param usLen is the length of the data to send.
//
// This function writes a block of data to the serial flash device.  Typically
// this will be data to be written to a flash page.
//
// It is assumed that SSIFlashInstructionWrite() has previously been called to
// set the device chip select and send the initial instruction to the device.
//
//*****************************************************************************
static void
SSIFlashInstructionDataWrite(unsigned char *ucData, unsigned short usLen)
{
    unsigned long ulLoop;
    unsigned long ulDummy;

    //
    // Send the data to the device.
    //
    for(ulLoop = 0; ulLoop < (unsigned long)usLen; ulLoop++)
    {
        SSIDataPut(SFLASH_SSI_BASE, ucData[ulLoop]);
        SSIDataGet(SFLASH_SSI_BASE, &ulDummy);
    }
}

//*****************************************************************************
//
// Read data from the serial flash following the write portion of an
// instruction.
//
// \param ucData is a pointer to storage for the bytes read from the serial
// flash.
// \param ulLen is the number of bytes to read from the device.
//
// This function read a given number of bytes from the device.  It is assumed
// that the flash chip select is already asserted on entry and that an
// instruction has previously been written using a call to
// SSIFlashInstructionWrite().
//
//*****************************************************************************
static void
SSIFlashInstructionRead(unsigned char *ucData, unsigned long ulLen)
{
    unsigned long ulData;
    unsigned long ulLoop;

    //
    // For each requested byte...
    //
    for(ulLoop = 0; ulLoop < ulLen; ulLoop++)
    {
        //
        // Write dummy data.
        //
        SSIDataPut(SFLASH_SSI_BASE, 0xFF);

        //
        // Get a byte back from the SSI controller.
        //
        SSIDataGet(SFLASH_SSI_BASE, &ulData); /* read data frm rx fifo */

        //
        // Stash it in the caller's buffer.
        //
        ucData[ulLoop] = (unsigned char)(ulData & 0xFF);
    }
}

//*****************************************************************************
//
// Finish an instruction by deasserting the serial flash chip select.
//
// This function must be called following SSIFlashInstructionWrite() and,
// depending upon the instruction, SSIFlashInstructionRead() to complete the
// instruction by deasserting the chip select to the serial flash device.
//
//*****************************************************************************
static void
SSIFlashInstructionEnd(void)
{
    //
    // Pull CS high to deassert it and complete the previous instruction.
    //
    GPIOPinWrite(SFLASH_CS_BASE, SFLASH_CS_PIN, SFLASH_CS_PIN);
}

//*****************************************************************************
//
// Waits for the flash device to report that it is idle.
//
// \param ulMaxRetries is the maximum number of times the serial flash device
// should be polled before we give up and report an error.  If this value is
// 0, the function continues polling indefinitely.
//
// This function polls the serial flash device and returns when either the
// maximum number of polling attempts is reached or the device reports that it
// is no longer busy.
//
// \return Returns \e true if the device reports that it is idle before the
// specified number of polling attempts is exceeded or \e false otherwise.
//
//*****************************************************************************
static tBoolean
SSIFlashIdleWait(unsigned long ulMaxRetries)
{
    unsigned long ulDelay;
    tBoolean bBusy;

    //
    // Wait for the device to be ready to receive an instruction.
    //
    ulDelay = ulMaxRetries;

    do
    {
        bBusy = SSIFlashIsBusy();

        //
        // Increment our counter.  If we have waited too long, assume the
        // device is not present.
        //
        ulDelay--;
    }
    while(bBusy && (ulDelay || !ulMaxRetries));

    //
    // If we get here and we're still busy, we need to return false to indicate
    // a problem.
    //
    return(!bBusy);
}

//*****************************************************************************
//
// Sends a command to the flash to enable program and erase operations.
//
// This function sends a write enable instruction to the serial flash device
// in preparation for an erase or program operation.
//
// \return Returns \b true if the instruction was accepted or \b false if
// write operations could not be enabled.
//
//*****************************************************************************
static tBoolean
SSIFlashWriteEnable(void)
{
    tBoolean bRetcode;
    unsigned char ucStatus;

    //
    // Issue the instruction we need to write-enable the chip.
    //
    SSIFlashInstructionWrite(INSTR_WRITE_ENABLE, (unsigned char *)0, 0);
    SSIFlashInstructionEnd();

    //
    // Wait for the instruction to complete.
    //
    bRetcode = SSIFlashIdleWait(MAX_BUSY_POLL_IDLE);

    //
    // Is the flash idle?
    //
    if(bRetcode)
    {
        //
        // Read the status and make sure that the Write Enable Latch bit is
        // set (indicating that a write may proceed).
        //
        ucStatus = SSIFlashStatusGet();

        bRetcode = (ucStatus & STATUS_WRITE_ENABLE_LATCH) ? true : false;
    }

    //
    // Tell the caller how we got on.
    //
    return(bRetcode);
}

//*****************************************************************************
//
//! Initializes the SSI port and determines if the serial flash is available.
//!
//! This function must be called prior to any other function offered by the
//! serial flash driver.  It configures the SSI port to run in mode 0 at 10MHz
//! and queries the ID of the serial flash device to ensure that it is
//! available.
//!
//! \note SSI0 is shared between the serial flash and the SDCard on the
//! development board.  Two independent GPIOs are used to provide chip selects
//! for these two devices but care must be taken when using both in a single
//! application, especially during initialization of the SDCard when the SSI
//! clock rate must initially be set to 400KHz and later increased.  Since both
//! the SSI flash and SDCard drivers initialize the SSI peripheral, application
//! writers must be aware of the possible contention and ensure that they do
//! not allow the possibility of two different interrupt handlers or execution
//! threads from attempting to access both peripherals simultaneously.
//!
//! This driver assumes that the application is aware of the possibility of
//! contention and has been designed with this in mind.  Other than disabling
//! the SDCard when an attempt is made to access the serial flash, no code is
//! included here to arbitrate for ownership of the SSI peripheral.
//!
//! \return Returns \b true on success or \b false if an error is reported or
//! the expected serial flash device is not present.
//!
//*****************************************************************************
tBoolean
SSIFlashInit(void)
{
    tBoolean bRetcode;
    unsigned char ucManufacturer, ucDevice;

    //
    // Enable the peripherals used to drive the SDC on SSI.
    //
    SysCtlPeripheralEnable(SFLASH_SSI_PERIPH);
    SysCtlPeripheralEnable(SFLASH_SSI_GPIO_PERIPH);

    //
    // Configure the appropriate pins to be SSI instead of GPIO. The CS
    // is configured as GPIO since this board has 2 devices connected to
    // SSI0 and each uses a separate CS.
    //
    GPIOPinTypeSSI(SFLASH_SSI_GPIO_BASE, SFLASH_SSI_PINS);
    GPIOPinTypeGPIOOutput(SFLASH_CS_BASE, SFLASH_CS_PIN);
    GPIOPinTypeGPIOOutput(SDCARD_CS_BASE, SDCARD_CS_PIN);
    GPIOPadConfigSet(SFLASH_SSI_GPIO_BASE, SFLASH_SSI_PINS, GPIO_STRENGTH_4MA,
                     GPIO_PIN_TYPE_STD_WPU);

    //
    // Deassert the SSI0 chip selects for both the SD card and serial flash
    //
    GPIOPinWrite(SDCARD_CS_BASE, SDCARD_CS_PIN, SDCARD_CS_PIN);
    GPIOPinWrite(SFLASH_CS_BASE, SFLASH_CS_PIN, SFLASH_CS_PIN);

    //
    // Configure the SSI0 port for 10MHz operation
    //
    SSIConfigSetExpClk(SFLASH_SSI_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 10000000, 8);
    SSIEnable(SFLASH_SSI_BASE);

    //
    // Wait for the device to be ready to receive an instruction.
    //
    bRetcode = SSIFlashIdleWait(MAX_BUSY_POLL_IDLE);

    //
    // If the device didn't report that it was idle, assume it isn't there
    // and return an error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Read the device ID and check to see that it is one we recognize.
    //
    bRetcode = SSIFlashIDGet(&ucManufacturer, &ucDevice);
    if(!bRetcode || (ucManufacturer != MANUFACTURER_WINBOND) ||
       (ucDevice != DEVICE_ID_W25X80A))
    {
        //
        // This is not a device we recognize so return an error.
        //
        return(false);
    }
    else
    {
        //
        // All is well.
        //
        return(true);
    }
}

//*****************************************************************************
//
//! Determines if the serial flash is able to accept a new instruction.
//!
//! This function reads the serial flash status register and determines whether
//! or not the device is currently busy.  No new instruction may be issued to
//! the device if it is busy.
//!
//! \return Returns \b true if the device is busy and unable to receive a new
//! instruction or \b false if it is idle.
//
//*****************************************************************************
tBoolean
SSIFlashIsBusy(void)
{
    unsigned char ucStatus;

    //
    // Get the flash status.
    //
    ucStatus = SSIFlashStatusGet();

    //
    // Is the busy bit set?
    //
    return((ucStatus & STATUS_BUSY) ? true : false);
}

//*****************************************************************************
//
//! Returns the manufacturer and device IDs for the attached serial flash.
//!
//! \param pucManufacturer points to storage which will be written with the
//! manufacturer ID of the attached serial flash device.
//! \param pucDevice points to storage which will be written with the device
//! ID of the attached serial flash device.
//!
//! This function may be used to determine the manufacturer and device IDs
//! of the attached serial flash device.
//!
//! \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
SSIFlashIDGet(unsigned char *pucManufacturer, unsigned char *pucDevice)
{
    tBoolean bRetcode;
    unsigned char pucBuffer[3];

    //
    // Wait for the flash to be idle.
    //
    bRetcode = SSIFlashIdleWait(MAX_BUSY_POLL_IDLE);

    //
    // If the device is not idle, return an error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Now perform the instruction we need to read the IDs.
    //
    pucBuffer[0] = 0;
    pucBuffer[1] = 0;
    pucBuffer[2] = 0;
    SSIFlashInstructionWrite(INSTR_MAN_DEV_ID, pucBuffer, 3);
    SSIFlashInstructionRead(pucBuffer, 2);
    SSIFlashInstructionEnd();

    //
    // Copy the returned IDs into the caller's storage.
    //
    *pucManufacturer = pucBuffer[0];
    *pucDevice = pucBuffer[1];

    //
    // Tell the caller everything went well.
    //
    return(true);
}

//*****************************************************************************
//
//! Returns the size of a sector for this device.
//!
//! This function returns the size of an erasable sector for the serial flash
//! device.  All addresses passed to SSIFlashSectorErase() must be aligned on
//! a sector boundary.
//!
//! \return Returns the number of bytes in a sector.
//
//*****************************************************************************
unsigned long
SSIFlashSectorSizeGet(void)
{
    //
    // This device supports 4KB sectors.
    //
    return(W25X80A_SECTOR_SIZE);
}

//*****************************************************************************
//
//! Returns the size of a block for this device.
//!
//! This function returns the size of an erasable block for the serial flash
//! device.  All addresses passed to SSIFlashBlockErase() must be aligned on
//! a block boundary.
//!
//! \return Returns the number of bytes in a block.
//
//*****************************************************************************
unsigned long
SSIFlashBlockSizeGet(void)
{
    //
    // This device support 64KB blocks.
    //
    return(W25X80A_BLOCK_SIZE);
}

//*****************************************************************************
//
//! Returns the total amount of storage offered by this device.
//!
//! This function returns the size of the programmable area provided by the
//! attached SSI flash device.
//!
//! \return Returns the number of bytes in the device.
//
//*****************************************************************************
unsigned long
SSIFlashChipSizeGet(void)
{
    //
    // This device is 1MB in size.
    //
    return(W25X80A_CHIP_SIZE);
}

//*****************************************************************************
//
//! Erases the contents of a single serial flash sector.
//!
//! \param ulAddress is the start address of the sector which is to be erased.
//! This value must be an integer multiple of the sector size returned
//! by SSIFlashSectorSizeGet().
//! \param bSync should be set to \b true if the function is to block until
//! the erase operation is complete or \b false to return immediately after
//! the operation is started.
//!
//! This function erases a single sector of the serial flash, returning all
//! bytes in that sector to their erased value of 0xFF.  The sector size and,
//! hence, start address granularity can be determined by calling
//! SSIFlashSectorSizeGet().
//!
//! The function may be synchronous (\e bSync set to \b true) or asynchronous
//! (\e bSync set to \b false).  If asynchronous, the caller is responsible for
//! ensuring that no further serial flash operations are requested until the
//! erase operation has completed.  The state of the device may be queried by
//! calling SSIFlashIsBusy().
//!
//! Three options for erasing are provided. Sectors provide the smallest
//! erase granularity, blocks provide the option of erasing a larger section of
//! the device in one operation and, finally, the whole device may be erased
//! in a single operation via SSIFlashChipErase().
//!
//! \note This operation will take between 120mS and 250mS to complete. If the
//! \e bSync parameter is set to \b true, this function will, therefore, not
//! not return for a significant period of time.
//!
//! \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
SSIFlashSectorErase(unsigned long ulAddress, tBoolean bSync)
{
    tBoolean bRetcode;
    unsigned char pucBuffer[3];

    //
    // Make sure the address passed is aligned correctly.
    //
    if(ulAddress & (W25X80A_SECTOR_SIZE - 1))
    {
        //
        // Oops - it's not on a sector boundary so fail the call.
        //
        return(false);
    }

    //
    // Wait for the flash to be idle.
    //
    bRetcode = SSIFlashIdleWait(0);

    //
    // If the device is not idle, return an error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Enable write operations.
    //
    bRetcode = SSIFlashWriteEnable();

    //
    // If write operations could not be enabled, return the error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Now perform the instruction we need to erase the sector.
    //
    pucBuffer[0] = (unsigned char)((ulAddress >> 16) & 0xFF);
    pucBuffer[1] = (unsigned char)((ulAddress >> 8) & 0xFF);
    pucBuffer[2] = (unsigned char)(ulAddress & 0xFF);
    SSIFlashInstructionWrite(INSTR_SECTOR_ERASE, pucBuffer, 3);
    SSIFlashInstructionEnd();

    //
    // Wait for the instruction to complete if the function is being called
    // synchronously.
    //
    if(bSync)
    {
        bRetcode = SSIFlashIdleWait(MAX_BUSY_POLL_ERASE_SECTOR);
    }

    //
    // Tell the caller how things went.
    //
    return(bRetcode);
}

//*****************************************************************************
//
//! Erases the contents of a single serial flash block.
//!
//! \param ulAddress is the start address of the block which is to be erased.
//! This value must be an integer multiple of the block size returned
//! by SSIFlashBlockSizeGet().
//! \param bSync should be set to \b true if the function is to block until
//! the erase operation is complete or \b false to return immediately after
//! the operation is started.
//!
//! This function erases a single block of the serial flash, returning all
//! bytes in that block to their erased value of 0xFF.  The block size and,
//! hence, start address granularity can be determined by calling
//! SSIFlashBlockSizeGet().
//!
//! The function may be synchronous (\e bSync set to \b true) or asynchronous
//! (\e bSync set to \b false).  If asynchronous, the caller is responsible for
//! ensuring that no further serial flash operations are requested until the
//! erase operation has completed.  The state of the device may be queried by
//! calling SSIFlashIsBusy().
//!
//! Three options for erasing are provided. Sectors provide the smallest
//! erase granularity, blocks provide the option of erasing a larger section of
//! the device in one operation and, finally, the whole device may be erased
//! in a single operation via SSIFlashChipErase().
//!
//! \note This operation will take between 400mS and 1000mS to complete.  If the
//! \e bSync parameter is set to \b true, this function will, therefore, not
//! not return for a significant period of time.
//!
//! \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
SSIFlashBlockErase(unsigned long ulAddress, tBoolean bSync)
{
    tBoolean bRetcode;
    unsigned char pucBuffer[3];

    //
    // Make sure the address passed is aligned correctly.
    //
    if(ulAddress & (W25X80A_BLOCK_SIZE - 1))
    {
        //
        // Oops - it's not on a block boundary so fail the call.
        //
        return(false);
    }

    //
    // Wait for the flash to be idle.
    //
    bRetcode = SSIFlashIdleWait(0);

    //
    // If the device is not idle, return an error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Enable write operations.
    //
    bRetcode = SSIFlashWriteEnable();

    //
    // If write operations could not be enabled, return the error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Now perform the instruction we need to erase the block.
    //
    pucBuffer[0] = (unsigned char)((ulAddress >> 16) & 0xFF);
    pucBuffer[1] = (unsigned char)((ulAddress >> 8) & 0xFF);
    pucBuffer[2] = (unsigned char)(ulAddress & 0xFF);
    SSIFlashInstructionWrite(INSTR_BLOCK_ERASE, pucBuffer, 3);
    SSIFlashInstructionEnd();

    //
    // Wait for the instruction to complete if the function is being called
    // synchronously.
    //
    if(bSync)
    {
        bRetcode = SSIFlashIdleWait(MAX_BUSY_POLL_ERASE_BLOCK);
    }

    //
    // Tell the caller how things went.
    //
    return(bRetcode);
}

//*****************************************************************************
//
//! Erases the entire serial flash device.
//!
//! \param bSync should be set to \b true if the function is to block until
//! the erase operation is complete or \b false to return immediately after
//! the operation is started.
//!
//! This function erases the entire serial flash device, returning all
//! bytes in the device to their erased value of 0xFF.
//!
//! The function may be synchronous (\e bSync set to \b true) or asynchronous
//! (\e bSync set to \b false).  If asynchronous, the caller is responsible for
//! ensuring that no further serial flash operations are requested until the
//! erase operation has completed.  The state of the device may be queried by
//! calling SSIFlashIsBusy().
//!
//! \note This operation will take between 6 and 10 seconds to complete.  If the
//! \e bSync parameter is set to \b true, this function will, therefore, not
//! not return for a significant period of time.
//!
//! \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
SSIFlashChipErase(tBoolean bSync)
{
    tBoolean bRetcode;

    //
    // Wait for the flash to be idle.
    //
    bRetcode = SSIFlashIdleWait(0);

    //
    // If the device is not idle, return an error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Enable write operations.
    //
    bRetcode = SSIFlashWriteEnable();

    //
    // If write operations could not be enabled, return the error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Now perform the instruction we need to erase the chip.
    //
    SSIFlashInstructionWrite(INSTR_CHIP_ERASE, (unsigned char *)0, 0);
    SSIFlashInstructionEnd();

    //
    // Wait for the instruction to complete if the function is being called
    // synchronously.
    //
    if(bSync)
    {
        bRetcode = SSIFlashIdleWait(MAX_BUSY_POLL_ERASE_CHIP);
    }

    //
    // Tell the caller how things went.
    //
    return(bRetcode);
}

//*****************************************************************************
//
//! Reads a block of serial flash into a buffer.
//!
//! \param ulAddress is the serial flash address of the first byte to be read.
//! \param ulLength is the number of bytes of data to read.
//! \param pucDst is a pointer to storage for the data read.
//!
//! This function reads a contiguous block of data from a given address in the
//! serial flash device into a buffer supplied by the caller.
//!
//! \return Returns the number of bytes read.
//
//*****************************************************************************
unsigned long
SSIFlashRead(unsigned long ulAddress, unsigned long ulLength,
             unsigned char *pucDst)
{
    tBoolean bRetcode;
    unsigned char pucBuffer[3];

    //
    // Wait for the flash to be idle.
    //
    bRetcode = SSIFlashIdleWait(0);

    //
    // If the device is not idle, return an error.
    //
    if(!bRetcode)
    {
        return(0);
    }

    //
    // Send the read instruction and start address.
    //
    pucBuffer[0] = (unsigned char)((ulAddress >> 16) & 0xFF);
    pucBuffer[1] = (unsigned char)((ulAddress >> 8) & 0xFF);
    pucBuffer[2] = (unsigned char)(ulAddress & 0xFF);
    SSIFlashInstructionWrite(INSTR_READ_DATA, pucBuffer, 3);

    //
    // Read back the data.
    //
    SSIFlashInstructionRead(pucDst, ulLength);

    //
    // Tell the flash device we have read everything we need.
    //
    SSIFlashInstructionEnd();

    //
    // Return the number of bytes read.
    //
    return(ulLength);
}

//*****************************************************************************
//
//! Writes a block of data to the serial flash device.
//!
//! \param ulAddress is the first serial flash address to be written.
//! \param ulLength is the number of bytes of data to write.
//! \param pucSrc is a pointer to the data which is to be written.
//!
//! This function writes a block of data into the serial flash device at the
//! given address.  It is assumed that the area to be written has previously
//! been erased.
//!
//! \return Returns the number of bytes written.
//
//*****************************************************************************
unsigned long
SSIFlashWrite(unsigned long ulAddress, unsigned long ulLength,
              unsigned char *pucSrc)
{
    tBoolean bRetcode;
    unsigned long ulLeft, ulStart, ulPageLeft;
    unsigned char pucBuffer[3];
    unsigned char *pucStart;

    //
    // Wait for the flash to be idle.
    //
    bRetcode = SSIFlashIdleWait(MAX_BUSY_POLL_IDLE);

    //
    // If the device is not idle, return an error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Get set up to start writing pages.
    //
    ulStart = ulAddress;
    ulLeft = ulLength;
    pucStart = pucSrc;

    //
    // Keep writing pages until we have nothing left to write.
    //
    while(ulLeft)
    {
        //
        // How many bytes do we have to write in the current page?
        //
        ulPageLeft = W25X80A_PAGE_SIZE - (ulStart & (W25X80A_PAGE_SIZE - 1));

        //
        // How many bytes can we write in the current page?
        //
        ulPageLeft = (ulPageLeft >= ulLeft) ? ulLeft : ulPageLeft;

        //
        // Enable write operations.
        //
        if(!SSIFlashWriteEnable())
        {
            //
            // If we can't write enable the device, exit, telling the caller
            // how many bytes we've already written.
            //
            return(ulLength - ulLeft);
        }

        //
        // Write a chunk of data into one flash page.
        //
        pucBuffer[0] = (unsigned char)((ulStart >> 16) & 0xFF);
        pucBuffer[1] = (unsigned char)((ulStart >> 8) & 0xFF);
        pucBuffer[2] = (unsigned char)(ulStart & 0xFF);
        SSIFlashInstructionWrite(INSTR_PAGE_PROGRAM, pucBuffer, 3);
        SSIFlashInstructionDataWrite(pucStart, ulPageLeft);
        SSIFlashInstructionEnd();

        //
        // Wait for the page write to complete.
        //
        bRetcode = SSIFlashIdleWait(MAX_BUSY_POLL_PROGRAM_PAGE);
        if(!bRetcode)
        {
            //
            // If we timed out waiting for the program operation to finish,
            // exit, telling the caller how many bytes we've already written.
            //
            return(ulLength - ulLeft);
        }

        //
        // Update our pointers and counters for the next page.
        //
        ulLeft -= ulPageLeft;
        ulStart += ulPageLeft;
        pucStart += ulPageLeft;
    }

    //
    // If we get here, all is well and we wrote all the required data.
    //
    return(ulLength);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
