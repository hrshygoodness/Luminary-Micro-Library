//*****************************************************************************
//
// master_slave_loopback.c - Example demonstrating a simple I2C message
// transmission and reception.
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
#include "inc/hw_i2c.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup i2c_examples_list
//! <h1>I2C Master Loopback (i2c_master_slave_loopback)</h1>
//!
//! This example shows how to configure the I2C0 module for loopback mode.
//! This includes setting up the master and slave module.  Loopback mode
//! internally connects the master and slave data and clock lines together.
//! The address of the slave module is set in order to read data from the
//! master.  Then the data is checked to make sure the received data matches
//! the data that was transmitted.  This example uses a polling method for
//! sending and receiving data.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - I2C0 peripheral
//! - GPIO Port B peripheral (for I2C0 pins)
//! - I2C0SCL - PB2
//! - I2C0SDA - PB3
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of I2C.
//! - UART0 peripheral
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - None.
//!
//
//*****************************************************************************

//*****************************************************************************
//
// Number of I2C data packets to send.
//
//*****************************************************************************
#define NUM_I2C_DATA 3

//*****************************************************************************
//
// Set the address for slave module. This is a 7-bit address sent in the
// following format:
//                      [A6:A5:A4:A3:A2:A1:A0:RS]
//
// A zero in the "RS" position of the first byte means that the master
// transmits (sends) data to the selected slave, and a one in this position
// means that the master receives data from the slave.
//
//*****************************************************************************
#define SLAVE_ADDRESS 0x3C

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
// Configure the I2C0 master and slave and connect them using loopback mode.
//
//*****************************************************************************
int
main(void)
{
unsigned long ulDataTx[NUM_I2C_DATA];
unsigned long ulDataRx[NUM_I2C_DATA];
unsigned long ulindex;

    //
    // Set the clocking to run directly from the external crystal/oscillator.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal on your board.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // The I2C0 peripheral must be enabled before use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    //
    // For this example I2C0 is used with PortB[3:2].  The actual port and
    // pins used may be different on your part, consult the data sheet for
    // more information.  GPIO port B needs to be enabled so these pins can
    // be used.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    // This step is not necessary if your part does not support pin muxing.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    //
    // Select the I2C function for these pins.  This function will also
    // configure the GPIO pins pins for I2C operation, setting them to
    // open-drain operation with weak pull-ups.  Consult the data sheet
    // to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Enable loopback mode.  Loopback mode is a built in feature that is
    // useful for debugging I2C operations.  It internally connects the I2C
    // master and slave terminals, which effectively let's you send data as
    // a master and receive data as a slave.
    // NOTE: For external I2C operation you will need to use external pullups
    // that are stronger than the internal pullups.  Refer to the datasheet for
    // more information.
    //
    HWREG(I2C0_MASTER_BASE + I2C_O_MCR) |= 0x01;

    //
    // Enable and initialize the I2C0 master module.  Use the system clock for
    // the I2C0 module.  The last parameter sets the I2C data transfer rate.
    // If false the data rate is set to 100kbps and if true the data rate will
    // be set to 400kbps.  For this example we will use a data rate of 100kbps.
    //
    I2CMasterInitExpClk(I2C0_MASTER_BASE, SysCtlClockGet(), false);

    //
    // Enable the I2C0 slave module. This module is enabled only for testing
    // purposes.  It does not need to be enabled for proper operation of the
    // I2Cx master module.
    //
    I2CSlaveEnable(I2C0_SLAVE_BASE);

    //
    // Set the slave address to SLAVE_ADDRESS.  In loopback mode, it's an
    // arbitrary 7-bit number (set in a macro above) that is sent to the
    // I2CMasterSlaveAddrSet function.
    //
    I2CSlaveInit(I2C0_SLAVE_BASE, SLAVE_ADDRESS);

    //
    // Tell the master module what address it will place on the bus when
    // communicating with the slave.  Set the address to SLAVE_ADDRESS
    // (as set in the slave module).  The receive parameter is set to false
    // which indicates the I2C Master is initiating a writes to the slave.  If
    // true, that would indicate that the I2C Master is initiating reads from
    // the slave.
    //
    I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, SLAVE_ADDRESS, false);

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for I2C operation.
    //
    InitConsole();

    //
    // Display the example setup on the console.
    //
    UARTprintf("I2C Loopback Example ->");
    UARTprintf("\n   Module = I2C0");
    UARTprintf("\n   Mode = Single Send/Receive");
    UARTprintf("\n   Rate = 100kbps\n\n");

    //
    // Initalize the data to send.
    //
    ulDataTx[0] = 'I';
    ulDataTx[1] = '2';
    ulDataTx[2] = 'C';

    //
    // Initalize the receive buffer.
    //
    for(ulindex = 0; ulindex < NUM_I2C_DATA; ulindex++)
    {
        ulDataRx[ulindex] = 0;
    }

    //
    // Indicate the direction of the data.
    //
    UARTprintf("Tranferring from: Master -> Slave\n");

    //
    // Send 3 peices of I2C data from the master to the slave.
    //
    for(ulindex = 0; ulindex < NUM_I2C_DATA; ulindex++)
    {
        //
        // Display the data that the I2C0 master is transferring.
        //
        UARTprintf("  Sending: '%c'  . . .  ", ulDataTx[ulindex]);

        //
        // Place the data to be sent in the data register
        //
        I2CMasterDataPut(I2C0_MASTER_BASE, ulDataTx[ulindex]);

        //
        // Initiate send of data from the master.  Since the loopback
        // mode is enabled, the master and slave units are connected
        // allowing us to receive the same data that we sent out.
        //
        I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);

        //
        // Wait until the slave has received and acknowledged the data.
        //
        while(!(I2CSlaveStatus(I2C0_SLAVE_BASE) & I2C_SCSR_RREQ))
        {
        }

        //
        // Read the data from the slave.
        //
        ulDataRx[ulindex] = I2CSlaveDataGet(I2C0_SLAVE_BASE);

        //
        // Wait until master module is done transferring.
        //
        while(I2CMasterBusy(I2C0_MASTER_BASE))
        {
        }

        //
        // Display the data that the slave has received.
        //
        UARTprintf("Received: '%c'\n", ulDataRx[ulindex]);
    }

    //
    // Reset receive buffer.
    //
    for(ulindex = 0; ulindex < NUM_I2C_DATA; ulindex++)
    {
        ulDataRx[ulindex] = 0;
    }

    //
    // Indicate the direction of the data.
    //
    UARTprintf("\n\nTranferring from: Slave -> Master\n");

    //
    // Modifiy the data direction to true, so that seeing the address will
    // indicate that the I2C Master is initiating a read from the slave.
    //
    I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, SLAVE_ADDRESS, true);

    //
    // Do a dummy receive to make sure you don't get junk on the first receive.
    //
    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

    //
    // Dummy acknowledge and wait for the receive request from the master.
    // This is done to clear any flags that should not be set.
    //
    while(!(I2CSlaveStatus(I2C0_SLAVE_BASE) & I2C_SLAVE_ACT_TREQ))
    {
    }

    for(ulindex = 0; ulindex < NUM_I2C_DATA; ulindex++)
    {
        //
        // Display the data that I2C0 slave module is transferring.
        //
        UARTprintf("  Sending: '%c'  . . .  ", ulDataTx[ulindex]);

        //
        // Place the data to be sent in the data register
        //
        I2CSlaveDataPut(I2C0_SLAVE_BASE, ulDataTx[ulindex]);

        //
        // Tell the master to read data.
        //
        I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

        //
        // Wait until the slave is done sending data.
        //
        while(!(I2CSlaveStatus(I2C0_SLAVE_BASE) & I2C_SLAVE_ACT_TREQ))
        {
        }

        //
        // Read the data from the master.
        //
        ulDataRx[ulindex] = I2CMasterDataGet(I2C0_MASTER_BASE);

        //
        // Display the data that the slave has received.
        //
        UARTprintf("Received: '%c'\n", ulDataRx[ulindex]);
    }


    //
    // Tell the user that the test is done.
    //
    UARTprintf("\nDone.\n\n");

    //
    // Return no errors
    //
    return(0);
}
