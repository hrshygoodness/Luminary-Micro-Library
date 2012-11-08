//*****************************************************************************
//
// dbeeprom.c - Read and write the daughter board ID EEPROM.
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
// This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include <stddef.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/epi.h"
#include "driverlib/uart.h"
#include "driverlib/i2c.h"
#include "driverlib/rom.h"
#include "driverlib/debug.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "drivers/set_pinout.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "utils/cmdline.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Daughter Board EEPROM Read/Write Example (dbeeprom)</h1>
//!
//! This application may be used to read and write the ID structures written
//! into the 128 byte EEPROMs found on the optional Flash/SRAM and FPGA
//! daughter boards for the development board.  A command line interface is
//! provided via UART 0 and commands allow the existing ID EEPROM content to be
//! read and one of the standard structures identifying the available daughter
//! boards to be written to the device.
//!
//! The ID EEPROM is read in function PinoutSet() and used to configure the
//! EPI interface appropriately for the attached daughter board.  If the EEPROM
//! content is incorrect, this auto-configuration will not be possible and
//! example applications will typically show merely a blank display when run.
//
//*****************************************************************************

//*****************************************************************************
//
// Daughter board ID information blocks to be written to EEPROM.
//
//*****************************************************************************
const tDaughterIDInfo g_psIDBlock[] =
{
    {
        //
        // SRAM/Flash daughter board.
        //
        {'I', 'D'}, 0, 0, DAUGHTER_SRAM_FLASH, 1, EPI_MODE_HB8, 0xFFFFFFFF,
        20, 20, 90, 90, 0, 0, (EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6), 0,
        0, 0, 0, 0, (EPI_HB8_MODE_ADMUX | EPI_HB8_WORD_ACCESS), {0}
    },
    {
         //
         // FPGA daughter board.
         //
         {'I', 'D'}, 0, 0, DAUGHTER_FPGA, 1, EPI_MODE_GENERAL, 0xBBFFFFFF,
         25, 25, 0, 0, 0, 0, (EPI_ADDR_PER_SIZE_64KB | EPI_ADDR_PER_BASE_A),
         0, 0, 0, 0, 0, (EPI_GPMODE_DSIZE_16 | EPI_GPMODE_ASIZE_12 |
         EPI_GPMODE_WORD_ACCESS | EPI_GPMODE_READWRITE | EPI_GPMODE_READ2CYCLE |
         EPI_GPMODE_CLKPIN | EPI_GPMODE_RDYEN), {0}
    },
    {
         //
         // EM2 daughter board (which doesn't use EPI at all)
         //
         {'I', 'D'}, 0, 0, DAUGHTER_EM2, 1, EPI_MODE_DISABLE, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, {0}
    }
};


const char *g_pcIDNames[] =
{
    "Flash/SRAM/LCD",
    "FPGA/Camera",
    "EM2 Dual EM Socket Adapter"
};

#define NUM_DAUGHTER_STRUCTS (sizeof(g_psIDBlock) / sizeof(tDaughterIDInfo))

//*****************************************************************************
//
// I2C address for the EEPROM device used on DK daughter boards to provide
// an ID to applications.
//
//*****************************************************************************
#define ID_I2C_ADDR                0x50

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)

//*****************************************************************************
//
// Additional command line handler return codes.
//
//*****************************************************************************
#define COMMAND_OK              0
#define COMMAND_TOO_FEW_ARGS    -10
#define COMMAND_INVALID_ARG     -11

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];

//*****************************************************************************
//
// A global system tick counter.
//
//*****************************************************************************
static volatile unsigned long g_ulSysTickCount = 0;

static tBoolean
WaitI2CFinished(void)
{
    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(I2C0_MASTER_BASE, false) == 0)
    {
    }

    if(I2CMasterErr(I2C0_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        ROM_I2CMasterIntClear(I2C0_MASTER_BASE);
        return(false);
    }

    //
    // Clear any interrupts set.
    //
    while(ROM_I2CMasterIntStatus(I2C0_MASTER_BASE, false))
    {
        ROM_I2CMasterIntClear(I2C0_MASTER_BASE);
    }

    return(true);
}

//*****************************************************************************
//
// Reads from the I2C-attached EEPROM device.
//
// \param pucData points to storage for the data read from the EEPROM.
// \param ulOffset is the EEPROM address of the first byte to read.
// \param ulCount is the number of bytes of data to read from the EEPROM.
//
// This function reads one or more bytes of data from a given address in the
// ID EEPROM found on several of the development kit daughter boards.  The
// return code indicates whether the read was successful.
//
// \return Returns \b true on success of \b false on failure.
//
//*****************************************************************************
static tBoolean
EEPROMReadPolled(unsigned char *pucData, unsigned long ulOffset,
                 unsigned long ulCount)
{
    unsigned long ulToRead;

    //
    // Clear any previously signalled interrupts.
    //
    ROM_I2CMasterIntClear(I2C0_MASTER_BASE);

    //
    // Start with a dummy write to get the address set in the EEPROM.
    //
    ROM_I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, ID_I2C_ADDR, false);

    //
    // Place the address to be written in the data register.
    //
    ROM_I2CMasterDataPut(I2C0_MASTER_BASE, ulOffset);

    //
    // Perform a single send, writing the address as the only byte.
    //
    ROM_I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait until the current byte has been transferred.
    //
    if(!WaitI2CFinished())
    {
        return(false);
    }

    //
    // Put the I2C master into receive mode.
    //
    ROM_I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, ID_I2C_ADDR, true);

    //
    // Start the receive.
    //
    ROM_I2CMasterControl(I2C0_MASTER_BASE,
                         ((ulCount > 1) ? I2C_MASTER_CMD_BURST_RECEIVE_START :
                         I2C_MASTER_CMD_SINGLE_RECEIVE));

    //
    // Receive the required number of bytes.
    //
    ulToRead = ulCount;

    while(ulToRead)
    {
        //
        // Wait until the current byte has been read.
        //
        while(ROM_I2CMasterIntStatus(I2C0_MASTER_BASE, false) == 0)
        {
        }

        //
        // Clear pending interrupt notification.
        //
        ROM_I2CMasterIntClear(I2C0_MASTER_BASE);

        //
        // Read the received character.
        //
        *pucData++ = ROM_I2CMasterDataGet(I2C0_MASTER_BASE);
        ulToRead--;

        //
        // Set up for the next byte if any more remain.
        //
        if(ulToRead)
        {
            ROM_I2CMasterControl(I2C0_MASTER_BASE,
                                 ((ulToRead == 1) ?
                                  I2C_MASTER_CMD_BURST_RECEIVE_FINISH :
                                  I2C_MASTER_CMD_BURST_RECEIVE_CONT));
        }
    }

    //
    // Tell the caller we read the required data.
    //
    return(true);
}

//*****************************************************************************
//
// Writes a single byte to an address in the I2C-attached EEPROM device.
//
// \param ucAddr is the EEPROM address to write.
// \param ucData is the data byte to write.
//
// This function writes a single byte to the I2C-attached EEPROM on the
// daughter board.  The location to write is passed in parameter \e ucAddr
// where values 0 to 127 are valid (this is a 128 byte EEPROM).
//
// This is not used in this particular application but is included for
// completeness.
//
// \return Returns \b true on success of \b false on failure.
//
//*****************************************************************************
static tBoolean
EEPROMWritePolled(unsigned char ucAddr, unsigned char ucData)
{
    //
    // Start with a dummy write to get the address set in the EEPROM.
    //
    ROM_I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, ID_I2C_ADDR, false);

    //
    // Place the address to be written in the data register.
    //
    ROM_I2CMasterDataPut(I2C0_MASTER_BASE, ucAddr);

    //
    // Start a burst send, sending the device address as the first byte.
    //
    ROM_I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait until the current byte has been transferred.
    //
    if(!WaitI2CFinished())
    {
        return(false);
    }

    //
    // Write the value to be written to the flash.
    //
    ROM_I2CMasterDataPut(I2C0_MASTER_BASE, ucData);

    //
    // Send the data byte.
    //
    ROM_I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    //
    // Wait until the current byte has been transferred.
    //
    if(!WaitI2CFinished())
    {
        return(false);
    }

    //
    // Delay 5mS to allow the write to complete.  We should really poll the
    // device waiting for an ACK but this is easier (and this code is only
    // for testcase use so this should be safe).
    //
    SysCtlDelay(ROM_SysCtlClockGet() / (200 * 3));

    //
    // Tell the caller we wrote the required data.
    //
    return(true);
}

//*****************************************************************************
//
// This function implements the "weprom" command.  It writes a single byte to
// a given location in the I2C flash part on the daughter board.  This is not
// used in the application but is included in case you find a use for it in
// your code.
//
//*****************************************************************************
int
Cmd_weprom(int argc, char *argv[])
{
    const char *pcPos;
    tBoolean bRetcode;
    unsigned long ulAddr, ulVal;

    //
    // Were we passed the correct number of parameters?
    //
    if(argc < 3)
    {
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 3)
    {
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Get the parameters.
    //
    ulAddr = ustrtoul(argv[1], &pcPos, 0);
    ulVal  = ustrtoul(argv[2], &pcPos, 0);

    if(ulAddr >= 128)
    {
        UARTprintf("Error: Write address must be between 0 and 127.\n");
        return(COMMAND_INVALID_ARG);
    }

    //
    // Tell the user what we are about to do.
    //
    UARTprintf("Writing value %d (0x%02x) to I2C flash address 0x%02x\n",
               ulVal, ulVal, ulAddr);

    //
    // Write the byte to the flash device.
    //
    bRetcode = EEPROMWritePolled((unsigned char)(ulAddr & 0xFF),
                                 (unsigned char)(ulVal & 0xFF));

    if(bRetcode)
    {
        UARTprintf("Byte written successfully.\n");
    }
    else
    {
        UARTprintf("Error writing byte!\n");
    }

    return(COMMAND_OK);
}

//*****************************************************************************
//
// This function implements the "writeid" command.  It writes the ID block for
// either the SRAM/Flash daughter board or the FPGA daughter board to the
// I2C EEPROM on the board.
//
//*****************************************************************************
int
Cmd_writeid(int argc, char *argv[])
{
    const char *pcPos;
    tBoolean bRetcode;
    unsigned long ulIndex, ulLength;
    unsigned char pucBuffer[128];

    //
    // Were we passed the correct number of parameters?
    //
    if(argc < 2)
    {
        UARTprintf("This function requires a single command line argument\n");
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 2)
    {
        UARTprintf("This function requires a single command line argument\n");
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Get the parameters.
    //
    ulIndex = ustrtoul(argv[1], &pcPos, 0);

    if(ulIndex >= NUM_DAUGHTER_STRUCTS)
    {
        UARTprintf("Error: Index must be 0 for SRAM, 1 for FPGA or 2 for EM2.\n");
        return(COMMAND_INVALID_ARG);
    }

    //
    // Tell the user what we are about to do.
    //
    UARTprintf("Writing ID block for '%s' daughter board...\n",
               g_pcIDNames[ulIndex]);

    //
    // Fix up the ID block before we write it.  How long will the block be?
    //
    ulLength = strlen(g_pcIDNames[ulIndex]) + sizeof(tDaughterIDInfo);

    //
    // Make sure the block is not too long.
    //
    if(ulLength > 128)
    {
        //
        // The block is too long. Tell the user and abort.
        //
        UARTprintf("ID block length %d exceeds maximum 128! "
                   "Reduce string length.\n", ulLength);
        return(COMMAND_OK);
    }

    //
    // Copy the basic structure into our buffer.
    //
    memcpy(pucBuffer, &g_psIDBlock[ulIndex], sizeof(tDaughterIDInfo) - 1);

    //
    // Fix up the length.
    //
    pucBuffer[offsetof(tDaughterIDInfo, ucLength)] =
                                            (unsigned char)(ulLength & 0xFF);

    //
    // Append the string to the end of the structure.
    //
    memcpy(pucBuffer + (sizeof(tDaughterIDInfo) - 1),
           g_pcIDNames[ulIndex], ulLength - sizeof(tDaughterIDInfo));

    //
    // Add the terminating NULL to the end of the string.
    //
    pucBuffer[ulLength - 1] = (unsigned char)0;

    //
    // Write the structure to flash, one byte at a time.
    //
    for(ulIndex = 0; ulIndex < ulLength; ulIndex++)
    {
        bRetcode = EEPROMWritePolled((unsigned char)ulIndex,
                                     pucBuffer[ulIndex]);
        if(!bRetcode)
        {
            //
            // There was an error - abort the operation.
            //
            UARTprintf("Error writing ID block byte %d!\n", ulIndex);
            return(COMMAND_OK);
        }
    }

    //
    // If we get here, the block was written correctly.
    //
    UARTprintf("ID block written successfully.\n");
    return(COMMAND_OK);
}

//*****************************************************************************
//
// This function implements the "read" command.  It reads a block of bytes from
// a given location in the I2C flash part on the daughter board.
//
//*****************************************************************************
int
Cmd_read(int argc, char *argv[])
{
    const char *pcPos;
    tBoolean bRetcode;
    unsigned long ulAddr, ulCount, ulLoop, ulLoop2;
    unsigned char ucData;

    //
    // Were we passed the correct number of parameters?
    //
    if(argc < 3)
    {
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 3)
    {
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Get the parameters.
    //
    ulAddr  = ustrtoul(argv[1], &pcPos, 0);
    ulCount = ustrtoul(argv[2], &pcPos, 0);

    //
    // Make sure the start address is valid.
    //
    if(ulAddr >= 128)
    {
        UARTprintf("Error: Address must be between 0 and 127.\n");
        return(COMMAND_INVALID_ARG);
    }

    //
    // Make sure the end address is valid.
    //
    if((ulAddr + ulCount) > 128)
    {
        UARTprintf("Error: End address must be < 128\n");
        return(COMMAND_INVALID_ARG);
    }

    //
    // Loop through the requested range in steps of 8 bytes.  This double loop
    // is purely to allow us to format the output a bit more cleanly.
    //
     for(ulLoop = (ulAddr & 0xF8); ulLoop < (((ulAddr + ulCount) + 7) & 0xF8);
        ulLoop += 8)
    {
        //
        // Show the address we are currently reading.
        //
        UARTprintf("\n0x%02x: ", ulLoop);

        //
        // Now loop through the individual bytes in this 8 byte block.
        //
        for(ulLoop2 = ulLoop;
           (ulLoop2 < (ulLoop + 8)) && (ulLoop2 < (ulAddr + ulCount));
           ulLoop2++)
        {
            //
            // Are we inside the requested range?  If not, just print a
            // placemarker.
            //
            if(ulLoop2 < ulAddr)
            {
                UARTprintf("   ");
            }
            else
            {
                //
                // We are inside the requested address range so read the value
                // and output it.
                //
                bRetcode = EEPROMReadPolled(&ucData, ulLoop2, 1);

                //
                // Did we read the EEPROM correctly?
                //
                if(bRetcode)
                {
                    UARTprintf("%02x ", ucData);
                }
                else
                {
                    UARTprintf("\nError reading byte from address 0x%02x!\n",
                               ulLoop2);
                    return(COMMAND_OK);
                }
            }
        }
    }

    //
    // Tell the command line processor that we processed the command
    // successfully.
    //
    return(COMMAND_OK);
}

//*****************************************************************************
//
// This function implements the "readid" command.  It reads a the ID block from
// the I2C EEPROM on the daughter board and displays the contents.
//
//*****************************************************************************
int
Cmd_readid(int argc, char *argv[])
{
    tBoolean bRetcode;
    tDaughterIDInfo sInfo;

    //
    // Read a byte from the flash device.
    //
    bRetcode = EEPROMReadPolled((unsigned char *)&sInfo, 0,
                                sizeof(tDaughterIDInfo));

    if(bRetcode)
    {
        UARTprintf("\nDaughter Board ID Block\n");
        UARTprintf(  "-----------------------\n\n");
        UARTprintf("Marker:       %c%c (0x%02x, 0x%02x)\n", sInfo.pucMarker[0],
                   sInfo.pucMarker[1], sInfo.pucMarker[0], sInfo.pucMarker[1]);
        UARTprintf("Length:       %d bytes\n", sInfo.ucLength);
        UARTprintf("Version:      %d\n", sInfo.ucVersion);
        UARTprintf("BoardID:      %d (0x%04x)\n", sInfo.usBoardID,
                   sInfo.usBoardID);
        UARTprintf("BoardRev:     %d\n", sInfo.ucBoardRev);
        UARTprintf("EPI Mode:     0x%02x\n", sInfo.ucEPIMode);
        UARTprintf("EPI Pins:     0x%08x\n", sInfo.ulEPIPins);
        UARTprintf("Addr Map:     0x%02x\n", sInfo.ucAddrMap);
        UARTprintf("Rate 0:       %dnS\n", sInfo.usRate0nS);
        UARTprintf("Rate 1:       %dnS\n", sInfo.usRate1nS);
        UARTprintf("Max Wait:     %d cycles\n", sInfo.ucMaxWait);
        UARTprintf("Config:       0x%08x\n", sInfo.ulConfigFlags);
        UARTprintf("Read Access:  %dnS\n", sInfo.ucReadAccTime);
        UARTprintf("Write Access: %dnS\n", sInfo.ucWriteAccTime);
        UARTprintf("Read Cycle    %dnS\n", sInfo.usReadCycleTime);
        UARTprintf("Write Cycle:  %dnS\n", sInfo.usWriteCycleTime);
        UARTprintf("Columns:      %d\n", sInfo.usNumColumns);
        UARTprintf("Rows:         %d\n", sInfo.usNumRows);
        UARTprintf("Refresh:      %dmS\n", sInfo.ucRefreshInterval);
        UARTprintf("Frame count:  %d\n", sInfo.ucFrameCount);
        if(sInfo.pucName[0])
        {
            unsigned long ulLoop;
            unsigned long ulIndex;
            unsigned char ucChar;

            UARTprintf("Name:         %c", sInfo.pucName[0]);

            ulIndex = offsetof(tDaughterIDInfo, pucName) + 1;
            ulLoop = (unsigned long)sInfo.ucLength - ulIndex;

            //
            // Read and display the remaining characters in the name string.
            //
            while(ulLoop)
            {
                //
                // Get the next character from the name string.
                //
                bRetcode = EEPROMReadPolled(&ucChar, ulIndex, 1);

                if(bRetcode && ucChar)
                {
                    UARTprintf("%c", ucChar);
                }
                else
                {
                    break;
                }

                ulLoop--;
                ulIndex++;
            }
             UARTprintf("\n");
        }
    }
    else
    {
        UARTprintf("Error reading ID block from daughter board!\n");
    }

    return(COMMAND_OK);
}

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list
// of the available commands with a brief description.
//
//*****************************************************************************
int
Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    //
    // Print some header text.
    //
    UARTprintf("\nAvailable commands\n");
    UARTprintf("------------------\n");

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_sCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The
    // end of the table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        UARTprintf("%s%s\n", pEntry->pcCmd, pEntry->pcHelp);

        //
        // Advance to the next entry in the table.
        //
        pEntry++;
    }

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions,
// and brief description.
//
//*****************************************************************************
tCmdLineEntry g_sCmdTable[] =
{
    { "read",    Cmd_read,    "   : <ad> <n> Read <n> bytes from location <ad>" },
    { "readid",  Cmd_readid,    " : Read the ID block from the EEPROM" },
    { "writeid", Cmd_writeid,    ": <0|1|2> Write ID block. 0 SRAM, 1 FPGA, 2 EM2" },
    { "help",    Cmd_help,    "   : Display list of commands" },
    { "h",       Cmd_help, "      : alias for help" },
    { "?",       Cmd_help, "      : alias for help" },
    { 0, 0, 0 }
};

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
// Main application entry function.
//
//*****************************************************************************
int main(void)
{
    int nStatus;

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the default pinout (and query any daughter board that may be
    // there already.  This has the side-effect of initializing the I2C
    // controller for us too.
    //
    PinoutSet();

    //
    // Enable UART0.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART standard I/O.
    //
    UARTStdioInit(0);
    UARTprintf("\n\nDaughter Board ID EEPROM Read/Write\n");
    UARTprintf(    "-----------------------------------\n\n");
    UARTprintf("Use this tool to read or repair the information stored\n");
    UARTprintf("in the 128 byte ID EEPROM on optional development kit\n");
    UARTprintf("daughter boards.\n");

    //
    // Output our help screen.
    //
    Cmd_help(0, 0);

    //
    // Tell the user which daughter board we have detected.
    //
    UARTprintf("\nCurrent daughter board: ");

    switch(g_eDaughterType)
    {
        case DAUGHTER_NONE:
        {
            UARTprintf("None or SDRAM\n");
            break;
        }

        case DAUGHTER_SRAM_FLASH:
        case DAUGHTER_FPGA:
        case DAUGHTER_EM2:
        {
            UARTprintf(g_pcIDNames[g_eDaughterType - 1]);
            break;
        }

        default:
        {
            UARTprintf("Unrecognized\n");
            break;
        }
    }

    //
    //
    // Fall into the command line processing loop.
    //
    while (1)
    {
        //
        // Print a prompt to the console
        //
        UARTprintf("\n> ");

        //
        // Get a line of text from the user.
        //
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));

        //
        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        //
        nStatus = CmdLineProcess(g_cCmdBuf);

        //
        // Handle the case of bad command.
        //
        if(nStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(nStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }
    }
}
