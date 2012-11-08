//*****************************************************************************
//
// gpio_jtag.c - Example to demonstrate recovering the JTAG interface.
//
// Copyright (c) 2006-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM3S811 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "drivers/display96x16x1.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>GPIO JTAG Recovery (gpio_jtag)</h1>
//!
//! This example demonstrates changing the JTAG pins into GPIOs, along with a
//! mechanism to revert them to JTAG pins.  When first run, the pins remain in
//! JTAG mode.  Pressing the user push button will toggle the pins between JTAG
//! mode and GPIO mode.  Because there is no debouncing of the push button
//! (either in hardware or software), a button press will occasionally result
//! in more than one mode change.
//!
//! In this example, all five pins (PB7, PC0, PC1, PC2, and PC3) are switched,
//! though the more typical use would be to change PB7 into a GPIO.  Note that
//! because of errata in Rev Bx and Rev C0  of Sandstorm-class Stellaris
//! microcontrollers, JTAG and SWD will not function if PB7 is configured as a
//! GPIO.  This errata is fixed in Rev C2 of Sandstorm-class Stellaris
//! microcontrollers.
//
//*****************************************************************************

//*****************************************************************************
//
// The current mode of pins PB7, PC0, PC1, PC2, and PC3.  When zero, the pins
// are in JTAG mode; when non-zero, the pins are in GPIO mode.
//
//*****************************************************************************
volatile unsigned long g_ulMode;

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
// The interrupt handler for the PC4 pin interrupt.  When triggered, this will
// toggle the JTAG pins between JTAG and GPIO mode.
//
//*****************************************************************************
void
GPIOCIntHandler(void)
{
    //
    // Clear the GPIO interrupt.
    //
    GPIOPinIntClear(GPIO_PORTC_BASE, GPIO_PIN_4);

    //
    // Toggle the pin mode.
    //
    g_ulMode ^= 1;

    //
    // See if the pins should be in JTAG or GPIO mode.
    //
    if(g_ulMode == 0)
    {
        //
        // Change PB7 and PC0-3 into hardware (i.e. JTAG) pins.
        //
        GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_DIR_MODE_HW);
        GPIODirModeSet(GPIO_PORTC_BASE,
                       GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                       GPIO_DIR_MODE_HW);
    }
    else
    {
        //
        // Change PB7 and PC0-3 into GPIO inputs.
        //
        GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7);
        GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, (GPIO_PIN_0 | GPIO_PIN_1 |
                                               GPIO_PIN_2 | GPIO_PIN_3));
    }
}

//*****************************************************************************
//
// Toggle the JTAG pins between JTAG and GPIO mode with a push button selecting
// between the two.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulMode;

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_6MHZ);

    //
    // Enable the peripherals used by this application.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    //
    // Configure the push button as an input and enable the pin to interrupt on
    // the falling edge (i.e. when the push button is pressed).
    //
    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_4);
    GPIOIntTypeSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOPinIntEnable(GPIO_PORTC_BASE, GPIO_PIN_4);
    IntEnable(INT_GPIOC);

    //
    // Set the global and local indicator of pin mode to zero, meaning JTAG.
    //
    g_ulMode = 0;
    ulMode = 0;

    //
    // Initialize the OLED display.
    //
    Display96x16x1Init(false);
    Display96x16x1StringDraw("PB7/PC0-3 are", 9, 0);
    Display96x16x1StringDraw("JTAG", 36, 1);

    //
    // Loop forever.  This loop simply exists to display on the OLED display
    // the current state of PB7/PC0-3; the handling of changing the JTAG pins
    // to and from GPIO mode is done in GPIOCIntHandler().
    //
    while(1)
    {
        //
        // Wait until the pin mode changes.
        //
        while(g_ulMode == ulMode)
        {
        }

        //
        // Save the new mode locally so that a subsequent pin mode change can
        // be detected.
        //
        ulMode = g_ulMode;

        //
        // See what the new pin mode was changed to.
        //
        if(ulMode == 0)
        {
            //
            // Indicate that PB7 and PC0-3 are currently JTAG pins.
            //
            Display96x16x1StringDraw("JTAG", 36, 1);
        }
        else
        {
            //
            // Indicate that PB7 and PC0-3 are currently GPIO pins.
            //
            Display96x16x1StringDraw("GPIO", 36, 1);
        }
    }
}
