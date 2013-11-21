//*****************************************************************************
//
// simple_display.c - Simple display example for the ek-evalbot board.
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ethernet.h"
#include "driverlib/ethernet.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "drivers/display96x16x1.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Simple Display (simple_display)</h1>
//!
//! This example application demonstrates the use of the display and LEDs
//! on the EK-EVALBOT by printing a series of messages on the display and
//! blinking the LEDs.  It uses the system tick timer (SysTick) as a time
//! reference.
//
//*****************************************************************************

//*****************************************************************************
//
// Counter for the 100 ms system clock ticks.  Used for tracking time.
//
//*****************************************************************************
static volatile unsigned long g_ulTickCount;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    for(;;)
    {
    }
}
#endif

//*****************************************************************************
//
// The interrupt handler for the SysTick timer.  This handler will increment a
// tick counter to keep track of time.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Increment the tick counter.
    //
    g_ulTickCount++;
}

//*****************************************************************************
//
// The main application.  It configures the board and then enters a loop
// to show messages on the display and blink the LEDs.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulPHYMR0;
    unsigned long ulNextTickLED = 0;
    unsigned long ulNextTickDisplay = 0;
    unsigned long ulDisplayState = 0;

    //
    // Set the clocking to directly from the crystal
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Since the Ethernet is not used, power down the PHY to save battery.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    ulPHYMR0 = ROM_EthernetPHYRead(ETH_BASE, PHY_MR0);
    ROM_EthernetPHYWrite(ETH_BASE, PHY_MR0, ulPHYMR0 | PHY_MR0_PWRDN);

    //
    // Initialize the board display
    //
    Display96x16x1Init(true);

    //
    // Initialize the GPIO used for the LEDs, and then turn one LED on
    // and the other off
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_5, GPIO_PIN_5);

    //
    // Set up and enable the SysTick timer to use as a time reference.
    // It will be set up for a 100 ms tick.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 10);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enter loop to run forever, blinking LEDs and printing messages to
    // the display
    //
    for(;;)
    {
        //
        // If LED blink period has timed out, then toggle LEDs.
        //
        if(g_ulTickCount >= ulNextTickLED)
        {
            //
            // Set next LED toggle timeout for 1 second
            //
            ulNextTickLED += 10;

            //
            // Toggle the state of each LED
            //
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_5,
                             ~ROM_GPIOPinRead(GPIO_PORTF_BASE,
                                              GPIO_PIN_4 | GPIO_PIN_5));
        }

        //
        // If display interval has elapsed, then update display state
        //
        if(g_ulTickCount >= ulNextTickDisplay)
        {
            //
            // Process the display state.  Each state, the display does
            // something different.
            //
            // Odd time intervals are used, like 5.3 seconds, to keep the
            // display updates out of sync with the LED blinking which is
            // happening on a 1 second interval.  This is just for cosmetic
            // effect, there is no technical reason it needs to be that way.
            //
            switch(ulDisplayState)
            {
                //
                // Initial state, show TEXAS INSTRUMENTS for 5.3 seconds
                //
                case 0:
                {
                    Display96x16x1StringDraw("TEXAS", 29, 0);
                    Display96x16x1StringDraw("INSTRUMENTS", 11, 1);
                    ulNextTickDisplay += 53;
                    ulDisplayState = 1;
                    break;
                }

                //
                // Next, clear display for 1.3 seconds
                //
                case 1:
                {
                    Display96x16x1Clear();
                    ulNextTickDisplay += 13;
                    ulDisplayState = 2;
                    break;
                }

                //
                // Show STELLARIS for 5.3 seconds
                //
                case 2:
                {
                    Display96x16x1StringDraw("STELLARIS", 21, 0);
                    ulNextTickDisplay += 53;
                    ulDisplayState = 3;
                    break;
                }

                //
                // Clear the previous message and then show EVALBOT on
                // the second line for 5.3 seconds
                //
                case 3:
                {
                    Display96x16x1Clear();
                    Display96x16x1StringDraw("EVALBOT", 27, 1);
                    ulNextTickDisplay += 53;
                    ulDisplayState = 4;
                    break;
                }

                //
                // Clear display for 1.3 seconds, then go back to start
                //
                case 4:
                {
                    Display96x16x1Clear();
                    ulNextTickDisplay += 13;
                    ulDisplayState = 0;
                    break;
                }

                //
                // Default state.  Should never get here, but if so just
                // go back to starting state.
                //
                default:
                {
                    ulDisplayState = 0;
                    break;
                }
            } // end switch
        } // end if
    } // end for(;;)
}

