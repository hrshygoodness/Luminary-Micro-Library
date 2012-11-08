//*****************************************************************************
//
// watchdog.c - Watchdog timer example.
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
// This is part of revision 9107 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/watchdog.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Watchdog (watchdog)</h1>
//!
//! This example application demonstrates the use of the watchdog as a simple
//! heartbeat for the system.  If the watchdog is not periodically fed, it will
//! reset the system.  Each time the watchdog is fed, the LED is inverted so
//! that it is easy to see that it is being fed, which occurs once every
//! second.  To stop the watchdog being fed and, hence, cause a system reset,
//! tap the screen.
//
//*****************************************************************************

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Flag to tell the watchdog interrupt handler whether or not to clear the
// interrupt (feed the watchdog).
//
//*****************************************************************************
volatile tBoolean g_bFeedWatchdog = true;

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
// The interrupt handler for the watchdog.  This feeds the dog (so that the
// processor does not get reset) and winks the LED connected to GPIO B3.
//
//*****************************************************************************
void
WatchdogIntHandler(void)
{
    //
    // If we have been told to stop feeding the watchdog, return immediately
    // without clearing the interrupt.  This will cause the system to reset
    // next time the watchdog interrupt fires.
    //
    if(!g_bFeedWatchdog)
    {
        return;
    }

    //
    // Clear the watchdog interrupt.
    //
    ROM_WatchdogIntClear(WATCHDOG0_BASE);

    //
    // Invert the GPIO PF3 value.
    //
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3,
                     (ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3) ^
                                     GPIO_PIN_3));
}

//*****************************************************************************
//
// The touch screen driver calls this function to report all state changes.
//
//*****************************************************************************
static long
WatchdogTouchCallback(unsigned long ulMessage, long lX,  long lY)
{
    //
    // If the screen is tapped, we will receive a PTR_DOWN then a PTR_UP
    // message. We pick one (pretty much at random) to use as the trigger to
    // stop feeding the watchdog.
    //
    if(ulMessage == WIDGET_MSG_PTR_UP)
    {
        //
        // Let the user know that the tap has been registered and that the
        // watchdog is being starved.
        //
        GrContextFontSet(&g_sContext, g_pFontCmss20);
        GrStringDrawCentered(&g_sContext, "Watchdog is not being fed!", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2 ,
                             (GrContextDpyHeightGet(&g_sContext) / 2), 1);
        GrContextFontSet(&g_sContext, g_pFontCmss14);
        GrStringDrawCentered(&g_sContext,
                             "           System will reset shortly.           ",
                             -1, GrContextDpyWidthGet(&g_sContext) / 2 ,
                             (GrContextDpyHeightGet(&g_sContext) / 2) + 20, 1);

        //
        // Set the flag that tells the interrupt handler not to clear the
        // watchdog interrupt.
        //
        g_bFeedWatchdog = false;
    }

    return(0);
}

//*****************************************************************************
//
// This example demonstrates the use of the watchdog timer.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;

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
    TouchScreenCallbackSet(WatchdogTouchCallback);

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
    GrStringDrawCentered(&g_sContext, "watchdog", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 8, 0);

    //
    // Show the state and offer some instructions to the user.
    //
    GrContextFontSet(&g_sContext, g_pFontCmss20);
    GrStringDrawCentered(&g_sContext, "Watchdog is being fed.", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2 ,
                         (GrContextDpyHeightGet(&g_sContext) / 2), 1);
    GrContextFontSet(&g_sContext, g_pFontCmss14);
    GrStringDrawCentered(&g_sContext, "Tap the screen to starve the watchdog",
                         -1, GrContextDpyWidthGet(&g_sContext) / 2 ,
                         (GrContextDpyHeightGet(&g_sContext) / 2) + 20, 1);

    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Set GPIO PF3 as an output.  This drives an LED on the board that will
    // toggle when a watchdog interrupt is processed.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);

    //
    // Enable the watchdog interrupt.
    //
    ROM_IntEnable(INT_WATCHDOG);

    //
    // Set the period of the watchdog timer.
    //
    ROM_WatchdogReloadSet(WATCHDOG0_BASE, ROM_SysCtlClockGet());

    //
    // Enable reset generation from the watchdog timer.
    //
    ROM_WatchdogResetEnable(WATCHDOG0_BASE);

    //
    // Enable the watchdog timer.
    //
    ROM_WatchdogEnable(WATCHDOG0_BASE);

    //
    // Loop forever while the LED winks as watchdog interrupts are handled.
    //
    while(1)
    {
    }
}
