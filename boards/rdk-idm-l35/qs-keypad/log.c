//*****************************************************************************
//
// log.c - Functions to log events on the UART.
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
// This is part of revision 8555 of the RDK-IDM-L35 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "log.h"
#include "qs-keypad.h"

//*****************************************************************************
//
// Strings for the names of the months, used to create the date code associated
// with a logged event.
//
//*****************************************************************************
static const char *g_ppcMonths[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

//*****************************************************************************
//
// Strings for the names of the week days, used to create the date code
// associated with a logged event.
//
//*****************************************************************************
static const char *g_ppcDays[7] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

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
// Prototypes for command handlers.
//
//*****************************************************************************
extern int Cmd_help(int argc, char *argv[]);
extern int Cmd_update(int argc, char *argv[]);
extern int Cmd_stats(int argc, char *argv[]);
extern int Cmd_show(int argc, char *argv[]);
extern int Cmd_set(int argc, char *argv[]);

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions,
// and brief description.
//
//*****************************************************************************
tCmdLineEntry g_sCmdTable[] =
{
    { "help",   Cmd_help,    " : Display list of commands" },
    { "h",      Cmd_help,    " : alias for help" },
    { "?",      Cmd_help,    " : alias for help" },
    { "show",   Cmd_show,    " : Show the current access code" },
    { "set",    Cmd_set,     " : <code> Set a new access code" },
    { "stats",  Cmd_stats,   " : Show access statistics" },
    { "swupd",  Cmd_update,  " : Initiate a firmware update via serial" },
    { 0, 0, 0 }
};

//*****************************************************************************
//
// Writes a message to the log.  The message is preceeded by a time stamp, and
// the message can not exceed 28 characters in length (unless g_pcBuffer is
// increased in size).
//
//*****************************************************************************
void
LogWrite(char *pcPtr)
{
    tTime sTime;

    //
    // Convert the current time from seconds since Jan 1, 1970 to the month,
    // day, year, hour, minute, and second equivalent.
    //
    ulocaltime(g_ulTime, &sTime);

    //
    // Construct the log message with the time stamp preceeding it.
    //
    UARTprintf("%s %s %2d %02d:%02d:%02d.%02d UT %d => %s\r\n",
               g_ppcDays[sTime.ucWday], g_ppcMonths[sTime.ucMon], sTime.ucMday,
               sTime.ucHour, sTime.ucMin, sTime.ucSec, g_ulTimeCount / 10,
               sTime.usYear, pcPtr);
}

//*****************************************************************************
//
// Checks to see if a new command has been received via the serial port and,
// if so, processes it.
//
//*****************************************************************************
void
LogProcessCommands(void)
{
    int nStatus;

    //
    // Check to see if there is a new serial command to process.
    //
    if(UARTPeek('\r') >= 0)
    {
        //
        // A new command has been entered so read it.
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

        //
        // Otherwise the command was executed.  Print the error
        // code if one was returned.
        //
        else if(nStatus != 0)
        {
            UARTprintf("Command returned error code %d\n", nStatus);
        }

        //
        // Print a prompt on the console.
        //
        UARTprintf("\n> ");
    }
}

//*****************************************************************************
//
// Initializes the logging interface.
//
//*****************************************************************************
void
LogInit(void)
{
    //
    // Enable the peripherals used to perform logging.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Configure the UART pins appropriately.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1 | GPIO_PIN_0);

    //
    // Initialize the UART as a console for text I/O.
    //
    UARTStdioInit(0);

    //
    // Print hello message to user via the serial port.
    //
    UARTprintf("\n\nQuickstart Keypad Example Program\n");
    UARTprintf("Type \'help\' for help.\n");

    //
    // Write a log message to indicate that the application has started.
    //
    LogWrite("Application started");
}

//*****************************************************************************
//
// This function implements the "swupd" command.  It transfers control to the
// bootloader to update the firmware via ethernet.
//
//*****************************************************************************
int
Cmd_update(int argc, char *argv[])
{
    //
    // Tell the user what we are doing.
    //
    UARTprintf("Serial firmware update requested.\n");

    //
    // Transfer control to the bootloader.
    //
    UARTprintf("Transfering control to boot loader...\n\n");
    UARTprintf("***********************************\n");
    UARTprintf("*** Close your serial terminal ****\n");
    UARTprintf("***   before running LMFlash.  ****\n");
    UARTprintf("***********************************\n\n");
    UARTFlushTx(false);

    //
    // Signal the main loop that it should begin the software update process.
    //
    g_bFirmwareUpdate = true;

    return(0);
}

//*****************************************************************************
//
// This function implements the "help" command.  It displays all the supported
// commands and provides a brief description of each.
//
//*****************************************************************************
int Cmd_help(int argc, char *argv[])
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
// This function implements the "stats" command.  It shows various statistics
// relating to door access attempts.
//
//*****************************************************************************
int
Cmd_stats(int argc, char *argv[])
{
    //
    // Tell the user how often attempts to unlock the door have been
    // successful or unsuccessful.
    //
    UARTprintf("Successful attempts:   %d\n", g_ulAllowedCount);
    UARTprintf("Unsuccessful attempts: %d\n", g_ulDeniedCount);

    return(0);
}

//*****************************************************************************
//
// This function implements the "show" command.  It shows the current access
// code for the door.
//
//*****************************************************************************
int
Cmd_show(int argc, char *argv[])
{
    //
    // Print out the current access code.
    //
    UARTprintf("Current access code: %04x\n", g_ulAccessCode);

    return(0);
}

//*****************************************************************************
//
// This function implements the "set" command.  It allows the door access code
// to be changed.
//
//*****************************************************************************
int
Cmd_set(int argc, char *argv[])
{
    unsigned long ulLoop, ulCode, ulDigit;

    //
    // Ensure we have only a single parameter.
    //
    if(argc != 2)
    {
        UARTprintf("This function requires a single, 4 digit number as an "
                   "argument.\n");
        return(-3);
    }

    //
    // Initialize the new code value.
    //
    ulCode = 0;

    //
    // Read the code from the parameters string as a BCD number.
    //
    for(ulLoop = 0; (ulLoop < 4) && (argv[1][ulLoop] != 0); ulLoop++)
    {
        //
        // Shift the code to accomodate the next digit.
        //
        ulCode <<= 4;

        //
        // Get the next digit.
        //
        ulDigit = (unsigned long)(argv[1][ulLoop]) - (unsigned long)'0';

        //
        // Is it valid?
        //
        if(ulDigit > 9)
        {
            UARTprintf("The code supplied is not valid. Please enter a 4 "
                       "digit decimal number.\n");
            return(-3);
        }

        //
        // Append the digit to the code.
        //
        ulCode |= ulDigit;
    }

    //
    // If we drop out, we either processed 4 characters or hit the end of
    // the string.  If we processed 4 characters, make sure that the string
    // does not continue after this since that would be an invalid code.
    //
    if((ulLoop == 4) && (argv[1][ulLoop] != '\0'))
    {
        //
        // The string is longer than 4 characters so inform the user of the
        // error and ignore the change.
        //
        UARTprintf("The code supplied is not valid. Please enter a 4 "
                   "digit decimal number.\n");
        return(-3);
    }

    //
    // At this point, we got a valid code so set it.
    //
    SetAccessCode(ulCode);

    return(0);
}
