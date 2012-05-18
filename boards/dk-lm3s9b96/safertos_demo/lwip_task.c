//*****************************************************************************
//
// lwip_task.c - Tasks to serve web pages over Ethernet using lwIP.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "utils/lwiplib.h"
#include "utils/locator.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "cgifuncs.h"
#include "led_task.h"
#include "idle_task.h"
#include "lwip_task.h"
#include "spider_task.h"

//*****************************************************************************
//
// This structure contains the details of a SSI tag.
//
//*****************************************************************************
typedef struct
{
    //
    // The text name of the tag.  If the name is "foo", it will appear in the
    // HTML source as "<!--#foo-->".
    //
    const char *pcName;

    //
    // A pointer to the variable that contains the value of this tag.
    //
    unsigned long *pulValue;
}
tSSITag;

//*****************************************************************************
//
// The list of tags.
//
//*****************************************************************************
const tSSITag g_pTags[] =
{
    { "linksent", &lwip_stats.link.xmit },
    { "linkrecv", &lwip_stats.link.recv },
    { "linkdrop", &lwip_stats.link.drop },
    { "linkcksm", &lwip_stats.link.chkerr },
    { "linklen", &lwip_stats.link.lenerr },
    { "linkmem", &lwip_stats.link.memerr },
    { "linkrte", &lwip_stats.link.rterr },
    { "linkprot", &lwip_stats.link.proterr },
    { "linkopt", &lwip_stats.link.opterr },
    { "linkmisc", &lwip_stats.link.err },
    { "arpsent", &lwip_stats.etharp.xmit },
    { "arprecv", &lwip_stats.etharp.recv },
    { "arpdrop", &lwip_stats.etharp.drop },
    { "arpcksm", &lwip_stats.etharp.chkerr },
    { "arplen", &lwip_stats.etharp.lenerr },
    { "arpmem", &lwip_stats.etharp.memerr },
    { "arprte", &lwip_stats.etharp.rterr },
    { "arpprot", &lwip_stats.etharp.proterr },
    { "arpopt", &lwip_stats.etharp.opterr },
    { "arpmisc", &lwip_stats.etharp.err },
    { "icmpsent", &lwip_stats.icmp.xmit },
    { "icmprecv", &lwip_stats.icmp.recv },
    { "icmpdrop", &lwip_stats.icmp.drop },
    { "icmpcksm", &lwip_stats.icmp.chkerr },
    { "icmplen", &lwip_stats.icmp.lenerr },
    { "icmpmem", &lwip_stats.icmp.memerr },
    { "icmprte", &lwip_stats.icmp.rterr },
    { "icmpprot", &lwip_stats.icmp.proterr },
    { "icmpopt", &lwip_stats.icmp.opterr },
    { "icmpmisc", &lwip_stats.icmp.err },
    { "ipsent", &lwip_stats.ip.xmit },
    { "iprecv", &lwip_stats.ip.recv },
    { "ipdrop", &lwip_stats.ip.drop },
    { "ipcksm", &lwip_stats.ip.chkerr },
    { "iplen", &lwip_stats.ip.lenerr },
    { "ipmem", &lwip_stats.ip.memerr },
    { "iprte", &lwip_stats.ip.rterr },
    { "ipprot", &lwip_stats.ip.proterr },
    { "ipopt", &lwip_stats.ip.opterr },
    { "ipmisc", &lwip_stats.ip.err },
    { "tcpsent", &lwip_stats.tcp.xmit },
    { "tcprecv", &lwip_stats.tcp.recv },
    { "tcpdrop", &lwip_stats.tcp.drop },
    { "tcpcksm", &lwip_stats.tcp.chkerr },
    { "tcplen", &lwip_stats.tcp.lenerr },
    { "tcpmem", &lwip_stats.tcp.memerr },
    { "tcprte", &lwip_stats.tcp.rterr },
    { "tcpprot", &lwip_stats.tcp.proterr },
    { "tcpopt", &lwip_stats.tcp.opterr },
    { "tcpmisc", &lwip_stats.tcp.err },
    { "udpsent", &lwip_stats.udp.xmit },
    { "udprecv", &lwip_stats.udp.recv },
    { "udpdrop", &lwip_stats.udp.drop },
    { "udpcksm", &lwip_stats.udp.chkerr },
    { "udplen", &lwip_stats.udp.lenerr },
    { "udpmem", &lwip_stats.udp.memerr },
    { "udprte", &lwip_stats.udp.rterr },
    { "udpprot", &lwip_stats.udp.proterr },
    { "udpopt", &lwip_stats.udp.opterr },
    { "udpmisc", &lwip_stats.udp.err },
    { "ledrate", &g_ulLEDDelay },
    { "spider", g_pulSpiderDelay }
};

//*****************************************************************************
//
// The number of tags.
//
//*****************************************************************************
#define NUM_TAGS                (sizeof(g_pTags) / sizeof(g_pTags[0]))

//*****************************************************************************
//
// An array of names for the tags, as required by the web server.
//
//*****************************************************************************
static const char *g_ppcSSITags[NUM_TAGS];

//*****************************************************************************
//
// The CGI handler for changing the toggle rate of the LED task.
//
//*****************************************************************************
static char *
ToggleRateCGIHandler(int iIndex, int iNumParams, char *ppcParam[],
                     char *ppcValue[])
{
    tBoolean bParamError;
    long lRate;

    //
    // Get the value of the time parameter.
    //
    bParamError = false;
    lRate = GetCGIParam("time", ppcParam, ppcValue, iNumParams, &bParamError);

    //
    // Return a parameter error if the time parameter was not supplied or was
    // out of range.
    //
    if(bParamError || (lRate < 1) || (lRate > 10000))
    {
        return("/perror.htm");
    }

    //
    // Update the delay between toggles of the LED.
    //
    g_ulLEDDelay = lRate;

    //
    // Refresh the current page.
    //
    return("/io.ssi");
}

//*****************************************************************************
//
// The CGI handler for changing the spider speed.
//
//*****************************************************************************
static char *
SpiderSpeedCGIHandler(int iIndex, int iNumParams, char *ppcParam[],
                      char *ppcValue[])
{
    tBoolean bParamError;
    long lRate;

    //
    // Get the value of the time parameter.
    //
    bParamError = false;
    lRate = GetCGIParam("time", ppcParam, ppcValue, iNumParams, &bParamError);

    //
    // Return a parameter error if the time parameter was not supplied or was
    // out of range.
    //
    if(bParamError)
    {
        return("/perror.htm");
    }

    //
    // Update the speed of the spiders.
    //
    SpiderSpeedSet(lRate);

    //
    // Refresh the current page.
    //
    return("/io.ssi");
}

//*****************************************************************************
//
// The array of CGI handlers, as required by the web server.
//
//*****************************************************************************
static const tCGI g_psCGIs[] =
{
    { "/toggle_rate.cgi", ToggleRateCGIHandler },
    { "/spider_rate.cgi", SpiderSpeedCGIHandler }
};

//*****************************************************************************
//
// The number of CGI handlers.
//
//*****************************************************************************
#define NUM_CGIS                (sizeof(g_psCGIs) / sizeof(g_psCGIs[0]))

//*****************************************************************************
//
// The handler for server side includes.
//
//*****************************************************************************
static int
SSIHandler(int iIndex, char *pcInsert, int iInsertLen)
{
    //
    // Replace the tag with an appropriate value.
    //
    if(iIndex < NUM_TAGS)
    {
        //
        // This is a valid tag, so print out its value.
        //
        usnprintf(pcInsert, iInsertLen, "%d", *(g_pTags[iIndex].pulValue));
    }
    else
    {
        //
        // This is an unknown tag.
        //
        usnprintf(pcInsert, iInsertLen, "??");
    }

    //
    // Determine the length of the replacement text.
    //
    for(iIndex = 0; pcInsert[iIndex] != 0; iIndex++)
    {
    }

    //
    // Return the length of the replacement text.
    //
    return(iIndex);
}

//*****************************************************************************
//
// Sets up the additional lwIP raw API services provided by the application.
//
//*****************************************************************************
void
SetupServices(void *pvArg)
{
    unsigned char pucMAC[6];
    unsigned long ulIdx;

    //
    // Setup the device locator service.
    //
    LocatorInit();
    lwIPLocalMACGet(pucMAC);
    LocatorMACAddrSet(pucMAC);
    LocatorAppTitleSet("DK-LM3S9B96 safertos_demo");

    //
    // Initialize the sample httpd server.
    //
    httpd_init();

    //
    // Initialize the tag array used by the web server's SSI processing.
    //
    for(ulIdx = 0; ulIdx < NUM_TAGS; ulIdx++)
    {
        g_ppcSSITags[ulIdx] = g_pTags[ulIdx].pcName;
    }

    //
    // Register the SSI tags and handler with the web server.
    //
    http_set_ssi_handler(SSIHandler, g_ppcSSITags, NUM_TAGS);

    //
    // Register the CGI handlers with the web server.
    //
    http_set_cgi_handlers(g_psCGIs, NUM_CGIS);
}

//*****************************************************************************
//
// Initializes the lwIP tasks.
//
//*****************************************************************************
unsigned long
lwIPTaskInit(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMAC[6];

    //
    // Get the MAC address from the user registers.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);
    if((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
        return(1);
    }

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    //
    pucMAC[0] = ((ulUser0 >>  0) & 0xff);
    pucMAC[1] = ((ulUser0 >>  8) & 0xff);
    pucMAC[2] = ((ulUser0 >> 16) & 0xff);
    pucMAC[3] = ((ulUser1 >>  0) & 0xff);
    pucMAC[4] = ((ulUser1 >>  8) & 0xff);
    pucMAC[5] = ((ulUser1 >> 16) & 0xff);

    //
    // Lower the priority of the Ethernet interrupt handler.  This is required
    // so that the interrupt handler can safely call the interrupt-safe
    // SafeRTOS functions (specifically to send messages to the queue).
    //
    ROM_IntPrioritySet(INT_ETH, 0xc0);

    //
    // Initialize lwIP.
    //
    lwIPInit(pucMAC, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the remaining services inside the TCP/IP thread's context.
    //
    tcpip_callback(SetupServices, 0);

    //
    // The base lwIP stack uses two threads (one for managing the Ethernet
    // interrupt and one for running the stack), so increment the task count
    // accordingly.
    //
    TaskCreated();
    TaskCreated();

    //
    // Success.
    //
    return(0);
}
