//*****************************************************************************
//
// io.c - EVALBOT LED and pushbutton functions.
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
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "io.h"

//*****************************************************************************
//
//! \addtogroup io_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Holds the debounced state of the user buttons.
//
//*****************************************************************************
static unsigned char g_ucDebouncedButtons = GPIO_PIN_6 | GPIO_PIN_7;

//*****************************************************************************
//
//! Initializes the EVALBOT's push buttons.
//!
//! This function must be called prior to PushButtonGetStatus() to configure
//! the GPIOs used to support the user switches on EVALBOT.
//!
//! \return None.
//
//*****************************************************************************
void
PushButtonsInit (void)
{
    //
    // Enable the GPIO port used by the pushbuttons.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Set the button GPIOs as inputs.
    //
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    ROM_GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

//*****************************************************************************
//
//! Debounces the EVALBOT push buttons when called periodically.
//!
//! If button debouncing is used, this function should be called periodically,
//! for example every 10 ms.  It will check the buttons state and save a
//! debounced state that can be read by the application at any time.
//!
//! \return None.
//
//*****************************************************************************
void
PushButtonDebouncer(void)
{
    static unsigned char ucButtonsClockA = 0;
    static unsigned char ucButtonsClockB = 0;
    unsigned char ucData;
    unsigned char ucDelta;

    //
    // Read the current state of the hardware push buttons
    //
    ucData = ROM_GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    //
    // Determine buttons that are in a different state from the debounced state
    //
    ucDelta = ucData ^ g_ucDebouncedButtons;

    //
    // Increment the debounce counter by one
    //
    ucButtonsClockA ^= ucButtonsClockB;
    ucButtonsClockB = ~ucButtonsClockB;

    //
    // Reset the debounce counter for any button that is unchanged
    //
    ucButtonsClockA &= ucDelta;
    ucButtonsClockB &= ucDelta;

    //
    // Determine the new debounced button state based on the debounce
    // counter.
    //
    g_ucDebouncedButtons &= ucButtonsClockA | ucButtonsClockB;
    g_ucDebouncedButtons |= (~(ucButtonsClockA | ucButtonsClockB)) & ucData;
}

//*****************************************************************************
//
//! Get the status of a push button on the EVALBOT.
//!
//! \param eButton is the ID of the push button to query.  Valid values are \e
//!           BUTTON_1 and \e BUTTON_2.
//!
//! This function may be called to determine the state of one of the two user
//! switches on EVALBOT.  Prior to calling it, an application must ensure that
//! it has called PushButtonsInit().
//!
//! \return Returns \b false if the push button is pressed or \b true if it is
//!          not pressed.
//
//*****************************************************************************
tBoolean  PushButtonGetStatus (tButton eButton)
{
    tBoolean  status;

    //
    // Check for invalid parameter values.
    //
    ASSERT((eButton == BUTTON_1) || (eButton == BUTTON_2));

    //
    // Which button are we to query?
    //
    switch (eButton)
    {
        //
        // Switch 1.
        //
        case BUTTON_1:
        {
            status = ROM_GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_6) ?
                                     true : false;
            break;
        }

        //
        // Switch 2.
        //
        case BUTTON_2:
        {
            status = ROM_GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_7) ?
                                     true : false;
            break;
        }

        //
        // An illegal button ID was passed.
        //
        default:
            status = true;
            break;
    }

    return (status);
}

//*****************************************************************************
//
//! Get the debounced state of a push button on the EVALBOT.
//!
//! \param eButton is the ID of the push button to query.  Valid values are \e
//!           BUTTON_1 and \e BUTTON_2.
//!
//! This function may be called to determine the debounced state of one of the
//! two user  switches on EVALBOT.  Prior to calling it, an application must
//! ensure that it has called PushButtonsInit(), and that the function
//! PushButtonDebouncer() is called periodically by the application to keep the
//! debounce processing running.
//!
//! \return Returns \b false if the push button is pressed or \b true if it is
//!          not pressed.
//
//*****************************************************************************
tBoolean  PushButtonGetDebounced(tButton eButton)
{
    tBoolean  status;

    //
    // Check for invalid parameter values.
    //
    ASSERT((eButton == BUTTON_1) || (eButton == BUTTON_2));

    //
    // Which button are we to query?
    //
    switch (eButton)
    {
        //
        // Switch 1.
        //
        case BUTTON_1:
        {
            status = g_ucDebouncedButtons & GPIO_PIN_6 ? true : false;
            break;
        }

        //
        // Switch 2.
        //
        case BUTTON_2:
        {
            status = g_ucDebouncedButtons & GPIO_PIN_7 ? true : false;
            break;
        }

        //
        // An illegal button ID was passed.
        //
        default:
            status = true;
            break;
    }

    return (status);
}

//*****************************************************************************
//
//! Initializes the EVALBOT's LEDs
//!
//! This function must be called to initialize the GPIO pins used to control
//! EVALBOT's LEDs prior to calling LED_Off(), LED_On() or LED_Toggle().
//!
//! \return None.
//
//*****************************************************************************
void
LEDsInit (void)
{
    //
    // Enable the GPIO port used to control the LEDs.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Set the LED GPIOs as output.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // Turn off both LEDs
    //
    LED_Off(BOTH_LEDS);
}

//*****************************************************************************
//
//! Turn one or both of the EVALBOT LEDs on.
//!
//! \param eLED indicates the LED or LEDs to turn on.  Valid values are \e
//!        LED_1, \e LED_2 or \e BOTH_LEDS.
//!
//! This function may be used to light either one or both of the LEDs on the
//! EVALBOT.  Callers must ensure that they have previously called LEDsInit().
//!
//! \return None.
//
//*****************************************************************************
void
LED_On(tLED eLED)
{
    //
    // Check for invalid parameter values.
    //
    ASSERT((eLED == BOTH_LEDS) || (eLED == LED_1) || (eLED == LED_2));

    //
    // Which LED are we to turn on?
    //
    switch (eLED)
    {
        //
        // Turn both LEDs on.
        //
        case BOTH_LEDS:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_5,
                             GPIO_PIN_4 | GPIO_PIN_5);
            break;
        }

        //
        // Turn LED 1 on.
        //
        case LED_1:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4);
            break;
        }

        //
        // Turn LED 2 on.
        //
        case LED_2:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_5, GPIO_PIN_5);
            break;
        }

        //
        // An invalid LED value was passed.
        //
        default:
            break;
    }
}

//*****************************************************************************
//
//! Turn one or both of the EVALBOT LEDs off.
//!
//! \param eLED indicates the LED or LEDs to turn off.  Valid values are \e
//!        LED_1, \e LED_2 or \e BOTH_LEDS.
//!
//! This function may be used to turn off either one or both of the LEDs on the
//! EVALBOT.  Callers must ensure that they have previously called LEDsInit().
//!
//! \return None.
//
//*****************************************************************************
void
LED_Off (tLED eLED)
{
    //
    // Check for invalid parameter values.
    //
    ASSERT((eLED == BOTH_LEDS) || (eLED == LED_1) || (eLED == LED_2));

    //
    // Which LED are we to turn off?
    //
    switch (eLED)
    {
        //
        // Turn both LEDs off.
        //
        case BOTH_LEDS:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_5, 0);
            break;
        }

        //
        // Turn LED 1 off.
        //
        case LED_1:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
            break;
        }

        //
        // Turn LED 2 off.
        //
        case LED_2:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_5, 0);
            break;
        }

        //
        // An invalid value was passed.
        //
        default:
            break;
    }
}

//*****************************************************************************
//
//! Toggle one or both of the EVALBOT LEDs.
//!
//! \param eLED indicates the LED or LEDs to toggle.  Valid values are \e
//!        LED_1, \e LED_2 or \e BOTH_LEDS.
//!
//! This function may be used to toggle either one or both of the LEDs on the
//! EVALBOT.  If the LED is currently lit, it will be turned off and vice verse.
//! Callers must ensure that they have previously called LEDsInit().
//!
//! \return None.
//
//*****************************************************************************
void
LED_Toggle (tLED eLED)
{
    //
    // Check for invalid parameter values.
    //
    ASSERT((eLED == BOTH_LEDS) || (eLED == LED_1) || (eLED == LED_2));

    //
    // Which LED are we to toggle?
    //
    switch (eLED)
    {
        //
        // Toggle both LEDs.
        //
        case BOTH_LEDS:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_5,
                      ~ROM_GPIOPinRead(GPIO_PORTF_BASE,
                                       GPIO_PIN_4 | GPIO_PIN_5));
             break;
        }

        //
        // Toggle LED 1.
        //
        case LED_1:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4,
                             ~ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4));
            break;
        }

        //
        // Toggle LED 2.
        //
        case LED_2:
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_5,
                             ~ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_5));
            break;
        }

        //
        // An invalid value was passed.
        //
        default:
            break;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
