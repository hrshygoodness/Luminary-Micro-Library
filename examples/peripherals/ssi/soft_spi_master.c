//*****************************************************************************
//
// soft_spi_master.c - Example demonstrating how to configure SoftSSI.
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
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/softssi.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup ssi_examples_list
//! <h1>SoftSSI Master (soft_spi_master)</h1>
//!
//! This example shows how to configure the SoftSSI module.  The code will send
//! three characters on the master Tx then polls the receive FIFO until 3
//! characters are received on the master Rx.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - GPIO Port A peripheral (for SoftSSI pins)
//! - SoftSSICLK - PA2
//! - SoftSSIFss - PA3
//! - SoftSSIRx  - PA4
//! - SoftSSITx  - PA5
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of
//! SoftSSI.
//! - UART0 peripheral
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - SysTickIntHandler
//!
//! \note This example provide the same functionality using the same pins as
//! the spi_master example.  As such, it can be used as a guide for how to
//! convert code which uses hardware SSI to the SoftSSI module.
//
//*****************************************************************************

//*****************************************************************************
//
// Number of bytes to send and receive.
//
//*****************************************************************************
#define NUM_SSI_DATA            3

//*****************************************************************************
//
// The persistent state of the SoftSSI peripheral.
//
//*****************************************************************************
tSoftSSI g_sSoftSSI;

//*****************************************************************************
//
// The data buffer that is used as the transmit FIFO.  The size of this buffer
// can be increased or decreased as required to match the transmit buffering
// requirements of your application.
//
//*****************************************************************************
unsigned short g_pusTxBuffer[16];

//*****************************************************************************
//
// The data buffer that is used as the receive FIFO.  The size of this buffer
// can be increased or decreased as required to match the receive buffering
// requirements of your application.
//
//*****************************************************************************
unsigned short g_pusRxBuffer[16];

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
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the SoftSSI timer tick.
    //
    SoftSSITimerTick(&g_sSoftSSI);
}

//*****************************************************************************
//
// Configure SoftSSI in SPI mode 0.  This example will send out 3 bytes of
// data, then wait for 3 bytes of data to come in.  This will all be done using
// the polling method.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulDataTx[NUM_SSI_DATA];
    unsigned long ulDataRx[NUM_SSI_DATA];
    unsigned long ulindex;

    //
    // Set the clocking to run directly from the external crystal/oscillator.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal on your board.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for SSI operation.
    //
    InitConsole();

    //
    // Display the setup on the console.
    //
    UARTprintf("SoftSSI ->\n");
    UARTprintf("  Data: 8-bit\n\n");

    //
    // For this example SoftSSI is used with PortA[5:2].  The actual port and
    // pins used may be different on your design based on the set of GPIO pins
    // available.  GPIO port A needs to be enabled so these pins can be used.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Configure the SoftSSI module.  The size of the FIFO buffers can be
    // changed to accommodate the requirements of your application.  The GPIO
    // pins utilized can also be changed.
    // The pins are assigned as follows:
    //      PA2 - SoftSSICLK
    //      PA3 - SoftSSIFss
    //      PA4 - SoftSSIRx
    //      PA5 - SoftSSITx
    // TODO: change this to select the port/pin you are using.
    // TODO: change the buffer sizes to match your requirements.
    //
    memset(&g_sSoftSSI, 0, sizeof(g_sSoftSSI));
    SoftSSIClkGPIOSet(&g_sSoftSSI, GPIO_PORTA_BASE, GPIO_PIN_2);
    SoftSSIFssGPIOSet(&g_sSoftSSI, GPIO_PORTA_BASE, GPIO_PIN_3);
    SoftSSIRxGPIOSet(&g_sSoftSSI, GPIO_PORTA_BASE, GPIO_PIN_4);
    SoftSSITxGPIOSet(&g_sSoftSSI, GPIO_PORTA_BASE, GPIO_PIN_5);
    SoftSSIRxBufferSet(&g_sSoftSSI, g_pusRxBuffer,
                       sizeof(g_pusRxBuffer) / sizeof(g_pusRxBuffer[0]));
    SoftSSITxBufferSet(&g_sSoftSSI, g_pusTxBuffer,
                       sizeof(g_pusTxBuffer) / sizeof(g_pusTxBuffer[0]));

    //
    // Configure the SoftSSI module.  Use idle clock level low and active low
    // clock (mode 0) and 8-bit data.  You can set the polarity of the SoftSSI
    // clock when the SoftSSI module is idle.  You can also configure what
    // clock edge you want to capture data on.  Please reference the datasheet
    // for more information on the different SPI modes.
    //
    SoftSSIConfigSet(&g_sSoftSSI, SOFTSSI_FRF_MOTO_MODE_0, 8);

    //
    // Enable the SoftSSI module.
    //
    SoftSSIEnable(&g_sSoftSSI);

    //
    // Configure SysTick to provide an interrupt at a 10 KHz rate.  This is
    // used to control the clock rate of the SoftSSI module; the clock rate of
    // the SoftSSI Clk signal will be 1/2 the interrupt rate.
    // TODO: change this to a different timer if SysTick is not available.
    // TODO: change the interrupt rate to adjust the SoftSSI clock rate.
    //
    SysTickPeriodSet(SysCtlClockGet() / 10000);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Initialize the data to send.
    //
    ulDataTx[0] = 's';
    ulDataTx[1] = 'p';
    ulDataTx[2] = 'i';

    //
    // Display indication that the SoftSSI is transmitting data.
    //
    UARTprintf("Sent:\n  ");

    //
    // Send 3 bytes of data.
    //
    for(ulindex = 0; ulindex < NUM_SSI_DATA; ulindex++)
    {
        //
        // Display the data that SSI is transferring.
        //
        UARTprintf("'%c' ", ulDataTx[ulindex]);

        //
        // Send the data using the "blocking" put function.  This function
        // will wait until there is room in the send FIFO before returning.
        // This allows you to assure that all the data you send makes it into
        // the send FIFO.
        //
        SoftSSIDataPut(&g_sSoftSSI, ulDataTx[ulindex]);
    }

    //
    // Wait until SoftSSI is done transferring all the data in the transmit
    // FIFO.
    //
    while(SoftSSIBusy(&g_sSoftSSI))
    {
    }

    //
    // Display indication that the SoftSSI is receiving data.
    //
    UARTprintf("\nReceived:\n  ");

    //
    // Receive 3 bytes of data.
    //
    for(ulindex = 0; ulindex < NUM_SSI_DATA; ulindex++)
    {
        //
        // Receive the data using the "blocking" Get function. This function
        // will wait until there is data in the receive FIFO before returning.
        //
        SoftSSIDataGet(&g_sSoftSSI, &ulDataRx[ulindex]);

        //
        // Since we are using 8-bit data, mask off the MSB.
        //
        ulDataRx[ulindex] &= 0x00FF;

        //
        // Display the data that SoftSSI received.
        //
        UARTprintf("'%c' ", ulDataRx[ulindex]);
    }


    //
    // Return no errors
    //
    return(0);
}
