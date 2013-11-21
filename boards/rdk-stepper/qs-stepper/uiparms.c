//*****************************************************************************
//
// uiparms.c - Contains the parameters needed for the user interface module.
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

#include <limits.h>
#include "commands.h"
#include "stepcfg.h"
#include "stepper.h"
#include "ui.h"
#include "uiparms.h"
#include "ui_common.h"
#include "ui_onboard.h"
#include "ui_serial.h"

//*****************************************************************************
//
//! \page uiparms_intro Introduction
//!
//! This module contains the parameters that are maintained and updated
//! by the User Interface Module.
//!
//! The code for implementing UI Parameters is contained in
//! <tt>uiparms.c</tt>, with <tt>uiparms.h</tt> containing the definitions
//! for the variables and functions exported to the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup uiparms_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The target type for this drive. This is used by the serial user interface
//! module.
//
//*****************************************************************************
const unsigned long g_ulUITargetType = RESP_ID_TARGET_STEPPER;

//*****************************************************************************
//
//! The version of the firmware. Changing this value will make it more
//! difficult for Texas Instruments support personnel to determine the firmware
//! in use when trying to provide assistance; it should be changed only after
//! careful consideration.
//
//*****************************************************************************
const unsigned short g_usFirmwareVersion = 10636;

//*****************************************************************************
//
//! The target position for the motor motion that the UI is requesting.
//
//*****************************************************************************
long g_lTargetPos = 0;

//*****************************************************************************
//
//! A flag to indicate if the on-board user interface should be used.
//! This is normally disabled if the serial interface is used.
//
//*****************************************************************************
unsigned char g_bUIUseOnboard = 1;

//*****************************************************************************
//
//! The CPU usage in percent.
//
//*****************************************************************************
unsigned char g_ucCPUUsage = 0;

//*****************************************************************************
//
//! The bus voltage that was measured by the ADC in millivolts.
//
//*****************************************************************************
unsigned short g_usBusVoltage = 60000;

//*****************************************************************************
//
//! The peak current of the two motor windings averaged together. This is
//! the peak winding current when the chopper mode is used. The units
//! are in milliamps.
//
//*****************************************************************************
unsigned short g_usMotorCurrent;

//*****************************************************************************
//
//! The internal temperature of the microcontroller in deg C.
//
//*****************************************************************************
short g_sAmbientTemp;

//*****************************************************************************
//
//! This structure instance contains the configuration values for the
//! stepper motor drive. This is where the initial default values for
//! all the parameters are set. However, the default values can be changed
//! by saving an update to flash memory.
//
//*****************************************************************************
tDriveParameters g_sParameters =
{
    //
    // The sequence number (ucSequenceNum); this value is not important for
    // the copy in SRAM.
    //
    0,

    //
    // The CRC (ucCRC); this value is not important for the copy in SRAM.
    //
    0,

    //
    // The control mode (ucControlMode)
    //
    CONTROL_MODE_CHOP,

    //
    // The decay mode (ucDecayMode)
    //
    DECAY_MODE_SLOW,

    //
    // The stepping mode (ucStepMode).
    //
    STEP_MODE_HALF,

    //
    // The running speed (usSpeed).
    //
    200,

    //
    // The acceleration (usAccel).
    //
    30000,

    //
    // The deceleration (usDecel).
    //
    60000,

    //
    // The fixed on time (usFixedOnTime).
    //
    500,

    //
    // The PWM frequency (usPWMFrequency).
    //
    20000,

    //
    // The off blanking time (usBlankOffTime).
    //
    100,

    //
    // The drive current (usDriveCurrent).
    //
    1500,

    //
    // The hold current (usHoldCurrent).
    //
    0,

    //
    // The maximum current (usMaxCurrent).
    //
    6000,

    //
    // The winding resistance (usResistance).
    //
    750
};

//*****************************************************************************
//
//! An array of structures describing the stepper motor drive parameters
//! to the serial user interface module. This table contains all of the
//! parameters including the size, limits, and resolution for each.
//
//*****************************************************************************
const tUIParameter g_sUIParameters[] =
{
    //
    // The firmware version.
    //
    {
        PARAM_FIRMWARE_VERSION,
        2,
        0,
        0,
        0,
        (unsigned char *)&g_usFirmwareVersion,
        0
    },

    //
    // The target position, in 1/256 of a step (fixed-point 24.8 format).
    //
    {
        PARAM_TARGET_POS,
        4,
        (unsigned long)INT_MIN,
        (unsigned long)INT_MAX,
        256,
        (unsigned char *)&g_lTargetPos,
        UISetMotion
    },

    //
    // The target speed, in steps/sec.
    //
    {
        PARAM_TARGET_SPEED,
        2,
        10,
        10000,
        1,
        (unsigned char *)&(g_sParameters.usSpeed),
        UISetMotion
    },

    //
    // The acceleration rate, in steps/sec^2
    //
    {
        PARAM_ACCEL,
        2,
        100,
        60000,
        1,
        (unsigned char *)&(g_sParameters.usAccel),
        UISetMotion
    },

    //
    // The deceleration rate, in steps/sec^2
    //
    {
        PARAM_DECEL,
        2,
        100,
        60000,
        1,
        (unsigned char *)&(g_sParameters.usDecel),
        UISetMotion
    },

    //
    // The actual position (as a read-only), in 1/256 of a step.
    //
    {
        PARAM_CURRENT_POS,
        4,
        (unsigned long)INT_MIN,
        (unsigned long)INT_MAX,
        0,
        (unsigned char *)&sStepperStatus.lPosition,
        0
    },

    //
    // The motor speed, in steps/sec, read-only.
    //
    {
        PARAM_CURRENT_SPEED,
        2,
        0,
        0xffff,
        0,
        (unsigned char *)&sStepperStatus.usSpeed,
        0
    },

    //
    // The control mode. 0 is open-loop PWM, 1 is chopper,
    // 2 is closed-loop PWM.
    //
    {
        PARAM_CONTROL_MODE,
        1,
        0,
        2,
        1,
        &(g_sParameters.ucControlMode),
        UISetControlMode
    },

    //
    // The current decay mode. 0 is fast, 1 is slow.
    //
    {
        PARAM_DECAY_MODE,
        1,
        0,
        1,
        1,
        &(g_sParameters.ucDecayMode),
        UISetDecayMode
    },

    //
    // The stepping mode. 0 is full, 1 is half,
    // 2 is micro, and 3 is wave.
    //
    {
        PARAM_STEP_MODE,
        1,
        0,
        3,
        1,
        &(g_sParameters.ucStepMode),
        UISetStepMode
    },

    //
    // The fixed rise ON time, in microseconds.
    //
    {
        PARAM_FIXED_ON_TIME,
        2,
        0,
        10000,
        1,
        (unsigned char *)&(g_sParameters.usFixedOnTime),
        UISetFixedOnTime
    },

    //
    // The PWM frequency, in Hz.
    //
    {
        PARAM_PWM_FREQUENCY,
        2,
        16000,
        32000,
        1,
        (unsigned char *)&(g_sParameters.usPWMFrequency),
        UISetPWMFreq
    },

    //
    // The chopper mode, OFF blanking interval, in microseconds.
    //
    {
        PARAM_BLANK_OFF,
        2,
        20,
        10000,
        1,
        (unsigned char *)&(g_sParameters.usBlankOffTime),
        UISetChopperBlanking
    },

    //
    // The drive current allowed through the winding, in milliamps.
    // Used for regulating in chopper mode, and for calculating duty
    // cycle in PWM mode.
    //
    {
        PARAM_TARGET_CURRENT,
        2,
        100,
        3000,
        1,
        (unsigned char *)&(g_sParameters.usDriveCurrent),
        UISetMotorParms
    },

    //
    // The holding current, in milliamps.
    //
    {
        PARAM_HOLDING_CURRENT,
        2,
        0,
        3000,
        1,
        (unsigned char *)&(g_sParameters.usHoldCurrent),
        UISetMotorParms
    },

    //
    // The maximum current, before signalling an alarm, in milliamps.
    //
    {
        PARAM_MAX_CURRENT,
        2,
        1000,
        10000,
        1,
        (unsigned char *)&(g_sParameters.usMaxCurrent),
        UISetFaultParms
    },

    //
    // The winding resistance, used for computing PWM duty cycle,
    // in milliohms.
    //
    {
        PARAM_RESISTANCE,
        2,
        100,
        5000,
        1,
        (unsigned char *)&(g_sParameters.usResistance),
        UISetMotorParms
    },

    //
    // The fault status.
    //
    {
        PARAM_FAULT_STATUS,
        1,
        0,
        3,
        1,
        &sStepperStatus.ucFaultFlags,
        UIClearFaults
    },

    //
    // The motor status, read-only.
    //
    {
        PARAM_MOTOR_STATUS,
        1,
        0,
        0,
        0,
        &sStepperStatus.ucMotorStatus,
        0
    },

    //
    // This indicates if the on-board user interface should be utilized. When
    // one, the on-board user interface is active, and when zero it is not.
    //
    {
        PARAM_USE_ONBOARD_UI,
        1,
        0,
        1,
        1,
        (unsigned char *)&g_bUIUseOnboard,
        UIOnBoard
    }
};

//*****************************************************************************
//
//! The number of motor drive parameters. This is used by the serial user
//! interface module.
//
//*****************************************************************************
const unsigned long g_ulUINumParameters = (sizeof(g_sUIParameters) /
                                           sizeof(g_sUIParameters[0]));

//*****************************************************************************
//
//! An array of structures describing the stepper motor drive real-time
//! data items to the serial user interface module.
//
//*****************************************************************************
const tUIRealTimeData g_sUIRealTimeData[] =
{
    //
    // The motor speed in steps/sec. This comes directly from
    // the stepper status.
    //
    {
        DATA_ROTOR_SPEED,
        2,
        (unsigned char *)&sStepperStatus.usSpeed
    },

    //
    // The motor current in milliamps. This is calculated in the
    // UI SysTickHandler based on the raw current data from the stepper.
    //
    {
        DATA_MOTOR_CURRENT,
        2,
        (unsigned char *)&g_usMotorCurrent
    },

    //
    // The bus voltage, in millivolts. This comes from the UI SysTickHandler.
    //
    {
        DATA_BUS_VOLTAGE,
        2,
        (unsigned char *)&g_usBusVoltage
    },

    //
    // The motor position in 1/256 step. This comes directly from the
    // stepper status.
    //
    {
        DATA_MOTOR_POSITION,
        4,
        (unsigned char *)&sStepperStatus.lPosition
    },

    //
    // The motor status. 0 is stop, 1 is run, 2 is accel, 3 is decel.
    // This comes directly from the stepper status.
    //
    {
        DATA_MOTOR_STATUS,
        1,
        &sStepperStatus.ucMotorStatus
    },

    //
    // The processor usage. This is a 8-bit value providing the percentage
    // between 0 and 100. This is maintained in the UI SysTickHandler.
    //
    {
        DATA_PROCESSOR_USAGE,
        1,
        &g_ucCPUUsage
    },

    //
    // The fault status flags. This comes from the stepper status.
    //
    {
        DATA_FAULT_STATUS,
        1,
        &sStepperStatus.ucFaultFlags
    },

    //
    // The ambient temperature of the microcontroller. This is an 8-bit value
    // providing the temperature in C, and is computed in the UI
    // SysTickHandler.
    //
    {
        DATA_TEMPERATURE,
        2,
        (unsigned char *)&g_sAmbientTemp
    }
};

//*****************************************************************************
//
//! The number of motor drive real-time data items. This is used by the serial
//! user interface module.
//
//*****************************************************************************
const unsigned long g_ulUINumRealTimeData = (sizeof(g_sUIRealTimeData) /
                                             sizeof(g_sUIRealTimeData[0]));

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
