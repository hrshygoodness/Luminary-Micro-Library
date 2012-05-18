//*****************************************************************************
//
// uart_if.c - Control interface using the first UART.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/uart.h"
#include "driverlib/watchdog.h"
#include "shared/can_proto.h"
#include "can_if.h"
#include "constants.h"
#include "controller.h"
#include "message.h"
#include "param.h"
#include "pins.h"
#include "uart_if.h"

//*****************************************************************************
//
// The buffer that contains the message received from the UART.
//
//*****************************************************************************
static unsigned char g_pucUARTMessage[12];

//*****************************************************************************
//
// The size of the message being received from the UART.
//
//*****************************************************************************
static unsigned long g_ulUARTSize;

//*****************************************************************************
//
// The length of the message received from the UART.
//
//*****************************************************************************
static unsigned long g_ulUARTLength;

//*****************************************************************************
//
// The buffer that contains the message(s) to be sent via the UART.
//
//*****************************************************************************
#define UART_XMIT_SIZE          64
static unsigned char g_pucUARTXmit[UART_XMIT_SIZE];

//*****************************************************************************
//
// The position of the next byte to be sent via the UART.  The buffer is empty
// when this value is equal to g_ulUARTXmitWrite.
//
//*****************************************************************************
static unsigned long g_ulUARTXmitRead;

//*****************************************************************************
//
// The position of the next available space in the UART send message buffer.
// The buffer is full when this value is one less than g_ulUARTXmitRead.
//
//*****************************************************************************
static unsigned long g_ulUARTXmitWrite;

//*****************************************************************************
//
// The current state of the character receive state machine.  This takes care
// of parsing the start of the packet as well as decoding the escaped
// characters.
//
//*****************************************************************************
#define UART_STATE_IDLE         0
#define UART_STATE_LENGTH       1
#define UART_STATE_DATA         2
#define UART_STATE_ESCAPE       3
static unsigned long g_ulUARTState = UART_STATE_IDLE;

//*****************************************************************************
//
// Flags that indicate various events
//
//*****************************************************************************
#define UART_FLAG_ENUM          0
#define UART_FLAG_PSTATUS       1
static unsigned long g_ulUARTFlags = 0;

//*****************************************************************************
//
// Sends a character to the UART.
//
//*****************************************************************************
static void
UARTIFPutChar(unsigned long ulChar)
{
    //
    // See if the character being sent is 0xff.
    //
    if(ulChar == 0xff)
    {
        //
        // Send 0xfe 0xfe, the escaped version of 0xff.  A sign extended
        // version of 0xfe is used to avoid the check below for 0xfe, thereby
        // avoiding an infinite loop.  Only the lower 8 bits are actually sent,
        // so 0xfe is what is actually transmitted.
        //
        UARTIFPutChar(0xfffffffe);
        UARTIFPutChar(0xfffffffe);
    }

    //
    // Otherwise, see if the character being sent is 0xfe.
    //
    else if(ulChar == 0xfe)
    {
        //
        // Send 0xfe 0xfd, the escaped version of 0xfe.  A sign extended
        // version of 0xfe is used to avoid the check above for 0xfe, thereby
        // avoiding an infinite loop.  Only the lower 8 bits are actually sent,
        // so 0xfe is what is actually transmitted.
        //
        UARTIFPutChar(0xfffffffe);
        UARTIFPutChar(0xfd);
    }

    //
    // Otherwise, simply send this character.
    //
    else
    {
        //
        // Disable the UART interrupt to avoid having characters stick in the
        // local buffer.
        //
        ROM_UARTIntDisable(UART0_BASE, UART_INT_TX);

        //
        // See if the local buffer is empty and there is space available in the
        // UART FIFO.
        //
        if((g_ulUARTXmitRead == g_ulUARTXmitWrite) &&
           ROM_UARTSpaceAvail(UART0_BASE))
        {
            //
            // Simply write this character into the UART FIFO.
            //
            ROM_UARTCharPut(UART0_BASE, ulChar);
        }
        else
        {
            //
            // Write this character into the local buffer.
            //
            g_pucUARTXmit[g_ulUARTXmitWrite] = ulChar;

            //
            // Increment the local write buffer pointer.
            //
            g_ulUARTXmitWrite = (g_ulUARTXmitWrite + 1) % UART_XMIT_SIZE;
        }

        //
        // Re-enable the UART interrupt.
        //
        ROM_UARTIntEnable(UART0_BASE, UART_INT_TX);
    }
}

//*****************************************************************************
//
// Sends a message to the UART.
//
//*****************************************************************************
void
UARTIFSendMessage(unsigned long ulID, unsigned char *pucData,
                  unsigned long ulDataLength)
{
    //
    // Send the start of packet indicator.  A sign extended version of 0xff is
    // used to avoid having it escaped.
    //
    UARTIFPutChar(0xffffffff);

    //
    // Send the length of the data packet.
    //
    UARTIFPutChar(ulDataLength + 4);

    //
    // Send the message ID.
    //
    UARTIFPutChar(ulID & 0xff);
    UARTIFPutChar((ulID >> 8) & 0xff);
    UARTIFPutChar((ulID >> 16) & 0xff);
    UARTIFPutChar((ulID >> 24) & 0xff);

    //
    // Send the associated data, if any.
    //
    while(ulDataLength--)
    {
        UARTIFPutChar(*pucData++);
    }
}

//*****************************************************************************
//
// Handles general commands.
//
//*****************************************************************************
static void
UARTIFCommandHandler(void)
{
    unsigned long ulID, ulAck, *pulMessage, *pulResponse;

    //
    // Create local pointers of a different type to avoid later type casting.
    //
    pulMessage = (unsigned long *)g_pucUARTMessage;
    pulResponse = (unsigned long *)g_pucResponse;

    //
    // Get the message ID from the message buffer.
    //
    ulID = pulMessage[0];

    //
    // See if this is a system command or a message not intended for this
    // device.
    //
    if(((ulID & (CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M |
                 CAN_MSGID_API_CLASS_M)) == 0) ||
       ((ulID & CAN_MSGID_DEVNO_M) != g_sParameters.ucDeviceNumber) ||
       (g_sParameters.ucDeviceNumber == 0))
    {
        //
        // Send this message out over the CAN bus.
        //
        CANIFSendBridgeMessage(ulID, g_pucUARTMessage + 4, g_ulUARTLength - 4);
    }

    //
    // See if this is a system command or a message intended for this device.
    //
    if(((ulID & (CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M |
                 CAN_MSGID_API_CLASS_M)) == 0) ||
       (((ulID & CAN_MSGID_DEVNO_M) == g_sParameters.ucDeviceNumber) &&
        (g_sParameters.ucDeviceNumber != 0)))
    {
        //
        // Handle this command.
        //
        ulAck = MessageCommandHandler(ulID, g_pucUARTMessage + 4,
                                      g_ulUARTLength - 4);

        //
        // Send back the response if one was generated.
        //
        if(g_ulResponseLength != 0)
        {
            UARTIFSendMessage(pulResponse[0], g_pucResponse + 4,
                              g_ulResponseLength - 4);
        }

        //
        // Send back an ACK if required.
        //
        if(ulAck == 1)
        {
            UARTIFSendMessage(LM_API_ACK | g_sParameters.ucDeviceNumber, 0, 0);
        }
    }
}

//*****************************************************************************
//
// Handles interrupts from the UART.
//
//*****************************************************************************
void
UART0IntHandler(void)
{
    unsigned long ulStatus;
    unsigned char ucChar;

    //
    // Get the interrupts that are being asserted by the UART.
    //
    ulStatus = ROM_UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    ROM_UARTIntClear(UART0_BASE, ulStatus);

    //
    // Indicate that the UART link is good.
    //
    ControllerLinkGood(LINK_TYPE_UART);

    //
    // See if the receive interrupt has been asserted.
    //
    if(ulStatus & (UART_INT_RX | UART_INT_RT))
    {
        //
        // Loop while there are more characters to be read from the UART.
        //
        while(ROM_UARTCharsAvail(UART0_BASE))
        {
            //
            // Read the next character from the UART.
            //
            ucChar = ROM_UARTCharGet(UART0_BASE);

            //
            // See if this is a start of packet byte.
            //
            if(ucChar == 0xff)
            {
                //
                // Reset the length of the UART message.
                //
                g_ulUARTLength = 0;

                //
                // Set the state such that the next byte received is the size
                // of the message.
                //
                g_ulUARTState = UART_STATE_LENGTH;
            }

            //
            // See if this byte is the size of the message.
            //
            else if(g_ulUARTState == UART_STATE_LENGTH)
            {
                //
                // Save the size of the message.
                //
                g_ulUARTSize = ucChar;

                //
                // Subsequent bytes received are the message data.
                //
                g_ulUARTState = UART_STATE_DATA;
            }

            //
            // See if the previous character was an escape character.
            //
            else if(g_ulUARTState == UART_STATE_ESCAPE)
            {
                //
                // See if this 0xfe, the escaped version of 0xff.
                //
                if(ucChar == 0xfe)
                {
                    //
                    // Store a 0xff in the message buffer.
                    //
                    g_pucUARTMessage[g_ulUARTLength++] = 0xff;

                    //
                    // Subsequent bytes received are the message data.
                    //
                    g_ulUARTState = UART_STATE_DATA;
                }

                //
                // Otherwise, see if this is 0xfd, the escaped version of 0xfe.
                //
                else if(ucChar == 0xfd)
                {
                    //
                    // Store a 0xfe in the message buffer.
                    //
                    g_pucUARTMessage[g_ulUARTLength++] = 0xfe;

                    //
                    // Subsequent bytes received are the message data.
                    //
                    g_ulUARTState = UART_STATE_DATA;
                }

                //
                // Otherwise, this is a corrupted sequence.  Set the receiver
                // to idle so this message is dropped, and subsequent data is
                // ignored until another start of packet is received.
                //
                else
                {
                    g_ulUARTState = UART_STATE_IDLE;
                }
            }

            //
            // See if this is a part of the message data.
            //
            else if(g_ulUARTState == UART_STATE_DATA)
            {
                //
                // See if this character is an escape character.
                //
                if(ucChar == 0xfe)
                {
                    //
                    // The next byte is an escaped byte.
                    //
                    g_ulUARTState = UART_STATE_ESCAPE;
                }
                else
                {
                    //
                    // Store this byte in the message buffer.
                    //
                    g_pucUARTMessage[g_ulUARTLength++] = ucChar;
                }
            }

            //
            // See if the entire message has been received but has not been
            // processed (i.e. the most recent byte received was the end of the
            // message).
            //
            if((g_ulUARTLength == g_ulUARTSize) &&
               (g_ulUARTState == UART_STATE_DATA))
            {
                //
                // Process this message.
                //
                UARTIFCommandHandler();

                //
                // The UART interface is idle, meaning all bytes will be
                // dropped until the next start of packet byte.
                //
                g_ulUARTState = UART_STATE_IDLE;
            }
        }

        //
        // Tell the controller that CAN activity was detected.
        //
        ControllerWatchdog(LINK_TYPE_UART);
    }

    //
    // See if the transmit interrupt has been asserted.
    //
    if(ulStatus & UART_INT_TX)
    {
        //
        // Loop while there are more characters to be transmitted and more
        // space in the UART FIFO.
        //
        while((g_ulUARTXmitRead != g_ulUARTXmitWrite) &&
              ROM_UARTSpaceAvail(UART0_BASE))
        {
            //
            // Put the next character into the UART FIFO.
            //
            ROM_UARTCharPut(UART0_BASE, g_pucUARTXmit[g_ulUARTXmitRead]);

            //
            // Increment the read pointer.
            //
            g_ulUARTXmitRead = (g_ulUARTXmitRead + 1) % UART_XMIT_SIZE;
        }
    }

    //
    // See if an enumeration response needs to be sent.
    //
    if(HWREGBITW(&g_ulUARTFlags, UART_FLAG_ENUM) != 0)
    {
        //
        // Send the enumeration response for this device.
        //
        UARTIFSendMessage(CAN_MSGID_API_ENUMERATE |
                          g_sParameters.ucDeviceNumber, 0, 0);

        //
        // Clear the enumeration response flag.
        //
        HWREGBITW(&g_ulUARTFlags, UART_FLAG_ENUM) = 0;
    }

    //
    // See if periodic status messages need to be sent.
    //
    if(HWREGBITW(&g_ulUARTFlags, UART_FLAG_PSTATUS) != 0)
    {
        //
        // Send out the first periodic status message if it needs to be sent.
        //
        if(g_ulPStatFlags & 1)
        {
            UARTIFSendMessage(LM_API_PSTAT_DATA_S0 |
                              g_sParameters.ucDeviceNumber,
                              g_ppucPStatMessages[0],
                              g_pucPStatMessageLen[0]);
        }

        //
        // Send out the second periodic status message if it needs to be sent.
        //
        if(g_ulPStatFlags & 2)
        {
            UARTIFSendMessage(LM_API_PSTAT_DATA_S1 |
                              g_sParameters.ucDeviceNumber,
                              g_ppucPStatMessages[1],
                              g_pucPStatMessageLen[1]);
        }

        //
        // Send out the third periodic status message if it needs to be sent.
        //
        if(g_ulPStatFlags & 4)
        {
            UARTIFSendMessage(LM_API_PSTAT_DATA_S2 |
                              g_sParameters.ucDeviceNumber,
                              g_ppucPStatMessages[2],
                              g_pucPStatMessageLen[2]);
        }

        //
        // Send out the fourth periodic status message if it needs to be sent.
        //
        if(g_ulPStatFlags & 8)
        {
            UARTIFSendMessage(LM_API_PSTAT_DATA_S3 |
                              g_sParameters.ucDeviceNumber,
                              g_ppucPStatMessages[3],
                              g_pucPStatMessageLen[3]);
        }

        //
        // Clear the periodic status message flag.
        //
        HWREGBITW(&g_ulUARTFlags, UART_FLAG_PSTATUS) = 0;
    }
}

//*****************************************************************************
//
// Indicates that an enumeration response should be sent for this device.
//
//*****************************************************************************
void
UARTIFEnumerate(void)
{
    //
    // Set the enumeration response flag.
    //
    HWREGBITW(&g_ulUARTFlags, UART_FLAG_ENUM) = 1;

    //
    // Generate a fake UART interrupt, during which the enumeration response
    // will be sent.
    //
    HWREG(NVIC_SW_TRIG) = INT_UART0 - 16;
}

//*****************************************************************************
//
// Indicates that an enumeration response should be sent for this device.
//
//*****************************************************************************
void
UARTIFPStatus(void)
{
    //
    // Set the Periodic Status flag.
    //
    HWREGBITW(&g_ulUARTFlags, UART_FLAG_PSTATUS) = 1;

    //
    // Generate a fake UART interrupt, during which the enumeration response
    // will be sent.
    //
    HWREG(NVIC_SW_TRIG) = INT_UART0 - 16;
}

//*****************************************************************************
//
// Initializes the UART and prepares it to be used as a control interface.
//
//*****************************************************************************
void
UARTIFInit(void)
{
    //
    // Configure the UART pins.
    //
#if UART_RX_PORT == UART_TX_PORT
    ROM_GPIOPinTypeUART(UART_RX_PORT, UART_RX_PIN | UART_TX_PIN);
#else
    ROM_GPIOPinTypeUART(UART_RX_PORT, UART_RX_PIN);
    ROM_GPIOPinTypeUART(UART_TX_PORT, UART_TX_PIN);
#endif

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, SYSCLK, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    //
    // Enable the UART interrupts.
    //
    ROM_UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
    ROM_IntEnable(INT_UART0);

    //
    // Send an enumeration response message to the UART to indicate that the
    // firmware has just started.
    //
    if(g_sParameters.ucDeviceNumber != 0)
    {
        UARTIFSendMessage(CAN_MSGID_API_ENUMERATE |
                          g_sParameters.ucDeviceNumber, 0, 0);
    }
}
