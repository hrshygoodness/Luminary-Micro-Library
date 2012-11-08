//*****************************************************************************
//
// uiparms.h - Parameters needed for the user interface.
//
// Copyright (c) 2006-2012 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __UIPARMS_H__
#define __UIPARMS_H__

//*****************************************************************************
//
//! \addtogroup uiparms_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! This structure contains the stepper motor parameters that are saved
//! to flash. A copy exists in RAM for use during the execution of the
//! application, which is loaded from flash at startup. The modified parameter
//! block can also be written back to flash for use on the next power cycle.
//
//*****************************************************************************
typedef struct
{
    //
    //! The sequence number of this parameter block. When in RAM, this value
    //! is not used. When in flash, this value is used to determine the
    //! parameter block with the most recent information.
    //
    unsigned char ucSequenceNum;

    //
    //! The CRC of the parameter block. When in RAM, this value is not used.
    //! When in flash, this value is used to validate the contents of the
    //! parameter block (to avoid using a partially written parameter block).
    //
    unsigned char ucCRC;

    //
    //! The stepper control mode: CONTROL_MODE_PWM or CONTROL_MODE_CHOP.
    //
    unsigned char ucControlMode;

    //
    //! The current decay mode: DECAY_MODE_SLOW or DECAY_MODE_FAST.
    //
    unsigned char ucDecayMode;

    //
    //! The stepping mode: STEP_MODE_FULL, STEP_MODE_WAVE,
    //! STEP_MODE_HALF or STEP_MODE_MICRO.
    //
    unsigned char ucStepMode;

    //
    //! The running speed in steps/sec.
    //
    unsigned short usSpeed;

    //
    //! The acceleration in steps/sec**2.
    //
    unsigned short usAccel;

    //
    //! The deceleration in steps/sec**2.
    //
    unsigned short usDecel;

    //
    //! The "on" interval for fixed rise time in microseconds.
    //
    unsigned short usFixedOnTime;

    //
    //! The PWM frequency, in Hz.
    //
    unsigned short usPWMFrequency;

    //
    //! The "off" blanking interval for chopper mode in microseconds.
    //
    unsigned short usBlankOffTime;

    //
    //! The driving current in milliamps.
    //
    unsigned short usDriveCurrent;

    //
    //! The holding current in milliamps.
    //
    unsigned short usHoldCurrent;

    //
    //! The maximum faulting current in milliamps.
    //
    unsigned short usMaxCurrent;

    //
    //! The motor winding resistance in milliohms.
    //
    unsigned short usResistance;
}
tDriveParameters;

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
//! These are globals that are exported parameters needed by the user
//! interface module.
//
//*****************************************************************************
extern tDriveParameters g_sParameters;
extern long g_lTargetPos;
extern unsigned char g_ucCPUUsage;
extern unsigned char g_bUIUseOnboard;
extern unsigned short g_usBusVoltage;
extern unsigned short g_usMotorCurrent;
extern short g_sAmbientTemp;

#endif
