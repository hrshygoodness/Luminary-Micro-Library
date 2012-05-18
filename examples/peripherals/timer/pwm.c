//*****************************************************************************
//
// pwm.c - Example demonstrating timer-based PWM on a 16-bit CCP.
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

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup timer_examples_list
//! <h1>PWM using Timer (pwm)</h1>
//!
//! This example shows how to configure Timer1B to generate a PWM signal on the
//! timer's CCP pin.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - TIMER1 peripheral
//! - GPIO Port E peripheral (for CCP3 pin)
//! - CCP3 - PE4
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
//! - None.
//!
//
//*****************************************************************************

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
// Prints out 5x "." with a second delay after each print.  This function will
// then backspace, clear the previously printed dots, backspace again so you
// continuously printout on the same line.
//
//*****************************************************************************
void
PrintRunningDots(void)
{
    UARTprintf(". ");
    SysCtlDelay(SysCtlClockGet() / 3);
    UARTprintf(". ");
    SysCtlDelay(SysCtlClockGet() / 3);
    UARTprintf(". ");
    SysCtlDelay(SysCtlClockGet() / 3);
    UARTprintf(". ");
    SysCtlDelay(SysCtlClockGet() / 3);
    UARTprintf(". ");
    SysCtlDelay(SysCtlClockGet() / 3);
    UARTprintf("\b\b\b\b\b\b\b\b\b\b");
    UARTprintf("          ");
    UARTprintf("\b\b\b\b\b\b\b\b\b\b");
    SysCtlDelay(SysCtlClockGet() / 3);
}

//*****************************************************************************
//
// Configure Timer1B as a 16-bit PWM with a duty cycle of 66%.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run directly from the external crystal/oscillator.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal on your board.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // The Timer1 peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    //
    // For this example CCP3 is used with port E pin 4.
    // The actual port and pins used may be different on your part, consult
    // the data sheet for more information.
    // GPIO port E needs to be enabled so these pins can be used.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    //
    // Configure the GPIO pin muxing for the Timer/CCP function.
    // This is only necessary if your part supports GPIO pin function muxing.
    // Study the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using
    //
    GPIOPinConfigure(GPIO_PE4_CCP3);

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for Timer/PWM operation.
    //
    InitConsole();

    //
    // Configure the ccp settings for CCP pin.  This function also gives
    // control of these pins to the SSI hardware.  Consult the data sheet to
    // see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypeTimer(GPIO_PORTE_BASE, GPIO_PIN_4);

    //
    // Display the example setup on the console.
    //
    UARTprintf("16-Bit Timer PWM ->");
    UARTprintf("\n   Timer = Timer1B");
    UARTprintf("\n   Mode = PWM");
    UARTprintf("\n   Duty Cycle = 66%%\n");
    UARTprintf("\nGenerating PWM on CCP3 (PE4) -> ");

    //
    // Configure Timer1B as a 16-bit periodic timer.
    //
    TimerConfigure(TIMER1_BASE, TIMER_CFG_16_BIT_PAIR |
                   TIMER_CFG_B_PWM);

    //
    // Set the Timer1B load value to 50000.  For this example a 66% duty
    // cycle PWM signal will be generated.  From the load value (i.e. 50000)
    // down to match value (set below) the signal will be high.  From the
    // match value to 0 the timer will be low.
    //
    TimerLoadSet(TIMER1_BASE, TIMER_B, 50000);

    //
    // Set the Timer1B match value to load value / 3.
    //
    TimerMatchSet(TIMER1_BASE, TIMER_B, TimerLoadGet(TIMER1_BASE, TIMER_B) / 3);

    //
    // Enable Timer1B.
    //
    TimerEnable(TIMER1_BASE, TIMER_B);

    //
    // Loop forever while the Timer1B PWM runs.
    //
    while(1)
    {
        //
        // Print out indication on the console that the program is running.
        //
        PrintRunningDots();
    }
}
