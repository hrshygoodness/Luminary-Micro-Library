//*****************************************************************************
//
// ui_onboard.c - A simple control interface utilizing push button(s) and a
//                potentiometer on the board.
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
// This is part of revision 8555 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#include "ui_onboard.h"

//*****************************************************************************
//
//! \page ui_onboard_intro Introduction
//!
//! The on-board user interface consists of a push button and a potentiometer.
//! The push button triggers actions when pressed, released, and when held for
//! a period of time.  The potentiometer specifies the value of a parameter.
//!
//! The push button is debounced using a vertical counter.  A vertical counter
//! is a method where each bit of the counter is stored in a different word,
//! and multiple counters can be incremented simultaneously.  They work really
//! well for debouncing switches; up to 32 switches can be debounced at the
//! same time.  Although only one switch is used, the code is already capable
//! of debouncing an additional 31 switches.
//!
//! A callback function can be called when the switch is pressed, when it is
//! released, and when it is held.  If held, the press function will not be
//! called for that button press.
//!
//! The potentiometer input is passed through a low-pass filter and then a
//! stable value detector.  The low-pass filter reduces the noise introduced
//! by the potentiometer and the ADC.  Even the low-pass filter does not remove
//! all the noise and does not produce an unchanging value when the
//! potentiometer is not being turned.  Therefore, a stable value detector is
//! used to find when the potentiometer value is only changing slightly.  When
//! this occurs, the output value is held constant until the potentiometer
//! value has changed significantly.  Because of this, the parameter value that
//! is adjusted by the potentiometer will not jitter around when the
//! potentiometer is left alone.
//!
//! The application is responsible for reading the value of the switch(es) and
//! the potentiometer on a periodic basis.  The routines provided here perform
//! all the processing of those values.
//!
//! The code for handling the on-board user interface elements is contained in
//! <tt>ui_onboard.c</tt>, with <tt>ui_onboard.h</tt> containing the
//! definitions for the structures and functions exported to the remainder of
//! the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup ui_onboard_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The debounced state of the switches.
//
//*****************************************************************************
static unsigned long g_ulUIOnboardSwitches;

//*****************************************************************************
//
//! This is the low order bit of the clock used to count the number of samples
//! with the switches in the non-debounced state.
//
//*****************************************************************************
static unsigned long g_ulUIOnboardClockA;

//*****************************************************************************
//
//! This is the high order bit of the clock used to count the number of samples
//! with the switches in the non-debounced state.
//
//*****************************************************************************
static unsigned long g_ulUIOnboardClockB;

//*****************************************************************************
//
//! The value of the potentiometer after being passed through the single pole
//! IIR low pass filter.
//
//*****************************************************************************
static unsigned long g_ulUIOnboardPotValue;

//*****************************************************************************
//
//! The detected stable value of the potentiometer.  This will be 0xffff.ffff
//! when the value of the potentiometer is changing and will be a value within
//! the potentiometer range when the potentiometer value is stable.
//
//*****************************************************************************
static unsigned long g_ulUIOnboardFilteredPotValue;

//*****************************************************************************
//
//! The minimum value of the potentiometer over a small period.  This is used
//! to detect a stable value of the potentiometer.
//
//*****************************************************************************
static unsigned long g_ulUIOnboardPotMin;

//*****************************************************************************
//
//! The maximum value of the potentiometer over a small period.  This is used
//! to detect a stable value of the potentiometer.
//
//*****************************************************************************
static unsigned long g_ulUIOnboardPotMax;

//*****************************************************************************
//
//! An accumulator of the low pass filtered potentiometer values for a small
//! period.  When a stable potentiometer value is detected, this is used to
//! compute the average value (and therefore the stable value of the
//! potentiometer).
//
//*****************************************************************************
static unsigned long g_ulUIOnboardPotSum;

//*****************************************************************************
//
//! The count of samples that have been collected into the accumulator
//! (g_ulUIOnboardPotSum).
//
//*****************************************************************************
static unsigned long g_ulUIOnboardPotCount;

//*****************************************************************************
//
//! Debounces a set of switches.
//!
//! \param ulSwitches is the current state of the switches.
//!
//! This function takes a set of switch inputs and performs software debouncing
//! of their state.  Changes in the debounced state of a switch are reflected
//! back to the application via callback functions.  For each switch, a press
//! can be distinguished from a hold, allowing two functions to coexist on a
//! single switch; a separate callback function is called for a hold as opposed
//! to a press.
//!
//! For best results, the switches should be sampled and passed to this
//! function on a periodic basis.  Randomness in the sampling time may result
//! in degraded performance of the debouncing routine.
//!
//! \return None.
//
//*****************************************************************************
void
UIOnboardSwitchDebouncer(unsigned long ulSwitches)
{
    unsigned long ulDelta, ulIdx;

    //
    // Determine the switches that are at a different state than the debounced
    // state.
    //
    ulDelta = ulSwitches ^ g_ulUIOnboardSwitches;

    //
    // Increment the clocks by one.
    //
    g_ulUIOnboardClockA ^= g_ulUIOnboardClockB;
    g_ulUIOnboardClockB = ~g_ulUIOnboardClockB;

    //
    // Reset the clocks corresponding to switches that have not changed state.
    //
    g_ulUIOnboardClockA &= ulDelta;
    g_ulUIOnboardClockB &= ulDelta;

    //
    // Get the new debounced switch state.
    //
    g_ulUIOnboardSwitches &= (g_ulUIOnboardClockA | g_ulUIOnboardClockB);
    g_ulUIOnboardSwitches |= ((~(g_ulUIOnboardClockA | g_ulUIOnboardClockB)) &
                              ulSwitches);

    //
    // Determine the switches that just changed debounced state.
    //
    ulDelta ^= (g_ulUIOnboardClockA | g_ulUIOnboardClockB);

    //
    // Loop through all of the switches.
    //
    for(ulIdx = 0; ulIdx < g_ulUINumButtons; ulIdx++)
    {
        //
        // See if this switch just changed state.
        //
        if(ulDelta & (1 << g_sUISwitches[ulIdx].ucBit))
        {
            //
            // See if the switch was just pressed or released.
            //
            if(!(g_ulUIOnboardSwitches & (1 << g_sUISwitches[ulIdx].ucBit)))
            {
                //
                // The switch was just pressed.  If there is no hold defined
                // and there is a press function to call then call it now.
                //
                if((g_sUISwitches[ulIdx].ulHoldTime == 0) &&
                   g_sUISwitches[ulIdx].pfnPress)
                {
                    g_sUISwitches[ulIdx].pfnPress();
                }

                //
                // Reset the hold count for this button.
                //
                g_pulUIHoldCount[ulIdx] = 0;
            }
            else
            {
                //
                // The switch was just released.  If there is a hold defined,
                // the switch was held for less than the hold time, and there
                // is a press function to call then call it now.
                //
                if(g_sUISwitches[ulIdx].pfnPress &&
                   (g_sUISwitches[ulIdx].ulHoldTime != 0) &&
                   (g_pulUIHoldCount[ulIdx] < g_sUISwitches[ulIdx].ulHoldTime))
                {
                    g_sUISwitches[ulIdx].pfnPress();
                }

                //
                // If there is a release function to call then call it now.
                //
                if(g_sUISwitches[ulIdx].pfnRelease)
                {
                    g_sUISwitches[ulIdx].pfnRelease();
                }
            }
        }

        //
        // See if there is a hold defined for this switch and the switch is
        // pressed.
        //
        if((g_sUISwitches[ulIdx].ulHoldTime != 0) &&
           (!(g_ulUIOnboardSwitches & (1 << g_sUISwitches[ulIdx].ucBit))))
        {
            //
            // Increment the hold count if it is not maxed out.
            //
            if(g_pulUIHoldCount[ulIdx] < 0xffffffff)
            {
                g_pulUIHoldCount[ulIdx]++;
            }

            //
            // If the hold count has reached the hold time for this switch and
            // there is a hold function then call it now.
            //
            if((g_pulUIHoldCount[ulIdx] == g_sUISwitches[ulIdx].ulHoldTime) &&
               g_sUISwitches[ulIdx].pfnHold)
            {
                g_sUISwitches[ulIdx].pfnHold();
            }
        }
    }
}

//*****************************************************************************
//
//! Filters the value of a potentiometer.
//!
//! \param ulValue is the current sample for the potentiometer.
//!
//! This function performs filtering on the sampled value of a potentiometer.
//! First, a single pole IIR low pass filter is applied to the raw sampled
//! value.  Then, the filtered value is examined to determine when the
//! potentiometer is being turned and when it is not.  When the potentiometer
//! is not being turned (and variations in the value are therefore the result
//! of noise in the system), a constant value is returned instead of the
//! filtered value.  When the potentiometer is being turned, the filtered value
//! is returned unmodified.
//!
//! This second filtering step eliminates the flutter when the potentiometer is
//! not being turned so that processes that are driven from its value (such as
//! a motor position) do not result in the motor jiggling back and forth to the
//! potentiometer flutter.  The downside to this filtering is a larger turn
//! of the potentiometer being required before the output value changes.
//!
//! \return Returns the filtered potentiometer value.
//
//*****************************************************************************
unsigned long
UIOnboardPotentiometerFilter(unsigned long ulValue)
{
    //
    // Pass the potentiometer value through a single pole IIR low pass filter
    // (with a coefficient of 0.75).
    //
    g_ulUIOnboardPotValue = ((g_ulUIOnboardPotValue * 3) + ulValue) / 4;

    //
    // Now, pass the potentiometer value through a stable value detector.
    // Start by seeing if the value is stable or changing.
    //
    if(g_ulUIOnboardFilteredPotValue == 0xffffffff)
    {
        //
        // The potenetiometer value is changing.  If the present value is less
        // than the current minimum then save it as the new minimum.
        //
        if(g_ulUIOnboardPotValue < g_ulUIOnboardPotMin)
        {
            g_ulUIOnboardPotMin = g_ulUIOnboardPotValue;
        }

        //
        // If the present value is greater than the current maximum then save
        // it as the new maximum.
        //
        if(g_ulUIOnboardPotValue > g_ulUIOnboardPotMax)
        {
            g_ulUIOnboardPotMax = g_ulUIOnboardPotValue;
        }

        //
        // Add the present value into the accumulator.
        //
        g_ulUIOnboardPotSum += g_ulUIOnboardPotValue;

        //
        // Increment the count of samples in the accumulator.
        //
        g_ulUIOnboardPotCount++;

        //
        // See if there are sixteen samples in the accumulator.
        //
        if(g_ulUIOnboardPotCount == 16)
        {
            //
            // If the range from minimum to maximum for the last sixteen
            // samples is less than ten then the potentiometer value has become
            // stable.
            //
            if((g_ulUIOnboardPotMax - g_ulUIOnboardPotMin) < 10)
            {
                //
                // Compute the stable potentiometer value based on the average
                // of the previous sixteen samples.
                //
                g_ulUIOnboardFilteredPotValue = g_ulUIOnboardPotSum / 16;
            }

            //
            // Reset the minimum, maximum, accumulator, and sample count
            // variables.
            //
            g_ulUIOnboardPotMin = 0xffffffff;
            g_ulUIOnboardPotMax = 0;
            g_ulUIOnboardPotSum = 0;
            g_ulUIOnboardPotCount = 0;
        }
    }
    else
    {
        //
        // The potentiometer value is stable.  If the present value is
        // different than the stable value by more than ten then the value
        // is considered to be changing.
        //
        if(((g_ulUIOnboardFilteredPotValue < g_ulUIOnboardPotValue) &&
            ((g_ulUIOnboardPotValue - g_ulUIOnboardFilteredPotValue) > 10)) ||
           ((g_ulUIOnboardFilteredPotValue > g_ulUIOnboardPotValue) &&
            ((g_ulUIOnboardFilteredPotValue - g_ulUIOnboardPotValue) > 10)))
        {
            g_ulUIOnboardFilteredPotValue = 0xffffffff;
        }
    }

    //
    // If the potentiometer value is stable then return the averaged value.  If
    // it is changing then return the output of the low pass filter.
    //
    return((g_ulUIOnboardFilteredPotValue == 0xffffffff) ?
           g_ulUIOnboardPotValue : g_ulUIOnboardFilteredPotValue);
}

//*****************************************************************************
//
//! Initializes the on-board user interface elements.
//!
//! \param ulSwitches is the initial state of the switches.
//! \param ulPotentiometer is the initial state of the potentiometer.
//!
//! This function initializes the internal state of the on-board user interface
//! handlers.  The initial state of the switches are used to avoid spurious
//! switch presses/releases, and the initial state of the potentiometer is used
//! to make the filtered potentiometer value track more accurately when first
//! starting (after a short period of time it will track correctly regardless
//! of the initial state).
//!
//! \return None.
//
//*****************************************************************************
void
UIOnboardInit(unsigned long ulSwitches, unsigned long ulPotentiometer)
{
    //
    // Set the default debounced switch state based on the provided switch
    // value.
    //
    g_ulUIOnboardSwitches = ulSwitches;

    //
    // Set the default filtered value of the potentiometer based on the
    // provided value.
    //
    g_ulUIOnboardPotValue = ulPotentiometer;

    //
    // Initialize the internal state of the stable value detector.
    //
    g_ulUIOnboardFilteredPotValue = 0xffffffff;
    g_ulUIOnboardPotMin = 0xffffffff;
    g_ulUIOnboardPotMax = 0;
    g_ulUIOnboardPotSum = 0;
    g_ulUIOnboardPotCount = 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
