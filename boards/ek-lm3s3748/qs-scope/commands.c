//*****************************************************************************
//
// commands.c - Command line handler functions for the quickstart scope
//              application (during development)
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "commands.h"
#include "data-acq.h"
#include "file.h"
#include "qs-scope.h"
#include "renderer.h"

//*****************************************************************************
//
// Command line input buffer.
//
//*****************************************************************************
#define COMMAND_BUFFER_LEN      80
static char g_pcCmdLine[COMMAND_BUFFER_LEN];

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
// Prototypes for each handler function
//
//*****************************************************************************
int CmdHelp(int argc, char *argv[]);
int CmdQuit(int argc, char *argv[]);
int CmdTrigger(int argc, char *argv[]);
int CmdLevel(int argc, char *argv[]);
int CmdCapture(int argc, char *argv[]);
int CmdDump(int argc, char *argv[]);
int CmdInfo(int argc, char *argv[]);
int CmdCls(int argc, char *argv[]);
int CmdStop(int argc, char *argv[]);
int CmdStart(int argc, char *argv[]);
int CmdVScale(int argc, char *argv[]);
int CmdVOffset(int argc, char *argv[]);
int CmdTimebase(int argc, char *argv[]);
int CmdHOffset(int argc, char *argv[]);
int CmdToggleCh2(int argc, char *argv[]);
int CmdSaveCSV(int argc, char *argv[]);
int CmdSaveBMP(int argc, char *argv[]);
int CmdCat(int argc, char *argv[]);
int CmdLs(int argc, char *argv[]);

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions,
// and brief description.
//
//*****************************************************************************
tCmdLineEntry g_sCmdTable[] =
{
    { "help",              CmdHelp,
      "- Display list of commands" },
    { "h",                 CmdHelp,
      "     alias for help" },
    { "?",                 CmdHelp,
      "     alias for help" },
    { "stop",              CmdStop,
      "- Stop continous capture" },
    { "start",             CmdStart,
      "- Start continous capture" },
    { "trigger",           CmdTrigger,
      "- Sets trigger type RISE, FALL, LEVEL or ALWAYS" },
    { "t",                 CmdTrigger,
      "     alias for trigger" },
    { "level",             CmdLevel,
      "- Sets trigger level to <param1>mV" },
    { "l",                 CmdLevel,
      "     alias for level" },
    { "ch2",               CmdToggleCh2,
      "- Channel 2 on (<param1> = 1) or off (0)" },
    { "capture",           CmdCapture,
      "- Request a single capture sequence" },
    { "c",                 CmdCapture,
      "     alias for capture" },
    { "cls",               CmdCls,
      "- Clear the display" },
    { "dump",              CmdDump,
      "- Dump latest captured samples to UART0" },
    { "d",                 CmdDump,
      "     alias for dump" },
    { "savecsv",           CmdSaveCSV,
      "- Save a CSV file to SD (<param1> = 0) or USB (1)" },
    { "savebmp",           CmdSaveBMP,
      "- Save a bitmap to SD (<param1> = 0) or USB (1)" },
    { "cat",               CmdCat,
      "- Dump contents of file <param1> to UART0" },
    { "ls",                CmdLs,
      "- Show files in directory <param1>." },
    { "scale",             CmdVScale,
      "- Set <param1>mV/div for channel <param2> (0|1)"},
    { "time",              CmdTimebase,
      "- Set <param1>uS/div timebase"},
    { "voff",              CmdVOffset,
      "- Set <param1>mV offset channel <param2> (0|1)"},
    { "hoff",              CmdHOffset,
      "- Set <param1> x pixel offset"},
    { "info",              CmdInfo,
      "- Output current trigger and capture status" },
    { "i",                 CmdInfo,
      "     alias for info" },
    { "quit",              CmdQuit,
      "- Exit the test" },
    { "q",                 CmdQuit,
      "     alias for quit" },
    { 0, 0, 0 }
};

//*****************************************************************************
//
// Data used to map between user entered strings and a trigger mode.
//
//*****************************************************************************
typedef struct
{
    char *pcString;
    tTriggerType eTrigger;
}
tTriggerMap;

static tTriggerMap g_psTriggerMap[] =
{
    {"LEVEL",   TRIGGER_LEVEL},
    {"RISING",  TRIGGER_RISING},
    {"FALLING", TRIGGER_FALLING},
    {"ALWAYS",  TRIGGER_ALWAYS}
};

#define NUM_TRIGGER_MAP_ENTRIES ((sizeof(g_psTriggerMap)/sizeof(tTriggerMap)))

//*****************************************************************************
//
// Data used to map between data acquisition states and a user readable string.
//
//*****************************************************************************
typedef struct
{
    char *pcString;
    tDataAcqState eState;
}
tStateMap;

static tStateMap g_psStateMap[] =
{
    { "IDLE",           DATA_ACQ_IDLE },
    { "BUFFERING",      DATA_ACQ_BUFFERING },
    { "TRIGGER_SEARCH", DATA_ACQ_TRIGGER_SEARCH },
    { "TRIGGERED",      DATA_ACQ_TRIGGERED },
    { "COMPLETE",       DATA_ACQ_COMPLETE },
    { "ERROR",          DATA_ACQ_ERROR }
};

#define NUM_STATE_MAP_ENTRIES   ((sizeof(g_psStateMap) / sizeof(tStateMap)))

//*****************************************************************************
//
// Flag indicating whether or not we should clear the screen on every new
// capture.
//
//*****************************************************************************
tBoolean g_bAutoCls = true;

//*****************************************************************************
//
// Map a trigger type to a human-readable string.
//
//*****************************************************************************
static char *
TriggerToString(tTriggerType eType)
{
    char *pcTrig = 0;
    unsigned long ulLoop;

    //
    // Get a description string for the trigger type.
    //
    for(ulLoop = 0; ulLoop < NUM_TRIGGER_MAP_ENTRIES; ulLoop++)
    {
        if(g_psTriggerMap[ulLoop].eTrigger == eType)
        {
            pcTrig = g_psTriggerMap[ulLoop].pcString;
            break;
        }
    }

    //
    // Catch cases where we end up with an unknown trigger type.
    //
    if(ulLoop == NUM_TRIGGER_MAP_ENTRIES)
    {
        pcTrig = "**UNKNOWN**";
    }

    //
    // Pass the string pointer back to the caller.
    //
    return(pcTrig);
}

//*****************************************************************************
//
// Map a data acquisition state to a human-readable string.
//
//*****************************************************************************
static char *
StateToString(tDataAcqState eState)
{
    char *pcState = 0;
    unsigned long ulLoop;

    //
    // Get a description string for the state.
    //
    for(ulLoop = 0; ulLoop < NUM_STATE_MAP_ENTRIES; ulLoop++)
    {
        if(g_psStateMap[ulLoop].eState == eState)
        {
            pcState = g_psStateMap[ulLoop].pcString;
            break;
        }
    }

    //
    // Catch cases where we end up with an unknown trigger type.
    //
    if(ulLoop == NUM_STATE_MAP_ENTRIES)
    {
        pcState = "**UNKNOWN**";
    }

    //
    // Pass the string pointer back to the caller.
    //
    return(pcState);
}

//*****************************************************************************
//
// Read a command line from the user and process it.
//
//*****************************************************************************
void
CommandReadAndProcess(void)
{
    int iCount;

    //
    // Check to see if there is a string available in the UART receive
    // buffer.
    //
    iCount = UARTPeek('\r');

    //
    // A negative return code indicates that there is no '\r' character in
    // the receive buffer and, hence, no complete string entered by the user
    // so just return.
    //
    if(iCount < 0)
    {
        return;
    }

    //
    // Here, we know that a string is available so read it.
    //
    iCount = UARTgets(g_pcCmdLine, COMMAND_BUFFER_LEN);

    //
    // If something sensible was entered, go ahead and process the command
    // line.
    //
    if(iCount)
    {
        //
        // Process the command entered by the user
        //
        iCount = CmdLineProcess(g_pcCmdLine);

        switch(iCount)
        {
            //
            // The command was not recognized
            //
            case CMDLINE_BAD_CMD:
            {
                UARTprintf("ERROR: Unrecognized command\n");
                break;
            }

            //
            // The command contained too many arguments for the command
            // line processor to handle.
            //
            case CMDLINE_TOO_MANY_ARGS:
            {
                UARTprintf("ERROR: Too many arguments\n");
                break;
            }
        }

        //
        // Display a prompt
        //
        UARTprintf(">");
    }
}

//*****************************************************************************
//
// Command Handler Functions
//
//*****************************************************************************

//*****************************************************************************
//
// \internal
//
// Handler for the "help" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "help", "h" or "?" commands.  It prints out a list of supported
// commands with a description of each.
//
// This handler takes no parameters.  argc and argv are ignored.
//
// \return COMMAND_OK in all cases
//
//*****************************************************************************
int
CmdHelp(int argc, char *argv[])
{
    int iLoop;

    iLoop = 0;

    UARTprintf("\nOscilloscope Test Commands:\n");
    UARTprintf("---------------------------\n\n");

    while(g_sCmdTable[iLoop].pcCmd != (const char *)0)
    {
        UARTprintf("%13s%s\n",
                         g_sCmdTable[iLoop].pcCmd,
                         g_sCmdTable[iLoop].pcHelp);
        iLoop++;

        //
        // Every 5 lines, wait for the UART to catch up with us.
        //
        if(!(iLoop % 5))
        {
            UARTFlushTx(false);
        }
    }

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "scale" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "scale" command.  It sets the voltage scale for one of the oscilloscope
// channels
//
// This handler takes two parameters.  The first is the voltage scale expressed
// in millivolts per division and the second is the channel number, 0 or 1.
//
// \return Returns COMMAND_OK if all is well or other return codes on failure.
//
//*****************************************************************************
int
CmdVScale(int argc, char *argv[])
{
    unsigned long ulVoltage;
    unsigned long ulChannel;
    const char *pcPos;

    //
    // Make sure we were passed the command and exactly 2 extra arguments
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
    // Now extract numbers from the two command parameters.
    //
    ulVoltage = ustrtoul(argv[1], &pcPos, 0);
    ulChannel = ustrtoul(argv[2], &pcPos, 0);

    //
    // Make sure we were passed a valid channel number
    //
    if(ulChannel < 2)
    {
        //
        // Set the scaling for this channel.  This will take effect the next
        // time the display is updated.
        //
        UARTprintf("Setting scale for channel %d to %dmV/division.\n",
                   ulChannel, ulVoltage);

        g_sRender.ulmVPerDivision[ulChannel] = ulVoltage;
        return(COMMAND_OK);
    }
    else
    {
        //
        // Illegal channel number
        //
        UARTprintf("Channel %d is invalid.\n", ulChannel);
        return(COMMAND_INVALID_ARG);
    }
}

//*****************************************************************************
// \internal
//
// Handler for the "voff" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "voff" command.  It sets the voltage offset for one of the oscilloscope
// channels
//
// This handler takes two parameters.  The first is the voltage offset
// expressed in millivolts and the second is the channel number, 0 or 1.
// Positive offset values cause the waveform to move towards the top of
// the display and negative values move it towards the bottom.
//
// \return Returns COMMAND_OK if all is well or other return codes on failure.
//
//*****************************************************************************
int
CmdVOffset(int argc, char *argv[])
{
    long lVoltage;
    unsigned long ulChannel;
    const char *pcPos;

    //
    // Make sure we were passed the command and exactly 2 extra arguments
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
    // Now extract numbers from the two command parameters.
    //
    lVoltage = (long)ustrtoul(argv[1], &pcPos, 0);
    ulChannel = ustrtoul(argv[2], &pcPos, 0);

    //
    // Make sure we were passed a valid channel number
    //
    if(ulChannel < 2)
    {
        //
        // Set the scaling for this channel.  This will take effect the next
        // time the display is updated.
        //
        UARTprintf("Setting offset for channel %d to %dmV.\n",
                   ulChannel, lVoltage);

        COMMAND_FLAG_WRITE((ulChannel == 0) ? SCOPE_CH1_POS : SCOPE_CH2_POS,
                           (unsigned long)lVoltage);

        return(COMMAND_OK);
    }
    else
    {
        //
        // Illegal channel number
        //
        UARTprintf("Channel %d is invalid.\n", ulChannel);
        return(COMMAND_INVALID_ARG);
    }
}

//*****************************************************************************
// \internal
//
// Handler for the "stop" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "stop" command.  It sets a flag to tell the main loop to stop capturing
// oscilloscope samples continuously.
//
// This handler takes no parameters.  argc and argv are ignored.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdStop(int argc, char *argv[])
{
    //
    // Stop continuous capture.
    //
    UARTprintf("Stopping continuous capture.\n");
    COMMAND_FLAG_WRITE(SCOPE_STOP, 0);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "start" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "start" command.  It sets a flag to tell the main loop to start
// capturing oscilloscope samples continuously.
//
// This handler takes no parameters.  argc and argv are ignored.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdStart(int argc, char *argv[])
{
    //
    // Start continuous capture.
    //
    UARTprintf("Starting continuous capture.\n");
    COMMAND_FLAG_WRITE(SCOPE_START, 0);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "ch2" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "ch2" command.  It sets a flag to tell the main loop to enabled or
// disable channel 2.
//
// This handler takes a single parameter, 0 to turn channel 2 off and 1 to
// turn it on.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdToggleCh2(int argc, char *argv[])
{
    unsigned long ulOnOff;
    const char *pcPos;

    //
    // Make sure we were passed the command and exactly 1 extra arguments
    //
    if(argc < 2)
    {
        UARTprintf("This command requires 1 argument.\n");
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 2)
    {
        UARTprintf("This command requires 1 argument.\n");
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Now extract the on/off flag.
    //
    ulOnOff = ustrtoul(argv[1], &pcPos, 0);

    //
    // Stop continuous capture.
    //
    UARTprintf("$abling channel 2.\n", ulOnOff ? "En" : "Dis");

    //
    // Tell the main loop to enable or disable channel 2.
    //
    COMMAND_FLAG_WRITE(SCOPE_CH2_DISPLAY, ulOnOff);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "savecsv" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "savecsv" command.  It sends a command to the main loop to save the
// latest captured data in CSV format to either the SD card or USB stick,
// depending upon which is connected.
//
// This handler takes a single parameter, 0 to save to the SD card or anything
// else to save to a USB flash stick.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdSaveCSV(int argc, char *argv[])
{
    //
    // Ensure we were passed the correct number of arguments
    //
    if(argc < 2)
    {
        UARTprintf("This command requires 1 argument.\n");
        return(COMMAND_TOO_FEW_ARGS);
    }

    //
    // Tell the main loop to save the data in CSV format to the appropriate
    // drive.
    //
    COMMAND_FLAG_WRITE(SCOPE_SAVE, ((argv[1][0] == '0') ?
                        SCOPE_SAVE_CSV | SCOPE_SAVE_SD :
                        SCOPE_SAVE_CSV | SCOPE_SAVE_USB));

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "savebmp" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "savebmp" command.  It sends a command to the main loop to save the
// latest captured data in Windows bitmap format to either the SD card or USB
// stick, depending upon which is connected.
//
// This handler takes a single parameter, 0 to save to the SD card or anything
// else to save to a USB flash stick.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdSaveBMP(int argc, char *argv[])
{
    //
    // Tell the main loop to save the data in BMP format
    //
    COMMAND_FLAG_WRITE(SCOPE_SAVE, ((argv[1][0] == '0') ?
                        SCOPE_SAVE_BMP | SCOPE_SAVE_SD :
                        SCOPE_SAVE_BMP | SCOPE_SAVE_USB));

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "cat" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "cat" command.  It opens the file whose base filename is passed in
// the first argument and echoes the file contents to UART0.
//
// This handler takes a single parameter, the filename of the file to
// dump.  This will typically be "scopexxx.csv" where "xxx" is a 3 digit
// decimal number.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdCat(int argc, char *argv[])
{
    //
    // Make sure we were passed the command and exactly 1 extra arguments
    //
    if(argc < 2)
    {
        UARTprintf("This command requires 1 argument.\n");
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 2)
    {
        UARTprintf("This command requires 1 argument.\n");
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Tell the file module to cat the requested file.
    //
    FileCatToUART(argv[1]);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "ls" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "ls" command.  It calls the file module to have it dump a list of the
// files in the current directory of the SD card or flash stick depending
// upon which is installed.
//
// This handler takes one optional parameter.  If no parameters are specified,
// the root directory of logical drive 0 (the SD card) is listed.  If a single
// parameter is given, it is assumed to be a directory name and is passed to
// FileLsToUART() as the directory to be listed.  To list a particular logical
// drive, use the format "n:/" where 'n' is the drive number, 0 or 1.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdLs(int argc, char *argv[])
{
    //
    // Tell the file module to show the directory listing.
    //
    if(argc == 1)
    {
        FileLsToUART("/");
    }
    else
    {
        FileLsToUART(argv[1]);
    }

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "quit" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "quit" or "q" commands.  It prints a cheerful message then exits the
// application.
//
// This handler takes no parameters.  argc and argv are ignored.
//
// \return Does not return.
//
//*****************************************************************************
int
CmdQuit(int argc, char *argv[])
{
    //
    // Exit the application.
    //
    UARTprintf("Bye!\n");
    while(1)
    {
    }
}

//*****************************************************************************
// \internal
//
// Handler for the "time" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "time" command.  It expects a single parameter which will be a
// string containing a decimal number representing the number of microseconds
// per division on the waveform display.
//
// \return Returns COMMAND_OK on success or COMMAND_TOO_FEW_ARGS or
// COMMAND_INVALID_ARG on failure.
//
//*****************************************************************************
int
CmdTimebase(int argc, char *argv[])
{
    unsigned long ulTimebase;
    const char *pcPos;

    //
    // Make sure we were passed the command and exactly 1 extra argument
    //
    if(argc < 2)
    {
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 2)
    {
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Now extract a number from the first command parameter.
    //
    ulTimebase = ustrtoul(argv[1], &pcPos, 0);

    //
    // Tell the main loop to update the timebase on the next iteration.
    //
    COMMAND_FLAG_WRITE(SCOPE_CHANGE_TIMEBASE, ulTimebase);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "hoff" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "hoff" command.  It expects a single parameter which will be a
// string containing a decimal number representing the number of pixels to
// offset the waveform display in the X direction.  Positive numbers move
// the trigger position to the right.
//
// \return Returns COMMAND_OK on success or COMMAND_TOO_FEW_ARGS or
// COMMAND_INVALID_ARG on failure.
//
//*****************************************************************************
int
CmdHOffset(int argc, char *argv[])
{
    long lPos;
    const char *pcPos;

    //
    // Make sure we were passed the command and exactly 1 extra argument
    //
    if(argc < 2)
    {
        UARTprintf("Command requires 1 parameter\n");
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 2)
    {
        UARTprintf("Command requires 1 parameter\n");
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Now extract a number from the first command parameter.
    //
    lPos = (long)ustrtoul(argv[1], &pcPos, 0);

    //
    // Pass the new position to the main command processor.
    //
    COMMAND_FLAG_WRITE(SCOPE_TRIGGER_POS, (unsigned long)lPos);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "trigger" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "trigger" or "t" commands.  It expects a single parameter which will be
// a string containing RISE, FALL, ALWAYS or LEVEL to indicate the desired
// trigger mode.
//
// \return Returns COMMAND_OK on success or COMMAND_TOO_FEW_ARGS or
// COMMAND_INVALID_ARG on failure.
//
//*****************************************************************************
int
CmdTrigger(int argc, char *argv[])
{
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usLevel;
    unsigned long ulLoop;

    //
    // Make sure we were passed the command and exactly 1 extra argument
    //
    if(argc < 2)
    {
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 2)
    {
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Get the existing trigger parameters.
    //
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usLevel);

    //
    // Determine the new trigger type from the parameter passed.
    //
    for(ulLoop = 0; ulLoop < NUM_TRIGGER_MAP_ENTRIES; ulLoop++)
    {
        if(strcmp(argv[1], g_psTriggerMap[ulLoop].pcString) == 0)
        {
            eType = g_psTriggerMap[ulLoop].eTrigger;
            break;
        }
    }

    //
    // Did we find a match?
    //
    if(ulLoop == NUM_TRIGGER_MAP_ENTRIES)
    {
        UARTprintf("Trigger type %s is not recognized.\n", argv[1]);
        return(COMMAND_INVALID_ARG);
    }

    //
    // All is well, so set the new trigger type and signal the main loop to
    // process the trigger change.
    //
    COMMAND_FLAG_WRITE(SCOPE_CHANGE_TRIGGER, eType);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "level" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "level" or "l" commands.  It expects a single parameter which will be a
// string containing a decimal number representing the ADC sample level at
// which to trigger capture.
//
// \return Returns COMMAND_OK on success or COMMAND_TOO_FEW_ARGS or
// COMMAND_INVALID_ARG on failure.
//
//*****************************************************************************
int
CmdLevel(int argc, char *argv[])
{
    const char *pcPos;
    unsigned long ulLevel;

    //
    // Make sure we were passed the command and exactly 1 extra argument
    //
    if(argc < 2)
    {
        return(COMMAND_TOO_FEW_ARGS);
    }

    if(argc > 2)
    {
        return(CMDLINE_TOO_MANY_ARGS);
    }

    //
    // Extract a number from the first command parameter.
    //
    ulLevel = ustrtoul(argv[1], &pcPos, 0);

    //
    // Pass the command to the main handler for action.
    //
    COMMAND_FLAG_WRITE(SCOPE_TRIGGER_LEVEL, ulLevel);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "capture" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "capture" or "c" commands.  The handler expects no parameters.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdCapture(int argc, char *argv[])
{
    //
    // Make sure we are not currently capturing data.
    //
    if(!DataAcquisitionIsComplete())
    {
        UARTprintf("Capture pending.\n");
        return(COMMAND_OK);
    }

    //
    // Tell the main loop to initiate a single trigger capture.
    //
    COMMAND_FLAG_WRITE(SCOPE_CAPTURE, 0);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "dump" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "dump" or "d" commands.  It causes the latest captured data samples
// to be sent to the serial port unless no samples have yet been captured
// in which case the command is ignored.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdDump(int argc, char *argv[])
{
    unsigned long ulLoop;
    tBoolean bRetcode;
    tDataAcqCaptureStatus sStatus;
    unsigned long ulSample;
    unsigned long ulInc;

    //
    // Query the capture status.
    //
    bRetcode = DataAcquisitionGetStatus(&sStatus);
    ERROR_CHECK(bRetcode, "ERROR! Can't request status!\n");

    //
    // In dual mode, we will handle 2 samples per loop.
    //
    ulInc = sStatus.bDualMode ? 2 : 1;

    //
    // Dump the samples from the start to the end, taking care of the buffer
    // wrap.
    //
    ulSample = sStatus.ulStartIndex;

    for(ulLoop = 0; ulLoop < sStatus.ulSamplesCaptured; ulLoop+=ulInc)
    {
        //
        // Allow the UART some time to flush the buffer
        //
        if((ulLoop % 32) == 0)
        {
            UARTFlushTx(false);
        }

        if(!sStatus.bDualMode)
        {
            //
            // Dump the sample to the output.
            //
            UARTprintf("%d, %d\n", ulSample,
                       sStatus.pusBuffer[ulSample]);

            //
            // Move on to the next sample.
            //
            ulSample++;

            //
            // Handle the wrap at the end of the buffer.
            //
            if(ulSample == MAX_SAMPLES_PER_TRIGGER)
            {
                ulSample = 0;
            }
        }
        else
        {
            //
            // Dump the sample to the output.
            //
            UARTprintf("%d, %d, %d\n", ulSample,
                       sStatus.pusBuffer[ulSample],
                       sStatus.pusBuffer[ulSample+1]);

            //
            // Move on to the next sample.
            //
            ulSample += 2;

            //
            // Handle the wrap at the end of the buffer.
            //
            if(ulSample >= MAX_SAMPLES_PER_TRIGGER)
            {
                ulSample = 0;
            }
        }
    }

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "info" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "info" or "i" commands.  It prints information on the current trigger
// setting and the status of the latest capture request on UART0.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdInfo(int argc, char *argv[])
{
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usLevel;
    tDataAcqCaptureStatus sStatus;

    //
    // Get the existing trigger parameters.
    //
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usLevel);

    //
    // Print out the trigger info.
    //
    UARTprintf("\nTrigger\n");
    UARTprintf("-------\n");
    UARTprintf("Type:     %s (%d)\n", TriggerToString(eType), eType);
    UARTprintf("Level:    %d\n", usLevel);
    UARTprintf("Position: %d\n", ulTrigPos);

    //
    // Print out channel 1 status
    //
    DataAcquisitionGetStatus(&sStatus);

    UARTprintf("\nStatus\n");
    UARTprintf("------\n");
    UARTprintf("State:      %s (%d)\n",
               StateToString(sStatus.eState), sStatus.eState);
    UARTprintf("Captured:   %d\n", sStatus.ulSamplesCaptured);
    UARTprintf("Buffer:     0x%08x\n", sStatus.pusBuffer);
    UARTprintf("Data start: %d\n", sStatus.ulStartIndex);
    UARTprintf("Trigger:    %d\n", sStatus.ulTriggerIndex);
    UARTprintf("Mode:       %s\n",
               sStatus.bDualMode ? "Dual" : "Single");
    UARTprintf("CH2 offset: %duS\n", sStatus.ulSampleOffsetuS);
    UARTprintf("Period:     %duS\n", sStatus.ulSamplePerioduS);

    return(COMMAND_OK);
}

//*****************************************************************************
// \internal
//
// Handler for the "cls" command.
//
// \param argc contains the number of entries in the argv[] array.
// \param argv is an array of pointers to each of the individual words parsed
// from the command line.  The first entry will be the command itself and later
// entries represent any parameters.
//
// This function is called by the command line parser when the user enters
// the "cls" command.  It clears the display.
//
// \return Returns COMMAND_OK.
//
//*****************************************************************************
int
CmdCls(int argc, char *argv[])
{
    //
    // Clear the frame buffer.
    //
    RendererFillRect((tRectangle *)0, 0);

    //
    // Update the display from the newly cleared frame buffer.
    //
    RendererUpdate();

    return(COMMAND_OK);
}
