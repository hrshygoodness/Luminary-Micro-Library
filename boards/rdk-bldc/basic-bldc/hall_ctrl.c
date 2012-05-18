//*****************************************************************************
//
// hall_ctrl.c - Routines to support use of the Hall Sensor inputs.
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
// \page hall_ctrl_intro Introduction
//
// Brushless DC motors may be configured with Hall sensors.  These sensors
// are used to determine motor speed and position.
//
// In this module, the Hall sensor input edges are monitored to determine
// the current Hall state value (position), and to determine motor speed.
//
// The Hall sensors inputs should be connected to GPIO inputs on the BLDC RDK
// input connected (Hall A, B, and C).  These inputs are configured as GPIO
// inputs, and configured to generate interrupts on both rising and falling
// edges.
//
// The Hall state value is stored at each interrupt.   The time between the
// interrupt edges is measured to determine the speed of the motor.
//
// The code for calculating the motor speed and updating the Hall state value
// is contained in <tt>hall_ctrl.c</tt>, with <tt>hall_ctrl.h</tt> containing
// the definitions for the variable and functions exported to the remainder
// of the application.
//
// \note  If the Hall sensors are configured as Linear Hall sensors, refer
// to the code in <tt>adc_ctrl.c</tt> for details about the processing of
// linear Hall sensor input data.
//
//*****************************************************************************

//*****************************************************************************
//
// \defgroup hall_ctrl_api Definitions
// @{
//
//*****************************************************************************

//*****************************************************************************
//
// The bit number of the flag in #g_ulHallSpeedFlags that indicates that the
// next edge should be ignored by the speed calculation code.  This is used
// at startup since there is no previous edge time to be used to calculate
// the time between edges.
//
//*****************************************************************************
#define FLAG_SKIP_BIT           0

//*****************************************************************************
//
// The bit number of the flag in #g_ulHallSpeedFlags that indicates that an
// edge has been seen.  If an edge hasn't been seen during a Hall timer
// interrupt period, the speed is forced to zero.
//
//*****************************************************************************
#define FLAG_EDGE_BIT           1

//*****************************************************************************
//
// A set of flags that provide status and control of the Hall Control
// module.
//
//*****************************************************************************
static unsigned long g_ulHallSpeedFlags = (1 << FLAG_SKIP_BIT);

//*****************************************************************************
//
// This is the time at which the previous edge was seen and is used to
// determine the time between edges.
//
//*****************************************************************************
static unsigned long g_ulHallSpeedPrevious;

//*****************************************************************************
//
// The current speed of the motor's rotor.
//
//*****************************************************************************
unsigned long g_ulHallRotorSpeed = 0;

//*****************************************************************************
//
// The current Hall Sensor value.
//
//*****************************************************************************
unsigned long g_ulHallValue = 0;

//*****************************************************************************
//
// Updates the current rotor speed.
//
// \param ulNewSpeed is the newly measured speed.
//
// This function takes a newly measured rotor speed and uses it to update
// the current rotor speed.  If the new speed is different from the
// current speed by too large a margin, the new speed measurement is
// discarded (a noise filter).  If the new speed is accepted, it is passed
// through a single-pole IIR low pass filter with a coefficient of 0.75.
//
// \return None.
//
//*****************************************************************************
static void
HallSpeedNewValue(unsigned long ulNewSpeed)
{
    unsigned short usDelta;

    //
    // Compute the difference in the speed.
    //
    if(ulNewSpeed > g_ulHallRotorSpeed)
    {
        usDelta = ulNewSpeed - g_ulHallRotorSpeed;
    }
    else
    {
        usDelta = g_ulHallRotorSpeed - ulNewSpeed;
    }

    //
    // If the speed difference is too large then return without updating the
    // motor speed.
    //
    if(usDelta > (UI_PARAM_MAX_SPEED / 2))
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
// Handles the GPIO port B interrupt.
//
// This function is called when GPIO port B asserts its interrupt.  GPIO port
// B is configured to generate an interrupt on both the rising and falling
// edges of the Hall sensor input signals.
//
// \return None.
//
//*****************************************************************************
void
GPIOBIntHandler(void)
{
    unsigned long ulTime, ulNewTime, ulTemp;
    static unsigned long ulLastHall = 1;

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
    // Read the current Hall sensor data.
    //
    g_ulHallValue = GPIOPinRead(PIN_HALLA_PORT,
            (PIN_HALLC_PIN | PIN_HALLB_PIN | PIN_HALLA_PIN)) >> 4;

    //
    // Invert the Hall Sensor value, if necessary.
    //
    if(UI_PARAM_SENSOR_POLARITY == SENSOR_POLARITY_LOW)
    {
        g_ulHallValue = g_ulHallValue ^ 0x07;
    }

    //
    // Update the output waveform if running Trapezoid modulation.
    //
    if(UI_PARAM_MODULATION == MODULATION_TRAPEZOID)
    {
        TrapModulate(g_ulHallValue);
    }

    //
    // Set the flag to indicate that we have seen an edge.
    // 
    HWREGBITW(&g_ulHallSpeedFlags, FLAG_EDGE_BIT) = 1;

    //
    // Check for rising edge of Hall A.
    //
    if(((ulLastHall & 1) == 0) && ((g_ulHallValue & 1) == 1))
    {
        //
        // See if this edge should be skipped.
        //
        if(HWREGBITW(&g_ulHallSpeedFlags, FLAG_SKIP_BIT))
        {
            //
            // This edge should be skipped, but an edge time now exists so the
            // next edge should not be skipped.
            //
            HWREGBITW(&g_ulHallSpeedFlags, FLAG_SKIP_BIT) = 0;

            //
            // Save the time of the current edge.
            //
            g_ulHallSpeedPrevious = ulNewTime;

            //
            // Save the Hall state data.
            //
            ulLastHall = g_ulHallValue;

            //
            // There is nothing further to be done.
            //
            return;
        }

        //
        // Compute the time between this edge and the previous edge.
        //
        ulTime = ulNewTime - g_ulHallSpeedPrevious;

        //
        // Save the time of the current edge.
        //
        g_ulHallSpeedPrevious = ulNewTime;

        //
        // Compute the new speed from the time between edges.
        //
        ulTemp = (SYSTEM_CLOCK * 60U);
        ulTemp = (ulTemp / ulTime);
        ulTemp = (ulTemp / (UI_PARAM_NUM_POLES / 2));
        HallSpeedNewValue(ulTemp);
    }

    //
    // Save the Hall state data.
    //
    ulLastHall = g_ulHallValue;
}

//*****************************************************************************
//
// Handles the Hall System Tick.
//
// This function is called by the system tick handler.  It's primary
// purpose is to reset the motor speed to 0 if no Hall interrupt edges have
// been detected for some period of time.
//
// \return None.
//
//*****************************************************************************
void
HallTickHandler(void)
{
    //
    // See if an edge was seen during this tick period.
    //
    if(HWREGBITW(&g_ulHallSpeedFlags, FLAG_EDGE_BIT) == 1)
    {
        //
        // An edge was seen, so clear the flag so the next period can be
        // checked as well.
        //
        HWREGBITW(&g_ulHallSpeedFlags, FLAG_EDGE_BIT) = 0;

        //
        // There is nothing more to do here, so return.
        //
        return;
    }

    //
    // Check to see if time since the last edge is to large.
    //
    if((UIGetTicks() - g_ulHallSpeedPrevious) > (SYSTEM_CLOCK / 5))
    {
        //
        // No edge was seen, so set the rotor speed to zero.
        //
        g_ulHallRotorSpeed = 0;

        //
        // Since the amount of time the rotor is stopped is indeterminate,
        // skip the first edge when the rotor starts rotating again.
        //
        HWREGBITW(&g_ulHallSpeedFlags, FLAG_SKIP_BIT) = 1;
    }
}

//*****************************************************************************
//
// Initializes the Hall sensor control routines.
//
// This function will initialize the peripherals used determine the speed of
// the motor's rotor.
//
// \return None.
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
// Configure the Hall sensor control routines, based on motor drive
// parameters.
//
// This function will configure the Hall sensor routines, mainly enable/
// disable the Hall interrupt based on the motor drive configuration.
//
// \return None.
//
//*****************************************************************************
void
HallConfigure(void)
{
    //
    // If running in sensorless mode, or in Linear Hall sensor configuration,
    // the Hall Sensor Interrupts should be disabled.
    //
    if((UI_PARAM_MODULATION == MODULATION_SENSORLESS) ||
       (UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_LINEAR) ||
       (UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_LINEAR_60))
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
// @}
//
//*****************************************************************************
