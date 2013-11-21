//*****************************************************************************
//
// stepper.c - Stepper motor control API.
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

#include "inc/hw_comp.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/comp.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "commands.h"
#include "stepctrl.h"
#include "stepcfg.h"
#include "stepper.h"
#include "stepseq.h"

//*****************************************************************************
//
//! \page stepper_intro Introduction
//!
//! This is the main API used for controlling the stepper motor. This
//! API can be used for configuring the stepper modes and parameters,
//! and commanding the motor to move. The API is also used to retrieve
//! status such as current motor position and speed.
//!
//! This API should be used by the application as the main interface to
//! the stepper motor control.
//!
//! Most of the functions in this API are passed through to lower level
//! modules in order to carry out the action or get status.
//!
//! The function StepperInit() should be called once during system
//! initialization, to initialize the stepper API module.
//!
//! The functions StepperEnable() and StepperDisable() are used for
//! enabling and disabling the motor. The stepper motor will not run until
//! it has been enabled. The function StepperEmergencyStop() can be used
//! when the motor needs to be stopped right away.
//!
//! The function StepperSetMotion() is the main function used for commanding
//! the stepper motor to move. This is used to set the position, speed,
//! and acceleration values for the motor.
//!
//! The function StepperGetMotorStatus() is used to retrieve status information
//! about the motor, such as the current position and speed.
//!
//! The following functions are used for configuring various parameters
//! used for the motor operation: StepperSetControlMode(),
//! StepperSetStepMode(), StepperSetDecayMode(), StepperSetPWMFreq(),
//! StepperSetFixedOnTime(), StepperSetBlankingTime(), StepperSetMotorParms(),
//! StepperSetFaultParms().
//!
//! The code for implementing the stepper API is contained in
//! <tt>stepper.c</tt>, with <tt>stepper.h</tt> containing the definitions
//! for the variables and functions exported to the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup stepper_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Holds the current status of the motor. The fields are updated when
//! the StepperGetMotorStatus() function is called, and a pointer to this
//! is returned to the caller.
//
//*****************************************************************************
tStepperStatus sStepperStatus;

//*****************************************************************************
//
//! Sets new position and motion parameters for the stepper,
//! and initiates motion (if enabled).
//!
//! \param lPos is the target position for the stepper in 24.8 format
//! \param usSpeed is the running speed in steps/second, that should be
//!                used when the motor is moving
//! \param usAccel is the acceleration in steps/second**2, that is
//!                used when the motor is accelerated to the running speed
//! \param usDecel is the deceleration in steps/second**2, that is
//!                used when decelerating the motor from the running speed
//!                to a stop
//!
//! This function will start the stepper motor moving to the target position
//! specified by lPos. The acceleration and deceleration parameters will be
//! used to create a speed profile that will be used as the motor runs.
//!
//! If the stepper has been enabled by a prior call to StepperEnable(), then
//! this function will take effect immediately. If the stepper is not enabled,
//! or has been stopped by StepperDisable(), then this function will have no
//! effect. If the motor is already moving, then the speed profile is
//! recalculated, and the motor speed adjusted if necessary.
//!
//! The parameter \e lPos is a signed number representing the motor position
//! in fixed-point 24.8 format.  The upper 24 bits are the (signed) whole step
//! position, while the lower 8 bits are the fractional step position.  While
//! this allows for a theoretical resolution of 1/256 step size, the motor
//! does not actually support micro-steps that small.  The value of the lower
//! 8 bits (the fractional step) is the fractional value multiplied by 256.
//! For example, the lower 8 bits of a half-step is 0x80 (0.5 * 256).
//! Likewise, a quarter step is 0x40 (0.25 * 256).  In order to use half-steps
//! you must be using half- or micro-stepping mode.  In order to use fractional
//! steps smaller than half, you must be using micro-stepping mode.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetMotion(long lPos, unsigned short usSpeed,
                 unsigned short usAccel, unsigned short usDecel)
{
    //
    // If the stepper is enabled for operation, then remember the
    // target position in the status structure, and then call in to
    // the step sequencer to start a motion based on the motion
    // parameters that were passed in to this function.
    //
    if(sStepperStatus.bEnabled)
    {
        sStepperStatus.lTargetPos = lPos;
        StepSeqMove(lPos, usSpeed, usAccel, usDecel);
    }
}

//*****************************************************************************
//
//! Sets the motor winding parameters used for controlling winding current.
//!
//! \param usDriveCurrent is the drive current in milliamps. The control
//!        method will attempt to drive the winding current to this value
//!        when stepping.
//! \param usHoldCurrent is the holding current in milliamps. The control
//!        method will attempt to maintain this current in the winding when
//!        the motor is stopped.
//! \param usBusVoltage is the bus voltage used for driving the motor
//!        in millivolts.
//! \param usDriveResistance is the stepper winding resistance in milliohms.
//!
//! This function will set the parameters associated with the current used
//! to drive the motor windings. The drive current is applied to the
//! windings when the motor is stepping. The hold current is applied to
//! the active windings when the motor is stopped. The holding current
//! can be zero. The resistance is the winding resistance from the
//! motor specification. The resistance and bus voltage is used for
//! calculating duty cycle when using a PWM control method.
//!
//! Calls to this function will take effect immediately. It is up to the
//! caller to ensure that the current values specified are within safe
//! limits for the motor, and that the actual bus voltage does not
//! exceed the value specified by usBusVoltage.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetMotorParms(unsigned short usDriveCurrent,
                     unsigned short usHoldCurrent,
                     unsigned short usBusVoltage,
                     unsigned short usDriveResistance)
{
    unsigned long ulMaxCurrent;

    //
    // Compute the maximum current through the winding if full voltage
    // is applied.
    //
    ulMaxCurrent = (usBusVoltage * 1000L) / usDriveResistance;

    //
    // Set the drive and hold current used by the step sequencer.
    //
    StepSeqCurrent(usDriveCurrent, usHoldCurrent, ulMaxCurrent);
}

//*****************************************************************************
//
//! Sets the PWM frequency used for PWM control modes.
//!
//! \param usPWMFreq is the PWM frequency in Hz.
//!
//! This function will set PWM frequency for PWM-based control modes.
//!
//! This function will only take effect if the motor is stopped. Otherwise,
//! it has no effect.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetPWMFreq(unsigned short usPWMFreq)
{
    //
    // If the motor is stopped, then allow change of PWM frequency.
    //
    if(g_ucMotorStatus == MOTOR_STATUS_STOP)
    {
        //
        // Remember the current PWM setting in the stepper status.
        //
        sStepperStatus.usPWMFrequency = usPWMFreq;

        //
        // Set the PWM frequency in the StepSeq module
        //
        g_usPwmFreq = usPWMFreq;

        //
        // If the control mode is already PWM, then setting the
        // control mode again will force an update of the PWM hardware.
        //
        if((sStepperStatus.ucControlMode == CONTROL_MODE_OPENPWM) ||
           (sStepperStatus.ucControlMode == CONTROL_MODE_CLOSEDPWM))
        {
            StepSeqControlMode(sStepperStatus.ucControlMode);
        }
    }
}

//*****************************************************************************
//
//! Sets the fixed-on interval when using open-loop PWM control mode.
//!
//! \param usFixedOnTime is the time the winding will remain on before PWM
//!        in microseconds.
//!
//! The fixed-on time is the amount of time that the winding will be left
//! turned on at the beginning of the step, in order to let the current
//! rise as fast as possible. At the end of the fixed-on time, the control
//! signal switches to PWM.
//!
//! Calls to this function will take effect immediately.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetFixedOnTime(unsigned short usFixedOnTime)
{
    //
    // Set the global variable that holds the fixed on time.
    //
    g_usFixedOnTime = usFixedOnTime;
}

//*****************************************************************************
//
//! Set the off blanking interval for chopper control mode.
//!
//! \param usBlankOffTime is the off blanking time in microseconds.
//!
//! This function is used to set the off blanking time used in
//! the chopper mode. The off blanking time is the amount of time that
//! the winding is kept off after the chopper turns the winding off,
//! before turning it on again. This is actually the minimum time, since
//! the chopper may dynamically lengthen the off blanking time if needed
//! to keep the current from rising too much.
//!
//! Calls to this function take effect immediately.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetBlankingTime(unsigned short usBlankOffTime)
{
    //
    // Update the off blanking time with the value passed in.
    //
    g_usBlankOffTime = usBlankOffTime;
}

//*****************************************************************************
//
//! Sets the current decay mode.
//!
//! \param ucDecayMode is the current decay mode, DECAY_MODE_FAST or
//!         DECAY_MODE_SLOW.
//!
//! This function is used to set the current decay mode used when the motor
//! winding is switched off during chopping or PWM control. Slow decay
//! mode closes both low-side switches on the H-bridge, which allows the
//! current in the winding to recirculate and decay slowly. Fast decay
//! mode opens all the switches on the winding so that the current cannot
//! recirculate and decays rapidly.
//!
//! Calls to this function take effect immediately.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetDecayMode(unsigned char ucDecayMode)
{
    //
    // Call to the Step Sequencer to set the current decay mode.
    //
    StepSeqDecayMode(ucDecayMode);
}

//*****************************************************************************
//
//! Set the current control mode for the motor windings.
//!
//! \param ucControlMode is the current control method,
//!        CONTROL_MODE_OPENPWM, CONTROL_MODE_CLOSEDPWM,
//!        or CONTROL_MODE_CHOPPER.
//!
//! This function is used to set the method that is used to control the
//! current in the winding.
//!
//! The open-loop PWM method applies voltage to the winding for a fixed amount
//! of time (fixed on time), and then switches to PWM. This allows the
//! current to rise rapidly in the winding before the PWM starts. The
//! fixed on time can be minimal which is the same as using PWM without
//! a fixed rise time.
//!
//! The chopper method monitors the winding current and turns the H-bridge
//! on and off to maintain the desired current.
//!
//! The closed-loop PWM method measures the current during the PWM pulse
//! and adjusts the duty cycle to maintain the current at the desired level.
//!
//! Calls to this function only take effect if the motor is not running.
//! Otherwise, the setting is ignored.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetControlMode(unsigned char ucControlMode)
{
    //
    // If the motor is stopped, then update the current control mode.
    //
    if(g_ucMotorStatus == MOTOR_STATUS_STOP)
    {
        //
        // Save the new control mode in the status structure, then call
        // step sequencer to set the new control mode.
        //
        sStepperStatus.ucControlMode = ucControlMode;
        StepSeqControlMode(ucControlMode);
    }
}

//*****************************************************************************
//
//! Sets the stepping mode step size.
//!
//! \param ucStepMode is the stepping size, STEP_MODE_FULL, STEP_MODE_WAVE,
//!                   STEP_MODE_HALF, or STEP_MODE_MICRO.
//!
//! This function is used to set the step size to full, half or micro steps.
//!
//! Calls to this function only take effect if the motor is not running.
//! Otherwise, the setting is ignored.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetStepMode(unsigned char ucStepMode)
{
    //
    // If the motor is stopped, then update the step size.
    //
    if(g_ucMotorStatus == MOTOR_STATUS_STOP)
    {
        //
        // Save the new step size in the status structure, then
        // call the Step Sequencer to set the new step size.
        //
        sStepperStatus.ucStepMode = ucStepMode;
        StepSeqStepMode(ucStepMode);
    }
}

//*****************************************************************************
//
//! Sets the fault current level that is used for hardware fault control.
//!
//! \param usFaultCurrent is the fault current in milliamps.
//!
//! This function is used to set the comparator that will be triggered
//! if the current rises above a certain level. If this happens, then the
//! comparator will trigger a fault condition, independent of software,
//! and will shut off all the control signals to the motor.
//!
//! \return None.
//
//*****************************************************************************
void
StepperSetFaultParms(unsigned short usFaultCurrent)
{
    //
    // Clear any pending interrupts
    //
    ComparatorIntClear(COMP_BASE, 0);

    //
    // If the specified fault current is non-zero, then update the
    // comparator.
    //
    if(usFaultCurrent)
    {
        //
        // Compute the comparator new voltage reference value based
        // on the specified fault current, then load the new vref
        // into the comparator register.
        //
        HWREG(COMP_BASE + COMP_O_ACREFCTL) = COMP_REF_0V |
                                           ((usFaultCurrent + 687) / 1375);

        //
        // Enable the comparator interrupt, so it will cause an
        // interrupt if the current goes above the setting.
        //
        ComparatorIntEnable(COMP_BASE, 0);
    }
}

//*****************************************************************************
//
//! Get the status of the motor.
//!
//! This function returns a pointer to a structure that holds the motor
//! status. Status items include position, speed, etc. This function
//! must be called in order to update the data items in the structure.
//!
//! \return A pointer to the structure containing the status information.
//
//*****************************************************************************
tStepperStatus *
StepperGetMotorStatus(void)
{
    //
    // Read the motor status and position from globals.
    //
    sStepperStatus.lPosition = g_lCurrentPos;
    sStepperStatus.ucMotorStatus = g_ucMotorStatus;

    //
    // Compute the speed from the step time. The step time must be
    // converted from 24.8 format.
    //
    if(g_ulStepTime)
    {
        sStepperStatus.usSpeed = SYSTEM_CLOCK / ((g_ulStepTime + 128) >> 8);
    }
    else
    {
        sStepperStatus.usSpeed = 0;
    }

    //
    // If half stepping is used then the speed needs to be
    // corrected to full step rate (from half step rate).
    //
    if(sStepperStatus.ucStepMode == STEP_MODE_HALF)
    {
        sStepperStatus.usSpeed /= 2;
    }

    //
    // Else if micro stepping is used then the speed needs to be
    // corrected to full step rate (from micro step rate).
    //
    else if(sStepperStatus.ucStepMode == STEP_MODE_MICRO)
    {
        sStepperStatus.usSpeed /= 8;
    }

    //
    // Read the winding currents and convert to milliamps.
    // Reset the peak counts.
    //
    sStepperStatus.usCurrent[0] = COUNTS2MILLIAMPS(g_ulPeakCurrentRaw[0]);
    sStepperStatus.usCurrent[1] = COUNTS2MILLIAMPS(g_ulPeakCurrentRaw[1]);
    g_ulPeakCurrentRaw[0] = 0;
    g_ulPeakCurrentRaw[1] = 0;

    return(&sStepperStatus);
}

//*****************************************************************************
//
//! Enable the stepper for running.
//!
//! This function enables the stepper motor for running. All motion commands
//! are ignored if the stepper is not enabled. The motor cannot be enabled
//! if there are any pending faults. The function StepperClearFaults() must
//! be called first.
//!
//! \return None.
//
//*****************************************************************************
void
StepperEnable(void)
{
    //
    // Only allow the stepper to be enabled if there is no fault pending.
    //
    if(!sStepperStatus.ucFaultFlags)
    {
        sStepperStatus.bEnabled = 1;
    }
}

//*****************************************************************************
//
//! Disable the stepper for running.
//!
//! This function disables the stepper motor for running. If the motor is
//! currently moving, it will be gracefully stopped. After that, all
//! motion commands will be ignored until StepperEnable() is called again.
//!
//! \return None.
//
//*****************************************************************************
void
StepperDisable(void)
{
    //
    // Call to Step Sequencer to gracefully stop the motor. Then clear
    // the enabled flag so no more operations can occur.
    //
    StepSeqStop();
    sStepperStatus.bEnabled = 0;
}

//*****************************************************************************
//
//! Immediately stop and place the motor in a safe state.
//!
//! This function disables all the motor control signals immediately. This
//! is not a graceful stop, and the position information will be lost. The
//! motor will be left in the disabled state.
//!
//! \return None.
//
//*****************************************************************************
void
StepperEmergencyStop(void)
{
    //
    // Call to Step Sequencer to shut down the motor immediately, and
    // clear the enabled flag so the stepper motor is disabled.
    //
    StepSeqShutdown();
    sStepperStatus.bEnabled = 0;
}

//*****************************************************************************
//
//! Sets the value of the current position.
//!
//! \param lNewPosition is the new position in 24.8 format.
//!
//! This function can be used to initialize or reset the current known
//! position. Whatever value is passed, the current position will be updated
//! to match, without moving the motor. This can be used when "homing" the
//! motor. For example, if the position is known due to a limit switch, then
//! the position could be reset to that known value.
//!
//! The value of the new position is restricted to whole steps, and
//! any fractional portion will be truncated.
//!
//! This function can only be used when the motor is not moving. If the
//! motor is moving then the new setting is ignored.
//!
//! The parameter \e lNewPosition is a signed number representing the
//! motor position in fixed-point 24.8 format.  The upper 24 bits are the
//! (signed) whole step position, while the lower 8 bits are the fractional
//! step position.  While this allows for a theoretical resolution of 1/256
//! step size, the motor does not actually support micro-steps that small.
//! The value of the lower 8 bits (the fractional step) is the fractional
//! value multiplied by 256.  For example, the lower 8 bits of a half-step
//! is 0x80 (0.5 * 256).  Likewise, a quarter step is 0x40 (0.25 * 256).  In
//! order to use half-steps you must be using half- or micro-stepping mode.
//! In order to use fractional steps smaller than half, you must be using
//! micro-stepping mode.
//!
//! \return None.
//
//*****************************************************************************
void
StepperResetPosition(long lNewPosition)
{
    //
    // If the motor is not running, then update the current position.
    //
    if(g_ucMotorStatus == MOTOR_STATUS_STOP)
    {
        g_lCurrentPos = lNewPosition & ~0xff;   // enforce whole steps
    }
}

//*****************************************************************************
//
//! Clears the fault flags.
//!
//! This function clears the fault flags, which will allow the motor to
//! run again, after a fault occurred.
//!
//! \return None.
//
//*****************************************************************************
void
StepperClearFaults(void)
{
    //
    // Clear all the fault flags.
    //
    sStepperStatus.ucFaultFlags = 0;
}

//*****************************************************************************
//
//! Interrupt handler for the comparator interrupt.
//!
//! This interrupt handler is triggered when the comparator trips. The
//! comparator is set to trip when the combined winding current goes above a
//! certain value. When this happens, the fault signal will be asserted,
//! which will automatically place the hardware in a safe state.
//!
//! In this function, the motor is stopped immediately and placed in a safe
//! state, and the over current fault flag is set.
//!
//! \return None.
//
//*****************************************************************************
void
StepperCompIntHandler(void)
{
    //
    // Clear the interrupt
    //
    ComparatorIntClear(COMP_BASE, 0);

    //
    // Stop and safe the motor as fast as possible.
    //
    StepperEmergencyStop();

    //
    // Set the fault flag to indicate overcurrent.
    //
    sStepperStatus.ucFaultFlags |= FAULT_FLAG_CURRENT;
}

//*****************************************************************************
//
//! Initializes the stepper control module.
//!
//! This function sets up the stepper software, and initializes the
//! hardware necessary for control of the stepper. This should be called
//! just once when the system is initialized. It will call the Init functions
//! for all lower modules.
//!
//! \return None.
//
//*****************************************************************************
void
StepperInit(void)
{
    //
    // Enable the GPIO ports and the comparator, needed for comparator
    // use for current fault detection.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_COMP0);

    //
    // Configure the comparator GPIO pins for comparator use.
    //
    GPIOPinTypeComparator(GPIO_PORTB_BASE, GPIO_PIN_4);
    GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_5, GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_5, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD);

    //
    // Configure the fault pin so that a comparator trigger will cause
    // the PWM outputs to safe.
    //
    GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD);

    //
    // Configure the comparator to generate an interrupt on the rising
    // edge, and using the internal voltage reference.
    //
    ComparatorConfigure(COMP_BASE, 0, COMP_TRIG_NONE | COMP_INT_RISE |
                                      COMP_ASRCP_REF | COMP_OUTPUT_INVERT);

    //
    // Set the initial reference to a high value to avoid triggering until
    // the correct value can be set, leave the comparator interrupt
    // disabled for now.
    //
    ComparatorRefSet(COMP_BASE, COMP_REF_2_0625V);
    ComparatorIntDisable(COMP_BASE, 0);
    IntEnable(INT_COMP0);
    IntPrioritySet(INT_COMP0, COMP_INT_PRI);

    //
    // Initialize the Step Sequencer module.
    //
    StepSeqInit();
}

//*****************************************************************************
//
// Close the Doxyen group.
//! @}
//
//*****************************************************************************
