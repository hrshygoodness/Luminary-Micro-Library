//*****************************************************************************
//
// periodic_16bit.c - Example demonstrating a periodic 16-bit timer.
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
#include "inc/hw_timer.h"
#include "inc/hw_ints.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup timer_examples_list
//! <h1>16-Bit Periodic Timer (periodic_16bit)</h1>
//!
//! This example shows how to configure Timer0B as a periodic timer with an
//! interrupt triggering every 1ms.  After a certain number of interrupts, the
//! Timer0B interrupt will be disabled.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - TIMER0 peripheral
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of
//! Timer0.
//! - UART0 peripheral
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - INT_TIMER0B - Timer0BIntHandler
//!
//
//*****************************************************************************

//*****************************************************************************
//
// Number of interrupts before the timer gets turned off.
//
//*****************************************************************************
#define NUMBER_OF_INTS 1000

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
// The interrupt handler for the Timer0B interrupt.
//
//*****************************************************************************
void
Timer0BIntHandler(void)
{
    //
    // Clear the timer interrupt flag.
    //
    TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

    //
    // Update the periodic interrupt counter.
    //
    g_ulCounter++;

    //
    // Once NUMBER_OF_INTS interrupts have been received, turn off the
    // TIMER0B interrupt.
    //
    if(g_ulCounter == NUMBER_OF_INTS)
    {
        //
        // Disable the Timer0B interrupt.
        //
        IntDisable(INT_TIMER0B);

        //
        // Turn off Timer0B interrupt.
        //
        TimerIntDisable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

        //
        // Clear any pending interrupt flag.
        //
        TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    }
}

//*****************************************************************************
//
// Configure Timer0B as a 16-bit periodic counter with an interrupt
// every 1ms.
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
    // The Timer0 peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for Timer operation.
    //
    InitConsole();

    //
    // Display the example setup on the console.
    //
    UARTprintf("16-Bit Timer Interrupt ->");
    UARTprintf("\n   Timer = Timer0B");
    UARTprintf("\n   Mode = Periodic");
    UARTprintf("\n   Number of interrupts = %d", NUMBER_OF_INTS);
    UARTprintf("\n   Rate = 1ms\n\n");

    //
    // Configure Timer0B as a 16-bit periodic timer.
    //
    TimerConfigure(TIMER0_BASE, TIMER_CFG_16_BIT_PAIR |
                   TIMER_CFG_B_PERIODIC);

    //
    // Set the Timer0B load value to 1ms.
    //
    TimerLoadSet(TIMER0_BASE, TIMER_B, SysCtlClockGet() / 1000);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Configure the Timer0B interrupt for timer timeout.
    //
    TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

    //
    // Enable the Timer0B interrupt on the processor (NVIC).
    //
    IntEnable(INT_TIMER0B);

    //
    // Initialize the interrupt counter.
    //
    g_ulCounter = 0;

    //
    // Enable Timer0B.
    //
    TimerEnable(TIMER0_BASE, TIMER_B);

    //
    // Loop forever while the Timer0B runs.
    //
    while(1)
    {
        //
        // If the interrupt count changed, print the new value
        //
        if(ulPrevCount != g_ulCounter)
        {
            //
            // Print the periodic interrupt counter.
            //
            UARTprintf("Number of interrupts: %d\r", g_ulCounter);
            ulPrevCount = g_ulCounter;
        }
    }
}
