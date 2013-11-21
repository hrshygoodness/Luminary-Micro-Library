//*****************************************************************************
//
// ui_can.c - A simple control interface utilizing a CAN network.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "ui_common.h"
#include "ui_can.h"

//*****************************************************************************
//
// This holds the information for the HeartBeat message object that is used
// to receive heartbeat messages from the target board.
//
//*****************************************************************************
tCANMsgObject g_MsgObjectHeartBeat;

//*****************************************************************************
//
// This holds the information for the button receive message object.  It is
// used to receive messages from the target board when button press and
// release events occur.  There are two buttons and two events(press/release).
//
//*****************************************************************************
tCANMsgObject g_MsgObjectButton;

//****************************************************************************
//
// This is the message object number used by the Button message object.
//
//*****************************************************************************
#define MSGOBJ_NUM_BUTTON       1

//*****************************************************************************
//
// This is the message object number used by the LED message object.
//
//*****************************************************************************
#define MSGOBJ_NUM_HEARTBEAT    2

//****************************************************************************
//
// This is the message identifier to use for receiving button update
// requestes.
//
//****************************************************************************
#define MSGOBJ_ID_BUTTON        0x10

//****************************************************************************
//
// This is the message identifier to use for receiving LED update requests
// from the host.
//
//****************************************************************************
#define MSGOBJ_ID_HEARTBEAT     0x20

//****************************************************************************
//
// This event indicates that a button was pressed, it should be followed by
// the number of the button that was pressed.
//
//****************************************************************************
#define EVENT_BUTTON_PRESS      0x10

//****************************************************************************
//
// This event indicates that a button was pressed, it should be followed by
// the number of the button that was released.
//
//****************************************************************************
#define EVENT_BUTTON_RELEASED   0x11

//****************************************************************************
//
// This is the identifier for the target boards up button.
//
//****************************************************************************
#define TARGET_BUTTON_UP        0

//****************************************************************************
//
// This is the identifier for the target boards down button.
//
//****************************************************************************
#define TARGET_BUTTON_DN        1

//*****************************************************************************
//
// This global is used by the button message object to store the events that
// are coming back from the target board.
//
//*****************************************************************************
static unsigned char g_pucButtonMsg[2];

//*****************************************************************************
//
// This global is used by the heartbeat message object to store the message
// that is coming on from the target board.
//
//*****************************************************************************
static unsigned char g_pucHeartBeatMsg[4];

//*****************************************************************************
//
// This global is used to store the number of CAN messages that have been
// received since power-up.
//
//*****************************************************************************
volatile unsigned long g_ulCANRXCount = 0;

//*****************************************************************************
//
// This global is used to store the number of CAN messages that have been
// transmitted since power-up.
//
//*****************************************************************************
volatile unsigned long g_ulCANTXCount = 0;

//*****************************************************************************
//
// External references.
//
//*****************************************************************************
extern void UIButtonPress(void);
extern void MainSetSpeed(void);
extern unsigned short g_usTargetSpeed;

//*****************************************************************************
//
// The CAN controller Interrupt handler.
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

    switch(ulStatus)
    {
        //
        // Let the forground loop handle sending this, just set a flag to
        // indicate that the data should be sent.
        //
        case MSGOBJ_NUM_BUTTON:
        {
            //
            // Increment the message count.
            //
            g_ulCANRXCount++;

            //
            // Read the Button Message.
            //
            CANMessageGet(CAN0_BASE, MSGOBJ_NUM_BUTTON,
                &g_MsgObjectButton, 1);

            //
            // Only respond to buttons being release.
            //
            if(g_MsgObjectButton.pucMsgData[0] == EVENT_BUTTON_RELEASED)
            {
                UIButtonPress();
            }
            break;
        }

        //
        // When the LED message object interrupts, just clear the flag so that
        // more LED messages are allowed to transfer.
        //
        case MSGOBJ_NUM_HEARTBEAT:
        {
            //
            // Increment the message count.
            //
            g_ulCANRXCount++;

            //
            // Read the Button Message.
            //
            CANMessageGet(CAN0_BASE, MSGOBJ_NUM_HEARTBEAT,
                &g_MsgObjectHeartBeat, 1);

            break;
        }

        //
        // This was a status interrupt so read the current status to
        // clear the interrupt and return.
        //
        default:
        {
            break;
        }
    }

    //
    // Acknowledge the CAN controller interrupt has been handled.
    //
    CANIntClear(CAN0_BASE, ulStatus);
}

//*****************************************************************************
//
// This function configures the message objects used by this application.
// The following four message objects used by this application:
// MSGOBJ_ID_BUTTON, MSGOBJ_ID_LED, MSGOBJ_ID_DATA_TX, and MSGOBJ_ID_DATA_RX.
//
//*****************************************************************************
static void
UICANConfigureNetwork(void)
{
    //
    // Set the identifier and mask for the button object.
    //
    g_MsgObjectButton.ulMsgID = MSGOBJ_ID_BUTTON;
    g_MsgObjectButton.ulMsgIDMask = 0;

    //
    // This enables interrupts for received messages.
    //
    g_MsgObjectButton.ulFlags = MSG_OBJ_RX_INT_ENABLE;

    //
    // Set the size of the message and the data buffer used.
    //
    g_MsgObjectButton.ulMsgLen = 2;
    g_MsgObjectButton.pucMsgData = g_pucButtonMsg;

    //
    // Configure the Button receive message object.
    //
    CANMessageSet(CAN0_BASE, MSGOBJ_NUM_BUTTON, &g_MsgObjectButton,
                  MSG_OBJ_TYPE_RX);

    //
    // Set the identifier and mask for the heartbeat object.
    //
    g_MsgObjectHeartBeat.ulMsgID = MSGOBJ_ID_HEARTBEAT;
    g_MsgObjectHeartBeat.ulMsgIDMask = 0;

    //
    // This enables interrupts for received messages.
    //
    g_MsgObjectHeartBeat.ulFlags = MSG_OBJ_RX_INT_ENABLE;

    //
    // Set the size of the message and the data buffer used.
    //
    g_MsgObjectHeartBeat.ulMsgLen = 4;
    g_MsgObjectHeartBeat.pucMsgData = g_pucHeartBeatMsg;

    //
    // Configure the Button receive message object.
    //
    CANMessageSet(CAN0_BASE, MSGOBJ_NUM_HEARTBEAT, &g_MsgObjectHeartBeat,
                  MSG_OBJ_TYPE_RX);
}

//*****************************************************************************
//
// This function configures the CAN hardware and the message objects so that
// they are ready to use once the application returns from this function.
//
//*****************************************************************************
void
UICANInit(void)
{
    //
    // Configure CAN Pins
    //
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Reset the state of all the message object and the state of the CAN
    // module to a known state.
    //
    CANInit(CAN0_BASE);

    //
    // Configure the bit rate for the CAN device, the clock rate to the CAN
    // controller is fixed at 8MHz for this class of device and the bit rate is
    // set to 250000.
    //
    CANBitRateSet(CAN0_BASE, 8000000, 250000);

    //
    // Take the CAN1 device out of INIT state.
    //
    CANEnable(CAN0_BASE);

    //
    // Enable interrups from CAN controller.
    //
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);

    //
    // Set up the message object that will receive all messages on the CAN
    // bus.
    //
    UICANConfigureNetwork();

    //
    // Enable interrupts for the CAN in the NVIC.
    //
    IntEnable(INT_CAN0);
}

//*****************************************************************************
//
// This function handles connection with the other CAN device and also
// handles incoming commands.
//
// /return None.
//
//*****************************************************************************
void
UICANThread(void)
{
#if 0
    unsigned char pucData[8];

    //
    // The data has been received.
    //
    if((g_ulFlags & FLAG_DATA_RECV) == 0)
    {
        return;
    }

    //
    // Read the data from the message object.
    //
    g_MsgObjectRx.pucMsgData = pucData;
    g_MsgObjectRx.ulMsgLen = 8;
    CANMessageGet(CAN0_BASE, MSGOBJ_NUM_DATA_RX, &g_MsgObjectRx, 1);

    //
    // Indicate that the data has been read.
    //
    g_ulFlags &= (~FLAG_DATA_RECV);

    switch(g_MsgObjectRx.pucMsgData[0])
    {
        case CMD_GET_VERSION:
        {
            //
            // Send the Version.
            //
            g_ulFlags |= FLAG_DATA_TX_PEND;

            g_MsgObjectTx.pucMsgData = (unsigned char *)&g_ulVersion;
            g_MsgObjectTx.ulMsgLen = 4;
            CANMessageSet(CAN0_BASE, MSGOBJ_NUM_DATA_TX, &g_MsgObjectTx,
                          MSG_OBJ_TYPE_TX);
        }
    }

    //
    // Clear the flag.
    //
    g_ulFlags &= ~(FLAG_DATA_RECV);
#endif
}
