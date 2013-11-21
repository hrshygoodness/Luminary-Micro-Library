//*****************************************************************************
//
// upnp.c - UPnP support routines.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-S2E Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/ethernet.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "config.h"
#include "upnp.h"

//*****************************************************************************
//
//! \addtogroup upnp_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The UPnP multi-cast IP Address.
//
//*****************************************************************************
static const struct ip_addr g_sIPAddrUPnP = { 0xFAFFFFEF };
#define IP_ADDR_UPNP            ((struct ip_addr *)&g_sIPAddrUPnP)

//*****************************************************************************
//
//! A pointer to the UPnP UDP Discovery PCB data structure.
//
//*****************************************************************************
static struct udp_pcb *g_psUPnP_DiscoveryPCB = NULL;

//*****************************************************************************
//
//! A pointer to the TCP PCB used to listen for incoming UPnP requests.
//
//*****************************************************************************
static struct tcp_pcb *g_psUPnP_ListenPCB = NULL;

//*****************************************************************************
//
//! The UPnP multi-cast IP Address.
//
//*****************************************************************************
static unsigned long g_ulUPnPAdverstisementTimer = 0;

//*****************************************************************************
//
//! The UPnP response counter.
//
//*****************************************************************************
static unsigned long g_ulResponseCount = 0;

//*****************************************************************************
//
//! The UPnP response address.
//
//*****************************************************************************
static unsigned long g_ulResponseAddr = 0;
#define IP_ADDR_RESPONSE        ((struct ip_addr *)&g_ulResponseAddr)

//*****************************************************************************
//
//! The UPnP response address.
//
//*****************************************************************************
static unsigned short g_usResponsePort = 0;

//*****************************************************************************
//
// The connection state information for a TCP connection to the UPnP
// LOCATION URL port.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulRetryCount;
    tBoolean bDescriptionSent;
}
tUPnPState;

//*****************************************************************************
//
//! The XML description file for the S2E module.
//
//*****************************************************************************
static const char g_pcDescriptionXML1ofN[] =
{
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/xml\n"
    "Connection: Keep-Alive\n"
    "\n"
    "<?xml version=\"1.0\"?>\n"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\n"
    "<specVersion>\n"
    "<major>1</major>\n"
    "<minor>0</minor>\n"
    "</specVersion>\n"
    "<device>\n"
    "<deviceType>urn:schemas-upnp-org:device:MDL-S2E:1</deviceType>\n"
    "<friendlyName>"
};
static const char g_pcDescriptionXML2ofN[] =
{
    "%s (%d.%d.%d.%d)"
};
static const char g_pcDescriptionXML3ofN[] =
{
    "</friendlyName>\n"
    "<manufacturer>Texas Instruments</manufacturer>\n"
    "<manufacturerURL>http://www.ti.com</manufacturerURL>\n"
    "<modelDescription>Serial to Ethernet Module</modelDescription>\n"
    "<modelName>Serial To Ethernet (2)</modelName>\n"
    "<modelNumber>MDL-S2E-2</modelNumber>\n"
    "<UDN>uuid:upnp_MDL-S2E-"
};
static const char g_pcDescriptionXML4ofN[] =
{
    "%02X%02X%02X%02X%02X%02X"
};
static const char g_pcDescriptionXML5ofN[] =
{
    "</UDN>\n"
    "<serviceList>\n"
    "<service>\n"
    "<serviceType>(null)</serviceType>\n"
    "<serviceId>(null)</serviceId>\n"
    "<controlURL>(null)</controlURL>\n"
    "<eventSubURL>(null)</eventSubURL>\n"
    "<SCPDURL>(null)</SCPDURL>\n"
    "</service>\n"
    "</serviceList>\n"
    "<presentationURL>http://"
};
static const char g_pcDescriptionXML6ofN[] =
{
    "%d.%d.%d.%d"
};
static const char g_pcDescriptionXML7ofN[] =
{
    ":80</presentationURL>\n"
    "</device>\n"
    "</root>\n"
};

//*****************************************************************************
//
//! The UPnP Adverstisement #1.
//
//*****************************************************************************
static const char g_pcDiscoverRoot1[] =
{
    "NOTIFY * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "CACHE-CONTROL: max-age=60\r\n"
    "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
    "NT: upnp:rootdevice\r\n"
    "NTS: ssdp:alive\r\n"
    "SERVER: lwIP/1.3.2\r\n"
    "USN: uuid:upnp_MDL-S2E-%02X%02X%02X%02X%02X%02X::upnp:rootdevice\r\n"
    "\r\n"
};

//*****************************************************************************
//
//! The UPnP Adverstisement #2.
//
//*****************************************************************************
static const char g_pcDiscoverRoot2[] =
{
    "NOTIFY * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "CACHE-CONTROL: max-age=60\r\n"
    "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
    "NT: uuid:upnp_MDL-S2E-%02X%02X%02X%02X%02X%02X\r\n"
    "NTS: ssdp:alive\r\n"
    "SERVER: lwIP/1.3.2\r\n"
    "USN: uuid:upnp_MDL-S2E-%02X%02X%02X%02X%02X%02X\r\n"
    "\r\n"
};

//*****************************************************************************
//
//! The UPnP Adverstisement #3.
//
//*****************************************************************************
static const char g_pcDiscoverRoot3[] =
{
    "NOTIFY * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "CACHE-CONTROL: max-age=60\r\n"
    "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
    "NT: urn:schemas-upnp-org:device:MDL-S2E:1\r\n"
    "NTS: ssdp:alive\r\n"
    "SERVER: lwIP/1.3.2\r\n"
    "USN: uuid:upnp_MDL-S2E-%02X%02X%02X%02X%02X%02X\r\n"
    "\r\n"
};

//*****************************************************************************
//
//! The UPnP Bye-Bye message.
//
//*****************************************************************************
static const char g_pcDiscoverByeBye[] =
{
    "NOTIFY * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "NT: urn:schemas-upnp-org:device:MDL-S2E:1\r\n"
    "NTS: ssdp:byebye\r\n"
    "USN: uuid:upnp_MDL-S2E-%02X%02X%02X%02X%02X%02X\r\n"
    "\r\n"
};

#define MAX_BYEBYE_LEN          160

//*****************************************************************************
//
//! The UPnP Discover Response message.
//
//*****************************************************************************
static const char g_pcResponseRoot1[] =
{
    "HTTP/1.1 200 OK\r\n"
    "CACHE-CONTROL: max-age=60\r\n"
    "DATE: Mon, 01 Jan 1970 00:00:00 GMT\r\n"
    "EXT:\r\n"
    "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
    "SERVER: lwIP/1.3.2\r\n"
    "ST: upnp:rootdevice\r\n"
    "USN: uuid:upnp_MDL-S2E-%02X%02X%02X%02X%02X%02X::upnp:rootdevice\r\n"
    "\r\n"
};

//*****************************************************************************
//
// The search target string corresponding to the URN for this device.
//
//*****************************************************************************
#define SCHEMA_SEARCH_TARGET "urn:schemas-upnp-org:device:MDL-S2E:1"

//*****************************************************************************
//
//! Builds and sends the location data.
//!
//! \param pcb is the TCP PCB for this connection.
//! \param pState is the state data for this connection.
//!
//! This function is called when the location data file needs to be sent
//! out the TCP connection.
//!
//! \return This function will return an lwIP defined error code..
//
//*****************************************************************************
static void
UPnP_send_data(struct tcp_pcb *pcb, tUPnPState *pState)
{
    unsigned long ulIPAddr;
    unsigned char pucMACAddr[6];
    static char pcBuf[30 + MOD_NAME_LEN];
    err_t err;

    //
    // First check to see if the description has already been queued.
    //
    if(pState->bDescriptionSent)
    {
        return;
    }

    //
    // Check to see if there is room in the TCP send buffer for a whole
    // description packet.
    //
    if(tcp_sndbuf(pcb) < 1500)
    {
        return;
    }

    //
    // Get the local IP and MAC address.
    //
    ulIPAddr = lwIPLocalIPAddrGet();
    lwIPLocalMACGet(pucMACAddr);

    //
    // Start writing the description XML file into the tcp output buffer.
    //
    err = tcp_write(pcb, g_pcDescriptionXML1ofN,
                    strlen(g_pcDescriptionXML1ofN), 0);
    if(err != ERR_OK)
    {
    }

    usprintf(pcBuf, g_pcDescriptionXML2ofN, &g_sParameters.ucModName[0],
             ((ulIPAddr >>  0) & 0xFF), ((ulIPAddr >>  8) & 0xFF),
             ((ulIPAddr >> 16) & 0xFF), ((ulIPAddr >> 24) & 0xFF));
    err = tcp_write(pcb, pcBuf, strlen(pcBuf), 1);
    if(err != ERR_OK)
    {
    }

    err = tcp_write(pcb, g_pcDescriptionXML3ofN,
                    strlen(g_pcDescriptionXML3ofN), 0);
    if(err != ERR_OK)
    {
    }

    usprintf(pcBuf, g_pcDescriptionXML4ofN, pucMACAddr[0], pucMACAddr[1],
             pucMACAddr[2], pucMACAddr[3], pucMACAddr[4], pucMACAddr[5]);
    err = tcp_write(pcb, pcBuf, strlen(pcBuf), 1);
    if(err != ERR_OK)
    {
    }

    err = tcp_write(pcb, g_pcDescriptionXML5ofN,
                    strlen(g_pcDescriptionXML5ofN), 0);
    if(err != ERR_OK)
    {
    }

    usprintf(pcBuf, g_pcDescriptionXML6ofN, ((ulIPAddr >>  0) & 0xFF),
             ((ulIPAddr >>  8) & 0xFF), ((ulIPAddr >> 16) & 0xFF),
             ((ulIPAddr >> 24) & 0xFF));
    err = tcp_write(pcb, pcBuf, strlen(pcBuf), 1);
    if(err != ERR_OK)
    {
    }

    err = tcp_write(pcb, g_pcDescriptionXML7ofN,
                    strlen(g_pcDescriptionXML7ofN), 0);
    if(err != ERR_OK)
    {
    }

    //
    // Flush the output buffer.
    //
    tcp_output(pcb);

    //
    // Indicate that we have queued this one up.
    //
    pState->bDescriptionSent = true;
}

//*****************************************************************************
//
//! Handles the lwIP transmit callback.
//!
//! \param arg is the state information block to be freed.
//! \param pcb is the PCB to be closed.
//! \param len is not used in this implementation.
//!
//! This function is called when the lwIP TCP/IP stack has received an
//! acknowledgement for data that has been transmitted.
//!
//! \return This function will return an lwIP defined error code..
//
//*****************************************************************************
static err_t
UPnP_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
    //
    // Close the TCP connection.
    //
    tcp_arg(pcb, NULL);
    tcp_sent(pcb, NULL);
    tcp_recv(pcb, NULL);
    mem_free(arg);
    tcp_close(pcb);

    //
    // And return ok.
    //
    return ERR_OK;
}

//*****************************************************************************
//
//! Handles the lwIP polling/timeout callback.
//!
//! \param arg is not used in this implementation.
//! \param pcb is not used in this implementation.
//!
//! This function is called when the lwIP TCP/IP stack has no incoming or
//! outgoing data.  It can be used to reset an idle connection.
//!
//! \return This function will return an lwIP defined error code..
//
//*****************************************************************************
static err_t
UPnP_poll(void *arg, struct tcp_pcb *pcb)
{
    tUPnPState *pState;

    //
    // Setup pointer to the location state variable.
    //
    pState = arg;

    //
    // Increment the retry counter.
    //
    pState->ulRetryCount++;

    //
    // If we have exceeded the retry limit, the abort the session.
    //
    if(pState->ulRetryCount++ >= 6)
    {
        tcp_abort(pcb);
        return(ERR_ABRT);
    }

    //
    // Attempt to (re-)send the location description data.
    //
    UPnP_send_data(pcb, pState);

    //
    // Return okay.
    //
    return ERR_OK;
}

//*****************************************************************************
//
//! Handles the lwIP error callback.
//!
//! \param arg is not used in this implementation.
//! \param err is not used in this implementation.
//!
//! This function is called when the lwIP TCP/IP stack has detected an
//! error.  The connection is no longer valid.
//!
//! \return This function will return an lwIP defined error code..
//
//*****************************************************************************
static void
UPnP_error(void *arg, err_t err)
{
    //
    // Check if the argument is non-zero.
    //
    if(arg != NULL)
    {
        mem_free(arg);
    }
}

//*****************************************************************************
//
//! Receives a TCP packet from lwIP for LOCATION data processing.
//!
//! \param arg is the tcp connection state data.
//! \param pcb is the pointer to the TCP control structure.
//! \param p is the pointer to the pbuf structure containing the packet data.
//! \param err is used to indicate if any errors are associated with the
//! incoming packet.
//!
//! This function is called when the lwIP TCP/IP stack has an incoming
//! packet to be processed.
//!
//! \return This function will return an lwIP defined error code..
//
//*****************************************************************************
static err_t
UPnP_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    tUPnPState *pState;

    //
    // Setup pointer to our state data.
    //
    pState = arg;

    //
    // First, verify that we have a valid packet to process.
    //
    if((err == ERR_OK) && (p != NULL))
    {
        //
        // Acknowledge reception of the data.
        //
        tcp_recved(pcb, p->tot_len);

        //
        // For now, just assume that any GET command on this
        // port/connection will be a request for the UPnP description data.
        //
        if(strncmp(p->payload, "GET ", 4) == 0)
        {
            //
            // Build and send the location description xml file.
            //
            UPnP_send_data(pcb, pState);

            //
            // Free the incoming pbuf.
            //
            pbuf_free(p);

            //
            // Setup the TCP sent callback function.
            //
            tcp_sent(pcb, UPnP_sent);
        }
        else
        {
            //
            // Free the incoming pbuf.
            //
            pbuf_free(p);

            //
            // Close the connection.
            //
            tcp_arg(pcb, NULL);
            tcp_sent(pcb, NULL);
            tcp_recv(pcb, NULL);
            mem_free(pState);
            tcp_close(pcb);
        }
    }

    if((err == ERR_OK) && (p == NULL))
    {
        //
        // Close the connection.
        //
        tcp_arg(pcb, NULL);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        mem_free(pState);
        tcp_close(pcb);
    }

    return ERR_OK;
}

//*****************************************************************************
//
//! Accepts a TCP connection for UPnP Location/Discovery file.
//!
//! \param arg is not used in this implementation.
//! \param pcb is the pointer to the TCP control structure.
//! \param err is not used in this implementation.
//!
//! This function is called when the lwIP TCP/IP stack has an incoming
//! connection request on the telnet port.
//!
//! \return This function will return an lwIP defined error code..
//
//*****************************************************************************
static err_t
UPnP_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
    tUPnPState *pState;

    //
    // Setup the TCP connection priority.
    //
    tcp_setprio(pcb, TCP_PRIO_MIN);

    //
    // Allocation memory for the Location state data structure.
    //
    pState = mem_malloc(sizeof(tUPnPState));
    if(pState == NULL)
    {
        return(ERR_MEM);
    }
    pState->ulRetryCount = 0;
    pState->bDescriptionSent = false;

    //
    // Setup the TCP callback argument.
    //
    tcp_arg(pcb, pState);

    //
    // Setup the TCP receive function.
    //
    tcp_recv(pcb, UPnP_recv);

    //
    // Setup the TCP error function.
    //
    tcp_err(pcb, UPnP_error);

    //
    // Setup the TCP polling function/interval.
    //
    tcp_poll(pcb, UPnP_poll, (1000/TCP_SLOW_INTERVAL));

    //
    // Return a success code.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
//! Receives a UDP packet from lwIP for UPnP processing.
//!
//! \param arg is not used in this implementation.
//! \param pcb is the pointer to the UDB control structure.
//! \param p is the pointer to the pbuf structure containing the packet data.
//! \param addr is the source (remote) IP address for this packet.
//! \param port is the source (remote) port for this packet.
//!
//! This function is called when the lwIP TCP/IP stack has an incoming
//! UDP packet to be processed on the UPnP port.
//!
//! \return None.
//
//*****************************************************************************
static void
UPnP_recv_udp(void *arg, struct udp_pcb *pcb, struct pbuf *p,
              struct ip_addr *addr, u16_t port)
{
    char *pcData = p->payload;
    char *pcST;
    static char pcBuf[40];
    unsigned char pucMACAddr[6];

    //
    // Check if this is an M-SEARCH packet.
    //
    if(strncmp(pcData, "M-SEARCH ", 9) != 0)
    {
        pbuf_free(p);
        return;
    }

    if(ustrstr(pcData, "ssdp:discover") == NULL)
    {
        pbuf_free(p);
        return;
    }

    //
    // Find the start of the search target string.
    //
    pcST = ustrstr(pcData, "ST:");
    if(pcST == NULL)
    {
        pbuf_free(p);
        return;
    }

    //
    // Build our unique identifier.
    //
    lwIPLocalMACGet(pucMACAddr);
    usprintf(pcBuf, "uuid:upnp_MDL-S2E-%02X%02X%02X%02X%02X%02X",
             pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
             pucMACAddr[4], pucMACAddr[5]);

    if((ustrstr(pcST, "upnp:rootdevice") == NULL) &&
       (ustrstr(pcST, "ssdp:all") == NULL) &&
       (ustrstr(pcST, SCHEMA_SEARCH_TARGET) == NULL) &&
       (ustrstr(pcST, pcBuf) == NULL))
    {
        pbuf_free(p);
        return;
    }

    //
    // Flag handler to send discover response messages.
    //
    if(g_ulResponseCount == 0)
    {
        g_ulResponseCount = 4;
        g_ulResponseAddr = addr->addr;
        g_usResponsePort = port;
    }

    //
    // For now, just free the pbuf and return.
    //
    pbuf_free(p);
    return;
}

//*****************************************************************************
//
//! Initializes the UPnP session for the Serial to Ethernet Module.
//!
//! This function initializes and configures the UPnP session for the module.
//!
//! \return None.
//
//*****************************************************************************
void
UPnPInit(void)
{
    unsigned long ulTemp;

    //
    // Enable Ethernet Multicast Reception (required for UPnP operation).
    //
    ulTemp = EthernetConfigGet(ETH_BASE);
    ulTemp |= ETH_CFG_RX_AMULEN;
    EthernetConfigSet(ETH_BASE, ulTemp);

    //
    // Set up the ports required to listen for discovery and location
    // requests.
    //
    UPnPStart();
}

//*****************************************************************************
//
//! Broadcasts a byebye message and stop UPnP discovery.
//!
//! This function broadcasts an SSDP byebye message indicating that the UPnP
//! device is no longer available then frees resources associated with UPnP
//! discovery and location.
//!
//! \return None.
//
//*****************************************************************************
void
UPnPStop(void)
{
    unsigned long ulIPAddr;
    unsigned char pucMACAddr[6];
    char pcBuf[MAX_BYEBYE_LEN];
    struct pbuf *p_out;

    //
    // Get our IP and MAC address.
    //
    ulIPAddr = lwIPLocalIPAddrGet();
    lwIPLocalMACGet(pucMACAddr);

    //
    // If we have a UPnP discovery port set up already...
    //
    if(g_psUPnP_DiscoveryPCB)
    {
        //
        // If we have an IP address, broadcast an SSDP byebye message.
        //
        if(ulIPAddr)
        {
            usnprintf(pcBuf, MAX_BYEBYE_LEN, g_pcDiscoverByeBye, pucMACAddr[0],
                      pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
                      pucMACAddr[4], pucMACAddr[5]);

            //
            // Allocate a pbuf for this packet.
            //
            p_out = pbuf_alloc(PBUF_TRANSPORT, strlen(pcBuf), PBUF_RAM);
            if(p_out != NULL)
            {
                //
                // Send the byebye message.
                //
                memcpy(p_out->payload, pcBuf, strlen(pcBuf));
                udp_sendto(g_psUPnP_DiscoveryPCB, p_out, IP_ADDR_UPNP, 1900);
                pbuf_free(p_out);
            }
        }

        //
        // Close the UDP PCB for SSDP discovery broadcasts.
        //
        udp_disconnect(g_psUPnP_DiscoveryPCB);
        udp_remove(g_psUPnP_DiscoveryPCB);
        g_psUPnP_DiscoveryPCB = NULL;
    }

    //
    // Are we currently listening for location requests?
    //
    if(g_psUPnP_ListenPCB)
    {
        //
        // Close the PCB which is currently listening for incoming location
        // requests.
        //
        tcp_close(g_psUPnP_ListenPCB);
        g_psUPnP_ListenPCB = NULL;
    }
}

//*****************************************************************************
//
//! Starts listening for UPnP requests.
//!
//! This function sets up the two ports which listen for UPnP location and
//! discovery requests.
//!
//! \return None.
//
//*****************************************************************************
void
UPnPStart(void)
{
    void *pcb;

    //
    // Initialize the application to listen on the LOCATION port for a request
    // for the description XML file.
    //
    pcb = tcp_new();
    g_psUPnP_ListenPCB = pcb;
    tcp_bind(pcb, IP_ADDR_ANY, g_sParameters.usLocationURLPort);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, UPnP_accept);

    //
    // Set up a UDP PCB to allow us to send SSDP discovery messages on port
    // 1900.
    //
    g_psUPnP_DiscoveryPCB = pcb = udp_new();
    udp_recv(pcb, UPnP_recv_udp, NULL);
    udp_bind(pcb, IP_ADDR_ANY, 1900);
    udp_connect(pcb, IP_ADDR_ANY, 1900);
}

//*****************************************************************************
//
//! Handles Ethernet interrupt for UPnP sessions.
//!
//! \param ulTimeMS is the absolute time (as maintained by the lwip handler)
//! in ms.
//!
//! This function should be called on a regular periodic basis to handle the
//! various timers and process any buffers for the UPnP sessions.
//!
//! \return None.
//
//*****************************************************************************
void
UPnPHandler(unsigned long ulTimeMS)
{
    struct pbuf *p_out;
    static char pcBuf[400];
    unsigned long ulIPAddr;
    unsigned char pucMACAddr[6];
    static unsigned long ulState = 0;
    static unsigned long ulLastTimeMS = 0;

    //
    // If the timer hasn't ticked, then don't run anything.
    //
    if(ulTimeMS == ulLastTimeMS)
    {
        return;
    }
    ulLastTimeMS = ulTimeMS;

    //
    // If no UDP PCB has been allocated, don't do this.
    //
    if(g_psUPnP_DiscoveryPCB == NULL)
    {
        return;
    }

    //
    // If no IP address has been assigned, don't do this.
    //
    ulIPAddr = lwIPLocalIPAddrGet();
    if(ulIPAddr == 0)
    {
        return;
    }
    lwIPLocalMACGet(pucMACAddr);

    //
    // Process the UPnP State Machine
    //
    switch(ulState)
    {
        case 0:
        {
            //
            // Check to see if timeout has expired.  If so, send our UPnP
            // Advertisement again.
            //
            if((ulTimeMS - g_ulUPnPAdverstisementTimer) >
               UPNP_ADVERTISEMENT_INTERVAL)
            {
                g_ulUPnPAdverstisementTimer = ulTimeMS;
                ulState++;
            }
            if(g_ulResponseCount)
            {
                ulState = 10;
            }
            break;
        }

        case 1:
        case 2:
        {
            //
            // Build the First Advertisement Message.
            //
            usprintf(pcBuf, g_pcDiscoverRoot1, ((ulIPAddr >>  0) & 0xFF),
                     ((ulIPAddr >>  8) & 0xFF), ((ulIPAddr >> 16) & 0xFF),
                     ((ulIPAddr >> 24) & 0xFF),
                     g_sParameters.usLocationURLPort, pucMACAddr[0],
                     pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
                     pucMACAddr[4], pucMACAddr[5]);

            //
            // Allocate a pbuf for this packet.
            //
            p_out = pbuf_alloc(PBUF_TRANSPORT, strlen(pcBuf), PBUF_RAM);
            if(p_out != NULL)
            {
                //
                // Send the response.
                //
                memcpy(p_out->payload, pcBuf, strlen(pcBuf));
                udp_sendto(g_psUPnP_DiscoveryPCB, p_out, IP_ADDR_UPNP, 1900);
                pbuf_free(p_out);
                ulState++;
            }

            break;
        }

        case 3:
        case 4:
        {
            //
            // Build the Second Advertisement Message.
            //
            usprintf(pcBuf, g_pcDiscoverRoot2, ((ulIPAddr >>  0) & 0xFF),
                     ((ulIPAddr >>  8) & 0xFF), ((ulIPAddr >> 16) & 0xFF),
                     ((ulIPAddr >> 24) & 0xFF),
                     g_sParameters.usLocationURLPort, pucMACAddr[0],
                     pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
                     pucMACAddr[4], pucMACAddr[5], pucMACAddr[0],
                     pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
                     pucMACAddr[4], pucMACAddr[5]);

            //
            // Allocate a pbuf for this packet.
            //
            p_out = pbuf_alloc(PBUF_TRANSPORT, strlen(pcBuf), PBUF_RAM);
            if(p_out != NULL)
            {
                //
                // Send the response.
                //
                memcpy(p_out->payload, pcBuf, strlen(pcBuf));
                udp_sendto(g_psUPnP_DiscoveryPCB, p_out, IP_ADDR_UPNP, 1900);
                pbuf_free(p_out);
                ulState++;
            }
            break;
        }

        case 5:
        case 6:
        {
            //
            // Build the Third Advertisement Message.
            //
            usprintf(pcBuf, g_pcDiscoverRoot3, ((ulIPAddr >>  0) & 0xFF),
                     ((ulIPAddr >>  8) & 0xFF), ((ulIPAddr >> 16) & 0xFF),
                     ((ulIPAddr >> 24) & 0xFF),
                     g_sParameters.usLocationURLPort, pucMACAddr[0],
                     pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
                     pucMACAddr[4], pucMACAddr[5]);

            //
            // Allocate a pbuf for this packet.
            //
            p_out = pbuf_alloc(PBUF_TRANSPORT, strlen(pcBuf), PBUF_RAM);
            if(p_out != NULL)
            {
                //
                // Send the response.
                //
                memcpy(p_out->payload, pcBuf, strlen(pcBuf));
                udp_sendto(g_psUPnP_DiscoveryPCB, p_out, IP_ADDR_UPNP, 1900);
                pbuf_free(p_out);
                ulState++;
            }
            break;
        }

        case 10:
        {
            if(g_ulResponseCount)
            {
                //
                // Build the discover response message.
                //
                usprintf(pcBuf, g_pcResponseRoot1, ((ulIPAddr >>  0) & 0xFF),
                         ((ulIPAddr >>  8) & 0xFF), ((ulIPAddr >> 16) & 0xFF),
                         ((ulIPAddr >> 24) & 0xFF),
                         g_sParameters.usLocationURLPort, pucMACAddr[0],
                         pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
                         pucMACAddr[4], pucMACAddr[5]);

                //
                // Allocate a pbuf for this packet.
                //
                p_out = pbuf_alloc(PBUF_TRANSPORT, strlen(pcBuf), PBUF_RAM);
                if(p_out != NULL)
                {
                    //
                    // Send the response.
                    //
                    memcpy(p_out->payload, pcBuf, strlen(pcBuf));
                    udp_sendto(g_psUPnP_DiscoveryPCB, p_out, IP_ADDR_RESPONSE,
                               g_usResponsePort);
                    pbuf_free(p_out);
                    g_ulResponseCount--;
                    if(g_ulResponseCount == 0)
                    {
                        ulState = 0;
                    }
                }
            }
            break;
        }

        default:
        {
            ulState = 0;
            break;
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
