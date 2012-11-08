//*****************************************************************************
//
// can_device_qs.c - Target application that runs on the CAN device board and
//                   uses the CAN controller to communicate with the main
//                   board.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "can_common.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>CAN Device Board Quickstart Application (can_device_qs)</h1>
//!
//! This application uses the CAN controller to communicate with the evaluation
//! board that is running the example game.  It receives messages over CAN to
//! turn on, turn off, or to pulse the LED on the device board.  It also sends
//! CAN messages when either of the up and down buttons are pressed or
//! released.
//
//*****************************************************************************

//*****************************************************************************
//
// This is the message identifier used to receive data from the host
// application board. The host application must use the message identifier
// specified by MSGOBJ_ID_DATA_0 to transmit data successfully.
//
//*****************************************************************************
#define MSGOBJ_ID_DATA_RX       (MSGOBJ_ID_DATA_0)

//*****************************************************************************
//
// This is the message identifier used to transmit data to the host
// application board. The host application must use the message identifier
// specified by MSGOBJ_ID_DATA_1 to receive data successfully.
//
//*****************************************************************************
#define MSGOBJ_ID_DATA_TX       (MSGOBJ_ID_DATA_1)

//*****************************************************************************
//
// These are used by the button message object to send button events to the
// main board.
//
//*****************************************************************************
#define MSG_OBJ_B0_PRESSED      0x01
#define MSG_OBJ_B0_RELEASED     0x02
#define MSG_OBJ_B1_PRESSED      0x04
#define MSG_OBJ_B1_RELEASED     0x08

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
// to receive updates for the LED.  This message object receives a single
// byte that indicates the brightness level for the LED.
//
//*****************************************************************************
tCANMsgObject g_MsgObjectLED;

//*****************************************************************************
//
// This holds the information for the button request message object.  It is
// used to transmit the current status of the buttons on the target board.
// It does this by sending a single byte containing the bitmask of the
// buttons.
//
//*****************************************************************************
tCANMsgObject g_MsgObjectButton;

//*****************************************************************************
//
// The counter of the number of consecutive times that the buttons have
// remained constant.
//
//*****************************************************************************
static unsigned long g_ulDebounceCounter;

//*****************************************************************************
//
// The counter value used to turn off the led after receiving a command to
// pulse the LED.
//
//*****************************************************************************
static long g_lFlashCounter = -1;

//*****************************************************************************
//
// This variable holds the last stable raw button status.
//
//*****************************************************************************
volatile unsigned char g_ucButtonStatus;

//*****************************************************************************
//
// This variable holds flags indicating which buttons have been pressed and
// released.
//
//*****************************************************************************
volatile unsigned char g_ucButtonFlags = 0;

//*****************************************************************************
//
// This used to hold the message data for the button message object.
//
//*****************************************************************************
unsigned char g_pucButtonMsg[2];

//*****************************************************************************
//
// This value holds the current LED brightness level.
//
//*****************************************************************************
unsigned char g_ucLEDLevel = 0;

//*****************************************************************************
//
// This holds the constant that holds the firmware version for this
// application.
//
//*****************************************************************************
static unsigned long const g_ulVersion = CURRENT_VERSION;

//*****************************************************************************
//
// This global holds the flags used to indicate the state of the message
// objects.
//
//*****************************************************************************
static volatile unsigned long g_ulFlags = 0;

//*****************************************************************************
//
// This flag is used by the g_ulFlags global variable to indicate that a
// button response transmission is pending an that no more responses can be
// sent until this flag clears.  This flag will be cleared by the interrupt
// handler when the tramission has completed.
//
//*****************************************************************************
#define FLAG_BUTTON_PEND        0x00000001

//*****************************************************************************
//
// This flag is used by the g_ulFlags global variable to indicate that a
// request to update the LED brightness has been received and has been read
// into the g_MsgObjectLED structure.  This flag will be cleared once the
// brightness has been updated.
//
//*****************************************************************************
#define FLAG_UPDATE_LED         0x00000002

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
// This is the interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulStatus;
    unsigned char ucTemp;
    static unsigned long ulLastStatus = 0;

    //
    // Read the current value of the button pins.
    //
    ulStatus = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    if(ulStatus != ulLastStatus)
    {
        //
        // Something changed reset the compare and reset the debounce counter.
        //
        ulLastStatus = ulStatus;
        g_ulDebounceCounter = 0;
    }
    else
    {
        //
        // Increment the number of counts with the push button in a different
        // state.
        //
        g_ulDebounceCounter++;

        //
        // If four consecutive counts have had the push button in a different
        // state then the state has changed.
        //
        if(g_ulDebounceCounter == 4)
        {
            //
            // XOR to see what has changed.
            //
            ucTemp = g_ucButtonStatus ^ ulStatus;

            //
            // If something changed on button 0 then see what changed.
            //
            if(ucTemp & GPIO_PIN_0)
            {
                if(GPIO_PIN_0 & ulStatus)
                {
                    //
                    // Button released.
                    //
                    g_ucButtonFlags |= MSG_OBJ_B0_RELEASED;
                }
                else
                {
                    //
                    // Button pressed.
                    //
                    g_ucButtonFlags |= MSG_OBJ_B0_PRESSED;
                }
            }

            //
            // If something changed on button 1 then see what changed.
            //
            if(ucTemp & GPIO_PIN_1)
            {
                if(GPIO_PIN_1 & ulStatus)
                {
                    //
                    // Button released.
                    //
                    g_ucButtonFlags |= MSG_OBJ_B1_RELEASED;
                }
                else
                {
                    //
                    // Button pressed.
                    //
                    g_ucButtonFlags |= MSG_OBJ_B1_PRESSED;
                }
            }

            //
            // Save the new stable state for comparison.
            //
            g_ucButtonStatus = (unsigned char)ulStatus;
        }
    }

    //
    // Clear the LED if it is time.
    //
    if(g_lFlashCounter == 0)
    {
        //
        // Turn off LED.
        //
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
    }

    //
    // Let this go below just below zero so that the LED is not repeatedly
    // turned off.
    //
    if(g_lFlashCounter >= 0)
    {
        g_lFlashCounter--;
    }
}

//*****************************************************************************
//
// The CAN controller interrupt handler.
//
// /return None.
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
            // Indicate a pending button transmission is complete.
            //
            g_ulFlags &= (~FLAG_BUTTON_PEND);

            break;
        }

        case MSGOBJ_NUM_LED:
        {
            //
            // Read the new LED level and let the foreground handle it.
            //
            CANMessageGet(CAN0_BASE, MSGOBJ_NUM_LED, &g_MsgObjectLED, 1);

            //
            // Limit the LED Level to MAX_LED_BRIGHTNESS.
            //
            if((g_ucLEDLevel & LED_FLASH_VALUE_MASK) > MAX_LED_BRIGHTNESS)
            {
                g_ucLEDLevel = MAX_LED_BRIGHTNESS |
                               (g_ucLEDLevel & LED_FLASH_ONCE);
            }

            //
            // Indicate that the LED needs to be updated.
            //
            g_ulFlags |= FLAG_UPDATE_LED;

            break;
        }

        //
        // The data transmit message object has been sent successfully.
        //
        case MSGOBJ_NUM_DATA_TX:
        {
            //
            // Clear the data transmit pending flag.
            //
            g_ulFlags &= (~FLAG_DATA_TX_PEND);
            break;
        }

        //
        // The data receive message object has received some data.
        //
        case MSGOBJ_NUM_DATA_RX:
        {
            //
            // Indicate that the data message object has new data.
            //
            g_ulFlags |= FLAG_DATA_RECV;
            break;
        }

        //
        // This was a status interrupt so read the current status to
        // clear the interrupt and return.
        //
        default:
        {
            CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
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
// Configure CAN message objects for the application.
//
// This function configures the message objects used by this application.
// The following four message objects used by this application:
// MSGOBJ_ID_BUTTON, MSGOBJ_ID_LED, MSGOBJ_ID_DATA_TX, and MSGOBJ_ID_DATA_RX.
//
// /return None.
//
//*****************************************************************************
void
CANConfigureNetwork(void)
{
    //
    // This is the message object used to send button updates.  This message
    // object will not be "set" right now as that would trigger a transmission.
    //
    g_MsgObjectButton.ulMsgID = MSGOBJ_ID_BUTTON;
    g_MsgObjectButton.ulMsgIDMask = 0;

    //
    // This enables interrupts for transmitted messages.
    //
    g_MsgObjectButton.ulFlags = MSG_OBJ_TX_INT_ENABLE;

    //
    // Set the length of the message, which should only be two bytes and the
    // data is always whatever is in g_pucButtonMsg.
    //
    g_MsgObjectButton.ulMsgLen = 2;
    g_MsgObjectButton.pucMsgData = g_pucButtonMsg;

    //
    // This message object will receive updates for the LED brightness.
    //
    g_MsgObjectLED.ulMsgID = MSGOBJ_ID_LED;
    g_MsgObjectLED.ulMsgIDMask = 0;

    //
    // This enables interrupts for received messages.
    //
    g_MsgObjectLED.ulFlags = MSG_OBJ_RX_INT_ENABLE;

    //
    // The length of the message, which should only be one byte.
    //
    g_MsgObjectLED.ulMsgLen = 1;
    g_MsgObjectLED.pucMsgData = &g_ucLEDLevel;
    CANMessageSet(CAN0_BASE, MSGOBJ_NUM_LED, &g_MsgObjectLED,
                  MSG_OBJ_TYPE_RX);

    //
    // This message object will transmit commands and command responses.  It
    // will not be "set" right now as that would trigger a transmission.
    //
    g_MsgObjectTx.ulMsgID = MSGOBJ_ID_DATA_TX;
    g_MsgObjectTx.ulMsgIDMask = 0;

    //
    // This enables interrupts for transmitted messages.
    //
    g_MsgObjectTx.ulFlags = MSG_OBJ_TX_INT_ENABLE;

    //
    // The length of the message, which should only be one byte.  Don't set
    // the pointer until it is used.
    //
    g_MsgObjectTx.ulMsgLen = 1;
    g_MsgObjectTx.pucMsgData = (unsigned char *)0xffffffff;

    //
    // This message object will receive commands or data from commands.
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
    CANMessageSet(CAN0_BASE, MSGOBJ_NUM_DATA_RX, &g_MsgObjectRx,
                  MSG_OBJ_TYPE_RX);
}

//*****************************************************************************
//
// This functions sends out a button update message.
//
//*****************************************************************************
void
SendButtonMsg(unsigned char ucEvent, unsigned char ucButton)
{
    //
    // Set the flag to indicate that a button status is being sent.
    //
    g_ulFlags |= FLAG_BUTTON_PEND;

    //
    // Send the button status.
    //
    g_MsgObjectButton.pucMsgData[0] = ucEvent;
    g_MsgObjectButton.pucMsgData[1] = ucButton;

    CANMessageSet(CAN0_BASE, MSGOBJ_NUM_BUTTON, &g_MsgObjectButton,
                  MSG_OBJ_TYPE_TX);
}

//*****************************************************************************
//
// Handle any interrupts.
//
//*****************************************************************************
void
ProcessInterrupts(void)
{
    //
    // A request to set or clear the LED was received.
    //
    if(g_ulFlags & FLAG_UPDATE_LED)
    {
        //
        // Turn the LED on or off based on the request.
        //
        if((g_ucLEDLevel & LED_FLASH_VALUE_MASK) > 0)
        {
            //
            // Turn on LED.
            //
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
        }
        else
        {
            //
            // Turn off LED.
            //
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
        }

        //
        // If the flash once flag was set then start the count down.
        //
        if(g_ucLEDLevel & LED_FLASH_ONCE)
        {
            g_lFlashCounter = 10;
        }

        //
        // Clear the flag.
        //
        g_ulFlags &= ~(FLAG_UPDATE_LED);
    }

    //
    // If there is a button event pending then send it.
    //
    if(g_ucButtonFlags != 0)
    {
        //
        // If button zero was pressed, send the message.
        //
        if(g_ucButtonFlags & MSG_OBJ_B0_PRESSED)
        {
            SendButtonMsg(EVENT_BUTTON_PRESS, TARGET_BUTTON_UP);

            //
            // Clear the flag since this has been handled.
            //
            g_ucButtonFlags &= (~MSG_OBJ_B0_PRESSED);
        }

        //
        // If button zero was released, send the message.
        //
        if(g_ucButtonFlags & MSG_OBJ_B0_RELEASED)
        {
            SendButtonMsg(EVENT_BUTTON_RELEASED, TARGET_BUTTON_UP);

            //
            // Clear the flag since this has been handled.
            //
            g_ucButtonFlags &= (~MSG_OBJ_B0_RELEASED);
        }

        //
        // If button one was pressed, send the message.
        //
        if(g_ucButtonFlags & MSG_OBJ_B1_PRESSED)
        {
            SendButtonMsg(EVENT_BUTTON_PRESS, TARGET_BUTTON_DN);

            //
            // Clear the flag since this has been handled.
            //
            g_ucButtonFlags &= (~MSG_OBJ_B1_PRESSED);
        }

        //
        // If button one was released, send the message.
        //
        if(g_ucButtonFlags & MSG_OBJ_B1_RELEASED)
        {
            SendButtonMsg(EVENT_BUTTON_RELEASED, TARGET_BUTTON_DN);

            //
            // Clear the flag since this has been handled.
            //
            g_ucButtonFlags &= (~MSG_OBJ_B1_RELEASED);
        }
    }
}

//*****************************************************************************
//
// This function handles incoming commands.
//
//*****************************************************************************
void
ProcessCmd(void)
{
    unsigned char pucData[8];

    //
    // If no data has been received, then there is nothing to do.
    //
    if((g_ulFlags & FLAG_DATA_RECV) == 0)
    {
        return;
    }

    //
    // Receive the command.
    //
    g_MsgObjectRx.pucMsgData = pucData;
    g_MsgObjectRx.ulMsgLen = 8;
    CANMessageGet(CAN0_BASE, MSGOBJ_NUM_DATA_RX, &g_MsgObjectRx, 1);

    //
    // Clear the flag to indicate that the data has been read.
    //
    g_ulFlags &= (~FLAG_DATA_RECV);

    switch(g_MsgObjectRx.pucMsgData[0])
    {
        //
        // This is a request for the firmware version for this application.
        //
        case CMD_GET_VERSION:
        {
            //
            // Send the Version.
            //
            g_ulFlags |= FLAG_DATA_TX_PEND;

            g_MsgObjectTx.pucMsgData = (unsigned char *)&g_ulVersion;
            g_MsgObjectTx.ulMsgLen = 4;
            CANMessageSet(CAN0_BASE, MSGOBJ_NUM_DATA_TX,
                          &g_MsgObjectTx, MSG_OBJ_TYPE_TX);
            break;
        }
    }
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
    // Set the clocking to run directly from the PLL at 25MHz.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_8 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable the pull-ups on the JTAG signals.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPadConfigSet(GPIO_PORTC_BASE,
                     GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Configure CAN 0 Pins.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure GPIO Pins used for the Buttons.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

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
    // set to 250000.
    //
    CANBitRateSet(CAN0_BASE, 8000000, 250000);

    //
    // Take the CAN0 device out of INIT state.
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

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Configure SysTick for a 10ms interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / 100);
    SysTickEnable();
    SysTickIntEnable();

    //
    // Initialize the button status.
    //
    g_ucButtonStatus = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Forground handling of interrupts.
        //
        ProcessInterrupts();

        //
        // Handle any incoming commands.
        //
        ProcessCmd();
    }
}
