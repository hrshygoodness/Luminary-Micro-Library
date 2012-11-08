//*****************************************************************************
//
// gpio_jtag.c - Example to demonstrate recovering the JTAG interface.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "drivers/formike128x128x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>GPIO JTAG Recovery (gpio_jtag)</h1>
//!
//! This example demonstrates changing the JTAG pins into GPIOs, along with a
//! mechanism to revert them to JTAG pins.  When first run, the pins remain in
//! JTAG mode.  Pressing the select push button will toggle the pins between
//! JTAG mode and GPIO mode.  Because there is no debouncing of the push button
//! (either in hardware or software), a button press will occasionally result
//! in more than one mode change.
//!
//! In this example, four pins (PC0, PC1, PC2, and PC3) are switched.
//
//*****************************************************************************

//*****************************************************************************
//
// The current mode of pins PC0, PC1, PC2, and PC3.  When zero, the pins
// are in JTAG mode; when non-zero, the pins are in GPIO mode.
//
//*****************************************************************************
volatile unsigned long g_ulMode;

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

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
// The interrupt handler for the PB7 pin interrupt.  When triggered, this will
// toggle the JTAG pins between JTAG and GPIO mode.
//
//*****************************************************************************
void
GPIOBIntHandler(void)
{
    //
    // Clear the GPIO interrupt.
    //
    ROM_GPIOPinIntClear(GPIO_PORTB_BASE, GPIO_PIN_7);

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
        // Change PC0-3 into hardware (i.e. JTAG) pins.
        //
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x01;
        HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x01;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x02;
        HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x02;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x04;
        HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x04;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x08;
        HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x08;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x00;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = 0;
    }
    else
    {
        //
        // Change PC0-3 into GPIO inputs.
        //
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x01;
        HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xfe;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x02;
        HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xfd;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x04;
        HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xfb;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x08;
        HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xf7;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
        HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x00;
        HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = 0;
        ROM_GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, (GPIO_PIN_0 | GPIO_PIN_1 |
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
    tRectangle sRect;
    unsigned long ulMode;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_8MHZ);

    //
    // Enable the peripherals used by this application.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    //
    // Configure the push button as an input and enable the pin to interrupt on
    // the falling edge (i.e. when the push button is pressed).
    //
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7);
    ROM_GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE);
    ROM_GPIOPinIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
    ROM_IntEnable(INT_GPIOB);

    //
    // Set the global and local indicator of pin mode to zero, meaning JTAG.
    //
    g_ulMode = 0;
    ulMode = 0;

    //
    // Initialize the display driver.
    //
    Formike128x128x16Init();

    //
    // Turn on the backlight.
    //
    Formike128x128x16BacklightOn();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike128x128x16);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 14;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "gpio_jtag", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Initialize the CSTN display.
    //
    GrStringDrawCentered(&g_sContext, "PC0-3 are", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 , 48, 0);
    GrStringDrawCentered(&g_sContext, "JTAG", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 , 56, 1);

    //
    // Loop forever.  This loop simply exists to display on the CSTN display
    // the current state of PC0-3; the handling of changing the JTAG pins
    // to and from GPIO mode is done in GPIO Interrupt Handler.
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
            // Indicate that PC0-3 are currently JTAG pins.
            //
            GrStringDrawCentered(&g_sContext, "JTAG", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2 , 56,
                                 1);
        }
        else
        {
            //
            // Indicate that PC0-3 are currently GPIO pins.
            //
            GrStringDrawCentered(&g_sContext, "GPIO", -1,
                                 GrContextDpyWidthGet(&g_sContext) / 2 , 56,
                                 1);
        }
    }
}
