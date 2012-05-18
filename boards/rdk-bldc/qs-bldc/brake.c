//*****************************************************************************
//
// brake.c - Dynamic braking control routines.
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
// This is part of revision 8555 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "adc_ctrl.h"
#include "brake.h"
#include "pins.h"
#include "ui.h"

//*****************************************************************************
//
//! \page brake_intro Introduction
//!
//! Dynamic braking is the application of a power resistor across the DC bus in
//! order to control the increase in the DC bus voltage.  The power resistor
//! reduces the DC bus voltage by converting current into heat.
//!
//! The dynamic braking routine is called every millisecond to monitor the DC
//! bus voltage and handle the dynamic brake.  When the DC bus voltage gets too
//! high, the dynamic brake is applied to the DC bus.  When the DC bus voltage
//! drops enough, the dynamic brake is removed.
//!
//! In order to control heat buildup in the power resistor, the amount of time
//! the brake is applied is tracked.  If the brake is applied for too long, it
//! will be forced off for a period of time (regardless of the DC bus voltage)
//! to prevent it from overheating.  The amount of time on and off is tracked
//! as an indirect measure of the heat buildup in the power resistor; the heat
//! increases when on and decreases when off.
//!
//! The code for handling dynamic braking is contained in <tt>brake.c</tt>,
//! with <tt>brake.h</tt> containing the definitions for the functions exported
//! to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup brake_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The dynamic brake is turned off.  The bus voltage going above the trigger
//! level will cause a transition to the #STATE_BRAKE_ON state.
//
//*****************************************************************************
#define STATE_BRAKE_OFF         0

//*****************************************************************************
//
//! The dynamic brake is turned on.  The bus voltage going below the trigger
//! level will cause a transition to the #STATE_BRAKE_OFF state, and the brake
//! being on for too long will cause a transition to #STATE_BRAKE_COOL.
//
//*****************************************************************************
#define STATE_BRAKE_ON          1

//*****************************************************************************
//
//! The dynamic brake is forced off to allow the power resistor to cool.  After
//! the minimum cooling period has expired, an automatic transition to
//! #STATE_BRAKE_OFF will occur if the bus voltage is below the trigger level
//! and to #STATE_BRAKE_ON if the bus voltage is above the trigger level.
//
//*****************************************************************************
#define STATE_BRAKE_COOL        2

//*****************************************************************************
//
//! The current state of the dynamic brake.  Will be one of #STATE_BRAKE_OFF,
//! #STATE_BRAKE_ON, or #STATE_BRAKE_COOL.
//
//*****************************************************************************
static unsigned long g_ulBrakeState;

//*****************************************************************************
//
//! The number of milliseconds that the dynamic brake has been on.  For each
//! brake update period, this is incremented if the brake is on and decremented
//! if it is off.  This effectively represents the heat buildup in the power
//! resistor; when on heat will increase and when off it will decrease.
//
//*****************************************************************************
static unsigned long g_ulBrakeCount;

//*****************************************************************************
//
//! Updates the dynamic brake.
//!
//! This function will update the state of the dynamic brake.  It must be
//! called at the PWM frequency to provide a time base for determining when to
//! turn off the brake to avoid overheating.
//!
//! \return None.
//
//*****************************************************************************
void
BrakeTick(void)
{
    //
    // See if the bus voltage exceeds the voltage required to turn on the
    // dynamic brake.
    //
    if(g_ulBusVoltage >= g_sParameters.ulBrakeOnV)
    {
        //
        // The bus voltage is too high, so see if the brake is currently off.
        //
        if((g_ulBrakeState == STATE_BRAKE_OFF) &&
           (HWREGBITH(&(g_sParameters.usFlags), FLAG_BRAKE_BIT) ==
            FLAG_BRAKE_ON))
        {
            //
            // Turn on the dynamic brake.
            //
            GPIOPinWrite(PIN_BRAKE_PORT, PIN_BRAKE_PIN, 0);

            //
            // Change the brake state to on.
            //
            g_ulBrakeState = STATE_BRAKE_ON;
        }
    }

    //
    // Otherwise, see if the dynamic brake is on and the bus voltage is less
    // than the voltage required to turn it off.
    //
    else if((g_ulBusVoltage < g_sParameters.ulBrakeOffV) &&
            (g_ulBrakeState == STATE_BRAKE_ON))
    {
        //
        // Turn off the dynamic brake.
        //
        GPIOPinWrite(PIN_BRAKE_PORT, PIN_BRAKE_PIN, PIN_BRAKE_PIN);

        //
        // Change the brake state to off.
        //
        g_ulBrakeState = STATE_BRAKE_OFF;
    }

    //
    // See if the dynamic brake is on.
    //
    if(g_ulBrakeState == STATE_BRAKE_ON)
    {
        //
        // Increment the number of brake ticks that the dynamic brake has been
        // on.
        //
        g_ulBrakeCount++;

        //
        // See if the dynamic brake has been on for too long.
        //
        if(g_ulBrakeCount == g_sParameters.ulBrakeMax)
        {
            //
            // Turn off the dynamic brake.
            //
            GPIOPinWrite(PIN_BRAKE_PORT, PIN_BRAKE_PIN, PIN_BRAKE_PIN);

            //
            // Change the brake state to cool to allow the braking resistor to
            // cool off (to avoid overheating).
            //
            g_ulBrakeState = STATE_BRAKE_COOL;
        }
    }

    //
    // Otherwise, see if the dynamic brake tick count is non-zero.
    //
    else if(g_ulBrakeCount != 0)
    {
        //
        // Decrement the number of brake ticks that the dynamic brake has been
        // off.
        //
        g_ulBrakeCount--;

        //
        // See if the dynamic brake is in the cooling state and has cooled off
        // for long enough.
        //
        if((g_ulBrakeState == STATE_BRAKE_COOL) &&
           (g_ulBrakeCount == g_sParameters.ulBrakeCool))
        {
            //
            // The dynamic brake has cooled off enough, so see if the bus
            // voltage exceeds the voltage required to turn it on.
            //
            if(g_ulBusVoltage >= g_sParameters.ulBrakeOnV)
            {
                //
                // Turn on the dynamic brake.
                //
                GPIOPinWrite(PIN_BRAKE_PORT, PIN_BRAKE_PIN, 0);

                //
                // Change the brake state to on.
                //
                g_ulBrakeState = STATE_BRAKE_ON;
            }

            //
            // Otherwise, the voltage is low enough that the brake should
            // remain off.
            //
            else
            {
                //
                // Change the brake state to off.
                //
                g_ulBrakeState = STATE_BRAKE_OFF;
            }
        }
    }
}

//*****************************************************************************
//
//! Initializes the dynamic braking control routines.
//!
//! This function initializes the ADC module and the control routines,
//! preparing them to monitor currents and voltages on the motor drive.
//!
//! \return None.
//
//*****************************************************************************
void
BrakeInit(void)
{
    //
    // Configure the brake control pin as an open-drain output, allowing the
    // signal to float high in the "1" state (Brake disabled).
    //
    GPIOPinTypeGPIOOutputOD(PIN_BRAKE_PORT, PIN_BRAKE_PIN);
    GPIOPinWrite(PIN_BRAKE_PORT, PIN_BRAKE_PIN, PIN_BRAKE_PIN);

    //
    // The initial brake state is off.
    //
    g_ulBrakeState = STATE_BRAKE_OFF;

    //
    // The initial brake count is zero.
    //
    g_ulBrakeCount = 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
