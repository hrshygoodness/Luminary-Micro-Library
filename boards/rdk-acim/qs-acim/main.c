//*****************************************************************************
//
// main.c - AC induction motor drive main application.
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
// This is part of revision 10636 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "utils/flash_pb.h"
#include "adc_ctrl.h"
#include "brake.h"
#include "commands.h"
#include "faults.h"
#include "inrush.h"
#include "main.h"
#include "pwm_ctrl.h"
#include "sinemod.h"
#include "speed_sense.h"
#include "svm.h"
#include "ui.h"
#include "vf.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>A/C Induction Motor Controller (qs-acim)</h1>
//!
//! This application is a motor drive for single and three phase A/C induction
//! motors.  The following features are supported:
//!
//! - V/f control
//! - Sine wave modulation
//! - Space vector modulation
//! - Closed loop speed control
//! - DC bus voltage monitoring and control
//! - AC in-rush current control
//! - Regenerative braking control
//! - DC braking control
//! - Simple on-board user interface (via a potentiometer and push button)
//! - Comprehensive serial user interface
//! - Over 30 configurable drive parameters
//! - Persistent storage of drive parameters in flash
//
//*****************************************************************************

//*****************************************************************************
//
//! \page main_intro Introduction
//!
//! This is the main AC induction motor application code.  It contains a state
//! machine that controls the operation of the drive, an interrupt handler for
//! the waveform update software interrupt, an interrupt handler for the
//! millisecond frequency update software interrupt, and the main application
//! startup code.
//!
//! The waveform update interrupt handler is responsible for computing new
//! values for the waveforms being driven to the inverter bridge.  Based on the
//! update rate, it will advance the drive angle and recompute new waveforms.
//! The V/f tables is used to determine the amplitude and the appropriate
//! modulation is performed.  The new waveform values are passed to the PWM
//! module to be supplied to the PWM hardware at the correct time.
//!
//! The millisecond frequency update interrupt handler is responsible for
//! handling the dynamic brake, computing the new drive frequency, and checking
//! for fault conditions.  If the drive is just starting, this is where the
//! precharging of the high-side gate drivers is handled.  If the drive has
//! just stopped, this is where the DC injection braking is handled.  Dynamic
//! braking is handled by simply calling the update function for the dynamic
//! braking module.
//!
//! When running, a variety of things are done to adjust the drive frequency.
//! First, the target frequency is adjusted by a PI controller if closed-loop
//! mode is enabled, moving the target frequency such that the rotor will reach
//! the desired frequency.  Then, the acceleration or deceleration rate is
//! applied as appropriate to move the output frequency towards the target
//! frequency.  In the case of deceleration, the deceleration rate may be
//! reduced based on the DC bus voltage.  The result of this frequency
//! adjustment is a new step angle, which is subsequently used by the waveform
//! update interrupt handler to generate the output waveforms.
//!
//! The over-temperature, DC bus under-voltage, DC bus over-voltage, motor
//! under-current, and motor over-current faults are all checked for by
//! examining the readings from the ADC.  Fault conditions are handled by
//! turning off the drive output and indicating the appropriate fault, which
//! must be cleared before the drive will run again.
//!
//! The state machine that controls the operation of the drive is woven
//! throughout the millisecond frequency update interrupt handler and the
//! routines that start, stop, and adjust the parameters of the motor drive.
//! Together, it ensures that the motor drive responds to commands and
//! parameter changes in a logical and predictable manner.
//!
//! The application startup code performs high-level initialization of the
//! microcontroller (such as enabling peripherals) and calls the initialization
//! routines for the various support modules.  Since all the work within the
//! motor drive occurs with interrupt handlers, its final task is to go into
//! an infinite loop that puts the processor into sleep mode.  This serves two
//! purposes; it allows the processor to wait until there is work to be done
//! (for example, an interrupt) before it executes any further code, and it
//! allows the processor usage meter to gather the data it needs to determine
//! processor usage.
//!
//! The main application code is contained in <tt>main.c</tt>, with
//! <tt>main.h</tt> containing the definitions for the defines, variables, and
//! functions exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup main_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! A state flag that indicates that the motor drive is in the forward
//! direction.
//
//*****************************************************************************
#define STATE_FLAG_FORWARD      0x01

//*****************************************************************************
//
//! A state flag that indicates that the motor drive is in the backward
//! direction.
//
//*****************************************************************************
#define STATE_FLAG_BACKWARD     0x00

//*****************************************************************************
//
//! A state flag that indicates that the motor drive is running.
//
//*****************************************************************************
#define STATE_FLAG_RUN          0x02

//*****************************************************************************
//
//! A state flag that indicates that the motor drive is stopping.
//
//*****************************************************************************
#define STATE_FLAG_STOPPING     0x04

//*****************************************************************************
//
//! A state flag that indicates that the motor drive is reversing direction.
//
//*****************************************************************************
#define STATE_FLAG_REV          0x08

//*****************************************************************************
//
//! A state flag that indicates that the motor drive is precharging the
//! bootstrap capacitors on the high side gate drivers.
//
//*****************************************************************************
#define STATE_FLAG_PRECHARGE    0x10

//*****************************************************************************
//
//! A state flag that indicates that the motor drive is performing DC injection
//! braking.
//
//*****************************************************************************
#define STATE_FLAG_BRAKE        0x20

//*****************************************************************************
//
//! The motor drive is stopped.  A run request will cause a transition to the
//! #STATE_PRECHARGE or #STATE_BACK_PRECHARGE states, depending upon the
//! direction flag.
//
//*****************************************************************************
#define STATE_STOPPED           0x00

//*****************************************************************************
//
//! The motor drive is precharging the bootstrap capacitors on the high side
//! gate drivers.  Once the capacitors are charged, the state machine will
//! automatically transition to #STATE_RUN.
//
//*****************************************************************************
#define STATE_PRECHARGE         (STATE_FLAG_PRECHARGE | STATE_FLAG_FORWARD)

//*****************************************************************************
//
//! The motor drive is running, either at the target frequency or slewing to
//! the target frequency.
//
//*****************************************************************************
#define STATE_RUN               (STATE_FLAG_RUN | STATE_FLAG_FORWARD)

//*****************************************************************************
//
//! The motor drive is decelerating down to a stop, at which point the state
//! machine will automatically transition to #STATE_BACK_RUN.  This results in
//! a direction change of the motor drive.
//
//*****************************************************************************
#define STATE_REV               (STATE_FLAG_RUN | STATE_FLAG_REV | \
                                 STATE_FLAG_FORWARD)

//*****************************************************************************
//
//! The motor drive is decelerating down to a stop, at which point the state
//! machine will automatically transition to #STATE_STOPPED.  This results in
//! the motor drive being stopped.
//
//*****************************************************************************
#define STATE_STOPPING          (STATE_FLAG_RUN | STATE_FLAG_STOPPING | \
                                 STATE_FLAG_FORWARD)

//*****************************************************************************
//
//! The motor drive is precharging the bootstrap capacitors on the high side
//! gate drivers while running in the backward direction.  Once the capacitors
//! are charged, the state machine will automatically transition to
//! #STATE_BACK_RUN.
//
//*****************************************************************************
#define STATE_BACK_PRECHARGE    (STATE_FLAG_PRECHARGE | STATE_FLAG_BACKWARD)

//*****************************************************************************
//
//! The motor drive is running in the backward direction, either at the target
//! frequency or slewing to the target frequency.
//
//*****************************************************************************
#define STATE_BACK_RUN          (STATE_FLAG_RUN | STATE_FLAG_BACKWARD)

//*****************************************************************************
//
//! The motor drive is decelerating down to a stop while running in the
//! backward direction, at which point the state machine will automatically
//! transition to #STATE_RUN.  This results in a direction change of the motor
//! drive.
//
//*****************************************************************************
#define STATE_BACK_REV          (STATE_FLAG_RUN | STATE_FLAG_REV | \
                                 STATE_FLAG_BACKWARD)

//*****************************************************************************
//
//! The motor drive is decelerating down to a stop while running in the
//! backward direction, at which point the state machine will automatically
//! transition to #STATE_STOPPED.  This results in the motor drive being
//! stopped.
//
//*****************************************************************************
#define STATE_BACK_STOPPING     (STATE_FLAG_RUN | STATE_FLAG_STOPPING | \
                                 STATE_FLAG_BACKWARD)

//*****************************************************************************
//
//! The motor drive is performing DC injection braking.  Once the braking has
//! completed, the state machine will automatically transition to
//! #STATE_STOPPED.
//
//*****************************************************************************
#define STATE_BRAKE             STATE_FLAG_BRAKE

//*****************************************************************************
//
//! The latched fault status flags for the motor drive, enumerated by
//! #FAULT_EMERGENCY_STOP, #FAULT_VBUS_LOW, #FAULT_VBUS_HIGH,
//! #FAULT_CURRENT_LOW, #FAULT_CURRENT_HIGH, #FAULT_POWER_MODULE, and
//! #FAULT_TEMPERATURE_HIGH.
//
//*****************************************************************************
unsigned long g_ulFaultFlags;

//*****************************************************************************
//
//! The current operation state of the motor drive.
//
//*****************************************************************************
unsigned char g_ucMotorStatus = MOTOR_STATUS_STOP;

//*****************************************************************************
//
//! The current motor drive frequency, expressed as a 16.16 fixed-point value.
//
//*****************************************************************************
static unsigned long g_ulFrequency = 0;

//*****************************************************************************
//
//! The whole part of the current motor drive frequency.  This is used in
//! conjunction with #g_ulFrequencyFract to compute the value for
//! #g_ulFrequency.
//
//*****************************************************************************
static unsigned long g_ulFrequencyWhole = 0;

//*****************************************************************************
//
//! The fractional part of the current motor drive frequency.  This value is
//! expressed as the numerator of a fraction whose denominator is the PWM
//! frequency.  This is used in conjunction with #g_ulFrequencyWhole to compute
//! the value for #g_ulFrequency.
//
//*****************************************************************************
static unsigned long g_ulFrequencyFract = 0;

//*****************************************************************************
//
//! The target frequency for the motor drive, expressed as a 16.16 fixed-point
//! value.
//
//*****************************************************************************
static unsigned long g_ulTargetFrequency = 0;

//*****************************************************************************
//
//! The current angle of the motor drive output, expressed as a 0.32
//! fixed-point value that is the percentage of the way around a circle.
//
//*****************************************************************************
unsigned long g_ulAngle = 0;

//*****************************************************************************
//
//! The amount by which the motor drive angle is updated for a single PWM
//! period, expressed as a 0.32 fixed-point value.  For example, if the motor
//! drive is being updated every fifth PWM period, this value should be
//! multiplied by five to determine the amount to adjust the angle.
//
//*****************************************************************************
static unsigned long g_ulAngleDelta = 0;

//*****************************************************************************
//
//! A count of the number of milliseconds to remain in a particular state.
//
//*****************************************************************************
static unsigned long g_ulStateCount = 0;

//*****************************************************************************
//
//! The current rate of acceleration.  This will start as the parameter value,
//! but may be reduced in order to manage increases in the motor current.
//
//*****************************************************************************
static unsigned long g_ulAccelRate;

//*****************************************************************************
//
//! The current rate of deceleration.  This will start as the parameter value,
//! but may be reduced in order to manage increases in the DC bus voltage.
//
//*****************************************************************************
static unsigned long g_ulDecelRate;

//*****************************************************************************
//
//! The accumulator for the integral term of the PI controller for the motor
//! drive frequency.
//
//*****************************************************************************
static long g_lFrequencyIntegrator;

//*****************************************************************************
//
//! The maximum value that of the PI controller accumulator
//! (#g_lFrequencyIntegrator).  This limit is based on the I coefficient and
//! the maximum frequency of the motor drive, and is used to avoid "integrator
//! windup", a potential pitfall of PI controllers.
//
//*****************************************************************************
static long g_lFrequencyIntegratorMax;

//*****************************************************************************
//
//! The current state of the motor drive state machine.  This state machine
//! controls acceleration, deceleration, starting, stopping, braking, and
//! reversing direction of the motor drive.
//
//*****************************************************************************
static unsigned long g_ulState = STATE_STOPPED;

//*****************************************************************************
//
//! Handles errors from the driver library.
//!
//! This function is called when an error is encountered by the driver library.
//! Typically, the errors will be invalid parameters passed to the library's
//! APIs.
//!
//! In this application, nothing is done in this function.  It does provide a
//! convenient location for a breakpoint that will catch all driver library
//! errors.
//!
//! \return None.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
//! Multiplies two 16.16 fixed-point numbers.
//!
//! \param lX is the first multiplicand.
//! \param lY is the second multiplicand.
//!
//! This function takes two fixed-point numbers, in 16.16 format, and
//! multiplies them together, returning the 16.16 fixed-point result.  It is
//! the responsibility of the caller to ensure that the dynamic range of the
//! integer portion of the value is not exceeded; if it is exceeded the result
//! will not be correct.
//!
//! \return Returns the result of the multiplication, in 16.16 fixed-point
//! format.
//
//*****************************************************************************
#if defined(ewarm) || defined(DOXYGEN)
static long
MainLongMul(long lX, long lY)
{
    //
    // The assembly code to efficiently perform the multiply (using the
    // instruction to multiply two 32-bit values and return the full 64-bit
    // result).
    //
    __asm("    smull   r0, r1, r0, r1\n"
          "    lsrs    r0, r0, #16\n"
          "    orr     r0, r0, r1, lsl #16\n"
          "    bx      lr");

    //
    // This return is never reached but is required to avoid a compiler
    // warning.
    //
    return(0);
}
#endif
#if defined(gcc) || defined(sourcerygxx) || defined(codered)
static long __attribute__((naked))
MainLongMul(long lX, long lY)
{
    //
    // The assembly code to efficiently perform the multiply (using the
    // instruction to multiply two 32-bit values and return the full 64-bit
    // result).
    //
    __asm("    smull   r0, r1, r0, r1\n"
          "    lsrs    r0, r0, #16\n"
          "    orr     r0, r0, r1, lsl #16\n"
          "    bx      lr");

    //
    // This return is never reached but is required to avoid a compiler
    // warning.
    //
    return(0);
}
#endif
#if defined(rvmdk) || defined(__ARMCC_VERSION)
__asm long
MainLongMul(long lX, long lY)
{
    //
    // The assembly code to efficiently perform the multiply (using the
    // instruction to multiply two 32-bit values and return the full 64-bit
    // result).
    //
    smull   r0, r1, r0, r1;
    lsrs    r0, r0, #16;
    orr     r0, r0, r1, lsl #16;
    bx      lr;
}
#endif
//
// For CCS implement this function in pure assembly.  This prevents the TI
// compiler from doing funny things with the optimizer.
//
#if defined(ccs)
long MainLongMul(long lX, long lY);
    __asm("    .sect \".text:MainLongMul\"\n"
          "    .clink\n"
          "    .thumbfunc MainLongMul\n"
          "    .thumb\n"
          "    .global MainLongMul\n"
          "MainLongMul:\n"
          "    smull   r0, r1, r0, r1\n"
          "    lsrs    r0, r0, #16\n"
          "    orr     r0, r0, r1, lsl #16\n"
          "    bx      lr\n");
#endif


//*****************************************************************************
//
//! Changes the PWM frequency of the motor drive.
//!
//! This function changes the period of the PWM signals produced by the motor
//! drive.  It is simply a wrapper function around the PWMSetFrequency()
//! function; the PWM frequency-based timing parameters of the motor drive are
//! adjusted as part of the PWM frequency update.
//!
//! \return None.
//
//*****************************************************************************
void
MainSetPWMFrequency(void)
{
    //
    // Disable the update interrupts temporarily.
    //
    IntDisable(INT_PWM0_1);
    IntDisable(INT_PWM0_2);

    //
    // Set the new PWM frequency.
    //
    PWMSetFrequency();

    //
    // Compute the new angle delta based on the new PWM frequency.
    //
    g_ulAngleDelta = ((g_ulFrequency * 128) / g_ulPWMFrequency) * 512;

    //
    // Re-enable the update interrupts.
    //
    IntEnable(INT_PWM0_1);
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
//! Changes the target frequency of the motor drive.
//!
//! This function changes the target frequency of the motor drive.  If
//! required, the state machine will be transitioned to a new state in order to
//! move the motor drive to the target frequency.
//!
//! \return None.
//
//*****************************************************************************
void
MainSetFrequency(void)
{
    //
    // Clip the target frequency to the minimum frequency if it is too small.
    //
    if(g_usTargetFrequency < g_sParameters.usMinFrequency)
    {
        g_usTargetFrequency = g_sParameters.usMinFrequency;
    }

    //
    // Clip the target frequency to the maximum frequency if it is too large.
    //
    if(g_usTargetFrequency > g_sParameters.usMaxFrequency)
    {
        g_usTargetFrequency = g_sParameters.usMaxFrequency;
    }

    //
    // Compute the 16.16 fixed-point representation of the target frequency.
    //
    g_ulTargetFrequency = (g_usTargetFrequency * 65536) / 10;
}

//*****************************************************************************
//
//! Sets the direction of the motor drive.
//!
//! \param bForward is a boolean that is true if the motor drive should be
//! run in the forward direction.
//!
//! This function changes the direction of the motor drive.  If required, the
//! state machine will be transitioned to a new state in order to change the
//! direction of the motor drive.
//!
//! \return None.
//
//*****************************************************************************
void
MainSetDirection(tBoolean bForward)
{
    //
    // Temporarily disable the millisecond interrupt.
    //
    IntDisable(INT_PWM0_2);

    //
    // See if the motor should be running in the forward direction.
    //
    if(bForward)
    {
        //
        // See if the motor is presently running backward.
        //
        if(g_ulState == STATE_BACK_RUN)
        {
            //
            // If not already decelerating, then set the initial deceleration
            // rate based on the configured parameter value.
            //
            if(g_ucMotorStatus != MOTOR_STATUS_DECEL)
            {
                g_ulDecelRate = g_sParameters.ucDecel * 65536;
            }

            //
            // Advance the state machine to the decelerate to reverse direction
            // state.
            //
            g_ulState = STATE_BACK_REV;
        }

        //
        // See if the motor is presently running forward but in the process of
        // reversing to the backward direction.
        //
        if(g_ulState == STATE_REV)
        {
            //
            // Leave the motor drive running in the forward direction.
            //
            g_ulState = STATE_RUN;
        }
    }

    //
    // Otherwise the motor should be running in the backward direction.
    //
    else
    {
        //
        // See if the motor is presently running forward.
        //
        if(g_ulState == STATE_RUN)
        {
            //
            // If not already decelerating, then set the initial deceleration
            // rate based on the configured parameter value.
            //
            if(g_ucMotorStatus != MOTOR_STATUS_DECEL)
            {
                g_ulDecelRate = g_sParameters.ucDecel * 65536;
            }

            //
            // Advance the state machine to the decelerate to reverse direction
            // state.
            //
            g_ulState = STATE_REV;
        }

        //
        // See if the motor is presently running backward but in the process of
        // reversing to the forward direction.
        //
        if(g_ulState == STATE_BACK_REV)
        {
            //
            // Leave the motor drive running in the backward direction.
            //
            g_ulState = STATE_BACK_RUN;
        }
    }

    //
    // Re-enable the millisecond interrupt.
    //
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
//! Sets the open-/closed-loop mode of the motor drive.
//!
//! \param bClosed is a boolean that is true if the motor drive should be run
//! in closed-loop mode.
//!
//! This function changes the open-/closed-loop mode of the motor drive.  When
//! enabling closed-loop mode, the integrator is initialized as if the current
//! motor frequency was achieved in closed-loop mode; this provides a smoother
//! transition into closed-loop mode.
//!
//! \return None.
//
//*****************************************************************************
void
MainSetLoopMode(tBoolean bClosed)
{
    //
    // Temporarily disable the millisecond interrupt.
    //
    IntDisable(INT_PWM0_2);

    //
    // See if the motor drive should be running in closed loop mode.
    //
    if(bClosed)
    {
        //
        // See if close loop mode is currently disabled.
        //
        if(HWREGBITH(&(g_sParameters.usFlags), FLAG_LOOP_BIT) ==
           FLAG_LOOP_OPEN)
        {
            //
            // Enabled closed loop mode.
            //
            HWREGBITH(&(g_sParameters.usFlags), FLAG_LOOP_BIT) =
                FLAG_LOOP_CLOSED;

            //
            // Initialize the integrator.
            //
            g_lFrequencyIntegrator = ((g_ulFrequency * 64) /
                                      g_sParameters.lFAdjI);
        }
    }
    else
    {
        //
        // Disable closed loop mode.
        //
        HWREGBITH(&(g_sParameters.usFlags), FLAG_LOOP_BIT) = FLAG_LOOP_OPEN;
    }

    //
    // Re-enable the millisecond interrupt.
    //
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
//! Updates the I coefficient of the frequency PI controller.
//!
//! \param lNewFAdjI is the new value of the I coefficient.
//!
//! This function updates the value of the I coefficient of the frequency PI
//! controller.  In addition to updating the I coefficient, it recomputes the
//! maximum value of the integrator and the current value of the integrator in
//! terms of the new I coefficient (eliminating any instantaneous jump in the
//! output of the PI controller).
//!
//! \return None.
//
//*****************************************************************************
void
MainUpdateFAdjI(long lNewFAdjI)
{
    //
    // Temporarily disable the millisecond interrupt.
    //
    IntDisable(INT_PWM0_2);

    //
    // See if the new I coefficient is zero.
    //
    if(lNewFAdjI == 0)
    {
        //
        // Since the I coefficient is zero, the integrator and integrator
        // maximum are also zero.
        //
        g_lFrequencyIntegratorMax = 0;
        g_lFrequencyIntegrator = 0;
    }
    else
    {
        //
        // Compute the maximum value of the integrator.  This is the value that
        // results in the maximum output frequency (i.e. integrator max * I =
        // max frequency).
        //
        g_lFrequencyIntegratorMax = ((((g_sParameters.usMaxFrequency * 65536) /
                                       10) * 64) / lNewFAdjI);

        //
        // Adjust the current value of the integrator to account for the new
        // value of the I coefficient.  This satisfies the equation "old
        // integrator * old I = new integrator * new I", so that the output
        // doesn't immediately change (in a drastic way) as a result of the
        // change in the I coefficient.
        //
        g_lFrequencyIntegrator = ((g_lFrequencyIntegrator / lNewFAdjI) *
                                  g_sParameters.lFAdjI);
    }

    //
    // Save the new I coefficient.
    //
    g_sParameters.lFAdjI = lNewFAdjI;

    //
    // Re-enable the millisecond interrupt.
    //
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
//! Handles the waveform update software interrupt.
//!
//! This function is periodically called as a result of the waveform update
//! software interrupt being asserted.  This interrupt is asserted at the
//! requested rate (based on the update rate parameter) by the PWM interrupt
//! handler.
//!
//! The angle of the motor drive will be updated, and new waveform values
//! computed and supplied to the PWM module.
//!
//! \note Since this interrupt is software triggered, there is no interrupt
//! source to clear in this handler.
//!
//! \return None.
//
//*****************************************************************************
void
MainWaveformTick(void)
{
    unsigned long ulAmplitude, pulDutyCycles[3];

    //
    // There is nothing to be done if the motor drive is not running.
    //
    if((g_ulState == STATE_STOPPED) || (g_ulState == STATE_PRECHARGE) ||
       (g_ulState == STATE_BACK_PRECHARGE) || (g_ulState == STATE_BRAKE))
    {
        //
        // Reduce the PWM period count based on the number of updates that
        // would have occurred if the motor drive was running.
        //
        PWMReducePeriodCount((PWMGetPeriodCount() /
                              (g_sParameters.ucUpdateRate + 1)) *
                             (g_sParameters.ucUpdateRate + 1));

        //
        // Return without doing anything further.
        //
        return;
    }

    //
    // Loop until the PWM period count is less than the update rate.  The angle
    // is updated based on the number of update periods that have passed, which
    // may be more than one.  After the angle is updated, the waveform
    // moulations are computed.  Since the period count may go above the update
    // rate while computing the waveform modulation, this outer loop is
    // required to ensure that the all update periods are accounted for before
    // this routine returns.
    //
    while(PWMGetPeriodCount() > g_sParameters.ucUpdateRate)
    {
        //
        // Get the number of updates that are pending.  Normally, this will be
        // one, but may be larger if the processor is heavily loaded.
        //
        ulAmplitude = PWMGetPeriodCount() / (g_sParameters.ucUpdateRate + 1);

        //
        // See if the motor drive direction is forward or backward.
        //
        if(g_ulState & STATE_FLAG_FORWARD)
        {
            //
            // The motor drive direction is forward, so increment the motor
            // drive angle by the angle delta.
            //
            g_ulAngle += (g_ulAngleDelta * (g_sParameters.ucUpdateRate + 1) *
                          ulAmplitude);
        }
        else
        {
            //
            // The motor drive direction is backward, so decrement the motor
            // drive angle by the angle delta.
            //
            g_ulAngle -= (g_ulAngleDelta * (g_sParameters.ucUpdateRate + 1) *
                          ulAmplitude);
        }

        //
        // Reduce the PWM period count by the number of updates just performed.
        //
        PWMReducePeriodCount(ulAmplitude * (g_sParameters.ucUpdateRate + 1));

        //
        // Based on the current frequency, get the appropriate amplitude of the
        // waveform.
        //
        ulAmplitude = VFGetAmplitude(g_ulFrequency);

        //
        // See if DC bus fluctuation compensation should be performed.
        //
        if(HWREGBITH(&(g_sParameters.usFlags), FLAG_BUS_COMP_BIT) ==
           FLAG_BUS_COMP_ON)
        {
            //
            // Adjust the waveform amplitude to compensate for fluctuations in
            // the DC bus voltage.  The nominal DC bus voltage is 325 V; do not
            // compensate for a DC bus voltage below 260 V (i.e. treat voltages
            // below 260 V as 260 V).
            //
            if(g_usBusVoltage < 260)
            {
                ulAmplitude = (ulAmplitude * 325) / 260;
            }
            else
            {
                ulAmplitude = (ulAmplitude * 325) / g_usBusVoltage;
            }

            //
            // Do not allow the amplitude to exceed 125%.
            //
            if(ulAmplitude > ((65536 * 5) / 4))
            {
                ulAmplitude = (65536 * 5) / 4;
            }
        }

        //
        // Determine the type of modulation to perform.
        //
        if(HWREGBITH(&(g_sParameters.usFlags), FLAG_DRIVE_BIT) ==
           FLAG_DRIVE_SINE)
        {
            //
            // Perform sine wave modulation.
            //
            SineModulate(g_ulAngle, ulAmplitude, pulDutyCycles);
        }
        else
        {
            //
            // Perform space vector modulation.
            //
            SpaceVectorModulate(g_ulAngle, ulAmplitude, pulDutyCycles);
        }

        //
        // Set the new duty cycle.
        //
        PWMSetDutyCycle(pulDutyCycles[0], pulDutyCycles[1], pulDutyCycles[2]);
    }
}

//*****************************************************************************
//
//! Handles the DC braking mode of the motor drive.
//!
//! This function performs the processing and state transitions associated with
//! the DC braking mode of the motor drive.
//!
//! \return None.
//
//*****************************************************************************
static void
MainDCBrakeHandler(void)
{
    //
    // Decrement the count of milliseconds while in this state.
    //
    g_ulStateCount--;

    //
    // See if the motor drive has been in the braking state long enough.
    //
    if(g_ulStateCount != 0)
    {
        //
        // There is nothing further to be done for this state.
        //
        return;
    }

    //
    // The DC injection braking period has completed, so turn off the PWM
    // outputs.
    //
    PWMOutputOff();

    //
    // Indicate that the motor drive is no longer running by changing the blink
    // rate on the run LED.
    //
    UIRunLEDBlink(200, 25);

    //
    // Advance the state machine to the stopped state.
    //
    g_ulState = STATE_STOPPED;

    //
    // Set the motor status to stopped.
    //
    g_ucMotorStatus = MOTOR_STATUS_STOP;
}

//*****************************************************************************
//
//! Handles the gate driver precharge mode of the motor drive.
//!
//! This function performs the processing and state transitions associated with
//! the gate driver precharge mode of the motor drive.
//!
//! \return None.
//
//*****************************************************************************
static void
MainPrechargeHandler(void)
{
    //
    // Decrement the count of milliseconds while in this state.
    //
    g_ulStateCount--;

    //
    // See if the motor drive has been in the precharge state long enough.
    //
    if(g_ulStateCount != 0)
    {
        //
        // There is nothing further to be done for this state.
        //
        return;
    }

    //
    // Turn on all the PWM outputs.
    //
    PWMOutputOn();

    //
    // Advance the state machine to the appropriate acceleration state based on
    // the motor direction.
    //
    if(g_ulState == STATE_PRECHARGE)
    {
        g_ulState = STATE_RUN;
    }
    else
    {
        g_ulState = STATE_BACK_RUN;
    }

    //
    // Start the motor drive at zero.
    //
    g_ulFrequency = 0;

    //
    // Set the frequency fraction and whole part.
    //
    g_ulFrequencyWhole = 0;
    g_ulFrequencyFract = 0;

    //
    // Set the motor drive frequency value that is used by the serial user
    // interface.
    //
    g_usCurrentFrequency = 0;

    //
    // Reset the integrator.
    //
    g_lFrequencyIntegrator = 0;

    //
    // Start the motor drive at an angle of zero degrees.
    //
    g_ulAngle = 0;
}

//*****************************************************************************
//
//! Checks for motor drive faults.
//!
//! This function checks for fault conditions that may occur during the
//! operation of the motor drive.  The ambient temperature, DC bus voltage, and
//! motor current are all monitored for fault conditions.
//!
//! \return None.
//
//*****************************************************************************
static void
MainCheckFaults(void)
{
    //
    // See if the ambient temperature is above the maximum value.
    //
    if(g_sAmbientTemp > (short)g_sParameters.ucMaxTemperature)
    {
        //
        // Emergency stop the motor drive.
        //
        MainEmergencyStop();

        //
        // Indicate an ambient overtemperature fault.
        //
        MainSetFault(FAULT_TEMPERATURE_HIGH);
    }

    //
    // See if the DC bus voltage is below the minimum value.
    //
    if(g_usBusVoltage < g_sParameters.usMinVBus)
    {
        //
        // Emergency stop the motor drive.
        //
        MainEmergencyStop();

        //
        // Indicate a DC bus undervoltage fault.
        //
        MainSetFault(FAULT_VBUS_LOW);
    }

    //
    // See if the DC bus voltage is above the maximum value.
    //
    if(g_usBusVoltage > g_sParameters.usMaxVBus)
    {
        //
        // Emergency stop the motor drive.
        //
        MainEmergencyStop();

        //
        // Indicate a DC bus overvoltage fault.
        //
        MainSetFault(FAULT_VBUS_HIGH);
    }

    //
    // See if the motor current is below the minimum value.  This check is only
    // performed when the motor is running and is being driven at or above its
    // minimum frequency.
    //
    if((g_usMotorCurrent <
        (((unsigned short)g_sParameters.ucMinCurrent * 256) / 10)) &&
       (g_ulState != STATE_STOPPED) &&
       (g_usCurrentFrequency >= g_sParameters.usMinFrequency))
    {
        //
        // Emergency stop the motor drive.
        //
        MainEmergencyStop();

        //
        // Indicate a motor undercurrent fault.
        //
        MainSetFault(FAULT_CURRENT_LOW);
    }

    //
    // See if the motor current is above the maximum value.
    //
    if((g_sParameters.ucMaxCurrent != 0) &&
       (g_usMotorCurrent >
        (((unsigned short)g_sParameters.ucMaxCurrent * 256) / 10)))
    {
        //
        // Emergency stop the motor drive.
        //
        MainEmergencyStop();

        //
        // Indicate a motor overcurrent fault.
        //
        MainSetFault(FAULT_CURRENT_HIGH);
    }
}

//*****************************************************************************
//
//! Adjusts the motor drive frequency based on the rotor frequency.
//!
//! This function uses a PI controller to adjust the motor drive frequency in
//! order to get the rotor frequency to match the target frequency (meaning
//! that the motor drive frequency will actually be above the target
//! frequency).
//!
//! \return Returns the new motor drive target frequency.
//
//*****************************************************************************
unsigned long
MainFrequencyController(void)
{
    long lTemp, lError;

    //
    // Compute the error between the target frequency and the rotor frequency.
    //
    lError = (((long)(g_ulTargetFrequency / 256) -
               (long)((g_usRotorFrequency * 256) / 10)));

    //
    // Add the error to the integrator accumulator, limiting the value of the
    // accumulator.
    //
    g_lFrequencyIntegrator += lError;
    if(g_lFrequencyIntegrator > g_lFrequencyIntegratorMax)
    {
        g_lFrequencyIntegrator = g_lFrequencyIntegratorMax;
    }
    if(g_lFrequencyIntegrator < 0)
    {
        g_lFrequencyIntegrator = 0;
    }

    //
    // Perform the actual PI controller computation.
    //
    lTemp = (MainLongMul(g_sParameters.lFAdjP, lError) +
             MainLongMul(g_sParameters.lFAdjI, g_lFrequencyIntegrator));

    //
    // Limit the output of the PI controller based on the maximum frequency
    // parameter (and don't allow it to go below zero).
    //
    if(lTemp < 0)
    {
        lTemp = 0;
    }
    if(lTemp > ((g_sParameters.usMaxFrequency * 64) / 10))
    {
        lTemp = (g_sParameters.usMaxFrequency * 64) / 10;
    }

    //
    // Return the output of the PI controller.
    //
    return(lTemp * 1024);
}

//*****************************************************************************
//
//! Adjusts the motor drive frequency based on the target frequency.
//!
//! \param ulTarget is the target frequency of the motor drive, specified as a
//! 16.16 fixed-point value.
//!
//! This function adjusts the motor drive frequency towards a given target
//! frequency.  Limitations such as acceleration and deceleration rate, along
//! with precautions such as limiting the deceleration rate to control the DC
//! bus voltage, are handled by this function.
//!
//! \return None.
//
//*****************************************************************************
static void
MainFrequencyHandler(unsigned long ulTarget)
{
    unsigned long ulNewValue;

    //
    // Return without doing anything if the target frequency has already been
    // reached.
    //
    if(ulTarget == g_ulFrequency)
    {
        return;
    }

    //
    // See if the target frequency is greater than the current frequency.
    //
    if(ulTarget > g_ulFrequency)
    {
        //
        // Compute the new maximum acceleration rate, based on the present
        // motor current.
        //
        if(g_usMotorCurrent >=
           (((g_sParameters.ucAccelCurrent + 20) * 256) / 10))
        {
            ulNewValue = g_sParameters.ucAccel * 128;
        }
        else
        {
            ulNewValue = (g_sParameters.ucAccel * 128 *
                          ((((g_sParameters.ucAccelCurrent + 20) * 256) / 10) -
                           g_usMotorCurrent));
        }

        //
        // See if the acceleration rate is greater than the requested
        // acceleration rate (i.e. the acceleration rate has been changed).
        //
        if(g_ulAccelRate > (g_sParameters.ucAccel * 65536))
        {
            //
            // Reduce the acceleration rate to the requested rate.
            //
            g_ulAccelRate = g_sParameters.ucAccel * 65536;
        }

        //
        // Then, see if the motor current exceeds the current at which the
        // acceleration rate should be reduced, and the newly computed
        // acceleration rate is less than the current rate.
        //
        else if((g_usMotorCurrent >
                 ((g_sParameters.ucAccelCurrent * 256) / 10)) &&
                (ulNewValue < g_ulAccelRate))
        {
            //
            // Set the acceleration rate to the newly computed acceleration
            // rate.
            //
            g_ulAccelRate = ulNewValue;
        }

        //
        // Otherwise, see if the acceleration rate is less than the requested
        // acceleration rate.
        //
        else if(g_ulAccelRate < (g_sParameters.ucAccel * 65536))
        {
            //
            // Increase the acceleration rate by 1/4 of a Hertz, slowly
            // returning it to the desired rate.
            //
            g_ulAccelRate += 65536 / 4;
        }

        //
        // Increase the frequency fraction by the acceleration rate.
        //
        g_ulFrequencyFract += g_ulAccelRate / 65536;

        //
        // Loop while the fraction is greater than one.
        //
        while(g_ulFrequencyFract >= 1000)
        {
            //
            // Increment the frequency whole part.
            //
            g_ulFrequencyWhole++;

            //
            // Decrement the frequency fraction by one.
            //
            g_ulFrequencyFract -= 1000;
        }

        //
        // Convert the frequency fraction and whole part into a 16.16 motor
        // drive frequency.
        //
        g_ulFrequency = ((g_ulFrequencyWhole * 65536) +
                         ((g_ulFrequencyFract * 65536) / 1000));

        //
        // See if the frequency has exceeded the target frequency.
        //
        if(g_ulFrequency >= ulTarget)
        {
            //
            // Set the motor drive frequency to the target frequency.
            //
            g_ulFrequency = ulTarget;

            //
            // Compute the frequency fraction and whole part from the drive
            // frequency.
            //
            g_ulFrequencyWhole = ulTarget / 65536;
            g_ulFrequencyFract = ((ulTarget & 65535) * 1000) / 65536;

            //
            // Set the motor status to running.
            //
            g_ucMotorStatus = MOTOR_STATUS_RUN;
        }
        else
        {
            //
            // Set the motor status to accelerating.
            //
            g_ucMotorStatus = MOTOR_STATUS_ACCEL;
        }
    }

    //
    // Otherwise, the target frequency is less than the current frequency.
    //
    else
    {
        //
        // Compute the new maximum deceleration rate, based on the current bus
        // voltage.
        //
        if(g_usBusVoltage > (g_sParameters.usDecelV + 63))
        {
            ulNewValue = g_sParameters.ucDecel * 1024;
        }
        else
        {
            ulNewValue = (g_sParameters.ucDecel * 1024 *
                          (64 - g_usBusVoltage + g_sParameters.usDecelV));
        }

        //
        // See if the deceleration rate is greater than the requested
        // deceleration rate (i.e. the deceleration rate has been changed).
        //
        if(g_ulDecelRate > (g_sParameters.ucDecel * 65536))
        {
            //
            // Reduce the deceleration rate to the requested rate.
            //
            g_ulDecelRate = g_sParameters.ucDecel * 65536;
        }

        //
        // Then, see if the bus voltage exceeds the voltage at which the
        // deceleration rate should be reduced, and the newly computed
        // deceleration rate is less than the current rate.
        //
        else if((g_usBusVoltage > g_sParameters.usDecelV) &&
                (ulNewValue < g_ulDecelRate))
        {
            //
            // Set the deceleration rate to the newly computed deceleration
            // rate.
            //
            g_ulDecelRate = ulNewValue;
        }

        //
        // Otherwise, see if the deceleration rate is less than the requested
        // deceleration rate.
        //
        else if(g_ulDecelRate < (g_sParameters.ucDecel * 65536))
        {
            //
            // Increase the deceleration rate by 1/4 of a Hertz, slowly
            // returning it to the desired rate.
            //
            g_ulDecelRate += 65536 / 4;
        }

        //
        // Decrease the frequency fraction by the decleration rate.
        //
        g_ulFrequencyFract -= g_ulDecelRate / 65536;

        //
        // Loop while the fraction is less than zero.
        //
        while(g_ulFrequencyFract >= 1000)
        {
            //
            // Decrement the frequency whole part.
            //
            g_ulFrequencyWhole--;

            //
            // Increment the frequency fraction by one.
            //
            g_ulFrequencyFract += 1000;
        }

        //
        // Convert the frequency fraction and whole part into a 16.16 motor
        // drive frequency.
        //
        g_ulFrequency = ((g_ulFrequencyWhole * 65536) +
                         ((g_ulFrequencyFract * 65536) / 1000));

        //
        // See if the target frequency has been reached (for non-zero target
        // frequencies).
        //
        if((ulTarget != 0) && (g_ulFrequency < ulTarget))
        {
            //
            // Set the motor drive frequency to the target frequency.
            //
            g_ulFrequency = ulTarget;

            //
            // Compute the frequency fraction and whole part from the drive
            // frequency.
            //
            g_ulFrequencyWhole = g_ulFrequency / 65536;
            g_ulFrequencyFract = (((g_ulFrequency & 65535) * 1000) /
                                  65536);

            //
            // Set the motor status to running.
            //
            g_ucMotorStatus = MOTOR_STATUS_RUN;
        }

        //
        // See if the frequency has reached zero.
        //
        else if((g_ulFrequency > 0xff000000) || (g_ulFrequency == 0))
        {
            //
            // Set the motor drive frequency to zero.
            //
            g_ulFrequency = 0;

            //
            // The frequency fraction and whole part are zero as well.
            //
            g_ulFrequencyWhole = 0;
            g_ulFrequencyFract = 0;

            //
            // See if the motor drive is stopping.
            //
            if(g_ulState & STATE_FLAG_STOPPING)
            {
                //
                // See if DC injection braking is enabled.
                //
                if(HWREGBITH(&(g_sParameters.usFlags), FLAG_DC_BRAKE_BIT) ==
                   FLAG_DC_BRAKE_ON)
                {
                    //
                    // Start DC injection braking.
                    //
                    PWMOutputDCBrake(g_sParameters.usDCBrakeV);

                    //
                    // Get the number of milliseconds to remain in the braking
                    // state.
                    //
                    g_ulStateCount = g_sParameters.usDCBrakeTime;

                    //
                    // Blink the run LED fast to indicate that DC injection
                    // braking is occurring.
                    //
                    UIRunLEDBlink(20, 10);

                    //
                    // Advance the state machine to the DC injection braking
                    // state.
                    //
                    g_ulState = STATE_BRAKE;
                }

                //
                // Otherwise, DC injection braking is disabled.
                //
                else
                {
                    //
                    // Turn off the PWM outputs.
                    //
                    PWMOutputOff();

                    //
                    // Indicate that the motor drive is no longer running by
                    // changing the blink rate on the run LED.
                    //
                    UIRunLEDBlink(200, 25);

                    //
                    // Advance the state machine to the stopped state.
                    //
                    g_ulState = STATE_STOPPED;

                    //
                    // Set the motor status to stopped.
                    //
                    g_ucMotorStatus = MOTOR_STATUS_STOP;
                }
            }

            //
            // Otherwise, the motor drive is not stopping.
            //
            else
            {
                //
                // Set the motor drive to the correct run state based on the
                // present direction (i.e. reverse direction).
                //
                if(g_ulState == STATE_REV)
                {
                    g_ulState = STATE_BACK_RUN;
                }
                else
                {
                    g_ulState = STATE_RUN;
                }
            }
        }
        else
        {
            //
            // Set the motor status to decelerating.
            //
            g_ucMotorStatus = MOTOR_STATUS_DECEL;
        }
    }

    //
    // Compute the motor drive frequency value that is used by the serial user
    // interface.
    //
    g_usCurrentFrequency = ((g_ulFrequency * 10) + 32768) / 65536;
}

//*****************************************************************************
//
//! Handles the millisecond frequency update software interrupt.
//!
//! This function is called as a result of the frequency update software
//! interrupt being asserted.  This interrupted is asserted every millisecond
//! by the PWM interrupt handler.
//!
//! The frequency of the motor drive will be updated, along with handling state
//! changes of the drive (such as initiating braking when the motor drive has
//! come to a stop).
//!
//! \note Since this interrupt is software triggered, there is no interrupt
//! source to clear in this handler.
//!
//! \return None.
//
//*****************************************************************************
void
MainMillisecondTick(void)
{
    unsigned long ulTarget;

    //
    // Update the state of the dynamic brake.
    //
    BrakeTick();

    //
    // Check for fault conditions.
    //
    MainCheckFaults();

    //
    // If the motor drive is currently stopped then there is nothing to be
    // done.
    //
    if(g_ulState == STATE_STOPPED)
    {
        return;
    }

    //
    // See if the motor drive is in DC injection braking mode.
    //
    if(g_ulState == STATE_BRAKE)
    {
        //
        // Handle DC injection braking.
        //
        MainDCBrakeHandler();

        //
        // There is nothing further to be done for this state.
        //
        return;
    }

    //
    // See if the motor drive is in precharge mode.
    //
    if(g_ulState & STATE_FLAG_PRECHARGE)
    {
        //
        // Handle precharge mode.
        //
        MainPrechargeHandler();

        //
        // There is nothing further to be done for this state.
        //
        return;
    }

    //
    // See if the motor drive is in run mode.
    //
    if(g_ulState & STATE_FLAG_RUN)
    {
        //
        // Determine the target frequency.  First, see if the motor drive is
        // stopping or reversing direction.
        //
        if((g_ulState & STATE_FLAG_STOPPING) || (g_ulState & STATE_FLAG_REV))
        {
            //
            // When stopping or reversing direction, the target frequency is
            // zero.
            //
            ulTarget = 0;
        }

        //
        // Then, see if the motor drive is in closed loop mode.
        //
        else if(HWREGBITH(&(g_sParameters.usFlags), FLAG_LOOP_BIT) ==
                FLAG_LOOP_CLOSED)
        {
            //
            // Get the target frequency from the frequency PI controller.
            //
            ulTarget = MainFrequencyController();
        }

        //
        // Otherwise, the motor drive is in open loop mode.
        //
        else
        {
            //
            // The target frequency is the user supplied value.
            //
            ulTarget = g_ulTargetFrequency;
        }

        //
        // Handle the update to the motor drive frequency based on the target
        // frequency.
        //
        MainFrequencyHandler(ulTarget);

        //
        // Compute the angle delta based on the new motor drive frequency.
        //
        g_ulAngleDelta = ((g_ulFrequency * 128) / g_ulPWMFrequency) * 512;
    }
}

//*****************************************************************************
//
//! Starts the motor drive.
//!
//! This function starts the motor drive.  If the motor is currently stopped,
//! it will begin the process of starting the motor.  If the motor is currently
//! stopping, it will cancel the stop operation and return the motor to the
//! target frequency.
//!
//! \return None.
//
//*****************************************************************************
void
MainRun(void)
{
    //
    // Do not allow the motor drive to start while there is an uncleared fault
    // condition.
    //
    if(MainIsFaulted())
    {
        return;
    }

    //
    // Temporarily disable the millisecond interrupt.
    //
    IntDisable(INT_PWM0_2);

    //
    // See if the motor drive is presently stopped.
    //
    if(g_ulState == STATE_STOPPED)
    {
        //
        // Set the initial acceleration and deceleration based on the current
        // parameter values.
        //
        g_ulAccelRate = g_sParameters.ucAccel * 65536;
        g_ulDecelRate = g_sParameters.ucDecel * 65536;

        //
        // Indicate that the motor drive is running by changing the blink rate
        // on the run LED.
        //
        UIRunLEDBlink(200, 175);

        //
        // Set the PWM outputs to start precharging the bootstrap capacitors on
        // the high side gate drivers.
        //
        PWMOutputPrecharge();

        //
        // Get the number of milliseconds to remain in the precharge state.
        //
        g_ulStateCount = g_sParameters.ucPrechargeTime;

        //
        // See if the motor drive should run forward or backward.
        //
        if(HWREGBITH(&(g_sParameters.usFlags), FLAG_DIR_BIT) ==
           FLAG_DIR_FORWARD)
        {
            //
            // Advance the state machine to the precharge state for running in
            // the forward direction.
            //
            g_ulState = STATE_PRECHARGE;
        }
        else
        {
            //
            // Advance the state machine to the precharge state for running in
            // the backward direction.
            //
            g_ulState = STATE_BACK_PRECHARGE;
        }
    }

    //
    // See if the motor drive is presently stopping while running forward.
    //
    else if(g_ulState == STATE_STOPPING)
    {
        //
        // Leave the motor drive running.
        //
        g_ulState = STATE_RUN;
    }

    //
    // See if the motor drive is presently stopping while running backward.
    //
    else if(g_ulState == STATE_BACK_STOPPING)
    {
        //
        // Leave the motor drive running in the backward direction.
        //
        g_ulState = STATE_BACK_RUN;
    }

    //
    // Re-enable the millisecond interrupt.
    //
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
//! Stops the motor drive.
//!
//! This function stops the motor drive.  If the motor is currently running,
//! it will begin the process of stopping the motor.
//!
//! \return None.
//
//*****************************************************************************
void
MainStop(void)
{
    //
    // Temporarily disable the millisecond interrupt.
    //
    IntDisable(INT_PWM0_2);

    //
    // See if the motor is running in the forward direction.
    //
    if(g_ulState == STATE_RUN)
    {
        //
        // If not already decelerating, then set the initial deceleration rate
        // based on the configured parameter value.
        //
        if(g_ucMotorStatus != MOTOR_STATUS_DECEL)
        {
            g_ulDecelRate = g_sParameters.ucDecel * 65536;
        }

        //
        // Advance the state machine to the forward decelerate to a stop state.
        //
        g_ulState = STATE_STOPPING;
    }

    //
    // See if the motor is running in the backward direction.
    //
    if(g_ulState == STATE_BACK_RUN)
    {
        //
        // If not already decelerating, then set the initial deceleration rate
        // based on the configured parameter value.
        //
        if(g_ucMotorStatus != MOTOR_STATUS_DECEL)
        {
            g_ulDecelRate = g_sParameters.ucDecel * 65536;
        }

        //
        // Advance the state machine to the backward decelerate to a stop
        // state.
        //
        g_ulState = STATE_BACK_STOPPING;
    }

    //
    // Re-enable the millisecond interrupt.
    //
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
//! Emergency stops the motor drive.
//!
//! This function performs an emergency stop of the motor drive.  The outputs
//! will be shut down immediately, the drive put into the stopped state with
//! the frequency at zero, and the emergency stop fault condition will be
//! asserted.
//!
//! \return None.
//
//*****************************************************************************
void
MainEmergencyStop(void)
{
    //
    // Temporarily disable the update interrupts.
    //
    IntDisable(INT_PWM0_1);
    IntDisable(INT_PWM0_2);

    //
    // Disable all the PWM outputs.
    //
    PWMOutputOff();

    //
    // Indicate that the motor drive is no longer running by changing the blink
    // rate on the run LED.
    //
    UIRunLEDBlink(200, 25);

    //
    // Set the state machine to the stopped state.
    //
    g_ulState = STATE_STOPPED;

    //
    // Set the motor status to stopped.
    //
    g_ucMotorStatus = MOTOR_STATUS_STOP;

    //
    // Set the motor drive frequency to zero.
    //
    g_usCurrentFrequency = 0;

    //
    // Re-enable the update interrupts.
    //
    IntEnable(INT_PWM0_1);
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
//! Determines if the motor drive is currently running.
//!
//! This function will determine if the motor drive is currently running.  By
//! this definition, running means not stopped; the motor drive is considered
//! to be running even when it is precharging before starting the waveforms and
//! DC injection braking after stopping the waveforms.
//!
//! \return Returns 0 if the motor drive is stopped and 1 if it is running.
//
//*****************************************************************************
unsigned long
MainIsRunning(void)
{
    //
    // Return the appropriate value based on whether or not the motor drive is
    // stopped.
    //
    if(g_ulState == STATE_STOPPED)
    {
        return(0);
    }
    else
    {
        return(1);
    }
}

//*****************************************************************************
//
//! Indicate that a fault condition has been detected.
//!
//! \param ulFaultFlag is a flag that indicates the fault condition that was
//! detected.
//!
//! This function is called when a fault condition is detected.  It will update
//! the fault flags to indicate the fault condition that was detected, and
//! cause the fault LED to blink to indicate a fault.
//!
//! \return None.
//
//*****************************************************************************
void
MainSetFault(unsigned long ulFaultFlag)
{
    //
    // Add the new fault condition to the fault flags.
    //
    g_ulFaultFlags |= ulFaultFlag;

    //
    // Flash the fault LED rapidly to indicate a fault.
    //
    UIFaultLEDBlink(20, 10);
}

//*****************************************************************************
//
//! Clears the latched fault conditions.
//!
//! This function will clear the latched fault conditions and turn off the
//! fault LED.
//!
//! \return None.
//
//*****************************************************************************
void
MainClearFaults(void)
{
    //
    // Clear the fault flags.
    //
    g_ulFaultFlags = 0;

    //
    // Turn off the fault LED.
    //
    UIFaultLEDBlink(0, 0);
}

//*****************************************************************************
//
//! Determines if a latched fault condition exists.
//!
//! This function determines if a fault condition has occurred but not been
//! cleared.
//!
//! \return Returns 1 if there is an uncleared fault condition and 0 otherwise.
//
//*****************************************************************************
unsigned long
MainIsFaulted(void)
{
    //
    // Determine if a latched fault condition exists.
    //
    return(g_ulFaultFlags ? 1 : 0);
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives a NMI.  This
// simply enters an infinite loop, preserving the system state for examination
// by a debugger.
//
//*****************************************************************************
void
NmiSR(void)
{
    //
    // Disable all interrupts.
    //
    IntMasterDisable();

    //
    // Turn off all the PWM outputs.
    //
    PWMOutputOff();

    //
    // Turn on the fault LED.
    //
    UIFaultLEDBlink(1, 1);

    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives a fault
// interrupt.  This simply enters an infinite loop, preserving the system state
// for examination by a debugger.
//
//*****************************************************************************
void
FaultISR(void)
{
    //
    // Disable all interrupts.
    //
    IntMasterDisable();

    //
    // Turn off all the PWM outputs.
    //
    PWMOutputOff();

    //
    // Turn on the fault LED.
    //
    UIFaultLEDBlink(1, 1);

    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives an unexpected
// interrupt.  This simply enters an infinite loop, preserving the system state
// for examination by a debugger.
//
//*****************************************************************************
void
IntDefaultHandler(void)
{
    //
    // Disable all interrupts.
    //
    IntMasterDisable();

    //
    // Turn off all the PWM outputs.
    //
    PWMOutputOff();

    //
    // Turn on the fault LED.
    //
    UIFaultLEDBlink(1, 1);

    //
    // Go into an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
//! Handles setup of the application on the AC induction motor drive.
//!
//! This is the main application entry point for the AC induction motor drive.
//! It is responsible for basic system configuration, initialization of the
//! various application drivers and peripherals, and the main application loop.
//!
//! \return Never returns.
//
//*****************************************************************************
int
main(void)
{
    //
    // Configure the processor to run at 50 MHz.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_6MHZ);

    //
    // Enable the peripherals used by the application.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    //
    // Enable the peripherals that should continue to run when the processor
    // is sleeping.
    //
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_QEI0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER1);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);

    //
    // Enable peripheral clock gating.  Note that this is required in order to
    // measure the the processor usage.
    //
    SysCtlPeripheralClockGating(true);

    //
    // Set the priorities of the interrupts used by the application.
    //
    IntPrioritySet(INT_GPIOB, 0x00);
    IntPrioritySet(INT_GPIOD, 0x00);
    IntPrioritySet(INT_PWM0_0, 0x20);
    IntPrioritySet(INT_PWM0_1, 0x40);
    IntPrioritySet(INT_PWM0_2, 0x60);
    IntPrioritySet(INT_ADC0SS0, 0x80);
    IntPrioritySet(INT_UART0, 0xa0);
    IntPrioritySet(FAULT_SYSTICK, 0xc0);

    //
    // Enable the weak pull-downs instead of the weak pull-ups.
    //
    GPIOPadConfigSet(GPIO_PORTA_BASE,
                     (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                      GPIO_PIN_4 | GPIO_PIN_5), GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPD);
    GPIOPadConfigSet(GPIO_PORTB_BASE,
                     (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                      GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7),
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPadConfigSet(GPIO_PORTC_BASE,
                     (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                      GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7),
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPadConfigSet(GPIO_PORTD_BASE,
                     (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                      GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7),
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPadConfigSet(GPIO_PORTE_BASE,
                     GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

    //
    // Initialize the dynamic brake control.
    //
    BrakeInit();

    //
    // Initialize the PWM driver.
    //
    PWMInit();

    //
    // Initialize the ADC.
    //
    ADCInit();

    //
    // Initialize the speed sense.
    //
    SpeedSenseInit();

    //
    // Initialize the flash parameter block driver.
    //
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, sizeof(tDriveParameters));

    //
    // Initialize the user interface.
    //
    UIInit();

    //
    // Delay while the in-rush current limiter operates.
    //
    InRushDelay();

    //
    // Simulate a hard fault if the parameter block size is not 128 bytes.
    //
    if(sizeof(tDriveParameters) != 128)
    {
        FaultISR();
    }

    //
    // Clear any fault conditions that may have erroneously triggered as the
    // ADC started acquiring readings (and were therefore based on unreliable
    // readings).
    //
    MainClearFaults();

    //
    // Indicate that the motor drive is stopped.
    //
    UIRunLEDBlink(200, 25);

    //
    // Loop forever.  All the real work is done in interrupt handlers.
    //
    while(1)
    {
        //
        // Put the processor to sleep.
        //
        SysCtlSleep();
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
