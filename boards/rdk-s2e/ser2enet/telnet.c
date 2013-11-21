//*****************************************************************************
//
// telnet.c - Telnet session support routines.
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

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "config.h"
#include "serial.h"
#include "telnet.h"
#ifdef DEBUG_UART
#include "utils/uartstdio.h"
#endif

//*****************************************************************************
//
//! \addtogroup telnet_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The bit in the flag that is set when the remote client has sent a WILL
//! request for SUPPRESS_GA and the server has accepted it.
//
//*****************************************************************************
#define OPT_FLAG_WILL_SUPPRESS_GA \
                                0

//*****************************************************************************
//
//! The bit in the flag that is set when the remote client has sent a DO
//! request for SUPPRESS_GA and the server has accepted it.
//
//*****************************************************************************
#define OPT_FLAG_DO_SUPPRESS_GA 1

//*****************************************************************************
//
//! The bit in the flag that is set when the remote client has sent a WILL
//! request for COM_PORT and the server has accepted it.
//
//*****************************************************************************
#if CONFIG_RFC2217_ENABLED
#define OPT_FLAG_WILL_RFC2217   2
#endif

//*****************************************************************************
//
//! The bit in the flag that is set when the remote client has sent a DO
//! request for COM_PORT and the server has accepted it.
//
//*****************************************************************************
#if CONFIG_RFC2217_ENABLED
#define OPT_FLAG_DO_RFC2217     3
#endif

//*****************************************************************************
//
//! The bit in the flag that is set when a connection is operating as a telnet
//! server.  If clear, this implies that this connection is a telnet client.
//
//*****************************************************************************
#define OPT_FLAG_SERVER         4

//*****************************************************************************
//
//! The possible states of the TCP session.
//
//*****************************************************************************
typedef enum
{
    //
    //! The TCP session is idle.  No connection has been attempted, nor has it
    //! been configured to listen on any port.
    //
    STATE_TCP_IDLE,

    //
    //! The TCP session is listening (server mode).
    //
    STATE_TCP_LISTEN,

    //
    //! The TCP session is connecting (client mode).
    //
    STATE_TCP_CONNECTING,

    //
    //! The TCP session is connected.
    //
    STATE_TCP_CONNECTED,
}
tTCPState;

//*****************************************************************************
//
//! The possible states of the telnet option parser.
//
//*****************************************************************************
typedef enum
{
    //
    //! The telnet option parser is in its normal mode.  Characters are passed
    //! as is until an IAC byte is received.
    //
    STATE_NORMAL,

    //
    //! The previous character received by the telnet option parser was an IAC
    //! byte.
    //
    STATE_IAC,

    //
    //! The previous character sequence received by the telnet option parser
    //! was IAC WILL.
    //
    STATE_WILL,

    //
    //! The previous character sequence received by the telnet option parser
    //! was IAC WONT.
    //
    STATE_WONT,

    //
    //! The previous character sequence received by the telnet option parser
    //! was IAC DO.
    //
    STATE_DO,

    //
    //! The previous character sequence received by the telnet option parser
    //! was IAC DONT.
    //
    STATE_DONT,

    //
    //! The previous character sequence received by the telnet option parser
    //! was IAC SB.
    //
    STATE_SB,

    //
    //! The previous character sequence received by the telnet option parser
    //! was IAC SB n, where n is an unsupported option.
    //
    STATE_SB_IGNORE,

    //
    //! The previous character sequence received by the telnet option parser
    //! was IAC SB n, where n is an unsupported option.
    //
    STATE_SB_IGNORE_IAC,

    //
    //! The previous character sequence received by the telnet option parser
    //! was IAC SB COM-PORT-OPTION (in other words, RFC 2217).
    //
#if defined(CONFIG_RFC2217_ENABLED) || defined(DOXYGEN)
    STATE_SB_RFC2217,
#endif
}
tTelnetState;

//*****************************************************************************
//
//! The possible states of the telnet COM-PORT option parser.
//
//*****************************************************************************
#if defined(CONFIG_RFC2217_ENABLED) || defined(DOXYGEN)
typedef enum
{
    //
    //! The telnet COM-PORT option parser is ready to process the first
    //! byte of data, which is the sub-option to be processed.
    //
    STATE_2217_GET_COMMAND,

    //
    //! The telnet COM-PORT option parser is processing data bytes for the
    //! specified command/sub-option.
    //
    STATE_2217_GET_DATA,

    //
    //! The telnet COM-PORT option parser has received an IAC in the data
    //! stream.
    //
    STATE_2217_GET_DATA_IAC,

}
tRFC2217State;
#endif

//*****************************************************************************
//
//! This structure is used holding the state of a given telnet session.
//
//*****************************************************************************
typedef struct
{
    //
    //! This value holds the pointer to the TCP PCB associated with this
    //! connected telnet session.
    //
    struct tcp_pcb *pConnectPCB;

    //
    //! This value holds the pointer to the TCP PCB associated with this
    //! listening telnet session.
    //
    struct tcp_pcb *pListenPCB;

    //
    //! The current state of the TCP session.
    //
    tTCPState eTCPState;

    //
    //! The current state of the telnet option parser.
    //
    tTelnetState eTelnetState;

    //
    //! The listen port for the telnet server or the local port for the telnet
    //! client.
    //
    unsigned short usTelnetLocalPort;

    //
    //! The remote port that the telnet client connects to.
    //
    unsigned short usTelnetRemotePort;

    //
    //! The remote address that the telnet client connects to.
    //
    unsigned long ulTelnetRemoteIP;

    //
    //! Flags for various options associated with the telnet session.
    //
    unsigned char ucFlags;

    //
    //! A counter for the TCP connection timeout.
    //
    unsigned long ulConnectionTimeout;

    //
    //! The max time for TCP connection timeout counter.
    //
    unsigned long ulMaxTimeout;

    //
    //! This value holds the UART Port Number for this telnet session.
    //
    unsigned long ulSerialPort;

    //
    //! This value holds an array of pbufs.
    //
    struct pbuf *pBufQ[PBUF_POOL_SIZE];

    //
    //! This value holds the read index for the pbuf queue.
    //
    int iBufQRead;

    //
    //! This value holds the write index for the pbuf queue.
    //
    int iBufQWrite;

    //
    //! This value holds the head of the pbuf that is currently being
    //! processed (that has been popped from the queue).
    //
    struct pbuf *pBufHead;

    //
    //! This value holds the actual pbuf that is being processed within the
    //! pbuf chain pointed to by the pbuf head.
    //
    struct pbuf *pBufCurrent;

    //
    //! This value holds the offset into the payload section of the current
    //! pbuf.
    //
    unsigned long ulBufIndex;

    //
    //! The amount of time passed since rx byte count has changed.
    //
    unsigned long ulLastTCPSendTime;

#if CONFIG_RFC2217_ENABLED
    //
    //! The current state of the telnet option parser.
    //
    tRFC2217State eRFC2217State;

    //
    //! The COM-PORT Command being processed.
    //
    unsigned char ucRFC2217Command;

    //
    //! The COM-PORT value received (associed with the COM-PORT Command).
    //
    unsigned long ulRFC2217Value;

    //
    //! The index into the COM-PORT value received (for multi-byte values).
    //
    unsigned char ucRFC2217Index;

    //
    //! The maximum number of bytes expected (0 means ignore data).
    //
    unsigned char ucRFC2217IndexMax;

    //
    //! The response buffer for RFC2217 commands.
    //
    unsigned char pucRFC2217Response[16];

    //
    //! The RFC 2217 flow control value.
    //
    unsigned char ucRFC2217FlowControl;

    //
    //! The modem state mask.
    //
    unsigned char ucRFC2217ModemMask;

    //
    //! The line state mask.
    //
    unsigned char ucRFC2217LineMask;

    //
    //! The reported modem state.
    //
    unsigned char ucModemState;

    //
    //! The last reported modem state.
    //
    unsigned char ucLastModemState;
#endif

    //
    //! The indication that link layer has been lost.
    //
    tBoolean bLinkLost;

    //
    //! Debug and diagnostic counters.
    //
    unsigned char ucErrorCount;
    unsigned char ucReconnectCount;
    unsigned char ucConnectCount;

    //
    //! The last error reported by lwIP while attempting to make a connection.
    //
    err_t eLastErr;
}
tTelnetSessionData;

//*****************************************************************************
//
//! The initialization sequence sent to a remote telnet client when it first
//! connects to the telnet server.
//
//*****************************************************************************
static const unsigned char g_pucTelnetInit[] =
{
    TELNET_IAC, TELNET_DO, TELNET_OPT_SUPPRESS_GA,
#if CONFIG_RFC2217_ENABLED
    TELNET_IAC, TELNET_DO, TELNET_OPT_RFC2217,
#endif
};

//*****************************************************************************
//
//! The telnet session data array, for use in the telnet handler function.
//
//*****************************************************************************
static tTelnetSessionData g_sTelnetSession[MAX_S2E_PORTS];

//*****************************************************************************
//
// External Reference to millisecond timer.
//
//*****************************************************************************
extern unsigned long g_ulSystemTimeMS;

//*****************************************************************************
//
// Forward References.
//
//*****************************************************************************
static err_t TelnetConnected(void *arg, struct tcp_pcb *pcb, err_t err);

//*****************************************************************************
//
//! Format a block of HTML containing connection diagnostic information.
//!
//! \param pcBuffer is a pointer to a buffer into which the diagnostic text
//! will be written.
//! \param iLen is the length of the buffer pointed to by \e pcBuffer.
//! \param ucPort is the port number whose diagnostics are to be written.
//! Valid values are 0 or 1.
//!
//! This function formats a block of HTML text containing diagnostic
//! information on a given port's telnet connection status into the supplied
//! buffer.
//!
//! \return None.
//
//*****************************************************************************
#if ((defined ENABLE_WEB_DIAGNOSTICS) || (defined DOXYGEN))
void TelnetWriteDiagInfo(char *pcBuffer, int iLen, unsigned char ucPort)
{
    char *pcState;

    //
    // Determine the current port state as a string.
    //
    switch(g_sTelnetSession[ucPort].eTCPState)
    {
        case STATE_TCP_IDLE:
            pcState = "IDLE";
            break;

        case STATE_TCP_LISTEN:
            pcState = "LISTEN";
            break;

        case STATE_TCP_CONNECTING:
            pcState = "CONNECTING";
            break;

        case STATE_TCP_CONNECTED:
            pcState = "CONNECTED";
            break;

        default:
            pcState = "ILLEGAL!";
            break;
    }

    usnprintf(pcBuffer, iLen,
              "<html><body><h1>Port %d Diagnostics</h1>\r\n"
              "State: %s<br>\r\n"
              "Last Send: %d<br>\r\n"
              "Link Lost: %s<br>\r\n"
              "</body></html>\r\n",
              (int)ucPort,
              pcState,
              g_sTelnetSession[ucPort].ulLastTCPSendTime,
              g_sTelnetSession[ucPort].bLinkLost ? "YES" : "NO");
}
#endif

//*****************************************************************************
//
//! Free up any queued pbufs associated with at telnet session.
//!
//! \param pState is the pointer ot the telnet session state data.
//!
//! This will free up any pbufs on the queue, and any currently active pbufs.
//!
//! \return None.
//
//*****************************************************************************
static void
TelnetFreePbufs(tTelnetSessionData *pState)
{
    SYS_ARCH_DECL_PROTECT(lev);

    //
    // This should be done in a protected/critical section.
    //
    SYS_ARCH_PROTECT(lev);

    //
    // Pop a pbuf off of the rx queue, if one is available, and we are
    // not already processing a pbuf.
    //
    if(pState->pBufHead != NULL)
    {
        pbuf_free(pState->pBufHead);
        pState->pBufHead = NULL;
        pState->pBufCurrent = NULL;
        pState->ulBufIndex = 0;
    }

    while(pState->iBufQRead != pState->iBufQWrite)
    {
        pbuf_free(pState->pBufQ[pState->iBufQRead]);
        pState->iBufQRead = ((pState->iBufQRead + 1) % PBUF_POOL_SIZE);
    }

    //
    // Restore previous level of protection.
    //
    SYS_ARCH_UNPROTECT(lev);
}

//*****************************************************************************
//
//! Processes a telnet character in IAC SB COM-PORT-OPT mode (RFC2217).
//!
//! \param pState is the telnet state data for this connection.
//!
//! This function will handle the RFC 2217 options.
//!
//! The response (if any) is written into the telnet transmit buffer.
//!
//! \return None.
//
//*****************************************************************************
#if defined(CONFIG_RFC2217_ENABLED) || defined(DOXYGEN)
static void
TelnetProcessRFC2217Command(tTelnetSessionData *pState)
{
    int iIndex = 0;
    unsigned long ulTemp;

    //
    // Set the com port option based on the command sent.
    //
    switch(pState->ucRFC2217Command)
    {
        //
        // Set serial port baud rate.
        //
        case TELNET_C2S_SET_BAUDRATE:
        {
            if(pState->ulRFC2217Value && pState->ucRFC2217Index)
            {
                SerialSetBaudRate(pState->ulSerialPort,
                                  pState->ulRFC2217Value);
            }
            break;
        }

        //
        // Set serial port data size.
        //
        case TELNET_C2S_SET_DATASIZE:
        {
            if(pState->ulRFC2217Value && pState->ucRFC2217Index)
            {
                SerialSetDataSize(pState->ulSerialPort,
                                 (unsigned char)pState->ulRFC2217Value);
            }
            break;
        }

        //
        // Set serial port parity.
        //
        case TELNET_C2S_SET_PARITY:
        {
            if(pState->ulRFC2217Value && pState->ucRFC2217Index)
            {
                SerialSetParity(pState->ulSerialPort,
                               (unsigned char)pState->ulRFC2217Value);
            }
            break;
        }

        //
        // Set serial port stop bits.
        //
        case TELNET_C2S_SET_STOPSIZE:
        {
            if(pState->ulRFC2217Value && pState->ucRFC2217Index)
            {
                SerialSetStopBits(pState->ulSerialPort,
                                 (unsigned char)pState->ulRFC2217Value);
            }
            break;
        }

        //
        // Set serial port flow control.
        //
        case TELNET_C2S_SET_CONTROL:
        {
            switch(pState->ulRFC2217Value)
            {
                case 1:
                case 3:
                {
                    SerialSetFlowControl(pState->ulSerialPort,
                            (unsigned char)pState->ulRFC2217Value);
                    break;
                }

                case 11:
                case 12:
                {
                    SerialSetFlowOut(pState->ulSerialPort,
                            (unsigned char)pState->ulRFC2217Value);
                    break;
                }

                default:
                {
                    break;
                }
            }
            break;
        }

        //
        // Set flow control suspend/resume option.
        //
        case TELNET_C2S_FLOWCONTROL_SUSPEND:
        case TELNET_C2S_FLOWCONTROL_RESUME:
        {
            pState->ucRFC2217FlowControl =
                (unsigned char)pState->ucRFC2217Command;
            break;
        }

        //
        // Set the line state mask.
        //
        case TELNET_C2S_SET_LINESTATE_MASK:
        {
            pState->ucRFC2217LineMask = (unsigned char)pState->ulRFC2217Value;
            break;
        }

        //
        // Set the modem state mask.
        //
        case TELNET_C2S_SET_MODEMSTATE_MASK:
        {
            pState->ucRFC2217ModemMask = (unsigned char)pState->ulRFC2217Value;
            break;
        }

        //
        // Purge the serial port data.
        //
        case TELNET_C2S_PURGE_DATA:
        {
            SerialPurgeData(pState->ulSerialPort,
                            (unsigned char)pState->ulRFC2217Value);
            break;
        }
    }

    //
    // Now, send an acknowledgement response with the current setting.
    //
    pState->pucRFC2217Response[iIndex++] = TELNET_IAC;
    pState->pucRFC2217Response[iIndex++] = TELNET_SB;
    pState->pucRFC2217Response[iIndex++] = TELNET_OPT_RFC2217;

    //
    // Use the "Server to Client" response code.
    //
    pState->pucRFC2217Response[iIndex++] = (pState->ucRFC2217Command + 100);

    //
    // Read the appropriate value from the serial port module.
    //
    switch(pState->ucRFC2217Command)
    {
        case TELNET_C2S_SET_BAUDRATE:
        {
            ulTemp = SerialGetBaudRate(pState->ulSerialPort);
            break;
        }

        case TELNET_C2S_SET_DATASIZE:
        {
            ulTemp = SerialGetDataSize(pState->ulSerialPort);
            break;
        }

        case TELNET_C2S_SET_PARITY:
        {
            ulTemp = SerialGetParity(pState->ulSerialPort);
            break;
        }

        case TELNET_C2S_SET_STOPSIZE:
        {
            ulTemp = SerialGetStopBits(pState->ulSerialPort);
            break;
        }

        case TELNET_C2S_SET_CONTROL:
        {
            switch(pState->ulRFC2217Value)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                {
                    ulTemp = SerialGetFlowControl(pState->ulSerialPort);
                    break;
                }

                case 10:
                case 11:
                case 12:
                {
                    ulTemp = SerialGetFlowOut(pState->ulSerialPort);
                    break;
                }

                default:
                {
                    ulTemp = 0;
                    break;
                }
            }
            break;
        }

        case TELNET_C2S_FLOWCONTROL_SUSPEND:
        case TELNET_C2S_FLOWCONTROL_RESUME:
        case TELNET_C2S_SET_LINESTATE_MASK:
        case TELNET_C2S_SET_MODEMSTATE_MASK:
        case TELNET_C2S_PURGE_DATA:
        {
            ulTemp = pState->ulRFC2217Value;
            break;
        }

        default:
        {
            ulTemp = 0;
            break;
        }
    }

    //
    // Now, set the response value into the output buffer.
    //
    if(pState->ucRFC2217Command == TELNET_C2S_SET_BAUDRATE)
    {
        //
        // 4-byte response value.
        //
        pState->pucRFC2217Response[iIndex++] = ((ulTemp >> 24) & 0xFF);
        if(((ulTemp >> 24) & 0xFF) == TELNET_IAC)
        {
            pState->pucRFC2217Response[iIndex++] = TELNET_IAC;
        }
        pState->pucRFC2217Response[iIndex++] = ((ulTemp >> 16) & 0xFF);
        if(((ulTemp >> 16) & 0xFF) == TELNET_IAC)
        {
            pState->pucRFC2217Response[iIndex++] = TELNET_IAC;
        }
        pState->pucRFC2217Response[iIndex++] = ((ulTemp >> 8) & 0xFF);
        if(((ulTemp >> 8) & 0xFF) == TELNET_IAC)
        {
            pState->pucRFC2217Response[iIndex++] = TELNET_IAC;
        }
        pState->pucRFC2217Response[iIndex++] = ((ulTemp >> 0) & 0xFF);
        if(((ulTemp >> 0) & 0xFF) == TELNET_IAC)
        {
            pState->pucRFC2217Response[iIndex++] = TELNET_IAC;
        }
    }
    else
    {
        //
        // 1-byte resposne value.
        //
        pState->pucRFC2217Response[iIndex++] = ((ulTemp >> 0) & 0xFF);
        if(((ulTemp >> 0) & 0xFF) == TELNET_IAC)
        {
            pState->pucRFC2217Response[iIndex++] = TELNET_IAC;
        }
    }

    //
    // Finish out the packet.
    //
    pState->pucRFC2217Response[iIndex++] = TELNET_IAC;
    pState->pucRFC2217Response[iIndex++] = TELNET_SE;

    //
    // Write the packet to the TCP output buffer.
    //
    tcp_write(pState->pConnectPCB, pState->pucRFC2217Response, iIndex, 1);
}
#endif

//*****************************************************************************
//
//! Processes a telnet character in IAC SB COM-PORT-OPT mode (RFC2217).
//!
//! \param ucChar is the telnet option in question.
//! \param pState is the telnet state data for this connection.
//!
//! This function will handle the RFC 2217 options.
//!
//! The response (if any) is written into the telnet transmit buffer.
//!
//! \return None.
//
//*****************************************************************************
#if defined(CONFIG_RFC2217_ENABLED) || defined(DOXYGEN)
static void
TelnetProcessRFC2217Character(unsigned char ucChar, tTelnetSessionData *pState)
{
    unsigned char *pucValue;

    //
    // Determine the current state of the telnet COM-PORT option parser.
    //
    switch(pState->eRFC2217State)
    {
        //
        // The initial state of the parser.  The IAC SB COM-PORT-OPTION data
        // sequence has been received.  The next character (this one) is the
        // specific option/command that is to be negotiated.
        //
        case STATE_2217_GET_COMMAND:
        {
            //
            // Save the command option.
            //
            pState->ucRFC2217Command = ucChar;

            //
            // Initialize the data value for this command.
            //
            pState->ulRFC2217Value = 0;
            pState->ucRFC2217Index = 0;

            //
            // Set the expected number of data bytes based on the
            // command type.
            //
            switch(ucChar)
            {
                case TELNET_C2S_SIGNATURE:
                case TELNET_C2S_FLOWCONTROL_SUSPEND:
                case TELNET_C2S_FLOWCONTROL_RESUME:
                {
                    //
                    // No data is expected, and will be ignored.
                    //
                    pState->ucRFC2217IndexMax = 0;

                    //
                    // This option has been handled.
                    //
                    break;
                }

                case TELNET_C2S_SET_BAUDRATE:
                {
                    //
                    // For baud rate command, we expect 4 bytes of data.
                    //
                    pState->ucRFC2217IndexMax = 4;

                    //
                    // This option has been handled.
                    //
                    break;
                }

                default:
                {
                    //
                    // For other commands, we expect 1 byte of data.
                    //
                    pState->ucRFC2217IndexMax = 1;

                    //
                    // This option has been handled.
                    //
                    break;
                }
            }

            //
            // Prepare to get command data.
            //
            pState->eRFC2217State = STATE_2217_GET_DATA;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // After getting the command, we need to process the data.
        //
        case STATE_2217_GET_DATA:
        {
            //
            // Check if this is the IAC byte.
            //
            if(ucChar == TELNET_IAC)
            {
                //
                //
                pState->eRFC2217State = STATE_2217_GET_DATA_IAC;

                //
                // This state has been handled.
                //
                break;
            }

            //
            // If we are not expecting any data, ignore this byte.
            //
            if(pState->ucRFC2217IndexMax == 0)
            {
                //
                // This state has been handled.
                //
                break;
            }

            //
            // Setup the pointer to the COM-PORT value.
            //
            pucValue = (unsigned char *)&pState->ulRFC2217Value;

            //
            // Save the data, if we still need it.
            // Note: Data arrives in "network" order, but must be stored
            // in "host" order, so we swap byte order as we store it if it
            // is a 4-octet value (e.g. baud rate)
            //
            if(pState->ucRFC2217Index < pState->ucRFC2217IndexMax)
            {
                if(pState->ucRFC2217IndexMax == 4)
                {
                    pucValue[3 - pState->ucRFC2217Index++] = ucChar;
                }
                else
                {
                    pucValue[pState->ucRFC2217Index++] = ucChar;
                }
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // After getting the command, we need to process the data.
        //
        case STATE_2217_GET_DATA_IAC:
        {
            //
            // Check if this is the SE byte to end the SB string.
            //
            if(ucChar == TELNET_SE)
            {
                //
                // Process the RFC2217 command.
                //
                TelnetProcessRFC2217Command(pState);

                //
                // Restore telnet state to normal processing.
                //
                pState->eTelnetState = STATE_NORMAL;

                //
                // This state has been handled.
                //
                break;
            }

            //
            // Make sure we go back to regular data processing for
            // the next byte.
            //
            pState->eRFC2217State = STATE_2217_GET_DATA;

            //
            // If we are not expecting any data, ignore this byte.
            //
            if(pState->ucRFC2217IndexMax == 0)
            {
                //
                // This state has been handled.
                //
                break;
            }

            //
            // Setup the pointer to the COM-PORT value.
            //
            pucValue = (unsigned char *)&pState->ulRFC2217Value;

            //
            // Save the data, if we still need it.
            // Note: Data arrives in "network" order, but must be stored
            // in "host" order, so we swap byte order as we store it if it
            // is a 4-octet value (e.g. baud rate)
            //
            if(pState->ucRFC2217Index < pState->ucRFC2217IndexMax)
            {
                if(pState->ucRFC2217IndexMax == 4)
                {
                    pucValue[3 - pState->ucRFC2217Index++] = ucChar;
                }
                else
                {
                    pucValue[pState->ucRFC2217Index++] = ucChar;
                }
            }

            //
            // This state has been handled.
            //
            break;
        }
    }
}
#endif

//*****************************************************************************
//
//! Processes a telnet WILL request.
//!
//! \param ucOption is the telnet option in question.
//! \param pState is the telnet state data for this connection.
//!
//! This function will handle a WILL request for a telnet option.  If it is an
//! option that is known by the telnet server, a DO response will be generated
//! if the option is not already enabled.  For unknown options, a DONT response
//! will always be generated.
//!
//! The response (if any) is written into the telnet transmit buffer.
//!
//! \return None.
//
//*****************************************************************************
static void
TelnetProcessWill(unsigned char ucOption, tTelnetSessionData *pState)
{
    unsigned char pucBuf[3];

    //
    // Check for supported options.
    //
    switch(ucOption)
    {
        //
        // Suppress Go ahead.
        //
        case TELNET_OPT_SUPPRESS_GA:
        {
            //
            // See if the WILL flag for this option is not yet set.
            //
            if(HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_SUPPRESS_GA) == 0)
            {
                //
                // Set the WILL flag for this option.
                //
                HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_SUPPRESS_GA) = 1;

                //
                // Send a DO response to this option.
                //
                pucBuf[0] = TELNET_IAC;
                pucBuf[1] = TELNET_DO;
                pucBuf[2] = ucOption;
                tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            }
            break;
        }

        //
        // RFC 2217 Com Port Control.
        //
#if CONFIG_RFC2217_ENABLED
        case TELNET_OPT_RFC2217:
        {
            //
            // See if the WILL flag for this option is not yet set.
            //
            if(HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_RFC2217) == 0)
            {
                //
                // Set the WILL flag for this option.
                //
                HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_RFC2217) = 1;

                //
                // Send a DO response to this option.
                //
                pucBuf[0] = TELNET_IAC;
                pucBuf[1] = TELNET_DO;
                pucBuf[2] = ucOption;
                tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            }
            break;
        }
#endif

        //
        // This option is not recognized, so send a DONT response.
        //
        default:
        {
            pucBuf[0] = TELNET_IAC;
            if(ucOption == TELNET_OPT_BINARY)
            {
                pucBuf[1] = TELNET_DO;
            }
            else
            {
                pucBuf[1] = TELNET_DONT;
            }
            pucBuf[2] = ucOption;
            tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            break;
        }
    }
}

//*****************************************************************************
//
//! Processes a telnet WONT request.
//!
//! \param ucOption is the telnet option in question.
//! \param pState is the telnet state data for this connection.
//!
//! This function will handle a WONT request for a telnet option.  If it is an
//! option that is known by the telnet server, a DONT response will be
//! generated if the option is not already disabled.  For unknown options, a
//! DONT response will always be generated.
//!
//! The response (if any) is written into the telnet transmit buffer.
//!
//! \return None.
//
//*****************************************************************************
static void
TelnetProcessWont(unsigned char ucOption, tTelnetSessionData *pState)
{
    unsigned char pucBuf[3];

    //
    // Check for supported options.
    //
    switch(ucOption)
    {
        //
        // Suppress Go ahead.
        //
        case TELNET_OPT_SUPPRESS_GA:
        {
            //
            // See if the WILL flag for this option is not yet cleared.
            //
            if(HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_SUPPRESS_GA) == 1)
            {
                //
                // Clear the WILL flag for this option.
                //
                HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_SUPPRESS_GA) = 0;

                //
                // Send a DONT response to this option.
                //
                pucBuf[0] = TELNET_IAC;
                pucBuf[1] = TELNET_DONT;
                pucBuf[2] = ucOption;
                tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            }
            break;
        }

        //
        // RFC 2217 Com Port Control.
        //
#if CONFIG_RFC2217_ENABLED
        case TELNET_OPT_RFC2217:
        {
            //
            // See if the WILL flag for this option is not yet cleared.
            //
            if(HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_RFC2217) == 1)
            {
                //
                // Clear the WILL flag for this option.
                //
                HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_RFC2217) = 0;

                //
                // Send a DONT response to this option.
                //
                pucBuf[0] = TELNET_IAC;
                pucBuf[1] = TELNET_DONT;
                pucBuf[2] = ucOption;
                tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            }
            break;
        }
#endif

        //
        // This option is not recognized, so send a DONT response.
        //
        default:
        {
            pucBuf[0] = TELNET_IAC;
            pucBuf[1] = TELNET_DONT;
            pucBuf[2] = ucOption;
            tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            break;
        }
    }
}

//*****************************************************************************
//
//! Processes a telnet DO request.
//!
//! \param ucOption is the telnet option in question.
//! \param pState is the telnet state data for this connection.
//!
//! This function will handle a DO request for a telnet optoin.  If it is an
//! option that is known by the telnet server, a WILL response will be
//! generated if the option is not already enabled.  For unknown options, a
//! WONT response will always be generated.
//!
//! The response (if any) is written into the telnet transmit buffer.
//!
//! \return None.
//
//*****************************************************************************
static void
TelnetProcessDo(unsigned char ucOption, tTelnetSessionData *pState)
{
    unsigned char pucBuf[3];

    //
    // Check for supported options.
    //
    switch(ucOption)
    {
        //
        // Suppress Go ahead.
        //
        case TELNET_OPT_SUPPRESS_GA:
        {
            //
            // See if the DO flag for this option is not yet set.
            //
            if(HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_SUPPRESS_GA) == 0)
            {
                //
                // Set the DO flag for this option.
                //
                HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_SUPPRESS_GA) = 1;

                //
                // Send a WILL response to this option.
                //
                pucBuf[0] = TELNET_IAC;
                pucBuf[1] = TELNET_WILL;
                pucBuf[2] = ucOption;
                tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            }
            break;
        }

        //
        // RFC 2217 Com Port Control.
        //
#if CONFIG_RFC2217_ENABLED
        case TELNET_OPT_RFC2217:
        {
            //
            // See if the DO flag for this option is not yet set.
            //
            if(HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_RFC2217) == 0)
            {
                //
                // Set the DO flag for this option.
                //
                HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_RFC2217) = 1;

                //
                // Send a WILL response to this option.
                //
                pucBuf[0] = TELNET_IAC;
                pucBuf[1] = TELNET_WILL;
                pucBuf[2] = ucOption;
                tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            }
            break;
        }
#endif

        //
        // This option is not recognized, so send a WONT response.
        //
        default:
        {
            pucBuf[0] = TELNET_IAC;
            if(ucOption == TELNET_OPT_BINARY)
            {
                pucBuf[1] = TELNET_WILL;
            }
            else
            {
                pucBuf[1] = TELNET_WONT;
            }
            pucBuf[2] = ucOption;
            tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            break;
        }
    }
}

//*****************************************************************************
//
//! Processes a telnet DONT request.
//!
//! \param ucOption is the telnet option in question.
//! \param pState is the telnet state data for this connection.
//!
//! This funciton will handle a DONT request for a telnet option.  If it is an
//! option that is known by the telnet server, a WONT response will be
//! generated if the option is not already disabled.  For unknown options, a
//! WONT resopnse will always be generated.
//!
//! The response (if any) is written into the telnet transmit buffer.
//!
//! \return None.
//
//*****************************************************************************
static void
TelnetProcessDont(unsigned char ucOption, tTelnetSessionData *pState)
{
    unsigned char pucBuf[3];

    //
    // Check for supported options.
    //
    switch(ucOption)
    {
        //
        // Suppress Go ahead.
        //
        case TELNET_OPT_SUPPRESS_GA:
        {
            //
            // See if the DO flag for this option is not yet cleared.
            //
            if(HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_SUPPRESS_GA) == 1)
            {
                //
                // Clear the DO flag for this option.
                //
                HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_SUPPRESS_GA) = 0;

                //
                // Send a WONT response to this option.
                //
                pucBuf[0] = TELNET_IAC;
                pucBuf[1] = TELNET_WONT;
                pucBuf[2] = ucOption;
                tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            }
            break;
        }

        //
        // RFC 2217 Com Port Control.
        //
#if CONFIG_RFC2217_ENABLED
        case TELNET_OPT_RFC2217:
        {
            //
            // See if the DO flag for this option is not yet cleared.
            //
            if(HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_RFC2217) == 1)
            {
                //
                // Clear the DO flag for this option.
                //
                HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_RFC2217) = 0;

                //
                // Send a WONT response to this option.
                //
                pucBuf[0] = TELNET_IAC;
                pucBuf[1] = TELNET_WONT;
                pucBuf[2] = ucOption;
                tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            }
            break;
        }
#endif

        //
        // This option is not recognized, so send a WONT response.
        //
        default:
        {
            pucBuf[0] = TELNET_IAC;
            pucBuf[1] = TELNET_WONT;
            pucBuf[2] = ucOption;
            tcp_write(pState->pConnectPCB, pucBuf, 3, 1);
            break;
        }
    }
}

//*****************************************************************************
//
//! Processes a character received from the telnet port.
//!
//! \param ucChar is the character in question.
//! \param pState is the telnet state data for this connection.
//!
//! This function processes a character received from the telnet port, handling
//! the interpretation of telnet commands (as indicated by the telnet interpret
//! as command (IAC) byte).
//!
//! \return None.
//
//*****************************************************************************
static void
TelnetProcessCharacter(unsigned char ucChar, tTelnetSessionData *pState)
{
    unsigned char pucBuf[9];

    if((g_sParameters.sPort[pState->ulSerialPort].ucFlags &
                PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_RAW)
    {
        //
        // Write this character to the UART with no telnet processing.
        //
        SerialSend(pState->ulSerialPort, ucChar);

        //
        // And return.
        //
        return;
    }

    //
    // Determine the current state of the telnet command parser.
    //
    switch(pState->eTelnetState)
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
                pState->eTelnetState = STATE_IAC;
            }
            else
            {
                //
                // Write this character to the UART.
                //
                SerialSend(pState->ulSerialPort, ucChar);
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
                    // Send 0xff to the UART.
                    //
                    SerialSend(pState->ulSerialPort, ucChar);

                    //
                    // Switch back to normal mode.
                    //
                    pState->eTelnetState = STATE_NORMAL;

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
                    pState->eTelnetState = STATE_WILL;

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
                    pState->eTelnetState = STATE_WONT;

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
                    pState->eTelnetState = STATE_DO;

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
                    pState->eTelnetState = STATE_DONT;

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
                    // that we're still alive.
                    //
                    pucBuf[0] = '\r';
                    pucBuf[1] = '\n';
                    pucBuf[2] = '[';
                    pucBuf[3] = 'Y';
                    pucBuf[4] = 'e';
                    pucBuf[5] = 's';
                    pucBuf[6] = ']';
                    pucBuf[7] = '\r';
                    pucBuf[8] = '\n';
                    tcp_write(pState->pConnectPCB, pucBuf, 9, 1);

                    //
                    // Switch back to normal mode.
                    //
                    pState->eTelnetState = STATE_NORMAL;

                    //
                    // This character has been handled.
                    //
                    break;
                }

                //
                // See if this is the SB request.
                //
#if CONFIG_RFC2217_ENABLED
                case TELNET_SB:
                {
                    //
                    // Switch to SB processing mode.
                    //
                    pState->eTelnetState = STATE_SB;

                    //
                    // This character has been handled.
                    //
                    break;
                }
#endif

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
                    pState->eTelnetState = STATE_NORMAL;

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
            TelnetProcessWill(ucChar, pState);

            //
            // Switch back to normal mode.
            //
            pState->eTelnetState = STATE_NORMAL;

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
            TelnetProcessWont(ucChar, pState);

            //
            // Switch back to normal mode.
            //
            pState->eTelnetState = STATE_NORMAL;

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
            TelnetProcessDo(ucChar, pState);

            //
            // Switch back to normal mode.
            //
            pState->eTelnetState = STATE_NORMAL;

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
            TelnetProcessDont(ucChar, pState);

            //
            // Switch back to normal mode.
            //
            pState->eTelnetState = STATE_NORMAL;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The previous character sequence was IAC SB.
        //
        case STATE_SB:
        {
            //
            // If the SB request is COM_PORT request (in other words, RFC
            // 2217).
            //
#if CONFIG_RFC2217_ENABLED
            if((ucChar == TELNET_OPT_RFC2217) &&
               (HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_RFC2217) == 1) &&
               (HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_RFC2217) == 1))
            {
                //
                // Initialize the COM PORT option state machine.
                //
                pState->eRFC2217State = STATE_2217_GET_COMMAND;

                //
                // Change state to COM PORT option processing state.
                //
                pState->eTelnetState = STATE_SB_RFC2217;
            }
            else
#endif
            {
                //
                // Ignore this SB option.
                //
                pState->eTelnetState = STATE_SB_IGNORE;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In the middle of an unsupported IAC SB sequence.
        //
        case STATE_SB_IGNORE:
        {
            //
            // Check for the IAC character.
            //
            if(ucChar == TELNET_IAC)
            {
                //
                // Change state to look for Telnet SE character.
                //
                pState->eTelnetState = STATE_SB_IGNORE_IAC;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // In the middle of a an RFC 2217 sequence.
        //
#if CONFIG_RFC2217_ENABLED
        case STATE_SB_RFC2217:
        {
            //
            // Allow the 2217 processor to handle this character.
            //
            TelnetProcessRFC2217Character(ucChar, pState);

            //
            // This state has been handled.
            //
            break;
        }
#endif

        //
        // Checking for the terminating IAC SE in unsupported IAC SB sequence.
        //
        case STATE_SB_IGNORE_IAC:
        {
            //
            // Check for the IAC character.
            //
            if(ucChar == TELNET_SE)
            {
                //
                // IAC SB sequence is terminated.  Revert to normal telnet
                // character processing.
                //
                pState->eTelnetState = STATE_NORMAL;
            }
            else
            {
                //
                // Go back to looking for the IAC SE sequence.
                //
                pState->eTelnetState = STATE_SB_IGNORE;
            }

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
            pState->eTelnetState = STATE_NORMAL;

            //
            // This state has been handled.
            //
            break;
        }
    }
}

//*****************************************************************************
//
//! Receives a TCP packet from lwIP for the telnet server.
//!
//! \param arg is the telnet state data for this connection.
//! \param pcb is the pointer to the TCP control structure.
//! \param p is the pointer to the pbuf structure containing the packet data.
//! \param err is used to indicate if any errors are associated with the
//! incoming packet.
//!
//! This function is called when the lwIP TCP/IP stack has an incoming packet
//! to be processed.
//!
//! \return This function will return an lwIP defined error code.
//
//*****************************************************************************
static err_t
TelnetReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    tTelnetSessionData *pState = arg;
    int iNextWrite;
    SYS_ARCH_DECL_PROTECT(lev);

    DEBUG_MSG("TelnetReceive 0x%08x, 0x%08x, 0x%08x, %d\n", arg, pcb, p, err);

    //
    // Place the incoming packet onto the queue if there is space.
    //
    if((err == ERR_OK) && (p != NULL))
    {
        //
        // This should be done in a protected/critical section.
        //
        SYS_ARCH_PROTECT(lev);

        //
        // Do we have space in the queue?
        //
        iNextWrite = ((pState->iBufQWrite + 1) % PBUF_POOL_SIZE);
        if(iNextWrite == pState->iBufQRead)
        {
            //
            // The queue is full - discard the pbuf and return since we can't
            // handle it just now.
            //

            //
            // Restore previous level of protection.
            //
            SYS_ARCH_UNPROTECT(lev);

            //
            // Free up the pbuf.  Note that we don't acknowledge receipt of
            // the data since we want it to be retransmitted later.
            //
            pbuf_free(p);
        }
        else
        {
            //
            // Place the pbuf in the circular queue.
            //
            pState->pBufQ[pState->iBufQWrite] = p;

            //
            // Increment the queue write index.
            //
            pState->iBufQWrite = iNextWrite;

            //
            // Restore previous level of protection.
            //
            SYS_ARCH_UNPROTECT(lev);
        }
    }

    //
    // If a null packet is passed in, close the connection.
    //
    else if((err == ERR_OK) && (p == NULL))
    {
        //
        // Clear out all of the TCP callbacks.
        //
        tcp_arg(pcb, NULL);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);
        tcp_poll(pcb, NULL, 1);

        //
        // Close the TCP connection.
        //
        tcp_close(pcb);

        //
        // Clear out any pbufs associated with this session.
        //
        TelnetFreePbufs(pState);

        //
        // Clear out the telnet session PCB.
        //
        pState->pConnectPCB = NULL;

        //
        // If we don't have a listen PCB, then we are in client mode, and
        // should try to reconnect.
        //
        if(pState->pListenPCB == NULL)
        {
            //
            // Re-open the connection.
            //
            TelnetOpen(pState->ulTelnetRemoteIP, pState->usTelnetRemotePort,
                       pState->usTelnetLocalPort, pState->ulSerialPort);
        }
        else
        {
            //
            // Revert to listening state.
            //
            pState->eTCPState = STATE_TCP_LISTEN;
        }
    }

    //
    // Return okay.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
//! Handles lwIP TCP/IP errors.
//!
//! \param arg is the telnet state data for this connection.
//! \param err is the error that was detected.
//!
//! This function is called when the lwIP TCP/IP stack has detected an error.
//! The connection is no longer valid.
//!
//! \return None.
//
//*****************************************************************************
static void
TelnetError(void *arg, err_t err)
{
    tTelnetSessionData *pState = arg;

    DEBUG_MSG("TelnetError 0x%08x, %d\n", arg, err);

    //
    // Increment our error counter.
    //
    pState->ucErrorCount++;
    pState->eLastErr = err;

    //
    // Free the pbufs associated with this session.
    //
    TelnetFreePbufs(pState);

    //
    // Reset the session data for this port.
    //
    if(pState->pListenPCB == NULL)
    {
        //
        // Attempt to reestablish the telnet connection to the server.
        //
        TelnetOpen(pState->ulTelnetRemoteIP, pState->usTelnetRemotePort,
                   pState->usTelnetLocalPort, pState->ulSerialPort);
    }
    else
    {
        //
        // Reinitialize the server state to wait for incoming connections.
        //
        pState->pConnectPCB = NULL;
        pState->eTCPState = STATE_TCP_LISTEN;
        pState->eTelnetState = STATE_NORMAL;
        pState->ucFlags = ((1 << OPT_FLAG_WILL_SUPPRESS_GA) |
                           (1 << OPT_FLAG_SERVER));
        pState->ulConnectionTimeout = 0;
        pState->iBufQRead = 0;
        pState->iBufQWrite = 0;
        pState->pBufHead = NULL;
        pState->pBufCurrent = NULL;
        pState->ulBufIndex = 0;
        pState->ulLastTCPSendTime = 0;
#if CONFIG_RFC2217_ENABLED
        pState->ucFlags |= (1 << OPT_FLAG_WILL_RFC2217);
        pState->ucRFC2217FlowControl =
            TELNET_C2S_FLOWCONTROL_RESUME;
        pState->ucRFC2217ModemMask = 0;
        pState->ucRFC2217LineMask = 0xff;
        pState->ucLastModemState = 0;
        pState->ucModemState = 0;
#endif
        pState->bLinkLost = false;
    }
}

//*****************************************************************************
//
//! Handles lwIP TCP/IP polling and timeout requests.
//!
//! \param arg is the telnet state data for this connection.
//! \param pcb is the pointer to the TCP control structure.
//!
//! This function is called periodically and is used to re-establish dropped
//! client connections and to reset idle server connections.
//!
//! \return This function will return an lwIP defined error code.
//
//*****************************************************************************
static err_t
TelnetPoll(void *arg, struct tcp_pcb *pcb)
{
    err_t eError;
    struct ip_addr sIPAddr;
    tTelnetSessionData *pState = arg;

    DEBUG_MSG("TelnetPoll 0x%08x, 0x%08x\n", arg, pcb);

    //
    // Are we operating as a server or a client?
    //
    if(!pState->pListenPCB)
    {
        //
        // We are operating as a client.  Are we currently trying to reconnect
        // to the server?
        //
        if(pState->eTCPState == STATE_TCP_CONNECTING)
        {
            //
            // We are trying to reconnect but can't have received the connection
            // callback in the last 3 seconds so we try connecting again.
            //
            pState->ucReconnectCount++;
            sIPAddr.addr = htonl(pState->ulTelnetRemoteIP);
            eError = tcp_connect(pcb, &sIPAddr, pState->usTelnetRemotePort,
                                 TelnetConnected);

            if(eError != ERR_OK)
            {
                //
                // Remember the error for later.
                //
                pState->eLastErr = eError;
            }
        }
    }
    else
    {
        //
        // We are operating as a server. Increment the timeout value and close
        // the telnet connection if the configured timeout has been exceeded.
        //
        pState->ulConnectionTimeout++;
        if((pState->ulMaxTimeout != 0) &&
           (pState->ulConnectionTimeout > pState->ulMaxTimeout))
        {
            //
            // Close the telnet connection.
            //
            tcp_abort(pcb);
        }
    }

    //
    // Return OK.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
//! Handles acknowledgment of data transmitted via Ethernet.
//!
//! \param arg is the telnet state data for this connection.
//! \param pcb is the pointer to the TCP control structure.
//! \param len is the length of the data transmitted.
//!
//! This function is called when the lwIP TCP/IP stack has received an
//! acknowledgment for data that has been transmitted.
//!
//! \return This function will return an lwIP defined error code.
//
//*****************************************************************************
static err_t
TelnetSent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
    tTelnetSessionData *pState = arg;

    DEBUG_MSG("TelnetSent 0x%08x, 0x%08x, %d\n", arg, pcb, len);

    //
    // Reset the connection timeout.
    //
    pState->ulConnectionTimeout = 0;

    //
    // Return OK.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
//! Finalizes the TCP connection in client mode.
//!
//! \param arg is the telnet state data for this connection.
//! \param pcb is the pointer to the TCP control structure.
//! \param err is not used in this implementation.
//!
//! This function is called when the lwIP TCP/IP stack has completed a TCP
//! connection.
//!
//! \return This function will return an lwIP defined error code.
//
//*****************************************************************************
static err_t
TelnetConnected(void *arg, struct tcp_pcb *pcb, err_t err)
{
    tTelnetSessionData *pState = arg;

    DEBUG_MSG("TelnetConnected 0x%08x, 0x%08x, %d\n", arg, pcb, err);

    //
    // Increment our connection counter.
    //
    pState->ucConnectCount++;

    //
    // If we are not in the listening state, refuse this connection.
    //
    if(pState->eTCPState != STATE_TCP_CONNECTING)
    {
        //
        // If we already have a connection, kill it and start over.
        //
        return(ERR_CONN);
    }

    if(err != ERR_OK)
    {
        //
        // Remember the error that is being reported.
        //
        pState->eLastErr = err;

        //
        // Clear out all of the TCP callbacks.
        //
        tcp_arg(pcb, NULL);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);
        tcp_poll(pcb, NULL, 1);

        //
        // Close the TCP connection.
        //
        tcp_close(pcb);

        //
        // Clear out any pbufs associated with this session.
        //
        TelnetFreePbufs(pState);

        //
        // Re-open the connection.
        //
        TelnetOpen(pState->ulTelnetRemoteIP, pState->usTelnetRemotePort,
                   pState->usTelnetLocalPort, pState->ulSerialPort);

        //
        // And return.
        //
        return(ERR_OK);
    }

    //
    // Save the PCB for future reference.
    //
    pState->pConnectPCB = pcb;

    //
    // Change TCP state to connected.
    //
    pState->eTCPState = STATE_TCP_CONNECTED;

    //
    // Reset the serial port associated with this session to its default
    // parameters.
    //
    SerialSetDefault(pState->ulSerialPort);

    //
    // Set the connection timeout to 0.
    //
    pState->ulConnectionTimeout = 0;

    //
    // Setup the TCP connection priority.
    //
    tcp_setprio(pcb, TCP_PRIO_MIN);

    //
    // Setup the TCP receive function.
    //
    tcp_recv(pcb, TelnetReceive);

    //
    // Setup the TCP error function.
    //
    tcp_err(pcb, TelnetError);

    //
    // Setup the TCP polling function/interval.
    //
    tcp_poll(pcb, TelnetPoll, (1000 / TCP_SLOW_INTERVAL));

    //
    // Setup the TCP sent callback function.
    //
    tcp_sent(pcb, TelnetSent);

    //
    // Send the telnet initialization string.
    //
    if((g_sParameters.sPort[pState->ulSerialPort].ucFlags &
                PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_TELNET)
    {
        tcp_write(pcb, g_pucTelnetInit, sizeof(g_pucTelnetInit), 1);
        tcp_output(pcb);
    }

    //
    // Return a success code.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
//! Accepts a TCP connection for the telnet port.
//!
//! \param arg is the telnet state data for this connection.
//! \param pcb is the pointer to the TCP control structure.
//! \param err is not used in this implementation.
//!
//! This function is called when the lwIP TCP/IP stack has an incoming
//! connection request on the telnet port.
//!
//! \return This function will return an lwIP defined error code.
//
//*****************************************************************************
static err_t
TelnetAccept(void *arg, struct tcp_pcb *pcb, err_t err)
{
    tTelnetSessionData *pState = arg;

    DEBUG_MSG("TelnetAccept 0x%08x, 0x%08x, 0x%08x\n", arg, pcb, err);

    //
    // If we are not in the listening state, refuse this connection.
    //
    if(pState->eTCPState != STATE_TCP_LISTEN)
    {
        //
        // If we haven't lost the link, then refuse this connection.
        //
        if(!pState->bLinkLost)
        {
            //
            // If we already have a connection, kill it and start over.
            //
            return(ERR_CONN);
        }

        //
        // Reset the flag for next time.
        //
        pState->bLinkLost = false;

        //
        // Abort the existing TCP connection.
        //
        tcp_abort(pState->pConnectPCB);

        //
        // Clear out any pbufs associated with this session.
        //
        TelnetFreePbufs(pState);

        //
        // Clear out the telnet session PCB.
        //
        pState->pConnectPCB = NULL;

    }

    //
    // Save the PCB for future reference.
    //
    pState->pConnectPCB = pcb;

    //
    // Change TCP state to connected.
    //
    pState->eTCPState = STATE_TCP_CONNECTED;

    //
    // Acknowledge that we have accepted this connection.
    //
    tcp_accepted(pcb);

    //
    // Reset the serial port associated with this session to its default
    // parameters.
    //
    SerialSetDefault(pState->ulSerialPort);

    //
    // Set the connection timeout to 0.
    //
    pState->ulConnectionTimeout = 0;

    //
    // Setup the TCP connection priority.
    //
    tcp_setprio(pcb, TCP_PRIO_MIN);

    //
    // Setup the TCP receive function.
    //
    tcp_recv(pcb, TelnetReceive);

    //
    // Setup the TCP error function.
    //
    tcp_err(pcb, TelnetError);

    //
    // Setup the TCP polling function/interval.
    //
    tcp_poll(pcb, TelnetPoll, (1000 / TCP_SLOW_INTERVAL));

    //
    // Setup the TCP sent callback function.
    //
    tcp_sent(pcb, TelnetSent);

    //
    // Send the telnet initialization string.
    //
    if((g_sParameters.sPort[pState->ulSerialPort].ucFlags &
                PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_TELNET)
    {
        tcp_write(pcb, g_pucTelnetInit, sizeof(g_pucTelnetInit), 1);
        tcp_output(pcb);
    }

    //
    // Return a success code.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
//! Closes an existing Ethernet connection.
//!
//! \param ulSerialPort is the serial port associated with this telnet session.
//!
//! This function is called when the the Telnet/TCP session associated with
//! the specified serial port is to be closed.
//!
//! \return None.
//
//*****************************************************************************
void
TelnetClose(unsigned long ulSerialPort)
{
    tTelnetSessionData *pState;

    DEBUG_MSG("TelnetClose UART %d\n", ulSerialPort);

    //
    // Check the arguments.
    //
    ASSERT(ulSerialPort < MAX_S2E_PORTS);
    pState = &g_sTelnetSession[ulSerialPort];

    //
    // If we have a connect PCB, close it down.
    //
    if(pState->pConnectPCB != NULL)
    {
        DEBUG_MSG("Closing connect pcb 0x%08x\n", pState->pConnectPCB);

        //
        // Clear out all of the TCP callbacks.
        //
        tcp_arg(pState->pConnectPCB, NULL);
        tcp_sent(pState->pConnectPCB, NULL);
        tcp_recv(pState->pConnectPCB, NULL);
        tcp_err(pState->pConnectPCB, NULL);
        tcp_poll(pState->pConnectPCB, NULL, 1);

        //
        // Abort the existing TCP connection.
        //
        tcp_abort(pState->pConnectPCB);

        //
        // Clear out any pbufs associated with this session.
        //
        TelnetFreePbufs(pState);
    }

    //
    // If we have a listen PCB, close it down as well.
    //
    if(pState->pListenPCB != NULL)
    {
        DEBUG_MSG("Closing listen pcb 0x%08x\n", pState->pListenPCB);

        //
        // Close the TCP connection.
        //
        tcp_close(pState->pListenPCB);

        //
        // Clear out any pbufs associated with this session.
        //
        TelnetFreePbufs(pState);
    }

    //
    // Reset the session data for this port.
    //
    pState->pConnectPCB = NULL;
    pState->pListenPCB = NULL;
    pState->eTCPState = STATE_TCP_IDLE;
    pState->eTelnetState = STATE_NORMAL;
    pState->ucFlags = 0;
    pState->ulConnectionTimeout = 0;
    pState->ulMaxTimeout = 0;
    pState->ulSerialPort = MAX_S2E_PORTS;
    pState->iBufQRead = 0;
    pState->iBufQWrite = 0;
    pState->pBufHead = NULL;
    pState->pBufCurrent = NULL;
    pState->ulBufIndex = 0;
    pState->ulLastTCPSendTime = 0;
#if CONFIG_RFC2217_ENABLED
    pState->eRFC2217State = STATE_2217_GET_DATA;
    pState->ucRFC2217Command = 0;
    pState->ulRFC2217Value = 0;
    pState->ucRFC2217Index = 0;
    pState->ucRFC2217IndexMax = 0;
    pState->ucRFC2217FlowControl = 0;
    pState->ucRFC2217ModemMask = 0;
    pState->ucRFC2217LineMask = 0;
    pState->ucLastModemState = 0;
    pState->ucModemState = 0;
#endif
    pState->bLinkLost = false;
}

//*****************************************************************************
//
//! Opens a telnet server session (client).
//!
//! \param ulIPAddr is the IP address of the telnet server.
//! \param usTelnetRemotePort is port number of the telnet server.
//! \param usTelnetLocalPort is local port number to connect from.
//! \param ulSerialPort is the serial port associated with this telnet session.
//!
//! This function establishes a TCP session by attempting a connection to
//! a telnet server.
//!
//! \return None.
//
//*****************************************************************************
void
TelnetOpen(unsigned long ulIPAddr, unsigned short usTelnetRemotePort,
           unsigned short usTelnetLocalPort, unsigned long ulSerialPort)
{
    void *pcb;
    struct ip_addr sIPAddr;
    err_t eError;
    tTelnetSessionData *pState;

    DEBUG_MSG("TelnetOpen %d.%d.%d.%d port %d, UART %d\n",
              (ulIPAddr >> 24), (ulIPAddr >> 16) & 0xFF,
              (ulIPAddr >> 8) & 0xFF, ulIPAddr & 0xFF, usTelnetRemotePort,
              ulSerialPort);

    //
    // Check the arguments.
    //
    ASSERT(ulIPAddr != 0);
    ASSERT(ulSerialPort < MAX_S2E_PORTS);
    ASSERT(usTelnetRemotePort != 0);
    ASSERT(usTelnetLocalPort != 0);
    pState = &g_sTelnetSession[ulSerialPort];

    //
    // Fill in the telnet state data structure for this session in client
    // mode.
    //
    pState->pConnectPCB = NULL;
    pState->pListenPCB = NULL;
    pState->eTCPState = STATE_TCP_CONNECTING;
    pState->eTelnetState = STATE_NORMAL;
    pState->ucFlags = (1 << OPT_FLAG_WILL_SUPPRESS_GA);
    pState->ulConnectionTimeout = 0;
    pState->ulMaxTimeout = g_sParameters.sPort[ulSerialPort].ulTelnetTimeout;
    pState->ulSerialPort = ulSerialPort;
    pState->usTelnetRemotePort = usTelnetRemotePort;
    pState->usTelnetLocalPort = usTelnetLocalPort;
    pState->ulTelnetRemoteIP = ulIPAddr;
    pState->iBufQRead = 0;
    pState->iBufQWrite = 0;
    pState->pBufHead = NULL;
    pState->pBufCurrent = NULL;
    pState->ulBufIndex = 0;
    pState->ulLastTCPSendTime = 0;

#if CONFIG_RFC2217_ENABLED
    pState->ucFlags |= (1 << OPT_FLAG_WILL_RFC2217);
    pState->ucRFC2217FlowControl =
        TELNET_C2S_FLOWCONTROL_RESUME;
    pState->ucRFC2217ModemMask = 0;
    pState->ucRFC2217LineMask = 0xff;
    pState->ucLastModemState = 0;
    pState->ucModemState = 0;
#endif
    pState->bLinkLost = false;

    //
    // Make a connection to the remote telnet server.
    //
    sIPAddr.addr = htonl(ulIPAddr);
    pcb = tcp_new();

    //
    // Save the requested information and set the TCP callback functions
    // and arguments.
    //
    tcp_arg(pcb, pState);

    //
    // Set the TCP error callback.
    //
    tcp_err(pcb, TelnetError);

    //
    // Set the callback that will be made after 3 seconds.  This allows us to
    // reattempt the connection if we do not receive a response.
    //
    tcp_poll(pcb, TelnetPoll, (3000 / TCP_SLOW_INTERVAL));

    //
    // Attempt to connect to the server.
    //
    eError = tcp_connect(pcb, &sIPAddr, usTelnetRemotePort, TelnetConnected);
    if(eError != ERR_OK)
    {
        //
        // Remember the error for later.
        //
        pState->eLastErr = eError;
        return;
    }
}

//*****************************************************************************
//
//! Opens a telnet server session (listen).
//!
//! \param usTelnetPort is the telnet port number to listen on.
//! \param ulSerialPort is the serial port associated with this telnet session.
//!
//! This function establishes a TCP session in listen mode as a telnet server.
//!
//! \return None.
//
//*****************************************************************************
void
TelnetListen(unsigned short usTelnetPort, unsigned long ulSerialPort)
{
    void *pcb;
    tTelnetSessionData *pState;

    DEBUG_MSG("TelnetListen port %d, UART %d\n", usTelnetPort, ulSerialPort);

    //
    // Check the arguments.
    //
    ASSERT(ulSerialPort < MAX_S2E_PORTS);
    ASSERT(usTelnetPort != 0);
    pState = &g_sTelnetSession[ulSerialPort];

    //
    // Fill in the telnet state data structure for this session in listen
    // (in other words, server) mode.
    //
    pState->pConnectPCB = NULL;
    pState->eTCPState = STATE_TCP_LISTEN;
    pState->eTelnetState = STATE_NORMAL;
    pState->ucFlags = ((1 << OPT_FLAG_WILL_SUPPRESS_GA) |
                       (1 << OPT_FLAG_SERVER));
    pState->ulConnectionTimeout = 0;
    pState->ulMaxTimeout = g_sParameters.sPort[ulSerialPort].ulTelnetTimeout;
    pState->ulSerialPort = ulSerialPort;
    pState->usTelnetLocalPort = usTelnetPort;
    pState->usTelnetRemotePort = 0;
    pState->ulTelnetRemoteIP = 0;
    pState->iBufQRead = 0;
    pState->iBufQWrite = 0;
    pState->pBufHead = NULL;
    pState->pBufCurrent = NULL;
    pState->ulBufIndex = 0;
    pState->ulLastTCPSendTime = 0;

#if CONFIG_RFC2217_ENABLED
    pState->ucFlags |= (1 << OPT_FLAG_WILL_RFC2217);
    pState->ucRFC2217FlowControl =
        TELNET_C2S_FLOWCONTROL_RESUME;
    pState->ucRFC2217ModemMask = 0;
    pState->ucRFC2217LineMask = 0xff;
    pState->ucLastModemState = 0;
    pState->ucModemState = 0;
#endif
    pState->bLinkLost = false;

    //
    // Initialize the application to listen on the requested telnet port.
    //
    pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, usTelnetPort);
    pcb = tcp_listen(pcb);
    pState->pListenPCB = pcb;

    //
    // Save the requested information and set the TCP callback functions
    // and arguments.
    //
    tcp_arg(pcb, pState);
    tcp_accept(pcb, TelnetAccept);
}

//*****************************************************************************
//
//! Gets the current local port for a connection's telnet session.
//!
//! \param ulSerialPort is the serial port associated with this telnet session.
//!
//! This function returns the local port in use by the telnet session
//! associated with the given serial port.  If operating as a telnet server,
//! this port is the port that is listening for an incoming connection.  If
//! operating as a telnet client, this is the local port used to connect to
//! the remote server.
//!
//! \return None.
//
//*****************************************************************************
unsigned short
TelnetGetLocalPort(unsigned long ulSerialPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulSerialPort < MAX_S2E_PORTS);

    return(g_sTelnetSession[ulSerialPort].usTelnetLocalPort);
}

//*****************************************************************************
//
//! Gets the current remote port for a connection's telnet session.
//!
//! \param ulSerialPort is the serial port associated with this telnet session.
//!
//! This function returns the remote port in use by the telnet session
//! associated with the given serial port.  If operating as a telnet server,
//! this function will return 0.  If operating as a telnet client, this is the
//! server port that the connection is using.
//!
//! \return None.
//
//*****************************************************************************
unsigned short
TelnetGetRemotePort(unsigned long ulSerialPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulSerialPort < MAX_S2E_PORTS);

    return(g_sTelnetSession[ulSerialPort].usTelnetRemotePort);
}

//*****************************************************************************
//
//! Initializes the telnet session(s) for the Serial to Ethernet Module.
//!
//! This function initializes the telnet session data parameter block.
//!
//! \return None.
//
//*****************************************************************************
void
TelnetInit(void)
{
    int iPort;

    //
    // Initialize the session data for each supported port.
    //
    for(iPort = 0; iPort < MAX_S2E_PORTS; iPort++)
    {
        g_sTelnetSession[iPort].pConnectPCB = NULL;
        g_sTelnetSession[iPort].pListenPCB = NULL;
        g_sTelnetSession[iPort].eTCPState = STATE_TCP_IDLE;
        g_sTelnetSession[iPort].eTelnetState = STATE_NORMAL;
        g_sTelnetSession[iPort].ucFlags = 0;
        g_sTelnetSession[iPort].ulConnectionTimeout = 0;
        g_sTelnetSession[iPort].ulMaxTimeout = 0;
        g_sTelnetSession[iPort].ulSerialPort = MAX_S2E_PORTS;
        g_sTelnetSession[iPort].usTelnetRemotePort = 0;
        g_sTelnetSession[iPort].usTelnetLocalPort = 0;
        g_sTelnetSession[iPort].ulTelnetRemoteIP = 0;
        g_sTelnetSession[iPort].iBufQRead = 0;
        g_sTelnetSession[iPort].iBufQWrite = 0;
        g_sTelnetSession[iPort].pBufHead = NULL;
        g_sTelnetSession[iPort].pBufCurrent = NULL;
        g_sTelnetSession[iPort].ulBufIndex = 0;
        g_sTelnetSession[iPort].ulLastTCPSendTime = 0;
#if CONFIG_RFC2217_ENABLED
        g_sTelnetSession[iPort].eRFC2217State = STATE_2217_GET_DATA;
        g_sTelnetSession[iPort].ucRFC2217Command = 0;
        g_sTelnetSession[iPort].ulRFC2217Value = 0;
        g_sTelnetSession[iPort].ucRFC2217Index = 0;
        g_sTelnetSession[iPort].ucRFC2217IndexMax = 0;
        g_sTelnetSession[iPort].ucRFC2217FlowControl = 0;
        g_sTelnetSession[iPort].ucRFC2217ModemMask = 0;
        g_sTelnetSession[iPort].ucRFC2217LineMask = 0;
        g_sTelnetSession[iPort].ucLastModemState = 0;
        g_sTelnetSession[iPort].ucModemState = 0;
#endif
        g_sTelnetSession[iPort].bLinkLost = false;
        g_sTelnetSession[iPort].ucConnectCount = 0;
        g_sTelnetSession[iPort].ucReconnectCount = 0;
        g_sTelnetSession[iPort].ucErrorCount = 0;
        g_sTelnetSession[iPort].eLastErr = ERR_OK;
    }
}

//*****************************************************************************
//
//! Handles periodic task for telnet sessions.
//!
//! This function is called periodically from the lwIP timer thread context.
//! This function will handle transferring data between the UART and the
//! the telnet sockets.  The time period for this should be tuned to the UART
//! ring buffer sizes to maintain optimal throughput.
//!
//! \return None.
//
//*****************************************************************************
void
TelnetHandler(void)
{
    long lCount, lIndex;
    static unsigned char pucTemp[PBUF_POOL_BUFSIZE];
    int iLoop;
    SYS_ARCH_DECL_PROTECT(lev);
    unsigned char *pucData;
    tTelnetSessionData *pState;

    //
    // Loop through the possible telnet sessions.
    //
    for(iLoop = 0; iLoop < MAX_S2E_PORTS; iLoop++)
    {
        //
        // Initialize the state pointer.
        //
        pState = &g_sTelnetSession[iLoop];

        //
        // If the telnet session is not connected, skip this port.
        //
        if(pState->eTCPState != STATE_TCP_CONNECTED)
        {
            continue;
        }

#if defined(CONFIG_RFC2217_ENABLED)
        //
        // Check to see if modem state options have changed.
        //
        if(pState->ucLastModemState != pState->ucModemState)
        {
            //
            // Save the current state for the next comparison.
            //
            pState->ucLastModemState = pState->ucModemState;

            //
            // Check to see if the modem state options have been negotiated,
            // and if they have changed.
            //
            if((HWREGBITB(&pState->ucFlags, OPT_FLAG_WILL_RFC2217) == 1) &&
               (HWREGBITB(&pState->ucFlags, OPT_FLAG_DO_RFC2217) == 1))
            {
                //
                // Check to see if the state change has been enabled.
                //
                if(pState->ucModemState & pState->ucRFC2217ModemMask)
                {
                    lIndex = 0;
                    pucTemp[lIndex++] = TELNET_IAC;
                    pucTemp[lIndex++] = TELNET_SB;
                    pucTemp[lIndex++] = TELNET_OPT_RFC2217;

                    //
                    // Use the "Server to Client" notification value.
                    //
                    pucTemp[lIndex++] = (TELNET_C2S_NOTIFY_MODEMSTATE + 100);

                    pucTemp[lIndex++] = (pState->ucModemState &
                            pState->ucRFC2217ModemMask);
                    if((pState->ucModemState & pState->ucRFC2217ModemMask) ==
                            TELNET_IAC)
                    {
                        pucTemp[lIndex++] = TELNET_IAC;
                    }
                    pucTemp[lIndex++] = TELNET_IAC;
                    pucTemp[lIndex++] = TELNET_SE;

                    //
                    // Write the data.
                    //
                    tcp_write(pState->pConnectPCB, pucTemp, lIndex, 1);
                    tcp_output(pState->pConnectPCB);

                    //
                    // Reset the index.
                    //
                    lIndex = 0;
                }
            }
        }
#endif

        //
        // While space is available in the serial output queue, process the
        // pbufs received on the telnet interface.
        //
        while(!SerialSendFull(iLoop))
        {
            //
            // Pop a pbuf off of the rx queue, if one is available, and we are
            // not already processing a pbuf.
            //
            if(pState->pBufHead == NULL)
            {
                if(pState->iBufQRead != pState->iBufQWrite)
                {
                    SYS_ARCH_PROTECT(lev);
                    pState->pBufHead = pState->pBufQ[pState->iBufQRead];
                    pState->iBufQRead =
                        ((pState->iBufQRead + 1) % PBUF_POOL_SIZE);
                    pState->pBufCurrent = pState->pBufHead;
                    pState->ulBufIndex = 0;
                    SYS_ARCH_UNPROTECT(lev);
                }
            }

            //
            // If there is no packet to be processed, break out of the loop.
            //
            if(pState->pBufHead == NULL)
            {
                break;
            }

            //
            // Setup the data pointer for the current buffer.
            //
            pucData = pState->pBufCurrent->payload;

            //
            // Process the next character in the buffer.
            //
            TelnetProcessCharacter(pucData[pState->ulBufIndex], pState);

            //
            // Increment to next data byte.
            //
            pState->ulBufIndex++;

            //
            // Check to see if we are at the end of the current buffer.  If so,
            // get the next pbuf in the chain.
            //
            if(pState->ulBufIndex >= pState->pBufCurrent->len)
            {
                pState->pBufCurrent = pState->pBufCurrent->next;
                pState->ulBufIndex = 0;
            }

            //
            // Check to see if we are at the end of the chain.  If so,
            // acknowledge this data as being consumed to open up the TCP
            // window.
            //
            if((pState->pBufCurrent == NULL) && (pState->ulBufIndex == 0))
            {
                tcp_recved(pState->pConnectPCB, pState->pBufHead->tot_len);
                pbuf_free(pState->pBufHead);
                pState->pBufHead = NULL;
                pState->pBufCurrent = NULL;
                pState->ulBufIndex = 0;
            }
        }

        //
        // Flush the TCP output buffer, in the event that data was
        // queued up by processing the incoming packet.
        //
        tcp_output(pState->pConnectPCB);

        //
        // If RFC2217 is enabled, and flow control has been set to suspended,
        // skip processing of this port.
        //
#if CONFIG_RFC2217_ENABLED
        if(pState->ucRFC2217FlowControl == TELNET_C2S_FLOWCONTROL_SUSPEND)
        {
            continue;
        }
#endif

        //
        // Process the RX ring buffer data if space is available in the
        // TCP output buffer.
        //
        if(SerialReceiveAvailable(pState->ulSerialPort) &&
           tcp_sndbuf(pState->pConnectPCB) &&
           (pState->pConnectPCB->snd_queuelen < TCP_SND_QUEUELEN))
        {
            //
            // Here, we have data, and we have space.  Set the total amount
            // of data we will process to the lesser of data available or
            // space available.
            //
            lCount = (long)SerialReceiveAvailable(pState->ulSerialPort);
            if(tcp_sndbuf(pState->pConnectPCB) < lCount)
            {
                lCount = tcp_sndbuf(pState->pConnectPCB);
            }

            //
            // While we have data remaining to process, continue in this
            // loop.
            //
            while((lCount) &&
                  (pState->pConnectPCB->snd_queuelen < TCP_SND_QUEUELEN))
            {
                //
                // First, reset the index into the local buffer.
                //
                lIndex = 0;

                //
                // Fill the local buffer with data while there is data
                // and/or space remaining.
                //
                while(lCount && (lIndex < sizeof(pucTemp)))
                {
                    pucTemp[lIndex] = SerialReceive(pState->ulSerialPort);
                    lIndex++;
                    lCount--;
                }

                //
                // Write the local buffer into the TCP buffer.
                //
                tcp_write(pState->pConnectPCB, pucTemp, lIndex, 1);
            }

            //
            // Flush the data that has been written into the TCP output
            // buffer.
            //
            tcp_output(pState->pConnectPCB);
            pState->ulLastTCPSendTime = g_ulSystemTimeMS;
        }
    }
}

//*****************************************************************************
//
//! Handles RFC2217 modem state notification.
//!
//! \param ulPort is the serial port for which the modem state changed.
//! \param ucModemState is the new modem state.
//!
//! This function should be called by the serial port code when the modem state
//! changes.  If RFC2217 is enabled, a notification message will be sent.
//!
//! \return None.
//
//*****************************************************************************
#if defined(CONFIG_RFC2217_ENABLED) || defined(DOXYGEN)
void
TelnetNotifyModemState(unsigned long ulPort, unsigned char ucModemState)
{
    tTelnetSessionData *pState;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);
    pState = &g_sTelnetSession[ulPort];

    //
    // If Telnet protocol is disabled, simply return.
    //
    if((g_sParameters.sPort[ulPort].ucFlags & PORT_FLAG_PROTOCOL) ==
            PORT_PROTOCOL_RAW)
    {
        return;
    }

    //
    // Save the value.
    //
    pState->ucModemState = ucModemState;
}
#endif

//*****************************************************************************
//
//! Handles link status notification.
//!
//! \param bLinkStatusUp is the boolean indicate of link layer status.
//!
//! This function should be called when the link layer status has changed.
//!
//! \return None.
//
//*****************************************************************************
void
TelnetNotifyLinkStatus(tBoolean bLinkStatusUp)
{
    int iPort;

    //
    // We don't care if the link is up, only if it goes down.
    //
    if(bLinkStatusUp)
    {
        return;
    }

    //
    // For every port, indicate that the link has been lost.
    //
    for(iPort = 0; iPort < MAX_S2E_PORTS; iPort++)
    {
        g_sTelnetSession[iPort].bLinkLost = true;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
