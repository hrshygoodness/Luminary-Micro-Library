//*****************************************************************************
//
// tftp.c - A very simple TFTP server allowing access to the serial flash.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "utils/fswrapper.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "idm-checkout.h"
#include "drivers/ssiflash.h"
#include "file.h"

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
// The UDP port for the TFTP server.
//
//*****************************************************************************
#define TFTP_PORT               69

static struct udp_pcb *g_pTFTPDataPCB = 0;
static unsigned long g_ulTFTPFileLength = 0;

//*****************************************************************************
//
// TFTP error codes.
//
//*****************************************************************************
#define TFTP_ERR_NOT_DEFINED  0x00
#define TFTP_FILE_NOT_FOUND   0x01
#define TFTP_ACCESS_VIOLATION 0x02
#define TFTP_DISK_FULL        0x03
#define TFTP_ILLEGAL_OP       0x04
#define TFTP_UNKNOWN_TID      0x05
#define TFTP_FILE_EXISTS      0x06
#define TFTP_NO_SUCH_USER     0x07

//*****************************************************************************
//
// Sends a TFTP error packet.
//
//*****************************************************************************
static void
TFTPErrorSend(unsigned long ulError, char *pcError)
{
    unsigned long ulLength;
    unsigned char *pucData;
    struct pbuf *p;

    //
    // How big is this packet going to be?
    //
    ulLength = 5 + strlen(pcError);

    //
    // Allocate a pbuf for this data packet.
    //
    p = pbuf_alloc(PBUF_TRANSPORT, ulLength, PBUF_RAM);
    if(!p)
    {
        return;
    }

    //
    // Get a pointer to the data packet.
    //
    pucData = (unsigned char *)p->payload;

    //
    // Fill in the packet.
    //
    pucData[0] = (TFTP_ERROR >> 8) & 0xff;
    pucData[1] = TFTP_ERROR & 0xff;
    pucData[2] = (ulError >> 8) & 0xff;
    pucData[3] = ulError & 0xff;
    memcpy(&pucData[4], pcError, ulLength - 5);

    //
    // Send the data packet.
    //
    udp_send(g_pTFTPDataPCB, p);

    //
    // Free the pbuf.
    //
    pbuf_free(p);
}

//*****************************************************************************
//
// Sends a TFTP data packet.
//
//*****************************************************************************
static void
TFTPDataSend(unsigned long ulBlockNum)
{
    unsigned long ulLength;
    unsigned char *pucData;
    struct pbuf *p;

    //
    // Determine the number of bytes to place into this packet.
    //
    if(g_ulTFTPFileLength < (ulBlockNum * 512))
    {
        ulLength = g_ulTFTPFileLength & 511;
    }
    else
    {
        ulLength = 512;
    }

    //
    // Allocate a pbuf for this data packet.
    //
    p = pbuf_alloc(PBUF_TRANSPORT, ulLength + 4, PBUF_RAM);
    if(!p)
    {
        return;
    }

    //
    // Get a pointer to the data packet.
    //
    pucData = (unsigned char *)p->payload;

    //
    // Fill in the packet header.
    //
    pucData[0] = (TFTP_DATA >> 8) & 0xff;
    pucData[1] = TFTP_DATA & 0xff;
    pucData[2] = (ulBlockNum >> 8) & 0xff;
    pucData[3] = ulBlockNum & 0xff;

    //
    // Read the data from serial flash into the packet.
    //
    SSIFlashRead((ulBlockNum - 1) * 512, ulLength, pucData + 4);

    //
    // Send the data packet.
    //
    udp_send(g_pTFTPDataPCB, p);

    //
    // Free the pbuf.
    //
    pbuf_free(p);
}

//*****************************************************************************
//
// Send an ACK packet back to the TFTP client.
//
//*****************************************************************************
static void
TFTPDataAck(unsigned long ulBlockNum)
{
    unsigned char *pucData;
    struct pbuf *p;

    //
    // Allocate a pbuf for this data packet.
    //
    p = pbuf_alloc(PBUF_TRANSPORT, 4, PBUF_RAM);
    if(!p)
    {
        return;
    }

    //
    // Get a pointer to the data packet.
    //
    pucData = (unsigned char *)p->payload;

    //
    // Fill in the packet header.
    //
    pucData[0] = (TFTP_ACK >> 8) & 0xff;
    pucData[1] = TFTP_ACK & 0xff;
    pucData[2] = (ulBlockNum >> 8) & 0xff;
    pucData[3] = ulBlockNum & 0xff;

    //
    // Send the data packet.
    //
    udp_send(g_pTFTPDataPCB, p);

    //
    // Free the pbuf.
    //
    pbuf_free(p);
}

//*****************************************************************************
//
// Handles datagrams received from the TFTP data connection.
//
//*****************************************************************************
static void
TFTPDataRecv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
             struct ip_addr *addr, u16_t port)
{
    unsigned char *pucData;
    unsigned long ulBlock, ulLen, ulTotLen;
    struct pbuf *pBuf;

    //
    // Get a pointer to the TFTP packet.
    //
    pucData = (unsigned char *)(p->payload);

    //
    // If this is an ACK packet, send back the next block to satisfy an
    // ongoing GET (read) request.
    //
    if((pucData[0] == ((TFTP_ACK >> 8) & 0xff)) &&
       (pucData[1] == (TFTP_ACK & 0xff)))
    {
        //
        // Extract the block number from the acknowledge.
        //
        ulBlock = (pucData[2] << 8) + pucData[3];

        //
        // See if there is more data to be sent.
        //
        if((ulBlock * 512) <= g_ulTFTPFileLength)
        {
            //
            // Send the next block of the file.
            //
            TFTPDataSend(ulBlock + 1);
        }
        else
        {
            //
            // The transfer is complete, so close the data connection.
            //
            udp_remove(g_pTFTPDataPCB);
            g_pTFTPDataPCB = 0;
            g_ulTFTPFileLength = 0;
        }
    }
    else
    {
        //
        // If this is a DATA packet, get the payload and write it to the
        // appropriate location in the serial flash.
        //
        if((pucData[0] == ((TFTP_DATA >> 8) & 0xff)) &&
           (pucData[1] == (TFTP_DATA & 0xff)))
        {
            //
            // This is a data packet.  Extract the block number from the
            // packet.
            //
            ulBlock = (pucData[2] << 8) + pucData[3];

            //
            // Have we filled the serial flash?
            //
            if(((ulBlock - 1) * 512) < 0x100000)
            {
                //
                // We have not overflowed the device so go ahead and write
                // this data packet.  Check to see if we are about to write to
                // a new flash sector and, if so, erase it.
                //
                if((((ulBlock - 1) * 512) % SSIFlashSectorSizeGet()) == 0)
                {
                    SSIFlashSectorErase((ulBlock - 1) * 512, true);
                }

                //
                // Now write the data to the relevant address.  Note that,
                // although we expect up to 512 bytes, these may be split
                // across several pbufs in the chain so we need to be careful
                // of this.
                //
                ulTotLen = 0;
                ulLen = p->len - 4;
                pucData+= 4;
                pBuf = p;

                //
                // Keep writing until we run out of data.
                //
                while((ulTotLen < (p->tot_len - 4)) && pBuf)
                {
                    //
                    // Write this payload from the current buffer to the flash.
                    //
                    SSIFlashWrite(ulTotLen + ((ulBlock - 1) * 512), ulLen,
                                  pucData);

                    //
                    // Remember how many bytes we've written so far.
                    //
                    ulTotLen += ulLen;

                    //
                    // Move to the next pbuf in the chain
                    //
                    pBuf = pBuf->next;
                    if(pBuf)
                    {
                        pucData=pBuf->payload;
                        ulLen = pBuf->len;
                    }
                }

                //
                // Acknowledge this block.
                //
                TFTPDataAck(ulBlock);

                //
                // Is the transfer finished?
                //
                if(p->tot_len < (512 + 4))
                {
                    //
                    // We got a short packet so the transfer is complete.  Close
                    // the connection.
                    //
                    udp_remove(g_pTFTPDataPCB);
                    g_pTFTPDataPCB = 0;
                    g_ulTFTPFileLength = 0;
                }
            }
            else
            {
                //
                // The data has reached the end of the flash so send back an
                // error and close the connection.
                //
                TFTPErrorSend(TFTP_DISK_FULL, "Disk full");
                udp_remove(g_pTFTPDataPCB);
                g_pTFTPDataPCB = 0;
            }
        }
    }

    //
    // Free the pbuf.
    //
    pbuf_free(p);
}

//*****************************************************************************
//
// Handles datagrams received on the TFTP server port.
//
//*****************************************************************************
static void
TFTPRecv(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr,
         u16_t port)
{
    unsigned char *pucData;

    //
    // Get a pointer to the TFTP packet.
    //
    pucData = (unsigned char *)(p->payload);

    //
    // Is this a read (GET) request?
    //
    if((pucData[0] == ((TFTP_RRQ >> 8) & 0xff)) &&
       (pucData[1] == (TFTP_RRQ & 0xff)))
    {
        //
        // Yes. This is a get request. Make sure that this is a request for the
        // serial flash (eeprom) image.  If not, we merely ignore the datagram.
        //
        if((memcmp(pucData + 2, "eeprom\0octet", 13) == 0))
        {
            //
            // If there is already a data connection, then abort it now.
            //
            if(g_pTFTPDataPCB)
            {
                udp_remove(g_pTFTPDataPCB);
            }

            //
            // Create a new TFTP data connection.
            //
            g_pTFTPDataPCB = udp_new();
            udp_recv(g_pTFTPDataPCB, TFTPDataRecv, NULL);
            udp_connect(g_pTFTPDataPCB, addr, port);
            udp_bind(g_pTFTPDataPCB, IP_ADDR_ANY, 34515);

            //
            // Determine the length of the image we will be sending back.
            //
            g_ulTFTPFileLength = FileSDRAMImageSizeGet();

            //
            // Send the first block of data if there is anything to send,
            // otherwise tell the caller that the file isn't found.
            //
            if(g_ulTFTPFileLength)
            {
                TFTPDataSend(1);
            }
            else
            {
                //
                // There is no image to get so return an error and close the
                // connection.
                //
                TFTPErrorSend(TFTP_FILE_NOT_FOUND, "File not found");
                udp_remove(g_pTFTPDataPCB);
                g_pTFTPDataPCB = 0;
            }
        }
    }
    else
    {
        //
        // Is this a write (PUT) request?
        //
        if((pucData[0] == ((TFTP_WRQ >> 8) & 0xff)) &&
           (pucData[1] == (TFTP_WRQ & 0xff)))
        {
            //
            // Yes - this is a put request.  Make sure that it is a request to
            // write the serial flash (eeprom) image.
            //
            if((memcmp(pucData + 2, "eeprom\0octet", 13) == 0))
            {
                //
                // If there is already a data connection, then abort it now.
                //
                if(g_pTFTPDataPCB)
                {
                    udp_remove(g_pTFTPDataPCB);
                }

                //
                // Create a new TFTP data connection.
                //
                g_pTFTPDataPCB = udp_new();
                udp_recv(g_pTFTPDataPCB, TFTPDataRecv, NULL);
                udp_connect(g_pTFTPDataPCB, addr, port);
                udp_bind(g_pTFTPDataPCB, IP_ADDR_ANY, 34515);

                //
                // Acknowledge this request to tell the client that it should
                // start sending data.
                //
                TFTPDataAck(0);
            }
        }
    }

    //
    // Free the pbuf.
    //
    pbuf_free(p);
}

//*****************************************************************************
//
// Initializes the TFTP server.
//
//*****************************************************************************
void
TFTPInit(void)
{
    void *pcb;

    pcb = udp_new();
    udp_recv(pcb, TFTPRecv, NULL);
    udp_bind(pcb, IP_ADDR_ANY, TFTP_PORT);
}
