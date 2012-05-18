//*****************************************************************************
//
// soft_i2c_atmel.c - I2C master example.
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
// This is part of revision 8555 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "utils/softi2c.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup i2c_examples_list
//! <h1>SoftI2C AT24C08A EEPROM (soft_i2c_atmel)</h1>
//!
//! This example shows how to configure the SoftI2C module to read and write an
//! Atmel AT24C08A EEPROM.  A pattern is written into the first 16 bytes of the
//! EEPROM and then read back.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - Timer0 peripheral (for the SoftI2C timer)
//! - GPIO Port B peripheral (for SoftI2C pins)
//! - PB2 (for SCL)
//! - PB3 (for SDA)
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of I2C.
//! - UART0 peripheral
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application, you must add these interrupt handlers to your
//! vector table.
//! - INT_TIMER0A - Timer0AIntHandler
//
//*****************************************************************************

//*****************************************************************************
//
// The I2C slave address of the AT24C08A EEPROM device.  This address is based
// on the A2 pin of the AT24C08A being pulled high on the board.
//
//*****************************************************************************
#define SLAVE_ADDR              0x54

//*****************************************************************************
//
// The states in the interrupt handler state machine.
//
//*****************************************************************************
#define STATE_IDLE              0
#define STATE_WRITE_NEXT        1
#define STATE_WRITE_FINAL       2
#define STATE_WAIT_ACK          3
#define STATE_SEND_ACK          4
#define STATE_READ_ONE          5
#define STATE_READ_FIRST        6
#define STATE_READ_NEXT         7
#define STATE_READ_FINAL        8
#define STATE_READ_WAIT         9

//*****************************************************************************
//
// The state of the SoftI2C module.
//
//*****************************************************************************
static tSoftI2C g_sI2C;

//*****************************************************************************
//
// The variables that track the data to be transmitted or received.
//
//*****************************************************************************
static unsigned char *g_pucData = 0;
static unsigned long g_ulCount = 0;

//*****************************************************************************
//
// The current state of the interrupt handler state machine.
//
//*****************************************************************************
static volatile unsigned long g_ulState = STATE_IDLE;

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
    // Enable GPIO port A which is used for UART0 pins.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Configure the pin muxing for UART0 functions on port A0 and A1.
    // This step is not necessary if your part does not support pin muxing.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    //
    // Select the alternate (UART) function for these pins.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioInit(0);
}

//*****************************************************************************
//
// The callback function for the SoftI2C module.
//
//*****************************************************************************
void
SoftI2CCallback(void)
{
    //
    // Clear the SoftI2C interrupt.
    //
    SoftI2CIntClear(&g_sI2C);

    //
    // Determine what to do based on the current state.
    //
    switch(g_ulState)
    {
        //
        // The idle state.
        //
        case STATE_IDLE:
        {
            //
            // There is nothing to be done.
            //
            break;
        }

        //
        // The state for the middle of a burst write.
        //
        case STATE_WRITE_NEXT:
        {
            //
            // Write the next data byte.
            //
            SoftI2CDataPut(&g_sI2C, *g_pucData++);
            g_ulCount--;

            //
            // Continue the burst write.
            //
            SoftI2CControl(&g_sI2C, SOFTI2C_CMD_BURST_SEND_CONT);

            //
            // If there is one byte left, set the next state to the final write
            // state.
            //
            if(g_ulCount == 1)
            {
                g_ulState = STATE_WRITE_FINAL;
            }

            //
            // This state is done.
            //
            break;
        }

        //
        // The state for the final write of a burst sequence.
        //
        case STATE_WRITE_FINAL:
        {
            //
            // Write the final data byte.
            //
            SoftI2CDataPut(&g_sI2C, *g_pucData++);
            g_ulCount--;

            //
            // Finish the burst write.
            //
            SoftI2CControl(&g_sI2C, SOFTI2C_CMD_BURST_SEND_FINISH);

            //
            // The next state is to wait for the burst write to complete.
            //
            g_ulState = STATE_SEND_ACK;

            //
            // This state is done.
            //
            break;
        }

        //
        // Wait for an ACK on the read after a write.
        //
        case STATE_WAIT_ACK:
        {
            //
            // See if there was an error on the previously issued read.
            //
            if(SoftI2CErr(&g_sI2C) == SOFTI2C_ERR_NONE)
            {
                //
                // Read the byte received.
                //
                SoftI2CDataGet(&g_sI2C);

                //
                // There was no error, so the state machine is now idle.
                //
                g_ulState = STATE_IDLE;

                //
                // This state is done.
                //
                break;
            }

            //
            // Fall through to STATE_SEND_ACK.
            //
        }

        //
        // Send a read request, looking for the ACK to indicate that the write
        // is done.
        //
        case STATE_SEND_ACK:
        {
            //
            // Put the I2C master into receive mode.
            //
            SoftI2CSlaveAddrSet(&g_sI2C, SLAVE_ADDR, true);

            //
            // Perform a single byte read.
            //
            SoftI2CControl(&g_sI2C, SOFTI2C_CMD_SINGLE_RECEIVE);

            //
            // The next state is the wait for the ack.
            //
            g_ulState = STATE_WAIT_ACK;

            //
            // This state is done.
            //
            break;
        }

        //
        // The state for a single byte read.
        //
        case STATE_READ_ONE:
        {
            //
            // Put the SoftI2C module into receive mode.
            //
            SoftI2CSlaveAddrSet(&g_sI2C, SLAVE_ADDR, true);

            //
            // Perform a single byte read.
            //
            SoftI2CControl(&g_sI2C, SOFTI2C_CMD_SINGLE_RECEIVE);

            //
            // The next state is the wait for final read state.
            //
            g_ulState = STATE_READ_WAIT;

            //
            // This state is done.
            //
            break;
        }

        //
        // The state for the start of a burst read.
        //
        case STATE_READ_FIRST:
        {
            //
            // Put the SoftI2C module into receive mode.
            //
            SoftI2CSlaveAddrSet(&g_sI2C, SLAVE_ADDR, true);

            //
            // Start the burst receive.
            //
            SoftI2CControl(&g_sI2C, SOFTI2C_CMD_BURST_RECEIVE_START);

            //
            // The next state is the middle of the burst read.
            //
            g_ulState = STATE_READ_NEXT;

            //
            // This state is done.
            //
            break;
        }

        //
        // The state for the middle of a burst read.
        //
        case STATE_READ_NEXT:
        {
            //
            // Read the received character.
            //
            *g_pucData++ = SoftI2CDataGet(&g_sI2C);
            g_ulCount--;

            //
            // Continue the burst read.
            //
            SoftI2CControl(&g_sI2C, SOFTI2C_CMD_BURST_RECEIVE_CONT);

            //
            // If there are two characters left to be read, make the next
            // state be the end of burst read state.
            //
            if(g_ulCount == 2)
            {
                g_ulState = STATE_READ_FINAL;
            }

            //
            // This state is done.
            //
            break;
        }

        //
        // The state for the end of a burst read.
        //
        case STATE_READ_FINAL:
        {
            //
            // Read the received character.
            //
            *g_pucData++ = SoftI2CDataGet(&g_sI2C);
            g_ulCount--;

            //
            // Finish the burst read.
            //
            SoftI2CControl(&g_sI2C, SOFTI2C_CMD_BURST_RECEIVE_FINISH);

            //
            // The next state is the wait for final read state.
            //
            g_ulState = STATE_READ_WAIT;

            //
            // This state is done.
            //
            break;
        }

        //
        // This state is for the final read of a single or burst read.
        //
        case STATE_READ_WAIT:
        {
            //
            // Read the received character.
            //
            *g_pucData++ = SoftI2CDataGet(&g_sI2C);
            g_ulCount--;

            //
            // The state machine is now idle.
            //
            g_ulState = STATE_IDLE;

            //
            // This state is done.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// Write to the Atmel device.
//
//*****************************************************************************
void
AtmelWrite(unsigned char *pucData, unsigned long ulOffset,
           unsigned long ulCount)
{
    //
    // Save the data buffer to be written.
    //
    g_pucData = pucData;
    g_ulCount = ulCount;

    //
    // Set the next state of the callback state machine based on the number of
    // bytes to write.
    //
    if(ulCount != 1)
    {
        g_ulState = STATE_WRITE_NEXT;
    }
    else
    {
        g_ulState = STATE_WRITE_FINAL;
    }

    //
    // Set the slave address and setup for a transmit operation.
    //
    SoftI2CSlaveAddrSet(&g_sI2C, SLAVE_ADDR | (ulOffset >> 8), false);

    //
    // Write the address to be written as the first data byte.
    //
    SoftI2CDataPut(&g_sI2C, ulOffset);

    //
    // Start the burst cycle, writing the address as the first byte.
    //
    SoftI2CControl(&g_sI2C, SOFTI2C_CMD_BURST_SEND_START);

    //
    // Wait until the SoftI2C callback state machine is idle.
    //
    while(g_ulState != STATE_IDLE)
    {
    }
}

//*****************************************************************************
//
// Read from the Atmel device.
//
//*****************************************************************************
void
AtmelRead(unsigned char *pucData, unsigned long ulOffset,
          unsigned long ulCount)
{
    //
    // Save the data buffer to be read.
    //
    g_pucData = pucData;
    g_ulCount = ulCount;

    //
    // Set the next state of the callback state machine based on the number of
    // bytes to read.
    //
    if(ulCount == 1)
    {
        g_ulState = STATE_READ_ONE;
    }
    else
    {
        g_ulState = STATE_READ_FIRST;
    }

    //
    // Start with a dummy write to get the address set in the EEPROM.
    //
    SoftI2CSlaveAddrSet(&g_sI2C, SLAVE_ADDR | (ulOffset >> 8), false);

    //
    // Write the address to be written as the first data byte.
    //
    SoftI2CDataPut(&g_sI2C, ulOffset);

    //
    // Perform a single send, writing the address as the only byte.
    //
    SoftI2CControl(&g_sI2C, SOFTI2C_CMD_SINGLE_SEND);

    //
    // Wait until the SoftI2C callback state machine is idle.
    //
    while(g_ulState != STATE_IDLE)
    {
    }
}

//*****************************************************************************
//
// This is the interrupt handler for the Timer0A interrupt.
//
//*****************************************************************************
void
Timer0AIntHandler(void)
{
    //
    // Clear the timer interrupt.
    // TODO: change this to whichever timer you are using.
    //
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Call the SoftI2C tick function.
    //
    SoftI2CTimerTick(&g_sI2C);
}

//*****************************************************************************
//
// This example demonstrates the use of the SoftI2C module to read and write an
// Atmel AT24C08A EEPROM.
//
//*****************************************************************************
int
main(void)
{
    unsigned char pucData[16];
    unsigned long ulIdx;

    //
    // Set the clocking to run directly from the external crystal/oscillator.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal on your board.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // For this example, PortB[3:2] are used for the SoftI2C pins.  GPIO port B
    // needs to be enabled so these pins can be used.
    // TODO: change this to whichever GPIO port(s) you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // For this example, Timer0 is used for the SoftI2C time base.  This timer
    // needs to be enabled before it can be used.
    // TODO: change this to whichever timer you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    //
    // Configure the appropriate pins to be I2C instead of GPIO.
    //
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Initialize the SoftI2C module, including the assignment of GPIO pins.
    // TODO: change this to whichever GPIO pins you are using.
    //
    memset(&g_sI2C, 0, sizeof(g_sI2C));
    SoftI2CCallbackSet(&g_sI2C, SoftI2CCallback);
    SoftI2CSCLGPIOSet(&g_sI2C, GPIO_PORTB_BASE, GPIO_PIN_2);
    SoftI2CSDAGPIOSet(&g_sI2C, GPIO_PORTB_BASE, GPIO_PIN_3);
    SoftI2CInit(&g_sI2C);

    //
    // Enable the SoftI2C interrupt.
    //
    SoftI2CIntEnable(&g_sI2C);

    //
    // Configure the timer to generate an interrupt at a rate of 40 KHz.  This
    // will result in a I2C rate of 10 KHz.
    // TODO: change this to whichever timer you are using.
    // TODO: change this to whichever I2C rate you require.
    //
    TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 40000);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);

    //
    // Enable the timer interrupt.
    // TODO: change this to whichever timer interrupt you are using.
    //
    IntEnable(INT_TIMER0A);

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for SoftI2C operation.
    //
    InitConsole();

    //
    // Display the example setup on the console.
    //
    UARTprintf("SoftI2C Atmel AT24C08A example\n");

    //
    // Write a data=address pattern into the first 16 bytes of the Atmel
    // device.
    //
    UARTprintf("Write:");
    for(ulIdx = 0; ulIdx < 16; ulIdx++)
    {
        pucData[ulIdx] = ulIdx;
        UARTprintf(" %02x", pucData[ulIdx]);
    }
    UARTprintf("\n");
    AtmelWrite(pucData, 0, 16);

    //
    // Read back the first 16 bytes of the Atmel device.
    //
    AtmelRead(pucData, 0, 16);
    UARTprintf("Read :");
    for(ulIdx = 0; ulIdx < 16; ulIdx++)
    {
        UARTprintf(" %02x", pucData[ulIdx]);
    }
    UARTprintf("\n");

    //
    // Tell the user that the test is done.
    //
    UARTprintf("Done.\n\n");

    //
    // Return no errors.
    //
    return(0);
}
