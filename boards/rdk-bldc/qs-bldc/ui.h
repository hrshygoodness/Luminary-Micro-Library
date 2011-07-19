//*****************************************************************************
//
// ui.h - Prototypes for the user interface.
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
//! This structure contains the Brushless DC motor parameters that are saved to
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
    //! The motor drive (modulation) scheme in use.
    //
    unsigned char ucModulationType;

    //
    //! The rate of acceleration, specified in RPM per second.
    //
    unsigned short usAccel;

    //
    //! The rate of deceleration, specified in RPM per second.
    //
    unsigned short usDecel;

    //
    //! The minimum current through the motor drive during operation, specified
    //! in milli-amperes.
    //
    short sMinCurrent;

    //
    //! The maximum current through the motor drive during operation, specified
    //! in milli-amperes.
    //
    short sMaxCurrent;

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
    //! FLAG_DECAY_BIT, FLAG_DIR_BIT, FLAG_ENCODER_BIT, FLAG_BRAKE_BIT,
    //! FLAG_SENSOR_TYPE_BIT, and FLAG_SENSOR_POLARITY_BIT.
    //
    unsigned short usFlags;

    //
    //! The number of lines in the (optional) optical encoder.
    //
    unsigned short usNumEncoderLines;

    //
    //! The rate of acceleration, specified in milliwatts per second.
    //
    unsigned short usAccelPower;

    //
    //! The minimum speed of the motor drive, specified in RPM.
    //
    unsigned long ulMinSpeed;

    //
    //! The maximum speed of the motor drive, specified in RPM.
    //
    unsigned long ulMaxSpeed;

    //
    //! The minimum bus voltage during operation, specified in millivolts.
    //
    unsigned long ulMinVBus;

    //
    //! The maximum bus voltage during operation, specified in millivolts.
    //
    unsigned long ulMaxVBus;

    //
    //! The bus voltage at which the braking circuit is engaged, specified in
    //! millivolts.
    //
    unsigned long ulBrakeOnV;

    //
    //! The bus voltage at which the braking circuit is disengaged, specified
    //! in millivolts.
    //
    unsigned long ulBrakeOffV;

    //
    //! The DC bus voltage at which the deceleration rate is reduced, specified
    //! in millivolts.
    //
    unsigned long ulDecelV;

    //
    //! The P coefficient of the frequency adjust PID controller.
    //
    long lFAdjP;

    //
    //! The I coefficient of the frequency adjust PID controller.
    //
    long lFAdjI;

    //
    //! The P coefficient of the power adjust PID controller.
    //
    long lPAdjP;

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
    //! in milli-amperes.
    //
    short sAccelCurrent;

    //
    //! The rate of deceleration, specified in milliwatts per second.
    //
    unsigned short usDecelPower;

    //
    //! The Ethernet Connection Timeout, specified in seconds.
    //
    unsigned long ulConnectionTimeout;

    //
    //! The number of PWM periods to skip in a commutation before looking for
    //! the Back EMF zero-crossing event.
    //
    unsigned char ucBEMFSkipCount;

    //
    //! The control mode for the motor drive algorithm.
    //
    unsigned char ucControlType;
    
    //
    //! The Back EMF Threshold Voltage for sensorless startup.
    //
    unsigned short usSensorlessBEMFThresh;

    //
    //! The number of counts (commutations) for startup in sensorless mode.
    //
    unsigned short usStartupCount;

    //
    //! The open-loop sensorless ramp time, specified in milliseconds.
    //
    unsigned short usSensorlessRampTime;

    //
    //! The motor current limit for motor operation.
    //
   short sTargetCurrent;

    //
    //! Padding to ensure consistent parameter block alignment.
    //
    unsigned char ucPad2[2];

    //
    //! The starting voltage for sensorless startup in millivolts.
    //
    unsigned long ulSensorlessStartVoltage;
    
    //
    //! The ending voltage for sensorless startup in millivolts.
    //
    unsigned long ulSensorlessEndVoltage;
    
    //
    //! The starting speed for sensorless startup in RPM.
    //
    unsigned long ulSensorlessStartSpeed;

    //
    //! The ending speed for sensorless startup in RPM.
    //
    unsigned long ulSensorlessEndSpeed;

    //
    //! The minimum power setting in milliwatts.
    //
    unsigned long ulMinPower;

    //
    //! The maximum power setting in milliwatts.
    //
    unsigned long ulMaxPower;

    //
    //! The target power setting in milliwatts.
    //
    unsigned long ulTargetPower;

    //
    //! The target speed setting in RPM.
    //
    unsigned long ulTargetSpeed;

    //
    //! The I coefficient of the power adjust PID controller.
    //
    long lPAdjI;
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
#define FLAG_PWM_FREQUENCY_MASK 0x000000C3

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
//! The value of the #FLAG_PWM_FREQUENCY_MASK bit field that indicates that the
//! PWM frequency is 25 KHz.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_25K  0x00000040

//*****************************************************************************
//
//! The value of the #FLAG_PWM_FREQUENCY_MASK bit field that indicates that the
//! PWM frequency is 40 KHz.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_40K  0x00000041

//*****************************************************************************
//
//! The value of the #FLAG_PWM_FREQUENCY_MASK bit field that indicates that the
//! PWM frequency is 50 KHz.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_50K  0x00000042

//*****************************************************************************
//
//! The value of the #FLAG_PWM_FREQUENCY_MASK bit field that indicates that the
//! PWM frequency is 80 KHz.
//
//*****************************************************************************
#define FLAG_PWM_FREQUENCY_80K  0x00000043

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the decay mode for the trapezoid motor drive.  This field will be
//! one of #FLAG_DECAY_FAST or #FLAG_DECAY_SLOW
//
//*****************************************************************************
#define FLAG_DECAY_BIT          2

//*****************************************************************************
//
//! The value of the #FLAG_DECAY_BIT flag that indicates that the motor is to
//! be driven with fast decay in trapezoid mode.
//
//*****************************************************************************
#define FLAG_DECAY_FAST         0

//*****************************************************************************
//
//! The value of the #FLAG_DECAY_BIT flag that indicates that the motor is to
//! be driven with slow decay in trapezoid mode.
//
//*****************************************************************************
#define FLAG_DECAY_SLOW         1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the direction the motor is to be driven.  The field will be one of
//! #FLAG_DIR_FORWARD or #FLAG_DIR_BACKWARD.
//
//*****************************************************************************
#define FLAG_DIR_BIT            4

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
#define FLAG_ENCODER_BIT        5

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
//! defines the application of dynamic brake to handle regeneration onto DC
//! bus.  This field will be one of #FLAG_BRAKE_ON or #FLAG_BRAKE_OFF.
//
//*****************************************************************************
#define FLAG_BRAKE_BIT          8

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
//! defines the type of Hall Effect Sensor(s) for position/speed feedback.
//! This field will be one of #FLAG_SENSOR_TYPE_GPIO or
//! #FLAG_SENSOR_TYPE_LINEAR.
//
//*****************************************************************************
#define FLAG_SENSOR_TYPE_BIT    11

//*****************************************************************************
//
//! The value of the #FLAG_SENSOR_TYPE_BIT flag that indicates that the Hall
//! Effect sensor(s) are digital GPIO inputs.
//
//*****************************************************************************
#define FLAG_SENSOR_TYPE_GPIO   0

//*****************************************************************************
//
//! The value of the #FLAG_SENSOR_TYPE_BIT flag that indicates that the Hall
//! Effect sensor(s) are Analog/Linear ADC inputs.
//
//*****************************************************************************
#define FLAG_SENSOR_TYPE_LINEAR 1

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the polarity of the Hall Effect Sensor(s) inputs.  This field will
//! be one of #FLAG_SENSOR_POLARITY_HIGH or #FLAG_SENSOR_POLARITY_LOW.
//
//*****************************************************************************
#define FLAG_SENSOR_POLARITY_BIT    12

//*****************************************************************************
//
//! The value of the #FLAG_SENSOR_POLARITY_BIT flag that indicates that the
//! Hall Effect sensor(s) are configured as active low.
//
//*****************************************************************************
#define FLAG_SENSOR_POLARITY_LOW    1

//*****************************************************************************
//
//! The value of the #FLAG_SENSOR_POLARITY_BIT flag that indicates that the
//! Hall Effect sensor(s) are configured as active high.
//
//*****************************************************************************
#define FLAG_SENSOR_POLARITY_HIGH   0

//*****************************************************************************
//
//! The bit number of the flag in the usFlags member of #tDriveParameters that
//! defines the spacing of the hall sensors.  This field will be one of 
//! #FLAG_SENSOR_SPACE_120 or #FLAG_SENSOR_SPACE_60.
//
//*****************************************************************************
#define FLAG_SENSOR_SPACE_BIT       13

//*****************************************************************************
//
//! The value of the #FLAG_SENSOR_SPACE_BIT flag that indicates that the
//! Hall Effect sensor(s) are spaced at 120 degrees.
//
//*****************************************************************************
#define FLAG_SENSOR_SPACE_120       0

//*****************************************************************************
//
//! The value of the #FLAG_SENSOR_POLARITY_BIT flag that indicates that the
//! Hall Effect sensor(s) are spaced at 60 degrees.
//
//*****************************************************************************
#define FLAG_SENSOR_SPACE_60        1

//*****************************************************************************
//
//! The value for ucModulationType that indicates that the motor is being
//! driven with trapezoid modulation, using hall sensors.
//
//*****************************************************************************
#define MOD_TYPE_TRAPEZOID          0

//*****************************************************************************
//
//! The value for ucModulationType that indicates that the motor is being
//! driven with trapezoid modulation, in sensorless mode.
//
//*****************************************************************************
#define MOD_TYPE_SENSORLESS         1

//*****************************************************************************
//
//! The value for ucModulationType that indicates that the motor is being
//! driven with sinusoidal modulation, using hall sensors for position
//! sensing.
//
//*****************************************************************************
#define MOD_TYPE_SINE               2

//*****************************************************************************
//
//! The value for ucControlType that indicates that the motor is being
//! driven using speed as the closed loop control target.
//
//*****************************************************************************
#define CONTROL_TYPE_SPEED          0

//*****************************************************************************
//
//! The value for ucControlType that indicates that the motor is being
//! driven using power as the closed loop control target.
//
//*****************************************************************************
#define CONTROL_TYPE_POWER          1

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
extern unsigned long g_ulCPUUsage;
extern unsigned long g_ulDebugInfo[];
extern void GPIODIntHandler(void);
extern void UIRunLEDBlink(unsigned short usRate, unsigned short usPeriod);
extern void UIFaultLEDBlink(unsigned short usRate, unsigned short usPeriod);
extern void SysTickIntHandler(void);
extern void UIInit(void);
extern unsigned long UIGetTicks(void);

#endif // __UI_H__
