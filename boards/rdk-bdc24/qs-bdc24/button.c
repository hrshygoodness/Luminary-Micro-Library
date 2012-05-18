//*****************************************************************************
//
// button.c - Driver for handling the user button.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "button.h"
#include "constants.h"
#include "led.h"
#include "message.h"
#include "param.h"
#include "pins.h"
#include "servo_if.h"

//*****************************************************************************
//
// The current debounced button state.
//
//*****************************************************************************
static unsigned long g_ulDebouncedState;

//*****************************************************************************
//
// The number of consecutive ticks that the button must be in the opposite
// state in order to be recognized as having changed state.  This is reset to
// the debounce timeout when the button matches the debounced state and is
// decrement when it does not match the debounced state.  When this value
// reaches zero (which will only happen after N consecutive samples with the
// button in the opposite state), the debounced state of the button is
// toggled.
//
//*****************************************************************************
static unsigned long g_ulDebounceCount;

//*****************************************************************************
//
// The number of consecutive ticks that the debounced state of the button is
// down.  When this reaches the hold time, the servo calibration process is
// started.
//
//*****************************************************************************
static unsigned long g_ulHoldCount;

//*****************************************************************************
//
// This function initializes the state of the button handler and prepares it
// to debounce the state of the button.  If the button is initially pressed,
// the state of the motor controller is reverted to the factory settings.
//
//*****************************************************************************
void
ButtonInit(void)
{
    //
    // Set the GPIO as an input and enable the weak pull up.  When pressed, the
    // button is read as 0.
    //
    ROM_GPIODirModeSet(BUTTON_PORT, BUTTON_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(BUTTON_PORT, BUTTON_PIN,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Delay for a bit before sampling the reset state of the button.
    //
    SysCtlDelay(1000);

    //
    // Read the current state of the button and consider it to be the debounced
    // state.
    //
    g_ulDebouncedState = ROM_GPIOPinRead(BUTTON_PORT, BUTTON_PIN);

    //
    // Reset the debounce and hold counters.
    //
    g_ulDebounceCount = BUTTON_DEBOUNCE_COUNT;
    g_ulHoldCount = 0;

    //
    // See if the button was initially pressed.
    //
    if(g_ulDebouncedState == BUTTON_DOWN)
    {
        //
        // Reset the parameters to the default values.
        //
        ParamLoadDefault();

        //
        // Save the default parameters.
        //
        ParamSave();

        //
        // Indicate that the configuration was just reset to the default
        // settings.
        //
        LEDParameterReset();

        //
        // Set the hold count such that the power-on hold of the push button
        // will not cause the servo calibration process to start.
        //
        g_ulHoldCount = BUTTON_HOLD_COUNT;
    }
}

//*****************************************************************************
//
// When called periodically, this function samples the state of the button in
// order to debounce its state.  When the button state changes, the
// appropriate action is taken:
//
// * When the button is first pressed, the CAN interface module is notified so
//   that it can accept a device ID assignment if one is pending.
//
// * If the button is held, the servo interface module is notified so that it
//   can start the servo calibration process.
//
// * When the button is released and it has been held, the servo interface
//   module is notified so that it can end the servo calibration process.
//
// The BUTTON_DEBOUNCE_COUNT and BUTTON_HOLD_COUNT constants control the
// behavior of the button handling.
//
//*****************************************************************************
void
ButtonTick(void)
{
    unsigned long ulButtonValue;

    //
    // Get the current button value.
    //
    ulButtonValue = ROM_GPIOPinRead(BUTTON_PORT, BUTTON_PIN);

    //
    // See if the button state matches the debounced state.
    //
    if(ulButtonValue == g_ulDebouncedState)
    {
        //
        // The button state matches the debounced state, so reset the debounce
        // counter.
        //
        g_ulDebounceCount = BUTTON_DEBOUNCE_COUNT;
    }
    else
    {
        //
        // The button states does not match the debounced state, so decrement
        // the debounce counter.
        //
        g_ulDebounceCount--;

        //
        // See if the debounce counter has reached zero.
        //
        if(g_ulDebounceCount == 0)
        {
            //
            // The debounce counter is zero, so save the new button state as
            // the debounced button state.
            //
            g_ulDebouncedState = ulButtonValue;

            //
            // See if the button was just pressed.
            //
            if(ulButtonValue == BUTTON_DOWN)
            {
                //
                // The button was just pressed, so reset the hold counter.
                //
                g_ulHoldCount = 0;

                //
                // Inform the message module that the button was just pressed.
                //
                MessageButtonPress();
            }

            //
            // Otherwise, the button was just released.  See if it was held
            // long enough to trigger a calibration.
            //
            else if(g_ulHoldCount >= BUTTON_HOLD_COUNT)
            {
                //
                // Finish the servo input calibration process.
                //
                ServoIFCalibrationEnd();
            }
        }
    }

    //
    // See if the debounced state of the button is pressed.
    //
    if(g_ulDebouncedState == BUTTON_DOWN)
    {
        //
        // Increment the hold counter.
        //
        g_ulHoldCount++;

        //
        // See if the hold counter has reached the hold count.
        //
        if(g_ulHoldCount == BUTTON_HOLD_COUNT)
        {
            //
            // Start the servo input calibration process.
            //
            ServoIFCalibrationStart();
        }
    }
}
