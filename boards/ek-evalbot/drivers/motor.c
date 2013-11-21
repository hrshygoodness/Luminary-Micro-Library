//*****************************************************************************
//
// motor.c - Functions related to running the EVALBOT motors.
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup motor_api
//! @{
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "motor.h"

//*****************************************************************************
//
// Defines the PWM period in ticks.
//
//*****************************************************************************
#define PWM_PERIOD (ROM_SysCtlClockGet() / 16000)

//*****************************************************************************
//
//! Initializes peripherals used to control the two EVALBOT motors.
//!
//! This function must be called before any other API in this file.  It
//! initializes the GPIO pins and PWMs used to drive the two motors on the
//! EVALBOT.
//!
//! \return None.
//
//*****************************************************************************
void
MotorsInit (void)
{
    //
    // Enable the PWM controller and set its clock rate.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    //
    // Enable the GPIO ports used by the motor.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);

    //
    // Set up the pin muxing for the PWM pins
    //
    GPIOPinConfigure(GPIO_PD0_PWM0);
    GPIOPinConfigure(GPIO_PH0_PWM2);

    //
    // Configure the PWM0 generator
    //
    ROM_PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
                        PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    ROM_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_PERIOD);

    //
    // Configure the PWM1 generator
    //
    ROM_PWMGenConfigure(PWM0_BASE, PWM_GEN_1,
                        PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    ROM_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWM_PERIOD);


    //
    // Configure the pulse widths for each PWM signal to initially 0%
    //
    ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0);
    ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);

    //
    // Initially disable the the PWM0 and PWM2 output signals.
    //
    ROM_PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT | PWM_OUT_2_BIT, false);

    //
    // Enable the PWM generators.
    //
    ROM_PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    ROM_PWMGenEnable(PWM0_BASE, PWM_GEN_1);

    //
    // Set the pins connected to the motor driver fault signal to input with
    // pull ups.
    //
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_3);
    ROM_GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);

    //
    // Enable slow decay mode.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_2);
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);

    //
    // Initially configure the direction control and enable pins as GPIO and
    // set low.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);
    ROM_GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);

    //
    // Enable the 12V boost
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_5);
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_5, GPIO_PIN_5);
}

//*****************************************************************************
//
//! Configures the DMOS Motor Driver to drive the motor in the required
//! direction.
//!
//! \param ucMotor determines which motor's direction should be set.  Valid
//! values are \e LEFT_SIDE or \e RIGHT_SIDE.
//! \param eDirection sets the motor direction.  Valid values are \e FORWARD
//! or \e REVERSE
//!
//! This function may be used to set the drive direction for one of the motors.
//!
//! \return None.
//
//*****************************************************************************
void
MotorDir (tSide ucMotor, tDirection eDirection)
{
    //
    // Check for invalid parameters.
    //
    ASSERT((ucMotor == LEFT_SIDE) || (ucMotor == RIGHT_SIDE));
    ASSERT((eDirection == FORWARD) || (eDirection == REVERSE));

    //
    // Which motor are we setting?
    //
    if(ucMotor == LEFT_SIDE)
    {
        //
        // Set the left side GPIO direction pin.
        //
        if(eDirection == FORWARD)
        {
            ROM_GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_1 , 0);
        }
        else
        {
            ROM_GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_1 , GPIO_PIN_1);
        }
    }
    else
    {
        //
        // Set the right side GPIO direction pin.
        //
        if(eDirection == FORWARD)
        {
            ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1 , GPIO_PIN_1);
        }
        else
        {
            ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1 , 0);
        }
    }
}

//*****************************************************************************
//
//! Starts the motor.
//!
//! \param ucMotor determines which motor should be started.  Valid values are
//! \e LEFT_SIDE or \e RIGHT_SIDE.
//!
//! This function will start either the right or left motor depending upon the
//! value of the \e ucMotor parameter.  The motor duty cycle will be the last
//! value passed to the MotorSpeed() function for this motor.
//!
//! \return None.
//!
//*****************************************************************************
void
MotorRun (tSide ucMotor)
{
    unsigned long ulPort;

    //
    // Check for invalid parameters.
    //
    ASSERT((ucMotor == LEFT_SIDE) || (ucMotor == RIGHT_SIDE));

    //
    // Select the correct GPIO port for the motor.
    //
    ulPort = (ucMotor == LEFT_SIDE) ? GPIO_PORTH_BASE : GPIO_PORTD_BASE;

    //
    // Configure the pin to be controlled by the PWM module.  This enables
    // the PWM signal onto the pin, which causes the motor to start running.
    //
    ROM_GPIOPinTypePWM(ulPort, GPIO_PIN_0);
}

//*****************************************************************************
//
//! Stops the motor.
//!
//! \param ucMotor determines which motor should be stopped.  Valid values are
//! \e LEFT_SIDE or \e RIGHT_SIDE.
//!
//! This function will stop either the right or left motor depending upon the
//! value of the \e ucMotor parameter.
//!
//! \return None.
//!
//*****************************************************************************
void
MotorStop (tSide ucMotor)
{
    unsigned long ulPort;

    //
    // Check for invalid parameters.
    //
    ASSERT((ucMotor == LEFT_SIDE) || (ucMotor == RIGHT_SIDE));

    //
    // Select the correct GPIO port for the motor.
    //
    ulPort = (ucMotor == LEFT_SIDE) ? GPIO_PORTH_BASE : GPIO_PORTD_BASE;

    //
    // Configure the pin to be a software controlled GPIO output.  This stops
    // the PWM generator from controlling this pin.  This causes the motor
    // to stop running.
    //
    ROM_GPIOPinTypeGPIOOutput(ulPort, GPIO_PIN_0);

    //
    // Set the pin low.
    //
    ROM_GPIOPinWrite(ulPort, GPIO_PIN_0, 0);
}

//*****************************************************************************
//
//! Sets the motor to be driven at the requested duty cycle.
//!
//! \param ucMotor determines which motor's duty cycle is to be set. Valid
//! values are \e LEFT_SIDE or \e RIGHT_SIDE.
//! \param usPercent Percent of the maximum speed to drive the motor in
//! 8.8 fixed point format.  This value must be less than (100 << 8).
//!
//! This function can be called to set the duty cycle of one or other of the
//! motors.
//!
//! \return None.
//!
//! \note The duty cycle and motor speed are not the same thing, although there
//! is a relation.
//
//*****************************************************************************
void
MotorSpeed(tSide ucMotor, unsigned short usPercent)
{
    unsigned long ulPWMOut, ulPWMOutBit;

    //
    // Check for invalid parameters.
    //
    ASSERT((ucMotor == LEFT_SIDE) || (ucMotor == RIGHT_SIDE));
    ASSERT(usPercent < (100 << 8));

    //
    // Which PWM output are we controlling?
    //
    if(ucMotor == LEFT_SIDE)
    {
        ulPWMOut = PWM_OUT_2;
        ulPWMOutBit = PWM_OUT_2_BIT;
    }
    else
    {
        ulPWMOut = PWM_OUT_0;
        ulPWMOutBit = PWM_OUT_0_BIT;
    }

    //
    // First, enable the PWM output in case it was disabled by the
    // previously requested speed being greater than 95%
    //
    ROM_PWMOutputState(PWM0_BASE, ulPWMOutBit, true);

    //
    // Make sure that output is not inverted.
    //
    ROM_PWMOutputInvert(PWM0_BASE, ulPWMOutBit, false);

    //
    // Set the pulse width to the requested value. Divide by two since
    // we are using 6V motors with 12V power rail.
    //
    ROM_PWMPulseWidthSet(PWM0_BASE, ulPWMOut, ((PWM_PERIOD * usPercent) /
                         (100 << 8)));
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
