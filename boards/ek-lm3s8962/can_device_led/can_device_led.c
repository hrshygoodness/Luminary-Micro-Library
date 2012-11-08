//*****************************************************************************
//
// can_device_led.c - Simple application that runs on the CAN device board and
//                    uses the buttons as a light switch for the LED.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>CAN Device Board LED Application (can_device_led)</h1>
//!
//! This simple application uses the two buttons on the board as a light
//! switch.  When the ``up'' button is pressed the status LED will turn on.
//! When the ``down'' button is pressed, the status LED will turn off.
//
//*****************************************************************************

//*****************************************************************************
//
// The counter of the number of consecutive times that the buttons have
// remained constant.
//
//*****************************************************************************
static unsigned long g_ulDebounceCounter;

//*****************************************************************************
//
// This variable holds the last stable raw button status.
//
//*****************************************************************************
volatile unsigned char g_ucButtonStatus;

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
// This is the interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulStatus;
    unsigned char ucTemp;
    static unsigned long ulLastStatus = 0;

    //
    // Read the current value of the button pins.
    //
    ulStatus = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    if(ulStatus != ulLastStatus)
    {
        //
        // Something changed reset the compare and reset the debounce counter.
        //
        ulLastStatus = ulStatus;
        g_ulDebounceCounter = 0;
    }
    else
    {
        //
        // Increment the number of counts with the push button in a different
        // state.
        //
        g_ulDebounceCounter++;

        //
        // If four consecutive counts have had the push button in a different
        // state then the state has changed.
        //
        if(g_ulDebounceCounter == 4)
        {
            //
            // XOR to see what has changed.
            //
            ucTemp = g_ucButtonStatus ^ ulStatus;

            if(ucTemp & GPIO_PIN_1)
            {
                //
                // The button status has changed to pressed.
                //
                if((GPIO_PIN_1 & ulStatus) == 0)
                {
                    //
                    // Turn off LED.
                    //
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
                }
            }

            if(ucTemp & GPIO_PIN_0)
            {
                //
                // The button status has changed to pressed.
                //
                if((GPIO_PIN_0 & ulStatus) == 0)
                {
                    //
                    // Turn on LED.
                    //
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                }
            }

            //
            // Save the new stable state for comparison.
            //
            g_ucButtonStatus = (unsigned char)ulStatus;
        }
    }
}

//*****************************************************************************
//
// This is the main loop for the application.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable the pull-ups on the JTAG signals.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPadConfigSet(GPIO_PORTC_BASE,
                     GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Configure GPIO Pins used for the Buttons.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Configure GPIO Pin used for the LED.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);

    //
    // Turn off the LED.
    //
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Initialize the button status.
    //
    g_ucButtonStatus = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure SysTick for a 10ms interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / 100);
    SysTickEnable();
    SysTickIntEnable();

    //
    // Loop forever.
    //
    while(1)
    {
    }
}
