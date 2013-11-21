//*****************************************************************************
//
// blinker.c - LED blinking module.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/gpio.h"

//*****************************************************************************
//
//! \page blinker_intro Introduction
//!
//! The blinker module provides a set of functions and a processing state
//! machine to assist with making LEDs blink in a certain pattern or rate.
//! The LED can be made to blink on for a certain time, then off for a
//! certain time, and can repeat that on-off cycle a number of times.
//!
//! To use the blinker module, a client should initialize the GPIO port
//! pins that will be used. The blinker module does not perform any
//! initializations of GPIO hardware. Then BlinkerInit() should be called
//! once for each LED that will be controlled by this module.
//!
//! Finally, BlinkerStart() can be used to start a blinking pattern on
//! a specific LED, and BlinkerUpdate() can be used to change the pattern
//! on an already blinking LED.
//!
//! The blinker state machine runs by periodic calls to the BlinkHandler()
//! function. The client should call this function on a regular interval
//! to keep the blinkers running.
//!
//! The code for implementing the LED Blinker API is contained in
//! <tt>blinker.c</tt>, with <tt>blinker.h</tt> containing the definitions
//! for the variables and functions exported to the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup blinker_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Defines the number of LED blinker instances that are supported.
//
//*****************************************************************************
#define NUM_BLINKERS    2

//*****************************************************************************
//
//! Defines the states that the LED blinking state machine can be in.
//
//*****************************************************************************
typedef enum
{
    //
    //! The blinker is not doing anything.
    //
    BLINK_IDLE,

    //
    //! The blinker has been started and is in the initial state.
    //
    BLINK_START,

    //
    //! The blinker is turned on and counting down to the end of the
    //! on period.
    //
    BLINK_ON,

    //
    //! The blinker is turned off and counting down to the end of the
    //! off period.
    //
    BLINK_OFF
}
tBlinkState;

//*****************************************************************************
//
//! This structure contains the parameters associated with making an LED
//! blink according to a certain pattern.
//
//*****************************************************************************
typedef struct
{
    //
    //! The state of blinking state machine for this LED.
    //
    tBlinkState eState;

    //
    //! The GPIO port base address for this LED.
    //
    unsigned long ulPort;

    //
    //! The GPIO pin (bit mask) for this LED.
    //
    unsigned long ulPin;

    //
    //! The number of counts remaining for the LED to be on.
    //
    unsigned long ulOnCount;

    //
    //! The number of counts remaining for the LED to be off.
    //
    unsigned long ulOffCount;

    //
    //! The reload value for the on counts.
    //
    unsigned long ulOnLoad;

    //
    //! The reload value for the off counts.
    //
    unsigned long ulOffLoad;

    //
    //! The number of times to repeat the pattern (should be >= 1).
    //
    unsigned long ulRepeat;
}
tBlinker;

//*****************************************************************************
//
//! The blinker instances. There should be one for each LED to be managed
//! by blinker.
//
//*****************************************************************************
tBlinker sBlinker[NUM_BLINKERS] =
{
    { BLINK_IDLE },
    { BLINK_IDLE }
};

//*****************************************************************************
//
//! Initializes a blinker instance.
//!
//! \param ulIdx is the index of the blinker instance. Blinker instances
//!              are managed by the caller.
//! \param ulPort is the base address of the GPIO port for this LED blinker.
//! \param ulPin is the bit mask (one bit set) of the position of the LED
//!              on the GPIO port.
//!
//! This function is used to set up a blinker instance to control a specific
//! GPIO pin (which presumably is driving an LED). The caller specifies the
//! GPIO port, and the pin on that port. The pin is specified as a bit mask,
//! not as the pin number.
//!
//! The blinker instances must be managed by the caller. This module does
//! not keep track of which instances are used.
//!
//! \return None.
//
//*****************************************************************************
void
BlinkInit(unsigned long ulIdx, unsigned long ulPort, unsigned long ulPin)
{
    //
    // Make sure a valid index is specified.
    //
    if(ulIdx >= NUM_BLINKERS)
    {
        return;
    }

    //
    // Initialize the blinker fields according to the port and pin provided.
    //
    sBlinker[ulIdx].ulPort = ulPort;
    sBlinker[ulIdx].ulPin = ulPin;
    sBlinker[ulIdx].ulRepeat = 0;
    sBlinker[ulIdx].eState = BLINK_IDLE;
}

//*****************************************************************************
//
//! Starts a blinker blinking according to the specified pattern.
//!
//! \param ulIdx is the index of the blinker instance. Blinker instances
//!              are managed by the caller.
//! \param ulOn is the number of counts for the LED to be turned on.
//! \param ulOff is the number of counts for the LED to be turned off.
//! \param ulRepeat is the number of times to repeat the on-off pattern
//!                 (should be at least 1).
//!
//! This function starts the LED blinking according to the specified pattern.
//! The LED will be turned on for the number of counts specified by ulOn,
//! then off for the number of counts specified by ulOff. The on-off cycle
//! will repeat for the number of times specified by ulRepeat. ulRepeat
//! must be 1 to get the pattern to blink once.
//!
//! If ulOn is 0, then the LED will just be turned off, likewise it will be
//! turned on if ulOff is 0.
//!
//! The number of counts is the number of times that BlinkHandler() is
//! called by the client.
//!
//! \return None.
//
//*****************************************************************************
void
BlinkStart(unsigned long ulIdx, unsigned long ulOn,
           unsigned long ulOff, unsigned long ulRepeat)
{
    //
    // Make sure a valid index is specified.
    //
    if(ulIdx >= NUM_BLINKERS)
    {
        return;
    }

    //
    // Initialize the blinker fields with the on, off, and repeat values,
    // and set the state machine to the initial state.
    //
    sBlinker[ulIdx].ulOnLoad = ulOn;
    sBlinker[ulIdx].ulOffLoad = ulOff;
    sBlinker[ulIdx].ulRepeat = ulRepeat;
    sBlinker[ulIdx].ulOnCount = ulOn;
    sBlinker[ulIdx].ulOffCount = ulOff;
    sBlinker[ulIdx].eState = BLINK_START;
}

//*****************************************************************************
//
//! Updates the blink on and off counts while the blinker is blinking.
//!
//! \param ulIdx is the index of the blinker instance. Blinker instances
//!              are managed by the caller.
//! \param ulOn is the number of counts for the LED to be turned on.
//! \param ulOff is the number of counts for the LED to be turned off.
//!
//! This function is used to update the on and off counts of a blinker that
//! is already blinking. This can be used in the case when a high repeat
//! count has been used in order to provide the appearance of continuous
//! blinking. Using the update function can change the blinking rate without
//! a visible glitch, which might happen if the BlinkStart() were used again. 
//!
//! \return None.
//
//*****************************************************************************
void
BlinkUpdate(unsigned long ulIdx, unsigned long ulOn, unsigned long ulOff)
{
    //
    // Make sure a valid index is specified.
    //
    if(ulIdx >= NUM_BLINKERS)
    {
        return;
    }

    //
    // Update the on and off reload counters with the new values.
    //
    sBlinker[ulIdx].ulOnLoad = ulOn;
    sBlinker[ulIdx].ulOffLoad = ulOff;
}

//*****************************************************************************
//
//! Runs the blinker state machine through one state. Should be called
//! periodically.
//!
//! This function runs the blinker state machine through one state cycle.
//! It is intended to be called periodically in order to keep the blinker
//! state machine running. One call to this function represents one
//! on or off count as specified by the BlinkStart() or BlinkUpdate()
//! functions.
//!
//! \return None.
//
//*****************************************************************************
void
BlinkHandler(void)
{
    tBlinker *pBlinker;
    unsigned int i;

    //
    // Enter a loop to process all of the blinker instances through
    // the blinking state machine.
    //
    for(i = 0; i < NUM_BLINKERS; i++)
    {
        //
        // Get a handy pointer to the blinker instance.
        //
        pBlinker = &sBlinker[i];

        //
        // Enter the blinker state machine.
        //
        switch(pBlinker->eState)
        {
            //
            // Always remain in the idle state and do nothing until
            // forced out by BlinkStart().
            //
            case BLINK_IDLE:
            {
                break;
            }

            //
            // The BLINK_START state is the initial state when a blinker
            // is started with BlinkStart().
            //
            case BLINK_START:
            {
                //
                // If there are on counts, and a repeat count, then
                // turn the LED on and go to the ON state.
                //
                if(pBlinker->ulOnCount && pBlinker->ulRepeat)
                {
                    pBlinker->ulRepeat--;
                    GPIOPinWrite(pBlinker->ulPort, pBlinker->ulPin,
                                                   pBlinker->ulPin);
                    pBlinker->eState = BLINK_ON;
                }

                //
                // If there are no on counts, but there are off counts,
                // then turn the LED off and go to the OFF state.
                //
                else if(pBlinker->ulOffCount && pBlinker->ulRepeat)
                {
                    GPIOPinWrite(pBlinker->ulPort, pBlinker->ulPin, 0);
                    pBlinker->eState = BLINK_OFF;
                }
                break;
            }

            //
            // In the BLINK_ON state, the LED has been turned on, and will
            // remain in the ON state until the number of on counts has
            // expired. Once expired, then the LED is turned off, and
            // next state is the OFF state.
            //
            case BLINK_ON:
            {
                pBlinker->ulOnCount--;

                //
                // If the on count has expired, and there is an off count
                // then turn the LED off and go to the OFF state.
                //
                if(!pBlinker->ulOnCount && pBlinker->ulOffCount)
                {
                    GPIOPinWrite(pBlinker->ulPort, pBlinker->ulPin, 0);
                    pBlinker->eState = BLINK_OFF;
                }
                break;
            }

            //
            // In the BLINK_OFF state, the LED has been turned off, and will
            // remain in the OFF state until the number of on counts has
            // expired. Once expired, then the LED is turned back on if
            // there is a repeat count, and the next state is the ON state.
            // It remains in this state if the repeat count is expired.
            //
            case BLINK_OFF:
            {
                pBlinker->ulOffCount--;

                //
                // If the off count is expired and there are repeat counts,
                // then turn the LED back on and go to the ON state, repeating
                // the cycle.
                //
                if(!pBlinker->ulOffCount && pBlinker->ulRepeat)
                {
                    pBlinker->ulRepeat--;

                    //
                    // Reload the on and off counts from the reload counters.
                    //
                    pBlinker->ulOnCount = pBlinker->ulOnLoad;
                    pBlinker->ulOffCount = pBlinker->ulOffLoad;
                    GPIOPinWrite(pBlinker->ulPort, pBlinker->ulPin,
                                                   pBlinker->ulPin);
                    pBlinker->eState = BLINK_ON;
                }
                break;
            }

            //
            // The default state should never be used. But if so, then
            // just go back to the IDLE state.
            //
            default:
            {
                pBlinker->eState = BLINK_IDLE;
                break;
            }
        }
    }
}

//*****************************************************************************
//
// Close the Doxyen group.
//! @}
//
//*****************************************************************************
