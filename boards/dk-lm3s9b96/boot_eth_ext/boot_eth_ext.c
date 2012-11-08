//*****************************************************************************
//
// boot_eth_ext.c - External flash Ethernet boot loader description.
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

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/flash.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "drivers/set_pinout.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/extflash.h"
#include "bl_config.h"
#include "boot_loader/bl_check.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet Boot Loader for External Flash(boot_eth_ext)</h1>
//!
//! The boot loader is a piece of code that can be programmed at the
//! beginning of internal flash to act as an application loader as well as an
//! update mechanism for an application running on a Stellaris microcontroller,
//! utilizing either UART0, I2C0, SSI0, or Ethernet.  The capabilities of the
//! boot loader are configured via the bl_config.h include file.  For this
//! example, the boot loader uses Ethernet to load an application into external
//! flash and runs it from there.  The boot loader itself is the only
//! application running in internal flash in this example.
//!
//! The configuration is set to boot applications which are linked to run from
//! address 0x60000000 (EPI-connected external flash) and requires that the
//! SRAM/Flash Daughter Board be installed.  If the daughter board is not
//! present, the boot loader will warn the user via a message on the display.
//!
//! Note that execution from external flash should be avoided if at all
//! possible due to significantly lower performance than achievable from
//! internal flash. Using an 8-bit wide interface to flash as found on the
//! Flash/SRAM/LCD daughter board and remembering that an external memory
//! access via EPI takes 8 or 9 system clock cycles, a program running from
//! off-chip memory will typically run at approximately 5% of the speed of the
//! same program in internal flash.
//
//*****************************************************************************

//*****************************************************************************
//
// Global flag used to keep track of whether or not the SRAM/flash daughter
// board is present.
//
//*****************************************************************************
static tBoolean g_bFlashPresent = false;

//*****************************************************************************
//
// Global flag used to hold an indication of errors during erase and
// programming.
//
//*****************************************************************************
static tBoolean g_bError = false;

//*****************************************************************************
//
// Graphics context used to access the display.
//
//*****************************************************************************
static tContext g_sContext;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    //
    // Hang on ABORT failure.
    //
    while(1)
    {
    }
}
#endif

//*****************************************************************************
//
// Determines whether or not to force a firmware update.
//
// This function is called by the boot loader early in the startup sequence to
// determine whether or not a valid application image exists and, if it does,
// whether to branch to it or remain within the boot loader waiting for a
// firmware image upload.  The function returns 1 to indicate that the boot
// loader should retain control or 0 to indicate that a valid application image
// was found and that it should be booted.
//
// In this implementation, a rather simple test is used to determine validity
// of the main application image.  The image is assumed to exist and be valid
// if the first word at APP_START_ADDRESS is a valid pointer to a location in
// SRAM (and, hence, likely to be a good stack pointer) and the second word is
// a pointer to a location in external flash and ends in a 1 (making it a
// likely candidate for a valid reset vector).
//
// If a valid image is found, the function also checks the state of the GPIO
// pin connected to the user button on the development board.  If this
// indicates that the button is pressed, the boot loader is told to retain
// control, thus providing the user a method of preventing the main image from
// being booted.
//
//*****************************************************************************
unsigned long
BootExtCheckUpdate(void)
{
    unsigned long *pulApp;
    unsigned long ulRetcode;

    //
    // If the flash is present, check to see if there appears to be a valid
    // image in it.
    //
    if(g_bFlashPresent)
    {
        //
        // See if the first location is 0xfffffffff or something that does not
        // look like a stack pointer, or if the second location is 0xffffffff or
        // something that does not look like a reset vector.  This
        // implementation assumes that the stack pointer may be in internal
        // or external SRAM and that the application will be in external flash.
        //
        pulApp = (unsigned long *)APP_START_ADDRESS;
        if((pulApp[0] == 0xffffffff) ||
           (((pulApp[0] & 0xfff00000) != 0x20000000) &&
            ((pulApp[0] & 0xfff00000) != EXT_SRAM_BASE))||
           (pulApp[1] == 0xffffffff) ||
           ((pulApp[1] & 0xff800001) != (EXT_FLASH_BASE + 1)))
        {
            UARTprintf("No valid app found in external flash.\n");
            UARTprintf("Stack ptr 0x%08x, Entry address 0x%08x\n",
                       pulApp[0], pulApp[1]);
            return(1);
        }
    }
    else
    {
        UARTprintf("No SRAM/Flash daughter board detected!\n");
        return(1);
    }

    //
    // Check to see if the user button is being pressed and, if it is,
    // don't boot the main application even if one exists.
    //
    ulRetcode = CheckGPIOForceUpdate();

    //
    // Was the user pressing the button?
    //
    if(ulRetcode)
    {
        //
        // Yes - remain in the boot loader waiting for an update.
        //
        UARTprintf("Forcing boot loader update.\n");
    }
    else
    {
        //
        // No - go ahead and boot the existing application image.
        //
        UARTprintf("Booting existing app from external flash.\n");

        //
        // Wait for the string to clear the UART before we return.  Without this,
        // any clock change in the main app will mess up the output.
        //
        while(UARTBusy(UART0_BASE))
        {
        }
    }

    return(ulRetcode);
}

//*****************************************************************************
//
// Low level hardware initialization for the boot_eth_ext boot loader.
//
// This function is called by the boot loader immediately after it relocates
// itself to SRAM.  It is responsible for performing any
// implementation-specific low level hardware initialization.  In this case,
// the system clock is set to the same rate that the Ethernet boot loader is
// using, configure the device pinout appropriately for the development board,
// and configure the EPI to allow access to daughter board flash and SRAM.
//
//*****************************************************************************
void
BootExtHwInit(void)
{
    //
    // Set the system to run at 12.5MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_16 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the device pinout appropriately for this application.
    //
    PinoutSet();

    //
    // Check to make sure that the external flash is present.
    //
    g_bFlashPresent = ExtFlashPresent();

    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Set GPIO A0 and A1 as UART.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1 | GPIO_PIN_0);

    //
    // Initialize UART0 for output.
    //
    UARTStdioInit(0);

    //
    // Tell the user what's up.
    //
    UARTprintf("External flash boot loader running\n");
}

//*****************************************************************************
//
// Application-specific initialization for the boot_eth_ext boot loader.
//
// This function is called by the boot loader after it initializes the
// communication channel to be used and after the system clock has been
// configured.  It is responsible for performing any high-level initialization
// that the boot loader implementation may require.  In this case, we configure
// the UART to allow us to display status messages to the user.
//
//*****************************************************************************
void
BootExtInit(void)
{
    tRectangle sRect;
    unsigned char pucMACAddr[6];
    unsigned long ulUser0, ulUser1;
    char pcBuffer[32];

    UARTprintf("Configuration completed.\n");

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
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
    GrStringDrawCentered(&g_sContext, "boot-eth-ext", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 10, 0);

    //
    // Tell the user what's happening.
    //
    GrContextFontSet(&g_sContext, g_pFontCmss16b);
    GrStringDrawCentered(&g_sContext, "External Flash Boot Loader is running.",
                         -1, GrContextDpyWidthGet(&g_sContext) / 2,
                         (GrContextDpyHeightGet(&g_sContext) / 2) - 16, 0);
    GrStringDrawCentered(&g_sContext, "Waiting for connection...",
                         -1, GrContextDpyWidthGet(&g_sContext) / 2,
                         (GrContextDpyHeightGet(&g_sContext) / 2) + 16, 0);

    //
    // Get the MAC address from the user registers in NV ram.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV ram into a MAC address
    // array.
    //
    pucMACAddr[0] = ulUser0 & 0xff;
    pucMACAddr[1] = (ulUser0 >> 8) & 0xff;
    pucMACAddr[2] = (ulUser0 >> 16) & 0xff;
    pucMACAddr[3] = ulUser1 & 0xff;
    pucMACAddr[4] = (ulUser1 >> 8) & 0xff;
    pucMACAddr[5] = (ulUser1 >> 16) & 0xff;

    //
    // Display the MAC address (so that the user can perform a firmware update
    // if required).
    //
    usprintf(pcBuffer,
             "MAC: %02x-%02x-%02x-%02x-%02x-%02x",
             pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
             pucMACAddr[4], pucMACAddr[5]);
    GrStringDrawCentered(&g_sContext, pcBuffer, -1,
                         GrContextDpyWidthGet(&g_sContext) / 2,
                         GrContextDpyHeightGet(&g_sContext) - 20, 0);
}

//*****************************************************************************
//
// Application-specific reinitialization for the boot_eth_ext boot loader.
//
// This function is called when the boot loader is entered via an SVC call from
// a running application. The call is made after the boot loader initializes the
// communication channel to be used and after the system clock has been
// reconfigured.  It is responsible for performing any board-specific
// initialization that the boot loader implementation may require.  In this
// case, we ensure that the device pinout is set correctly for the application
// and configure the UART to allow us to display status messages to the user.
//
//*****************************************************************************
void
BootExtReinit(void)
{
    //
    // Perform the standard initialization steps for this implementation.
    //
    BootExtInit();
}

//*****************************************************************************
//
// Boot loader callback hook called when a new download is about to begin.
//
//*****************************************************************************
void
BootExtStart(void)
{
    UARTprintf("Download starting...\n");

    GrStringDrawCentered(&g_sContext, "    Download in progress    ",
                         -1, GrContextDpyWidthGet(&g_sContext) / 2,
                         (GrContextDpyHeightGet(&g_sContext) / 2) + 16, 1);

}

//*****************************************************************************
//
// Boot loader callback hook called during a download to provide information
// on progress.
//
//*****************************************************************************
void
BootExtProgress(unsigned long ulCompleted, unsigned long ulTotal)
{
    if(ulTotal)
    {
        UARTprintf("Completed 0x%08x bytes of 0x%08x\r", ulCompleted, ulTotal);
    }
    else
    {
        UARTprintf("Completed 0x%08x bytes\r", ulCompleted);
    }
}

//*****************************************************************************
//
// Boot loader callback hook called following successful completion of a
// firmware download.  On return from this function, the system will typically
// be reset.
//
//*****************************************************************************
void
BootExtEnd(void)
{
    tRectangle sRect;

    //
    // Output some status via the serial port.
    //
    UARTprintf("\nDownload completed.\n");

    //
    // Clear the screen in preparation for a reboot.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);
}

//*****************************************************************************
//
// Returns the address of the first byte after the end of flash.
//
//*****************************************************************************
unsigned long
BootExtEndAddrGet(void)
{
    return(0x60000000 + ExtFlashChipSizeGet());
}

//*****************************************************************************
//
// Erases a single block of external flash.
//
// \param ulAddress is the address of the block of flash to erase.
//
// This function is called to erase a single block of the flash, blocking
// until the erase has completed.  The block size of external flash is,
// defined using FLASH_PAGE_SIZE in bl_config.h.  Since our external flash
// contains page of different sizes, we set this to 64KB which represents
// the largest page in the device then erase multiple pages if necessary to
// make the boot/parameter block area appear to have 64KB pages too.
//
// \return None
//
//*****************************************************************************
void
ExternalFlashErase(unsigned long ulAddress)
{
    unsigned long ulSize, ulStartAddr;
    long lEraseSize;

    //
    // We always erase FLASH_PAGE_SIZE bytes but, given that the target device
    // may have boot/parameter blocks which are smaller, we may need to erase
    // more than 1 block to accomplish this.
    //
    lEraseSize = FLASH_PAGE_SIZE;

    while(!g_bError && (lEraseSize > 0))
    {
        //
        // Determine the start address of the block containing the passed address.
        //
        ulSize = ExtFlashBlockSizeGet(ulAddress, &ulStartAddr);

        //
        // Erase this block.
        //
        if(ulSize && ulStartAddr)
        {
            UARTprintf("Erasing %dKB flash block at 0x%08x.\n", (ulSize / 1024),
                       ulAddress);
            g_bError |= !ExtFlashBlockErase(ulStartAddr, true);
        }
        else
        {
            g_bError = true;
        }

        if(g_bError)
        {
            UARTprintf("Error reported erasing flash block.\n");
        }

        //
        // Move on to the next block.
        //
        lEraseSize -= (long)ulSize;
        ulAddress = ulStartAddr + ulSize;
    }
}

//*****************************************************************************
//
// Writes a block of downloaded data to the external flash.
//
// \param ulDstAddr is the flash start address for the write operation.
// \param pucSrcData is a pointer to the first byte of the data to write.
// \param ulLength is the number of bytes of data to write.
//
// This function is called to write a block of data to the external flash.  It
// is assumed that the block has previously been erased.  If any error is
// reported during the operation, the global error flag is set.  The caller
// may query the error status using function ExternalFlashErrorCheck().
//
// \return None
//
//*****************************************************************************
void
ExternalFlashProgram(unsigned long ulDstAddr, unsigned char *pucSrcData,
                     unsigned long ulLength)
{
    unsigned long ulWritten;

    UARTprintf("Programming %d bytes from 0x%08x to 0x%08x.\n",
               ulLength, pucSrcData, ulDstAddr);

    //
    // Pass this request to the low level driver.
    //
    ulWritten = ExtFlashWrite(ulDstAddr, ulLength, pucSrcData);

    if(ulWritten != ulLength)
    {
        UARTprintf("Only wrote %d bytes!\n", ulWritten);
    }

    //
    // Was the write successful?
    //
    g_bError |= (ulWritten != ulLength);
}

//*****************************************************************************
//
// Determines whether or not an address and image size is valid for the target
// flash device.
//
// \param ulAddr is the requested flash address for the downloaded image.
// \param ulImgSize is the size of the image that is to be stored.
//
// This function is called by the boot loader to determine whether a downloaded
// image may be written at a particular address in the target flash device.
//
// \return Returns 0 if the size and/or address is invalid for the target
// device or 1 if they are valid.
//
//*****************************************************************************
unsigned long
ExternalFlashStartAddrCheck(unsigned long ulAddr, unsigned long ulImgSize)
{
    unsigned long ulChipSize, ulSize, ulStartAddr;

    //
    // How much storage is there in the flash chip?
    //
    ulChipSize = ExtFlashChipSizeGet();

    //
    // The address passed is somewhere inside the flash device.  Now get the
    // size and address of the block containing the address.
    //
    ulSize = ExtFlashBlockSizeGet(ulAddr, &ulStartAddr);

    //
    // If the address passed is not on the block boundary or the address is
    // not valid, don't allow the update.
    //
    if(!ulSize || (ulAddr != ulStartAddr))
    {
        return(0);
    }

    //
    // Will the image fit in the device?
    //
    if(ulImgSize > (ulChipSize - (ulAddr - EXT_FLASH_BASE)))
    {
        //
        // No - the image is too large to write at the provided address.
        //
        return(0);
    }

    //
    // At this point, all is well so tell the caller.
    //
    return(1);
}

//*****************************************************************************
//
// Determines whether any erase or programming error has occured.
//
// This function is called by the boot loader to determine whether or not an
// error has occured in an erase or program operation.  Any error condition
// remains set until ExternalFlashErrorClear() is called.
//
// \return Returns 1 if an error has occured or 0 otherwise.
//
//*****************************************************************************
unsigned long
ExternalFlashErrorCheck(void)
{
    return(g_bError);
}

//*****************************************************************************
//
// Clears the flash error flag.
//
// This function is called by the boot loader to clear the flag indicating
// whether or not an error has occured in an erase or program operation.  The
// current state of the error flag may be queried by calling function
// ExternalFlashErrorCheck().
//
// \return None.
//
//*****************************************************************************
void
ExternalFlashErrorClear(void)
{
    g_bError = false;
}
