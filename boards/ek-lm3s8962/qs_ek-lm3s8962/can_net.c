//*****************************************************************************
//
// can_net.c - This is the portion of the quick start application for CAN.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "can_device_qs/can_common.h"
#include "audio.h"
#include "can_net.h"

//*****************************************************************************
//
// This holds the information for the data receive message object that is used
// to receive commands.
//
//*****************************************************************************
tCANMsgObject g_MsgObjectRx;

//*****************************************************************************
//
// This holds the information for the data send message object that is used
// to send commands and to send command responses.
//
//*****************************************************************************
tCANMsgObject g_MsgObjectTx;

//*****************************************************************************
//
// This holds the information for the LED message object that is used
// to transmit updates for the LED.  This message object transmits a single
// byte that indicates the brightness level for the LED on the target board.
//
//*****************************************************************************
tCANMsgObject g_MsgObjectLED;

//*****************************************************************************
//
// This holds the information for the button receive message object.  It is
// used to receive messages from the target board when button press and
// release events occur.  There are two buttons and two events(press/release).
//
//*****************************************************************************
tCANMsgObject g_MsgObjectButton;

//*****************************************************************************
//
// This is the message identifier used to transmit data to the host
// application board. The host application must use the message identifier
// specified by MSGOBJ_ID_DATA_0 to receive data successfully.
//
//*****************************************************************************
#define MSGOBJ_ID_DATA_TX       (MSGOBJ_ID_DATA_0)

//*****************************************************************************
//
// This is the message identifier used to receive data from the host
// application board. The host application must use the message identifier
// specified by MSGOBJ_ID_DATA_1 to transmit data successfully.
//
//*****************************************************************************
#define MSGOBJ_ID_DATA_RX       (MSGOBJ_ID_DATA_1)

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
#define MSGOBJ_NUM_LED          2

//*****************************************************************************
//
// This is the message object number used to transfer data.
//
//*****************************************************************************
#define MSGOBJ_NUM_DATA_TX      3

//*****************************************************************************
//
// This is the message object number used to receive data.
//
//*****************************************************************************
#define MSGOBJ_NUM_DATA_RX      4

//*****************************************************************************
//
// This flag is used by the g_ulFlags global variable to indicate that a
// request to update the LED brightness is being transmitted.  This flag will
// be cleared once the message has been sent.
//
//*****************************************************************************
#define FLAG_LED_TX_PEND        0x00000002

//*****************************************************************************
//
// This flag is used by the g_ulFlags global variable to indicate that a
// data transmission is in process and that no further commands or responses
// can be sent until this flag is cleared.  This flag will be cleared by the
// interrupt handler when the tramission has completed.
//
//*****************************************************************************
#define FLAG_DATA_TX_PEND       0x00000004

//*****************************************************************************
//
// This flag is used by the g_ulFlags global variable to indicate that data
// has been received and ready to be read.  The data  may either be a command
// or response to a command.  This flag will be cleared once the data has
// been processed.
//
//*****************************************************************************
#define FLAG_DATA_RECV          0x00000008

//*****************************************************************************
//
// This global holds the flags used to indicate the state of the message
// objects.
//
//*****************************************************************************
static volatile unsigned long g_ulFlags=0;

//*****************************************************************************
//
// This holds the constant that holds the firmware version for this
// application.
//
//*****************************************************************************
unsigned long const g_ulVersion = CURRENT_VERSION;

//*****************************************************************************
//
// This global is used by the button message object to store the events that
// are coming back from the target board.
//
//*****************************************************************************
static unsigned char g_pucButtonMsg[2];

//*****************************************************************************
//
// This value holds the current LED brightness level.
//
//*****************************************************************************
unsigned char g_ucLEDLevel=0;

//*****************************************************************************
//
// This function handles connection with the other CAN device and also
// handles incoming commands.
//
// /return None.
//
//*****************************************************************************
void
CANMain(void)
{
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
}

//*****************************************************************************
//
// This function sends a message to retrieve the firmware version from the
// target board.
//
//*****************************************************************************
int
CANGetTargetVersion(unsigned long *pulVersion)
{
    static unsigned char ucVerCmd = CMD_GET_VERSION;

    //
    // If there was already a previous message being transmitted then just
    // return.
    //
    if(g_ulFlags & FLAG_DATA_TX_PEND)
    {
        return(-1);
    }

    //
    // A transmit request is about to be pending.
    //
    g_ulFlags |= FLAG_DATA_TX_PEND;

    //
    // Send the button update request.
    //
    g_MsgObjectTx.pucMsgData = &ucVerCmd;
    g_MsgObjectTx.ulMsgLen = 1;

    CANMessageSet(CAN0_BASE, MSGOBJ_NUM_DATA_TX, &g_MsgObjectTx,
                  MSG_OBJ_TYPE_TX);

    //
    // Wait for some data back from the target.
    //
    while ((g_ulFlags & FLAG_DATA_RECV) == 0)
    {
    }

    //
    // Read the data from the message object.
    //
    g_MsgObjectRx.pucMsgData = (unsigned char *)pulVersion;
    g_MsgObjectRx.ulMsgLen = 4;
    CANMessageGet(CAN0_BASE, MSGOBJ_NUM_DATA_RX, &g_MsgObjectRx, 1);

    return(0);
}

//*****************************************************************************
//
// This function sends a message to set the current brightness for the LED on
// the target board.
//
//*****************************************************************************
void
CANUpdateTargetLED(unsigned char ucLevel, tBoolean bFlash)
{
    //
    // If there was already a previous message being transmitted then just
    // return.
    //
    if(g_ulFlags & FLAG_LED_TX_PEND)
    {
        return;
    }

    //
    // Set the global LED level.
    //
    g_ucLEDLevel = ucLevel;

    //
    // If a flash was requested then set the flag.
    //
    if(bFlash == true)
    {
        g_ucLEDLevel |= LED_FLASH_ONCE;
    }

    //
    // A transmit request is about to be pending.
    //
    g_ulFlags |= FLAG_LED_TX_PEND;

    //
    // Send the button update request.
    //
    CANMessageSet(CAN0_BASE, MSGOBJ_NUM_LED, &g_MsgObjectLED,
                  MSG_OBJ_TYPE_TX);
}

//*****************************************************************************
//
// This function configures the message objects used by this application.
// The following four message objects used by this application:
// MSGOBJ_ID_BUTTON, MSGOBJ_ID_LED, MSGOBJ_ID_DATA_TX, and MSGOBJ_ID_DATA_RX.
//
//*****************************************************************************
void
CANConfigureNetwork(void)
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
    // This message object will receive updates to the LED.
    //
    g_MsgObjectLED.ulMsgID = MSGOBJ_ID_LED;
    g_MsgObjectLED.ulMsgIDMask = 0;

    //
    // This enables interrupts for received messages.
    //
    g_MsgObjectLED.ulFlags = MSG_OBJ_TX_INT_ENABLE;

    //
    // Set the length of the message and the data buffer used.
    //
    g_MsgObjectLED.ulMsgLen = 1;
    g_MsgObjectLED.pucMsgData = &g_ucLEDLevel;

    //
    // This message object will transmit commands.
    //
    g_MsgObjectTx.ulMsgID = MSGOBJ_ID_DATA_TX;
    g_MsgObjectTx.ulMsgIDMask = 0;

    //
    // This enables interrupts for received messages.
    //
    g_MsgObjectTx.ulFlags = MSG_OBJ_TX_INT_ENABLE;

    //
    // The length of the message, which should only be one byte.  Don't set
    // the pointer until it is used.
    //
    g_MsgObjectTx.ulMsgLen = 1;
    g_MsgObjectTx.pucMsgData = (unsigned char *)0xffffffff;

    //
    // This message object will received data from commands.
    //
    g_MsgObjectRx.ulMsgID = MSGOBJ_ID_DATA_RX;
    g_MsgObjectRx.ulMsgIDMask = 0;

    //
    // This enables interrupts for received messages.
    //
    g_MsgObjectRx.ulFlags = MSG_OBJ_RX_INT_ENABLE;

    //
    // The length of the message, which should only be one byte.  Don't set
    // the pointer until it is used.
    //
    g_MsgObjectRx.ulMsgLen = 1;
    g_MsgObjectRx.pucMsgData = (unsigned char *)0xffffffff;

    //
    // Configure the data receive message object.
    //
    CANMessageSet(CAN0_BASE, MSGOBJ_NUM_DATA_RX, &g_MsgObjectRx,
                  MSG_OBJ_TYPE_RX);
}

//*****************************************************************************
//
// The CAN controller Interrupt handler.
//
//*****************************************************************************
void
CANHandler(void)
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
            // Read the Button Message.
            //
            CANMessageGet(CAN0_BASE, MSGOBJ_NUM_BUTTON,
                &g_MsgObjectButton, 1);

            //
            // Only respond to buttons being release.
            //
            if(g_MsgObjectButton.pucMsgData[0] == EVENT_BUTTON_RELEASED)
            {
                //
                // Check if the up button was released.
                //
                if(g_MsgObjectButton.pucMsgData[1] == TARGET_BUTTON_UP)
                {
                    //
                    // Adjust the volume up by 10.
                    //
                    AudioVolumeUp(10);
                }

                //
                // Check if the down button was released.
                //
                if(g_MsgObjectButton.pucMsgData[1] == TARGET_BUTTON_DN)
                {
                    //
                    // Adjust the volume down by 10.
                    //
                    AudioVolumeDown(10);
                }
            }
            break;
        }

        //
        // When the LED message object interrupts, just clear the flag so that
        // more LED messages are allowed to transfer.
        //
        case MSGOBJ_NUM_LED:
        {
            g_ulFlags &= (~FLAG_LED_TX_PEND);
            break;
        }

        //
        // When the transmit data message object interrupts, clear the
        // flag so that more data can be trasferred.
        //
        case MSGOBJ_NUM_DATA_TX:
        {
            g_ulFlags &= (~FLAG_DATA_TX_PEND);
            break;
        }

        //
        // When a receive data message object interrupts, set the flag to
        // indicate that new data is ready.
        //
        case MSGOBJ_NUM_DATA_RX:
        {
            g_ulFlags |= FLAG_DATA_RECV;
            break;
        }

        //
        // This was a status interrupt so read the current status to
        // clear the interrupt and return.
        //
        default:
        {
            //
            // Read the controller status to acknowledge this interrupt.
            //
            CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

            //
            // If there was a LED transmission pending, then stop it and
            // clear the flag.
            //
            if(g_ulFlags & FLAG_LED_TX_PEND)
            {
                //
                // Disable this message object until we retry it later.
                //
                CANMessageClear(CAN0_BASE, MSGOBJ_NUM_LED);

                //
                // Clear the transmit pending flag.
                //
                g_ulFlags &= (~FLAG_LED_TX_PEND);
            }

            //
            // If there was a Data transmission pending, then stop it and
            // clear the flag.
            //
            if(g_ulFlags & FLAG_DATA_TX_PEND)
            {
                //
                // Disable this message object until we retry it later.
                //
                CANMessageClear(CAN0_BASE, MSGOBJ_NUM_DATA_TX);

                //
                // Clear the transmit pending flag.
                //
                g_ulFlags &= (~FLAG_DATA_TX_PEND);
            }
            return;
        }
    }

    //
    // Acknowledge the CAN controller interrupt has been handled.
    //
    CANIntClear(CAN0_BASE, ulStatus);
}

//*****************************************************************************
//
// This function configures the CAN hardware and the message objects so that
// they are ready to use once the application returns from this function.
//
//*****************************************************************************
void
CANConfigure(void)
{
    //
    // Configure CAN Pins
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Enable the CAN controllers.
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
    CANConfigureNetwork();

    //
    // Enable interrupts for the CAN in the NVIC.
    //
    IntEnable(INT_CAN0);
}
