//*****************************************************************************
//
// buttons.c - Functions for handling the on-board push buttons.
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
// This is part of revision 10636 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup buttons_api
//! @{
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "drivers/buttons.h"

//*****************************************************************************
//
// Default auto-repeat parameters.  Unless the client calls function
// ButtonsSetAutoRepeat(), these values will cause auto-repeat to be disabled
// for all keys.
//
//*****************************************************************************
#define DEFAULT_INITIAL_COUNT 50
#define DEFAULT_REPEAT_COUNT  5

//*****************************************************************************
//
// The button info structure used to track when auto-repeats are generated if
// a button is held down.
//
//*****************************************************************************
struct
{
    unsigned char ucBtn;
    unsigned char ucCount;
    unsigned char ucInitialCount;
    unsigned char ucRepeatCount;
}
g_psButtonInfo[NUM_BUTTONS] =
{
    { UP_BUTTON, 0, DEFAULT_INITIAL_COUNT, DEFAULT_REPEAT_COUNT },
    { DOWN_BUTTON, 0, DEFAULT_INITIAL_COUNT, DEFAULT_REPEAT_COUNT },
    { LEFT_BUTTON, 0, DEFAULT_INITIAL_COUNT, DEFAULT_REPEAT_COUNT },
    { RIGHT_BUTTON, 0, DEFAULT_INITIAL_COUNT, DEFAULT_REPEAT_COUNT },
    { SELECT_BUTTON, 0, DEFAULT_INITIAL_COUNT, DEFAULT_REPEAT_COUNT }
};

//*****************************************************************************
//
// These globals form a 4 state vertical counter which is used to allow us to
// debounce up to 8 buttons at once.
//
//*****************************************************************************
unsigned char g_ucDebounceClockA = 0;
unsigned char g_ucDebounceClockB = 0;

//*****************************************************************************
//
// This global holds the current, debounced state of each button.  A 0 in a bit
// indicates that that button is currently pressed, otherwise it is released.
// We assume that we start with all the buttons released (though if one is
// pressed when the application starts, this will be detected).
//
//*****************************************************************************
unsigned char g_ucButtonStates = ALL_BUTTONS;

//*****************************************************************************
//
// This global is set each time ButtonsPoll is called and indicates which
// buttons have changed state since the last call to the polling function.
//
//*****************************************************************************
unsigned char g_ucButtonDelta = 0;

//*****************************************************************************
//
// This global is set each time ButtonsPoll is called and indicates which
// buttons are generating an auto-repeat message as a result of the call.
//
//*****************************************************************************
unsigned char g_ucButtonRepeat = 0;

//*****************************************************************************
//
//! Initializes the GPIO pins used by the board pushbuttons.
//!
//! This function must be called during application initialization to
//! configure the GPIO pins to which the pushbuttons are attached.  It enables
//! the port used by the buttons and configures each button GPIO as an input
//! with a weak pull-up.
//!
//! \return None.
//
//*****************************************************************************
void
ButtonsInit(void)
{
    //
    // Enable the GPIO port to which the pushbuttons are connected.
    //
    ROM_SysCtlPeripheralEnable(BUTTONS_GPIO_PERIPH);

    //
    // Set each of the button GPIO pins as an input with a pull-up.
    //
    ROM_GPIODirModeSet(BUTTONS_GPIO_BASE, ALL_BUTTONS, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(BUTTONS_GPIO_BASE, ALL_BUTTONS,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Initialize the debounced button state with the current state read from
    // the GPIO bank.
    //
    g_ucButtonStates = ROM_GPIOPinRead(BUTTONS_GPIO_BASE, ALL_BUTTONS);
}

//*****************************************************************************
//
//! Polls the current state of the buttons and determines which have changed.
//!
//! \param pucDelta points to a character that will be written to indicate
//! which button states changed since the last time this function was called.
//! This value is derived from the debounced state of the buttons.
//! \param pucRepeat points to a character that will be written to indicate
//! which buttons are signaling an auto-repeat as a result of this call.
//!
//! This function should be called periodically by the application to poll the
//! pushbuttons.  It determines both which buttons have changed state since the
//! last call and also signals auto-repeats based on the button state and the
//! number of ButtonsPoll() calls made since the button was last pressed.
//!
//! Auto-repeats are signaled at an application-specified rate if a key has
//! been held down for longer than an initial delay period.  To ensure that
//! auto-repeats are generated at the desired rate, the application should
//! ensure that this function is called at a regular period since all
//! auto-repeat timing is calculated in terms of calls to ButtonsPoll().
//!
//! \return Returns the current debounced state of the buttons where a 1 in the
//! button ID's position indicates that the button is released and a 0
//! indicates that it is pressed.
//
//*****************************************************************************
unsigned char
ButtonsPoll(unsigned char *pucDelta, unsigned char *pucRepeat)
{
    unsigned char ucDelta;
    unsigned char ucData;
    unsigned char ucRepeat;
    unsigned long ulLoop;

    //
    // Get the current (raw) state of the buttons.
    //
    ucData = ROM_GPIOPinRead(BUTTONS_GPIO_BASE, ALL_BUTTONS);

    //
    // Determine the buttons that are in a different state than the debounced
    // state.
    //
    ucDelta = ucData ^ g_ucButtonStates;

    //
    // Increment the clocks by one.
    //
    g_ucDebounceClockA ^= g_ucDebounceClockB;
    g_ucDebounceClockB = ~g_ucDebounceClockB;

    //
    // Reset the clocks corresponding to switches that have not changed state.
    //
    g_ucDebounceClockA &= ucDelta;
    g_ucDebounceClockB &= ucDelta;

    //
    // Get the new debounced switch state.
    //
    g_ucButtonStates &= g_ucDebounceClockA | g_ucDebounceClockB;
    g_ucButtonStates |= (~(g_ucDebounceClockA | g_ucDebounceClockB)) & ucData;

    //
    // Determine the switches that just changed debounced state.
    //
    ucDelta ^= (g_ucDebounceClockA | g_ucDebounceClockB);

    //
    // Remember the delta state in our global variable.
    //
    g_ucButtonDelta = ucDelta;

    //
    // Now consider auto-repeats.
    //
    ucRepeat = 0;

    //
    // Loop through the buttons.
    //
    for(ulLoop = 0; ulLoop < NUM_BUTTONS; ulLoop++)
    {
        //
        // We only ever send auto-repeats for buttons that are in the pressed
        // state so check for this first.
        //
        if((g_ucButtonStates & g_psButtonInfo[ulLoop].ucBtn) == 0)
        {
            //
            // First, check for any buttons that have just been pressed.  We
            // need to initialize the auto-repeat counter for these.
            //
            if(ucDelta & g_psButtonInfo[ulLoop].ucBtn)
            {
                //
                // The button has just been pressed so set up for the initial
                // delay prior to repeating.
                //
                g_psButtonInfo[ulLoop].ucCount =
                    g_psButtonInfo[ulLoop].ucInitialCount;
            }

            //
            // Now determine if the button needs to send an auto-repeat.  This
            // will be necessary if auto-repeat is enabled (ucRepeatCount not
            // set to 0) and the button counter has reached 0.
            //
            if((g_psButtonInfo[ulLoop].ucCount == 0) &&
               (g_psButtonInfo[ulLoop].ucRepeatCount != 0))
            {
                //
                // Set the auto-repeat flag for this button.
                //
                ucRepeat |= g_psButtonInfo[ulLoop].ucBtn;

                //
                // Reset the button counter for the next auto-repeat interval.
                //
                g_psButtonInfo[ulLoop].ucCount =
                    g_psButtonInfo[ulLoop].ucRepeatCount;
            }

            //
            // Decrement the button counter.
            //
            g_psButtonInfo[ulLoop].ucCount--;
        }
    }

    //
    // Update our global with the auto-repeat state.
    //
    g_ucButtonRepeat = ucRepeat;

    //
    // Pass the returned button information back to the caller.
    //
    *pucDelta = ucDelta;
    *pucRepeat = ucRepeat;
    return(g_ucButtonStates);
}

//*****************************************************************************
//
//! Sets the auto-repeat parameters for one or more buttons.
//!
//! \param ucButtonIDs is a bitmask containing the OR-ed IDs of the buttons
//! whose auto-repeat parameters are to be set.
//! \param ucInitialTicks is the number of ticks (calls to ButtonsPoll())
//! before the first auto-repeat is reported for the key if it is pressed for
//! an extended period.
//! \param ucRepeatTicks is the number of ticks that must elapse after the
//! initial period (\e ucInitialTicks) has expired between each subsequent
//! auto-repeat is reported for the key.
//!
//! This function may be called to change the auto-repeat delay and repeat
//! period for one or more keys.  Auto-Repeat allows an application to be
//! signaled periodically if any key is held down for an extended period of
//! time.  After an initial delay following the original button press, a repeat
//! signal flag is generated at a period determined by \e ucRepeatTicks and the
//! interval between calls to ButtonsPoll().
//!
//! For example, to configure a button such that it starts auto-repeating 500mS
//! after it is initially pressed and signals an auto-repeat every 100mS until
//! it is released, and assuming that ButtonsPoll() is called every 50mS, the
//! following parameters would be used:
//!
//! <tt>ucInitialTicks = 10</tt>
//!
//! <tt>ucRepeatTicks = 2</tt>
//!
//! \return None.
//
//*****************************************************************************
void
ButtonsSetAutoRepeat(unsigned char ucButtonIDs, unsigned char ucInitialTicks,
                     unsigned char ucRepeatTicks)
{
    unsigned long ulLoop;

    //
    // Loop through each of the buttons updating all those specified in the
    // ucButtonIDs parameter.
    //
    for(ulLoop = 0; ulLoop < NUM_BUTTONS; ulLoop++)
    {
        //
        // Are the parameters for this button being updated?
        //
        if(ucButtonIDs & g_psButtonInfo[ulLoop].ucBtn)
        {
            //
            // Remember the new auto-repeat parameters and set the button's
            // auto-repeat count to the initial tick count just in case the
            // button is already pressed.  This ensure that it will start
            // auto-repeating at the correct rate.
            //
            g_psButtonInfo[ulLoop].ucInitialCount = ucInitialTicks;
            g_psButtonInfo[ulLoop].ucRepeatCount = ucRepeatTicks;
            g_psButtonInfo[ulLoop].ucCount = ucInitialTicks;
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
