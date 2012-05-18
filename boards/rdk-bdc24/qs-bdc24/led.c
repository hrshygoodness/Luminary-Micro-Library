//*****************************************************************************
//
// led.c - Driver for the LED.
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
#include "shared/can_proto.h"
#include "constants.h"
#include "controller.h"
#include "led.h"
#include "limit.h"
#include "param.h"
#include "pins.h"

//*****************************************************************************
//
// The possible states of the LED state machine.
//
//*****************************************************************************
typedef enum
{
    //
    // The initial state of the LED.
    //
    LED_STATE_INITIAL,

    //
    // The LED is indicating that the motor is running in full forward mode.
    //
    LED_STATE_FORWARD_FULL,

    //
    // The LED is indicating that the motor is running in proportional forward
    // mode.
    //
    LED_STATE_FORWARD,

    //
    // The LED is indicating that the motor is in neutral.
    //
    LED_STATE_NEUTRAL,

    //
    // The LED is indicating that the motor is running in proportional reverse
    // mode.
    //
    LED_STATE_REVERSE,

    //
    // The LED is indicating that the motor is running in full reverse mode.
    //
    LED_STATE_REVERSE_FULL,

    //
    // The LED is indicating that the motor is attempting to run but unable to
    // because of a limit switch.
    //
    LED_STATE_LIMIT_FAULT,

    //
    // The LED is indicating that there is no control link.
    //
    LED_STATE_LINK_BAD,

    //
    // The LED is indicating that a fault condition was detected.
    //
    LED_STATE_FAULT,

    //
    // The LED is indicating status information (parameter reset, calibration
    // success, or calibration failure).
    //
    LED_STATE_DELAY,

    //
    // The LED is indicating that a device ID has not been assigned to the
    // controller.
    //
    LED_STATE_NO_ID,

    //
    // The LED is indicating that it is ready to accept a device ID assignment
    // (if the user pressed the button).
    //
    LED_STATE_ASSIGN,

    //
    // The LED is indicating the current device ID assignment.
    //
    LED_STATE_BLINK_ID,

    //
    // The LED is indicating that servo input calibration mode is active.
    //
    LED_STATE_CALIBRATE
}
tLEDState;

//*****************************************************************************
//
// The bit number of the flag in #g_ulLEDFlags that indicates that the
// controller's parameters were reset to the default values.
//
//*****************************************************************************
#define LED_FLAG_PARAM_RESET    0

//*****************************************************************************
//
// The bit number of the flag in #g_ulLEDFlags that indicates that device ID
// assignment state has started.
//
//*****************************************************************************
#define LED_FLAG_ASSIGN_START   1

//*****************************************************************************
//
// The bit number of the flag in #g_ulLEDFlags that indicates that device ID
// assignment state has ended.
//
//*****************************************************************************
#define LED_FLAG_ASSIGN_STOP    2

//*****************************************************************************
//
// The bit number of the flag in #g_ulLEDFlags that indicates that the current
// device ID should be blinked on the LED.
//
//*****************************************************************************
#define LED_FLAG_BLINK_ID       3

//*****************************************************************************
//
// The bit number of the flag in #g_ulLEDFlags that indicates that the servo
// input calibration mode has started.
//
//*****************************************************************************
#define LED_FLAG_CAL_START      4

//*****************************************************************************
//
// The bit number of the flag in #g_ulLEDFlags that indicates that the servo
// input calibration mode was successful.
//
//*****************************************************************************
#define LED_FLAG_CAL_SUCCESS    5

//*****************************************************************************
//
// The bit number of the flag in #g_ulLEDFlags that indicates that the servo
// input calibration mode failed.
//
//*****************************************************************************
#define LED_FLAG_CAL_FAIL       6

//*****************************************************************************
//
// This indicates that the LED should be black (in other words, both diodes
// turned off).
//
//*****************************************************************************
#define BLACK                   0

//*****************************************************************************
//
// This indicates that the LED should be red (in other words, the red diode
// turned on and the green diode turned off).
//
//*****************************************************************************
#define RED                     1

//*****************************************************************************
//
// This indicates that the LED should be green (in other words, the green diode
// turned on and the red diode turned off).
//
//*****************************************************************************
#define GREEN                   2

//*****************************************************************************
//
// This indicates that the LED should be amber (in other words, both diodes
// turned on).
//
//*****************************************************************************
#define AMBER                   3

//*****************************************************************************
//
// This function converts a time in milliseconds into the corresponding number
// of system ticks.
//
//*****************************************************************************
#define MS_TO_TICKS(ulMS)       (((ulMS) * UPDATES_PER_SECOND) / 1000)

//*****************************************************************************
//
// This function sets the LED to a particular color.  The LED will stay this
// color until changed.
//
//*****************************************************************************
#define SOLID(ulState)                                                        \
        do                                                                    \
        {                                                                     \
            g_ulLEDOnCount = MS_TO_TICKS(100);                                \
            g_ulLEDOnState = ulState;                                         \
            g_ulLEDOffCount = MS_TO_TICKS(100);                               \
            g_ulLEDOffState = ulState;                                        \
            g_ulLEDCount = 0;                                                 \
        }                                                                     \
        while(0)

//*****************************************************************************
//
// This function sets the LED to blink at the specified rate between two
// colors.  The LED will continue blinking between these two colors until
// changed.
//
//*****************************************************************************
#define BLINK(ulOnMS, ulOnState, ulOffMS, ulOffState)                         \
        do                                                                    \
        {                                                                     \
            g_ulLEDOnCount = MS_TO_TICKS(ulOnMS);                             \
            g_ulLEDOnState = ulOnState;                                       \
            g_ulLEDOffCount = MS_TO_TICKS(ulOffMS);                           \
            g_ulLEDOffState = ulOffState;                                     \
            g_ulLEDCount = 0;                                                 \
        }                                                                     \
        while(0)

//*****************************************************************************
//
// The current state of the LED state machine.
//
//*****************************************************************************
static tLEDState g_eLEDState = LED_STATE_INITIAL;

//*****************************************************************************
//
// The number of system ticks before the state of the LED is changed.  The MSB
// indicates if the LED is current in the ``on'' state or the ``off' state.
//
//*****************************************************************************
static unsigned long g_ulLEDCount = 0;

//*****************************************************************************
//
// The number of system ticks that the LED should be in the ``on'' state.
//
//*****************************************************************************
static unsigned long g_ulLEDOnCount = 0;

//*****************************************************************************
//
// The ``on'' state for the LED.
//
//*****************************************************************************
static unsigned long g_ulLEDOnState = BLACK;

//*****************************************************************************
//
// The number of system ticks that the LED should be in the ``off'' state.
//
//*****************************************************************************
static unsigned long g_ulLEDOffCount = 0;

//*****************************************************************************
//
// The ``off'' state for the LED.
//
//*****************************************************************************
static unsigned long g_ulLEDOffState = BLACK;

//*****************************************************************************
//
// The current set of LED flags, which are used to indicate when the LED state
// machine needs to transition to a new state.
//
//*****************************************************************************
static unsigned long g_ulLEDFlags = 0;

//*****************************************************************************
//
// The number of system ticks until the LED state machine automatically
// transitions to a new state.  This is only used by the LED_STATE_DELAY and
// LED_STATE_BLINK_ID states.
//
//*****************************************************************************
static unsigned long g_ulLEDModeCount = 0;

//*****************************************************************************
//
// The device ID that should be blinked on the LED.
//
//*****************************************************************************
static unsigned long g_ulLEDBlinkID = 0;

//*****************************************************************************
//
// This function prepares the LED driver for use by the controller.
//
//*****************************************************************************
void
LEDInit(void)
{
    //
    // Configure the GPIOs as outputs and enable the 8 mA drive for the green
    // LED (but not the red) so that the intensity of the two LEDs match.
    //
    ROM_GPIODirModeSet(LED_RED_PORT, LED_RED_PIN, GPIO_DIR_MODE_OUT);
    ROM_GPIOPadConfigSet(LED_RED_PORT, LED_RED_PIN,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_DIR_MODE_OUT);
    ROM_GPIOPadConfigSet(LED_GREEN_PORT, LED_GREEN_PIN,
                         GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);

    //
    // Turn the LEDs off.
    //
    ROM_GPIOPinWrite(LED_RED_PORT, LED_RED_PIN, 0);
    ROM_GPIOPinWrite(LED_GREEN_PORT, LED_GREEN_PIN, 0);
}

//*****************************************************************************
//
// This function is used to detect if the LED should actually enter
// LED_STATE_NO_ID based on the current link mode.
//
// The function returns whether the function has changed the mode.
//
//*****************************************************************************
static unsigned long
CheckDeviceNumber(unsigned long ulLink)
{
    unsigned long ulCheckLink;

    ulCheckLink = 1;

    if(g_sParameters.ucDeviceNumber == 0)
    {
        //
        // See if the CAN link is being used but no device ID has been
        // assigned.
        //
        if((ulLink == LINK_TYPE_CAN) || (ulLink == LINK_TYPE_UART))
        {
            //
            // Override the link check because we are getting network
            // traffic and thus the lack of ID has high precedence.
            //
            ulCheckLink = 0;

            //
            // Fast blink the LED between amber and black to indicate
            // that no device ID has been assigned.
            //
            BLINK(100, AMBER, 100, BLACK);

            //
            // Go to the no ID state.
            //
            g_eLEDState = LED_STATE_NO_ID;
        }
    }
    return(ulCheckLink);
}

//*****************************************************************************
//
// This function is called on a periodic basis to manage the LED.  The ``on''
// and ``off'' states and times for the LED are adjusted when required based
// on events within the motor controller, and the LED is cycled between the
// ``on'' and ``off'' states in the configured times.
//
// This function should be called \b #UPDATES_PER_SECOND times per second.
//
//*****************************************************************************
void
LEDTick(void)
{
    unsigned long ulLink, ulFaults, ulCheckLink;
    long lVoltage;

    //
    // Get the current link and fault status.
    //
    ulLink = ControllerLinkType();
    ulFaults = ControllerFaultsActive();
    ulCheckLink = 0;

    //
    // See if the parameters have been reset.
    //
    if(HWREGBITW(&g_ulLEDFlags, LED_FLAG_PARAM_RESET) == 1)
    {
        HWREGBITW(&g_ulLEDFlags, LED_FLAG_PARAM_RESET) = 0;

        //
        // Slow blink the LED between red and green to indicate that the
        // parameters have been reset.
        //
        BLINK(500, RED, 500, GREEN);

        //
        // Set the state to delay for five seconds.
        //
        g_ulLEDModeCount = MS_TO_TICKS(5000);
        g_eLEDState = LED_STATE_DELAY;
    }
    //
    // See if device ID assignment mode has started.
    //
    else if(HWREGBITW(&g_ulLEDFlags, LED_FLAG_ASSIGN_START) == 1)
    {
        //
        // Entered Assignment mode.
        //
        HWREGBITW(&g_ulLEDFlags, LED_FLAG_ASSIGN_START) = 0;

        //
        // Slow blink the LED between green and black to indicate that
        // device ID assignment mode is active.
        //
        BLINK(500, GREEN, 250, BLACK);

        //
        // Go to the device ID assignment state.
        //
        g_eLEDState = LED_STATE_ASSIGN;
    }
    //
    // See if the device ID should be indicated on the LED.
    //
    else if(HWREGBITW(&g_ulLEDFlags, LED_FLAG_BLINK_ID) == 1)
    {
        //
        // Entered ID blink mode.
        //
        HWREGBITW(&g_ulLEDFlags, LED_FLAG_BLINK_ID) = 0;

        //
        // Slow blink the LED between amber and black to indicate the
        // device ID.
        //
        BLINK(500, BLACK, 100, AMBER);

        //
        // Go to the blink ID state for an amount of time equal to one
        // blink per device ID.
        //
        g_ulLEDModeCount = g_ulLEDBlinkID * MS_TO_TICKS(600);
        g_eLEDState = LED_STATE_BLINK_ID;
    }
    //
    // See if servo input calibration mode has started.
    //
    else if(HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_START) == 1)
    {
        //
        // Entered calibrate start mode.
        //
        HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_START) = 0;
        HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_SUCCESS) = 0;
        HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_FAIL) = 0;

        //
        // Fast blink the LED between red and green to indicate that
        // servo input calibration mode is active.
        //
        BLINK(100, RED, 100, GREEN);

        //
        // Go to the servo input calibration state.
        //
        g_eLEDState = LED_STATE_CALIBRATE;
    }

    //
    // See if a fault has occurred, ignoring communication faults.  Only make
    // this transition if not in the delay state (do not preempt a status
    // indication with a fault indication).
    //
    if((ulFaults != 0) && (ulFaults != LM_FAULT_COMM) &&
       (g_eLEDState != LED_STATE_LINK_BAD) &&
       (g_eLEDState != LED_STATE_FAULT) && (g_eLEDState != LED_STATE_DELAY))
    {
        //
        // See if this is a current fault.
        //
        if(ulFaults & LM_FAULT_CURRENT)
        {
            //
            // Slow blink the LED between red and yellow to indicate a current
            // fault.
            //
            BLINK(250, AMBER, 500, RED);
        }
        else
        {
            //
            // Slow blink the LED between red and black to indicate a fault
            // condition.
            //
            BLINK(500, RED, 250, BLACK);
        }

        //
        // Go to the fault state.
        //
        g_eLEDState = LED_STATE_FAULT;
    }

    //
    // Check the current state of the LED state machine.
    //
    switch(g_eLEDState)
    {
        //
        // The normal mode of operation.
        //
        case LED_STATE_INITIAL:
        case LED_STATE_FORWARD_FULL:
        case LED_STATE_FORWARD:
        case LED_STATE_NEUTRAL:
        case LED_STATE_REVERSE:
        case LED_STATE_REVERSE_FULL:
        case LED_STATE_LIMIT_FAULT:
        {
            //
            // Check if the device has no ID.
            //
            ulCheckLink = CheckDeviceNumber(ulLink);

            //
            // The function has no device number and should flash no ID LED.
            //
            if(ulCheckLink == 0)
            {
                break;
            }

            //
            // Get the current output voltage.
            //
            lVoltage = ControllerVoltageGet();

            //
            // See if the output voltage is non-zero and is being shut off
            // by the corresponding limit switch.
            //
            if(((lVoltage > 0) && !LimitForwardOK()) ||
               ((lVoltage < 0) && !LimitReverseOK()))
            {
                //
                // See if the LED is in the limit switch fault state.
                //
                if(g_eLEDState != LED_STATE_LIMIT_FAULT)
                {
                    //
                    // Slow blink the LED between red and black to indicate
                    // a fault condition.
                    //
                    BLINK(500, RED, 250, BLACK);

                    //
                    // Go to the limit switch fault state.
                    //
                    g_eLEDState = LED_STATE_LIMIT_FAULT;
                }
            }

            //
            // See if the output voltage is full forward and the LED state
            // is not full forward.
            //
            else if((lVoltage == 32767) &&
                    (g_eLEDState != LED_STATE_FORWARD_FULL))
            {
                //
                // Turn the LED solid green to indicate full forward.
                //
                SOLID(GREEN);

                //
                // Go to the full forward state.
                //
                g_eLEDState = LED_STATE_FORWARD_FULL;
            }

            //
            // See if the output voltage is proportional forward and the
            // LED state is not proportional forward.
            //
            else if((lVoltage > 0) && (lVoltage < 32767) &&
                    (g_eLEDState != LED_STATE_FORWARD))
            {
                //
                // Fast blink the LED between green and black to indicate
                // proportional forward.
                //
                BLINK(100, GREEN, 100, BLACK);

                //
                // Go to the proportional forward state.
                //
                g_eLEDState = LED_STATE_FORWARD;
            }

            //
            // See if the output voltage is neutral and the LED state is
            // not neutral.
            //
            else if((lVoltage == 0) && (g_eLEDState != LED_STATE_NEUTRAL))
            {
                //
                // Turn the LED solid amber to indicate neutral.
                //
                SOLID(AMBER);

                //
                // Go to the neutral state.
                //
                g_eLEDState = LED_STATE_NEUTRAL;
            }

            //
            // See if the output voltage is proportional reverse and the
            // LED state is not proportional reverse.
            //
            else if((lVoltage > -32768) && (lVoltage < 0) &&
                    (g_eLEDState != LED_STATE_REVERSE))
            {
                //
                // Fast blink the LED between red and black to indicate
                // proportional reverse.
                //
                BLINK(100, RED, 100, BLACK);

                //
                // Go to the proportional reverse state.
                //
                g_eLEDState = LED_STATE_REVERSE;
            }

            //
            // See if the output voltage is full reverse and the LED state
            // is not full reverse.
            //
            else if((lVoltage == -32768) &&
                    (g_eLEDState != LED_STATE_REVERSE_FULL))
            {
                //
                // Turn the LED solid red to indicate full reverse.
                //
                SOLID(RED);

                //
                // Go to the full reverse state.
                //
                g_eLEDState = LED_STATE_REVERSE_FULL;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The bad control link state.
        //
        case LED_STATE_LINK_BAD:
        {
            //
            // See if there is a control link.
            //
            if(ControllerLinkActive())
            {
                //
                // Turn the LED solid amber to indicate neutral.
                //
                SOLID(AMBER);

                //
                // Go to the neutral state.
                //
                g_eLEDState = LED_STATE_NEUTRAL;
            }
            //
            // Check if the device has no ID.
            //
            else
            {
                //
                // The function has no device number and should flash no ID LED.
                //
                CheckDeviceNumber(ulLink);
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The fault state.
        //
        case LED_STATE_FAULT:
        {
            //
            // See if there is a fault.
            //
            if(ulFaults == 0)
            {
                //
                // Turn the LED solid amber to indicate neutral.
                //
                SOLID(AMBER);

                //
                // Go to the neutral state.
                //
                g_eLEDState = LED_STATE_NEUTRAL;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The delay state, used for indicating status.
        //
        case LED_STATE_DELAY:
        {
            //
            // Decrement the mode count and see if it has reached zero.
            //
            if(--g_ulLEDModeCount == 0)
            {
                //
                // Turn the LED solid amber to indicate neutral.
                //
                SOLID(AMBER);

                //
                // Go to the neutral state.
                //
                g_eLEDState = LED_STATE_NEUTRAL;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The no device ID state.
        //
        case LED_STATE_NO_ID:
        {
            //
            // Only servo mode can force us to leave the NO ID flashing mode.
            //
            if(ulLink == LINK_TYPE_SERVO)
            {
                //
                // Go to the bad link state so that we can wait to restore the
                // the link
                //
                g_eLEDState = LED_STATE_LINK_BAD;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The device ID assignment state.
        //
        case LED_STATE_ASSIGN:
        {
            //
            // See if the device ID assignment mode has ended without the ID
            // being changed.
            //
            if(HWREGBITW(&g_ulLEDFlags, LED_FLAG_ASSIGN_STOP) == 1)
            {
                //
                // Handled leaving assignment mode.
                //
                HWREGBITW(&g_ulLEDFlags, LED_FLAG_ASSIGN_STOP) = 0;

                //
                // Turn the LED solid amber to indicate neutral.
                //
                SOLID(AMBER);

                //
                // Go to the neutral state.
                //
                g_eLEDState = LED_STATE_NEUTRAL;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The blink device ID state.
        //
        case LED_STATE_BLINK_ID:
        {
            //
            // Decrement the mode count and see if it has reached zero.
            //
            if(--g_ulLEDModeCount == 0)
            {
                //
                // Turn the LED solid black.
                //
                SOLID(BLACK);

                //
                // Set the state to delay for one second.
                //
                g_ulLEDModeCount = MS_TO_TICKS(1000);
                g_eLEDState = LED_STATE_DELAY;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The servo input calibration state.
        //
        case LED_STATE_CALIBRATE:
        {
            //
            // See if the servo input calibration mode has completed
            // successfully.
            //
            if(HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_SUCCESS) == 1)
            {
                //
                // Slow blink the LED between green and amber to indicate a
                // successful servo input calibration.
                //
                BLINK(250, GREEN, 250, AMBER);

                //
                // Set the state to delay for five seconds.
                //
                g_ulLEDModeCount = MS_TO_TICKS(5000);
                g_eLEDState = LED_STATE_DELAY;
            }

            //
            // See if the servo input calibration mode has completed
            // unsuccessfully.
            //
            else if(HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_FAIL) == 1)
            {
                //
                // Slow blink the LED between red and amber to indicate a
                // failed servo input calibration.
                //
                BLINK(250, RED, 250, AMBER);

                //
                // Set the state to delay for five seconds.
                //
                g_ulLEDModeCount = MS_TO_TICKS(5000);
                g_eLEDState = LED_STATE_DELAY;
            }

            //
            // This state has been handled.
            //
            break;
        }
    }

    //
    // See if the control link has been lost or has returned.
    //
    if((ulCheckLink != 0) && (ControllerLinkActive() == 0))
    {
        //
        // Slow blink the LED between amber and black to indicate the lack of a
        // control link.
        //
        BLINK(250, BLACK, 500, AMBER);

        //
        // Go to the bad link state.
        //
        g_eLEDState = LED_STATE_LINK_BAD;
    }

    //
    // See if the LED count has reached zero.
    //
    if((g_ulLEDCount & 0x7fffffff) == 0)
    {
        //
        // Toggle the LED state.
        //
        g_ulLEDCount ^= 0x80000000;

        //
        // See if the LED should be turned "on" or "off".
        //
        if(g_ulLEDCount & 0x80000000)
        {
            //
            // Set the LED delay count to the "on" delay.
            //
            g_ulLEDCount |= g_ulLEDOnCount;

            //
            // Set the state of the LEDs to the "on" state.
            //
            ROM_GPIOPinWrite(LED_RED_PORT, LED_RED_PIN,
                             (g_ulLEDOnState & RED) ? LED_RED_PIN : 0);
            ROM_GPIOPinWrite(LED_GREEN_PORT, LED_GREEN_PIN,
                             (g_ulLEDOnState & GREEN) ? LED_GREEN_PIN : 0);
        }
        else
        {
            //
            // Set the LED delay count to the "off" delay.
            //
            g_ulLEDCount |= g_ulLEDOffCount;

            //
            // Set the state of the LEDs to the "off" state.
            //
            ROM_GPIOPinWrite(LED_RED_PORT, LED_RED_PIN,
                             (g_ulLEDOffState & RED) ? LED_RED_PIN : 0);
            ROM_GPIOPinWrite(LED_GREEN_PORT, LED_GREEN_PIN,
                             (g_ulLEDOffState & GREEN) ? LED_GREEN_PIN : 0);
        }
    }
    else
    {
        //
        // Decrement the LED count.
        //
        g_ulLEDCount--;
    }
}

//*****************************************************************************
//
// This function is called to indicate that the parameters have been reset to
// the default values.
//
//*****************************************************************************
void
LEDParameterReset(void)
{
    //
    // Set the parameter reset flag.  This will cause the LED to get updated on
    // the next LED tick.
    //
    HWREGBITW(&g_ulLEDFlags, LED_FLAG_PARAM_RESET) = 1;
}

//*****************************************************************************
//
// This function is called to indicate that the CAN device ID assignment mode
// has started.
//
//*****************************************************************************
void
LEDAssignStart(void)
{
    //
    // Set the assignment mode start flag.  This will cause the LED to get
    // updated on the next LED tick.
    //
    HWREGBITW(&g_ulLEDFlags, LED_FLAG_ASSIGN_START) = 1;
}

//*****************************************************************************
//
// This function is called to indicate that the CAN device ID assignment mode
// has ended.  This should be called if device ID assignment mode exits without
// a new device ID being assigned.
//
//*****************************************************************************
void
LEDAssignStop(void)
{
    //
    // Set the assignment mode stop flag.  This will cause the LED to get
    // updated on the next LED tick.
    //
    HWREGBITW(&g_ulLEDFlags, LED_FLAG_ASSIGN_STOP) = 1;
}

//*****************************************************************************
//
// This function is called to indicate that the CAN device ID should be blinked
// on the LED.  This can be called any time to indicate the device ID, and
// should be called if device ID assignment mode exists with a new device ID
// being assigned.
//
//*****************************************************************************
void
LEDBlinkID(unsigned long ulID)
{
    //
    // Save the ID.
    //
    g_ulLEDBlinkID = ulID;

    //
    // Set the blink ID mode flag.  This will cause the LED to get updated on
    // the next LED tick.
    //
    HWREGBITW(&g_ulLEDFlags, LED_FLAG_BLINK_ID) = 1;
}

//*****************************************************************************
//
// This function is called to indicate that the servo input calibration mode
// has started.
//
//*****************************************************************************
void
LEDCalibrateStart(void)
{
    //
    // Set the calibration mode start flag.  This will cause the LED to get
    // updated on the next LED tick.
    //
    HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_START) = 1;
}

//*****************************************************************************
//
// This function is called to indcate that servo input calibration mode has
// ended and the calibration was successful.
//
//*****************************************************************************
void
LEDCalibrateSuccess(void)
{
    //
    // Set the calibration success flag.  This will cause the LED to get
    // updated on the next LED tick.
    //
    HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_SUCCESS) = 1;
}

//*****************************************************************************
//
// This function is called to indicate that servo input calibration mode has
// ended and the calibration failed.
//
//*****************************************************************************
void
LEDCalibrateFail(void)
{
    //
    // Set the calibration failure flag.  This will cause the LED to get
    // updated on the next LED tick.
    //
    HWREGBITW(&g_ulLEDFlags, LED_FLAG_CAL_FAIL) = 1;
}

//*****************************************************************************
//
// This function is called to indicate that a firmware update is starting.
// This should be called after all interrupts have been disabled.
//
//*****************************************************************************
void
LEDFirmwareUpdate(void)
{
    //
    // Since interrupts have been disabled, and therefore the LED tick will not
    // occur again, directly turn off both of the LEDs.
    //
    ROM_GPIOPinWrite(LED_RED_PORT, LED_RED_PIN, 0);
    ROM_GPIOPinWrite(LED_GREEN_PORT, LED_GREEN_PIN, 0);
}
