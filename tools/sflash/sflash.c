//*****************************************************************************
//
// sflash.c - This file holds the main routine for downloading an image to a
//            Stellaris(R) Device.
//
// Copyright (c) 2006-2012 Texas Instruments Incorporated.  All rights reserved.
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

//*****************************************************************************
//
// Serial Boot Loader Download Utility
//!
//! \page intro
//! This document describes how the example download utility uses the serial
//! boot loader in order to download a binary to a Stellaris(tm) device. The
//! example source code provided implements the packet based interface needed
//! to communicate with the device via the UART port.  The next few sections
//! will cover how to initiate communications with and send commands and data
//! to the boot loader.  All of the commands that are supported by the boot
//! loader will be covered in this document and are used by the example
//! download utility source code.
//
//! \page intro_app  
//! This next section covers the documentation of the actual application
//! without going into the details of all of the functions that are called.
//! The Code Documentation section covers the actual functions used and the
//! meaning of the parameters passed to functions.
//
//! \defgroup AutoBaud Automatic Baud Rate Detection
//! The boot loader provides a method to automatically detect the baud rate
//! being used to communicate with it.  The automatic baud rate detection is
//! implemented in source code in the function AutoBaud().   The call to
//! OpenUART() which is made before any call to AutoBaud() will program the
//! UART as 8 data bits, no parity and 1 stop bit and  to timeout after 500ms
//! so that calls will timeout if no response is received from the boot loader.
//!
//! The code begins by sending down the two byte synchronization pattern 
//! (0x55, 0x55) to the boot loader as seen in the first call to
//! UARTSendData().  The code then waits for the boot loader to send a byte
//! back to test if the synchronization completed successfully.  If no data is
//! received from the boot loader the program will exit and indicate that no
//! connection was found.  This is useful when program has been run and no
//! device is connected to the UART cable or the boot loader is no longer
//! programmed into the device.  In this example code,  once the program has
//! detected a successful synchronization it sends out a "ping" command to
//! ensure that normal communication are functioning before proceeding with
//! downloading.  This last step is not required but is provided as an example
//! of using the ``ping'' command.
//
//! \defgroup SendPacket Sending Packets
//! Once the baud rate has been determined, the program can start sending and
//! receiving packets from the boot loader.  The code example for sending
//! packets is implemented in the SendPacket() function provided as part of
//! this sample download utility. This function is located in packet_handler.c
//! and demonstrates the series of bytes that must be sent out and read back to
//! properly communicate with the boot loader. The following steps must be
//! performed to successfully send a packet:
//! 1) Send out the size of the packet that will be sent to the device.  The
//!    size is always the size of the data + 2.
//! 2) Send out the checksum of the data buffer to help insure proper
//!    transmission of the command. The checksum algorithm is implemented in
//!    the CheckSum() function provided in this example and is simply 
//!    Data[0]+Data[1]+...Data[n].
//! 3) Send out the actual data bytes.
//! 4) Wait for a single byte acknowledgment from the device that it either
//!    properly received the data or that it detected and error in the
//!    transmission.
//
//! \defgroup RecvPacket Receiving Packets
//! Received packets use the same format as sent packets.  The code example for
//! receiving packets is implemented in the GetPacket() function provided as
//! part of this sample download utility and is located in the packet_handler.c
//! file. The GetPacket() function demonstrates the steps necessary to receive
//! a packet from the serial boot loader. The following steps must be performed
//! to successfully receive a packet:
//! 1) Wait for non-zero data to be returned from the device. This is important
//!    as the device may send non-zero bytes between a sent and received data
//!    packet. The first non-zero byte received will be the size of the packet
//!    that is being received.
//! 2) Read the next byte which will be the check sum for the packet.
//! 3) Read the data bytes from the device. There will be packet size - 2 bytes
//!    of data sent during the data phase. For example, if the packet size,
//!    first non-zero byte received, was 3 then there is only 1 byte of data
//!    to be received.
//! 4) Calculate the check sum of the data bytes and insure if it matches the
//!    check sum received in the packet.
//! 5) Send an acknowledgment or not-acknowledge to the device to indicate the
//!    successful or unsuccessful reception of the packet.
//
//! \defgroup AckPacket Acknowledge Packet
//! The steps necessary to acknowledge reception of a packet is implemented in
//! the AckPacket() function in the file packet_handler.c. To send an
//! acknowledgment, a single byte with the value COMMAND_ACK should be sent
//! out on the serial port. Acknowledge bytes are sent out whenever a packet is
//! successfully received from the serial boot loader.
//
//! \defgroup NakPacket Not Acknowledge Packet
//! The steps necessary to not-acknowledge reception of a packet is implemented
//! in NakPacket() function in the file packet_handler.c. To send a
//! not-acknowledge, a single byte with the value COMMAND_NAK should be sent
//! out. A not-acknowledge byte is sent out whenever a received packet is
//! detected to have an error, usually as a result of checksum error. This
//! allows serial boot loader to re-transmit the previous packet.
//
//! \defgroup SendCmd Sending Commands
//! Sending a command involves sending and receiving packets to send the
//! command and retrieve the status of the command that was sent to the boot
//! loader.  The function SendCommand() provides an example of how to send any
//! command to the serial boot loader. The first step is always to generate a
//! properly formatted command in a memory buffer that will be passed to the
//! SendCommand() function. In the case of simple command like the "ping"
//! command, this only a single byte filled with the value COMMAND_PING. This
//! command will then be passed on to the packet function for further
//! formatting before it is transmitted to the boot loader. The following
//! example shows the format of a ``ping'' command: 
//!
//! \code
//!  ucCommand = COMMAND_PING;
//!  if(SendCommand(&ucCommand, 1) < 0)
//!  {
//!      return(-1);
//!  }
//! \endcode
//!
//! More complicated commands require the building an array of bytes in memory
//! and passing them into the SendCommand() function.  These are demonstrated
//! later in this document.
//!
//! The final step when sending commands is to check if the previous command
//! completed successfully. This is accomplished by sending a single byte
//! command, COMMAND_GET_STATUS, to the boot loader followed by reading back a
//! single byte packet from the boot loader.
//!
//! \code
//!  //
//!  // Send the get status command to tell the device to return status to
//!  // the host.
//!  //
//!  ucStatus = COMMAND_GET_STATUS;
//!  if(SendPacket(&ucStatus, 1, 1) < 0)
//!  {
//!      return(-1);
//!  }
//! 
//!  //
//!  // Read back the status provided from the device.
//!  //
//!  ucSize = 1;
//!  if(GetPacket(&ucStatus, &ucSize) < 0)
//!  {
//!      return(-1);
//!  }
//! \endcode
//
//! \defgroup DownApp Downloading Applications
//! The sample download application uses the commands described above to
//! download an application to the Stellaris device.  This downloaded
//! application can either overwrite the boot loader or download and execute
//! from a different area of the flash to preserve the boot loader for future
//! use.  
//!
//! Care must be taken when specifying the base address of a downloaded
//! application to insure the boot loader is left on the device. This is
//! because the serial boot loader will allow the base address to be anywhere
//! in flash.  However, if the address is anywhere in the first 2048 bytes,
//! the serial boot loader will be overwritten.  If the address is above 2048
//! bytes, subsequent boots of the device will continue to execute the boot
//! loader.
//!
//! The following code snippet shows how the download command,
//! COMMAND_DOWNLOAD, is formatted before being passed to the SendCommand()
//! function. The g_uiDownloadAddress specifies the starting programming
//! location and is sent following the COMMAND_DOWNLOAD byte as a 32 bit number
//! starting with the MSB and ending with the LSB. The next 4 bytes are the
//! length of the binary image to download, which is also sent MSB first.
//!
//! \code
//!  //
//!  // Build up the download command and send it to the board.
//!  //
//!  g_ucBuffer[0] = COMMAND_DOWNLOAD;
//!  g_ucBuffer[1] = (unsigned char)(g_uiDownloadAddress >> 24);
//!  g_ucBuffer[2] = (unsigned char)(g_uiDownloadAddress >> 16);
//!  g_ucBuffer[3] = (unsigned char)(g_uiDownloadAddress >> 8);
//!  g_ucBuffer[4] = (unsigned char)g_uiDownloadAddress;
//!  g_ucBuffer[5] = (unsigned char)(iFileLength>>24);
//!  g_ucBuffer[6] = (unsigned char)(iFileLength>>16);
//!  g_ucBuffer[7] = (unsigned char)(iFileLength>>8);
//!  g_ucBuffer[8] = (unsigned char)iFileLength;
//!  if(SendCommand(g_ucBuffer, 9) < 0)
//!  {
//!      return(-1);
//!  }
//! \endcode
//!
//! This next code snippet demonstrates how to send data some of the data
//! following a download command. First the command byte COMMAND_SEND_DATA is
//! added to the buffer followed by 1 to 8 bytes of data. In this code snippet
//! 8 bytes are sent to the boot loader. See the full example code in update.c
//! for an example of how to download the complete contents of a file.
//!
//! \code
//!  g_ucBuffer[0] = COMMAND_SEND_DATA;
//!
//!  //
//!  // Send out 8 bytes at a time.
//!  //
//!  if(fread(&g_ucBuffer[1], 1, 8, hFile) != 8)
//!  {
//!      return(-1);
//!  }
//!  if(SendCommand(g_ucBuffer, 9) < 0)
//!  {
//!      return(-1);
//!  }
//! \endcode
//!
//! The second parameter used in this example is the execution address. This
//! can be any valid address in flash.  The boot loader will execute from this
//! address without checking if there is any valid code at the given address.
//! The first byte of the command is COMMAND_RUN, followed by the 32 bit
//! execution address which is sent MSB first.   The boot loader does not
//! provide a method to return execution to the boot loader after a run command
//! is issued.  This means that to return to the boot loader the code that is
//! called as a result of a run command will need to perform a software reset
//! to restart the boot loader.  The following code shows an example of a run
//! command:
//!
//! \code
//!  //
//!  // Send the run command but just send the packet, there will likely
//!  // be no boot loader to answer after this command completes.
//!  //
//!  g_ucBuffer[0] = COMMAND_RUN;
//!  g_ucBuffer[1] = (unsigned char)(g_uiStartAddress>>24);
//!  g_ucBuffer[2] = (unsigned char)(g_uiStartAddress>>16);
//!  g_ucBuffer[3] = (unsigned char)(g_uiStartAddress>>8);
//!  g_ucBuffer[4] = (unsigned char)g_uiStartAddress;
//!  if(SendPacket(g_ucBuffer, 5, 0) < 0)
//!  {
//!      printf("Failed to Send Run command\n");
//!  }
//! \endcode
//!
//! A final command that is used in the example code occurs when no execution
//! address is specified as a parameter.  In this case the device is simply
//! reset. This is useful when the boot loader has been overwritten and the
//! newly programmed code needs to be executed. This is accomplished by sending
//! a reset command to the boot loader. This is a simple one byte command that
//! will cause the boot loader to issue a software reset. After sending this
//! command the boot loader will either be reset and waiting for serial input
//! or the new code that was programmed will be executed.
//!
//! \code
//!  //
//!  // Send the reset command but just send the packet, there will be no boot
//!  // loader to answer after this command completes.
//!  //
//!  g_ucBuffer[0] = COMMAND_RESET;
//!  if(SendPacket(g_ucBuffer, 1, 0) < 0)
//!  {
//!      printf("Failed to Send Reset command\n");
//!  }
//! \endcode
//!
//! The example code provided with this documentation is a fully functional
//! download utility that can be tailored to fit whatever need the user has.
//! This example utility implements all commands that can be used to
//! communicate with the boot loader.  The example code uses a standard UART
//! for communications so that this program can run on any PC with a 16550
//! compatible UART.
//
//*****************************************************************************

//*****************************************************************************
//
//! \page main_intro 
//! The next sections of this document cover the actual routines that are used
//! by the serial flash updater host program.  They include everything from the
//! top level command line parser, the serial flash loader command parser, the
//! packet handler and the UART data transport.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup main_handler Main Control Routine
//! These routines are the highest level functions that handle command line
//! parsing and the interactions with the serial flash loader at the command
//! level.
//! @{
//
//*****************************************************************************
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "uart_handler.h"
#include "packet_handler.h"

int SendCommand(unsigned char *pucCommand, unsigned char ucSize);
int UpdateFlash(FILE *hBootFile, FILE *hFile, unsigned long ulAddress);
int CheckArgs(void);

//*****************************************************************************
//
//! This variable holds the global buffer used to hold any data sent or
//! received from the device.
//
//*****************************************************************************
unsigned char g_ucBuffer[256];

unsigned int g_uiBaudRate;
unsigned int g_uiDataSize;
int g_iDisableAutoBaud;

//*****************************************************************************
//
//! This string will be printed if any errors are present in the command line 
//! that is passed into the program.
//
//*****************************************************************************
static char const UsageString[] =
{
"\n"
"Usage: sflash filename -p [program address] -r [execution address]\n"
#ifdef __WIN32
"    -c [COM port number] -d -l [Boot Loader filename] -b [baud rate]\n"
#else
"    -c [tty] -d -l [Boot Loader filename] -b [baud rate]\n"
#endif
"    -s [data size]\n\n"
"-p [program address]:\n"
"    if address is not specified it is assumed to be 0x00000000\n"
"    if there is no 0x prefix is added then the address is assumed to be \n"
"    in decimal\n"
"-r [execution address]:\n"
"    if address is not specified then no run command will be sent.\n"
#ifdef __WIN32
"-c [COM port number]:\n"
"    This is the number of the COM port to use.\n"
#else
"-c [tty]:\n"
"    This is the name of the TTY device to use.\n"
#endif
"-l [Boot Loader filename]:\n"
"    This specifies a boot loader binary that will be loaded to the device\n"
"    before downloading the application specified by the filename parameter.\n"
"-b [baud rate]:\n"
"    Specifies the baud rate in decimal.\n"
"-d  Disable Auto-Baud support\n"
"-s [data size]:\n"
"    Specifies the number of data bytes to be sent in each data packet.  Must\n"
"    be a multiple of 4 between 4 and 252 (inclusive).\n\n"
"    Example: Download test.bin using COM 1 to address 0x800 and run at 0x820\n"
"        sflash test.bin -p 0x800 -r 0x820 -c 1\n"
};

//*****************************************************************************
//
//! This variable will point to the filename parameter that is passed in as a
//! parameter to this program.
//
//*****************************************************************************
static char * g_pFilename;

//*****************************************************************************
//
//! This variable will point to the filename parameter that is passed in as a
//! boot loader file name.
//
//*****************************************************************************
static char * g_pBootLoadName;

//*****************************************************************************
//
//! This variable will be set to the starting address of the binary will be
//! programmed into to device.
//
//*****************************************************************************
static unsigned int g_uiDownloadAddress;

//*****************************************************************************
//
//! This variable will be set to the start execution address for the binary
//! that is downloaded.
//
//*****************************************************************************
static unsigned int g_uiStartAddress;

//*****************************************************************************
//
//! This variable is modified by a command line parameter to match the COM
//! port that has been requested.
//
//*****************************************************************************
static char g_szCOMName[32] =
{
#ifdef __WIN32
    "\\\\.\\COM1"
#else
    "/dev/ttyS0"
#endif
};

//****************************************************************************
//
//! AutoBaud() performs Automatic baud rate detection. 
//!
//! This function will send the sync pattern to the board and establish basic
//! communication with the device.  The call to OpenUART() in the routine
//! main() set the baud rate that will be used.
//!
//! \return If any part of the function fails, the function will return a
//!     negative error code. The function will return 0 to indicate success.
//
//****************************************************************************
int
AutoBaud(void)
{
    static unsigned char const ucSyncPattern[]={0x55, 0x55};
    unsigned char ucCommand;
    unsigned char ucAck;

    //
    // Send out the sync pattern and wait for an ack from the board.
    //
    if(UARTSendData(ucSyncPattern, 2))
    {
        return(-1);
    }

    //
    // Wait for the ACK to be received, if something besides an ACK or a zero
    // is received then something went wrong.
    //
    do
    {
        UARTReceiveData(&ucAck, 1);
    } while(ucAck == 0);

    if (ucAck != COMMAND_ACK)
    {
        return(-1);
    }

    //
    // Make sure we can at least communicate with the board.
    //
    ucCommand = COMMAND_PING;
    if(SendCommand(&ucCommand, 1) < 0)
    {
        return(-1);
    }
    return(0);
}

//****************************************************************************
//
//! SendCommand() sends a command to the serial boot loader.
//!
//! \param pucCommand is the properly formatted serial flash loader command to
//!     send to the device.
//! \param ucSize is the size, in bytes, of the command to be sent. 
//!
//! This function will send a command to the device and read back the status
//! code from the device to see if the command completed successfully.
//!
//! \return If any part of the function fails, the function will return a
//!     negative error code.  The function will return 0 to indicate success.
//
//****************************************************************************
int
SendCommand(unsigned char *pucCommand, unsigned char ucSize)
{
    unsigned char ucStatus;

    //
    // Send the command itself.
    //
    if(SendPacket(pucCommand, ucSize, 1) < 0)
    {
        return(-1);
    }

    //
    // Send the get status command to tell the device to return status to
    // the host.
    //
    ucStatus = COMMAND_GET_STATUS;
    if(SendPacket(&ucStatus, 1, 1) < 0)
    {
        printf("Failed to Get Status\n");
        return(-1);
    }

    //
    // Read back the status provided from the device.
    //
    ucSize = sizeof(ucStatus);
    if(GetPacket(&ucStatus, &ucSize) < 0)
    {
        printf("Failed to Get Packet\n");
        return(-1);
    }
    if(ucStatus != COMMAND_RET_SUCCESS)
    {
        printf("Failed to get download command Return Code: %04x\n",
            ucStatus);
        return(-1);
    }
    return(0);
}

//*****************************************************************************
//
//! parseArgs() handles command line processing.
//!
//! \param argc is the argc parameter passed into main().
//! \param argv is the argv parameter passed into main().
//!
//! This function parses the known arguments and generates an error if any
//! parameter was specified incorrectly.
//! 
//! \return A return value of zero indicates success while any other value
//!     indicates a problem with the parameters that were passed in.
//
//*****************************************************************************
int
parseArgs(int argc, char **argv)
{
    int i;
    char cArg;
    
    cArg = 0;
    
    for(i = 1; i < argc; i++)
    {
        if(argv[i][0] != '-')
        {
            if(cArg)
            {
                switch(cArg)
                {
                    case 'p':
                    {
                        g_uiDownloadAddress = strtoul(argv[i], 0, 0);
                        break;
                    }
                    case 'r':
                    {
                        g_uiStartAddress = strtoul(argv[i], 0, 0);
                        break;
                    }
                    case 'c':
                    {
#ifdef __WIN32
                        sprintf(g_szCOMName, "\\\\.\\COM%s", argv[i]);
#else
                        strncpy(g_szCOMName, argv[i], sizeof(g_szCOMName));
#endif
                        break;
                    }
                    case 'l':
                    {
                        g_pBootLoadName = argv[i];
                        break;
                    }
                    case 'b':
                    {
                        g_uiBaudRate = strtoul(argv[i], 0, 0);
                        break;
                    }
                    case 's':
                    {
                        g_uiDataSize = strtoul(argv[i], 0, 0);
                        if((g_uiDataSize < 4) || (g_uiDataSize > 252))
                        {
                            g_uiDataSize = 8;
                        }
                        g_uiDataSize &= ~3;
                        break;
                    }
                    default:
                    {
                        printf("ERROR: Invalid argument\n");
                        return(-1);
                    }
                }
                cArg = 0;
            }
            else
            {
                //
                // If we already have a filename then error out as everything
                // else should have an option before it.
                //
                if(g_pFilename)
                {
                    return(-1);
                }
                g_pFilename = argv[i];
            }
        }
        else
        {
            //
            // Single flag parameters go here.
            //
            switch(argv[i][1])
            {
                case '-':
                case '?':
                case 'h':
                {
                    return(-2);
                    break;
                }
                case 'd':
                {
                    g_iDisableAutoBaud = 1;
                    break;
                }
                default:
                {
                    cArg = argv[i][1];
                    break;
                }
            }
        }
    }
    return(0);
}

//*****************************************************************************
//
//! main() is the programs main routine.
//! 
//! \param argc is the number of parameters that were passed in via the command
//!     line.
//! \param argv is the list of strings that holds each parameter that was were
//!     passed in via the command line.
//! 
//! This is the main routine for downloading an image to the device via the
//! UART.
//!
//! \return This function either returns a negative value indicating a failure
//!     or zero if the download was successful.
//
//*****************************************************************************
int
main(int argc, char **argv)
{
    FILE *hFile;
    FILE *hFileBoot;
    
    g_uiDownloadAddress = 0;
    g_uiStartAddress = 0xffffffff;
    g_pFilename = 0;
    g_pBootLoadName = 0;
    g_uiBaudRate = 115200;
    g_uiDataSize = 8;
    g_iDisableAutoBaud = 0;

    setbuf(stdout, 0);

    //
    // Get any arguments that were passed in.
    //
    if(parseArgs(argc, argv))
    {
        printf("%s", UsageString);
        return(-1);
    }
    
    if(CheckArgs())
    {
        return(-1);
    }
    
    //
    // If a boot loader was specified then open it.
    //
    if(g_pBootLoadName)
    {
        //
        // Open the boot loader file to download.
        //
        hFileBoot = fopen(g_pBootLoadName, "rb");
        if(hFileBoot == 0)
        {
            printf("Failed to open file: %s\n", g_pBootLoadName);
            return(-1);
        }
    }
        
    //
    // Open the file to download.
    //
    hFile = fopen(g_pFilename, "rb");
    if(hFile == 0)
    {
        printf("Failed to open file: %s\n", g_pFilename);
        return(-1);
    }

    if(OpenUART(g_szCOMName, g_uiBaudRate))
    {
        printf("Failed to configure Host UART\n");
        return(-1);
    }

    //
    // Now try to auto baud with the board by sending the Sync and waiting
    // for an ack from the board.
    //
    if(g_iDisableAutoBaud == 0)
    {
        if(AutoBaud())
        {
            printf("Failed to synchronize with board.\n");
            return(-1);
        }
    }

    printf("\n");
    if(g_pBootLoadName)
    {
        printf("Boot Loader    : %s\n", g_pBootLoadName);
    }
    printf("Application    : %s\n", g_pFilename);
    printf("Program Address: 0x%x\n", g_uiDownloadAddress);
    printf("       COM Port: %s\n", g_szCOMName);
    printf("      Baud Rate: %d\n", g_uiBaudRate);

    printf("Erasing Flash:\n");

    //
    // If both a boot loader and an application were specified then update both
    // the boot loader and the application.
    //
    if(g_pBootLoadName != 0)
    {
        if(UpdateFlash(hFile, hFileBoot, g_uiDownloadAddress) < 0)
        {
            return(-1);
        }
    }
    //
    // Otherwise just update the application.
    //
    else if(UpdateFlash(hFile, 0, g_uiDownloadAddress) < 0)
    {
        return(-1);
    }
    
    //
    // If a start address was specified then send the run command to the 
    // boot loader.
    //
    if(g_uiStartAddress != 0xffffffff)
    {
        //
        // Send the run command but just send the packet, there will likely
        // be no boot loader to answer after this command completes.
        //
        g_ucBuffer[0] = COMMAND_RUN;
        g_ucBuffer[1] = (unsigned char)(g_uiStartAddress>>24);
        g_ucBuffer[2] = (unsigned char)(g_uiStartAddress>>16);
        g_ucBuffer[3] = (unsigned char)(g_uiStartAddress>>8);
        g_ucBuffer[4] = (unsigned char)g_uiStartAddress;
        if(SendPacket(g_ucBuffer, 5, 0) < 0)
        {
            printf("Failed to Send Run command\n");
        }
        else
        {
            printf("Running from address %08x\n",g_uiStartAddress);
        }
    }
    else
    {
        //
        // Send the reset command but just send the packet, there will likely
        // be no boot loader to answer after this command completes.
        //
        g_ucBuffer[0] = COMMAND_RESET;
        SendPacket(g_ucBuffer, 1, 0);
    }
    if(hFile != 0)
    {
        fclose(hFile);
    }
    printf("Successfully downloaded to device.\n");
    return(0); 
}

//*****************************************************************************
//
//! UpdateFlash() programs data to the flash.
//! 
//! \param hFile is an open file pointer to the binary data to program into the
//!     flash as the application.
//! \param hBootFile is an open file pointer to the binary data for the 
//!     boot loader binary.  This will be programmed at offset zero.
//! \param ulAddress is address to start programming data to the falsh.
//! 
//! This routine handles the commands necessary to program data to the flash.
//! If hFile should always have a value if hBootFile also has a valid value.
//! This function will concatenate the two files in memory to reduce the number
//! of flash erases that occur when both the boot loader and the application 
//! are being updated. 
//!
//! \return This function either returns a negative value indicating a failure
//!     or zero if the update was successful.
//
//*****************************************************************************
int
UpdateFlash(FILE *hFile, FILE *hBootFile, unsigned long ulAddress)
{
    unsigned long ulFileLength;
    unsigned long ulBootFileLength;
    unsigned long ulTransferStart;
    unsigned long ulTransferLength;
    unsigned char *pFileBuffer;
    unsigned long ulOffset;
    
    //
    // At least one file must be specified.
    //
    if(hFile == 0)
    {
        return(-1);
    }
        
    //
    // Get the file sizes.
    //
    fseek(hFile, 0, SEEK_END);
    ulFileLength = ftell(hFile);
    fseek(hFile, 0, SEEK_SET);

    //
    // Default the transfer length to be the size of the application.
    //
    ulTransferLength = ulFileLength;
    ulTransferStart = ulAddress;

    if(hBootFile)
    {
        fseek(hBootFile, 0, SEEK_END);
        ulBootFileLength = ftell(hBootFile);
        fseek(hBootFile, 0, SEEK_SET);

        ulTransferLength = ulAddress + ulFileLength;
        ulTransferStart = 0;
    }
    
    pFileBuffer = malloc(ulTransferLength);
    if(pFileBuffer == 0)
    {
        return(-1);
    }

    if(hBootFile)
    {
        if(fread(pFileBuffer, 1, ulBootFileLength, hBootFile) != 
            ulBootFileLength)
        {
            return(-1);
        }
        
        if(ulAddress < ulBootFileLength)
        {
            return(-1);
        }

        //
        // Pad the unused code space with 0xff to have all of the flash in 
        // a known state.
        //
        memset(&pFileBuffer[ulBootFileLength], 0xff, 
            ulAddress - ulBootFileLength);

        //
        // Append the application to the boot loader image.
        //
        if(fread(&pFileBuffer[ulAddress], 1, ulFileLength, hFile) !=
            ulFileLength)
        {
            return(-1);
        }
    }
    else
    {
        //
        // Just read in the full application since there is not boot loader.
        //
        if(fread(pFileBuffer, 1, ulTransferLength, hFile) != ulTransferLength)
        {
            return(-1);
        }
    }
        
    //
    // Build up the download command and send it to the board.
    //
    g_ucBuffer[0] = COMMAND_DOWNLOAD;
    g_ucBuffer[1] = (unsigned char)(ulTransferStart >> 24);
    g_ucBuffer[2] = (unsigned char)(ulTransferStart >> 16);
    g_ucBuffer[3] = (unsigned char)(ulTransferStart >> 8);
    g_ucBuffer[4] = (unsigned char)ulTransferStart;
    g_ucBuffer[5] = (unsigned char)(ulTransferLength>>24);
    g_ucBuffer[6] = (unsigned char)(ulTransferLength>>16);
    g_ucBuffer[7] = (unsigned char)(ulTransferLength>>8);
    g_ucBuffer[8] = (unsigned char)ulTransferLength;
    if(SendCommand(g_ucBuffer, 9) < 0)
    {
        printf("Failed to Send Download Command\n");
        return(-1);
    }

    ulOffset = 0;

    printf("Remaining Bytes: ");
    do
    {
        unsigned char ucBytesSent;
        
        g_ucBuffer[0] = COMMAND_SEND_DATA;

        printf("%08ld", ulTransferLength);
        
        //
        // Send out 8 bytes at a time to throttle download rate and avoid
        // overruning the device since it is programming flash on the fly.
        //
        if(ulTransferLength >= g_uiDataSize)
        {
            memcpy(&g_ucBuffer[1], &pFileBuffer[ulOffset], g_uiDataSize);

            ulOffset += g_uiDataSize;
            ulTransferLength -= g_uiDataSize;
            ucBytesSent = g_uiDataSize + 1;
        }
        else
        {
            memcpy(&g_ucBuffer[1], &pFileBuffer[ulOffset], ulTransferLength);
            ulOffset += ulTransferLength;
            ucBytesSent = ulTransferLength + 1;
            ulTransferLength = 0;
        }
        //
        // Send the Send Data command to the device.
        //
        if(SendCommand(g_ucBuffer, ucBytesSent) < 0)
        {
            printf("Failed to Send Packet data\n");
            break;
        }

        printf("\b\b\b\b\b\b\b\b");
    } while (ulTransferLength);
    printf("00000000\n");
    
    if(pFileBuffer)
    { 
        free(pFileBuffer);
    }
    return(0);
}

//*****************************************************************************
//
//! CheckArgs() ensures that the arguments passed in are correct.
//! 
//! This routine makes sure that the combination of command line parameters 
//! that were passed into the program are valid.  If only a Boot Loader binary
//! was specified then the parameters will be modified to be application update
//! update at address zero.  This step is done to simplfy logic later in the
//! update program.
//!
//! \return This function either returns a negative value indicating a failure
//!     or zero if the update was successful.
//
//*****************************************************************************
int
CheckArgs(void)
{
    //
    // No file names specified.
    //
    if((g_pFilename == 0) && (g_pBootLoadName == 0))
    {
        printf("ERROR: no file names specified.\n");
        return(-1);
    }
    
    //
    // Both filenames are specified but the address is zero.
    //
    if((g_pFilename != 0) && (g_pBootLoadName != 0) && 
        (g_uiDownloadAddress == 0))
    {
        printf(
        "ERROR: Download address cannot be zero and specify a boot loader \n"
        "binary as well as an application binary\n");
        return(-1);
    }
    
    //
    // If only a boot loader was specified then set the address to 0 and 
    // specify only one file to download.
    //
    if((g_pBootLoadName != 0) && (g_pFilename == 0))
    {
        g_pFilename = g_pBootLoadName;
        g_pBootLoadName = 0;
        g_uiDownloadAddress = 0;
    }
    return(0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
