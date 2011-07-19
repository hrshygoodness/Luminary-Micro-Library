//*****************************************************************************
//
// eflash.c - This file holds the main routine for downloading an image to a
//            Stellaris Device via Ethernet.
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
// This is part of revision 6594 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include <winsock2.h>
#include <errno.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include "eflash.h"
#include "bootp_server.h"

//*****************************************************************************
//
// The version of the application.
//
//*****************************************************************************
const unsigned short g_usApplicationVersion = 6594;

//*****************************************************************************
//
// Program strings used in various display routines.
//
//*****************************************************************************
static const char g_pcProgramName[] =
    "EFLASH Ethernet Boot Loader Download Utility";

static const char g_pcProgramCopyright[] =
    "Copyright (c) 2009 Texas Instruments Incorporated.  All rights reserved.";

static const char g_pcProgramHelp[] =
"usage: eflash [options] file\n"
"\n"
"Download a file to a remote device, using the Ethernet Boot Loader.\n"
"The file should be a binary image, and the IP and MAC address of the\n"
"target device must be specified.\n"
"Example: eflash -i 169.254.19.63 --mac=00.1a.b6.00.12.04 enet_lwip.bin\n"
"\n"
"Required options:\n"
"  -i addr, --ip=addr     IP address of remote device to program,\n"
"                         in dotted-decimal notation\n"
"                         (e.g. 169.254.19.63)\n"
"  -m addr, --mac=addr    MAC address of remote device to program,\n"
"                         specified as a series of hexadecimal numbers\n"
"                         delimited with '-', ':', or '.'.\n"
"                         (e.g. 00.1a.b6.00.12.04)\n"
"  file                   binary file to be downloaded to the remote device.\n"
"                         (e.g. enet_lwip.bin)\n"
"\n"
"Output control:\n"
"      --quiet, --silent  suppress all normal output\n"
"      --verbose          display additional status information\n"
"      --debug            display additional diagnostic information\n"
"\n"
"Miscellaneous:\n"
"      --version          display program version information, then exit\n"
"      --help             display this help text, then exit\n"
"\n"
"Support Information:\n"
"Report any bugs to <support_lmi@ti.com>\n";

//*****************************************************************************
//
// Command line option to set "printf" output display level.
//
//*****************************************************************************
int g_iOptVerbose = 1;

//*****************************************************************************
//
// MAC address of target (remote) device.
//
//*****************************************************************************
unsigned char g_pucRemoteMAC[6] = {0, 0, 0, 0, 0, 0};

//*****************************************************************************
//
// IP address of target (remote) device.
//
//*****************************************************************************
unsigned long g_ulRemoteAddress = 0;

//*****************************************************************************
//
// File name to download to target (remote) device.
//
//*****************************************************************************
char *g_pcFileName = NULL;

//*****************************************************************************
//
// A flag that is true if the application should terminate.
//
//*****************************************************************************
static unsigned long g_bAbortMain = 0;

//*****************************************************************************
//
// This function will convert a delimited address string into an array of
// unsigned bytes.  The numbers in the address string can be delimited by
// ".", "-", or ":".  The value for each "token" in the string will be stored
// into subsequent elements in the array.  The function will return the total
// number of tokens converted.
//
//*****************************************************************************
static int
AddressToBytes(char *pcString, void *pValue, int iCount, int iBase)
{
    int iConverted = 0;
    char *token = NULL;
    char *delimit = ".-:";
    char *tail = NULL;

    //
    // Extract the first token from the string to get the loop started.  Then,
    // For each token (up to iCount), convert it and find the next token in
    // the string.  Exit the loop when iCount has been reached, or when there
    // are no more tokens to convert.
    // 
    token = strtok(pcString, delimit);
    while((iConverted < iCount) && (NULL != token))
    {
        //
        // Convert the token into a number.  If the conversion fails, the
        // input value of "tail" will match "token", and that means that
        // the input string has been formatted incorrectly, so break out of
        // the process loop and simply return the number of bytes that have
        // been converted thus far.
        //
        ((unsigned char *)pValue)[iConverted] =
            (strtoul(token, &tail, iBase) & 0xFF);
        if(tail == token)
        {
            break;
        }

        //
        // Get the next token and setup for the next iteration in the loop.
        //
        token = strtok(NULL, delimit);
        iConverted++;
    }

    //
    // Return the number
    return(iConverted);
}

//******************************************************************************
//
// This function will display help text in conformance with the GNU coding
// standard.
//
//******************************************************************************
static void
DisplayHelp(void)
{
    puts(g_pcProgramHelp);
}

//******************************************************************************
//
// This function will display version information in conformance with the GNU
// coding standard.
//
//******************************************************************************
static void
DisplayVersion(void)
{
    printf("%s (Version: %u)\n", g_pcProgramName, g_usApplicationVersion);
    printf("\n");
    printf("%s\n", g_pcProgramCopyright);
    printf("\n");
}

//******************************************************************************
//
// This function will parse the command line arguments, storing any needed
// information in the appropriate variables for later reference.  The
// "getopts" command line processing library functions are used to parse
// the command line options.  The options are defined in accordance with
// the GNU coding standard.
//
//******************************************************************************
static void
ParseOptions(int argc, char **argv)
{
    struct option sLongOptions[] =
    {
        //
        // GNU Standard Options that set a flag for program operation.
        //
        {"quiet",         no_argument, &g_iOptVerbose, 0},
        {"silent",        no_argument, &g_iOptVerbose, 0},
        {"verbose",       no_argument, &g_iOptVerbose, 2},

        //
        // GNU Standard options that simply display information and exit.
        //
        {"help",          no_argument, 0,              0x100},
        {"version",       no_argument, 0,              0x101},

        //
        // Program specific options that set variables and/or require arguments.
        //
        {"mac",     required_argument, 0,              'm'},
        {"ip",      required_argument, 0,              'i'},

        //
        // Terminating Element of the array
        //
        {0, 0, 0, 0}
    };
    int iOptionIndex = 0;
    int iOption;
    int iReturnCode;

    // 
    // Continue parsing options till there are no more to parse.
    // Note:  The "m:i" allows the short and long options to be
    // used for the file, mac and ip parameters and will be processed
    // below by the same case statement.
    //
    while((iOption = getopt_long(argc, argv, "m:i:", sLongOptions,
                                 &iOptionIndex)) != -1)
    {
        //
        // Process the current option.
        //
        switch(iOption)
        {
            //
            // Long option with flag set.
            //
            case 0:
                break;

            //
            // --help
            // 
            case 0x100:
                DisplayHelp();
                exit(0);
                break;

            //
            // --version
            // 
            case 0x101:
                DisplayVersion();
                exit(0);
                break;

            //
            // --mac=string, -m string
            // 
           case 'm':
                iReturnCode = AddressToBytes(optarg, g_pucRemoteMAC, 6, 16);
                if(iReturnCode != 6)
                {
                    EPRINTF(("Error Processing MAC (%d)\n", iReturnCode));
                    DisplayHelp();
                    exit(-(__LINE__));
                }
                break;

            //
            // --ip=string, -i string
            // 
           case 'i':
                iReturnCode = AddressToBytes(optarg, &g_ulRemoteAddress, 4, 10);
                if(iReturnCode != 4)
                {
                    EPRINTF(("Error Processing IP (%d)\n", iReturnCode));
                    DisplayHelp();
                    exit(-(__LINE__));
                }
                break;

            //
            // Unrecognized option.
            //
           default:
                DisplayVersion();
                DisplayHelp();
                exit(-(__LINE__));
                break;
        }
    }

    //
    // Extract filename from the last argument on the command line (if
    // provided).
    //
    if(optind == argc)
    {
        EPRINTF(("No File Name Specified\n"));
        DisplayHelp();
        exit(-(__LINE__));
    }
    else if(optind > (argc -1))
    {
        EPRINTF(("Too Many Command Line Options\n"));
        DisplayHelp();
        exit(-(__LINE__));
    }
    else
    {
        g_pcFileName = argv[optind];
    }

    //
    // Check for non-zero MAC address.
    //
    if((0 == g_pucRemoteMAC[0]) && (0 == g_pucRemoteMAC[1]) &&
       (0 == g_pucRemoteMAC[2]) && (0 == g_pucRemoteMAC[3]) &&
       (0 == g_pucRemoteMAC[4]) && (0 == g_pucRemoteMAC[5]))
    {
        EPRINTF(("No MAC Address Specified\n"));
        DisplayHelp();
        exit(-(__LINE__));
    }

    //
    // Check for non-zero IP address.
    //
    if(0 == g_ulRemoteAddress)
    {
        EPRINTF(("No IP Address Specified\n"));
        DisplayHelp();
        exit(-(__LINE__));
    }
}

//*****************************************************************************
//
// A callback function to monitor the progress.
//
//*****************************************************************************
static void
StatusCallback(unsigned long ulPercent)
{
    //
    // Print out the percentage.
    //
    if(g_iOptVerbose == 1)
    {
        printf("%% Complete: %3d%%\r", ulPercent);
    }
    else if(g_iOptVerbose > 1)
    {
        printf("%% Complete: %3d%%\n", ulPercent);
    }
}

//*****************************************************************************
//
// A callback function to monitor the progress.
//
//*****************************************************************************
static void
SignalIntHandler(int iSignal)
{
    //
    // Display a diagnostic message.
    //
    fprintf(stderr, "Abort Received (%d)... cleaning up\n", iSignal);

    //
    // Abort the BOOTP process (if already running).
    //
    AbortBOOTPUpdate();

    //
    // Flag main to abort any processes that are running.
    //
    g_bAbortMain = 1;
}

//*****************************************************************************
//
// Main entry.  Process command line options, and start the bootp_server.
//
//*****************************************************************************
int
main(int argc, char **argv)
{
    HOSTENT *pHostEnt;
    WSADATA wsaData;
    unsigned long ulLocalAddress;

    //
    // Parse the command line options.
    //
    if(argc > 1)
    {
        ParseOptions(argc, argv);
    }
    else
    {
        DisplayVersion();
        DisplayHelp();
        return(0);
    }

    //
    // Display (if needed) verbose function entry.
    //
    if(g_iOptVerbose > 1)
    {
        DisplayVersion();
    }

    //
    // Install an abort handler.
    //
	signal(SIGINT, SignalIntHandler);

    //
    // Startup winsock.
    //
    VPRINTF(("Starting WINSOCK\n"));
    if(WSAStartup(0x202, &wsaData) != 0)
    {
        EPRINTF(("Winsock Failed to Start\n"));
        WSACleanup();
        return(1);
    }

    //
    // Determine what my local IP address is.
    //
    pHostEnt = gethostbyname("");
    ulLocalAddress = ((struct in_addr *)*pHostEnt->h_addr_list)->s_addr;

    //
    // Start the BOOTP/TFTP server to perform an update.
    //
    QPRINTF(("Starting BOOTP/TFTP Server ...\n"));
    StatusCallback(0);
    StartBOOTPUpdate(g_pucRemoteMAC, ulLocalAddress, g_ulRemoteAddress,
                     g_pcFileName, StatusCallback);

    //
    // Cleanup winsock.
    //
    VPRINTF(("Closing WINSOCK\n"));
    WSACleanup();

    //
    // Clean up and return.
    //
    if(g_bAbortMain)
    {
        return(2);
    }
    return(0);
}
