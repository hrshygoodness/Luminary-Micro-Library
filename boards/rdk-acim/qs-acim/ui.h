//*****************************************************************************
//
// ui.h - Prototypes for the user interface.
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

#ifndef __UI_H__
#define __UI_H__

//*****************************************************************************
//
//! \addtogroup ui_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! This structure contains the AC induction motor parameters that are saved to
//! flash.  A copy exists in RAM for use during the execution of the
//! application, which is loaded form flash at startup.  The modified parameter
//! block can also be written back to flash for use on the next power cycle.
//!
//! Note: All parameters exist in the version zero parameter block unless it is
//! explicitly stated otherwise.  If an older parameter block is loaded from
//! flash, the new parameters will get filled in with default values.  When the
//! parameter block is written to flash, it will always be written with the
//! latest parameter block version.
//
//*****************************************************************************
typedef struct
{
    //
    //! The sequence number of this parameter block.  When in RAM, this value
    //! is not used.  When in flash, this value is used to determine the
    //! parameter block with the most recent information.
    //
    unsigned char ucSequenceNum;

    //
    //! The CRC of the parameter block.  When in RAM, this value is not used.
    //! When in flash, this value is used to validate the contents of the
    //! parameter block (to avoid using a partially written parameter block).
    //
    unsigned char ucCRC;

    //
    //! The version of this parameter block.  This can be used to distinguish
    //! saved parameters that correspond to an old version of the parameter
    //! block.
    //
    unsigned char ucVersion;

    //
    //! The minimum width of a PWM pulse, specified in 0.1 us periods.
    //
    unsigned char ucMinPulseWidth;

    //
    //! The dead time between inverting the high and low side of a motor phase,
    //! specified in 20 ns periods.
    //
    unsigned char ucDeadTime;

    //
    //! The rate at which the PWM pulse width is updated, specified in the
    //! number of PWM periods.
    //
    unsigned char ucUpdateRate;

    //
    //! The number of pole pairs in the motor.
    //
    unsigned char ucNumPoles;

    //
    //! The rate of acceleration, specified in Hertz per second.
    //
    unsigned char ucAccel;

    //
    //! The rate of deceleration, specified in Hertz per second.
    //
    unsigned char ucDecel;

    //
    //! The minimum current through the motor drive during operation, specified
    //! in 1/10ths of an ampere.
    //
    unsigned char ucMinCurrent;

    //
    //! The maximum current through the motor drive during operation, specified
    //! in 1/10ths of an ampere.
    //
    unsigned char ucMaxCurrent;

    //
    //! The amount of time to precharge the bootstrap capacitor on the high
    //! side gate drivers, specified in milliseconds.
    //
    unsigned char ucPrechargeTime;

    //
    //! The maximum ambient temperature of the microcontroller, specified in
    //! degrees Celsius.
    //
    unsigned char ucMaxTemperature;

    //
    //! A set of flags, enumerated by FLAG_PWM_FREQUENCY_MASK,
    //! FLAG_MOTOR_TYPE_BIT, FLAG_LOOP_BIT, FLAG_DRIVE_BIT, FLAG_DIR_BIT,
    //! FLAG_ENCODER_BIT, FLAG_VF_RANGE_BIT, FLAG_BUS_COMP_BIT, FLAG_BRAKE_BIT,
    //! and FLAG_DC_BRAKE_BIT.
    //
    unsigned short usFlags;

    //
    //! The number of lines in the (optional) optical encoder.
    //
    unsigned short usNumEncoderLines;

    //
    //! The minimum frequency of the motor drive, specified in 1/10ths of a
    //! Hertz.
    //
    unsigned short usMinFrequency;

    //
    //! The maximum frequency of the motor drive, specified in 1/10ths of a
    //! Hertz.
    //
    unsigned short usMaxFrequency;

    //
    //! The minimum bus voltage during operation, specified in volts.
    //
    unsigned short usMinVBus;

    //
    //! The maximum bus voltage during operation, specified in volts.
    //
    unsigned short usMaxVBus;

    //
    //! The bus voltage at which the braking circuit is engaged, specified in
    //! volts.
    //
    unsigned short usBrakeOnV;

    //
    //! The bus voltage at which the braking circuit is disengaged, specified
    //! in volts.
    //
    unsigned short usBrakeOffV;

    //
    //! The voltage to be applied to the motor when performing DC injection
    //! braking, specified in volts.
    //
    unsigned short usDCBrakeV;

    //
    //! The amount of time to apply DC injection braking, specified in
    //! milliseconds.
    //
    unsigned short usDCBrakeTime;

    //
    //! The DC bus voltage at which the deceleration rate is reduced, specified
    //! in volts.
    //
    unsigned short usDecelV;

    //
    //! An array of coefficients that map from motor frequency to waveform
    //! amplitude, known as V/f control.  The first entry of this array
    //! corresponds to the minimum motor drive frequency, the last entry
    //! corresponds to the nominal motor drive frequency, and the other entries
    //! are equally spaced between the first and last.  For frequencies that do
    //! not appear in this table, linear interpolation is used to approximate
    //! the appropriate amplitude.  Each entry is in a 1.15 fixed point format.
    //
    unsigned short usVFTable[21];

    //
    //! The P coefficient of the frequency adjust PI controller.
    //
    long lFAdjP;

    //
    //! The I coefficient of the frequency adjust PI controller.
    //
    long lFAdjI;

    //
    //! The amount of time (assuming continuous application) that the dynamic
    //! braking can be utilized, specified in milliseconds.
    //
    unsigned long ulBrakeMax;

    //
    //! The amount of accumulated time that the dynamic brake can have before
    //! the cooling period will end, specified in milliseconds.
    //
    unsigned long ulBrakeCool;

    //
    //! The motor current at which the acceleration rate is reduced, specified
    //! in 1/10ths of an ampere.  Note: This parameter only exists in the
    //! version one parameter block.
    //
    unsigned char ucAccelCurrent;

    //
    //! The amount of unused space in the structure in order to pad it to 128
    //! bytes for storage into flash.
    //
    unsigned char ucReserved[31];
}
tDriveParameters;

//*****************************************************************************
//
//! The mask for the bits in the usFlags member of #tDriveParameters that
//! define the PWM output frequency.  This field will be one of
//! #FLAG_PWM_FREQUENCY_8K, #FLAG_PWM_FREQUENCY_12K, #FLAG_PWM_FREQUENCY_16K,
//! or #FLAG_PWM_FREQUENCY_20K.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_MASK 0x00000003

//*****************************************************************************
//
//! The value of the #FLAG_PWM_FREQUENCY_MASK bit field that indicates that the
//! PWM frequency is 8 KHz.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_8K   0x00000000

//*****************************************************************************
//
//! The value of the #FLAG_PWM_FREQUENCY_MASK bit field that indicates that the
//! PWM frequency is 12.5 KHz.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_12K  0x00000001

//*****************************************************************************
//
//! The value of the #FLAG_PWM_FREQUENCY_MASK bit field that indicates that the
//! PWM frequency is 16 KHz.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_16K  0x00000002

//*****************************************************************************
//
//! The value of the #FLAG_PWM_FREQUENCY_MASK bit field that indicates that the
//! PWM frequency is 20 KHz.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_20K  0x00000003

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the type of the motor.  This field will be one of
//! #FLAG_MOTOR_TYPE_3PHASE or #FLAG_MOTOR_TYPE_1PHASE.
//
//*****************************************************************************
#define FLAG_MOTOR_TYPE_BIT     2

//*****************************************************************************
//
//! The value of the #FLAG_MOTOR_TYPE_BIT flag that indicates that the motor is
//! a three phase motor.
//
//*****************************************************************************
#define FLAG_MOTOR_TYPE_3PHASE  0

//*****************************************************************************
//
//! The value of the #FLAG_MOTOR_TYPE_BIT flag that indicates that the motor is
//! a single phase motor.
//
//*****************************************************************************
#define FLAG_MOTOR_TYPE_1PHASE  1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the mode of operation for the motor drive.  This field will be one
//! of #FLAG_LOOP_OPEN or #FLAG_LOOP_CLOSED.
//
//*****************************************************************************
#define FLAG_LOOP_BIT           3

//*****************************************************************************
//
//! The value of the #FLAG_LOOP_BIT flag that indicates that the motor is
//! operated in open-loop mode.
//
//*****************************************************************************
#define FLAG_LOOP_OPEN          0

//*****************************************************************************
//
//! The value of the #FLAG_LOOP_BIT flag field that indicates that the motor is
//! operated in closed-loop mode.
//
//*****************************************************************************
#define FLAG_LOOP_CLOSED        1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the type of drive waveform for the motor drive.  This field will be
//! one of #FLAG_DRIVE_SINE or #FLAG_DRIVE_SPACE_VECTOR.
//
//*****************************************************************************
#define FLAG_DRIVE_BIT          4

//*****************************************************************************
//
//! The value of the #FLAG_DRIVE_BIT flag that indicates that the motor is to
//! be driven with sine wave modulation.
//
//*****************************************************************************
#define FLAG_DRIVE_SINE         0

//*****************************************************************************
//
//! The value of the #FLAG_DRIVE_BIT flag that indicates that the motor is to
//! be driven with space vector modulation.
//
//*****************************************************************************
#define FLAG_DRIVE_SPACE_VECTOR 1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the direction the motor is to be driven.  The field will be one of
//! #FLAG_DIR_FORWARD or #FLAG_DIR_BACKWARD.
//
//*****************************************************************************
#define FLAG_DIR_BIT            5

//*****************************************************************************
//
//! The value of the #FLAG_DIR_BIT flag that indicates that the motor is to be
//! driven in the forward direction.
//
//*****************************************************************************
#define FLAG_DIR_FORWARD        0

//*****************************************************************************
//
//! The value of the #FLAG_DIR_BIT flag that indicates that the motor is to be
//! driven in the backward direction.
//
//*****************************************************************************
#define FLAG_DIR_BACKWARD       1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the presence of an encoder for speed feedback.  This field will be
//! one of #FLAG_ENCODER_ABSENT or #FLAG_ENCODER_PRESENT.
//
//*****************************************************************************
#define FLAG_ENCODER_BIT        6

//*****************************************************************************
//
//! The value of the #FLAG_ENCODER_BIT flag that indicates that the encoder is
//! absent.
//
//*****************************************************************************
#define FLAG_ENCODER_ABSENT     0

//*****************************************************************************
//
//! The value of the #FLAG_ENCODER_BIT flag that indicates that the encoder is
//! present.
//
//*****************************************************************************
#define FLAG_ENCODER_PRESENT    1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the range of the V/f table.  This field will be one of
//! #FLAG_VF_RANGE_100 or #FLAG_VF_RANGE_400.
//
//*****************************************************************************
#define FLAG_VF_RANGE_BIT       7

//*****************************************************************************
//
//! The value of the #FLAG_VF_RANGE_BIT flag that indicates that the V/f table
//! ranges from 0 Hz to 100 Hz.
//
//*****************************************************************************
#define FLAG_VF_RANGE_100       0

//*****************************************************************************
//
//! The value of the #FLAG_VF_RANGE_BIT flag that indicates that the V/f table
//! ranges from 0 Hz to 400 Hz.
//
//*****************************************************************************
#define FLAG_VF_RANGE_400       1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the application of amplitude compensation for fluctuations in the
//! DC bus voltage.  This field will be one of #FLAG_BUS_COMP_ON or
//! #FLAG_BUS_COMP_OFF.
//
//*****************************************************************************
#define FLAG_BUS_COMP_BIT       8

//*****************************************************************************
//
//! The value of the #FLAG_BUS_COMP_BIT flag that indicates that the DC bus
//! compensation is disabled.
//
//*****************************************************************************
#define FLAG_BUS_COMP_OFF       0

//*****************************************************************************
//
//! The value of the #FLAG_BUS_COMP_BIT flag that indicates that the DC bus
//! compensation is enabled.
//
//*****************************************************************************
#define FLAG_BUS_COMP_ON        1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the application of dynamic brake to handle regeneration onto DC
//! bus.  This field will be one of #FLAG_BRAKE_ON or #FLAG_BRAKE_OFF.
//
//*****************************************************************************
#define FLAG_BRAKE_BIT          9

//*****************************************************************************
//
//! The value of the #FLAG_BRAKE_BIT flag that indicates that the dynamic brake
//! is disabled.
//
//*****************************************************************************
#define FLAG_BRAKE_OFF          0

//*****************************************************************************
//
//! The value of the #FLAG_BRAKE_BIT flag that indicates that the dynamic brake
//! is enabled.
//
//*****************************************************************************
#define FLAG_BRAKE_ON           1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the application of the DC injection brake to stop the motor.  This
//! field will be one of #FLAG_DC_BRAKE_ON or #FLAG_DC_BRAKE_OFF.
//
//*****************************************************************************
#define FLAG_DC_BRAKE_BIT      10

//*****************************************************************************
//
//! The value of the #FLAG_DC_BRAKE_BIT flag that indicates that the DC
//! injection brake is disabled.
//
//*****************************************************************************
#define FLAG_DC_BRAKE_OFF       0

//*****************************************************************************
//
//! The value of the #FLAG_DC_BRAKE_BIT flag that indicates that the DC
//! injection brake is enabled.
//
//*****************************************************************************
#define FLAG_DC_BRAKE_ON        1

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Prototypes for the globals exported from the user interface.
//
//*****************************************************************************
extern tDriveParameters g_sParameters;
extern unsigned short g_usCurrentFrequency;
extern unsigned short g_usTargetFrequency;
extern unsigned long g_ulCPUUsage;
extern void GPIODIntHandler(void);
extern void UIRunLEDBlink(unsigned short usRate, unsigned short usPeriod);
extern void UIFaultLEDBlink(unsigned short usRate, unsigned short usPeriod);
extern void UIStatus1LEDBlink(unsigned short usRate, unsigned short usPeriod);
extern void UIStatus2LEDBlink(unsigned short usRate, unsigned short usPeriod);
extern void SysTickIntHandler(void);
extern void UIInit(void);

#endif // __UI_H__
