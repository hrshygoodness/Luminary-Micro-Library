//*****************************************************************************
//
// speed_sense.c - Routines for determining the speed of the motor (if an
//                 encoder is present).
//
// Copyright (c) 2007-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6852 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_qei.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/qei.h"
#include "main.h"
#include "pins.h"
#include "speed_sense.h"
#include "ui.h"

//*****************************************************************************
//
//! \page speed_sense_intro Introduction
//!
//! When running at slow speeds the time between input edges is measured to
//! determine the speed of the rotor (referred to as edge timing mode).  The
//! edge triggering capability of the GPIO module is used for this measurement.
//!
//! When running at higher speeds while using the encoder, the number of edges
//! in a fixed time period are counted to determine the speed of the rotor
//! (referred to as edge count mode).  The velocity capture feature of the
//! quadrature encoder module is used for this measurement.
//!
//! The transition between the two speed capture modes is performed based on
//! the measured speed.  If in edge timing mode, when the edge time gets too
//! small (that is, there are too many edges per second), it will change into
//! edge count mode.  If in edge count mode, when the number of edges in the
//! time period gets too small (that is, there are not enough edges per time
//! period), it will change into edge timing mode.  There is a bit of
//! hysteresis on the changeover point to avoid constantly switching between
//! modes if the rotor is running near the changeover point.
//!
//! The code for sensing the rotor speed is contained in
//! <tt>speed_sense.c</tt>, with <tt>speed_sense.h</tt> containing the
//! definitions for the variable and functions exported to the remainder of the
//! application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup speed_sense_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The rate at which the QEI velocity interrupt occurs.
//
//*****************************************************************************
#define QEI_INT_RATE            50

//*****************************************************************************
//
//! The maximum number of edges per second allowed when using the edge timing
//! mode of speed determination (which is also the minimum number of edges
//! per second allowed when using the edge count mode).
//
//*****************************************************************************
#define MAX_EDGE_COUNT          2000

//*****************************************************************************
//
//! The hysteresis applied to #MAX_EDGE_COUNT when changing between the two
//! speed determination modes.
//
//*****************************************************************************
#define EDGE_DELTA              50

//*****************************************************************************
//
//! The bit number of the flag in #g_ulSpeedFlags that indicates that the next
//! edge should be ignored by the edge timing mode.  This is used when the edge
//! timing mode is first enabled since there is no previous edge time to be
//! used to calculate the time between edges.
//
//*****************************************************************************
#define FLAG_SKIP_BIT           0

//*****************************************************************************
//
//! The bit number of the flag in #g_ulSpeedFlags that indicates that edge
//! counting mode is being used to determine the speed.
//
//*****************************************************************************
#define FLAG_COUNT_BIT          1

//*****************************************************************************
//
//! The bit number of the flag in #g_ulSpeedFlags that indicates that an edge
//! has been seen by the edge timing mode.  If an edge hasn't been seen during
//! a QEI velocity interrupt period, the speed is forced to zero.
//
//*****************************************************************************
#define FLAG_EDGE_BIT           2

//*****************************************************************************
//
//! A set of flags that indicate the current state of the motor speed
//! determination routines.
//
//*****************************************************************************
static unsigned long g_ulSpeedFlags = (1 << FLAG_SKIP_BIT);

//*****************************************************************************
//
//! The time accumulated during the QEI velocity interrupts.  This is used to
//! extend the precision of the QEI timer.
//
//*****************************************************************************
static unsigned long g_ulSpeedTime;

//*****************************************************************************
//
//! In edge timing mode, this is the time at which the previous edge was seen
//! and is used to determine the time between edges.  In edge count mode, this
//! is the count of edges during the previous timing period and is used to
//! average the edge count from two periods.
//
//*****************************************************************************
static unsigned long g_ulSpeedPrevious;

//*****************************************************************************
//
//! The current speed of the motor's rotor.
//
//*****************************************************************************
unsigned long g_ulRotorSpeed = 0;

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
SpeedNewValue(unsigned short ulNewSpeed)
{
    unsigned short usDelta;

    //
    // Compute the difference in the speed.
    //
    if(ulNewSpeed > g_ulRotorSpeed)
    {
        usDelta = ulNewSpeed - g_ulRotorSpeed;
    }
    else
    {
        usDelta = g_ulRotorSpeed - ulNewSpeed;
    }

    //
    // If the speed difference is too large then return without updating the
    // motor speed.
    //
    if(usDelta > (g_sParameters.ulMaxSpeed / 2))
    {
        return;
    }

    //
    // Pass the new rotor speed reading through the low pass filter.
    //
    g_ulRotorSpeed = ((g_ulRotorSpeed * 3) + ulNewSpeed) / 4;
}

//*****************************************************************************
//
//! Handles the GPIO port C interrupt.
//!
//! This function is called when GPIO port C asserts its interrupt.  GPIO port
//! C is configured to generate an interrupt on the rising edge of the encoder
//! input signal.  The time between the current edge and the previous edge is
//! computed and used as a measure of the rotor speed.
//!
//! \return None.
//
//*****************************************************************************
void
GPIOCIntHandler(void)
{
    unsigned long ulTime, ulNewTime;

    //
    // Clear the GPIO interrupt.
    //
    GPIOPinIntClear(PIN_ENCA_PORT, PIN_ENCA_PIN);

    //
    // Punch the watchdog timer.
    //
    MainPunchWatchdog();

    //
    // Get the time of this edge.
    //
    ulNewTime = g_ulSpeedTime + HWREG(QEI0_BASE + QEI_O_TIME);

    //
    // Compute the time between this edge and the previous edge.
    //
    ulTime = ulNewTime - g_ulSpeedPrevious;

    //
    // Save the time of the current edge.
    //
    g_ulSpeedPrevious = ulNewTime;

    //
    // See if this edge should be skipped.
    //
    if(HWREGBITW(&g_ulSpeedFlags, FLAG_SKIP_BIT))
    {
        //
        // This edge should be skipped, but an edge time now exists so the next
        // edge should not be skipped.
        //
        HWREGBITW(&g_ulSpeedFlags, FLAG_SKIP_BIT) = 0;

        //
        // There is nothing further to be done.
        //
        return;
    }

    //
    // Indicate that an edge has been seen to prevent the QEI interrupt handler
    // from forcing the rotor speed to zero.
    //
    HWREGBITW(&g_ulSpeedFlags, FLAG_EDGE_BIT) = 1;

    //
    // Compute the new speed from the time between edges.
    //
    SpeedNewValue(((unsigned long)SYSTEM_CLOCK * (unsigned long)60) /
                  (ulTime * (g_sParameters.usNumEncoderLines + 1)));

    //
    // See if the edge time has become too small, meaning that the number of
    // edges per second is too large.
    //
    if(ulTime < (SYSTEM_CLOCK / (MAX_EDGE_COUNT + EDGE_DELTA)))
    {
        //
        // Edge counting mode should be used instead of edge timing mode.
        //
        HWREGBITW(&g_ulSpeedFlags, FLAG_COUNT_BIT) = 1;

        //
        // Disable the GPIO interrupt while using edge counting mode.
        //
        IntDisable(INT_GPIOC);

        //
        // Indicate that the first timing period should be skipped in edge
        // count mode.
        //
        HWREGBITW(&g_ulSpeedFlags, FLAG_SKIP_BIT) = 1;
    }
}

//*****************************************************************************
//
//! Handles the QEI velocity interrupt.
//!
//! This function is called when the QEI velocity timer expires.  If using the
//! edge counting mode for rotor speed determination, the number of edges
//! counted during the last velocity period is used as a measure of the rotor
//! speed.
//!
//! \return None.
//
//*****************************************************************************
void
QEIIntHandler(void)
{
    unsigned long ulPrev, ulCount;

    //
    // Clear the QEI interrupt.
    //
    QEIIntClear(QEI0_BASE, QEI_INTTIMER);

    //
    // Increment the accumulated time to extend the range of the QEI timer,
    // which is used by the edge timing mode.
    //
    g_ulSpeedTime += SYSTEM_CLOCK / QEI_INT_RATE;

    //
    // See if edge counting mode is enabled.
    //
    if(HWREGBITW(&g_ulSpeedFlags, FLAG_COUNT_BIT) == 0)
    {
        //
        // Edge timing mode is currenting operating, so see if an edge was seen
        // during this QEI timing period.
        //
        if(HWREGBITW(&g_ulSpeedFlags, FLAG_EDGE_BIT) == 0)
        {
            //
            // No edge was seen, so set the rotor speed to zero.
            //
            g_ulRotorSpeed = 0;

            //
            // Since the amount of time the rotor is stopped is indeterminate,
            // skip the first edge when the rotor starts rotating again.
            //
            HWREGBITW(&g_ulSpeedFlags, FLAG_SKIP_BIT) = 1;
        }
        else
        {
            //
            // An edge was seen, so clear the flag so the next period can be
            // checked as well, and restart the edge reset counter.
            //
            HWREGBITW(&g_ulSpeedFlags, FLAG_EDGE_BIT) = 0;
        }

        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // Get the number of edges during the most recent period.
    //
    ulCount = QEIVelocityGet(QEI0_BASE);

    if(ulCount)
    {
        //
        // Punch the watchdog timer.
        //
        MainPunchWatchdog();
    }

    //
    // Get the count of edges in the previous timing period.
    //
    ulPrev = g_ulSpeedPrevious;

    //
    // Save the count of edges during this timing period.
    //
    g_ulSpeedPrevious = ulCount;

    //
    // See if this timing period should be skipped.
    //
    if(HWREGBITW(&g_ulSpeedFlags, FLAG_SKIP_BIT))
    {
        //
        // This timing period should be skipped, but an edge count from a
        // previous timing period now exists so the next timing period should
        // not be skipped.
        //
        HWREGBITW(&g_ulSpeedFlags, FLAG_SKIP_BIT) = 0;

        //
        // There is nothing further to be done.
        //
        return;
    }

    //
    // Average the edge count for the previous two timing periods.
    //
    ulCount = (ulPrev + ulCount) / 2;

    //
    // Compute the new speed from the number of edges.  Note that both
    // edges are counted by the QEI block, so the count for a full revolution
    // is double the number of encoder lines.
    //
    SpeedNewValue((ulCount * QEI_INT_RATE * 30) /
                  (g_sParameters.usNumEncoderLines + 1));

    //
    // See if the number of edges has become too small, meaning that the edge
    // time has become large enough.
    //
    if(ulCount < (((MAX_EDGE_COUNT - EDGE_DELTA) * 2) / QEI_INT_RATE))
    {
        //
        // Edge timing mode should be used instead of edge counting mode.
        //
        HWREGBITW(&g_ulSpeedFlags, FLAG_COUNT_BIT) = 0;

        //
        // Indicate that the first edge should be skipped in edge timing mode.
        //
        HWREGBITW(&g_ulSpeedFlags, FLAG_SKIP_BIT) = 1;

        //
        // Enable the GPIO interrupt to enable edge timing mode.
        //
        IntEnable(INT_GPIOC);
    }
}

//*****************************************************************************
//
//! Initializes the speed sensing routines.
//!
//! This function will initialize the peripherals used determine the speed of
//! the motor's rotor.
//!
//! \return None.
//
//*****************************************************************************
void
SpeedSenseInit(void)
{
    //
    // Configure the encoder A pin for use by the QEI block.  Even though this
    // pin is now used to drive the QEI block, its state is still visible to
    // the GPIO block.
    // Encoder B and Index pins are not used, but should be configured here,
    // for test support.
    //
    GPIOPinTypeQEI(PIN_ENCA_PORT, PIN_ENCA_PIN);
    GPIOPinTypeQEI(PIN_ENCB_PORT, PIN_ENCB_PIN);
    GPIOPinTypeQEI(PIN_INDEX_PORT, PIN_INDEX_PIN);
    
    //
    // A GPIO interrupt should be generated on rising edges of the encoder A
    // pin.
    //
    GPIOIntTypeSet(PIN_ENCA_PORT, PIN_ENCA_PIN, GPIO_RISING_EDGE);

    //
    // Enable the encoder A pin GPIO interrupt.
    //
    GPIOPinIntEnable(PIN_ENCA_PORT, PIN_ENCA_PIN);
    IntEnable(INT_GPIOC);

    //
    // Configure the QEI block for capturing the velocity of the encoder A pin
    // (which it does by counting the number of edges during a fixed time
    // period).
    //
    QEIConfigure(QEI0_BASE, (QEI_CONFIG_CAPTURE_A | QEI_CONFIG_NO_RESET |
                             QEI_CONFIG_CLOCK_DIR | QEI_CONFIG_NO_SWAP), 0);
    QEIVelocityConfigure(QEI0_BASE, QEI_VELDIV_1, SYSTEM_CLOCK / QEI_INT_RATE);

    //
    // Enable the QEI block and the velocity capture.
    //
    QEIEnable(QEI0_BASE);
    QEIVelocityEnable(QEI0_BASE);

    //
    // Enable the QEI velocity interrupt.
    //
    QEIIntEnable(QEI0_BASE, QEI_INTTIMER);
    IntEnable(INT_QEI0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
