//*****************************************************************************
//
// reload_interrupt.c - Example demonstrating the PWM interrupt.
//
// Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
// 
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the  
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup pwm_examples_list
//! <h1>PWM Reload Interrupt (reload_interrupt)</h1>
//!
//! This example shows how to setup an interrupt on PWM0.  This example
//! demonstrates how to setup an interrupt on the PWM when the PWM timer is
//! equal to the configurable PWM0LOAD register.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - GPIO Port D peripheral (for PWM0 pin)
//! - PWM0 - PD0
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of the
//! PWM.
//! - UART0 peripheral
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - INT_PWM0 - PWM0IntHandler
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
// continuously printout on the same line.  The purpose of this function is to
// indicate to the user that the program is running.
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
// The interrupt handler for the for PWM0 interrupts.
//
//*****************************************************************************
void
PWM0IntHandler(void)
{
    //
    // Clear the PWM0 LOAD interrupt flag.  This flag gets set when the PWM
    // counter gets reloaded.
    //
    PWMGenIntClear(PWM_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);

    //
    // If the duty cycle is less or equal to 75% then add 0.1% to the duty
    // cycle.  Else, reset the duty cycle to 0.1% cycles.  Note that 64 is
    // 0.01% of the period (64000 cycles).
    //
    if((PWMPulseWidthGet(PWM_BASE, PWM_OUT_0) + 64) <=
       ((PWMGenPeriodGet(PWM_BASE, PWM_GEN_0) * 3) / 4))
    {
        PWMPulseWidthSet(PWM_BASE, PWM_OUT_0,
                         PWMPulseWidthGet(PWM_BASE, PWM_OUT_0) + 64);
    }
    else
    {
        PWMPulseWidthSet(PWM_BASE, PWM_OUT_0, 64);
    }
}

//*****************************************************************************
//
// Configure PWM0 for a load interrupt.  This interrupt will trigger everytime
// the PWM0 counter gets reloaded.  In the interrupt, 0.1% will be added to
// the current duty cycle.  This will continue until a duty cycle of 75% is
// received, then the duty cycle will get reset to 0.1%.
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
    // Set the PWM clock to the system clock.
    //
    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for PWM0 operation.
    //
    InitConsole();

    //
    // Display the setup on the console.
    //
    UARTprintf("PWM ->\n");
    UARTprintf("  Module: PWM0\n");
    UARTprintf("  Pin: PD0\n");
    UARTprintf("  Duty Cycle: Variable -> ");
    UARTprintf("0.1%% to 75%% in 0.1%% increments.\n");
    UARTprintf("  Features: ");
    UARTprintf("Variable pulse-width done using a reload interrupt.\n\n");
    UARTprintf("Generating PWM on PWM0 (PD0) -> ");

    //
    // The PWM peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM);

    //
    // For this example PWM0 is used with PortD Pin0.  The actual port and pins
    // used may be different on your part, consult the data sheet for more
    // information.  GPIO port D needs to be enabled so these pins can be used.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Configure the GPIO pin muxing to select PWM00 functions for these pins.
    // This step selects which alternate function is available for these pins.
    // This is necessary if your part supports GPIO pin function muxing.
    // Consult the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinConfigure(GPIO_PD0_PWM0);

    //
    // Configure the PWM function for this pin.
    // Consult the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);

    //
    // Configure the PWM0 to count down without synchronization.
    //
    PWMGenConfigure(PWM_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN |
                    PWM_GEN_MODE_NO_SYNC);

    //
    // Set the PWM period to 250Hz.  To calculate the appropriate parameter
    // use the following equation: N = (1 / f) * SysClk.  Where N is the
    // function parameter, f is the desired frequency, and SysClk is the
    // system clock frequency.
    // In this case you get: (1 / 250Hz) * 16MHz = 64000 cycles.  Note that
    // the maximum period you can set is 2^16.
    //
    PWMGenPeriodSet(PWM_BASE, PWM_GEN_0, 64000);

    //
    // For this example the PWM0 duty cycle will be variable.  The duty cycle
    // will start at 0.1% (0.01 * 64000 cycles = 640 cycles) and will increase
    // to 75% (0.5 * 64000 cycles = 32000 cycles).  After a duty cycle of 75%
    // is reached, it is reset to 0.1%.  This dynamic adjustment of the pulse
    // width is done in the PWM0 load interrupt, which increases the duty
    // cycle by 0.1% everytime the reload interrupt is received.
    //
    PWMPulseWidthSet(PWM_BASE, PWM_OUT_0, 64);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Allow PWM0 generated interrupts.  This configuration is done to
    // differentiate fault interrupts from other PWM0 related interrupts.
    //
    PWMIntEnable(PWM_BASE, PWM_INT_GEN_0);

    //
    // Enable the PWM0 LOAD interrupt on PWM0.
    //
    PWMGenIntTrigEnable(PWM_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);

    //
    // Enable the PWM0 interrupts on the processor (NVIC).
    //
    IntEnable(INT_PWM0);

    //
    // Enable the PWM0 output signal (PD0).
    //
    PWMOutputState(PWM_BASE, PWM_OUT_0_BIT, true);

    //
    // Enables the PWM generator block.
    //
    PWMGenEnable(PWM_BASE, PWM_GEN_0);

    //
    // Loop forever while the PWM signals are generated and PWM0 interrupts
    // get received.
    //
    while(1)
    {
        //
        // Print out indication on the console that the program is running.
        //
        PrintRunningDots();
    }
}
