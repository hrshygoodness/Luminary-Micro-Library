//*****************************************************************************
//
// hall_ctrl.c - Routines to support use of the Hall Sensor inputs.
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
// This is part of revision 10636 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "hall_ctrl.h"
#include "main.h"
#include "pins.h"
#include "trapmod.h"
#include "ui.h"

//*****************************************************************************
//
//! \page hall_ctrl_intro Introduction
//!
//! Brushless DC motors may be configured with Hall sensors.  These sensors
//! are used to determine motor speed and position.
//!
//! In this module, the Hall sensor input edges are monitored to determine
//! the current Hall state value (position), and to determine motor speed.
//!
//! The Hall sensor inputs should be connected to GPIO inputs on the BLDC RDK
//! input connected (Hall A, B, and C).  These inputs are configured as GPIO
//! inputs, and configured to generate interrupts on both rising and falling
//! edges.
//!
//! The Hall state value is stored at each interrupt.   The time between the
//! interrupt edges is measured to determine the speed of the motor.
//!
//! The code for calculating the motor speed and updating the Hall state value
//! is contained in <tt>hall_ctrl.c</tt>, with <tt>hall_ctrl.h</tt> containing
//! the definitions for the variable and functions exported to the remainder
//! of the application.
//!
//! \note  If the Hall sensors are configured as Linear Hall sensors, refer
//! to the code in <tt>adc_ctrl.c</tt> for details about the processing of
//! linear Hall sensor input data.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup hall_ctrl_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! A bit-mapped flag of Hall edges to skip before starting a speed
//! calculations for a given Hall edge.
//
//*****************************************************************************
static unsigned char g_ucSkipFlag = 0xff;

//*****************************************************************************
//
//! This is the time at which the previous edge was seen and is used to
//! determine the time between edges.
//
//*****************************************************************************
static unsigned long g_ulOldTime[8];

//*****************************************************************************
//
//! The current speed of the motor's rotor.
//
//*****************************************************************************
unsigned long g_ulHallRotorSpeed = 0;

//*****************************************************************************
//
//! The current Hall Sensor value.
//
//*****************************************************************************
unsigned long g_ulHallValue = 0;

//*****************************************************************************
//
//! Updates the current rotor speed.
//!
//! \param ulNewSpeed is the newly measured speed.
//!
//! This function takes a newly measured rotor speed and uses it to update
//! the current rotor speed.  If the new speed is different from the
//! current speed by too large a margin, the new speed measurement is
//! discarded (a noise filter).  If the new speed is accepted, it is passed
//! through a single-pole IIR low pass filter with a coefficient of 0.75.
//!
//! \return None.
//
//*****************************************************************************
static void
HallSpeedNewValue(unsigned long ulNewSpeed)
{
    unsigned long ulDelta;

    //
    // Compute the difference in the speed.
    //
    if(ulNewSpeed > g_ulHallRotorSpeed)
    {
        ulDelta = ulNewSpeed - g_ulHallRotorSpeed;
    }
    else
    {
        ulDelta = g_ulHallRotorSpeed - ulNewSpeed;
    }

    //
    // If the speed difference is too large then return without updating the
    // motor speed.
    //
    if(ulDelta > (g_sParameters.ulMaxSpeed / 2))
    {
        return;
    }

    //
    // Pass the new rotor speed reading through the low pass filter.
    //
    g_ulHallRotorSpeed = ((g_ulHallRotorSpeed * 3) + ulNewSpeed) / 4;
}

//*****************************************************************************
//
//! Handles the GPIO port B interrupt.
//!
//! This function is called when GPIO port B asserts its interrupt.  GPIO port
//! B is configured to generate an interrupt on both the rising and falling
//! edges of the Hall sensor input signals.
//!
//! \return None.
//
//*****************************************************************************
void
GPIOBIntHandler(void)
{
    unsigned long ulTime, ulNewTime, ulTemp;

    //
    // Get the time of this edge.
    //
    ulNewTime = UIGetTicks();

    //
    // Clear the Hall GPIO pin interrupts.
    //
    GPIOPinIntClear(PIN_HALLA_PORT, (PIN_HALLA_PIN | PIN_HALLB_PIN |
                    PIN_HALLC_PIN));

    //
    // Punch the watchdog timer.
    //
    MainPunchWatchdog();

    //
    // Read the current Hall sensor data.
    //
    g_ulHallValue = GPIOPinRead(PIN_HALLA_PORT,
            (PIN_HALLC_PIN | PIN_HALLB_PIN | PIN_HALLA_PIN)) >> 4;

    //
    // Invert the Hall Sensor value, if necessary.
    //
    if(HWREGBITH(&(g_sParameters.usFlags),
        FLAG_SENSOR_POLARITY_BIT) == FLAG_SENSOR_POLARITY_LOW)
    {
        g_ulHallValue = g_ulHallValue ^ 0x07;
    }

    //
    // Update the output waveform if running Trapezoid modulation.
    //
    if(g_sParameters.ucModulationType == MOD_TYPE_TRAPEZOID)
    {
        TrapModulate(g_ulHallValue);
    }

    //
    // See if this edge should be skipped.
    //
    if(HWREGBITW(&g_ucSkipFlag, g_ulHallValue))
    {
        //
        // This edge should be skipped, but an edge time now exists so the
        // next edge should not be skipped.
        //
        HWREGBITW(&g_ucSkipFlag, g_ulHallValue) = 0;

        //
        // Save the time of the current edge.
        //
        g_ulOldTime[g_ulHallValue] = ulNewTime;

        //
        // There is nothing further to be done.
        //
        return;
    }

    //
    // Compute the time between this edge and the previous edge.
    //
    ulTime = ulNewTime - g_ulOldTime[g_ulHallValue];

    //
    // Save the time of the current edge.
    //
    g_ulOldTime[g_ulHallValue] = ulNewTime;

    //
    // Compute the new speed from the time between edges.
    //
    ulTemp = (SYSTEM_CLOCK * 60U);
    ulTemp = (ulTemp / ulTime);
    ulTemp = (ulTemp / (g_sParameters.ucNumPoles / 2));
    HallSpeedNewValue(ulTemp);
}

//*****************************************************************************
//
//! Handles the Hall System Tick.
//!
//! This function is called by the system tick handler.  It's primary
//! purpose is to reset the motor speed to 0 if no Hall interrupt edges have
//! been detected for some period of time.
//!
//! \return None.
//
//*****************************************************************************
void
HallTickHandler(void)
{
    //
    // If the motor is NOT running, then force a skip of the speed
    // calculation code for the next time that the motor is running,
    // and also force the Rotor Speed to 0.
    //
    if(!MainIsRunning())
    {
        g_ucSkipFlag = 0xff;
        g_ulHallRotorSpeed = 0;
    }
}

//*****************************************************************************
//
//! Initializes the Hall sensor control routines.
//!
//! This function will initialize the peripherals used determine the speed of
//! the motor's rotor.
//!
//! \return None.
//
//*****************************************************************************
void
HallInit(void)
{
    //
    // Configure the Hall effect GPIO pins as inputs.
    //
    GPIOPinTypeGPIOInput(PIN_HALLA_PORT, (PIN_HALLA_PIN | PIN_HALLB_PIN |
                         PIN_HALLC_PIN));

    //
    // Configure the Hall effect GPIO pins as interrupts on both edges.
    //
    GPIOIntTypeSet(PIN_HALLA_PORT, (PIN_HALLA_PIN | PIN_HALLB_PIN |
                   PIN_HALLC_PIN), GPIO_BOTH_EDGES);
}

//*****************************************************************************
//
//! Configure the Hall sensor control routines, based on motor drive
//! parameters.
//!
//! This function will configure the Hall sensor routines, mainly
//! enable/disable the Hall interrupt based on the motor drive configuration.
//!
//! \return None.
//
//*****************************************************************************
void
HallConfigure(void)
{
    //
    // If running in sensorless mode, or in Linear Hall sensor configuration,
    // the Hall Sensor Interrupts should be disabled.
    //
    if((g_sParameters.ucModulationType == MOD_TYPE_SENSORLESS) ||
        (HWREGBITH(&(g_sParameters.usFlags), FLAG_SENSOR_TYPE_BIT) ==
                   FLAG_SENSOR_TYPE_LINEAR))
    {
        //
        // Disable the GPIO interrupt for Hall sensors.
        //
        IntDisable(INT_GPIOB);

        //
        // Disable the individual hall sensor interrupts.
        //
        GPIOPinIntDisable(PIN_HALLA_PORT, (PIN_HALLA_PIN | PIN_HALLB_PIN |
                          PIN_HALLC_PIN));

        //
        // And we're done.
        //
        return;
    }

    //
    // Clear any pending Hall GPIO pin interrupts.
    //
    GPIOPinIntClear(PIN_HALLA_PORT, (PIN_HALLA_PIN | PIN_HALLB_PIN |
                    PIN_HALLC_PIN));

    //
    // (Re)Enable the Hall effect GPIO pin interrupts.
    //
    GPIOPinIntEnable(PIN_HALLA_PORT, (PIN_HALLA_PIN | PIN_HALLB_PIN |
                     PIN_HALLC_PIN));

    //
    // (Re)Enable the Hall GPIO interrupt.
    //
    IntEnable(INT_GPIOB);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
