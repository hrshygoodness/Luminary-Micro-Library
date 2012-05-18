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
// This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
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
#include "grlib/widget.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>GPIO JTAG Recovery (gpio_jtag)</h1>
//!
//! This example demonstrates changing the JTAG pins into GPIOs, along with a
//! mechanism to revert them to JTAG pins.  When first run, the pins remain in
//! JTAG mode.  Pressing the touchscreen will toggle the pins between JTAG
//! and GPIO modes.
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
// The touch screen driver calls this function to report all state changes.
//
//*****************************************************************************
static long
GPIOJTAGTestCallback(unsigned long ulMessage, long lX,  long lY)
{
    //
    // Determine what to do now.  The only message we act upon here is PTR_UP
    // which indicates that someone has just ended a touch on the screen.
    //
    if(ulMessage == WIDGET_MSG_PTR_UP)
    {

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
            // Change PC0-3 into hardware (i.e. JTAG) pins.  First open the
            // lock and select the bits we want to modify in the GPIO commit
            // register.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x0F;

            //
            // Now modify the configuration of the pins that we unlocked.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= 0x0F;

            //
            // Finally, clear the commit register and the lock to prevent
            // the pin configuration from being changed accidentally later.
            // Note that the lock is closed whenever we write to the GPIO_O_CR
            // register so we need to reopen it here.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x00;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = 0;
        }
        else
        {
            //
            // Change PC0-3 into GPIO inputs. First open the lock and select
            // the bits we want to modify in the GPIO commit register.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x0F;

            //
            // Now modifiy the configuration of the pins that we unlocked.
            // Note that the DriverLib GPIO functions may need to access
            // registers protected by the lock mechanism so these calls should
            // be made while the lock is open here.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) &= 0xf0;
            ROM_GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, (GPIO_PIN_0 | GPIO_PIN_1 |
                                     GPIO_PIN_2 | GPIO_PIN_3));

            //
            // Finally, clear the commit register and the lock to prevent
            // the pin configuration from being changed accidentally later.
            // Note that the lock is closed whenever we write to the GPIO_O_CR
            // register so we need to reopen it here.
            //
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
            HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = 0x00;
            HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = 0;
        }
    }

    return(0);
}

//*****************************************************************************
//
// Toggle the JTAG pins between JTAG and GPIO mode with touches on the
// touchscreen toggling between the two states.
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
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();
    TouchScreenCallbackSet(GPIOJTAGTestCallback);

    //
    // Set the global and local indicator of pin mode to zero, meaning JTAG.
    //
    g_ulMode = 0;
    ulMode = 0;

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context and find the middle X coordinate.
    //
    GrContextInit(&g_sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 23;
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
    GrContextFontSet(&g_sContext, g_pFontCm20);
    GrStringDrawCentered(&g_sContext, "gpio-jtag", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 8, 0);

    //
    // Tell the user what to do.
    //
    GrStringDrawCentered(&g_sContext, "Tap display to toggle pin mode.", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 ,
                         (GrContextDpyHeightGet(&g_sContext) - 24), 0);

    //
    // Tell the user what state we are in.
    //
    GrContextFontSet(&g_sContext, g_pFontCmss22b);
    GrStringDrawCentered(&g_sContext, "PC0-3 are", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 ,
                         GrContextDpyHeightGet(&g_sContext) / 2, 0);
    GrStringDrawCentered(&g_sContext, "JTAG", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 ,
                         (GrContextDpyHeightGet(&g_sContext) / 2) + 26, 0);

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
        // Indicate the current mode for the PC0-3 pins.
        //
        GrStringDrawCentered(&g_sContext, ulMode ? " GPIO " : " JTAG ", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2 ,
                             (GrContextDpyHeightGet(&g_sContext) / 2) + 26, 1);
    }
}
