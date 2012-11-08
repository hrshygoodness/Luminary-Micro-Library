//*****************************************************************************
//
// stepper.h - Prototypes and definitions for the Stepper motor control API.
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
// This is part of revision 9453 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#ifndef __STEPPER_H__
#define __STEPPER_H__

//*****************************************************************************
//
//! \addtogroup stepper_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! This is a structure used to hold status information about the stepper
//! motor.
//
//*****************************************************************************
typedef struct
{
    //
    //! The current position of the motor in fixed-point 24.8 format.
    //
    long lPosition;

    //
    //! The target position of the motor in fixed-point 24.8 format.
    //
    long lTargetPos;

    //
    //! The status of the motor represented as one of the following
    //! values: MOTOR_STATUS_STOP, MOTOR_STATUS_RUN, MOTOR_STATUS_ACCEL,
    //! or MOTOR_STATUS_DECEL
    //
    unsigned char ucMotorStatus;

    //
    //! The stepping mode of the motor represented as one of the following
    //! values: STEP_MODE_FULL, STEP_MODE_WAVE, STEP_MODE_HALF,
    //! or STEP_MODE_MICRO.
    //
    unsigned char ucStepMode;

    //
    //! The control mode of the motor represented as one of the following
    //! values: CONTROL_MODE_OPENPWM, CONTROL_MODE_CLOSEDPWM or
    //! CONTROL_MODE_CHOP.
    //
    unsigned char ucControlMode;

    //
    //! The PWM frequency setting in Hz.
    //
    unsigned short usPWMFrequency;

    //
    //! The speed of the motor in whole steps per second.
    //
    unsigned short usSpeed;

    //
    //! An array holding the current for each winding in milliamps.
    //! Winding A is at index 0, and Winding B is at index 1.
    //
    unsigned short usCurrent[2];

    //
    //! The flags indicating a fault.  Bit 0 is overcurrent.
    //
    unsigned char ucFaultFlags;

    //
    //! A flag indicating if the motor is enabled.
    //
    unsigned char bEnabled;
}
tStepperStatus;

//*****************************************************************************
//
// Close the Doxyen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Exported variables.
//
//*****************************************************************************
extern tStepperStatus sStepperStatus;

//*****************************************************************************
//
// Prototypes for the exported variables and functions.
//
//*****************************************************************************
void StepperSetMotion(long lPos, unsigned short usSpeed,
                 unsigned short usAccel, unsigned short usDecel);
void StepperSetMotorParms(unsigned short usDriveCurrent,
                     unsigned short usHoldCurrent,
                     unsigned short usBusVoltage,
                     unsigned short usDriveResistance);
void StepperSetPWMFreq(unsigned short usPWMFreq);
void StepperSetFixedOnTime(unsigned short usFixedOnTime);
void StepperSetBlankingTime(unsigned short usBlankOffTime);
void StepperSetDecayMode(unsigned char ucDecayMode);
void StepperSetControlMode(unsigned char ucControlMode);
void StepperSetStepMode(unsigned char ucStepMode);
void StepperSetFaultParms(unsigned short usFaultCurrent);
tStepperStatus *StepperGetMotorStatus(void);
void StepperEnable(void);
void StepperDisable(void);
void StepperEmergencyStop(void);
void StepperResetPosition(long lNewPosition);
void StepperClearFaults(void);
void StepperCompIntHandler(void);
void StepperInit(void);

#endif
