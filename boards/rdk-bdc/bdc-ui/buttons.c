//*****************************************************************************
//
// buttons.c - Routines for handling the on-board push buttons.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "buttons.h"

//*****************************************************************************
//
// The debounced state of the five push buttons.
//
//*****************************************************************************
static unsigned char g_ucSwitches = 0x1f;

//*****************************************************************************
//
// The vertical counter used to debounce the push buttons.  The bit positions
// are the same as g_ucSwitches.
//
//*****************************************************************************
static unsigned char g_ucClockA = 0;
static unsigned char g_ucClockB = 0;
static unsigned char g_ucClockC = 0;

//*****************************************************************************
//
// This structure describes the auto-repeat configuration of a push button.
//
//*****************************************************************************
typedef struct
{
    unsigned char ucButton;
    unsigned char ucRepeatCount;
    unsigned short usCount;
    unsigned short usDelay;
    unsigned short usRepeatDelay;
    unsigned char ucRepeatCount1;
    unsigned char ucRepeatCount2;
    unsigned char ucRepeatCount3;
}
tButtons;

//*****************************************************************************
//
// The auto-repeat configuration for the push buttons.
//
//*****************************************************************************
static tButtons g_psButtons[] =
{
    { BUTTON_UP, 0, 0, 333, 100, 10, 30, 50 },
    { BUTTON_DOWN, 0, 0, 333, 100, 10, 30, 50 },
    { BUTTON_LEFT, 0, 0, 333, 100, 10, 30, 50 },
    { BUTTON_RIGHT, 0, 0, 333, 100, 10, 30, 50 },
    { BUTTON_SELECT, 0, 0, 0, 0, 0, 0, 0 }
};

//*****************************************************************************
//
// The number of push buttons.
//
//*****************************************************************************
#define NUM_BUTTONS             (sizeof(g_psButtons) / sizeof(g_psButtons[0]))

//*****************************************************************************
//
// Initializes the push button driver.
//
//*****************************************************************************
void
ButtonsInit(void)
{
    //
    // Enable the peripherals used by the buttons.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);

    //
    // Configure the GPIOs used to read the state of the on-board push buttons.
    //
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE,
                         GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPadConfigSet(GPIO_PORTF_BASE,
                     GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPinTypeGPIOInput(GPIO_PORTG_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);
}

//*****************************************************************************
//
// The button polling routine that must be called on a periodic basis.
//
//*****************************************************************************
unsigned long
ButtonsTick(void)
{
    unsigned long ulData, ulDelta;
    unsigned long ulReturn;

    //
    // Read the state of the push buttons.
    //
    ulData = ((GPIOPinRead(GPIO_PORTF_BASE, (GPIO_PIN_4 | GPIO_PIN_5 |
                                             GPIO_PIN_6 | GPIO_PIN_7)) >> 4) |
              GPIOPinRead(GPIO_PORTG_BASE, GPIO_PIN_4));

    //
    // Determine the switches that are at a different state than the debounced
    // state.
    //
    ulDelta = ulData ^ g_ucSwitches;

    //
    // Increment the clocks by one.
    //
    g_ucClockA ^= (g_ucClockB & g_ucClockC);
    g_ucClockB ^= g_ucClockC;
    g_ucClockC = ~g_ucClockC;

    //
    // Reset the clocks corresponding to switches that have not changed state.
    //
    g_ucClockA &= ulDelta;
    g_ucClockB &= ulDelta;
    g_ucClockC &= ulDelta;

    //
    // Get the new debounced switch state.
    //
    g_ucSwitches &= g_ucClockA | g_ucClockB | g_ucClockC;
    g_ucSwitches |= (~(g_ucClockA | g_ucClockB | g_ucClockC)) & ulData;

    //
    // Determine the switches that just changed debounced state.
    //
    ulDelta ^= (g_ucClockA | g_ucClockB | g_ucClockC);

    //
    // Loop through all of the buttons.
    //
    ulReturn = 0;
    for(ulData = 0; ulData < NUM_BUTTONS; ulData++)
    {
        //
        // Ignore this button if it is not pressed.
        //
        if(g_ucSwitches & (1 << ulData))
        {
            continue;
        }

        //
        // Return a button press for this button if it was just pressed.
        //
        if(ulDelta & (1 << ulData))
        {
            ulReturn |= 1 << ulData;
        }

        //
        // See if this button has auto-repeat enabled.
        //
        if(g_psButtons[ulData].usDelay != 0)
        {
            //
            // Reset the auto-repeat counters if this button was just pressed.
            //
            if(ulDelta & (1 << ulData))
            {
                g_psButtons[ulData].ucRepeatCount = 0;
                g_psButtons[ulData].usCount = g_psButtons[ulData].usDelay;
            }

            //
            // Decrement the auto-repeat count and see if it has reached zero.
            //
            if(--g_psButtons[ulData].usCount == 0)
            {
                //
                // Return a button press for this button.
                //
                ulReturn |= 1 << ulData;

                //
                // Increment the count of auto-repeats if it has not yet
                // reached 255.
                //
                if(g_psButtons[ulData].ucRepeatCount < 255)
                {
                    g_psButtons[ulData].ucRepeatCount++;
                }

                //
                // Reset the delay count.
                //
                g_psButtons[ulData].usCount =
                    g_psButtons[ulData].usRepeatDelay;

                //
                // See if the button has been held long enough for an
                // accelerator.
                //
                if(g_psButtons[ulData].ucRepeatCount <=
                   g_psButtons[ulData].ucRepeatCount1)
                {
                }
                else if(g_psButtons[ulData].ucRepeatCount <=
                        g_psButtons[ulData].ucRepeatCount2)
                {
                    ulReturn |= 0x100 << ulData;
                }
                else if(g_psButtons[ulData].ucRepeatCount <=
                        g_psButtons[ulData].ucRepeatCount3)
                {
                    ulReturn |= 0x10000 << ulData;
                }
                else
                {
                    ulReturn |= 0x1000000 << ulData;
                }
            }
        }
    }

    //
    // Return the set of buttons that were just pressed (or had phantom presses
    // as a result of auto-repeat).
    //
    return(ulReturn);
}
