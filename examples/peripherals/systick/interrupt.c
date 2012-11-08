//*****************************************************************************
//
// interrupt.c - Example demonstrating how to configure the systick interrupt.
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
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup systick_examples_list
//! <h1>Systick Interrupt (systick_int)</h1>
//!
//! This example shows how to configure the SysTick and the SysTick
//! interrupt.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - NONE
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of
//! Systick.
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
//
//*****************************************************************************

//*****************************************************************************
//
// Counter to count the number of interrupts that have been called.
//
//*****************************************************************************
unsigned long g_ulCounter = 0;

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
// The interrupt handler for the for Systick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update the Systick interrupt counter.
    //
    g_ulCounter++;
}

//*****************************************************************************
//
// Configure the SysTick and SysTick interrupt with a period of 1 second.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulPrevCount = 0;

    //
    // Set the clocking to run directly from the external crystal/oscillator.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal on your board.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for Systick operation.
    //
    InitConsole();

    //
    // Display the setup on the console.
    //
    UARTprintf("SysTick Firing Interrupt ->");
    UARTprintf("\n   Rate = 1sec\n\n");

    //
    // Initialize the interrupt counter.
    //
    g_ulCounter = 0;

    //
    // Set up the period for the SysTick timer.  The SysTick timer period will
    // be equal to the system clock, resulting in a period of 1 second.
    //
    SysTickPeriodSet(SysCtlClockGet());

    //
    // Enable interrupts to the processor.
    //
    IntMasterEnable();

    //
    // Enable the SysTick Interrupt.
    //
    SysTickIntEnable();

    //
    // Enable SysTick.
    //
    SysTickEnable();

    //
    // Loop forever while the SysTick runs.
    //
    while(1)
    {
        //
        // Check to see if systick interrupt count changed, and if so then
        // print a message with the count.
        //
        if(ulPrevCount != g_ulCounter)
        {
            //
            // Print the interrupt counter.
            //
            UARTprintf("Number of interrupts: %d\r", g_ulCounter);
            ulPrevCount = g_ulCounter;
        }
    }
}
