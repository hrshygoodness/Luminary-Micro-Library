//*****************************************************************************
//
// can_device_fifo.c - This is the application that runs on the CAN device
//                     board and uses the CAN controller to communicate with
//                     the main board in FIFO mode.
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
// This is part of revision 9453 of the EK-LM3S2965 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_can.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>CAN device FIFO mode example (can_device_fifo)</h1>
//!
//! This application uses the CAN controller in FIFO mode to communicate with
//! the CAN FIFO mode example program that is running on the main board. The
//! main board must have the can_fifo example program loaded for this example
//! to function. This application will simply echo all data that it receives
//! back in it's receive FIFO back out it's transmit FIFO.
//
//*****************************************************************************

//
// Size of the FIFOs allocated to the CAN controller.
//
#define CAN_FIFO_SIZE           (8 * 8)

//
// Message object used by the transmit message FIFO.
//
#define TRANSMIT_MESSAGE_ID     11

//
// Message object used by the receive message FIFO.
//
#define RECEIVE_MESSAGE_ID      8

//
// The number of FIFO transfers that cause a toggle of the LED.
//
#define TOGGLE_RATE             100

//
// The CAN bit rate.
//
#define CAN_BITRATE             250000

//
// This structure holds all of the state information for the CAN transfers.
//
struct
{
    //
    // This holds the information for the data receive message object that is
    // used to receive data for each CAN controller.
    //
    tCANMsgObject MsgObjectRx;

    //
    // This holds the information for the data send message object that is used
    // to send data for each CAN controller.
    //
    tCANMsgObject MsgObjectTx;

    //
    // Data Buffer.
    //
    unsigned char pucBuffer[CAN_FIFO_SIZE];

    //
    // Bytes remaining to be received.
    //
    unsigned long ulBytesRemaining;

    //
    // Bytes transmitted.
    //
    unsigned long ulBytesTransmitted;

    //
    // The current state of the CAN controller.
    //
    enum
    {
        CAN_IDLE,
        CAN_SENDING,
        CAN_WAIT_RX,
        CAN_PROCESS,
    } eState;
} g_sCAN;

//
// Used by the ToggleLED function to set the toggle rate.
//
unsigned long g_ulLEDCount;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// The CAN controller interrupt handler.
//
//*****************************************************************************
void
CANIntHandler(void)
{
    unsigned long ulStatus;

    //
    // Find the cause of the interrupt, if it is a status interrupt then just
    // acknowledge the interrupt by reading the status register.
    //
    ulStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    //
    // The first eight message objects make up the Transmit message FIFO.
    //
    if(ulStatus <= 8)
    {
        //
        // Increment the number of bytes transmitted.
        //
        g_sCAN.ulBytesTransmitted += 8;
    }
    //
    // The second eight message objects make up the Receive message FIFO.
    //
    else if((ulStatus > 8) && (ulStatus <= 16))
    {
        //
        // Read the data out and acknowledge that it was read.
        //
        CANMessageGet(CAN0_BASE, ulStatus, &g_sCAN.MsgObjectRx, 1);

        //
        // Advance the read pointer.
        //
        g_sCAN.MsgObjectRx.pucMsgData += 8;

        //
        // Decrement the expected bytes remaining.
        //
        g_sCAN.ulBytesRemaining -= 8;
    }
    else
    {
        //
        // This was a status interrupt so read the current status to
        // clear the interrupt and return.
        //
        CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
    }

    //
    // Acknowledge the CAN controller interrupt has been handled.
    //
    CANIntClear(CAN0_BASE, ulStatus);
}

//*****************************************************************************
//
// This function configures the transmit FIFO and copies data into the FIFO.
//
//*****************************************************************************
int
CANTransmitFIFO(unsigned char *pucData, unsigned long ulSize)
{
    int iIdx;

    //
    // This is the message object used to send button updates.  This message
    // object will not be "set" right now as that would trigger a transmission.
    //
    g_sCAN.MsgObjectTx.ulMsgID = TRANSMIT_MESSAGE_ID;
    g_sCAN.MsgObjectTx.ulMsgIDMask = 0;

    //
    // This enables interrupts for transmitted messages.
    //
    g_sCAN.MsgObjectTx.ulFlags = MSG_OBJ_TX_INT_ENABLE;

    //
    // Return the maximum possible number of bytes that can be sent in a single
    // FIFO.
    //
    if(ulSize > CAN_FIFO_SIZE)
    {
        return(CAN_FIFO_SIZE);
    }

    //
    // Loop through all eight message objects that are part of the transmit
    // FIFO.
    //
    for(iIdx = 0; iIdx < 8; iIdx++)
    {
        //
        // If there are more than eight bytes remaining then use a full message
        // to transfer these 8 bytes.
        //
        if(ulSize > 8)
        {
            //
            // Set the length of the message, which can only be eight bytes
            // in this case as it is all that can be sent with a single message
            // object.
            //
            g_sCAN.MsgObjectTx.ulMsgLen = 8;
            g_sCAN.MsgObjectTx.pucMsgData = &pucData[iIdx * 8];

            //
            // Set the MSG_OBJ_FIFO to indicate that this is not the last
            // data in a chain of FIFO entries.
            //
            g_sCAN.MsgObjectTx.ulFlags |= MSG_OBJ_FIFO;

            //
            // There are now eight less bytes to transmit.
            //
            ulSize -= 8;

            //
            // Write out this message object.
            //
            CANMessageSet(CAN0_BASE, iIdx + 1, &g_sCAN.MsgObjectTx,
                          MSG_OBJ_TYPE_TX);
        }
        //
        // If there are less than or exactly eight bytes remaining then use a
        // message object to transfer these 8 bytes and do not set the
        // MSG_OBJ_FIFO flag to indicate that this is the last of the entries
        // in this FIFO.
        //
        else
        {
            //
            // Set the length to the remaining bytes and transmit the data.
            //
            g_sCAN.MsgObjectTx.ulMsgLen = ulSize;
            g_sCAN.MsgObjectTx.pucMsgData = &pucData[iIdx * 8];

            //
            // Write out this message object.
            //
            CANMessageSet(CAN0_BASE, iIdx + 1, &g_sCAN.MsgObjectTx,
                          MSG_OBJ_TYPE_TX);
        }
    }
    return(0);
}

//*****************************************************************************
//
// This function configures the receive FIFO and should only be called once.
//
//*****************************************************************************
int
CANReceiveFIFO(unsigned char *pucData, unsigned long ulSize)
{
    int iIdx;

    if(ulSize > CAN_FIFO_SIZE)
    {
        return(CAN_FIFO_SIZE);
    }

    //
    // Configure the receive message FIFO to accept the transmit message object.
    //
    g_sCAN.MsgObjectRx.ulMsgID = RECEIVE_MESSAGE_ID;
    g_sCAN.MsgObjectRx.ulMsgIDMask = 0;

    //
    // This enables interrupts for received messages.
    //
    g_sCAN.MsgObjectRx.ulFlags = MSG_OBJ_RX_INT_ENABLE;

    //
    // Remember the beginning of the FIFO location.
    //
    g_sCAN.MsgObjectRx.pucMsgData = pucData;

    //
    // Transfer bytes in multiples of eight bytes.
    //
    for(iIdx=0; iIdx < (CAN_FIFO_SIZE / 8); iIdx++)
    {
        //
        // If there are more than eight remaining to be sent then just queue up
        // eight bytes and go on to the next message object(s) for the
        // remaining bytes.
        //
        if(ulSize > 8)
        {
            //
            // The length is always eight as the full buffer is divisible by 8.
            //
            g_sCAN.MsgObjectRx.ulMsgLen = 8;

            //
            // There are now eight less bytes to receive.
            //
            ulSize -=8;

            //
            // Set the MSG_OBJ_FIFO to indicate that this is not the last
            // data in a chain of FIFO entries.
            //
            g_sCAN.MsgObjectRx.ulFlags |= MSG_OBJ_FIFO;

            //
            // Make sure that all message objects up to the last indicate that
            // they are part of a FIFO.
            //
            CANMessageSet(CAN0_BASE, iIdx + 9, &g_sCAN.MsgObjectRx,
                          MSG_OBJ_TYPE_RX);
        }
        else
        {
            //
            // Get the remaining bytes.
            //
            g_sCAN.MsgObjectRx.ulMsgLen = ulSize;

            //
            // This is the last message object in a FIFO so don't set the FIFO
            // to indicate that the FIFO ends with this message object.
            //
            CANMessageSet(CAN0_BASE, iIdx + 9, &g_sCAN.MsgObjectRx,
                          MSG_OBJ_TYPE_RX);
        }
    }
    return(0);
}

//*****************************************************************************
//
// This function just handles toggling the LED periodically during data
// transfer.
//
//*****************************************************************************
void
ToggleLED(void)
{
    //
    // Handle toggling the LED.
    //
    if(g_ulLEDCount < TOGGLE_RATE)
    {
        //
        // Turn off the LED.
        //
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
    }
    else if(g_ulLEDCount == TOGGLE_RATE)
    {
        //
        // Turn on the LED.
        //
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
    }
    else if(g_ulLEDCount == (TOGGLE_RATE << 1))
    {
        //
        // Reset the toggle count.
        //
        g_ulLEDCount = 0;
    }

    //
    // Increment the toggle count.
    //
    g_ulLEDCount++;
}

//*****************************************************************************
//
// This is the main loop for the application.
//
//*****************************************************************************
int
main(void)
{
    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Set the clocking to run directly from the PLL at 50MHz.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Configure CAN 0 Pins.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure LED pin.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Configure GPIO Pin used for the LED.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);

    //
    // Turn off the LED.
    //
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    //
    // Enable the CAN controller.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    //
    // Reset the state of all the message object and the state of the CAN
    // module to a known state.
    //
    CANInit(CAN0_BASE);

    //
    // Configure the bit rate for the CAN device, the clock rate to the CAN
    // controller is fixed at 8MHz for this class of device and the bit rate is
    // set to CAN_BITRATE.
    //
    CANBitRateSet(CAN0_BASE, 8000000, CAN_BITRATE);

    //
    // Take the CAN0 device out of INIT state.
    //
    CANEnable(CAN0_BASE);

    //
    // Enable interrupts from CAN controller.
    //
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);

    //
    // Enable interrupts for the CAN in the NVIC.
    //
    IntEnable(INT_CAN0);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Set the initial state to wait for data.
    //
    g_sCAN.eState = CAN_WAIT_RX;

    //
    // Reset the buffer pointer.
    //
    g_sCAN.MsgObjectRx.pucMsgData = g_sCAN.pucBuffer;

    //
    // Set the total number of bytes expected.
    //
    g_sCAN.ulBytesRemaining = CAN_FIFO_SIZE;

    //
    // Configure the receive message FIFO.
    //
    CANReceiveFIFO(g_sCAN.pucBuffer, CAN_FIFO_SIZE);

    //
    // Initialized the LED toggle count.
    //
    g_ulLEDCount = 0;

    //
    // Loop forever.
    //
    while(1)
    {
        switch(g_sCAN.eState)
        {
            case CAN_IDLE:
            {
                //
                // Switch to sending state.
                //
                g_sCAN.eState = CAN_SENDING;

                //
                // Initialize the transmit count to zero.
                //
                g_sCAN.ulBytesTransmitted = 0;

                //
                // Schedule all of the CAN transmissions.
                //
                CANTransmitFIFO(g_sCAN.pucBuffer, CAN_FIFO_SIZE);

                break;
            }
            case CAN_SENDING:
            {
                //
                // Wait for all bytes to go out.
                //
                if(g_sCAN.ulBytesTransmitted == CAN_FIFO_SIZE)
                {
                    //
                    // Switch to wait for RX state.
                    //
                    g_sCAN.eState = CAN_WAIT_RX;
                }

                break;
            }
            case CAN_WAIT_RX:
            {
                //
                // Wait for all new data to be received.
                //
                if(g_sCAN.ulBytesRemaining == 0)
                {
                    //
                    // Switch to wait for Process data state.
                    //
                    g_sCAN.eState = CAN_PROCESS;

                    //
                    // Reset the buffer pointer.
                    //
                    g_sCAN.MsgObjectRx.pucMsgData = g_sCAN.pucBuffer;

                    //
                    // Reset the number of bytes expected.
                    //
                    g_sCAN.ulBytesRemaining = CAN_FIFO_SIZE;
                }
                break;
            }
            case CAN_PROCESS:
            {
                //
                // Handle the LED toggle.
                //
                ToggleLED();

                //
                // Return to the idle state.
                //
                g_sCAN.eState = CAN_IDLE;

                break;
            }
            default:
            {
                break;
            }
        }
    }
}
