//*****************************************************************************
//
// finder.c - Program to locate Ethernet-based Stellaris board on the network.
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
// This is part of revision 8555 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifndef __WIN32
#include <pthread.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif
#ifndef __WIN32
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif
#include <sys/types.h>
#ifndef __WIN32
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#endif
#include <FL/x.H>
#include <FL/fl_ask.H>
#include "gui.h"

//*****************************************************************************
//
// These defines are used to describe the device locator protocol.
//
//*****************************************************************************
#define TAG_CMD                 0xff
#define TAG_STATUS              0xfe
#define CMD_DISCOVER_TARGET     0x02
#define RESP_ID_TARGET_BLDC     0x00
#define RESP_ID_TARGET_STEPPER  0x01
#define RESP_ID_TARGET_ACIM     0x02

//*****************************************************************************
//
// A structure that contains the details of a local network interface.
//
//*****************************************************************************
typedef struct
{
    //
    // The UDP socket on this network interface.
    //
    int iSocket;

    //
    // The name of this network interface.
    //
#ifndef __WIN32
    char pcIFName[32];
#endif
}
tSocketData;

//*****************************************************************************
//
// A structure that contains the details of a board found on the network.
//
//*****************************************************************************
typedef struct
{
    //
    // The type of board, typically used by motor control boards.
    //
    unsigned char ucBoardType;

    //
    // The board ID.
    //
    unsigned char ucBoardID;

    //
    // The MAC address of the board.
    //
    unsigned char pucMACArray[6];

    //
    // The IP address of the board.
    //
    unsigned long ulIPAddr;

    //
    // The IP address of the client presently connected to the board.
    //
    unsigned long ulClientIPAddr;

    //
    // The version of the firmware on the board.
    //
    unsigned long ulVersion;

    //
    // The title of the application on the board.
    //
    char pcAppTitle[64];
}
tBoardData;

//*****************************************************************************
//
// An array of active network interfaces.  There is room in this array for 16
// network interfaces, which should be more than enough for most any machine.
//
//*****************************************************************************
tSocketData g_sSockets[16];

//*****************************************************************************
//
// The number of sockets in g_sSockets that are valid.
//
//*****************************************************************************
unsigned long g_ulNumSockets;

//*****************************************************************************
//
// An array of data describing the boards found on the network(s).  There is
// room in this array for 256 board, which should be enough for typical network
// setups.
//
//*****************************************************************************
tBoardData g_psBoardData[256];

//*****************************************************************************
//
// The number of board descriptions in g_psBoardData.
//
//*****************************************************************************
unsigned long g_ulNumBoards;

//*****************************************************************************
//
// A boolean that is true when the network should be rescanned for devices.
//
//*****************************************************************************
volatile unsigned long g_ulRefresh = 1;

//*****************************************************************************
//
// This function creates a UDP socket for each active network interface.
//
//*****************************************************************************
void
CreateIFSockets(void)
{
#ifdef __WIN32
    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize, dwRetValue, dwIndex;
    struct sockaddr_in sAddr;
    unsigned long ulTemp;
    int iSocket;

    //
    // Initialize the number of sockets to zero.
    //
    g_ulNumSockets = 0;

    //
    // Allocate space for a single-entry IP address table.
    //
    pIPAddrTable = (MIB_IPADDRTABLE *)malloc(sizeof(MIB_IPADDRTABLE));
    if(pIPAddrTable == NULL)
    {
        return;
    }

    //
    // Determine the size of the IP address table required.
    //
    dwSize = 0;
    if(GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
    {
        pIPAddrTable = (MIB_IPADDRTABLE *)realloc(pIPAddrTable, dwSize);
        if(pIPAddrTable == NULL)
        {
            return;
        }
    }

    //
    // Read the IP address table.
    //
    if(GetIpAddrTable(pIPAddrTable, &dwSize, 0) != NO_ERROR)
    {
        free(pIPAddrTable);
        return;
    }

    //
    // Loop through the IP addresses in the table.
    //
    for(dwIndex = 0; dwIndex < pIPAddrTable->dwNumEntries; dwIndex++)
    {
        //
        // Skip this entry if it is unconfigured or the loopback device.
        //
        if((pIPAddrTable->table[dwIndex].dwAddr == 0x00000000) ||
           (pIPAddrTable->table[dwIndex].dwAddr == 0x0100007f))
        {
            continue;
        }

        //
        // Create a socket.
        //
        if((iSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
            continue;
        }

        //
        // Bind this socket to the IP address of this interface.
        //
        sAddr.sin_family = AF_INET;
        sAddr.sin_addr.s_addr = pIPAddrTable->table[dwIndex].dwAddr;
        sAddr.sin_port = htons(23);
        if(bind(iSocket, (struct sockaddr *)&sAddr,
                sizeof(struct sockaddr_in)) < 0)
        {
            close(iSocket);
            continue;
        }

        //
        // Put this socket into non-blocking mode.
        //
        ulTemp = 0;
        if(ioctlsocket(iSocket, FIONBIO, &ulTemp) < 0)
        {
            close(iSocket);
            continue;
        }

        //
        // Enable broadcast transmission on this socket.
        //
        ulTemp = 1;
        if(setsockopt(iSocket, SOL_SOCKET, SO_BROADCAST, (char *)&ulTemp,
                      sizeof(ulTemp)) < 0)
        {
            close(iSocket);
            continue;
        }

        //
        // Save this socket.
        //
        g_sSockets[g_ulNumSockets++].iSocket = iSocket;

        //
        // Ignore the remaining network interfaces if the socket array is now
        // full.
        //
        if(g_ulNumSockets == (sizeof(g_sSockets) / sizeof(g_sSockets[0])))
        {
            break;
        }
    }

    //
    // Free memory allocated for IP Address Table.
    //
    free(pIPAddrTable);
#else
    int iSocket, iIdx, iNumInterfaces;
    struct sockaddr_in *pAddr;
    unsigned long ulTemp;
    char pcBuffer[1024];
    struct ifconf sIFC;
    struct ifreq *pIFR;

    //
    // Initialize the number of sockets to zero.
    //
    g_ulNumSockets = 0;

    //
    // Create a generic UDP socket to use to query the network interfaces.
    //
    if((iSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        return;
    }

    //
    // Get the list of network interfaces.
    //
    sIFC.ifc_len = sizeof(pcBuffer);
    sIFC.ifc_buf = pcBuffer;
    if(ioctl(iSocket, SIOCGIFCONF, &sIFC) < 0)
    {
        close(iSocket);
        return;
    }

    //
    // Close the socket.
    //
    close(iSocket);

    //
    // Get the size and pointer to the network interface list.
    //
    iNumInterfaces = sIFC.ifc_len / sizeof(struct ifreq);
    pIFR = sIFC.ifc_req;

    //
    // Loop through the network interfaces.
    //
    for(iIdx = 0; iIdx < iNumInterfaces; iIdx++)
    {
        //
        // Get the address pointer for this network interface.
        //
        pAddr = (struct sockaddr_in *)&(pIFR[iIdx].ifr_addr);

        //
        // Skip this network interface if it is not configured (the network
        // address is zero) or it is the loopback interface (the network
        // address is 127.0.0.1).
        //
        if((pAddr->sin_addr.s_addr == 0x00000000) ||
           (pAddr->sin_addr.s_addr == 0x0100007f))
        {
            continue;
        }

        //
        // Create a socket.
        //
        if((iSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
            continue;
        }

        //
        // Bind this socket to the IP address of this interface.
        //
        pAddr->sin_port = htons(23);
        if(bind(iSocket, (struct sockaddr *)pAddr,
                sizeof(struct sockaddr_in)) < 0)
        {
            close(iSocket);
            continue;
        }

        //
        // Put this socket into non-blocking mode.
        //
        ulTemp = 0;
        if(ioctl(iSocket, FIONBIO, &ulTemp) < 0)
        {
            close(iSocket);
            continue;
        }

        //
        // Enable broadcast transmission on this socket.
        //
        ulTemp = 1;
        if(setsockopt(iSocket, SOL_SOCKET, SO_BROADCAST, (char *)&ulTemp,
                      sizeof(ulTemp)) < 0)
        {
            close(iSocket);
            continue;
        }

        //
        // Save this socket.
        //
        strncpy(g_sSockets[g_ulNumSockets].pcIFName, pIFR[iIdx].ifr_name,
                sizeof(g_sSockets[g_ulNumSockets].pcIFName));
        g_sSockets[g_ulNumSockets].pcIFName[sizeof(g_sSockets[0].pcIFName) -
                                            1] = 0;
        g_sSockets[g_ulNumSockets++].iSocket = iSocket;

        //
        // Ignore the remaining network interfaces if the socket array is now
        // full.
        //
        if(g_ulNumSockets == (sizeof(g_sSockets) / sizeof(g_sSockets[0])))
        {
            break;
        }
    }
#endif
}

//*****************************************************************************
//
// This function sends device discover messages on the UDP sockets for each
// active network interface.
//
//*****************************************************************************
void
SendDiscovers(void)
{
    struct sockaddr_in sAddr;
    char pcBuffer[4];
    int iIdx;

    //
    // Loop over the UDP sockets.
    //
    for(iIdx = 0; iIdx < g_ulNumSockets; iIdx++)
    {
        //
        // Construct the discover message.
        //
        pcBuffer[0] = TAG_CMD;
        pcBuffer[1] = 4;
        pcBuffer[2] = CMD_DISCOVER_TARGET;
        pcBuffer[3] = (0 - TAG_CMD - 4 - CMD_DISCOVER_TARGET) & 0xff;

        //
        // Set the address for this message.
        //
        sAddr.sin_family = AF_INET;
        sAddr.sin_addr.s_addr = INADDR_BROADCAST;
        sAddr.sin_port = htons(23);

        //
        // Send the discover message on this socket.
        //
        sendto(g_sSockets[iIdx].iSocket, pcBuffer, sizeof(pcBuffer), 0,
               (struct sockaddr *)&sAddr, sizeof(sAddr));
    }
}

//*****************************************************************************
//
// This function waits for response messages on the UDP sockets.
//
//*****************************************************************************
void
ReadResponses(void)
{
    unsigned char pucBuffer[256];
    struct sockaddr_in sAddr;
    struct timeval sDelay;
    int iIdx, iMax, iLoop;
    socklen_t sLen;
    fd_set sRead;
#ifdef __WIN32
    unsigned long ulTemp;
#else
    struct sockaddr_in *pAddr;
    struct sockaddr *pMAC;
    struct arpreq sARP;
#endif

    //
    // Initially there are no boards.
    //
    g_ulNumBoards = 0;

    //
    // Loop while there is still room in the board data array.
    //
    while(g_ulNumBoards != (sizeof(g_psBoardData) / sizeof(g_psBoardData[0])))
    {
        //
        // Set the delay for the select() call based on the number of boards.
        // Wait for 5 seconds for the first response, and after a response is
        // received only wait for 2 seconds for the remaining responses.
        //
        if(g_ulNumBoards == 0)
        {
            sDelay.tv_sec = 5;
            sDelay.tv_usec = 0;
        }
        else
        {
            sDelay.tv_sec = 1;
            sDelay.tv_usec = 0;
        }

        //
        // Clear the read socket set.
        //
        FD_ZERO(&sRead);

        //
        // Loop over all the sockets to set the appropriate bit in the read
        // socket set and find the maximum socket file descriptor number.
        //
        iMax = 0;
        for(iIdx = 0; iIdx < g_ulNumSockets; iIdx++)
        {
            //
            // Set this file descriptor in the read socket set.
            //
            FD_SET(g_sSockets[iIdx].iSocket, &sRead);

            //
            // Save this as the maximum file descriptor number if it is larger
            // than the previous maximum.
            //
            if(g_sSockets[iIdx].iSocket > iMax)
            {
                iMax = g_sSockets[iIdx].iSocket;
            }
        }

        //
        // Wait until one of the sockets has data to be read or the timeout
        // expires.
        //
        iIdx = select(iMax + 1, &sRead, 0, 0, &sDelay);

        //
        // Retry if there was a select failure.
        //
        if(iIdx < 0)
        {
            continue;
        }

        //
        // Break out of the loop if the timeout expired.
        //
        if(iIdx == 0)
        {
            break;
        }

        //
        // Loop through the sockets.
        //
        for(iIdx = 0; iIdx < g_ulNumSockets; iIdx++)
        {
            //
            // Skip this socket if it does not have data waiting to be read.
            //
            if(!FD_ISSET(g_sSockets[iIdx].iSocket, &sRead))
            {
                continue;
            }

            //
            // Read the datagram from this socket.
            //
            sLen = sizeof(sAddr);
            if(recvfrom(g_sSockets[iIdx].iSocket, (char *)pucBuffer,
                        sizeof(pucBuffer), 0, (struct sockaddr *)&sAddr,
                        &sLen) < 4)
            {
                continue;
            }

            //
            // Ignore this datagram if the first three bytes are not as
            // expected.
            //
            if((pucBuffer[0] != TAG_STATUS) || (pucBuffer[1] < 4) ||
               (pucBuffer[2] != CMD_DISCOVER_TARGET))
            {
                continue;
            }

            //
            // Ignore this datagram if the checksum is not correct.
            //
            for(iLoop = 0, iMax = 0; iLoop < pucBuffer[1]; iLoop++)
            {
                iMax -= pucBuffer[iLoop];
            }
            if((iMax & 0xff) != 0)
            {
                continue;
            }

            //
            // Loop through the boards that have already been found and ignore
            // this datagram if it is a duplicate.
            //
            for(iLoop = 0; iLoop < g_ulNumBoards; iLoop++)
            {
                if(g_psBoardData[iLoop].ulIPAddr == sAddr.sin_addr.s_addr)
                {
                    break;
                }
            }
            if(iLoop != g_ulNumBoards)
            {
                continue;
            }

            //
            // Clear the next board data entry.
            //
            memset(&(g_psBoardData[g_ulNumBoards]), 0,
                   sizeof(g_psBoardData[0]));

            //
            // Save the IP address of this board.
            //
            g_psBoardData[g_ulNumBoards].ulIPAddr = sAddr.sin_addr.s_addr;

            //
            // If the board type was supplied in the response, then save it.
            //
            if(pucBuffer[1] > 4)
            {
                g_psBoardData[g_ulNumBoards].ucBoardType = pucBuffer[3];
            }

            //
            // If the board ID was supplied in the response, then save it.
            //
            if(pucBuffer[1] > 5)
            {
                g_psBoardData[g_ulNumBoards].ucBoardID = pucBuffer[4];
            }

            //
            // If the client IP was supplied in the repsonse, then save it.
            //
            if(pucBuffer[1] > 9)
            {
                memcpy(&(g_psBoardData[g_ulNumBoards].ulClientIPAddr),
                       pucBuffer + 5, 4);
            }

            //
            // If the MAC address was supplied in the response, then save it.
            //
            if(pucBuffer[1] > 15)
            {
                memcpy(g_psBoardData[g_ulNumBoards].pucMACArray, pucBuffer + 9,
                       6);
            }
            else
            {
#ifdef __WIN32
                //
                // Send an ARP request to get the MAC address of the board.
                //
                ulTemp = 6;
                SendARP(g_psBoardData[g_ulNumBoards].ulIPAddr, 0,
                        (DWORD *)g_psBoardData[g_ulNumBoards].pucMACArray,
                        &ulTemp);
#else
                //
                // Query the ARP table for the MAC address of the board.  It is
                // possible in some cases that the MAC address does not appear
                // in the table.
                //
                memset(&sARP, 0, sizeof(sARP));
                pAddr = (struct sockaddr_in *)&(sARP.arp_ha);
                pAddr->sin_family = ARPHRD_ETHER;
                pAddr = (struct sockaddr_in *)&(sARP.arp_pa);
                pAddr->sin_family = AF_INET;
                pAddr->sin_addr.s_addr = sAddr.sin_addr.s_addr;
                sARP.arp_flags = ATF_PUBL;
                strncpy(sARP.arp_dev, g_sSockets[iIdx].pcIFName,
                        sizeof(sARP.arp_dev));
                if(strchr(sARP.arp_dev, ':') != 0)
                {
                    *strchr(sARP.arp_dev, ':') = 0;
                }
                if(ioctl(g_sSockets[iIdx].iSocket, SIOCGARP,
                         (char *)&sARP) == 0)
                {
                    pMAC = (struct sockaddr *)&(sARP.arp_ha);
                    memcpy(g_psBoardData[g_ulNumBoards].pucMACArray,
                           pMAC->sa_data, 6);
                }
#endif
            }

            //
            // If the firmware version was supplied in the response, then save
            // it.
            //
            if(pucBuffer[1] > 19)
            {
                memcpy(&(g_psBoardData[g_ulNumBoards].ulVersion),
                       pucBuffer + 15, 4);
            }

            //
            // If the application title was supplied in the response, then save
            // it.
            //
            if(pucBuffer[1] > 83)
            {
                memcpy(g_psBoardData[g_ulNumBoards].pcAppTitle, pucBuffer + 19,
                       64);
                g_psBoardData[g_ulNumBoards].pcAppTitle[63] = 0;
            }
            else if(pucBuffer[1] == 10)
            {
                //
                // The application title was not supplied, by the packet size
                // is 10, indicating the older motor control device discovery
                // protocol.  Construct a application title based on the board
                // type.
                //
                if(g_psBoardData[g_ulNumBoards].ucBoardType ==
                   RESP_ID_TARGET_BLDC)
                {
                    strcpy(g_psBoardData[g_ulNumBoards].pcAppTitle,
                           "Stellaris RDK-BLDC");
                }
                else if(g_psBoardData[g_ulNumBoards].ucBoardType ==
                        RESP_ID_TARGET_STEPPER)
                {
                    strcpy(g_psBoardData[g_ulNumBoards].pcAppTitle,
                           "Stellaris RDK-STEPPER");
                }
                else if(g_psBoardData[g_ulNumBoards].ucBoardType ==
                        RESP_ID_TARGET_ACIM)
                {
                    strcpy(g_psBoardData[g_ulNumBoards].pcAppTitle,
                           "Stellaris RDK-ACIM");
                }
            }

            //
            // Increment the count of board that have been found.
            //
            g_ulNumBoards++;

            //
            // Break out of this loop if the board array is now full.
            //
            if(g_ulNumBoards ==
               (sizeof(g_psBoardData) / sizeof(g_psBoardData[0])))
            {
                break;
            }
        }
    }
}

//*****************************************************************************
//
// This function closes the UDP sockets.
//
//*****************************************************************************
void
CloseSockets(void)
{
    int iIdx;

    //
    // Loop through the UDP sockets.
    //
    for(iIdx = 0; iIdx < g_ulNumSockets; iIdx++)
    {
        //
        // Close this socket.
        //
#ifdef __WIN32
        closesocket(g_sSockets[iIdx].iSocket);
#else
        close(g_sSockets[iIdx].iSocket);
#endif
    }

    //
    // There are no sockets open now.
    //
    g_ulNumSockets = 0;
}

//*****************************************************************************
//
// A callback function to display an alert window when the required sockets
// could not be created (likely due to permission problems).
//
//*****************************************************************************
void
RunAsRoot(void *pvData)
{
    //
    // Display an alert window.
    //
    fl_alert("Could not create required sockets.  This is either\n"
             "a permission problem or another application is\n"
             "already using the required ports.");

    //
    // Exit the application.
    //
    exit(1);
}

//*****************************************************************************
//
// A callback function to update the list of boards.
//
//*****************************************************************************
void
UpdateDisplay(void *pvData)
{
    unsigned long ulIdx;

    //
    // Remove the existing boards from the display.
    //
    RemoveBoards();

    //
    // Loop through the boards that were found.
    //
    for(ulIdx = 0; ulIdx < g_ulNumBoards; ulIdx++)
    {
        //
        // Add this board to the display.
        //
        AddBoard(ulIdx, g_psBoardData[ulIdx].ulIPAddr,
                 g_psBoardData[ulIdx].pucMACArray,
                 g_psBoardData[ulIdx].ulClientIPAddr,
                 g_psBoardData[ulIdx].pcAppTitle);
    }

    //
    // Awaken the main thread.
    //
    Fl::awake(pvData);
}

//*****************************************************************************
//
// A worker thread that performs the network queries in the background, so that
// the foreground thread can handle the GUI (allowing it to remain responsive).
//
//*****************************************************************************
#ifdef __WIN32
void
#else
void *
#endif
WorkerThread(void *pvData)
{
    //
    // Loop forever.  This loop will be explicitly exited as required.
    //
    while(1)
    {
        //
        // Wait until a refresh of the device list is requested.
        //
        while(g_ulRefresh == 0)
        {
            //
            // Awaken the main thread, so that it processes messages in a
            // timely manner.
            //
            Fl::awake(pvData);

            //
            // Sleep for 100ms.
            //
            usleep(100000);
        }

        //
        // Clear the refresh flag.
        //
        g_ulRefresh = 0;

        //
        // Create the sockets needed to query the network.
        //
        CreateIFSockets();

        //
        // See if the sockets could be created.
        //
        if(g_ulNumSockets == 0)
        {
            //
            // Tell the main thread to display the error message dialog box.
            //
            Fl::awake(RunAsRoot, 0);

            //
            // End this thread.
            //
#ifdef __WIN32
            _endthread();
#else
            pthread_exit(0);
#endif
        }

        //
        // Send the discover request packets.
        //
        SendDiscovers();

        //
        // Read back the response(s) from the devices on the network.
        //
        ReadResponses();

        //
        // Close the sockets.
        //
        CloseSockets();

        //
        // Tell the main thread to update the board display.
        //
        Fl::awake(UpdateDisplay, 0);
    }
}

//*****************************************************************************
//
// This program finds Stellaris boards that provide the locator service on the
// Ethernet.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
#ifndef __WIN32
    pthread_t thread;
#endif
#ifdef __WIN32
    WSADATA wsaData;
#endif

    //
    // If running on Windows, initialize the COM library (required for multi-
    // threading).
    //
#ifdef __WIN32
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif

    //
    // If running on Windows, initialize the WSOCK32 library (required for
    // TCP/IP sockets).
    //
#ifdef __WIN32
    WSAStartup(0x202, &wsaData);
#endif

    //
    // Create and show the main window.
    //
    CreateAppWindow()->show(argc, argv);

    //
    // Prepare FLTK for multi-threaded operation.
    //
    Fl::lock();

    //
    // Create the worker thread that performs the network scan in the
    // background.
    //
#ifdef __WIN32
    _beginthread(WorkerThread, 0, 0);
#else
    pthread_create(&thread, 0, WorkerThread, 0);
#endif

    //
    // Handle the FLTK events in the main thread.
    //
    return(Fl::run());
}
