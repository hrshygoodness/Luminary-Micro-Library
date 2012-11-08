//*****************************************************************************
//
// multi_rx.c - Peripheral example demonstrating multiple CAN message
//              reception.
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "driverlib/can.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup can_examples_list
//! <h1>Multiple CAN RX (multi_rx)</h1>
//!
//! This example shows how to set up the CAN to receive multiple CAN messages
//! using separate message objects for different messages, and using CAN ID
//! filtering to control which messages are received.  Three message objects
//! are set up to receive 3 of the 4 CAN message IDs that are used by the
//! multi_tx example.  Filtering is used to demonstrate how to receive only
//! specific messages, and therefore not receiving all 4 messages from the
//! multi_tx example.  As messages are received the content of each are printed
//! to the serial console.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - CAN0 peripheral
//! - GPIO port D peripheral (for CAN0 pins)
//! - CAN0RX - PD0
//! - CAN0TX - PD1
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of CAN.
//! - GPIO port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - INT_CAN0 - CANIntHandler
//
//*****************************************************************************

//*****************************************************************************
//
// A counter that keeps track of the number of times the RX interrupt has
// occurred, which should match the number of messages that were received.
//
//*****************************************************************************
volatile unsigned long g_ulMsgCount = 0;

//*****************************************************************************
//
// A flag for the interrupt handler to indicate that a message was received.
//
//*****************************************************************************
volatile unsigned long g_bRXFlag1 = 0;
volatile unsigned long g_bRXFlag2 = 0;
volatile unsigned long g_bRXFlag3 = 0;

//*****************************************************************************
//
// A flag to indicate that some reception error occurred.
//
//*****************************************************************************
volatile unsigned long g_bErrFlag = 0;

//*****************************************************************************
//
// This function sets up UART0 to be used for a console to display information
// as the example is running.
//
//*****************************************************************************
void
InitConsole(void)
{
    //
    // Enable GPIO port A which is used for UART0 pins
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Configure the pin muxing for UART0 functions on port A0 and A1.
    // This step is not necessary if your part does not support pin muxing.
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    //
    // Select the alternate (UART) function for these pins.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioInit(0);
}

//*****************************************************************************
//
// This function prints some information about the CAN message to the
// serial port for information purposes only.
//
//*****************************************************************************
void
PrintCANMessageInfo(tCANMsgObject *pCANMsg, unsigned long ulMsgObj)
{
    unsigned int uIdx;

    //
    // Check to see if there is an indication that some messages were
    // lost.
    //
    if(pCANMsg->ulFlags & MSG_OBJ_DATA_LOST)
    {
        UARTprintf("CAN message loss detected on message object %d\n",
                   ulMsgObj);
    }

    //
    // Print out the contents of the message that was received.
    //
    UARTprintf("Msg Obj=%u ID=0x%05X len=%u data=0x", ulMsgObj,
               pCANMsg->ulMsgID, pCANMsg->ulMsgLen);
    for(uIdx = 0; uIdx < pCANMsg->ulMsgLen; uIdx++)
    {
        UARTprintf("%02X ", pCANMsg->pucMsgData[uIdx]);
    }
    UARTprintf("\n");
}

//*****************************************************************************
//
// This function is the interrupt handler for the CAN peripheral.  It checks
// for the cause of the interrupt, and maintains a count of all messages that
// have been received.
//
//*****************************************************************************
void
CANIntHandler(void)
{
    unsigned long ulStatus;

    //
    // Read the CAN interrupt status to find the cause of the interrupt
    //
    ulStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    //
    // If the cause is a controller status interrupt, then get the status
    //
    if(ulStatus == CAN_INT_INTID_STATUS)
    {
        //
        // Read the controller status.  This will return a field of status
        // error bits that can indicate various errors.  Error processing
        // is not done in this example for simplicity.  Refer to the
        // API documentation for details about the error status bits.
        // The act of reading this status will clear the interrupt.
        //
        ulStatus = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

        //
        // Set a flag to indicate some errors may have occurred.
        //
        g_bErrFlag = 1;
    }

    //
    // Check if the cause is message object 1.
    //
    else if(ulStatus == 1)
    {
        //
        // Getting to this point means that the RX interrupt occurred on
        // message object 1, and the message reception is complete.  Clear the
        // message object interrupt.
        //
        CANIntClear(CAN0_BASE, 1);

        //
        // Increment a counter to keep track of how many messages have been
        // received.  In a real application this could be used to set flags to
        // indicate when a message is received.
        //
        g_ulMsgCount++;

        //
        // Set flag to indicate received message is pending for this message
        // object.
        //
        g_bRXFlag1 = 1;

        //
        // Since a message was received, clear any error flags.
        //
        g_bErrFlag = 0;
    }

    //
    // Check if the cause is message object 2.
    //
    else if(ulStatus == 2)
    {
        CANIntClear(CAN0_BASE, 2);
        g_ulMsgCount++;
        g_bRXFlag2 = 1;
        g_bErrFlag = 0;
    }

    //
    // Check if the cause is message object 3.
    //
    else if(ulStatus == 3)
    {
        CANIntClear(CAN0_BASE, 3);
        g_ulMsgCount++;
        g_bRXFlag3 = 1;
        g_bErrFlag = 0;
    }

    //
    // Otherwise, something unexpected caused the interrupt.  This should
    // never happen.
    //
    else
    {
        //
        // Spurious interrupt handling can go here.
        //
    }
}

//*****************************************************************************
//
// Configure the CAN and enter a loop to receive CAN messages.
//
//*****************************************************************************
int
main(void)
{
    tCANMsgObject sCANMessage;
    unsigned char ucMsgData[8];

    //
    // Set the clocking to run directly from the external crystal/oscillator.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal used on your board.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for CAN operation.
    //
    InitConsole();

    //
    // For this example CAN0 is used with RX and TX pins on port D0 and D1.
    // The actual port and pins used may be different on your part, consult
    // the data sheet for more information.
    // GPIO port D needs to be enabled so these pins can be used.
    // TODO: change this to whichever GPIO port you are using
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Configure the GPIO pin muxing to select CAN0 functions for these pins.
    // This step selects which alternate function is available for these pins.
    // This is necessary if your part supports GPIO pin function muxing.
    // Consult the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using
    //
    GPIOPinConfigure(GPIO_PD0_CAN0RX);
    GPIOPinConfigure(GPIO_PD1_CAN0TX);

    //
    // Enable the alternate function on the GPIO pins.  The above step selects
    // which alternate function is available.  This step actually enables the
    // alternate function instead of GPIO for these pins.
    // TODO: change this to match the port/pin you are using
    //
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // The GPIO port and pins have been set up for CAN.  The CAN peripheral
    // must be enabled.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    //
    // Initialize the CAN controller
    //
    CANInit(CAN0_BASE);

    //
    // Set up the bit rate for the CAN bus.  This function sets up the CAN
    // bus timing for a nominal configuration.  You can achieve more control
    // over the CAN bus timing by using the function CANBitTimingSet() instead
    // of this one, if needed.
    // In this example, the CAN bus is set to 500 kHz.  In the function below,
    // the call to SysCtlClockGet() is used to determine the clock rate that
    // is used for clocking the CAN peripheral.  This can be replaced with a
    // fixed value if you know the value of the system clock, saving the extra
    // function call.  For some parts, the CAN peripheral is clocked by a fixed
    // 8 MHz regardless of the system clock in which case the call to
    // SysCtlClockGet() should be replaced with 8000000.  Consult the data
    // sheet for more information about CAN peripheral clocking.
    //
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    //
    // Enable interrupts on the CAN peripheral.  This example uses static
    // allocation of interrupt handlers which means the name of the handler
    // is in the vector table of startup code.  If you want to use dynamic
    // allocation of the vector table, then you must also call CANIntRegister()
    // here.
    //
    // CANIntRegister(CAN0_BASE, CANIntHandler); // if using dynamic vectors
    //
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);

    //
    // Enable the CAN interrupt on the processor (NVIC).
    //
    IntEnable(INT_CAN0);

    //
    // Enable the CAN for operation.
    //
    CANEnable(CAN0_BASE);

    //
    // Initialize a message object to receive CAN messages with ID 0x1001.
    // The expected ID must be set along with the mask to indicate that all
    // bits in the ID must match.
    //
    sCANMessage.ulMsgID = 0x1001;               // CAN msg ID
    sCANMessage.ulMsgIDMask = 0xfffff;          // mask, all 20 bits must match
    sCANMessage.ulFlags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER |
                          MSG_OBJ_EXTENDED_ID;
    sCANMessage.ulMsgLen = 8;                   // allow up to 8 bytes

    //
    // Now load the message object into the CAN peripheral message object 1.
    // Once loaded the CAN will receive any messages with this CAN ID into
    // this message object, and an interrupt will occur.
    //
    CANMessageSet(CAN0_BASE, 1, &sCANMessage, MSG_OBJ_TYPE_RX);

    //
    // Change the ID to 0x2001, and load into message object 2 which will be
    // used for receiving any CAN messages with this ID.  Since only the CAN
    // ID field changes, we don't need to reload all the other fields.
    //
    sCANMessage.ulMsgID = 0x2001;
    CANMessageSet(CAN0_BASE, 2, &sCANMessage, MSG_OBJ_TYPE_RX);

    //
    // Change the ID to 0x3001, and load into message object 3 which will be
    // used for receiving any CAN messages with this ID.  Since only the CAN
    // ID field changes, we don't need to reload all the other fields.
    //
    sCANMessage.ulMsgID = 0x3001;               // CAN msg ID
    CANMessageSet(CAN0_BASE, 3, &sCANMessage, MSG_OBJ_TYPE_RX);

    //
    // Enter loop to process received messages.  This loop just checks flags
    // for each of the 3 expected messages.  The flags are set by the interrupt
    // handler, and if set this loop reads out the message and displays the
    // contents to the console.  This is not a robust method for processing
    // incoming CAN data and can only handle one messages at a time per message
    // object.  If many messages are being received close together using the
    // same message object, then some messages may be dropped.  In a real
    // application, some other method should be used for queuing received
    // messages in a way to ensure they are not lost.  You can also make use
    // of CAN FIFO mode which will allow messages to be buffered before they
    // are processed.
    //
    for(;;)
    {
        //
        // If the flag for message object 1 is set, that means that the RX
        // interrupt occurred and there is a message ready to be read from
        // this CAN message object.
        //
        if(g_bRXFlag1)
        {
            //
            // Reuse the same message object that was used earlier to configure
            // the CAN for receiving messages.  A buffer for storing the
            // received data must also be provided, so set the buffer pointer
            // within the message object.  This same buffer is used for all
            // messages in this example, but your application could set a
            // different buffer each time a message is read in order to store
            // different messages in different buffers.
            //
            sCANMessage.pucMsgData = ucMsgData;

            //
            // Read the message from the CAN.  Message object number 1 is used
            // (which is not the same thing as CAN ID).  The interrupt clearing
            // flag is not set because this interrupt was already cleared in
            // the interrupt handler.
            //
            CANMessageGet(CAN0_BASE, 1, &sCANMessage, 0);

            //
            // Clear the pending message flag so that the interrupt handler can
            // set it again when the next message arrives.
            //
            g_bRXFlag1 = 0;

            //
            // Print information about the message just received.
            //
            PrintCANMessageInfo(&sCANMessage, 1);
        }

        //
        // Check for message received on message object 2.  If so then
        // read message and print information.
        //
        if(g_bRXFlag2)
        {
            sCANMessage.pucMsgData = ucMsgData;
            CANMessageGet(CAN0_BASE, 2, &sCANMessage, 0);
            g_bRXFlag2 = 0;
            PrintCANMessageInfo(&sCANMessage, 2);
        }

        //
        // Check for message received on message object 3.  If so then
        // read message and print information.
        //
        if(g_bRXFlag3)
        {
            sCANMessage.pucMsgData = ucMsgData;
            CANMessageGet(CAN0_BASE, 3, &sCANMessage, 0);
            g_bRXFlag3 = 0;
            PrintCANMessageInfo(&sCANMessage, 3);
        }
    }

    //
    // Return no errors
    //
    return(0);
}

