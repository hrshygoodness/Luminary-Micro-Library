//*****************************************************************************
//
// bootp_server.c - A simplistic BOOTP/TFTP server for use by the BOOTP client
//                  in the boot loader.
//
// Copyright (c) 2007-2010 Texas Instruments Incorporated.  All rights reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include "bootp_server.h"
#include "eflash.h"

//*****************************************************************************
//
// This structure defines the fields in a BOOTP request/reply packet.
//
//*****************************************************************************
typedef struct
{
    //
    // The operation; 1 is a request, 2 is a reply.
    //
    unsigned char ucOp;

    //
    // The hardware type; 1 is Ethernet.
    //
    unsigned char ucHType;

    //
    // The hardware address length; for Ethernet this will be 6, the length of
    // the MAC address.
    //
    unsigned char ucHLen;

    //
    // Hop count, used by gateways for cross-gateway booting.
    //
    unsigned char ucHops;

    //
    // The transaction ID.
    //
    unsigned long ulXID;

    //
    // The number of seconds elapsed since the client started trying to boot.
    //
    unsigned short usSecs;

    //
    // The BOOTP flags.
    //
    unsigned short usFlags;

    //
    // The client's IP address, if it knows it.
    //
    unsigned long ulCIAddr;

    //
    // The client's IP address, as assigned by the BOOTP server.
    //
    unsigned long ulYIAddr;

    //
    // The TFTP server's IP address.
    //
    unsigned long ulSIAddr;

    //
    // The gateway IP address, if booting cross-gateway.
    //
    unsigned long ulGIAddr;

    //
    // The hardware address; for Ethernet this is the MAC address.
    //
    unsigned char pucCHAddr[16];

    //
    // The name, or nickname, of the server that should handle this BOOTP
    // request.
    //
    char pcSName[64];

    //
    // The name of the boot file to be loaded via TFTP.
    //
    char pcFile[128];

    //
    // Optional vendor-specific area; not used for BOOTP.
    //
    unsigned char pucVend[64];
}
tBOOTPPacket;

//*****************************************************************************
//
// The BOOTP commands.
//
//*****************************************************************************
#define BOOTP_REQUEST           1
#define BOOTP_REPLY             2

//*****************************************************************************
//
// The TFTP commands.
//
//*****************************************************************************
#define TFTP_RRQ                1
#define TFTP_WRQ                2
#define TFTP_DATA               3
#define TFTP_ACK                4
#define TFTP_ERROR              5

//*****************************************************************************
//
// The UDP ports used by the BOOTP protocol.
//
//*****************************************************************************
#define BOOTP_SERVER_PORT       67
#define BOOTP_CLIENT_PORT       68

//*****************************************************************************
//
// The UDP port for the TFTP server.
//
//*****************************************************************************
#define TFTP_PORT               69

//*****************************************************************************
//
// The UDP port used to send the remote firmware update request signal. This
// is the well-known port associated with "discard" function and is also used
// by some Wake-On-LAN implementations.
//
//*****************************************************************************
#define MPACKET_PORT             9

//*****************************************************************************
//
// The length of the remote firmware update request magic packet. This contains
// a 6 byte header followed by 4 copies of the target MAC address.
//
//*****************************************************************************
#define MPACKET_HEADER_LEN 6
#define MPACKET_MAC_REP    4
#define MPACKET_MAC_LEN    6

#define MPACKET_LEN (MPACKET_HEADER_LEN + (MPACKET_MAC_REP * MPACKET_MAC_LEN))

//*****************************************************************************
//
// The marker byte used at the start of the magic packet. This is repeated
// MPACKET_HEADER_LEN times.
//
//*****************************************************************************
#define MPACKET_MARKER     0xAA

//*****************************************************************************
//
// A flag that is true if the BOOTP server should be aborted even though the
// update has not completed.
//
//*****************************************************************************
static unsigned long g_bAbortBOOTP;

//*****************************************************************************
//
// Creates a UDP socket and binds it to a particular port.
//
//*****************************************************************************
static SOCKET
CreateSocket(unsigned long ulAddress, unsigned long ulPort,
             unsigned long bBroadcast, BOOL bBind)
{
    struct sockaddr_in sAddr;
    SOCKET sRet;

    //
    // Create a socket.
    //
    if((sRet = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
    {
        return(INVALID_SOCKET);
    }

    //
    // Set the broadcast flag as specified.
    //
    if(setsockopt(sRet, SOL_SOCKET, SO_BROADCAST, (const char *)&bBroadcast,
               sizeof(unsigned long)) == SOCKET_ERROR)
	{
        int iTmp = WSAGetLastError();
        closesocket(sRet);
        return(INVALID_SOCKET);
	}

    //
    // Bind the socket to the given port.
    //
    if(bBind)
    {
        memset(&sAddr, 0, sizeof(sAddr));
        sAddr.sin_family = AF_INET;
        sAddr.sin_addr.s_addr = ulAddress;
        sAddr.sin_port = (USHORT)ulPort;
        if(bind(sRet, (struct sockaddr *)&sAddr, sizeof(sAddr)) == SOCKET_ERROR)
        {
            int iTmp = WSAGetLastError();
            closesocket(sRet);
            return(INVALID_SOCKET);
        }
    }

    //
    // Return the created socket.
    //
    return(sRet);
}

//*****************************************************************************
//
// Builds a TFTP data packet.
//
//*****************************************************************************
static unsigned long
BuildTFTPDataPacket(char *pcFileData, unsigned long ulFileLen,
                    unsigned char *pcPacketData, unsigned long ulBlockNum)
{
    unsigned long ulLength;

    //
    // Determine the number of bytes to place into this packet.
    //
    if(ulFileLen < (ulBlockNum * 512))
    {
        ulLength = ulFileLen & 511;
    }
    else
    {
        ulLength = 512;
    }

    //
    // Fill in the packet header.
    //
    pcPacketData[0] = (TFTP_DATA >> 8) & 0xff;
    pcPacketData[1] = TFTP_DATA & 0xff;
    pcPacketData[2] = (unsigned char)((ulBlockNum >> 8) & 0xff);
    pcPacketData[3] = (unsigned char)(ulBlockNum & 0xff);

    //
    // Copy the file data into the packet.
    //
    memcpy(pcPacketData + 4, pcFileData + ((ulBlockNum - 1) * 512), ulLength);

    //
    // Return the length of the packet.
    //
    return(ulLength + 4);
}

//*****************************************************************************
//
// Sends a magic packet to the client whose MAC address is supplied. This 
// packet informs the client that a remote firmware update is being requested.
//
// sSocket is the socket that the magic packet is to be sent on. This must be
// a UDP broadcast socket bound to port 9 (well-known port which is supposed 
// to discard all packets sent to it. This port is used by some wake-on-LAN
// implementations).
//
// pucMACAddr is the MAC address of the device to which the magic packet will
// be sent.
//
// This functions sends a UDP broadcast packet to the client identified by
// pucMACAddr. The packet is based on the wake-on-LAN magic packet but altered
// to ensure that it will not be recognized by WOL LAN adapters. The structure
// is as follows:
//
// AA AA AA AA AA AA followed by the target MAC repeated 4 times.
//
// (a WOL packet uses 6 bytes of FF as a start marker followed by the MAC 
// address repeated 16 times. We reduce this to 4 repetitions here to reduce
// the buffering requirement on the target).
//
// On receipt of this packet, a client may chose to pass control to the boot
// loader which will issue a BOOTP request to indicate that it is ready to
// start a remote firmware update.
//
// Returns TRUE on success of FALSE on error.
//
//*****************************************************************************
BOOL
SendFirmwareUpdateMagicPacket(SOCKET sSocket, unsigned char *pucMACAddr)
{
    unsigned char pucPacket[MPACKET_LEN];
    int iLoop, iMACLoop, iWritten;
    struct sockaddr_in sAddr;

    //
    // Build the magic packet. Start with the 6 marker bytes.
    //
    for(iLoop = 0; iLoop < MPACKET_HEADER_LEN; iLoop++)
    {
        pucPacket[iLoop] = MPACKET_MARKER;
    }

    //
    // Populate the rest of the packet with 8 copies of the target MAC
    // address.
    //
    for(iLoop = 0; iLoop < MPACKET_MAC_REP; iLoop++)
    {
        for(iMACLoop = 0; iMACLoop < MPACKET_MAC_LEN; iMACLoop++)
        {
            pucPacket[MPACKET_HEADER_LEN +
                      (iLoop * MPACKET_MAC_LEN) + iMACLoop] = 
                                                    pucMACAddr[iMACLoop];
        }
    }

    //
    // Set up the target address.
    //
    sAddr.sin_port = htons(MPACKET_PORT);
    sAddr.sin_addr.s_addr = INADDR_BROADCAST;
    sAddr.sin_family = AF_INET;

    //
    // Now send the packet.
    //
    iWritten = sendto(sSocket, (const char *)pucPacket, MPACKET_LEN, 0,
                      (SOCKADDR *)&sAddr, sizeof(sAddr));

    //
    // Tell the caller whether or not we were successful.
    //
    return((iWritten == MPACKET_LEN) ? TRUE : FALSE);
}

//*****************************************************************************
//
// Provides a simple BOOTP and TFTP server for use by the boot loader to
// perform a firmware update.
//
// pucMACAddr is the MAC address of the device for which BOOTP requests will
// be serviced.  If this is 0, then a BOOTP request from any source will be
// processed.
//
// ulLocalAddress it the IP address of the local machine.  This must be the
// address associated with the interface card used to communicate with the
// target.
//
// ulRemoteAddr is the IP address to be given to the target.
//
// pcFilename is the name of the local file to be sent to the target; it should
// be a binary firmare image.
//
// pfnCallback is the function that is periodically called to provide a
// completion percentage.  If this is 0, then the callback is disabled.
//
//*****************************************************************************
unsigned long
StartBOOTPUpdate(unsigned char *pucMACAddr, unsigned long ulLocalAddr,
                 unsigned long ulRemoteAddr, char *pcFilename,
                 tfnCallback pfnCallback)
{
    unsigned long ulFileLen, ulBlockNum, ulLen, ulCount;
    unsigned char pcPacketData[700];
	char *pcFileData;
    SOCKET sBOOTP, sTFTP, sTFTPData, sUpdateSocket;
    struct sockaddr_in sAddr;
    tBOOTPPacket *pPacket;
    fd_set sDescriptors;
    TIMEVAL sTime;
    int iAddrLen;
    FILE *pFile;
    BOOL bBOOTPPacketSeen = FALSE;

    //
    // Open the file to be used as the update image.
    //
    VPRINTF(("Reading file (%s) to be downloaded\n", pcFilename));
    pFile = fopen(pcFilename, "rb");
    if(!pFile)
    {
        EPRINTF(("File (%s) not found\n", pcFilename));
        return(ERROR_FILE);
    }

    //
    // Get the length of the file.
    //
    fseek(pFile, 0, SEEK_END);
    ulFileLen = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    //
    // Allocate a buffer to hold the file contents.
    //
    VPRINTF(("... Allocating buffer for file length %u\n", ulFileLen));
    pcFileData = (char *)malloc(ulFileLen);
    if(!pcFileData)
    {
        EPRINTF(("Cannot allocate memory for file\n"));
        fclose(pFile);
        return(ERROR_FILE);
    }

    //
    // Read the file contents into the buffer.
    //
    VPRINTF(("... Reading the file\n"));
    if(fread(pcFileData, 1, ulFileLen, pFile) != ulFileLen)
    {
        free(pcFileData);
        fclose(pFile);
        return(ERROR_FILE);
    }

    //
    // Close the file.
    //
    fclose(pFile);

    //
    // Create a socket for the BOOTP server.
    //
    VPRINTF(("Setting up network connections\n"));
    VPRINTF(("... Creating sockete for BOOTP server\n"));
    sBOOTP = CreateSocket(ulLocalAddr, htons(BOOTP_SERVER_PORT), 1, TRUE);
    if(sBOOTP == INVALID_SOCKET)
    {
        EPRINTF(("Cannot allocate socket\n"));
        free(pcFileData);
        return(ERROR_BOOTPD);
    }

    //
    // Create a socket for the TFTP server.
    //
    VPRINTF(("... Creating sockete for TFTP server\n"));
    sTFTP = CreateSocket(ulLocalAddr, htons(TFTP_PORT), 0, TRUE);
    if(sTFTP == INVALID_SOCKET)
    {
        EPRINTF(("Cannot allocate socket\n"));
        free(pcFileData);
        closesocket(sBOOTP);
        return(ERROR_TFTPD);
    }

    //
    // Create the socket we use to send remote firmware update magic packets.
    VPRINTF(("... Creating sockete for MAGIG packet transmission\n"));
    sUpdateSocket = CreateSocket(ulLocalAddr, htons(MPACKET_PORT), 1, FALSE);
    if(sUpdateSocket == INVALID_SOCKET)
    {
        EPRINTF(("Cannot allocate socket\n"));
        free(pcFileData);
        closesocket(sTFTP);
        closesocket(sBOOTP);
        return(ERROR_MPACKET);
    }

    //
    // Start off with no TFTP data socket.
    //
    sTFTPData = INVALID_SOCKET;

    //
    // Create a pointer to the packet data to use for interpreting it as a
    // BOOTP request.
    //
    pPacket = (tBOOTPPacket *)pcPacketData;

    //
    // Clear the flag that indicates that the BOOTP server should be aborted.
    //
    g_bAbortBOOTP = 0;
  
    //
    // Send an initial remote firmware magic packet.
    //
    VPRINTF(("Sending \"Magic\" packet to initiate Ethernet Boot Loader\n"));
    SendFirmwareUpdateMagicPacket(sUpdateSocket, pucMACAddr);

    //
    // Loop until the BOOTP server is aborted.  Additionally, once the update
    // has completed, the server will exit on its own.
    //
    VPRINTF(("Starting BOOTP/TFTP Server\n"));
    while(!g_bAbortBOOTP)
    {
        //
        // Set the file descriptor set to the currently opened sockets.
        //
        FD_ZERO(&sDescriptors);
        FD_SET(sBOOTP, &sDescriptors);
        FD_SET(sTFTP, &sDescriptors);
        iAddrLen = (sTFTP > sBOOTP) ? sTFTP : sBOOTP;
        if(sTFTPData != INVALID_SOCKET)
        {
            FD_SET(sTFTPData, &sDescriptors);
            iAddrLen = (sTFTPData > iAddrLen) ? sTFTPData : iAddrLen;
        }

        //
        // Set a timeout of 500ms.
        //
        sTime.tv_sec = 0;
        sTime.tv_usec = 500000;

        //
        // Display additional status about what type of packet we are
        // waiting on.
        //
        VPRINTF(("Waiting for packet ...\n"));

        //
        // Wait until there is a packet to be read on one of the open sockets.
        //
        if(select(iAddrLen + 1, &sDescriptors, 0, 0, &sTime) != 0)
        {
            //
            // See if there is a packet waiting to be read from the BOOTP port.
            //
            if(FD_ISSET(sBOOTP, &sDescriptors))
            {
                //
                // Read the packet from the BOOTP port.
                //
                VPRINTF(("... Reading BOOTP packet\n"));
                iAddrLen = sizeof(sAddr);
                if(recvfrom(sBOOTP, (char *)pcPacketData,
                            sizeof(pcPacketData), 0, (struct sockaddr *)&sAddr,
                            &iAddrLen) != SOCKET_ERROR)
                {
                    //
                    // Make sure this is a valid BOOTP request.
                    //
                    VPRINTF(("... Verifying BOOTP packet\n"));
                    if((pPacket->ucOp == BOOTP_REQUEST) &&
                       (pPacket->ucHType == 1) &&
                       (pPacket->ucHLen == 6) &&
                       (pucMACAddr ?
                        memcmp(pPacket->pucCHAddr, pucMACAddr, 6) == 0 : 1) &&
                       (stricmp(pPacket->pcSName, "stellaris") == 0))
                    {
                        VPRINTF(("... Generating BOOTP response\n"));
                        //
                        // Change the operation code from BOOTP request to
                        // BOOTP response.
                        //
                        pPacket->ucOp = BOOTP_REPLY;

                        //
                        // Fill in the client's address and our address (i.e.
                        // the TFTP server address) in the BOOTP response.
                        //
                        pPacket->ulYIAddr = ulRemoteAddr;
                        pPacket->ulSIAddr = ulLocalAddr;

                        //
                        // Provide a image filename to the client.  This is
                        // ignored by our TFTP server, but some string is
                        // required.
                        //
                        strcpy(pPacket->pcFile, "firmware.bin");

                        //
                        // Remember that we have seen a BOOTP request from the
                        // client so that we don't send any more firmware
                        // update magic packets.
                        //
                        bBOOTPPacketSeen = TRUE;

                        //
                        // Send the response back to the client using a
                        // broadcast message.
                        //
                        VPRINTF(("... Sending BOOTP response\n"));
                        sAddr.sin_addr.s_addr = INADDR_BROADCAST;
                        sendto(sBOOTP, (char *)pcPacketData,
                               sizeof(tBOOTPPacket), 0,
                               (struct sockaddr *)&sAddr, iAddrLen);
                    }
                }
            }

            //
            // See if there is a packet waiting to be read from the TFTP port.
            //
            if(FD_ISSET(sTFTP, &sDescriptors))
            {
                //
                // Read the packet from the TFTP port.
                //
                VPRINTF(("... Reading TFTP packet\n"));
                iAddrLen = sizeof(sAddr);
                if(recvfrom(sTFTP, (char *)pcPacketData, sizeof(pcPacketData),
                            0, (struct sockaddr *)&sAddr, &iAddrLen) !=
                        SOCKET_ERROR)
                {
                    //
                    // Make sure this is a RRQ request.
                    //
                    VPRINTF(("... Verifying TFTP RRQ request\n"));
                    if((pcPacketData[0] != ((TFTP_RRQ >> 8) & 0xff)) ||
                       (pcPacketData[1] != (TFTP_RRQ & 0xff)))
                    {
                        //
                        // Construct an error packet since only RRQ is
                        // supported by this simple TFTP server.
                        //
                        VPRINTF(("... Generate ERROR packet since RRQ\n"));
                        VPRINTF(("... packet not received\n"));
                        pcPacketData[0] = (TFTP_ERROR >> 8) & 0xff;
                        pcPacketData[1] = TFTP_ERROR & 0xff;
                        pcPacketData[2] = 0;
                        pcPacketData[3] = 4;
                        strcpy((char *)pcPacketData + 4,
                               "Only RRQ is supported!");

                        //
                        // Send the error packet back to the client.
                        //
                        sendto(sTFTP, (char *)pcPacketData,
                               strlen((char *)pcPacketData + 4) + 5, 0,
                               (struct sockaddr *)&sAddr, iAddrLen);
                    }
                    else
                    {
                        //
                        // If there is already a TFTP data connection, then
                        // close it now (i.e. restart).
                        //
                        if(sTFTPData != INVALID_SOCKET)
                        {
                            VPRINTF(("... Restarting TFTP session\n"));
                            closesocket(sTFTPData);
                        }

                        //
                        // Create a TFTP data socket.
                        //
                        VPRINTF(("... Create TFTP data socket\n"));
                        while((sTFTPData =
                            CreateSocket(ulLocalAddr,
                                         htons(32768 + (rand() & 8191)), 0,
                                         TRUE)) == INVALID_SOCKET)
                        {
                        }

                        //
                        // Start by sending block number one.
                        //
                        ulBlockNum = 1;

                        //
                        // Generate the TFTP data packet.
                        //
                        VPRINTF(("... Building TFTP packet (Block %d)\n",
                                ulBlockNum));
                        ulLen = BuildTFTPDataPacket(pcFileData, ulFileLen,
                                                    pcPacketData, ulBlockNum);

                        //
                        // Send the TFTP data packet.
                        //
                        VPRINTF(("... Sending TFTP data packet (Block %d)\n",
                                ulBlockNum));
                        sendto(sTFTPData, (char *)pcPacketData, ulLen, 0,
                               (struct sockaddr *)&sAddr, iAddrLen);

                        //
                        // Set the count of timeouts that we've waited for the
                        // ACK for this data packet.
                        //
                        ulCount = 0;
                    }
                }
            }

            //
            // See if there is a packet waiting to be read from the TFTP data
            // port.
            //
            if((sTFTPData != INVALID_SOCKET) &&
               FD_ISSET(sTFTPData, &sDescriptors))
            {
                //
                // Read the packet from the TFTP data port.
                //
                VPRINTF(("... Reading TFTP data packet\n"));
                iAddrLen = sizeof(sAddr);
                if(recvfrom(sTFTPData, (char *)pcPacketData,
                            sizeof(pcPacketData), 0, (struct sockaddr *)&sAddr,
                            &iAddrLen) != SOCKET_ERROR)
                {
                    //
                    // See if this is an ACK.
                    //
                    VPRINTF(("... Verify TFTP packet is ACK\n"));
                    if((pcPacketData[0] == ((TFTP_ACK >> 8) & 0xff)) &&
                       (pcPacketData[1] == (TFTP_ACK & 0xff)) &&
                       (pcPacketData[2] == ((ulBlockNum >> 8) & 0xff)) &&
                       (pcPacketData[3] == (ulBlockNum & 0xff)))
                    {
                        //
                        // Exit if this is the ACK for the last data packet.
                        //
                        if(ulFileLen < (ulBlockNum * 512))
                        {
                            if(pfnCallback)
                            {
                                pfnCallback(100);
                            }
                            break;
                        }

                        //
                        // Update the completion percentage.
                        //
                        if(pfnCallback)
                        {
                            pfnCallback((ulBlockNum * 512 * 100) / ulFileLen);
                        }

                        //
                        // Go to the next block of data.
                        //
                        ulBlockNum++;

                        //
                        // Generate the TFTP data packet.
                        //
                        VPRINTF(("... Building TFTP packet (Block %d)\n",
                                ulBlockNum));
                        ulLen = BuildTFTPDataPacket(pcFileData, ulFileLen,
                                                    pcPacketData, ulBlockNum);

                        //
                        // Send the TFTP data packet.
                        //
                        VPRINTF(("... Sending TFTP data packet (Block %d)\n",
                                ulBlockNum));
                        sendto(sTFTPData, (char *)pcPacketData, ulLen, 0,
                               (struct sockaddr *)&sAddr, iAddrLen);

                        //
                        // Set the count of timeouts that we've waited for the
                        // ACK for this data packet.
                        //
                        ulCount = 0;
                    }
                }
            }
        }
        else
        {
            //
            // Increment the count of timeouts that we've waited for an ACK.
            //
            ulCount++;

            //
            // See if there is a TFTP data socket and the timeout count has
            // exceeded 1 (i.e. that we've waited for a second for the ACK).
            //
            if((sTFTPData != INVALID_SOCKET) && (ulCount > 1))
            {
                //
                // Generate a TFTP data packet for the most recently sent
                // block.
                //
                VPRINTF(("... Rebuilding TFTP data packet (Block %d)\n",
                        ulBlockNum));
                ulLen = BuildTFTPDataPacket(pcFileData, ulFileLen,
                                            pcPacketData, ulBlockNum);

                //
                // Resend the most recent TFTP data packet.
                //
                VPRINTF(("Resending TFTP data packet (Block %d)\n",
                        ulBlockNum));
                sendto(sTFTPData, (char *)pcPacketData, ulLen, 0,
                       (struct sockaddr *)&sAddr, iAddrLen);

                //
                // Set the count of timeouts that we've waited for the ACK for
                // this data packet.
                //
                ulCount = 0;
            }

            //
            // If we have not seen a BOOTP request yet, send the firmare
            // update magic packet again.
            //
            if(!bBOOTPPacketSeen)
            {
                VPRINTF(("... Resending \"Magic\" packet.\n"));
                SendFirmwareUpdateMagicPacket(sUpdateSocket, pucMACAddr);
            }
        }
    }

    //
    // Free the buffer used to hold the file contents.
    //
    free(pcFileData);

    //
    // Close the sockets.
    //
    VPRINTF(("Closing network connections\n"));
    closesocket(sTFTPData);
    closesocket(sTFTP);
    closesocket(sBOOTP);
    closesocket(sUpdateSocket);

    //
    // Return success.
    //
    return(ERROR_NONE);
}

//*****************************************************************************
//
// Abort an in-progress BOOTP update.
//
//*****************************************************************************
void
AbortBOOTPUpdate(void)
{
    //
    // Set the flag to tell the BOOTP/TFTP servers to exit.
    //
    g_bAbortBOOTP = 1;
}
