//*****************************************************************************
//
// motor.c - Ethernet interface to the BLDC motor control RDK.
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
// This is part of revision 10636 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/interrupt.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "motor.h"

//*****************************************************************************
//
// The commands, parameters, and tags required for communicating with the BLDC
// motor control RDK.
//
//*****************************************************************************
#define TAG_CMD                 0xff
#define TAG_STATUS              0xfe
#define TAG_DATA                0xfd
#define CMD_GET_PARAM_VALUE     0x12
#define CMD_SET_PARAM_VALUE     0x13
#define CMD_ENABLE_DATA_ITEM    0x21
#define CMD_START_DATA_STREAM   0x23
#define CMD_RUN                 0x30
#define CMD_STOP                0x31
#define PARAM_TARGET_SPEED      0x04
#define DATA_ROTOR_SPEED        0x07

//*****************************************************************************
//
// A set of flags that indicate commands that need to be sent to the BLDC
// motor control RDK.
//
//*****************************************************************************
#define FLAG_RUN                0
#define FLAG_STOP               1
#define FLAG_SET_SPEED          2
static unsigned long g_ulFlags;

//*****************************************************************************
//
// The current state of the motor drive Ethernet connection.  This will be one
// of MOTOR_STATE_DISCON, MOTOR_STATE_CONNECTING, or MOTOR_STATE_CONNECTED.
//
//*****************************************************************************
unsigned long g_ulMotorState;

//*****************************************************************************
//
// The target speed for the BLDC motor.
//
//*****************************************************************************
unsigned long g_ulTargetSpeed;

//*****************************************************************************
//
// The current speed of the BLDC motor.
//
//*****************************************************************************
unsigned long g_ulMotorSpeed;

//*****************************************************************************
//
// The board's ethernet MAC address.
//
//*****************************************************************************
unsigned char g_pucMACAddr[6];

//*****************************************************************************
//
// The current state of the state machine that parses data received from the
// BLDC motor control RDK board.
//
//*****************************************************************************
#define STATE_NONE              0
#define STATE_STATUS1           1
#define STATE_STATUS2           2
#define STATE_DATA1             3
#define STATE_DATA2             4
static unsigned long g_ulState = STATE_NONE;

//*****************************************************************************
//
// A buffer for storing the data from status packets.
//
//*****************************************************************************
static unsigned char g_pucPacket[16];

//*****************************************************************************
//
// The index into the status data buffer of the next byte to write.
//
//*****************************************************************************
static unsigned long g_ulIndex;

//*****************************************************************************
//
// The number of bytes left to read from the status packet.
//
//*****************************************************************************
static unsigned long g_ulCount;

//*****************************************************************************
//
// A flag that is set every time data is received on the TCP connection.
// Failure to set this flag for two polling periods indicates that the
// connection was lost, causing it to be aborted.
//
//*****************************************************************************
static unsigned long g_ulReceiveFlag;

//*****************************************************************************
//
// lwIP callback function that is called when data is received on the TCP
// connection.
//
//*****************************************************************************
static err_t
BLDCReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    unsigned long ulCount, ulLen;
    unsigned char *pucBuffer;
    struct pbuf *q;

    //
    // Only process the data if there was not an error and there is a pbuf
    // containing the data.
    //
    if((err == ERR_OK) && (p != NULL))
    {
        //
        // Set the receive flag, indicating that the connection is still
        // active.
        //
        g_ulReceiveFlag = 1;

        //
        // Tell lwIP that the data has been received by the application.
        //
        tcp_recved(pcb, p->tot_len);

        //
        // Get the total number of bytes in this pbuf chain.
        //
        ulCount = p->tot_len;

        //
        // Get a new pointer to the beginning of the pbuf chain.  This
        // perserves the pointer to the head fo the pbuf chain so that it can
        // be freed when the data has been processed.
        //
        q = p;

        //
        // The initial buffer pointer and length are zero.
        //
        pucBuffer = 0;
        ulLen = 0;

        //
        // Loop while there is more data to be processed.
        //
        while(ulCount)
        {
            //
            // If the data length is zero, then get the data from the next
            // pbuf in the chain.
            //
            if(ulLen == 0)
            {
                //
                // Set the buffer pointer and length based on the current pbuf.
                //
                pucBuffer = q->payload;
                ulLen = q->len;

                //
                // Advance to the next pbuf in the chain.
                //
                q = q->next;
            }

            //
            // Determine the action to take based on the current state of
            // the data stream.
            //
            switch(g_ulState)
            {
                //
                // Search for a packet start tag.
                //
                case STATE_NONE:
                {
                    //
                    // See if this is a status tag.
                    //
                    if(*pucBuffer == TAG_STATUS)
                    {
                        //
                        // Advance to the first status state.
                        //
                        g_ulState = STATE_STATUS1;
                    }

                    //
                    // See if this is a real-time data tag.
                    //
                    else if(*pucBuffer == TAG_DATA)
                    {
                        //
                        // Advance to the first real-time data state.
                        //
                        g_ulState = STATE_DATA1;
                    }

                    //
                    // Skip this byte of data.
                    //
                    pucBuffer++;
                    ulLen--;

                    //
                    // This state has been handled.
                    //
                    break;
                }

                //
                // The first status state, in which the packet length is
                // extracted.
                //
                case STATE_STATUS1:
                {
                    //
                    // Get the size of the packet, minus the two bytes that
                    // have already been received.
                    //
                    g_ulCount = *pucBuffer++ - 2;

                    //
                    // Skip this byte of data.
                    //
                    ulLen--;

                    //
                    // Advance to the second status state.
                    //
                    g_ulState = STATE_STATUS2;

                    //
                    // This state has been handled.
                    //
                    break;
                }

                //
                // The second status state, in which the status packet is
                // consumed.
                //
                case STATE_STATUS2:
                {
                    //
                    // Skip this byte of data.
                    //
                    pucBuffer++;
                    ulLen--;

                    //
                    // Decrement the count of status packet bytes.
                    //
                    g_ulCount--;

                    //
                    // If this was the last byte of the status packet, then
                    // go back to the search for packet start state.
                    //
                    if(g_ulCount == 0)
                    {
                        g_ulState = STATE_NONE;
                    }

                    //
                    // This state has been handled.
                    //
                    break;
                }

                //
                // The first real-time data state, in which the packet length
                // is extracted.
                //
                case STATE_DATA1:
                {
                    //
                    // Reset the packet buffer index to zero.
                    //
                    g_ulIndex = 0;

                    //
                    // Get the size of the packet, minus the two bytes that
                    // have already been received.
                    //
                    g_ulCount = *pucBuffer++ - 2;

                    //
                    // Skip this byte of data.
                    //
                    ulLen--;

                    //
                    // Advance to the second real-time data state.
                    //
                    g_ulState = STATE_DATA2;

                    //
                    // This state has been handled.
                    //
                    break;
                }

                //
                // The second real-time data state, in which the status packet
                // is consumed.
                //
                case STATE_DATA2:
                {
                    //
                    // Save this byte of the packet to the packet buffer.
                    //
                    g_pucPacket[g_ulIndex++] = *pucBuffer++;

                    //
                    // Decrement the count of bytes in the pbuf.
                    //
                    ulLen--;

                    //
                    // Decrement the count of real-time data packet bytes.
                    //
                    g_ulCount--;

                    //
                    // See if this was the last byte of the real-time data
                    // packet.
                    //
                    if(g_ulCount == 0)
                    {
                        //
                        // Extract the current motor speed from the real-time
                        // data packet.
                        //
                        g_ulMotorSpeed = *(unsigned long *)g_pucPacket;

                        //
                        // Go back to the search for packet start state.
                        //
                        g_ulState = STATE_NONE;
                    }

                    //
                    // This state has been handled.
                    //
                    break;
                }
            }

            //
            // Decrement the count of bytes in the pbuf chain.
            //
            ulCount--;
        }

        //
        // Free this pbuf chain.
        //
        pbuf_free(p);
    }

    //
    // Return success.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
// lwIP callback function that is called when the TCP connection is
// established.
//
//*****************************************************************************
static err_t
BLDCConnect(void *arg, struct tcp_pcb *pcb, err_t err)
{
    unsigned char pucBuffer[18];

    //
    // Indicate that the connection is established.
    //
    g_ulMotorState = MOTOR_STATE_CONNECTED;

    //
    // Reset the motor command flags.
    //
    g_ulFlags = 0;

    //
    // Construct a command to the board to enable the rotor speed real-time
    // data item.
    //
    pucBuffer[0] = TAG_CMD;
    pucBuffer[1] = 0x05;
    pucBuffer[2] = CMD_ENABLE_DATA_ITEM;
    pucBuffer[3] = DATA_ROTOR_SPEED;
    pucBuffer[4] = ((0 - TAG_CMD - 0x05 - CMD_ENABLE_DATA_ITEM -
                     DATA_ROTOR_SPEED) & 0xff);


    //
    // Construct a command to the board to enable the real-time data stream.
    //
    pucBuffer[5] = TAG_CMD;
    pucBuffer[6] = 0x04;
    pucBuffer[7] = CMD_START_DATA_STREAM;
    pucBuffer[8] = (0 - TAG_CMD - 0x04 - CMD_START_DATA_STREAM) & 0xff;

    //
    // Construct a command to the board to set the target speed.
    //
    pucBuffer[9] = TAG_CMD;
    pucBuffer[10] = 0x09;
    pucBuffer[11] = CMD_SET_PARAM_VALUE;
    pucBuffer[12] = PARAM_TARGET_SPEED;
    pucBuffer[13] = g_ulTargetSpeed & 0xff;
    pucBuffer[14] = (g_ulTargetSpeed >> 8) & 0xff;
    pucBuffer[15] = (g_ulTargetSpeed >> 16) & 0xff;
    pucBuffer[16] = (g_ulTargetSpeed >> 24) & 0xff;
    pucBuffer[17] = ((0 - TAG_CMD - 0x09 - CMD_SET_PARAM_VALUE -
                      PARAM_TARGET_SPEED - pucBuffer[13] - pucBuffer[14] -
                      pucBuffer[15] - pucBuffer[16]) & 0xff);

    //
    // Send the constructed commands to the board.
    //
    tcp_write(pcb, pucBuffer, 18, 1);

    //
    // Return success.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
// lwIP callback function that is called periodically while the TCP connection
// is active.
//
//*****************************************************************************
static err_t
BLDCPoll(void *arg, struct tcp_pcb *pcb)
{
    unsigned char pucBuffer[12];

    //
    // See if the motor connection is established.
    //
    if(g_ulMotorState == MOTOR_STATE_CONNECTED)
    {
        //
        // Determine the action to take based on the current receive flag.
        //
        switch(g_ulReceiveFlag)
        {
            //
            // See if the receive flag has not been set by the reception of
            // data.
            //
            case 0:
            default:
            {
                //
                // Set the receive flag to two so that the connection can be
                // aborted on the next poll if data has still not been
                // received.
                //
                g_ulReceiveFlag = 2;

                //
                // This state has been handled.
                //
                break;
            }

            //
            // See if the receive flag was set by the reception of data.
            //
            case 1:
            {
                //
                // Reset the receive flag.
                //
                g_ulReceiveFlag = 0;

                //
                // This state has been handled.
                //
                break;
            }

            //
            // See if the receive flag has not been set by the reception of
            // data for two polling periods.
            //
            case 2:
            {
                //
                // Abort the TCP connection.
                //
                tcp_abort(pcb);

                //
                // Set the motor connection state to disconnected.
                //
                g_ulMotorState = MOTOR_STATE_DISCON;

                //
                // Return success.
                //
                return(ERR_OK);
            }
        }
    }

    //
    // See if a run command should be sent to the board.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_RUN))
    {
        //
        // Construct a run command.
        //
        pucBuffer[0] = TAG_CMD;
        pucBuffer[1] = 0x04;
        pucBuffer[2] = CMD_RUN;
        pucBuffer[3] = (0 - TAG_CMD - 0x04 - CMD_RUN) & 0xff;

        //
        // Send the run command to the board.
        //
        tcp_write(pcb, pucBuffer, 4, 1);
    }

    //
    // See if a stop command should be sent to the board.
    //
    else if(HWREGBITW(&g_ulFlags, FLAG_STOP))
    {
        //
        // Construct a stop command.
        //
        pucBuffer[0] = TAG_CMD;
        pucBuffer[1] = 0x04;
        pucBuffer[2] = CMD_STOP;
        pucBuffer[3] = (0 - TAG_CMD - 0x04 - CMD_STOP) & 0xff;

        //
        // Send the stop command to the board.
        //
        tcp_write(pcb, pucBuffer, 4, 1);
    }

    //
    // See if a set target speed command should be sent to the board.
    //
    else if(HWREGBITW(&g_ulFlags, FLAG_SET_SPEED))
    {
        //
        // Construct a set target speed command.
        //
        pucBuffer[0] = TAG_CMD;
        pucBuffer[1] = 0x09;
        pucBuffer[2] = CMD_SET_PARAM_VALUE;
        pucBuffer[3] = PARAM_TARGET_SPEED;
        pucBuffer[4] = g_ulTargetSpeed & 0xff;
        pucBuffer[5] = (g_ulTargetSpeed >> 8) & 0xff;
        pucBuffer[6] = (g_ulTargetSpeed >> 16) & 0xff;
        pucBuffer[7] = (g_ulTargetSpeed >> 24) & 0xff;
        pucBuffer[8] = ((0 - TAG_CMD - 0x09 - CMD_SET_PARAM_VALUE -
                         PARAM_TARGET_SPEED - pucBuffer[4] - pucBuffer[5] -
                         pucBuffer[6] - pucBuffer[7]) & 0xff);

        //
        // Send the set target speed command to the board.
        //
        tcp_write(pcb, pucBuffer, 9, 1);
    }

    //
    // Clear the motor command flags.
    //
    g_ulFlags = 0;

    //
    // Return success.
    //
    return(ERR_OK);
}

//*****************************************************************************
//
// lwIP callback fucntion that is called when an error is encountered on the
// TCP connection.
//
//*****************************************************************************
static void
BLDCError(void *arg, err_t err)
{
    //
    // Set the motor conoection state to disconnected.
    //
    g_ulMotorState = MOTOR_STATE_DISCON;
}

//*****************************************************************************
//
// Initializes the interface to the BLDC motor control RDK board.  This does
// not make the initial connection.
//
//*****************************************************************************
void
MotorInit(void)
{
    unsigned long ulUser0, ulUser1;

    //
    // Get the MAC address from the UART0 and UART1 registers in NV ram.
    //
    FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV ram into a MAC address
    // array.
    //
    g_pucMACAddr[0] = ulUser0 & 0xff;
    g_pucMACAddr[1] = (ulUser0 >> 8) & 0xff;
    g_pucMACAddr[2] = (ulUser0 >> 16) & 0xff;
    g_pucMACAddr[3] = ulUser1 & 0xff;
    g_pucMACAddr[4] = (ulUser1 >> 8) & 0xff;
    g_pucMACAddr[5] = (ulUser1 >> 16) & 0xff;

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(g_pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(g_pucMACAddr);
    LocatorAppTitleSet("RDK-IDM bldc_ctrl");

    //
    // Reset the target speed to 3000 rpm.
    //
    g_ulTargetSpeed = 3000;

    //
    // Set the motor connection state to disconnected.
    //
    g_ulMotorState = MOTOR_STATE_DISCON;
}

//*****************************************************************************
//
// Initiates a connection to the BLDC motor control RDK board.
//
//*****************************************************************************
void
MotorConnect(void)
{
    struct ip_addr sAddr;
    struct tcp_pcb *pcb;

    //
    // Set the motor connection state to connecting.
    //
    g_ulMotorState = MOTOR_STATE_CONNECTING;

    //
    // Clear the motor command flags.
    //
    g_ulFlags = 0;

    //
    // Reset the data received flag.
    //
    g_ulReceiveFlag = 0;

    //
    // Disable the Ethernet interrupt while the connection is being created;
    // this is required since lwIP is not re-entrant.
    //
    IntDisable(INT_ETH);

    //
    // Create a new TCP socket.
    //
    pcb = tcp_new();
    if(pcb)
    {
        //
        // Initiate the connection to the telnet port on the board.
        //
        IP4_ADDR(&sAddr, 169, 254, 89, 71);
        tcp_connect(pcb, &sAddr, 23, BLDCConnect);

        //
        // Set the functions to be called upon errors, polls, and data
        // reception for this TCP socket.
        //
        tcp_err(pcb, BLDCError);
        tcp_poll(pcb, BLDCPoll, 1);
        tcp_recv(pcb, BLDCReceive);
    }

    //
    // Re-enable the Ethernet interrupt.
    //
    IntEnable(INT_ETH);
}

//*****************************************************************************
//
// Requests that a run command be sent to the BLDC motor control RDK board.
//
//*****************************************************************************
void
MotorRun(void)
{
    //
    // Set the flag indicating that a run command should be sent.  The actual
    // command will be sent on the next poll.
    //
    HWREGBITW(&g_ulFlags, FLAG_RUN) = 1;
}

//*****************************************************************************
//
// Requests that a stop command be sent to the BLDC motor control RDK board.
//
//*****************************************************************************
void
MotorStop(void)
{
    //
    // Set the flag indicating that a stop command should be sent.  The actual
    // command will be sent on the next poll.
    //
    HWREGBITW(&g_ulFlags, FLAG_STOP) = 1;
}

//*****************************************************************************
//
// Requests that the set target speed command be sent to the BLDC motor control
// RDK board.
//
//*****************************************************************************
void
MotorSpeedSet(unsigned long ulSpeed)
{
    //
    // Set the new target speed.
    //
    g_ulTargetSpeed = ulSpeed;

    //
    // Set the flag indicating that a set target speed command should be sent.
    // The actual command willb e sent on the next poll.
    //
    HWREGBITW(&g_ulFlags, FLAG_SET_SPEED) = 1;
}
