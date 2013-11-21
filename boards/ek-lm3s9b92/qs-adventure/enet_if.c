//*****************************************************************************
//
// enet_if.c - Ethernet telnet interface for the game.
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
// This is part of revision 9107 of the EK-LM3S9B92 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/uartstdio.h"
#include "zip/ztypes.h"
#include "common.h"

//*****************************************************************************
//
// Telnet commands, as defined by RFC854.
//
//*****************************************************************************
#define TELNET_IAC              255
#define TELNET_WILL             251
#define TELNET_WONT             252
#define TELNET_DO               253
#define TELNET_DONT             254
#define TELNET_SE               240
#define TELNET_NOP              241
#define TELNET_DATA_MARK        242
#define TELNET_BREAK            243
#define TELNET_IP               244
#define TELNET_AO               245
#define TELNET_AYT              246
#define TELNET_EC               247
#define TELNET_EL               248
#define TELNET_GA               249
#define TELNET_SB               250

//*****************************************************************************
//
// Telnet options, as defined by RFC856-RFC861.
//
//*****************************************************************************
#define TELNET_OPT_BINARY       0
#define TELNET_OPT_ECHO         1
#define TELNET_OPT_SUPPRESS_GA  3
#define TELNET_OPT_STATUS       5
#define TELNET_OPT_TIMING_MARK  6
#define TELNET_OPT_EXOPL        255

//*****************************************************************************
//
// A twirling line used to indicate that DHCP/AutoIP address acquisition is in
// progress.
//
//*****************************************************************************
static char g_pcTwirl[4] = { '\\', '|', '/', '-' };

//*****************************************************************************
//
// The index into the twirling line array of the next line orientation to be
// printed.
//
//*****************************************************************************
static unsigned long g_ulTwirlPos = 0;

//*****************************************************************************
//
// The most recently assigned IP address.  This is used to detect when the IP
// address has changed (due to DHCP/AutoIP) so that the new address can be
// printed.
//
//*****************************************************************************
static unsigned long g_ulLastIPAddr = 0;

//*****************************************************************************
//
// The possible states of the telnet option parser.
//
//*****************************************************************************
typedef enum
{
    //
    // The telnet option parser is in its normal mode.  Characters are passed
    // as is until an IAC byte is received.
    //
    STATE_NORMAL,

    //
    // The previous character received by the telnet option parser was an IAC
    // byte.
    //
    STATE_IAC,

    //
    // The previous character sequence received by the telnet option parser was
    // IAC WILL.
    //
    STATE_WILL,

    //
    // The previous character sequence received by the telnet option parser was
    // IAC WONT.
    //
    STATE_WONT,

    //
    // The previous character sequence received by the telnet option parser was
    // was IAC DO.
    //
    STATE_DO,

    //
    // The previous character sequence received by the telnet option parser was
    // was IAC DONT.
    //
    STATE_DONT,
}
tTelnetState;

//*****************************************************************************
//
// The current state of the telnet option parser.
//
//*****************************************************************************
static tTelnetState g_eState;

//*****************************************************************************
//
// A structure that contains the state of the options supported by the telnet
// server, along with the possible flags.
//
//*****************************************************************************
typedef struct
{
    //
    // The option byte.
    //
    unsigned char ucOption;

    //
    // The flags for this option.  The bits in this byte are defined by
    // OPT_FLAG_WILL and OPT_FLAG_DO.
    //
    unsigned char ucFlags;
}
tTelnetOpts;

//*****************************************************************************
//
// The bit in the ucFlags member of tTelnetOpts that is set when the remote
// client has sent a WILL request and the server has accepted it.
//
//*****************************************************************************
#define OPT_FLAG_WILL           1

//*****************************************************************************
//
// The bit in the ucFlags member of tTelnetOpts that is set when the remote
// client has sent a DO request and the server has accepted it.
//
//*****************************************************************************
#define OPT_FLAG_DO             2

//*****************************************************************************
//
// The telnet options supported by this server.
//
//*****************************************************************************
static tTelnetOpts g_psOptions[] =
{
    //
    // This telnet server will always suppress go ahead generation, regardless
    // of this setting.
    //
    { TELNET_OPT_SUPPRESS_GA, (1 << OPT_FLAG_WILL) },
    { TELNET_OPT_ECHO, (1 << OPT_FLAG_DO) },
};

//*****************************************************************************
//
// The initialization sequence sent to a remote telnet client when it first
// connects to the telnet server.
//
//*****************************************************************************
static const unsigned char g_pucTelnetInit[] =
{
    TELNET_IAC, TELNET_DO, TELNET_OPT_SUPPRESS_GA,
    TELNET_IAC, TELNET_WILL, TELNET_OPT_ECHO
};

//*****************************************************************************
//
// A count of the number of bytes that have been transmitted but have not yet
// been ACKed.
//
//*****************************************************************************
static volatile unsigned long g_ulTelnetOutstanding;

//*****************************************************************************
//
// A value that is non-zero when the telnet connection should be closed down.
//
//*****************************************************************************
static unsigned long g_ulTelnetClose;

//*****************************************************************************
//
// A buffer used to construct a packet of data to be transmitted to the telnet
// client.
//
//*****************************************************************************
static unsigned char g_pucTelnetBuffer[512];

//*****************************************************************************
//
// The number of bytes of valid data in the telnet packet buffer.
//
//*****************************************************************************
static volatile unsigned long g_ulTelnetLength;

//*****************************************************************************
//
// A buffer used to receive data from the telnet connection.
//
//*****************************************************************************
static unsigned char g_pucTelnetRecvBuffer[512];

//*****************************************************************************
//
// The offset into g_pucTelnetRecvBuffer of the next location to be written in
// the buffer.  The buffer is full if this value is one less than
// g_ulTelnetRecvRead (modulo the buffer size).
//
//*****************************************************************************
static volatile unsigned long g_ulTelnetRecvWrite = 0;

//*****************************************************************************
//
// The offset into g_pucTelnetRecvRead of the next location to be read from the
// buffer.  The buffer is empty if this value is equal to g_ulTelnetRecvWrite.
//
//*****************************************************************************
static volatile unsigned long g_ulTelnetRecvRead = 0;

//*****************************************************************************
//
// A pointer to the telnet session PCB data structure.
//
//*****************************************************************************
static struct tcp_pcb *g_psTelnetPCB = NULL;

//*****************************************************************************
//
// The character most recently received via the telnet interface.  This is used
// to convert CR/LF sequences into a simple CR sequence.
//
//*****************************************************************************
static unsigned char g_ucTelnetPrevious = 0;

//*****************************************************************************
//
// Writes a character into the telnet receive buffer.
//
//*****************************************************************************
static void
TelnetRecvBufferWrite(unsigned char ucChar)
{
    unsigned long ulWrite;

    //
    // Ignore this character if it is the NUL character.
    //
    if(ucChar == 0)
    {
        return;
    }

    //
    // Ignore this character if it is the second part of a CR/LF or LF/CR
    // sequence.
    //
    if(((ucChar == '\r') && (g_ucTelnetPrevious == '\n')) ||
       ((ucChar == '\n') && (g_ucTelnetPrevious == '\r')))
    {
        return;
    }

    //
    // Store this character into the receive buffer if there is space for it.
    //
    ulWrite = g_ulTelnetRecvWrite;
    if(((ulWrite + 1) % sizeof(g_pucTelnetRecvBuffer)) != g_ulTelnetRecvRead)
    {
        g_pucTelnetRecvBuffer[ulWrite] = ucChar;
        g_ulTelnetRecvWrite = (ulWrite + 1) % sizeof(g_pucTelnetRecvBuffer);
    }

    //
    // Save this character as the previously received telnet character.
    //
    g_ucTelnetPrevious = ucChar;
}

//*****************************************************************************
//
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************
void
lwIPHostTimerHandler(void)
{
    unsigned long ulIPAddress;

    //
    // Get the local IP address.
    //
    ulIPAddress = lwIPLocalIPAddrGet();

    //
    // See if an IP address has been assigned.
    //
    if(ulIPAddress == 0)
    {
        //
        // Draw a spinning line to indicate that the IP address is being
        // discoverd.
        //
        UARTprintf("\b%c", g_pcTwirl[g_ulTwirlPos]);

        //
        // Update the index into the twirl.
        //
        g_ulTwirlPos = (g_ulTwirlPos + 1) & 3;
    }

    //
    // Check if IP address has changed, and display if it has.
    //
    else if(ulIPAddress != g_ulLastIPAddr)
    {
        //
        // Display the new IP address.
        //
        UARTprintf("\rIP: %d.%d.%d.%d       \n", ulIPAddress & 0xff,
                   (ulIPAddress >> 8) & 0xff, (ulIPAddress >> 16) & 0xff,
                   (ulIPAddress >> 24) & 0xff);

        //
        // Save the new IP address.
        //
        g_ulLastIPAddr = ulIPAddress;

        //
        // Display the new network mask.
        //
        ulIPAddress = lwIPLocalNetMaskGet();
        UARTprintf("Netmask: %d.%d.%d.%d\n", ulIPAddress & 0xff,
                   (ulIPAddress >> 8) & 0xff, (ulIPAddress >> 16) & 0xff,
                   (ulIPAddress >> 24) & 0xff);

        //
        // Display the new gateway address.
        //
        ulIPAddress = lwIPLocalGWAddrGet();
        UARTprintf("Gateway: %d.%d.%d.%d\n", ulIPAddress & 0xff,
                   (ulIPAddress >> 8) & 0xff, (ulIPAddress >> 16) & 0xff,
                   (ulIPAddress >> 24) & 0xff);
    }
}

//*****************************************************************************
//
// This function is called when the the TCP connection should be closed.
//
//*****************************************************************************
static void
TelnetClose(struct tcp_pcb *pcb)
{
    //
    // Clear out all of the TCP callbacks.
    //
    tcp_sent(pcb, NULL);
    tcp_recv(pcb, NULL);
    tcp_err(pcb, NULL);

    //
    // Clear the telnet data structure pointer, to indicate that there is no
    // longer a connection.
    //
    g_psTelnetPCB = NULL;

    //
    // Close the TCP connection.
    //
    tcp_close(pcb);

    //
    // Reset the client IP in the locator response packet.
    //
    LocatorClientIPSet(0);
}

//*****************************************************************************
//
// The periodic tick function for the Ethernet interface.
//
//*****************************************************************************
void
EnetIFTick(unsigned long ulMS)
{
    unsigned long ulLength;

    //
    // Call the lwIP timer handler.
    //
    lwIPTimer(ulMS);

    //
    // See if there is a telnet connection and data waiting to be transmitted.
    //
    ulLength = g_ulTelnetLength;
    if(g_psTelnetPCB && (ulLength != 0))
    {
        //
        // Write the data from the transmit buffer.
        //
        tcp_write(g_psTelnetPCB, g_pucTelnetBuffer, ulLength, 1);

        //
        // Increment the count of outstanding bytes.
        //
        g_ulTelnetOutstanding += ulLength;

        //
        // Output the telnet data.
        //
        tcp_output(g_psTelnetPCB);

        //
        // Reset the size of the data in the transmit buffer.
        //
        g_ulTelnetLength = 0;
    }

    //
    // See if the telnet connection should be closed; this will only occur once
    // all transmitted data has been ACKed by the client (so that some or all
    // of the final message is not lost).
    //
    if(g_psTelnetPCB && (g_ulTelnetOutstanding == 0) && (g_ulTelnetClose != 0))
    {
        TelnetClose(g_psTelnetPCB);
    }
}

//*****************************************************************************
//
// This function will handle a WILL request for a telnet option.  If it is an
// option that is known by the telnet server, a DO response will be generated
// if the option is not already enabled.  For unknown options, a DONT response
// will always be generated.
//
// The response (if any) is written into the telnet transmit buffer.
//
//*****************************************************************************
static void
TelnetProcessWill(unsigned char ucOption)
{
    unsigned long ulIdx;

    //
    // Loop through the known options.
    //
    for(ulIdx = 0; ulIdx < (sizeof(g_psOptions) / sizeof(g_psOptions[0]));
        ulIdx++)
    {
        //
        // See if this option matches the option in question.
        //
        if(g_psOptions[ulIdx].ucOption == ucOption)
        {
            //
            // See if the WILL flag for this option has already been set.
            //
            if(HWREGBITB(&(g_psOptions[ulIdx].ucFlags), OPT_FLAG_WILL) == 0)
            {
                //
                // Set the WILL flag for this option.
                //
                HWREGBITB(&(g_psOptions[ulIdx].ucFlags), OPT_FLAG_WILL) = 1;

                //
                // Send a DO response to this option.
                //
                g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_IAC;
                g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_DO;
                g_pucTelnetBuffer[g_ulTelnetLength++] = ucOption;
            }

            //
            // Return without any further processing.
            //
            return;
        }
    }

    //
    // This option is not recognized, so send a DONT response.
    //
    g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_IAC;
    g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_DONT;
    g_pucTelnetBuffer[g_ulTelnetLength++] = ucOption;
}

//*****************************************************************************
//
// This function will handle a WONT request for a telnet option.  If it is an
// option that is known by the telnet server, a DONT response will be generated
// if the option is not already disabled.  For unknown options, a DONT response
// will always be generated.
//
// The response (if any) is written into the telnet transmit buffer.
//
//*****************************************************************************
static void
TelnetProcessWont(unsigned char ucOption)
{
    unsigned long ulIdx;

    //
    // Loop through the known options.
    //
    for(ulIdx = 0; ulIdx < (sizeof(g_psOptions) / sizeof(g_psOptions[0]));
        ulIdx++)
    {
        //
        // See if this option matches the option in question.
        //
        if(g_psOptions[ulIdx].ucOption == ucOption)
        {
            //
            // See if the WILL flag for this option is currently set.
            //
            if(HWREGBITB(&(g_psOptions[ulIdx].ucFlags), OPT_FLAG_WILL) == 1)
            {
                //
                // Clear the WILL flag for this option.
                //
                HWREGBITB(&(g_psOptions[ulIdx].ucFlags), OPT_FLAG_WILL) = 0;

                //
                // Send a DONT response to this option.
                //
                g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_IAC;
                g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_DONT;
                g_pucTelnetBuffer[g_ulTelnetLength++] = ucOption;
            }

            //
            // Return without any further processing.
            //
            return;
        }
    }

    //
    // This option is not recognized, so send a DONT response.
    //
    g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_IAC;
    g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_DONT;
    g_pucTelnetBuffer[g_ulTelnetLength++] = ucOption;
}

//*****************************************************************************
//
// This function will handle a DO request for a telnet option.  If it is an
// option that is known by the telnet server, a WILL response will be generated
// if the option is not already enabled.  For unknown options, a WONT response
// will always be generated.
//
// The response (if any) is written into the telnet transmit buffer.
//
//*****************************************************************************
static void
TelnetProcessDo(unsigned char ucOption)
{
    unsigned long ulIdx;

    //
    // Loop through the known options.
    //
    for(ulIdx = 0; ulIdx < (sizeof(g_psOptions) / sizeof(g_psOptions[0]));
        ulIdx++)
    {
        //
        // See if this option matches the option in question.
        //
        if(g_psOptions[ulIdx].ucOption == ucOption)
        {
            //
            // See if the DO flag for this option has already been set.
            //
            if(HWREGBITB(&(g_psOptions[ulIdx].ucFlags), OPT_FLAG_DO) == 0)
            {
                //
                // Set the DO flag for this option.
                //
                HWREGBITB(&(g_psOptions[ulIdx].ucFlags), OPT_FLAG_DO) = 1;

                //
                // Send a WILL response to this option.
                //
                g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_IAC;
                g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_WILL;
                g_pucTelnetBuffer[g_ulTelnetLength++] = ucOption;
            }

            //
            // Return without any further processing.
            //
            return;
        }
    }

    //
    // This option is not recognized, so send a WONT response.
    //
    g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_IAC;
    g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_WONT;
    g_pucTelnetBuffer[g_ulTelnetLength++] = ucOption;
}

//*****************************************************************************
//
// This funciton will handle a DONT request for a telnet option.  If it is an
// option that is known by the telnet server, a WONT response will be generated
// if the option is not already disabled.  For unknown options, a WONT resopnse
// will always be generated.
//
// The response (if any) is written into the telnet transmit buffer.
//
//*****************************************************************************
static void
TelnetProcessDont(unsigned char ucOption)
{
    unsigned long ulIdx;

    //
    // Loop through the known options.
    //
    for(ulIdx = 0; ulIdx < (sizeof(g_psOptions) / sizeof(g_psOptions[0]));
        ulIdx++)
    {
        //
        // See if this option matches the option in question.
        //
        if(g_psOptions[ulIdx].ucOption == ucOption)
        {
            //
            // See if the DO flag for this option is currently set.
            //
            if(HWREGBITB(&(g_psOptions[ulIdx].ucFlags), OPT_FLAG_DO) == 1)
            {
                //
                // Clear the DO flag for this option.
                //
                HWREGBITB(&(g_psOptions[ulIdx].ucFlags), OPT_FLAG_DO) = 0;

                //
                // Send a WONT response to this option.
                //
                g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_IAC;
                g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_WONT;
                g_pucTelnetBuffer[g_ulTelnetLength++] = ucOption;
            }

            //
            // Return without any further processing.
            //
            return;
        }
    }

    //
    // This option is not recognized, so send a WONT response.
    //
    g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_IAC;
    g_pucTelnetBuffer[g_ulTelnetLength++] = TELNET_WONT;
    g_pucTelnetBuffer[g_ulTelnetLength++] = ucOption;
}

//*****************************************************************************
//
// This function processes a character received from the telnet port, handling
// the interpretation of telnet commands (as indicated by the telnet interpret
// as command (IAC) byte).
//
//*****************************************************************************
static void
TelnetProcessCharacter(unsigned char ucChar)
{
    //
    // Determine the current state of the telnet command parser.
    //
    switch(g_eState)
    {
        //
        // The normal state of the parser, were each character is either sent
        // to the UART or is a telnet IAC character.
        //
        case STATE_NORMAL:
        {
            //
            // See if this character is the IAC character.
            //
            if(ucChar == TELNET_IAC)
            {
                //
                // Skip this character and go to the IAC state.
                //
                g_eState = STATE_IAC;
            }
            else
            {
                //
                // Write this character to the receive buffer.
                //
                TelnetRecvBufferWrite(ucChar);
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The previous character was the IAC character.
        //
        case STATE_IAC:
        {
            //
            // Determine how to interpret this character.
            //
            switch(ucChar)
            {
                //
                // See if this character is also an IAC character.
                //
                case TELNET_IAC:
                {
                    //
                    // Write 0xff to the receive buffer.
                    //
                    TelnetRecvBufferWrite(0xff);

                    //
                    // Switch back to normal mode.
                    //
                    g_eState = STATE_NORMAL;

                    //
                    // This character has been handled.
                    //
                    break;
                }

                //
                // See if this character is the WILL request.
                //
                case TELNET_WILL:
                {
                    //
                    // Switch to the WILL mode; the next character will have
                    // the option in question.
                    //
                    g_eState = STATE_WILL;

                    //
                    // This character has been handled.
                    //
                    break;
                }

                //
                // See if this character is the WONT request.
                //
                case TELNET_WONT:
                {
                    //
                    // Switch to the WONT mode; the next character will have
                    // the option in question.
                    //
                    g_eState = STATE_WONT;

                    //
                    // This character has been handled.
                    //
                    break;
                }

                //
                // See if this character is the DO request.
                //
                case TELNET_DO:
                {
                    //
                    // Switch to the DO mode; the next character will have the
                    // option in question.
                    //
                    g_eState = STATE_DO;

                    //
                    // This character has been handled.
                    //
                    break;
                }

                //
                // See if this character is the DONT request.
                //
                case TELNET_DONT:
                {
                    //
                    // Switch to the DONT mode; the next character will have
                    // the option in question.
                    //
                    g_eState = STATE_DONT;

                    //
                    // This character has been handled.
                    //
                    break;
                }

                //
                // See if this character is the AYT request.
                //
                case TELNET_AYT:
                {
                    //
                    // Send a short string back to the client so that it knows
                    // that the server is still alive.
                    //
                    g_pucTelnetBuffer[g_ulTelnetLength++] = '\r';
                    g_pucTelnetBuffer[g_ulTelnetLength++] = '\n';
                    g_pucTelnetBuffer[g_ulTelnetLength++] = '[';
                    g_pucTelnetBuffer[g_ulTelnetLength++] = 'Y';
                    g_pucTelnetBuffer[g_ulTelnetLength++] = 'e';
                    g_pucTelnetBuffer[g_ulTelnetLength++] = 's';
                    g_pucTelnetBuffer[g_ulTelnetLength++] = ']';
                    g_pucTelnetBuffer[g_ulTelnetLength++] = '\r';
                    g_pucTelnetBuffer[g_ulTelnetLength++] = '\n';

                    //
                    // Switch back to normal mode.
                    //
                    g_eState = STATE_NORMAL;

                    //
                    // This character has been handled.
                    //
                    break;
                }

                //
                // Explicitly ignore the GA and NOP request, plus provide a
                // catch-all ignore for unrecognized requests.
                //
                case TELNET_GA:
                case TELNET_NOP:
                default:
                {
                    //
                    // Switch back to normal mode.
                    //
                    g_eState = STATE_NORMAL;

                    //
                    // This character has been handled.
                    //
                    break;
                }
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The previous character sequence was IAC WILL.
        //
        case STATE_WILL:
        {
            //
            // Process the WILL request on this option.
            //
            TelnetProcessWill(ucChar);

            //
            // Switch back to normal mode.
            //
            g_eState = STATE_NORMAL;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The previous character sequence was IAC WONT.
        //
        case STATE_WONT:
        {
            //
            // Process the WONT request on this option.
            //
            TelnetProcessWont(ucChar);

            //
            // Switch back to normal mode.
            //
            g_eState = STATE_NORMAL;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The previous character sequence was IAC DO.
        //
        case STATE_DO:
        {
            //
            // Process the DO request on this option.
            //
            TelnetProcessDo(ucChar);

            //
            // Switch back to normal mode.
            //
            g_eState = STATE_NORMAL;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The previous character sequence was IAC DONT.
        //
        case STATE_DONT:
        {
            //
            // Process the DONT request on this option.
            //
            TelnetProcessDont(ucChar);

            //
            // Switch back to normal mode.
            //
            g_eState = STATE_NORMAL;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // A catch-all for unknown states.  This should never be reached, but
        // is provided just in case it is ever needed.
        //
        default:
        {
            //
            // Switch back to normal mode.
            //
            g_eState = STATE_NORMAL;

            //
            // This state has been handled.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// This function is called when the lwIP TCP/IP stack has an incoming packet to
// be processed.
//
//*****************************************************************************
static err_t
TelnetReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    struct pbuf *q;
    unsigned long ulIdx;
    unsigned char *pucData;

    //
    // Process the incoming packet.
    //
    if((err == ERR_OK) && (p != NULL))
    {
        //
        // Accept the packet from TCP.
        //
        tcp_recved(pcb, p->tot_len);

        //
        // Loop through the pbufs in this packet.
        //
        for(q = p, pucData = q->payload; q != NULL; q = q->next)
        {
            //
            // Loop through the bytes in this pbuf.
            //
            for(ulIdx = 0; ulIdx < q->len; ulIdx++)
            {
                //
                // Process this character.
                //
                TelnetProcessCharacter(pucData[ulIdx]);
            }
        }

        //
        // Free the pbuf.
        //
        pbuf_free(p);
    }

    //
    // If a null packet is passed in, close the connection.
    //
    else if((err == ERR_OK) && (p == NULL))
    {
        g_ulTelnetLength = 0;
        TelnetClose(pcb);
    }

    //
    // Return okay.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
// This function is called when the lwIP TCP/IP stack has detected an error.
// The connection is no longer valid.
//
//*****************************************************************************
static void
TelnetError(void *arg, err_t err)
{
    //
    // If there is currently a telnet connection, then halt the Z-machine
    // interpreter.
    //
    if(g_psTelnetPCB)
    {
        halt = 1;
    }

    //
    // Reset our connection.
    //
    g_psTelnetPCB = NULL;

    //
    // Reset the client IP in the locator response packet.
    //
    LocatorClientIPSet(0);
}

//*****************************************************************************
//
// This function is called when the lwIP TCP/IP stack has received an
// acknowledge for data that has been transmitted.
//
//*****************************************************************************
static err_t
TelnetSent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
    //
    // See if this is for the game connection or for a secondary connection.
    //
    if(arg)
    {
        //
        // Decrement the count of outstanding bytes.
        //
        g_ulTelnetOutstanding -= len;
    }
    else
    {
        //
        // See if this is the ACK for the error message.
        //
        if(len == sizeof(g_pucErrorMessage))
        {
            //
            // Close this telnet connection now that the error message has been
            // transmitted.
            //
            tcp_sent(pcb, NULL);
            tcp_close(pcb);
        }
    }

    //
    // Return OK.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
// This function is called when the lwIP TCP/IP stack has an incoming
// connection request on the telnet port.
//
//*****************************************************************************
static err_t
TelnetAccept(void *arg, struct tcp_pcb *pcb, err_t err)
{
    //
    // Check if already connected.
    //
    if(g_psTelnetPCB || (g_ulGameIF != GAME_IF_NONE))
    {
        //
        // There is already a game in progress, so refuse this connection with
        // a message indicating this fact.
        //
        tcp_accepted(pcb);
        tcp_arg(pcb, NULL);
        tcp_sent(pcb, TelnetSent);
        tcp_write(pcb, g_pucErrorMessage, sizeof(g_pucErrorMessage), 1);
        tcp_output(pcb);

        //
        // Temporarily accept this connection until the message is transmitted.
        //
        return(ERR_OK);
    }

    //
    // Select the Ethernet interface for game play.
    //
    g_ulGameIF = GAME_IF_ENET;

    //
    // Set the connection as busy.
    //
    g_psTelnetPCB = pcb;

    //
    // Accept this connection.
    //
    tcp_accepted(pcb);

    //
    // Setup the TCP connection priority.
    //
    tcp_setprio(pcb, TCP_PRIO_MIN);

    //
    // Setup the TCP callback argument.
    //
    tcp_arg(pcb, pcb);

    //
    // Setup the TCP receive function.
    //
    tcp_recv(pcb, TelnetReceive);

    //
    // Setup the TCP error function.
    //
    tcp_err(pcb, TelnetError);

    //
    // Setup the TCP sent callback function.
    //
    tcp_sent(pcb, TelnetSent);

    //
    // Initialize the count of outstanding bytes.  The initial byte acked as
    // part of the SYN -> SYN/ACK sequence is included so that the byte count
    // works out correctly at the end.
    //
    g_ulTelnetOutstanding = sizeof(g_pucTelnetInit) + 1;

    //
    // Do not close the telnet connection until requested.
    //
    g_ulTelnetClose = 0;

    //
    // Send the telnet initialization string.
    //
    tcp_write(pcb, g_pucTelnetInit, sizeof(g_pucTelnetInit), 1);
    tcp_output(pcb);

    //
    // Set the client IP address in the locator response packet.
    //
    LocatorClientIPSet(pcb->remote_ip.addr);

    //
    // Return a success code.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
// This function initializes the Ethernet telnet interface to the game.
//
//*****************************************************************************
void
EnetIFInit(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACArray[8];
    void *pcb;

    //
    // Enable and Reset the Ethernet Controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

    //
    // Enable Port F for Ethernet LEDs.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinConfigure(GPIO_PF2_LED1);
    GPIOPinConfigure(GPIO_PF3_LED0);
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Read the MAC address from the user registers.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);
    if((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
        //
        // We should never get here.  This is an error if the MAC address has
        // not been programmed into the device.  Exit the program.
        //
        UARTprintf("MAC Address Not Programmed!\n");
        while(1)
        {
        }
    }

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    //
    pucMACArray[0] = ((ulUser0 >>  0) & 0xff);
    pucMACArray[1] = ((ulUser0 >>  8) & 0xff);
    pucMACArray[2] = ((ulUser0 >> 16) & 0xff);
    pucMACArray[3] = ((ulUser1 >>  0) & 0xff);
    pucMACArray[4] = ((ulUser1 >>  8) & 0xff);
    pucMACArray[5] = ((ulUser1 >> 16) & 0xff);

    //
    // Initialze the lwIP library, using DHCP.
    //
    lwIPInit(pucMACArray, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACArray);
    LocatorAppTitleSet("EK-LM3S9B92 qs-adventure");

    //
    // Initialize the application to listen on the telnet port.
    //
    pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, 23);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, TelnetAccept);

    //
    // Indicate that DHCP has started.
    //
    UARTprintf("Waiting for IP... ");
}

//*****************************************************************************
//
// Reads a character from the telnet interface.
//
//*****************************************************************************
unsigned char
EnetIFRead(void)
{
    unsigned long ulRead;
    unsigned char ucRet;

    //
    // Return a NUL if there is no data in the receive buffer.
    //
    ulRead = g_ulTelnetRecvRead;
    if(ulRead == g_ulTelnetRecvWrite)
    {
        return(0);
    }

    //
    // Read the next byte from the receive buffer.
    //
    ucRet = g_pucTelnetRecvBuffer[ulRead];
    g_ulTelnetRecvRead = (ulRead + 1) % sizeof(g_pucTelnetRecvBuffer);

    //
    // Return the byte that was read.
    //
    return(ucRet);
}

//*****************************************************************************
//
// Writes a character to the telnet interface.
//
//*****************************************************************************
void
EnetIFWrite(unsigned char ucChar)
{
    //
    // Delay until there is some space in the output buffer.  The buffer is not
    // completly filled here to leave some room for the processing of received
    // telnet commands.
    //
    while(g_ulTelnetLength > (sizeof(g_pucTelnetBuffer) - 32))
    {
    }

    //
    // Write this character into the output buffer.  Disable Ethernet
    // interrupts during this process in order to prevent an intervening
    // interrupt from corrupting the output buffer.
    //
    ROM_IntDisable(INT_ETH);
    g_pucTelnetBuffer[g_ulTelnetLength++] = ucChar;
    ROM_IntEnable(INT_ETH);
}

//*****************************************************************************
//
// Closes the telnet connection.
//
//*****************************************************************************
void
EnetIFClose(void)
{
    //
    // Request that the telnet connection be closed as soon as all the
    // transmitted has been ACKed.
    //
    g_ulTelnetClose = 1;
}
