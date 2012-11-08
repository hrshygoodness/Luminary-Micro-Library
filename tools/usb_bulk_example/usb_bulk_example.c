//*****************************************************************************
//
// usb_bulk_example.c - A very simple command line application that attempts
//      to open the Stellaris Generic Bulk USB device and exchange data
//      with it.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

//****************************************************************************
//
// If the device is successfully opened, the user is prompted for a string
// which is then sent to the device. The device toggles the case of any
// alphabet characters in the the data and sends it back. We receive this
// and print out the result.
//
// Communication with the device is carried out via a simple DLL, lmusbdll(64),
// which is responsible for opening and closing the device and for
// sending and receiving blocks of data.  This structure allows the
// application code to be kept independent of the underlying USB driver
// in use.  In this implementation, Microsoft's WinUSB interface is used
// but a switch to another stack, for example the open source libusb-win32
// interface could be made merely be replacing the lmusbdll(64).dll with
// another version writing to the different USB stack.
//
// One downside to using WinUSB is that it requires you to have the Windows
// Device Driver Kit (DDK) installed to build applications which use the
// interface. This is a very large download making it awkward for people
// with slow internet connections. The use of lmusbdll(64).dll also offers
// the advantage that applications using it can still be modified and
// rebuilt even without the DDK installed on the development system.  To
// update and rebuild lmusbdll itself, however, the DDK is still required.
//
//****************************************************************************
#include <windows.h>
#include <strsafe.h>
#include <initguid.h>
#include "lmusbdll.h"
#include "luminary_guids.h"

//****************************************************************************
//
// Buffer size definitions.
//
//****************************************************************************
#define MAX_STRING_LEN 256
#define MAX_ENTRY_LEN 256
#define USB_BUFFER_LEN 1216

//****************************************************************************
//
// The build version number
//
//****************************************************************************
#define BLDVER "9453"

//****************************************************************************
//
// The number of bytes we read and write per transaction if in echo mode.
//
//****************************************************************************
#define ECHO_PACKET_SIZE 64

//****************************************************************************
//
// Buffer into which error messages are written.
//
//****************************************************************************
TCHAR g_pcErrorString[MAX_STRING_LEN];

//****************************************************************************
//
// The number of bytes transfered in the last measurement interval.
//
//****************************************************************************
ULONG g_ulByteCount = 0;

//****************************************************************************
//
// The total number of packets transfered.
//
//****************************************************************************
ULONG g_ulPacketCount = 0;

//****************************************************************************
//
// Return a string describing the supplied system error code.
//
// \param dwError is a Windows system error code.
//
// This function returns a pointer to a string describing the error code
// passed in the dwError parameter. If no description string can be found
// the string "Unknown" is returned.
//
// \return Returns a pointer to a string describing the error.
//
//****************************************************************************
LPTSTR GetSystemErrorString(DWORD dwError)
{
    DWORD dwRetcode;

    //
    // Ask Windows for the error message description.
    //
    dwRetcode = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, "%0", dwError, 0,
                              g_pcErrorString, MAX_STRING_LEN, NULL);

    if(dwRetcode == 0)
    {
        return((LPTSTR)L"Unknown");
    }
    else
    {
        //
        // Remove the trailing "\n\r" if present.
        //
        if(dwRetcode >= 2)
        {
            if(g_pcErrorString[dwRetcode - 2] == '\r')
            {
                g_pcErrorString[dwRetcode - 2] = '\0';
            }
        }

        return(g_pcErrorString);
    }
}

//****************************************************************************
//
// Print the throughput in terms of Kbps once per second.
//
//****************************************************************************
void UpdateThroughput(void)
{
    static ULONG ulStartTime = 0;
    static ULONG ulLast = 0;
    ULONG ulNow;
    ULONG ulElapsed;
    SYSTEMTIME sSysTime;

    //
    // Get the current system time.
    //
    GetSystemTime(&sSysTime);
    ulNow = (((((sSysTime.wHour * 60) +
               sSysTime.wMinute) * 60) +
              sSysTime.wSecond) * 1000) + sSysTime.wMilliseconds;

    //
    // If this is the first call, set the start time.
    //
    if(ulStartTime == 0)
    {
        ulStartTime = ulNow;
        ulLast = ulNow;
        return;
    }

    //
    // How much time has elapsed since the last measurement?
    //
    ulElapsed = (ulNow > ulStartTime) ? (ulNow - ulStartTime) : (ulStartTime - ulNow);

    //
    // We dump a new measurement every second.
    //
    if(ulElapsed > 1000)
    {

        //printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
        printf("\r%6dKbps Packets: %10d ", ((g_ulByteCount * 8) / ulElapsed), g_ulPacketCount);
        g_ulByteCount = 0;
        ulStartTime = ulNow;
    }
}

//****************************************************************************
//
// The main application entry function.
//
// \param None.
//
// This function forms the main entry point of the example application. It
// initializes the USB bulk device, prompts the user for an ASCII string,
// sends this string to the device, reads the response from the device (which
// will be the same string with the character order reversed if using the
// sample device provided in StellarisWare) and displays the returned
// string.
//
// \return Set the exit code to 0 of no errors cause the application to end
// or a non-zero value otherwise.
//
//****************************************************************************
int main(int argc, char *argv[])
{
    BOOL bResult;
    BOOL bDriverInstalled;
    BOOL bEcho;
    char szBuffer[USB_BUFFER_LEN];
    ULONG ulWritten;
    ULONG ulRead;
    ULONG ulLength;
    DWORD dwError;
    LMUSB_HANDLE hUSB;

    //
    // Are we operating in echo mode or not? The "-e" parameter tells the
    // app to echo everything it receives back to the device unchanged.
    //
    bEcho = ((argc > 1) && (argv[1][1] == 'e')) ? TRUE : FALSE;

    //
    // Print a cheerful welcome.
    //
    printf("\nStellaris Bulk USB Device Example\n");
    printf( "---------------------------------\n\n");
    printf("Version %s\n\n", BLDVER);
    if(!bEcho)
    {
        printf("This is a partner application to the usb_dev_bulk example\n");
        printf("shipped with StellarisWare software releases for USB-enabled\n");
        printf("boards. Strings entered here are sent to the board which\n");
        printf("inverts the case of the characters in the string and returns\n");
        printf("them to the host.\n\n");
    }
    else
    {
        printf("If run with the \"-e\" command line switch, this application\n");
        printf("echoes all data received on the bulk IN endpoint to the bulk\n");
        printf("OUT endpoint.  This feature may be helpful during development\n");
        printf("and debug of your own USB devices.  Note that this will not\n");
        printf("do anything exciting if run with the usb_dev_bulk example\n");
        printf("device attached since it expects the host to initiate transfers.\n\n");
    }

    //
    // Find our USB device and prepare it for communication.
    //
    hUSB = InitializeDevice(BULK_VID, BULK_PID,
                            (LPGUID)&(GUID_DEVINTERFACE_STELLARIS_BULK),
                            &bDriverInstalled);

    if(hUSB)
    {
        //
        // Are we operating in echo mode or not? The "-e" parameter tells the
        // app to echo everything it receives back to the device unchanged.
        //
        if(bEcho)
        {
            //
            // Yes - we are in echo mode.
            //
            printf("Running in echo mode. Press Ctrl+C to exit.\n\n");

            while(1)
            {
                //
                // Read a block of data from the device.
                //
                dwError = ReadUSBPacket(hUSB, szBuffer, ECHO_PACKET_SIZE, &ulRead,
                                        INFINITE, NULL);

                if(dwError != ERROR_SUCCESS)
                {
                    //
                    // We failed to read from the device.
                    //
                    printf("\n\nError %d (%S) reading from bulk IN pipe.\n", dwError,
                           GetSystemErrorString(dwError));
                    break;
                }
                else
                {
                    //
                    // Update our byte and packet counters.
                    //
                    g_ulByteCount += ulRead;
                    g_ulPacketCount++;

                    //
                    // Write the data back out to the device.
                    //
                    bResult = WriteUSBPacket(hUSB, szBuffer, ulRead, &ulWritten);
                    if(!bResult)
                    {
                        //
                        // We failed to write the data for some reason.
                        //
                        dwError = GetLastError();
                        printf("\n\nError %d (%S) writing to bulk OUT pipe.\n", dwError,
                               GetSystemErrorString(dwError));
                        break;
                    }

                    //
                    // Display the throughput.
                    //
                    UpdateThroughput();
                }
            }
        }
        else
        {
            //
            // We are running in normal mode.  Keep sending and receiving
            // strings until the user indicates that it is time to exit.
            //
            while(1)
            {

                //
                // The device was found and successfully configured. Now get a string from
                // the user...
                //
                do
                {
                    printf("\nEnter a string (EXIT to exit): ");
                    fgets(szBuffer, MAX_ENTRY_LEN, stdin);
                    printf("\n");

                    //
                    // How many characters were entered (including the trailing '\n')?
                    //
                    ulLength = (ULONG)strlen(szBuffer);

                    if(ulLength <= 1)
                    {
                        //
                        // The string is either nothing at all or a single '\n' so reprompt the user.
                        //
                        printf("\nPlease enter some text.\n");
                        ulLength = 0;
                    }
                    else
                    {
                        //
                        // Get rid of the trailing '\n' if there is one there.
                        //
                        if(szBuffer[ulLength - 1] == '\n')
                        {
                            szBuffer[ulLength - 1] = '\0';
                            ulLength--;
                        }
                    }
                }
                while(ulLength == 0);

                //
                // Are we being asked to exit the application?
                //
                if(!(strcmp("EXIT", szBuffer)))
                {
                    //
                    // Yes - drop out and exit.
                    //
                    printf("Exiting on user request.\n");
                    break;
                }

                //
                // Write the user's string to the device.
                //
                bResult = WriteUSBPacket(hUSB, szBuffer, ulLength, &ulWritten);
                if(!bResult)
                {
                    //
                    // We failed to write the data for some reason.
                    //
                    dwError = GetLastError();
                    printf("Error %d (%S) writing to bulk OUT pipe.\n", dwError,
                           GetSystemErrorString(dwError));
                }
                else
                {
                    //
                    // We wrote data successfully so now read it back.
                    //
                    printf("Wrote %d bytes to the device. Expected %d\n",
                           ulWritten, ulLength);

                    //
                    // We expect the same number of bytes as we just sent.
                    //
                    dwError = ReadUSBPacket(hUSB, szBuffer, ulWritten, &ulRead,
                                            INFINITE, NULL);

                    if(dwError != ERROR_SUCCESS)
                    {
                        //
                        // We failed to read from the device.
                        //
                        printf("Error %d (%S) reading from bulk IN pipe.\n", dwError,
                               GetSystemErrorString(dwError));
                    }
                    else
                    {
                        //
                        // Add a string terminator to the returned data (this
                        // should already be there but, just in case...)
                        //
                        szBuffer[ulRead] = '\0';

                        printf("Read %d bytes from device. Expected %d\n",
                               ulRead, ulWritten);
                        printf("\nReturned string: \"%s\"\n", szBuffer);
                    }
                }
            }
        }
    }
    else
    {
        //
        // An error was reported while trying to connect to the device.
        //
        dwError = GetLastError();

        printf("\nUnable to initialize the Stellaris Bulk USB Device.\n");
        printf("Error code is %d (%S)\n\n", dwError, GetSystemErrorString(dwError));
        printf("Please make sure you have a Stellaris USB-enabled evaluation\n");
        printf("or development kit running the usb_dev_bulk example\n");
        printf("application connected to this system via the \"USB OTG\" or\n");
        printf("\"USB DEVICE\" connectors. Once the device is connected, run\n");
        printf("this application again.\n\n");

        printf("\nPress \"Enter\" to exit: ");
        fgets(szBuffer, MAX_STRING_LEN, stdin);
        printf("\n");
        return(2);
    }

    TerminateDevice(hUSB);

    return(0);
}
