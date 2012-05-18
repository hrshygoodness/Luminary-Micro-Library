//*****************************************************************************
//
// pwm_ctrl.c - PWM control routines.
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
// This is part of revision 8555 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_pwm.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "faults.h"
#include "main.h"
#include "pins.h"
#include "pwm_ctrl.h"
#include "ui.h"

//*****************************************************************************
//
//! \page pwm_ctrl_intro Introduction
//!
//! The generated motor drive waveforms are driven to the inverter bridge with
//! the PWM module.  The PWM generators are run in a fully synchronous manner;
//! the counters are synchronized (that is, the values of the three counters
//! are always the same) and updates to the duty cycle registers are
//! synchronized to the zero value of the PWM counters.
//!
//! The dead-band unit in each PWM generator is used to prevent shoot-through
//! current in the inverter bridge when switching between the high side to the
//! low of a phase.  Shoot-through occurs because the turn-on time of one gate
//! doesn't always match the turn-off time of the other, so both may be on for
//! a short period despite the fact that only one of their inputs is on.  By
//! providing a period of time where both inputs are off when making the
//! transition, shoot-through is not possible.
//!
//! The PWM outputs can be in one of four modes during the operation of the
//! motor drive.  The first is off, where all six outputs are in the inactive
//! state.  This is the state used when the motor drive is stopped; the motor
//! is electrically disconnected during this time (effectively the same as
//! disconnecting the cable) and is free to spin as if it were unplugged.
//!
//! The next mode is precharge, where the three outputs to the high
//! side switches are inactive and the three outputs to the low side switches
//! are switches at a 50% duty cycle.  The high side gate drivers have a
//! bootstrap circuit for generating the voltage to drive the gates that only
//! charges when the low side is switching; this precharge mode allows the
//! bootstrap circuit to generate the required gate drive voltage before real
//! waveforms are driven.  Failure to precharge the high side gate drivers
//! would simply result in distortion of the first part of the output waveform
//! (until the bootstrap circuit generates a voltage high enough to turn on the
//! high side gate).  This mode is used briefly when going from a non-driving
//! state to a driving state.
//!
//! The next mode is running, where all six outputs are actively toggling.
//! This will create a magnetic field in the stator of the motor, inducing a
//! magnetic field in the rotor and causing it to spin.  This mode is used to
//! drive the motor.
//!
//! The final mode is DC injection braking, where the first PWM pair are
//! actively toggling, the low side of the second PWM pair is always on, and
//! the third PWM pair is inactive.  This results in a fixed DC voltage being
//! applied across the motor, resulting in braking.  This mode is optionally
//! used briefly when going from a driving state to a non-driving state in
//! order to completely stop the rotation of the rotor.  For loads with high
//! inertia, or low friction rotors, this can reduce the rotor stop time from
//! minutes to seconds.  Applying a DC voltage to an AC induction motor does
//! generate a lot of heat in the windings, so it should only be used for as
//! long as required to stop the rotor and no longer.
//!
//! The PWM outputs are configured to immediately switch to the inactive state
//! when the processor is stopped by a debugger.  This prevents the current
//! PWM state from becoming a DC voltage (since the processor is no longer
//! running to change the duty cycles) and damaging the motor.  In general,
//! though, it is not a good idea to stop the processor when the motor is
//! running.  When no longer driven, the motor will start to slow down due to
//! friction; when the processor is restarted, it will continue driving at the
//! previous drive frequency.  The difference between rotor and stator
//! frequency (that is, the slip) has become much greater due to the time that
//! the motor was not being driven.  This will likely result in an immediate
//! motor over-current fault since the increased slip will result in a rise
//! motor current.  While not harmful, it does not allow the typically desired
//! behavior of being able to stop the application, look at the internal state,
//! and then restart the application as if nothing had happened.
//!
//! An interrupt is generated at each zero value of the counter in PWM
//! generator zero; this is used as a time base for the generation of waveforms
//! as well as a time to queue the next duty cycle update into the hardware.
//! At any given time, the PWM module is outputting the duty cycle for period
//! N, has the duty cycle for period N+1 queued in its holding registers
//! waiting for the next zero value of the counter, and the microcontroller is
//! computing the duty cycle for period N+2.
//!
//! Two ``software'' interrupts are generated by the PWM interrupt handler.
//! One is used to update the waveform; this occurs at a configurable rate of
//! every X PWM period.  The other is used to update the drive frequency and
//! perform other periodic system tasks such as fault monitoring; this occurs
//! every millisecond.  The unused interrupts from the second and third PWM
//! generator are used for these ``software'' interrupts; the ability to fake
//! the assertion of an interrupt through the NVIC software interrupt trigger
//! register is used to generate these ``software'' interrupts.
//!
//! The code for handling the PWM module is contained in <tt>pwm_ctrl.c</tt>,
//! with <tt>pwm_ctrl.h</tt> containing the definitions for the variables and
//! functions exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup pwm_ctrl_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The frequency of the clock that drives the PWM generators.
//
//*****************************************************************************
#define PWM_CLOCK               50000000

//*****************************************************************************
//
//! The width of a single PWM clock, in nanoseconds.
//
//*****************************************************************************
#define PWM_CLOCK_WIDTH         20

//*****************************************************************************
//
//! The number of PWM clocks in a single PWM period.
//
//*****************************************************************************
static unsigned long g_ulPWMClock;

//*****************************************************************************
//
//! The frequency of the output PWM waveforms.
//
//*****************************************************************************
unsigned long g_ulPWMFrequency;

//*****************************************************************************
//
//! The duty cycle of the waveform output to the U phase of the bridge.
//
//*****************************************************************************
static unsigned long g_ulPWMDutyCycleU;

//*****************************************************************************
//
//! The duty cycle of the waveform output to the V phase of the bridge.
//
//*****************************************************************************
static unsigned long g_ulPWMDutyCycleV;

//*****************************************************************************
//
//! The duty cycle of the waveform output to the W phase of the bridge.
//
//*****************************************************************************
static unsigned long g_ulPWMDutyCycleW;

//*****************************************************************************
//
//! The minimum width of an output PWM pulse, in PWM clocks.
//
//*****************************************************************************
static unsigned long g_ulMinPulseWidth;

//*****************************************************************************
//
//! A set of flags that control the operation of the PWM control routines.  The
//! flags are #PWM_FLAG_NEW_FREQUENCY, and #PWM_FLAG_NEW_DUTY_CYCLE.
//
//*****************************************************************************
static unsigned long g_ulPWMFlags = 0;

//*****************************************************************************
//
//! The bit number of the flag in #g_ulPWMFlags that indicates that a new
//! PWM frequency (that is, period) is ready to be supplied to the PWM module.
//
//*****************************************************************************
#define PWM_FLAG_NEW_FREQUENCY  0

//*****************************************************************************
//
//! The bit number of the flag in #g_ulPWMFlags that indicates that a new duty
//! cycle (that is, compare) is ready to be supplied to the PWM module.
//
//*****************************************************************************
#define PWM_FLAG_NEW_DUTY_CYCLE 1

//*****************************************************************************
//
//! A count of the number of PWM periods have occurred, based on the number of
//! PWM module interrupts.  This is incremented when a PWM interrupt is handled
//! and decremented by the waveform generation handler.
//
//*****************************************************************************
static unsigned long g_ulPWMPeriodCount;

//*****************************************************************************
//
//! A counter that is used to determine when a millisecond has passed.  The
//! millisecond software interrupt is triggered based on this count.
//
//*****************************************************************************
static unsigned long g_ulPWMMillisecondCount;

//*****************************************************************************
//
//! Computes the minimum PWM pulse width.
//!
//! This function computes the minimum PWM pulse width based on the minimum
//! pulse width parameter and the dead time parameter.  The dead timers will
//! reduce the width of a PWM pulse, so their value must be considered to avoid
//! pulses shorter than the parameter value being produced.
//!
//! \return None.
//
//*****************************************************************************
void
PWMSetMinPulseWidth(void)
{
    //
    // Compute the minimum pulse width in PWM clocks.
    //
    g_ulMinPulseWidth = ((((g_sParameters.ucDeadTime + 1) * 20) +
                          (g_sParameters.ucMinPulseWidth * 100) +
                          (PWM_CLOCK_WIDTH - 1)) / PWM_CLOCK_WIDTH);

    //
    // If the minimum pulse width parameter is zero, then increment the minimum
    // pulse width (that is, the dead time) by one to avoid sending pulses into
    // the dead band unit that are too short.
    //
    if(g_sParameters.ucMinPulseWidth == 0)
    {
        g_ulMinPulseWidth++;
    }
}

//*****************************************************************************
//
//! Configures the dead timers for the PWM generators.
//!
//! This function configures the dead timers for all three PWM generators based
//! on the dead time parameter.
//!
//! \return None.
//
//*****************************************************************************
void
PWMSetDeadBand(void)
{
    //
    // Set the dead band times for all three PWM generators.
    //
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_0, g_sParameters.ucDeadTime,
                      g_sParameters.ucDeadTime);
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_1, g_sParameters.ucDeadTime,
                      g_sParameters.ucDeadTime);
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_2, g_sParameters.ucDeadTime,
                      g_sParameters.ucDeadTime);

    //
    // Update the minimum PWM pulse width.
    //
    PWMSetMinPulseWidth();
}

//*****************************************************************************
//
//! Sets the frequency of the generated PWM waveforms.
//!
//! This function configures the frequency of the generated PWM waveforms.  The
//! frequency update will not occur immediately; the change will be registered
//! for synchronous application to the output waveforms to avoid
//! discontinuities.
//!
//! \return None.
//
//*****************************************************************************
void
PWMSetFrequency(void)
{
    //
    // Disable the PWM interrupt temporarily.
    //
    IntDisable(INT_PWM0_0);

    //
    // Determine the configured PWM frequency.
    //
    switch(g_sParameters.usFlags & FLAG_PWM_FREQUENCY_MASK)
    {
        //
        // The PWM frequency is 8 KHz.
        //
        case FLAG_PWM_FREQUENCY_8K:
        {
            //
            // Set the PWM frequency variable.
            //
            g_ulPWMFrequency = 8000;

            //
            // Get the number of PWM clocks in a 8 KHz period.
            //
            g_ulPWMClock = PWM_CLOCK / 8000;

            //
            // Done with this PWM frequency.
            //
            break;
        }

        //
        // The PWM frequency is 12.5 KHz.
        //
        case FLAG_PWM_FREQUENCY_12K:
        {
            //
            // Set the PWM frequency variable.
            //
            g_ulPWMFrequency = 12500;

            //
            // Get the number of PWM clocks in a 12.5 KHz period.
            //
            g_ulPWMClock = PWM_CLOCK / 12500;

            //
            // Done with this PWM frequency.
            //
            break;
        }

        //
        // The PWM frequency is 16 KHz.
        //
        case FLAG_PWM_FREQUENCY_16K:
        {
            //
            // Set the PWM frequency variable.
            //
            g_ulPWMFrequency = 16000;

            //
            // Get the number of PWM clocks in a 16 KHz period.
            //
            g_ulPWMClock = PWM_CLOCK / 16000;

            //
            // Done with this PWM frequency.
            //
            break;
        }

        //
        // The PWM frequency is 20 KHz.
        //
        case FLAG_PWM_FREQUENCY_20K:
        default:
        {
            //
            // Set the PWM frequency variable.
            //
            g_ulPWMFrequency = 20000;

            //
            // Get the number of PWM clocks in a 20 KHz period.
            //
            g_ulPWMClock = PWM_CLOCK / 20000;

            //
            // Done with this PWM frequency.
            //
            break;
        }
    }

    //
    // Indicate that the PWM frequency needs to be updated.
    //
    HWREGBITW(&g_ulPWMFlags, PWM_FLAG_NEW_FREQUENCY) = 1;

    //
    // Re-enable the PWM interrupt.
    //
    IntEnable(INT_PWM0_0);
}

//*****************************************************************************
//
//! Updates the duty cycle in the PWM module.
//!
//! This function programs the duty cycle of the PWM waveforms into the PWM
//! module.  The changes will be written to the hardware and the hardware
//! instructed to start using the new values the next time its counters reach
//! zero.
//!
//! \return None.
//
//*****************************************************************************
static void
PWMUpdateDutyCycle(void)
{
    unsigned long ulWidth;

    //
    // Get the pulse width of the U phase of the motor.  If the width of the
    // positive portion of the pulse is less than the minimum pulse width, then
    // force the signal low continuously.  If the width of the negative portion
    // of the pulse is less than the minimum pulse width, then force the signal
    // high continuously.
    //
    ulWidth = (g_ulPWMDutyCycleU * g_ulPWMClock) / 65536;
    if(ulWidth < g_ulMinPulseWidth)
    {
        ulWidth = g_ulMinPulseWidth;
    }
    if((g_ulPWMClock - ulWidth) < g_ulMinPulseWidth)
    {
        ulWidth = g_ulPWMClock - g_ulMinPulseWidth;
    }

    //
    // Set the pulse width of the U phase of the motor.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, ulWidth);

    //
    // Get the pulse width of the V phase of the motor.
    //
    ulWidth = (g_ulPWMDutyCycleV * g_ulPWMClock) / 65536;
    if(ulWidth < g_ulMinPulseWidth)
    {
        ulWidth = g_ulMinPulseWidth;
    }
    if((g_ulPWMClock - ulWidth) < g_ulMinPulseWidth)
    {
        ulWidth = g_ulPWMClock - g_ulMinPulseWidth;
    }

    //
    // Set the pulse width of the V phase of the motor.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, ulWidth);

    //
    // Get the pulse width of the W phase of the motor.
    //
    ulWidth = (g_ulPWMDutyCycleW * g_ulPWMClock) / 65536;
    if(ulWidth < g_ulMinPulseWidth)
    {
        ulWidth = g_ulMinPulseWidth;
    }
    if((g_ulPWMClock - ulWidth) < g_ulMinPulseWidth)
    {
        ulWidth = g_ulPWMClock - g_ulMinPulseWidth;
    }

    //
    // Set the pulse width of the W phase of the motor.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, ulWidth);

    //
    // Perform a synchronous update of all three PWM generators.
    //
    PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT | PWM_GEN_2_BIT);
}

//*****************************************************************************
//
//! Handles the PWM interrupt.
//!
//! This function is called as a result of the interrupt generated by the PWM
//! module when the counter reaches zero.  If an updated PWM frequency or duty
//! cycle is available, they will be updated in the hardware by this function.
//!
//! \return None.
//
//*****************************************************************************
void
PWM0IntHandler(void)
{
    //
    // Clear the PWM interrupt.  This is done twice since the clear will be
    // ignored by hardware if it occurs on the same cycle as another interrupt
    // event; the second clear takes care of the case wehre the first gets
    // ignored.
    //
    PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_ZERO);
    PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_ZERO);

    //
    // Increment the count of PWM periods.
    //
    g_ulPWMPeriodCount++;

    //
    // See if it is time for a new PWM duty cycle, based on the correct number
    // of PWM periods passing and the availability of new duty cycle values.
    //
    if((g_ulPWMPeriodCount > g_sParameters.ucUpdateRate) &&
       (HWREGBITW(&g_ulPWMFlags, PWM_FLAG_NEW_DUTY_CYCLE) == 1))
    {
        //
        // See if the PWM frequency needs to be updated.
        //
        if(HWREGBITW(&g_ulPWMFlags, PWM_FLAG_NEW_FREQUENCY) == 1)
        {
            //
            // Set the new PWM period in each of the PWM generators.
            //
            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, g_ulPWMClock);
            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, g_ulPWMClock);
            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, g_ulPWMClock);

            //
            // Indicate that the PWM frequency has been updated.
            //
            HWREGBITW(&g_ulPWMFlags, PWM_FLAG_NEW_FREQUENCY) = 0;
        }

        //
        // Update the duty cycle.
        //
        PWMUpdateDutyCycle();

        //
        // Clear the duty cycle update flag.
        //
        HWREGBITW(&g_ulPWMFlags, PWM_FLAG_NEW_DUTY_CYCLE) = 0;
    }

    //
    // If the required number of PWM periods have expired, request an update of
    // the duty cycle computations.
    //
    if(g_ulPWMPeriodCount >= (g_sParameters.ucUpdateRate + 1))
    {
        //
        // Trigger the waveform update software interrupt.
        //
        HWREG(NVIC_SW_TRIG) = INT_PWM0_1 - 16;
    }

    //
    // Increment the millisecond counter.  By adding 1000 for each PWM
    // interrupt, it will take one millisecond for the counter to reach the PWM
    // frequency.
    //
    g_ulPWMMillisecondCount += 1000;

    //
    // See if a millisecond has expired.
    //
    if(g_ulPWMMillisecondCount >= g_ulPWMFrequency)
    {
        //
        // Trigger the millisecond software interrupt.
        //
        HWREG(NVIC_SW_TRIG) = INT_PWM0_2 - 16;

        //
        // Decrement the millisecond counter by the PWM frequency, which
        // corresponds to one millisecond.
        //
        g_ulPWMMillisecondCount -= g_ulPWMFrequency;
    }
}

//*****************************************************************************
//
//! Handles the PWM fault interrupt.
//!
//! This function is called as a result of the interrupt generated by the
//! assertion of the PWM fault input.  It is treated as a sticky fault
//! condition and will emergency stop the motor drive.
//!
//! \return None.
//
//*****************************************************************************
void
PWMFaultHandler(void)
{
    //
    // Clear the fault interrupt.
    //
    PWMFaultIntClear(PWM0_BASE);

    //
    // Emergency stop the motor drive.
    //
    MainEmergencyStop();

    //
    // Indicate that the power module fault has occurred.
    //
    MainSetFault(FAULT_POWER_MODULE);
}

//*****************************************************************************
//
//! Gets the number of PWM interrupts that have occurred.
//!
//! This function returns the number of PWM interrupts that have been counted.
//! Used in conjunction with the desired update rate, missed waveform updates
//! can be detected and compensated for.
//!
//! \return The number of PWM interrupts that have been counted.
//
//*****************************************************************************
unsigned long
PWMGetPeriodCount(void)
{
    //
    // Return the count of PWM periods.
    //
    return(g_ulPWMPeriodCount);
}

//*****************************************************************************
//
//! Reduces the count of PWM interrupts.
//!
//! \param ulCount is the number by which to reduce the PWM interrupt count.
//!
//! This function reduces the PWM interrupt count by a given number.  When the
//! waveform values are updated, the interrupt count can be reduced by the
//! appropriate amount to maintain a proper indication of when the next
//! waveform update should occur.
//!
//! If the PWM interrupt count is not reduced when the waveforms are
//! recomputed, the waveform update software interrupt will not be triggered as
//! desired.
//!
//! \return None.
//
//*****************************************************************************
void
PWMReducePeriodCount(unsigned long ulCount)
{
    //
    // Disable the PWM interrupt temporarily.
    //
    IntDisable(INT_PWM0_0);

    //
    // Decrement the PWM period count by the given number.
    //
    g_ulPWMPeriodCount -= ulCount;

    //
    // Re-enable the PWM interrupt.
    //
    IntEnable(INT_PWM0_0);
}

//*****************************************************************************
//
//! Sets the duty cycle of the generated PWM waveforms.
//!
//! \param ulDutyCycleU is the duty cycle of the waveform for the U phase of
//! the motor, specified as a 16.16 fixed point value between 0.0 and 1.0.
//! \param ulDutyCycleV is the duty cycle of the waveform for the V phase of
//! the motor, specified as a 16.16 fixed point value between 0.0 and 1.0.
//! \param ulDutyCycleW is the duty cycle of the waveform for the W phase of
//! the motor, specified as a 16.16 fixed point value between 0.0 and 1.0.
//!
//! This function configures the duty cycle of the generated PWM waveforms.
//! The duty cycle update will not occur immediately; the change will be
//! registered for synchronous application to the output waveforms to avoid
//! discontinuities.
//!
//! \return None.
//
//*****************************************************************************
void
PWMSetDutyCycle(unsigned long ulDutyCycleU, unsigned long ulDutyCycleV,
                unsigned long ulDutyCycleW)
{
    //
    // Disable the PWM interrupt temporarily.
    //
    IntDisable(INT_PWM0_0);

    //
    // Save the duty cycles for the three phases.
    //
    g_ulPWMDutyCycleU = ulDutyCycleU;
    g_ulPWMDutyCycleV = ulDutyCycleV;
    g_ulPWMDutyCycleW = ulDutyCycleW;

    //
    // Set the flag indicating that the duty cycles need to be updated.
    //
    HWREGBITW(&g_ulPWMFlags, PWM_FLAG_NEW_DUTY_CYCLE) = 1;

    //
    // Re-enable the PWM interrupt.
    //
    IntEnable(INT_PWM0_0);
}

//*****************************************************************************
//
//! Sets the PWM outputs to precharge the high side gate drives.
//!
//! This function configures the PWM outputs such that they will start charging
//! the bootstrap capacitor on the high side gate drives.  Without this step,
//! the high side gates will not turn on properly for the first several PWM
//! cycles when starting the motor drive.
//!
//! \return None.
//
//*****************************************************************************
void
PWMOutputPrecharge(void)
{
    //
    // Disable the high side switches.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_3_BIT | PWM_OUT_5_BIT,
                   false);

    //
    // Enable the low side switches.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT | PWM_OUT_2_BIT | PWM_OUT_4_BIT,
                   true);

    //
    // Set the PWM duty cycles to 50%.
    //
    g_ulPWMDutyCycleU = 32768;
    g_ulPWMDutyCycleV = 32768;
    g_ulPWMDutyCycleW = 32768;

    //
    // Set the PWM period based on the configured PWM frequency.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, g_ulPWMClock);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, g_ulPWMClock);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, g_ulPWMClock);

    //
    // Update the PWM duty cycles.
    //
    PWMUpdateDutyCycle();
}

//*****************************************************************************
//
//! Turns on all the PWM outputs.
//!
//! This function turns on all of the PWM outputs, allowing them to be
//! propagated to the gate drivers.
//!
//! \return None.
//
//*****************************************************************************
void
PWMOutputOn(void)
{
    //
    // Enable all six PWM outputs.
    //
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                               PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT),
                   true);
}

//*****************************************************************************
//
//! Turns off all the PWM outputs.
//!
//! This function turns off all of the PWM outputs, preventing them from being
//! propagates to the gate drivers.
//!
//! \return None.
//
//*****************************************************************************
void
PWMOutputOff(void)
{
    //
    // Disable all six PWM outputs.
    //
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                               PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT),
                   false);

    //
    // Set the PWM duty cycles to 50%.
    //
    g_ulPWMDutyCycleU = 32768;
    g_ulPWMDutyCycleV = 32768;
    g_ulPWMDutyCycleW = 32768;

    //
    // Set the PWM period so that the ADC runs at 1 KHz.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_CLOCK / 1000);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWM_CLOCK / 1000);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_CLOCK / 1000);

    //
    // Update the PWM duty cycles.
    //
    PWMUpdateDutyCycle();
}

//*****************************************************************************
//
//! Sets the PWM outputs to DC injection brake the motor.
//!
//! \param ulVoltage is the DC voltage to be applied to the motor.  This value
//! must be less than 160V, half the nominal DC bus voltage (and likely much
//! less than that).
//!
//! This function configures the PWM outputs such that they will provide DC
//! injection braking of the motor.
//!
//! \note Once the motor comes to a complete stop, DC injection braking will
//! simply generate heat within the motor, likely causing damage.  It is
//! important that the DC injection braking be disabled to avoid this
//! situation.
//!
//! \return None.
//
//*****************************************************************************
void
PWMOutputDCBrake(unsigned long ulVoltage)
{
    //
    // Enable the U and V phases and disable the W phase.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT | PWM_OUT_5_BIT, false);
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                               PWM_OUT_3_BIT), true);

    //
    // Set the PWM duty cycles of the U phase to the required voltage level and
    // the V and W phase to 50%.
    //
    g_ulPWMDutyCycleU = ((g_ulPWMClock * ulVoltage) / 160) + 32768;
    g_ulPWMDutyCycleV = 32768;
    g_ulPWMDutyCycleW = 32768;

    //
    // Update the PWM duty cycles.
    //
    PWMUpdateDutyCycle();
}

//*****************************************************************************
//
//! Changes the update rate of the motor drive.
//!
//! \param ucUpdateRate is the number of PWM periods between updates.
//!
//! This function changes the rate at which the motor drive waveforms are
//! recomputed.  Lower update values recompute the waveforms more frequently,
//! providing more accurate waveforms at the cost of increased processor usage.
//!
//! \return None.
//
//*****************************************************************************
void
PWMSetUpdateRate(unsigned char ucUpdateRate)
{
    //
    // Temporarily disable the PWM period interrupt.  Once disabled, it is no
    // longer possible for the waveform update software interrupt to be
    // triggered.
    //
    IntDisable(INT_PWM0_0);

    //
    // Change the update rate parameter.
    //
    g_sParameters.ucUpdateRate = ucUpdateRate;

    //
    // Re-enable the PWM period interrupt.
    //
    IntEnable(INT_PWM0_0);
}

//*****************************************************************************
//
//! Handles the PWM fault interrupt.
//!
//! This function is called as a result of the interrupt generated by the
//! assertion of the PWM fault input.  It is treated as a sticky fault
//! condition and will emergency stop the motor drive.
//!
//! \return None.
//
//*****************************************************************************
void
GPIOBIntHandler(void)
{
    //
    // Clear the GPIO interrupt.
    //
    GPIOPinIntClear(PIN_FAULT_PORT, PIN_FAULT_PIN);

    //
    // Emergency stop the motor drive.
    //
    MainEmergencyStop();

    //
    // Indicate that the power module fault has occurred.
    //
    MainSetFault(FAULT_POWER_MODULE);
}

//*****************************************************************************
//
//! Initializes the PWM control routines.
//!
//! This function initializes the PWM module and the control routines,
//! preparing them to produce PWM waveforms to drive the power module.
//!
//! \return None.
//
//*****************************************************************************
void
PWMInit(void)
{
    //
    // Make the PWM pins be peripheral function.
    //
    GPIOPinTypePWM(PIN_PHASEU_LOW_PORT,
                   PIN_PHASEU_LOW_PIN | PIN_PHASEU_HIGH_PIN);
    GPIOPinTypePWM(PIN_PHASEV_LOW_PORT,
                   PIN_PHASEV_LOW_PIN | PIN_PHASEV_HIGH_PIN);
    GPIOPinTypePWM(PIN_PHASEW_LOW_PORT,
                   PIN_PHASEW_LOW_PIN | PIN_PHASEV_HIGH_PIN);

    //
    // Configure the three PWM generators for up/down counting mode,
    // synchronous updates, and to stop at zero on debug events.
    //
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, (PWM_GEN_MODE_UP_DOWN |
                                           PWM_GEN_MODE_SYNC |
                                           PWM_GEN_MODE_DBG_STOP));
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, (PWM_GEN_MODE_UP_DOWN |
                                           PWM_GEN_MODE_SYNC |
                                           PWM_GEN_MODE_DBG_STOP));
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, (PWM_GEN_MODE_UP_DOWN |
                                           PWM_GEN_MODE_SYNC |
                                           PWM_GEN_MODE_DBG_STOP));

    //
    // Set the initial duty cycles to 50%.
    //
    g_ulPWMDutyCycleU = 32768;
    g_ulPWMDutyCycleV = 32768;
    g_ulPWMDutyCycleW = 32768;

    //
    // Configure the PWM period, duty cycle, and dead band.  The initial period
    // is 1 KHz (for triggering the ADC), which will be increased when the
    // motor starts running.
    //
    PWMSetDeadBand();
    PWMSetFrequency();
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_CLOCK / 1000);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWM_CLOCK / 1000);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_CLOCK / 1000);
    PWMUpdateDutyCycle();

    //
    // Enable the PWM generators.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

    //
    // Synchronize the time base of the generators.
    //
    PWMSyncTimeBase(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT | PWM_GEN_2_BIT);

    //
    // Configure an interrupt on the zero event of the first generator, and an
    // ADC trigger on the load event of the first generator.
    //
    PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_ZERO);
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0,
                        PWM_INT_CNT_ZERO | PWM_TR_CNT_LOAD);
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_1, 0);
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, 0);
    IntEnable(INT_PWM0_0);
    IntEnable(INT_PWM0_1);
    IntEnable(INT_PWM0_2);

    //
    // Set all six PWM outputs to go to the inactive state when a fault event
    // occurs (which includes debug events).
    //
    PWMOutputFault(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                               PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT),
                   true);

    //
    // Disable all six PWM outputs.
    //
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                               PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT),
                   false);

    //
    // Configure the fault signal from the power module as a falling edge
    // interrupt.
    //
    GPIODirModeSet(PIN_FAULT_PORT, PIN_FAULT_PIN, GPIO_DIR_MODE_IN);
    GPIOIntTypeSet(PIN_FAULT_PORT, PIN_FAULT_PIN, GPIO_FALLING_EDGE);
    GPIOPinIntClear(PIN_FAULT_PORT, PIN_FAULT_PIN);
    GPIOPinIntEnable(PIN_FAULT_PORT, PIN_FAULT_PIN);
    IntEnable(INT_GPIOB);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
