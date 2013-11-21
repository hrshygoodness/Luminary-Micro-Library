//*****************************************************************************
//
// main.c - Brushless DC motor drive main application.
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
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/watchdog.h"
#include "adc_ctrl.h"
#include "brake.h"
#include "faults.h"
#include "hall_ctrl.h"
#include "main.h"
#include "pwm_ctrl.h"
#include "speed_sense.h"
#include "sinemod.h"
#include "trapmod.h"
#include "ui.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Basic Brushless DC Motor Drive Application (basic-bldc)</h1>
//!
//! This application is a motor drive for Brushless DC (BLDC) motors.  The
//! following features are supported:
//!
//! - Trapezoid, Sensorless, or Sinusoid modulation
//! - Closed loop speed control
//! - DC bus voltage monitoring and control
//! - Regenerative braking control
//! - Simple on-board user interface (via a push button)
//! - Over 25 configurable drive parameters
//
//*****************************************************************************

//*****************************************************************************
//
// \page main_intro Introduction
//
// This is the main Brushless DC motor application code.  It contains a state
// machine that controls the operation of the drive, an interrupt handler for
// the waveform update software interrupt, an interrupt handler for the
// millisecond speed update software interrupt, and the main application
// startup code.
//
// The waveform update interrupt handler is responsible for computing new
// values for the waveforms being driven to the inverter bridge.  Based on the
// update rate, it will advance the drive angle and recompute new waveforms.
// The new waveform values are passed to the PWM module to be supplied to
// the PWM hardware at the correct time.
//
// The millisecond speed update interrupt handler is responsible for
// handling the dynamic brake, computing the new drive speed, and checking
// for fault conditions.  If the drive is just starting, this is where the
// precharging of the high-side gate drivers is handled.  If the drive has
// just stopped, this is where the DC injection braking is handled.  Dynamic
// braking is handled by simply calling the update function for the dynamic
// braking module.
//
// When running, a variety of things are done to adjust the drive speed.
// First, the acceleration or deceleration rate is applied as appropriate to
// move the drive speed towards the target speed.  Also, the amplitude of
// the PWM outputs is adjusted by a PI controller, moving the rotor speed to
// the desired speed.  In the case of deceleration, the deceleration rate
// may be reduced based on the DC bus voltage.  The result of this speed
// adjustment is a new step angle, which is subsequently used by the waveform
// update interrupt handler to generate the output waveforms.
//
// The over-temperature, DC bus under-voltage, DC bus over-voltage, motor
// under-current, and motor over-current faults are all checked for by
// examining the readings from the ADC.  Fault conditions are handled by
// turning off the drive output and indicating the appropriate fault, which
// must be cleared before the drive will run again.
//
// The state machine that controls the operation of the drive is woven
// throughout the millisecond speed update interrupt handler and the
// routines that start, stop, and adjust the parameters of the motor drive.
// Together, it ensures that the motor drive responds to commands and
// parameter changes in a logical and predictable manner.
//
// The application startup code performs high-level initialization of the
// microcontroller (such as enabling peripherals) and calls the initialization
// routines for the various support modules.  Since all the work within the
// motor drive occurs with interrupt handlers, its final task is to go into
// an infinite loop that puts the processor into sleep mode.  This serves two
// purposes; it allows the processor to wait until there is work to be done
// (for example, an interrupt) before it executes any further code, and it
// allows the processor usage meter to gather the data it needs to determine
// processor usage.
//
// The main application code is contained in <tt>main.c</tt>, with
// <tt>main.h</tt> containing the definitions for the defines, variables, and
// functions exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
// \defgroup main_api Definitions
// @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is the motor status when the motor drive is stopped.
//
//*****************************************************************************
#define MOTOR_STATUS_STOP       0x00

//*****************************************************************************
//
// This is the motor status when the motor drive is running at a fixed speed.
//
//*****************************************************************************
#define MOTOR_STATUS_RUN        0x01

//*****************************************************************************
//
// This is the motor status when the motor drive is accelerating.
//
//*****************************************************************************
#define MOTOR_STATUS_ACCEL      0x02

//*****************************************************************************
//
// This is the motor status when the motor drive is decelerating.
//
//*****************************************************************************
#define MOTOR_STATUS_DECEL      0x03

//*****************************************************************************
//
// A state flag that indicates that the motor drive is in the forward
// direction.
//
//*****************************************************************************
#define STATE_FLAG_FORWARD      0x01

//*****************************************************************************
//
// A state flag that indicates that the motor drive is in the backward
// direction.
//
//*****************************************************************************
#define STATE_FLAG_BACKWARD     0x00

//*****************************************************************************
//
// A state flag that indicates that the motor drive is running.
//
//*****************************************************************************
#define STATE_FLAG_RUN          0x02

//*****************************************************************************
//
// A state flag that indicates that the motor drive is stopping.
//
//*****************************************************************************
#define STATE_FLAG_STOPPING     0x04

//*****************************************************************************
//
// A state flag that indicates that the motor drive is reversing direction.
//
//*****************************************************************************
#define STATE_FLAG_REV          0x08

//*****************************************************************************
//
// A state flag that indicates that the motor drive is precharging the
// bootstrap capacitors on the high side gate drivers.
//
//*****************************************************************************
#define STATE_FLAG_PRECHARGE    0x10

//*****************************************************************************
//
// A state flag that indicates that the motor drive is in the startup
// condition, getting the motor spinning for sensorless opertation.
//
//*****************************************************************************
#define STATE_FLAG_STARTUP      0x20

//*****************************************************************************
//
// The motor drive is stopped.  A run request will cause a transition to the
// #STATE_PRECHARGE or #STATE_BACK_PRECHARGE states, depending upon the
// direction flag.
//
//*****************************************************************************
#define STATE_STOPPED           0x00

//*****************************************************************************
//
// The motor drive is precharging the bootstrap capacitors on the high side
// gate drivers.  Once the capacitors are charged, the state machine will
// automatically transition to #STATE_RUN.
//
//*****************************************************************************
#define STATE_PRECHARGE         (STATE_FLAG_PRECHARGE | STATE_FLAG_FORWARD)

//*****************************************************************************
//
// The motor drive is starting.  The motor will be spun in the forward
// direction until a minimum speed is reached.  At that point, the motor
// state will be transitioned to #STATE_RUN.
//
//*****************************************************************************
#define STATE_STARTUP           (STATE_FLAG_STARTUP | STATE_FLAG_FORWARD)

//*****************************************************************************
//
// The motor drive is running, either at the target speed or slewing to
// the target speed.
//
//*****************************************************************************
#define STATE_RUN               (STATE_FLAG_RUN | STATE_FLAG_FORWARD)

//*****************************************************************************
//
// The motor drive is decelerating down to a stop, at which point the state
// machine will automatically transition to #STATE_BACK_RUN.  This results in
// a direction change of the motor drive.
//
//*****************************************************************************
#define STATE_REV               (STATE_FLAG_RUN | STATE_FLAG_REV | \
                                 STATE_FLAG_FORWARD)

//*****************************************************************************
//
// The motor drive is decelerating down to a stop, at which point the state
// machine will automatically transition to #STATE_STOPPED.  This results in
// the motor drive being stopped.
//
//*****************************************************************************
#define STATE_STOPPING          (STATE_FLAG_RUN | STATE_FLAG_STOPPING | \
                                 STATE_FLAG_FORWARD)

//*****************************************************************************
//
// The motor drive is precharging the bootstrap capacitors on the high side
// gate drivers while running in the backward direction.  Once the capacitors
// are charged, the state machine will automatically transition to
// #STATE_BACK_RUN.
//
//*****************************************************************************
#define STATE_BACK_PRECHARGE    (STATE_FLAG_PRECHARGE | STATE_FLAG_BACKWARD)

//*****************************************************************************
//
// The motor drive is starting.  The motor will be spun in the backward
// direction until a minimum speed is reached.  At that point, the motor
// state will be transitioned to #STATE_BACK_RUN.
//
//*****************************************************************************
#define STATE_BACK_STARTUP      (STATE_FLAG_STARTUP | STATE_FLAG_BACKWARD)

//*****************************************************************************
//
// The motor drive is running in the backward direction, either at the target
// speed or slewing to the target speed.
//
//*****************************************************************************
#define STATE_BACK_RUN          (STATE_FLAG_RUN | STATE_FLAG_BACKWARD)

//*****************************************************************************
//
// The motor drive is decelerating down to a stop while running in the
// backward direction, at which point the state machine will automatically
// transition to #STATE_RUN.  This results in a direction change of the motor
// drive.
//
//*****************************************************************************
#define STATE_BACK_REV          (STATE_FLAG_RUN | STATE_FLAG_REV | \
                                 STATE_FLAG_BACKWARD)

//*****************************************************************************
//
// The motor drive is decelerating down to a stop while running in the
// backward direction, at which point the state machine will automatically
// transition to #STATE_STOPPED.  This results in the motor drive being
// stopped.
//
//*****************************************************************************
#define STATE_BACK_STOPPING     (STATE_FLAG_RUN | STATE_FLAG_STOPPING | \
                                 STATE_FLAG_BACKWARD)

//*****************************************************************************
//
// The latched fault status flags for the motor drive, enumerated by
// #FAULT_EMERGENCY_STOP, #FAULT_VBUS_LOW, #FAULT_VBUS_HIGH,
// #FAULT_CURRENT_LOW, #FAULT_CURRENT_HIGH, and #FAULT_TEMPERATURE_HIGH.
//
//*****************************************************************************
unsigned long g_ulFaultFlags;

//*****************************************************************************
//
// The current operation state of the motor drive.
//
//*****************************************************************************
unsigned char g_ucMotorStatus = MOTOR_STATUS_STOP;

//*****************************************************************************
//
// The current motor drive speed in RPM, expressed as a 18.14 fixed-point
// value.
//
//*****************************************************************************
static unsigned long g_ulSpeed = 0;

//*****************************************************************************
//
// The whole part of the current motor drive speed.  This is used in
// conjunction with #g_ulSpeedFract to compute the value for
// #g_ulSpeed.
//
//*****************************************************************************
static unsigned long g_ulSpeedWhole = 0;

//*****************************************************************************
//
// The fractional part of the current motor drive speed.  This value is
// expressed as the numerator of a fraction whose denominator is the PWM
// frequency.  This is used in conjunction with #g_ulSpeedWhole to compute
// the value for #g_ulSpeed.
//
//*****************************************************************************
static unsigned long g_ulSpeedFract = 0;

//*****************************************************************************
//
// The current motor drive power in watts, expressed as a 18.14 fixed-point
// value.
//
//*****************************************************************************
static unsigned long g_ulPower = 0;

//*****************************************************************************
//
// The whole part of the current motor drive power.  This is used in
// conjunction with #g_ulPowerFract to compute the value for
// #g_ulPower.
//
//*****************************************************************************
static unsigned long g_ulPowerWhole = 0;

//*****************************************************************************
//
// The fractional part of the current motor drive power.  This value is
// expressed as the numerator of a fraction whose denominator is the PWM
// frequency.  This is used in conjunction with #g_ulPowerWhole to compute
// the value for #g_ulPower.
//
//*****************************************************************************
static unsigned long g_ulPowerFract = 0;

//*****************************************************************************
//
// The current duty cycle for the motor drive, expressed as a 16.16
// fixed-point value in the range from 0.0 to 1.0.
//
//*****************************************************************************
static unsigned long g_ulDutyCycle = 0;

//*****************************************************************************
//
// The current angle of the motor drive output, expressed as a 0.32
// fixed-point value that is the percentage of the way around a circle.
//
//*****************************************************************************
unsigned long g_ulAngle = 0;

//*****************************************************************************
//
// The amount by which the motor drive angle is updated for a single PWM
// period, expressed as a 0.32 fixed-point value.  For example, if the motor
// drive is being updated every fifth PWM period, this value should be
// multiplied by five to determine the amount to adjust the angle.
//
//*****************************************************************************
static unsigned long g_ulAngleDelta = 0;

//*****************************************************************************
//
// A count of the number of milliseconds to remain in a particular state.
//
//*****************************************************************************
static unsigned long g_ulStateCount = 0;

//*****************************************************************************
//
// The current rate of acceleration.  This will start as the parameter value,
// but may be reduced in order to manage increases in the motor current.
//
//*****************************************************************************
static unsigned long g_ulAccelRate;

//*****************************************************************************
//
// The current rate of deceleration.  This will start as the parameter value,
// but may be reduced in order to manage increases in the DC bus voltage.
//
//*****************************************************************************
static unsigned long g_ulDecelRate;

//*****************************************************************************
//
// The accumulator for the integral term of the PI controller for the motor
// drive duty cycle.
//
//*****************************************************************************
static long g_lSpeedIntegrator;

//*****************************************************************************
//
// The maximum value that of the PI controller accumulator
// (#g_lSpeedIntegrator).  This limit is based on the I coefficient and
// the maximum duty cycle of the motor drive, and is used to avoid
// "integrator windup", a potential pitfall of PI controllers.
//
//*****************************************************************************
static long g_lSpeedIntegratorMax;

//*****************************************************************************
//
// The accumulator for the integral term of the PI controller for the motor
// drive duty cycle.
//
//*****************************************************************************
static long g_lPowerIntegrator;

//*****************************************************************************
//
// The maximum value that of the PI controller accumulator
// (#g_lPowerIntegrator).  This limit is based on the I coefficient and
// the maximum duty cycle of the motor drive, and is used to avoid
// "integrator windup", a potential pitfall of PI controllers.
//
//*****************************************************************************
static long g_lPowerIntegratorMax;

//*****************************************************************************
//
// The current state of the motor drive state machine.  This state machine
// controls acceleration, deceleration, starting, stopping, braking, and
// reversing direction of the motor drive.
//
//*****************************************************************************
static unsigned long g_ulState = STATE_STOPPED;

//*****************************************************************************
//
// The current speed of the motor.  This value is updated based on whether
// the encoder or Hall sensors are being used.
//
//*****************************************************************************
unsigned long g_ulMeasuredSpeed = 0;

//*****************************************************************************
//
// The previous value of the "Hall" state for trapezoid modulation.
//
//*****************************************************************************
static unsigned long g_ulHallPrevious = 8;

//*****************************************************************************
//
// The Stall Detection Count.
//
//*****************************************************************************
static unsigned long g_ulStallDetectCount = 0;

//*****************************************************************************
//
// The Sine modulation target speed.
//
//*****************************************************************************
static unsigned long g_ulSineTarget = 0;

//*****************************************************************************
//
// The current state for the Startup state machine.
//
//*****************************************************************************
static unsigned long g_ulStartupState;

//*****************************************************************************
//
// The current index for the Startup hall commutation sequence.
//
//*****************************************************************************
static unsigned char g_ucStartupHallIndex;

//*****************************************************************************
//
// The period, in ticks, for startup commutation timer.
//
//*****************************************************************************
static unsigned long g_ulStartupPeriod;

//*****************************************************************************
//
// The current Duty Cycle for startup mode.
//
//*****************************************************************************
static unsigned long g_ulStartupDutyCycle;

//*****************************************************************************
//
// The Startup Duty Cycle acceleration ramp value.
//
//*****************************************************************************
static unsigned long g_ulStartupDutyCycleRamp;

//*****************************************************************************
//
// The direction that the motor is to be spinning.
//
//*****************************************************************************
static tBoolean g_bForward = true;

//*****************************************************************************
//
// The runtime target speed.
//
//*****************************************************************************
static unsigned long g_ulTargetSpeed = 0;

//*****************************************************************************
//
// The runtime target Power.
//
//*****************************************************************************
static unsigned long g_ulTargetPower = 0;

//*****************************************************************************
//
// A local variable used to save the state of the PWM decay mode during
// sensorless startup.
//
//*****************************************************************************
static unsigned char g_ucLocalDecayMode = 1;

//*****************************************************************************
//
// Handles errors from the driver library.
//
// This function is called when an error is encountered by the driver library.
// Typically, the errors will be invalid parameters passed to the library's
// APIs.
//
// In this application, nothing is done in this function.  It does provide a
// convenient location for a breakpoint that will catch all driver library
// errors.
//
// \return None.
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
// Multiplies two 16.16 fixed-point numbers.
//
// \param lX is the first multiplicand.
// \param lY is the second multiplicand.
//
// This function takes two fixed-point numbers, in 16.16 format, and
// multiplies them together, returning the 16.16 fixed-point result.  It is
// the responsibility of the caller to ensure that the dynamic range of the
// integer portion of the value is not exceeded; if it is exceeded the result
// will not be correct.
//
// \return Returns the result of the multiplication, in 16.16 fixed-point
// format.
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
// Handles the Back EMF Timer Interrupt.
//
// This function is called when the Back EMF timer expires.  This code will
// set the Back EMF Hall state value to the next value, as determined by
// the Back EMF processing code.  If the motor is running in the startup
// state, then open-loop commutation of the motor is performed by indexing
// through a predefined hall sequence.  The timer is then restarted based
// on the period calculated in the startup state machine.  If the motor is
// running in the normal run state, then the motor is commutated based on
// the values calculated in the Back EMF processing code.  The timer will
// be restarted when the next zero-crossing event has been detected.
//
// \return None.
//
//*****************************************************************************
void
Timer0AIntHandler(void)
{
    //
    // Clear the Timer interrupt.
    //
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    if(UI_PARAM_MODULATION == MODULATION_SENSORLESS)
    {
        //
        // If running in startup mode, we are operating open loop, so just
        // commute the motor and restart the timer.
        //
        if(g_ulState & STATE_FLAG_STARTUP)
        {
            static unsigned char ucHallSequence[6] = {5, 1, 3, 2, 6, 4};

            //
            // Commute the motor.
            //
            TrapModulate(ucHallSequence[g_ucStartupHallIndex]);

            //
            // Increment/Decrement the startup hall index for next time.
            //
            if((g_ulState & STATE_FLAG_FORWARD) == STATE_FLAG_BACKWARD)
            {
                g_ucStartupHallIndex--;
                if(g_ucStartupHallIndex > 5)
                {
                    g_ucStartupHallIndex = 5;
                }
            }
            else
            {
                g_ucStartupHallIndex++;
                if(g_ucStartupHallIndex >5)
                {
                    g_ucStartupHallIndex = 0;
                }
            }

            //
            // Restart the timer for the next commutation.
            //
            HWREG(TIMER0_BASE + TIMER_O_TAILR) = g_ulStartupPeriod;
            HWREG(TIMER0_BASE + TIMER_O_CTL) |=
                (TIMER_A & (TIMER_CTL_TAEN | TIMER_CTL_TBEN));
        }
        else if(g_ulState & STATE_FLAG_RUN)
        {
            //
            // Set the new Hall Sensor value.
            //
            g_ulBEMFHallValue = g_ulBEMFNextHall;

            //
            // If Motor is running and NOT startup mode, commute the motor.
            //
            TrapModulate(g_ulBEMFHallValue);
        }
    }
}

//*****************************************************************************
//
// Changes the PWM frequency of the motor drive.
//
// This function changes the period of the PWM signals produced by the motor
// drive.  It is simply a wrapper function around the PWMSetFrequency()
// function; the PWM frequency-based timing parameters of the motor drive are
// adjusted as part of the PWM frequency update.
//
// \return None.
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
    // Compute the new angle delta based on the new PWM frequency and the
    // number of poles in the motor.
    //
    g_ulAngleDelta = (((g_ulSpeed / 60) << 9) / g_ulPWMFrequency) << 9;
    g_ulAngleDelta *= (UI_PARAM_NUM_POLES / 2);

    //
    // Re-enable the update interrupts.
    //
    IntEnable(INT_PWM0_1);
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
// Changes the target speed of the motor drive.
//
// This function changes the target speed of the motor drive.  If
// required, the state machine will be transitioned to a new state in order to
// move the motor drive to the target speed.
//
// \return None.
//
//*****************************************************************************
void
MainSetSpeed(void)
{
    //
    // First, set the speed.
    //
    g_ulTargetSpeed = UI_PARAM_TARGET_SPEED;

    //
    // Clip the target speed to the minimum speed if it is too small.
    //
    if(UI_PARAM_TARGET_SPEED < UI_PARAM_MIN_SPEED)
    {
        g_ulTargetSpeed = UI_PARAM_MIN_SPEED;
    }

    //
    // Clip the target speed to the sensorless end speed if sensorless mode
    // is enabled.
    //
    if(UI_PARAM_MODULATION == MODULATION_SENSORLESS)
    {
        if(UI_PARAM_TARGET_SPEED < UI_PARAM_STARTUP_ENDSP)
        {
            g_ulTargetSpeed = UI_PARAM_STARTUP_ENDSP;
        }
    }

    //
    // Clip the target speed to the maximum speed if it is too large.
    //
    if(UI_PARAM_TARGET_SPEED > UI_PARAM_MAX_SPEED)
    {
        g_ulTargetSpeed = UI_PARAM_MAX_SPEED;
    }
}

//*****************************************************************************
//
// Changes the target power of the motor drive.
//
// This function changes the target power of the motor drive.  If
// required, the state machine will be transitioned to a new state in order to
// move the motor drive to the target power.
//
// \note This function is not yet implemented/used.
//
// \return None.
//
//*****************************************************************************
void
MainSetPower(void)
{
    //
    // First, set the power.
    //
    g_ulTargetPower = UI_PARAM_TARGET_POWER;

    //
    // Clip the target speed to the minimum speed if it is too small.
    //
    if(UI_PARAM_TARGET_POWER < UI_PARAM_MIN_POWER)
    {
        g_ulTargetPower = UI_PARAM_MIN_POWER;
    }

    //
    // Clip the target speed to the maximum speed if it is too large.
    //
    if(UI_PARAM_TARGET_POWER > UI_PARAM_MAX_POWER)
    {
        g_ulTargetPower = UI_PARAM_MAX_POWER;
    }
}

//*****************************************************************************
//
// Sets the direction of the motor drive.
//
// \param bForward is a boolean that is true if the motor drive should be
// run in the forward direction.
//
// This function changes the direction of the motor drive.  If required, the
// state machine will be transitioned to a new state in order to change the
// direction of the motor drive.
//
// \return None.
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
    // Set the run-time direction flag.
    //
    g_bForward = bForward;

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
                g_ulDecelRate = UI_PARAM_DECEL << 16;
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
                g_ulDecelRate = UI_PARAM_DECEL << 16;
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
// Updates the I coefficient of the speed PI controller.
//
// \param lNewFAdjI is the new value of the I coefficient.
//
// This function updates the value of the I coefficient of the duty cycle PI
// controller.  In addition to updating the I coefficient, it recomputes the
// maximum value of the integrator and the current value of the integrator in
// terms of the new I coefficient (eliminating any instantaneous jump in the
// output of the PI controller).
//
// \return None.
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
        g_lSpeedIntegratorMax = 0;
        g_lSpeedIntegrator = 0;
    }
    else
    {
        //
        // Compute the maximum value of the integrator.  This is the value that
        // results in the maximum output duty cycle (i.e. integrator max * I =
        // max speed).
        //
        g_lSpeedIntegratorMax = 65536 * 100;

        //
        // Adjust the current value of the integrator to account for the new
        // value of the I coefficient.  This satisfies the equation "old
        // integrator * old I = new integrator * new I", so that the output
        // doesn't immediately change (in a drastic way) as a result of the
        // change in the I coefficient.
        //
        g_lSpeedIntegrator = ((g_lSpeedIntegrator / lNewFAdjI) *
                                  UI_PARAM_SPEED_I);
    }

    //
    // Re-enable the millisecond interrupt.
    //
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
// Updates the I coefficient of the power PI controller.
//
// \param lNewPAdjI is the new value of the I coefficient.
//
// This function updates the value of the I coefficient of the duty cycle PI
// controller.  In addition to updating the I coefficient, it recomputes the
// maximum value of the integrator and the current value of the integrator in
// terms of the new I coefficient (eliminating any instantaneous jump in the
// output of the PI controller).
//
// \return None.
//
//*****************************************************************************
void
MainUpdatePAdjI(long lNewPAdjI)
{
    //
    // Temporarily disable the millisecond interrupt.
    //
    IntDisable(INT_PWM0_2);

    //
    // See if the new I coefficient is zero.
    //
    if(lNewPAdjI == 0)
    {
        //
        // Since the I coefficient is zero, the integrator and integrator
        // maximum are also zero.
        //
        g_lPowerIntegratorMax = 0;
        g_lPowerIntegrator = 0;
    }
    else
    {
        //
        // Compute the maximum value of the integrator.  This is the value that
        // results in the maximum output duty cycle (i.e. integrator max * I =
        // max speed).
        //
        g_lPowerIntegratorMax = 65536 * 100;

        //
        // Adjust the current value of the integrator to account for the new
        // value of the I coefficient.  This satisfies the equation "old
        // integrator * old I = new integrator * new I", so that the output
        // doesn't immediately change (in a drastic way) as a result of the
        // change in the I coefficient.
        //
        g_lPowerIntegrator = ((g_lPowerIntegrator / lNewPAdjI) *
                                  UI_PARAM_POWER_I);
    }

    //
    // Re-enable the millisecond interrupt.
    //
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
// Handles the waveform update software interrupt.
//
// This function is periodically called as a result of the waveform update
// software interrupt being asserted.  This interrupt is asserted at the
// requested rate (based on the update rate parameter) by the PWM interrupt
// handler.
//
// The angle of the motor drive will be updated, and new waveform values
// computed and supplied to the PWM module.
//
// \note Since this interrupt is software triggered, there is no interrupt
// source to clear in this handler.
//
// \return None.
//
//*****************************************************************************
void
MainWaveformTick(void)
{
    unsigned long ulTemp, pulDutyCycles[3];

    //
    // There is nothing to be done if the motor drive is not running.
    //
    if((g_ulState == STATE_STOPPED) || (g_ulState == STATE_PRECHARGE) ||
       (g_ulState == STATE_BACK_PRECHARGE))
    {
        //
        // Reduce the PWM period count based on the number of updates that
        // would have occurred if the motor drive was running.
        //
        PWMReducePeriodCount((PWMGetPeriodCount() /
                              (UI_PARAM_PWM_UPDATE + 1)) *
                             (UI_PARAM_PWM_UPDATE + 1));

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
    while(PWMGetPeriodCount() > UI_PARAM_PWM_UPDATE)
    {
        //
        // Get the number of updates that are pending.  Normally, this will be
        // one, but may be larger if the processor is heavily loaded.
        //
        ulTemp = PWMGetPeriodCount() / (UI_PARAM_PWM_UPDATE + 1);

        //
        // See if the motor drive direction is forward or backward.
        //
        if(g_ulState & STATE_FLAG_FORWARD)
        {
            //
            // The motor drive direction is forward, so increment the motor
            // drive angle by the angle delta.
            //
            g_ulAngle += (g_ulAngleDelta * (UI_PARAM_PWM_UPDATE + 1) *
                          ulTemp);
        }
        else
        {
            //
            // The motor drive direction is backward, so decrement the motor
            // drive angle by the angle delta.
            //
            g_ulAngle -= (g_ulAngleDelta * (UI_PARAM_PWM_UPDATE + 1) *
                          ulTemp);
        }

        //
        // Reduce the PWM period count by the number of updates just performed.
        //
        PWMReducePeriodCount(ulTemp * (UI_PARAM_PWM_UPDATE + 1));

        //
        // Perform sine wave modulation.
        //
        if(UI_PARAM_MODULATION == MODULATION_SINE)
        {
            static const unsigned long ulHallToAngle[] =
                { 0, 270, 30, 330, 150, 210, 90, 0 };

            //
            // Check for change in Hall State.
            //
            if((g_ulHallPrevious != g_ulHallValue) &&
               ((g_ulSineTarget != g_ulSpeed) || (g_ulHallValue == 5)))
            {
                g_ulAngle = ulHallToAngle[g_ulHallValue];
                if((g_ulState & STATE_FLAG_FORWARD) == STATE_FLAG_BACKWARD)
                {
                    g_ulAngle += 60;
                    g_ulAngle %= 360;
                }
                g_ulAngle = (((g_ulAngle << 16) / 360) << 16);
            }
            g_ulHallPrevious = g_ulHallValue;

            //
            // Run the sine modulation code.
            //
            SineModulate(g_ulAngle, g_ulDutyCycle, pulDutyCycles);

            //
            // If running in reverse, the duty cycles must be inverted
            //
            if((g_ulState & STATE_FLAG_FORWARD) == STATE_FLAG_BACKWARD)
            {
                pulDutyCycles[0] = 65536 - pulDutyCycles[0];
                pulDutyCycles[1] = 65536 - pulDutyCycles[1];
                pulDutyCycles[2] = 65536 - pulDutyCycles[2];
            }
        }

        //
        // For now, there are no other modulations enabled, so ensure that
        // the Duty Cycle is set to minimal.
        //
        else
        {
            pulDutyCycles[0] = pulDutyCycles[1] = pulDutyCycles[2] = 0;
        }

        //
        // Set the new duty cycle.
        //
        PWMSetDutyCycle(pulDutyCycles[0], pulDutyCycles[1], pulDutyCycles[2]);
    }
}

//*****************************************************************************
//
// Handles the gate driver precharge mode of the motor drive.
//
// This function performs the processing and state transitions associated with
// the gate driver precharge mode of the motor drive.
//
// \return None.
//
//*****************************************************************************
static void
MainPrechargeHandler(void)
{
    //
    // Punch the watchdog to prevent timeout from killing our startup
    // sequence.
    //
    WatchdogReloadSet(WATCHDOG0_BASE, WATCHDOG_RELOAD_VALUE);

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

    if(UI_PARAM_MODULATION == MODULATION_SENSORLESS)
    {
        //
        // Set the startup state to precharge mode.
        //
        g_ulStartupState = 0;

        //
        // Set the forward/reverse state accordingly.
        //
        if(g_ulState == STATE_PRECHARGE)
        {
            g_ulState = STATE_STARTUP;
        }
        else
        {
            g_ulState = STATE_BACK_STARTUP;
        }

        //
        // And return.
        //
        return;
    }

    //
    // Set the miniumum duty cycle.
    //
    g_ulDutyCycle = 0;
    PWMSetDutyCycle(g_ulDutyCycle, g_ulDutyCycle, g_ulDutyCycle);

    //
    // If trapezoid drive, kick start the motor by running a Hall interrupt.
    // Otherwise, enable all the PWM outputs for other drive modes.
    //
    if(UI_PARAM_MODULATION == MODULATION_TRAPEZOID)
    {
        GPIOBIntHandler();
    }
    else
    {
        PWMOutputOn();
    }

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
    g_ulSpeed = g_ulPower = 0;

    //
    // Reset the integrator.
    //
    g_lSpeedIntegrator = g_lPowerIntegrator = 0;

    //
    // Start the motor drive at an angle of zero degrees.
    //
    g_ulAngle = 0;
}

//*****************************************************************************
//
// Handles the startup mode of the motor drive.
//
// This function performs the processing and state transitions associated with
// the startup mode of the motor drive for sensorless operation.
//
// \return None.
//
//*****************************************************************************
static void
MainStartupHandler(void)
{
    unsigned long ulTemp;

    //
    // Punch the watchdog to prevent timeout from killing our startup
    // sequence.
    //
    WatchdogReloadSet(WATCHDOG0_BASE, WATCHDOG_RELOAD_VALUE);

    //
    // Startup State Machine for Sensorless Control.
    //
    switch(g_ulStartupState)
    {
        //
        // Start driving the motor to a known position by driving the motor
        // phases B+, A-.  This corresponds to a hall sensor detction of
        // Hall A = 1, Hall B = 0, Hall C = 1.  The effective phase voltage
        // will start at 0V, and ramp to the sensorless startup voltage over
        // the course of the startup count (milliseconds).
        //
        case 0:
        {
            g_ucLocalDecayMode = UI_PARAM_DECAY_MODE;
            g_ucDecayMode = DECAY_SLOW;
            g_ulStartupDutyCycle = 0;
            ulTemp =
                (((UI_PARAM_STARTUP_STARTV << 16) /
                g_ulBusVoltage) * 100);
            g_ulStartupDutyCycleRamp = ((ulTemp - g_ulStartupDutyCycle) /
                UI_PARAM_STARTUP_COUNT);
            g_ucStartupHallIndex = 0;
            g_ulDutyCycle = (g_ulStartupDutyCycle / 100);
            PWMSetDutyCycle(g_ulDutyCycle, g_ulDutyCycle, g_ulDutyCycle);
            TrapModulate(5);
            g_ulStateCount = UI_PARAM_STARTUP_COUNT;
            g_ulStartupState++;
            break;
        }

        //
        // Increaase the effective phase voltage voltage till we have reached
        // the startup voltage (based on ramp time).
        //
        case 1:
        {
            g_ulStartupDutyCycle += g_ulStartupDutyCycleRamp;
            g_ulDutyCycle = (g_ulStartupDutyCycle / 100);
            PWMSetDutyCycle(g_ulDutyCycle, g_ulDutyCycle, g_ulDutyCycle);
            g_ulStateCount--;
            if(g_ulStateCount == 0)
            {
                g_ulStartupState++;
                g_ulStateCount = UI_PARAM_STARTUP_COUNT;
            }
            break;
        }

        //
        // Allow the motor to stabilize in this position for about half the
        // startup count.
        //
        case 2:
        {
            g_ulStateCount--;
            if(g_ulStateCount == 0)
            {
                g_ulStartupState++;
            }
            break;
        }

        //
        // Setup the initial parameters for the startup speed/period for
        // the timer.
        //
        case 3:
        {
            g_ulSpeedWhole = UI_PARAM_STARTUP_STARTSP;
            g_ulSpeedFract = 0;
            g_ulSpeed = ((g_ulSpeedWhole << 14) +
                ((g_ulSpeedFract << 14) / 1000));
            g_ulAccelRate =
                (((UI_PARAM_STARTUP_ENDSP -
                UI_PARAM_STARTUP_STARTSP) * 1000) /
                (unsigned long)UI_PARAM_STARTUP_RAMP);
            g_ulStartupDutyCycle =
                (((UI_PARAM_STARTUP_STARTV << 16) /
                g_ulBusVoltage) * 100);
            ulTemp =
                (((UI_PARAM_STARTUP_ENDV << 16) /
                g_ulBusVoltage) * 100);
            g_ulStartupDutyCycleRamp = ((ulTemp - g_ulStartupDutyCycle) /
                (unsigned long)UI_PARAM_STARTUP_RAMP);
            ulTemp = (g_ulSpeed >> 14);
            ulTemp = (ulTemp * 3 * UI_PARAM_NUM_POLES);
            ulTemp = (ulTemp / 60);
            g_ulStartupPeriod = (SYSTEM_CLOCK / ulTemp);
            g_ulStartupState++;
            Timer0AIntHandler();
            break;
        }

        //
        // Ramp up the speed/voltage till the speed has exceeded the
        // startup ending speed.  At this point, transition to a holding
        // state to allow power to stabilize before switching to closed
        // loop mode.
        //
        case 4:
        {
            if(g_ulSpeed > (UI_PARAM_STARTUP_ENDSP << 14))
            {
                g_ulStartupState++;
                g_ulStateCount = 250;
            }
            else
            {
                g_ulSpeedFract += g_ulAccelRate;
                while(g_ulSpeedFract > 1000)
                {
                    g_ulSpeedFract -= 1000;
                    g_ulSpeedWhole += 1;
                }
                g_ulSpeed = ((g_ulSpeedWhole << 14) +
                    ((g_ulSpeedFract << 14) / 1000));
                g_ulStartupDutyCycle += g_ulStartupDutyCycleRamp;
                g_ulDutyCycle = (g_ulStartupDutyCycle / 100);
                PWMSetDutyCycle(g_ulDutyCycle, g_ulDutyCycle, g_ulDutyCycle);
                ulTemp = (g_ulSpeed >> 14);
                ulTemp = (ulTemp * 3 * UI_PARAM_NUM_POLES);
                ulTemp = (ulTemp / 60);
                g_ulStartupPeriod = (SYSTEM_CLOCK / ulTemp);
            }
            break;
        }

        //
        // Wait in this state for the drive current to stabilize prior to
        // transitioning into closed-loop mode.
        //
        case 5:
        {
            g_ulStateCount--;
            if(g_ulStateCount == 0)
            {
                g_ulStartupState++;
                g_ulStateCount = 250;
                g_lSpeedIntegrator =
                    (g_ulDutyCycle * 65536) / UI_PARAM_SPEED_I ;
                g_lPowerIntegrator =
                    (g_ulDutyCycle * 65536) / UI_PARAM_POWER_I ;
                g_ulPower = ((g_ulMotorPower / 2) << 14);
                g_ulPowerWhole = g_ulPower >> 14;
                g_ulPowerFract = ((g_ulPower & 0x3FFF) * 1000) >> 14;
                if(UI_PARAM_CONTROL_MODE == CONTROL_MODE_SPEED)
                {
                    g_ulAccelRate = UI_PARAM_ACCEL << 16;
                    g_ulDecelRate = UI_PARAM_DECEL << 16;
                }
                else
                {
                    g_ulAccelRate = UI_PARAM_ACCEL_POWER << 16;
                    g_ulDecelRate = UI_PARAM_DECEL_POWER << 16;
                }
                if((g_ulState & STATE_FLAG_FORWARD) == STATE_FLAG_FORWARD)
                {
                    g_ulState = STATE_RUN;
                }
                else
                {
                    g_ulState = STATE_BACK_RUN;
                }
            }
            break;
        }

        default:
        {
            g_ulStateCount = 0;
            MainEmergencyStop();
            MainSetFault(FAULT_EMERGENCY_STOP);
        }
    }
}

//*****************************************************************************
//
// Checks for motor drive faults.
//
// This function checks for fault conditions that may occur during the
// operation of the motor drive.  The ambient temperature, DC bus voltage, and
// motor current are all monitored for fault conditions.
//
// \return None.
//
//*****************************************************************************
static void
MainCheckFaults(void)
{
    //
    // Check for watchdog fault.
    //
    if((g_ulFaultFlags & FAULT_WATCHDOG) == FAULT_WATCHDOG)
    {
        //
        // Emergency stop the motor drive (PWM should have already been
        // disabled).
        //
        MainEmergencyStop();
    }

    //
    // See if the ambient temperature is above the maximum value.
    //
    if(g_ucAmbientTemp > UI_PARAM_MAX_TEMPERATURE)
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
    if(g_ulBusVoltage < UI_PARAM_MIN_BUS_VOLTAGE)
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
    if(g_ulBusVoltage > UI_PARAM_MAX_BUS_VOLTAGE)
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
    // minimum speed.
    //
    if((UI_PARAM_MIN_CURRENT != 0) &&
       (g_sMotorCurrent < UI_PARAM_MIN_CURRENT) &&
       (g_ulState != STATE_STOPPED))
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
    if((UI_PARAM_MAX_CURRENT != 0) &&
       (g_sMotorCurrent > UI_PARAM_MAX_CURRENT))
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

    //
    // See if motor is stalled.
    //
    if(g_ulState & STATE_FLAG_RUN)
    {
        if(g_ulMeasuredSpeed == 0)
        {
            g_ulStallDetectCount++;
            if(g_ulStallDetectCount > 1500)
            {
                //
                // Emergency stop the motor drive.
                //
                MainEmergencyStop();

                //
                // Indicate a motor stall fault.
                //
                MainSetFault(FAULT_STALL);
            }
        }
        else
        {
            g_ulStallDetectCount = 0;
        }
    }
    else
    {
        g_ulStallDetectCount = 0;
    }
}

//*****************************************************************************
//
// Adjusts the motor drive duty cycle based on the rotor speed.
//
// This function uses a PI controller to adjust the motor drive duty cycle
// in order to get the rotor speed to match the target speed.
//
// \return Returns the new motor drive duty cycle.
//
//*****************************************************************************
unsigned long
MainSpeedController(void)
{
    long lTempP, lTempI, lError;

    //
    // Compute the error between the current drive speed and the rotor speed.
    // (-MaxSpeed < lError < MaxSpeed)
    //
    lError = (long)(g_ulSpeed >> 14) - (long)g_ulMeasuredSpeed;

    //
    // Add the error to the integrator accumulator, limiting the value of the
    // accumulator.
    //
    g_lSpeedIntegrator += lError;
    if(g_lSpeedIntegrator > g_lSpeedIntegratorMax)
    {
        g_lSpeedIntegrator = g_lSpeedIntegratorMax;
    }
    if(g_lSpeedIntegrator < 0)
    {
        g_lSpeedIntegrator = 0;
    }

    //
    // Perform the actual PI controller computation.
    //
    lTempP = MainLongMul(UI_PARAM_SPEED_P, lError);
    lTempI = MainLongMul(UI_PARAM_SPEED_I, g_lSpeedIntegrator);
    lError = lTempP + lTempI;

    //
    // Limit the output of the PI controller based on the maximum speed
    // parameter (and don't allow it to go below zero).
    //
    if(lError < 0)
    {
        lError = 0;
    }
    if(lError > 65536)
    {
        lError = 65536;
    }

    //
    // Return the output of the PI controller.
    //
    return(lError);
}

//*****************************************************************************
//
// Adjusts the motor drive duty cycle based on the rotor speed.
//
// This function uses a PI controller to adjust the motor drive duty cycle
// in order to get the rotor speed to match the target speed.
//
// \return Returns the new motor drive duty cycle.
//
//*****************************************************************************
unsigned long
MainPowerController(void)
{
    long lTempP, lTempI, lError;

    //
    // Compute the error between the current drive speed and the rotor speed.
    // (-MaxSpeed < lError < MaxSpeed)
    //
    lError = (long)(g_ulPower >> 14) - (long)g_ulMotorPower;

    //
    // Add the error to the integrator accumulator, limiting the value of the
    // accumulator.
    //
    g_lPowerIntegrator += lError;
    if(g_lPowerIntegrator > g_lPowerIntegratorMax)
    {
        g_lPowerIntegrator = g_lPowerIntegratorMax;
    }
    if(g_lPowerIntegrator < 0)
    {
        g_lPowerIntegrator = 0;
    }

    //
    // Perform the actual PI controller computation.
    //
    lTempP = MainLongMul(UI_PARAM_SPEED_P, lError);
    lTempI = MainLongMul(UI_PARAM_SPEED_I, g_lPowerIntegrator);
    lError = lTempP + lTempI;

    //
    // Limit the output of the PI controller based on the maximum speed
    // parameter (and don't allow it to go below zero).
    //
    if(lError < 0)
    {
        lError = 0;
    }
    if(lError > 65536)
    {
        lError = 65536;
    }

    //
    // Return the output of the PI controller.
    //
    return(lError);
}

//*****************************************************************************
//
// Adjusts the motor drive speed based on the target speed.
//
// \param ulTarget is the target speed of the motor drive, specified as RPM.
//
// This function adjusts the motor drive speed towards a given target
// speed.  Limitations such as acceleration and deceleration rate, along
// with precautions such as limiting the deceleration rate to control the DC
// bus voltage, are handled by this function.
//
// \return None.
//
//*****************************************************************************
static void
MainSpeedHandler(unsigned long ulTarget)
{
    unsigned long ulNewValue;

    //
    // Return without doing anything if the target speed has already been
    // reached.
    //
    if(ulTarget == g_ulSpeed)
    {
        return;
    }

    //
    // See if the target speed is greater than the current speed.
    //
    if(ulTarget > g_ulSpeed)
    {
        //
        // Compute the new maximum acceleration rate, based on the present
        // motor current.
        //
        if(g_sMotorCurrent >= (UI_PARAM_ACCEL_CURRENT + 200))
        {
            ulNewValue = UI_PARAM_ACCEL * 128;
        }
        else
        {
            ulNewValue = UI_PARAM_ACCEL * 128 *
                          (((UI_PARAM_ACCEL_CURRENT + 200)) -
                           g_sMotorCurrent);
        }

        //
        // See if the acceleration rate is greater than the requested
        // acceleration rate (i.e. the acceleration rate has been changed).
        //
        if(g_ulAccelRate > (UI_PARAM_ACCEL << 16))
        {
            //
            // Reduce the acceleration rate to the requested rate.
            //
            g_ulAccelRate = UI_PARAM_ACCEL << 16;
        }

        //
        // Then, see if the motor current exceeds the current at which the
        // acceleration rate should be reduced, and the newly computed
        // acceleration rate is less than the current rate.
        //
        else if((g_sMotorCurrent > UI_PARAM_ACCEL_CURRENT) &&
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
        else if(g_ulAccelRate < (UI_PARAM_ACCEL << 16))
        {
            //
            // Increase the acceleration rate by 15 RPM, slowly
            // returning it to the desired rate.
            //
            g_ulAccelRate += (15 << 16);
        }

        //
        // Increase the speed fraction by the acceleration rate.
        //
        g_ulSpeedFract += (g_ulAccelRate >> 16);

        //
        // Loop while the fraction is greater than one.
        //
        while(g_ulSpeedFract >= 1000)
        {
            //
            // Increment the speed whole part.
            //
            g_ulSpeedWhole++;

            //
            // Decrement the speed fraction by one.
            //
            g_ulSpeedFract -= 1000;
        }

        //
        // Convert the speed fraction and whole part into a 16.16 motor
        // drive speed.
        //
        g_ulSpeed = ((g_ulSpeedWhole << 14) +
                         ((g_ulSpeedFract << 14) / 1000));

        //
        // See if the speed has exceeded the target speed.
        //
        if(g_ulSpeed >= ulTarget)
        {
            //
            // Set the motor drive speed to the target speed.
            //
            g_ulSpeed = ulTarget;

            //
            // Compute the speed fraction and whole part from the drive
            // speed.
            //
            g_ulSpeedWhole = ulTarget >> 14;
            g_ulSpeedFract = ((ulTarget & 0x3FFF) * 1000) >> 14;

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
    // Otherwise, the target speed is less than the current speed.
    //
    else
    {
        //
        // Compute the new maximum deceleration rate, based on the current bus
        // voltage.
        //
        if(g_ulBusVoltage > (UI_PARAM_DECEL_VOLTAGE + 63))
        {
            ulNewValue = UI_PARAM_DECEL * 1024;
        }
        else
        {
            ulNewValue = (UI_PARAM_DECEL * 1024 *
                          (64 - g_ulBusVoltage + UI_PARAM_DECEL_VOLTAGE));
        }

        //
        // See if the deceleration rate is greater than the requested
        // deceleration rate (i.e. the deceleration rate has been changed).
        //
        if(g_ulDecelRate > (UI_PARAM_DECEL << 16))
        {
            //
            // Reduce the deceleration rate to the requested rate.
            //
            g_ulDecelRate = UI_PARAM_DECEL << 16;
        }

        //
        // Then, see if the bus voltage exceeds the voltage at which the
        // deceleration rate should be reduced, and the newly computed
        // deceleration rate is less than the current rate.
        //
        else if((g_ulBusVoltage > UI_PARAM_DECEL_VOLTAGE) &&
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
        else if(g_ulDecelRate < (UI_PARAM_DECEL << 16))
        {
            //
            // Increase the deceleration rate by 15 RPM, slowly
            // returning it to the desired rate.
            //
            g_ulDecelRate += (15 << 16);
        }

        //
        // Decrease the speed fraction by the decleration rate.
        //
        g_ulSpeedFract -= (g_ulDecelRate >> 16);

        //
        // Loop while the fraction is less than zero.
        //
        while(g_ulSpeedFract >= 1000)
        {
            //
            // Decrement the speed whole part.
            //
            g_ulSpeedWhole--;

            //
            // Increment the sped fraction by one.
            //
            g_ulSpeedFract += 1000;
        }

        //
        // Convert the speed fraction and whole part into a 16.16 motor
        // drive speed.
        //
        g_ulSpeed = ((g_ulSpeedWhole << 14) +
                         ((g_ulSpeedFract << 14) / 1000));

        //
        // See if the target speed has been reached (for non-zero target
        // speeds).
        //
        if((ulTarget != 0) && (g_ulSpeed < ulTarget))
        {
            //
            // Set the motor drive speed to the target speed.
            //
            g_ulSpeed = ulTarget;

            //
            // Compute the speed fraction and whole part from the drive
            // speed.
            //
            g_ulSpeedWhole = g_ulSpeed >> 14;
            g_ulSpeedFract = (((g_ulSpeed & 0x3FFF) * 1000) >> 14);

            //
            // Set the motor status to running.
            //
            g_ucMotorStatus = MOTOR_STATUS_RUN;
        }

        //
        // See if the speed has reached zero.
        //
        else if(((g_ulSpeed > 0xff000000) || (g_ulSpeed == 0)) ||
                ((UI_PARAM_MODULATION == MODULATION_SENSORLESS) &&
                (g_ulSpeedWhole < UI_PARAM_STARTUP_ENDSP)))
        {
            //
            // Set the motor drive speed to zero.
            //
            g_ulSpeed = 0;

            //
            // The speed fraction and whole part are zero as well.
            //
            g_ulSpeedWhole = 0;
            g_ulSpeedFract = 0;

            //
            // See if the motor drive is stopping.
            //
            if(g_ulState & STATE_FLAG_STOPPING)
            {
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

                //
                // Turn off the PWM outputs.
                //
                PWMOutputOff();
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
}

//*****************************************************************************
//
// Adjusts the motor drive power based on the target power.
//
// \param ulTarget is the target power of the motor drive, specified as mW.
//
// This function adjusts the motor drive power towards a given target
// power.  Limitations such as acceleration and deceleration rate, along
// with precautions such as limiting the deceleration rate to control the DC
// bus voltage, are handled by this function.
//
// \return None.
//
//*****************************************************************************
static void
MainPowerHandler(unsigned long ulTarget)
{
    unsigned long ulNewValue;

    //
    // Return without doing anything if the target speed has already been
    // reached.
    //
    if(ulTarget == g_ulPower)
    {
        return;
    }
    //
    // See if the target speed is greater than the current speed.
    //
    if(ulTarget > g_ulPower)
    {
        //
        // Compute the new maximum acceleration rate, based on the present
        // motor current.
        //
        if(g_sMotorCurrent >= (UI_PARAM_ACCEL_CURRENT + 200))
        {
            ulNewValue = UI_PARAM_ACCEL_POWER * 128;
        }
        else
        {
            ulNewValue = UI_PARAM_ACCEL_POWER * 128 *
                          (((UI_PARAM_ACCEL_CURRENT + 200)) -
                           g_sMotorCurrent);
        }

        //
        // See if the acceleration rate is greater than the requested
        // acceleration rate (i.e. the acceleration rate has been changed).
        //
        if(g_ulAccelRate > (UI_PARAM_ACCEL_POWER << 16))
        {
            //
            // Reduce the acceleration rate to the requested rate.
            //
            g_ulAccelRate = UI_PARAM_ACCEL_POWER << 16;
        }

        //
        // Then, see if the motor current exceeds the current at which the
        // acceleration rate should be reduced, and the newly computed
        // acceleration rate is less than the current rate.
        //
        else if((g_sMotorCurrent > UI_PARAM_ACCEL_CURRENT) &&
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
        else if(g_ulAccelRate < (UI_PARAM_ACCEL_POWER << 16))
        {
            //
            // Increase the acceleration rate by 15 RPM, slowly
            // returning it to the desired rate.
            //
            g_ulAccelRate += (15 << 16);
        }

        //
        // Increase the speed fraction by the acceleration rate.
        //
        g_ulPowerFract += (g_ulAccelRate >> 16);

        //
        // Loop while the fraction is greater than one.
        //
        while(g_ulPowerFract >= 1000)
        {
            //
            // Increment the speed whole part.
            //
            g_ulPowerWhole++;

            //
            // Decrement the speed fraction by one.
            //
            g_ulPowerFract -= 1000;
        }

        //
        // Convert the speed fraction and whole part into a 16.16 motor
        // drive speed.
        //
        g_ulPower = ((g_ulPowerWhole << 14) +
                         ((g_ulPowerFract << 14) / 1000));

        //
        // See if the speed has exceeded the target speed.
        //
        if(g_ulPower >= ulTarget)
        {
            //
            // Set the motor drive speed to the target speed.
            //
            g_ulPower = ulTarget;

            //
            // Compute the speed fraction and whole part from the drive
            // speed.
            //
            g_ulPowerWhole = ulTarget >> 14;
            g_ulPowerFract = ((ulTarget & 0x3FFF) * 1000) >> 14;

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
    // Otherwise, the target speed is less than the current speed.
    //
    else
    {
        //
        // Compute the new maximum deceleration rate, based on the current bus
        // voltage.
        //
        if(g_ulBusVoltage > (UI_PARAM_DECEL_VOLTAGE + 63))
        {
            ulNewValue = UI_PARAM_DECEL_POWER * 1024;
        }
        else
        {
            ulNewValue = (UI_PARAM_DECEL_POWER * 1024 *
                          (64 - g_ulBusVoltage + UI_PARAM_DECEL_VOLTAGE));
        }

        //
        // See if the deceleration rate is greater than the requested
        // deceleration rate (i.e. the deceleration rate has been changed).
        //
        if(g_ulDecelRate > (UI_PARAM_DECEL_POWER << 16))
        {
            //
            // Reduce the deceleration rate to the requested rate.
            //
            g_ulDecelRate = UI_PARAM_DECEL_POWER << 16;
        }

        //
        // Then, see if the bus voltage exceeds the voltage at which the
        // deceleration rate should be reduced, and the newly computed
        // deceleration rate is less than the current rate.
        //
        else if((g_ulBusVoltage > UI_PARAM_DECEL_VOLTAGE) &&
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
        else if(g_ulDecelRate < (UI_PARAM_DECEL_POWER << 16))
        {
            //
            // Increase the deceleration rate by 15 RPM, slowly
            // returning it to the desired rate.
            //
            g_ulDecelRate += (15 << 16);
        }

        //
        // Decrease the speed fraction by the decleration rate.
        //
        g_ulPowerFract -= (g_ulDecelRate >> 16);

        //
        // Loop while the fraction is less than zero.
        //
        while(g_ulPowerFract >= 1000)
        {
            //
            // Decrement the speed whole part.
            //
            g_ulPowerWhole--;

            //
            // Increment the sped fraction by one.
            //
            g_ulPowerFract += 1000;
        }

        //
        // Convert the speed fraction and whole part into a 16.16 motor
        // drive speed.
        //
        g_ulPower = ((g_ulPowerWhole << 14) +
                         ((g_ulPowerFract << 14) / 1000));

        //
        // See if the target speed has been reached (for non-zero target
        // speeds).
        //
        if((ulTarget != 0) && (g_ulPower < ulTarget))
        {
            //
            // Set the motor drive speed to the target speed.
            //
            g_ulPower = ulTarget;

            //
            // Compute the speed fraction and whole part from the drive
            // speed.
            //
            g_ulPowerWhole = g_ulPower >> 14;
            g_ulPowerFract = (((g_ulPower & 0x3FFF) * 1000) >> 14);

            //
            // Set the motor status to running.
            //
            g_ucMotorStatus = MOTOR_STATUS_RUN;
        }

        //
        // See if the speed has reached zero.
        //
        else if((g_ulPower > 0xff000000) || (g_ulPower == 0))
        {
            //
            // Set the motor drive power to zero.
            //
            g_ulPower = 0;

            //
            // The power fraction and whole part are zero as well.
            //
            g_ulPowerWhole = 0;
            g_ulPowerFract = 0;

            //
            // See if the motor drive is stopping.
            //
            if(g_ulState & STATE_FLAG_STOPPING)
            {
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

                //
                // Turn off the PWM outputs.
                //
                PWMOutputOff();
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
}

//*****************************************************************************
//
// Handles the millisecond speed update software interrupt.
//
// This function is called as a result of the speed update software
// interrupt being asserted.  This interrupted is asserted every millisecond
// by the PWM interrupt handler.
//
// The speed of the motor drive will be updated, along with handling state
// changes of the drive (such as initiating braking when the motor drive has
// come to a stop).
//
// \note Since this interrupt is software triggered, there is no interrupt
// source to clear in this handler.
//
// \return None.
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
        g_ulMeasuredSpeed = 0;
        return;
    }

    //
    // Set the measured speed based on the encoder/sensor settings.
    //
    if (UI_PARAM_MODULATION == MODULATION_SENSORLESS)
    {
        g_ulMeasuredSpeed = g_ulBEMFRotorSpeed;
    }
    else if(UI_PARAM_ENCODER_PRESENT == ENCODER_PRESENT)
    {
        g_ulMeasuredSpeed = g_ulRotorSpeed;
    }
    else if(UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_GPIO)
    {
        g_ulMeasuredSpeed = g_ulHallRotorSpeed;
    }
    else if(UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_GPIO_60)
    {
        g_ulMeasuredSpeed = g_ulHallRotorSpeed;
    }
    else
    {
        g_ulMeasuredSpeed = g_ulLinearRotorSpeed;
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
    // See if the motor drive is in startup mode.
    //
    if(g_ulState & STATE_FLAG_STARTUP)
    {
        //
        // Handle precharge mode.
        //
        MainStartupHandler();

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
        // Determine the target speed.  First, see if the motor drive is
        // stopping or reversing direction.
        //
        if((g_ulState & STATE_FLAG_STOPPING) || (g_ulState & STATE_FLAG_REV))
        {
            //
            // When stopping or reversing direction, the target speed is
            // zero.
            //
            ulTarget = 0;
        }

        //
        // Otherwise, use the target speed specified from the UI.
        //
        else
        {
            if(UI_PARAM_CONTROL_MODE == CONTROL_MODE_SPEED)
            {
                //
                // The target speed is the user supplied value,
                // converted to 18.14 fixed-point format.
                //
                ulTarget = (g_ulTargetSpeed << 14);
            }
            else
            {
                //
                // The target power is the user supplied value,
                // converted to 18.14 fixed-point format.
                //
                ulTarget = (g_ulTargetPower << 14);
            }
        }
        g_ulSineTarget = ulTarget;

        //
        // If transitioning out of Startup, allow the speed/power/current
        // to stabilize.
        //
        if(UI_PARAM_MODULATION == MODULATION_SENSORLESS)
        {
            if(g_ulStateCount)
            {
                g_ulStateCount--;
                if(UI_PARAM_CONTROL_MODE == CONTROL_MODE_SPEED)
                {
                    //
                    // The target speed is the user supplied value,
                    // converted to 18.14 fixed-point format.
                    //
                    ulTarget = g_ulSpeed;
                }
                else
                {
                    //
                    // The target power is the user supplied value,
                    // converted to 18.14 fixed-point format.
                    //
                    ulTarget = g_ulPower;
                }
            }
            if(g_ulStateCount == 1)
            {
                //
                // Restore the decay mode flag in the flags variable.
                //
                g_ucDecayMode = g_ucLocalDecayMode;
            }
        }

        if(UI_PARAM_CONTROL_MODE == CONTROL_MODE_SPEED)
        {
            //
            // Handle the update to the motor drive speed based on the target
            // speed.
            //
            MainSpeedHandler(ulTarget);
        }
        else
        {
            //
            // Handle the update to the motor drive power based on the target
            // power.
            //
            MainPowerHandler(ulTarget);
        }

        //
        // Compute the angle delta based on the new motor drive speed and the
        // number of poles in the motor.
        //
        g_ulAngleDelta = (((g_ulSpeed / 60) << 9) / g_ulPWMFrequency) << 9;
        g_ulAngleDelta *= (UI_PARAM_NUM_POLES / 2);

        //
        // Update the target Amplitude/Duty Cycle for the motor drive.
        // First, check if current has exceeded maximum current value.  If
        // it has, then just reduce the duty cycle to reduce current.
        //
#if (UI_PARAM_TARGET_CURRENT != 0)
        if(g_sMotorCurrent > UI_PARAM_TARGET_CURRENT)
        {
            static short sPreviousMotorCurrent = 0;

            if(sPreviousMotorCurrent != g_sMotorCurrent)
            {
                //
                // Save the motor current value.
                //
                sPreviousMotorCurrent = g_sMotorCurrent;

                //
                // Get the difference between the target and motor current.
                //
                ulTarget = (g_sMotorCurrent - UI_PARAM_TARGET_CURRENT);

                //
                // Compute the percentage over, in 16.16 fixed point format.
                //
                ulTarget = ((ulTarget * 65536) / UI_PARAM_TARGET_CURRENT);

                //
                // Compute the equivalent percentage of the current duty cycle.
                //
                ulTarget = ((ulTarget * g_ulDutyCycle) / 65536);

                //
                // Reduce the duty cycle.
                //
                g_ulDutyCycle = g_ulDutyCycle - ulTarget;
            }
        }
        else
#endif

        //
        // Here, we are under current, so just use the normal
        // control algorithm to determine duty cycle.
        //
        if((UI_PARAM_TARGET_CURRENT == 0) ||
          ((UI_PARAM_TARGET_CURRENT != 0) &&
           (g_sMotorCurrent <= UI_PARAM_TARGET_CURRENT)))
        {
            if(UI_PARAM_CONTROL_MODE == CONTROL_MODE_SPEED)
            {
                g_ulDutyCycle = MainSpeedController();
            }
            else
            {
                g_ulDutyCycle = MainPowerController();
            }
        }
        if(UI_PARAM_MODULATION != MODULATION_SINE)
        {
            PWMSetDutyCycle(g_ulDutyCycle, g_ulDutyCycle, g_ulDutyCycle);
        }
    }
}

//*****************************************************************************
//
// Starts the motor drive.
//
// This function starts the motor drive.  If the motor is currently stopped,
// it will begin the process of starting the motor.  If the motor is currently
// stopping, it will cancel the stop operation and return the motor to the
// target speed.
//
// \return None.
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
        // If Phase Back EMF Voltage is more than 500mV, then it is likely
        // that the motor shaft is still spinning, and we shouldn't attempt
        // to start the motor at this time.
        //
        // Eventually, we should use the Back EMF voltage to estimate rotor
        // shaft speed, and use that value to prime the motor drive parameters
        // to restart a spinning motor after a brownout condition.
        //
        if((UI_PARAM_STARTUP_THRESH) &&
           (g_ulPhaseBEMFVoltage > UI_PARAM_STARTUP_THRESH))
        {
            IntEnable(INT_PWM0_2);
            return;
        }

        //
        // Force the previous "Hall" sensor value to non-valid number
        // to trigger a Hall edge.
        //
        g_ulHallPrevious = 8;

        //
        // Reset the Stall Detection Count.
        //
        g_ulStallDetectCount = 0;

        //
        // Set the initial acceleration and deceleration based on the current
        // parameter values.
        //
        if(UI_PARAM_CONTROL_MODE == CONTROL_MODE_SPEED)
        {
            g_ulAccelRate = UI_PARAM_ACCEL << 16;
            g_ulDecelRate = UI_PARAM_DECEL << 16;
        }
        else
        {
            g_ulAccelRate = UI_PARAM_ACCEL_POWER << 16;
            g_ulDecelRate = UI_PARAM_DECEL_POWER << 16;
        }

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
        g_ulStateCount = UI_PARAM_PRECHARGE_TIME + 1;

        //
        // See if the motor drive should run forward or backward.
        //
        if(g_bForward)
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
// Stops the motor drive.
//
// This function stops the motor drive.  If the motor is currently running,
// it will begin the process of stopping the motor.
//
// \return None.
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
            g_ulDecelRate = UI_PARAM_DECEL << 16;
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
            g_ulDecelRate = UI_PARAM_DECEL << 16;
        }

        //
        // Advance the state machine to the backward decelerate to a stop
        // state.
        //
        g_ulState = STATE_BACK_STOPPING;
    }

    //
    // See if the motor is running in the startup mode.
    //
    if(g_ulState & STATE_FLAG_STARTUP)
    {
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

        //
        // Turn off the PWM outputs.
        //
        PWMOutputOff();
    }

    //
    // Re-enable the millisecond interrupt.
    //
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
// Emergency stops the motor drive.
//
// This function performs an emergency stop of the motor drive.  The outputs
// will be shut down immediately, the drive put into the stopped state with
// the speed at zero, and the emergency stop fault condition will be
// asserted.
//
// \return None.
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
    // Disable all the PWM outputs.
    //
    PWMOutputOff();
    g_ulDutyCycle = 0;
    PWMSetDutyCycle(g_ulDutyCycle, g_ulDutyCycle, g_ulDutyCycle);

    //
    // Clear out all of the main run-time parameters.
    //
    g_ulSpeed = g_ulSpeedWhole = g_ulSpeedFract = 0;
    g_ulPower = g_ulPowerWhole = g_ulPowerFract = 0;
    g_ulAngle = g_ulAngleDelta = 0;
    g_ulStallDetectCount = 0;
    if(UI_PARAM_CONTROL_MODE == CONTROL_MODE_SPEED)
    {
        g_ulAccelRate = UI_PARAM_ACCEL << 16;
        g_ulDecelRate = UI_PARAM_DECEL << 16;
    }
    else
    {
        g_ulAccelRate = UI_PARAM_ACCEL_POWER << 16;
        g_ulDecelRate = UI_PARAM_DECEL_POWER << 16;
    }
    g_lSpeedIntegrator = 0;
    g_lPowerIntegrator = 0;
    g_ulMeasuredSpeed = 0;

    //
    // Re-enable the update interrupts.
    //
    IntEnable(INT_PWM0_1);
    IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
// Determines if the motor drive is currently running.
//
// This function will determine if the motor drive is currently running.  By
// this definition, running means not stopped; the motor drive is considered
// to be running even when it is precharging before starting the waveforms and
// DC injection braking after stopping the waveforms.
//
// \return Returns 0 if the motor drive is stopped and 1 if it is running.
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
// Determines if the motor drive is in precharge/startup.
//
// This function will determine if the motor drive is currently in startup.
// (or precharge) mode.
//
// \return Returns 0 if the motor drive is not in startup and 1 if it is
// running in startup (or precharge).
//
//*****************************************************************************
unsigned long
MainIsStartup(void)
{
    //
    // Return the appropriate value based on whether or not the motor drive is
    // stopped.
    //
    if(g_ulState & (STATE_FLAG_STARTUP | STATE_FLAG_PRECHARGE))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// Determines if the motor drive is currently in reverse mode.
//
// This function will determine if the motor drive is currently running.  By
// this definition, running means not stopped; the motor drive is considered
// to be running even when it is precharging before starting the waveforms and
// DC injection braking after stopping the waveforms.
//
// \return Returns 0 if the motor drive is not in reverse and 1 if it is
// in reverse.
//
//*****************************************************************************
unsigned long
MainIsReverse(void)
{
    //
    // Return the appropriate value based on whether or not the motor drive is
    // in reverse or not.
    //
    if((g_ulState & STATE_FLAG_FORWARD) == STATE_FLAG_BACKWARD)
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// Indicate that a fault condition has been detected.
//
// \param ulFaultFlag is a flag that indicates the fault condition that was
// detected.
//
// This function is called when a fault condition is detected.  It will update
// the fault flags to indicate the fault condition that was detected, and
// cause the fault LED to blink to indicate a fault.
//
// \return None.
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
// Clears the latched fault conditions.
//
// This function will clear the latched fault conditions and turn off the
// fault LED.
//
// \return None.
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
// Determines if a latched fault condition exists.
//
// This function determines if a fault condition has occurred but not been
// cleared.
//
// \return Returns 1 if there is an uncleared fault condition and 0 otherwise.
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
// This is the code that gets called when the watchdog timer expires for the
// first time.  If this code doesn't handle the situation, then the watchdog
// will expire again and reset the board.
//
//*****************************************************************************
void
WatchdogIntHandler(void)
{
    //
    // Clear the watchdog interrupt.
    //
    WatchdogIntClear(WATCHDOG0_BASE);

    //
    // If the motor state is stopped, do nothing here.
    //
    if(g_ulState == STATE_STOPPED)
    {
        return;
    }

    //
    // Indicate an ambient overtemperature fault.
    //
    MainSetFault(FAULT_WATCHDOG);

    //
    // Disable all PWM outputs.
    //
    PWMOutputOff();
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
// Handles setup of the application on the Brushless DC motor drive.
//
// This is the main application entry point for the Brushless DC motor drive.
// It is responsible for basic system configuration, initialization of the
// various application drivers and peripherals, and the main application loop.
//
// \return Never returns.
//
//*****************************************************************************
int
main(void)
{
    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Configure the processor to run at 50 MHz.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable the peripherals used by the application.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    //
    // Enable the peripherals that should continue to run when the processor
    // is sleeping.
    //
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_QEI0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER1);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_WDOG0);

    //
    // Enable peripheral clock gating.  Note that this is required in order to
    // measure the the processor usage.
    //
    SysCtlPeripheralClockGating(true);

    //
    // Set the priorities of the interrupts used by the application.
    //
    IntPrioritySet(INT_TIMER1A,     0x00);
    IntPrioritySet(INT_GPIOB,       0x20);
    IntPrioritySet(INT_GPIOD,       0x20);
    IntPrioritySet(INT_TIMER0A,     0x20);
    IntPrioritySet(INT_WATCHDOG,    0x40);
    IntPrioritySet(INT_ADC0SS0,     0x60);
    IntPrioritySet(INT_PWM0_0,      0x80);
    IntPrioritySet(INT_PWM0_1,      0xa0);
    IntPrioritySet(INT_PWM0_2,      0xc0);
    IntPrioritySet(FAULT_SYSTICK,   0xc0);

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
    // Initialize the Hall Sensor control routines.
    //
    HallInit();

    //
    // Initialize the user interface.
    //
    UIInit();

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
    // Initialize the Watchdog timer for a 100ms timeout.
    //
    IntEnable(INT_WATCHDOG);
    WatchdogReloadSet(WATCHDOG0_BASE, WATCHDOG_RELOAD_VALUE);
    WatchdogResetEnable(WATCHDOG0_BASE);
    WatchdogEnable(WATCHDOG0_BASE);

    //
    // Configure Timer 0 as a one-shot timer to be used for commutating the
    // motor in Sensorless mode, based on Back EMF detection.
    //
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);

    //
    // Loop forever.  All the real work is done in interrupt handlers.
    //
    while(1)
    {
        //
        // Put the processor to sleep.
        //
#ifndef DEBUG
        SysCtlSleep();
#endif
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
// @}
//
//*****************************************************************************
