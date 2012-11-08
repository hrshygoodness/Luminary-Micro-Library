//*****************************************************************************
//
// ui.c - User interface module.
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
// This is part of revision 9453 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "utils/cpu_usage.h"
#include "utils/flash_pb.h"
#include "adc_ctrl.h"
#include "commands.h"
#include "faults.h"
#include "hall_ctrl.h"
#include "main.h"
#include "pins.h"
#include "pwm_ctrl.h"
#include "ui.h"
#include "ui_common.h"
#include "ui_can.h"
#include "ui_ethernet.h"
#include "ui_onboard.h"

//*****************************************************************************
//
//! \page ui_intro Introduction
//!
//! There are two user interfaces for the the Brushless DC motor application.
//! One uses an push button for basic control of
//! the motor and two LEDs for basic status feedback, and the other uses the
//! Ethernet port to provide complete control of all aspects of the motor drive
//! as well as monitoring of real-time performance data.
//!
//! The on-board user interface consists of a push button and
//! two LEDs.  The push button cycles between run
//! forward, stop, run backward, stop.
//!
//! The ``Run'' LED flashes the entire time the application is running.  The
//! LED is off most of the time if the motor drive is stopped and on most of
//! the time if it is running.  The ``Fault'' LED is normally off but flashes
//! at a fast rate when a fault occurs.
//!
//! A periodic interrupt is used to poll the state of the push button and
//! perform debouncing.
//!
//! The Ethernet user interface is entirely handled by the Ethernet user
//! interface
//! module.  The only thing provided here is the list of parameters and
//! real-time data items, plus a set of helper functions that are required in
//! order to properly set the values of some of the parameters.
//!
//! This user interface (and the accompanying Ethernet and on-board user
//! interface modules) is more complicated and consumes more program space than
//! would typically exist in a real motor drive application.  The added
//! complexity allows a great deal of flexibility to configure and evaluate the
//! motor drive, its capabilities, and adjust it for the target motor.
//!
//! The code for the user interface is contained in <tt>ui.c</tt>, with
//! <tt>ui.h</tt> containing the definitions for the structures, defines,
//! variables, and functions exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup ui_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The rate at which the user interface interrupt occurs.
//
//*****************************************************************************
#define UI_INT_RATE             200
#define UI_TICK_MS              (1000/UI_INT_RATE)
#define UI_TICK_US              (1000000/UI_INT_RATE)
#define UI_TICK_NS              (1000000000/UI_INT_RATE)

//*****************************************************************************
//
//! The rate at which the timer interrupt occurs.
//
//*****************************************************************************
#define TIMER1A_INT_RATE        100
#define TIMER1A_TICK_MS         (1000/TIMER1A_INT_RATE)
#define TIMER1A_TICK_US         (1000000/TIMER1A_INT_RATE)
#define TIMER1A_TICK_NS         (1000000000/TIMER1A_INT_RATE)

//*****************************************************************************
//
// Forward declarations for functions declared within this source file for use
// in the parameter and real-time data structures.
//
//*****************************************************************************
static void UIConnectionTimeout(void);
static void UIEncoderPresent(void);
static void UIControlType(void);
static void UISensorType(void);
static void UISensorPolarity(void);
static void UIModulationType(void);
static void UIDirectionSet(void);
static void UIPWMFrequencySet(void);
static void UIUpdateRate(void);
static void UIFAdjI(void);
static void UIPAdjI(void);
static void UIDynamicBrake(void);
void UIButtonPress(void);
static void UIButtonHold(void);
static void UIDecayMode(void);

//*****************************************************************************
//
//! Debug Information.
//
//*****************************************************************************
unsigned long g_ulDebugInfo[8] = {0, 0, 0, 0, 0, 0, 0, 0};

//*****************************************************************************
//
//! The blink rate of the two LEDs on the board; this is the number of user
//! interface interrupts for an entire blink cycle.  The run LED is the first
//! entry of the array and the fault LED is the second entry of the array.
//
//*****************************************************************************
static unsigned short g_pusBlinkRate[2] =
{
    0, 0
};

//*****************************************************************************
//
//! The blink period of the two LEDs on the board; this is the number of user
//! interface interrupts for which the LED will be turned on.  The run LED is
//! the first entry of the array and the fault LED is the second entry of the
//! array.
//
//*****************************************************************************
static unsigned short g_pusBlinkPeriod[2];

//*****************************************************************************
//
//! The count of user interface interrupts that have occurred.  This is used
//! to determine when to toggle the LEDs that are blinking.
//
//*****************************************************************************
static unsigned long g_ulBlinkCount = 0;

//*****************************************************************************
//
//! This array contains the base address of the GPIO blocks for the two LEDs
//! on the board.
//
//*****************************************************************************
static const unsigned long g_pulLEDBase[2] =
{
    PIN_LEDRUN_PORT,
    PIN_LEDFAULT_PORT
};

//*****************************************************************************
//
//! This array contains the pin numbers of the two LEDs on the board.
//
//*****************************************************************************
static const unsigned char g_pucLEDPin[2] =
{
    PIN_LEDRUN_PIN,
    PIN_LEDFAULT_PIN
};

//*****************************************************************************
//
//! The specification of the encoder presence on the motor.  This variable is
//! used by the serial interface as a staging area before the value gets
//! placed into the flags in the parameter block by UIEncoderPresent().
//
//*****************************************************************************
static unsigned char g_ucEncoder = 0;

//*****************************************************************************
//
//! The specification of the control variable on the motor.  This variable is
//! used by the serial interface as a staging area before the value gets
//! placed into the flags in the parameter block by UIControlType().
//
//*****************************************************************************
static unsigned char g_ucControlType = 0;

//*****************************************************************************
//
//! The specification of the type of sensor presence on the motor.  This
//! variable is used by the serial interface as a staging area before the
//! value gets placed into the flags in the parameter block by
//! UISensorType().
//
//*****************************************************************************
static unsigned char g_ucSensorType = 0;

//*****************************************************************************
//
//! The specification of the polarity of sensor on the motor.  This
//! variable is used by the serial interface as a staging area before the
//! value gets placed into the flags in the parameter block by
//! UISensorPolarity().
//
//*****************************************************************************
static unsigned char g_ucSensorPolarity = 0;

//*****************************************************************************
//
//! The specification of the modulation waveform type for the motor drive.
//! This variable is used by the serial interface as a staging area before the
//! value gets placed into the flags in the parameter block by
//! UIModulationType().
//
//*****************************************************************************
static unsigned char g_ucModulationType = 0;

//*****************************************************************************
//
//! The specification of the motor drive direction.  This variable is used by
//! the serial interface as a staging area before the value gets placed into
//! the flags in the parameter block by UIDirectionSet().
//
//*****************************************************************************
static unsigned char g_ucDirection = 0;

//*****************************************************************************
//
//! The specification of the PWM frequency for the motor drive.  This variable
//! is used by the serial interface as a staging area before the value gets
//! placed into the flags in the parameter block by UIPWMFrequencySet().
//
//*****************************************************************************
static unsigned char g_ucFrequency = 0;

//*****************************************************************************
//
//! The specification of the update rate for the motor drive.  This variable is
//! used by the serial interface as a staging area before the value gets
//! updated in a synchronous manner by UIUpdateRate().
//
//*****************************************************************************
static unsigned char g_ucUpdateRate = 0;

//*****************************************************************************
//
//! The I coefficient of the frequency PI controller.  This variable is used by
//! the serial interface as a staging area before the value gets placed into
//! the parameter block by UIFAdjI().
//
//*****************************************************************************
static long g_lFAdjI = 0;

//*****************************************************************************
//
//! The I coefficient of the power PI controller.  This variable is used by
//! the serial interface as a staging area before the value gets placed into
//! the parameter block by UIPAdjI().
//
//*****************************************************************************
static long g_lPAdjI = 0;

//*****************************************************************************
//
//! A boolean that is true when the on-board user interface should be active
//! and false when it should not be.
//
//*****************************************************************************
static unsigned long g_ulUIUseOnboard = 1;

//*****************************************************************************
//
//! A boolean that is true when dynamic braking should be utilized.  This
//! variable is used by the serial interface as a staging area before the value
//! gets placed into the flags in the parameter block by UIDynamicBrake().
//
//*****************************************************************************
static unsigned char g_ucDynamicBrake = 0;

//*****************************************************************************
//
//! The processor usage for the most recent measurement period.  This is a
//! value between 0 and 100, inclusive.
//
//*****************************************************************************
unsigned char g_ucCPUUsage = 0;

//*****************************************************************************
//
//! A boolean that is true when slow decay mode should be utilized.  This
//! variable is used by the serial interface as a staging area before the value
//! gets placed into the flags in the parameter block by UIDecayMode().
//
//*****************************************************************************
static unsigned char g_ucDecayMode = 1;

//*****************************************************************************
//
//! A 32-bit unsigned value that represents the value of various GPIO signals
//! on the board.  Bit 0 corresponds to CFG0; Bit 1 corresponds to CFG1; Bit
//! 2 correpsonds to CFG2; Bit 8 corresponds to the Encoder A input; Bit 9
//! corresponds to the Encode B input; Bit 10 corresponds to the Encoder
//! Index input.
//
//*****************************************************************************
unsigned long g_ulGPIOData = 0;

//*****************************************************************************
//
//! The Analog Input voltage, specified in millivolts.
//
//*****************************************************************************
static unsigned short g_usAnalogInputVoltage;

//*****************************************************************************
//
//! This structure instance contains the configuration values for the
//! Brushless DC motor drive.
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
    // The parameter block version number (ucVersion).
    //
    5,

    //
    // The minimum pulse width (ucMinPulseWidth).
    //
    25,

    //
    // The PWM dead time (ucDeadTime).
    //
    3,

    //
    // The PWM update rate (ucUpdateRate).
    //
    0,

    //
    // The number of poles (ucNumPoles).
    //
    2,

    //
    // The modulation type (ucModulationType).
    //
    MOD_TYPE_TRAPEZOID,

    //
    // The acceleration rate (usAccel).
    //
    5000,

    //
    // The deceleration rate (usDecel).
    //
    5000,

    //
    // The minimum motor drive current (sMinCurrent).
    //
    0,

    //
    // The maximum motor drive current (sMaxCurrent).
    //
    10000,

    //
    // The precharge time (ucPrechargeTime).
    //
    3,

    //
    // The maximum ambient microcontroller temperature (ucMaxTemperature).
    //
    85,

    //
    // The flags (usFlags).
    //
    (FLAG_PWM_FREQUENCY_20K |
     (FLAG_DIR_FORWARD << FLAG_DIR_BIT) |
     (FLAG_ENCODER_ABSENT << FLAG_ENCODER_BIT) |
     (FLAG_BRAKE_ON << FLAG_BRAKE_BIT) |
     (FLAG_SENSOR_TYPE_GPIO << FLAG_SENSOR_TYPE_BIT) |
     (FLAG_SENSOR_POLARITY_HIGH << FLAG_SENSOR_POLARITY_BIT) |
     (FLAG_SENSOR_SPACE_120 << FLAG_SENSOR_SPACE_BIT)),


    //
    // The number of encoder lines (usNumEncoderLines).
    //
    1000,

    //
    // The power acceleration (usAccelPower).
    //
    1000,

    //
    // The minimum motor speed (ulMinSpeed).
    //
    200,

    //
    // The maximum motor speed (ulMaxSpeed).
    //
    12000,

    //
    // The minimum DC bus voltage (ulMinVBus).
    //
    10000,

    //
    // The maximum DC bus voltage (ulMaxVBus).
    //
    36000,

    //
    // The brake engage voltage (ulBrakeOnV).
    //
    38000,

    //
    // The brake disengage voltage (ulBrakeOffV).
    //
    37000,

    //
    // The DC bus voltage at which the deceleration rate is reduced (ulDecelV).
    //
    36000,

    //
    // The frequency adjust P coefficient (lFAdjP).
    //
    (unsigned long)(2.0 * 65536),

    //
    // The frequency adjust I coefficient (lFAdjI).
    //
    (unsigned long)(0.006 * 65536),

    //
    // The power adjust P coefficient (lPAdjP).
    //
    (unsigned long)(2.0 * 65536),

    //
    // The brake maximum time (ulBrakeMax).
    //
    60 * 1000,

    //
    // The brake cooling time (ulBrakeCool).
    //
    55 * 1000,

    //
    // The motor current at which the acceleration rate is reduced, specified
    // in milli-amperes (sAccelCurrent).
    //
    2000,

    //
    // The power deceleration (usAccelPower).
    //
    1000,

    //
    // The ethernet connection timeout, specified in seconds
    // (ulConnectionTimeout).
    //
    10,

    //
    // The number of PWM periods to skip in a commuation before looking for
    // the Back EMF zero crossing event (ucBEMFSkipCount).
    //
    3,

    //
    // The closed-loop control target type (ucControlType).
    //
    CONTROL_TYPE_SPEED,

    //
    // The Back EMF Threshold Voltage for sensorless startup
    // (usSensorlessBEMFThresh).
    //
    500,

    //
    // The sensorless startup hold time (usStartupCount);
    //
    500,

    //
    // The open-loop sensorless ramp time (usSensorlessRampTime).
    //
    500,

    //
    // The motor current limit for motor operation (sTargetCurrent).
    //
    0,

    //
    // Padding (2 Bytes)
    //
    {0, 0},

    //
    // The starting voltage for sensorless startup (ulSensorlessStartVoltage).
    //
    1200,

    //
    // The ending voltage for sensorless startup (ulSensorlessEndVoltage).
    //
    3600,

    //
    // The starting speed for sensorless startup (ulSensorlessStartSpeed).
    //
    400,

    //
    // The ending speed for sensorless startup (ulSensorlessEndSpeed).
    //
    1500,

    //
    // The minimum motor power (ulMinPower).
    //
    0,

    //
    // The maximum motor power (ulMaxPower).
    //
    100000,

    //
    // The target motor power (ulTargetPower).
    //
    0,

    //
    // The target motor speed (ulTargetSpeed).
    //
    3000,

    //
    // The power adjust I coefficient (lPAdjI).
    //
    (unsigned long)(0.006 * 65536),
};

//*****************************************************************************
//
//! The target type for this drive.  This is used by the user interface
//! module.
//
//*****************************************************************************
const unsigned long g_ulUITargetType = RESP_ID_TARGET_BLDC;

//*****************************************************************************
//
//! The version of the firmware.  Changing this value will make it much more
//! difficult for Texas Instruments support personnel to determine the firmware
//! in use when trying to provide assistance; it should only be changed after
//! careful consideration.
//
//*****************************************************************************
const unsigned short g_usFirmwareVersion = 9453;

//*****************************************************************************
//
//! An array of structures describing the Brushless DC motor drive parameters
//! to the Ethernet user interface module.
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
    // The minimum motor speed.  This is specified in RPM, ranging from 0 to
    // 20000 RPM.
    //
    {
        PARAM_MIN_SPEED,
        4,
        0,
        60000,
        1,
        (unsigned char *)&(g_sParameters.ulMinSpeed),
        0
    },

    //
    // The maximum motor speed.  This is specified in RPM, ranging from 0 to
    // 20000 RPM.
    //
    {
        PARAM_MAX_SPEED,
        4,
        0,
        60000,
        1,
        (unsigned char *)&(g_sParameters.ulMaxSpeed),
        UIFAdjI
    },

    //
    // The target motor speed.  This is specified in RPM, ranging from 0 to
    // 20000 RPM.
    //
    {
        PARAM_TARGET_SPEED,
        4,
        0,
        60000,
        1,
        (unsigned char *)&(g_sParameters.ulTargetSpeed),
        MainSetSpeed
    },

    //
    // The current motor speed.  This is specified in RPM, ranging from 0 to
    // 20000 RPM.  This is a read-only parameter.
    //
    {
        PARAM_CURRENT_SPEED,
        4,
        0,
        60000,
        0,
        (unsigned char *)&g_ulMeasuredSpeed,
        0
    },

    //
    // The acceleration rate for the motor drive.  This is specified in RPM per
    // second, ranging from 1 RPM/sec to 50000 RPM/sec.
    //
    {
        PARAM_ACCEL,
        2,
        1,
        50000,
        1,
        (unsigned char *)&(g_sParameters.usAccel),
        0
    },

    //
    // The deceleration rate for the motor drive.  This is specified in RPM per
    // second, ranging from 1 RPM/sec to 6000 RPM/sec.
    //
    {
        PARAM_DECEL,
        2,
        1,
        50000,
        1,
        (unsigned char *)&(g_sParameters.usDecel),
        0
    },

    //
    // Indication of the presence of an encoder feedback.  This is a boolean
    // that is true when an encoder is present.
    //
    {
        PARAM_ENCODER_PRESENT,
        1,
        0,
        1,
        1,
        &g_ucEncoder,
        UIEncoderPresent
    },

    //
    // Indication of the type of sensor feedback.  This is an unsigned value
    // that will indicate digital 120/digital 60, linear 120, and linear 60.
    //
    {
        PARAM_SENSOR_TYPE,
        1,
        0,
        3,
        1,
        &g_ucSensorType,
        UISensorType
    },

    //
    // Indication of the polarity of sensor feedback.  This is a boolean
    // that is true when the sensors are active high, and false for active
    // low sensors.
    //
    {
        PARAM_SENSOR_POLARITY,
        1,
        0,
        1,
        1,
        &g_ucSensorPolarity,
        UISensorPolarity
    },

    //
    // The type of modulation to be used to drive the motor.  The following
    // values are defined.
    // MOD_TYPE_TRAPEZOID   - 6-point/trapezoid modulation, using Hall sensors
    //                        for position/commutation.
    // MOD_TYPE_SENSORLESS  - 6-point/trapezoid modulation, sensorless, using
    //                        Back EMF for position/commutation.
    // MOD_TYPE_SINE        - Sinusoid modulation, using Hall sensors for
    //                        position.
    //
    {
        PARAM_MODULATION,
        1,
        0,
        2,
        1,
        &g_ucModulationType,
        UIModulationType
    },

    //
    // The direction of motor rotation.  When the value is zero, the motor is
    // driven in the forward direction.  When the value is one, the motor is
    // driven in the backward direction.
    //
    {
        PARAM_DIRECTION,
        1,
        0,
        1,
        1,
        &g_ucDirection,
        UIDirectionSet
    },

    //
    // The PWM frequency to be used.  When this value is zero, the PWM
    // frequency is 8 KHz.  When this value is one, the PWM frequency is
    // 12.5 KHz.  When this value is two, the PWM frequency is 16 KHz.  When
    // this value is three, the PWM frequency is 20 KHz.
    //
    {
        PARAM_PWM_FREQUENCY,
        1,
        0,
        7,
        1,
        &g_ucFrequency,
        UIPWMFrequencySet
    },

    //
    // The dead-time between switching off one side of a motor phase (high or
    // low) and turning on the other.  This is specified in 20 nanoseconds
    // units, ranging from 500 ns to 5100 ns.
    //
    {
        PARAM_PWM_DEAD_TIME,
        1,
        2,
        255,
        1,
        &(g_sParameters.ucDeadTime),
        PWMSetDeadBand
    },

    //
    // The rate at which the PWM duty cycles are updated.  This is specified in
    // PWM periods, ranging from 1 to 256.
    //
    {
        PARAM_PWM_UPDATE,
        1,
        0,
        255,
        1,
        &g_ucUpdateRate,
        UIUpdateRate
    },

    //
    // The minimum PWM pulse width.  This is specified in 1/10ths of a
    // microsecond, ranging from 0 us to 25 us.
    //
    {
        PARAM_PWM_MIN_PULSE,
        1,
        0,
        250,
        1,
        &(g_sParameters.ucMinPulseWidth),
        PWMSetMinPulseWidth
    },

    //
    // The number of poles in the motor.  This ranges from 1 to 256 poles.
    //
    {
        PARAM_NUM_POLES,
        1,
        2,
        254,
        2,
        &(g_sParameters.ucNumPoles),
        0
    },

    //
    // The number of lines in the encoder attached to the motor.  This ranges
    // from 1 to 65536 lines.
    //
    {
        PARAM_NUM_LINES,
        2,
        0,
        65535,
        1,
        (unsigned char *)&(g_sParameters.usNumEncoderLines),
        0
    },

    //
    // The minimum allowable drive current during operation.  This is specified
    // in milli-amperes, ranging from 0 to 10A.
    //
    {
        PARAM_MIN_CURRENT,
        2,
        0,
        15000,
        100,
        (unsigned char *)&(g_sParameters.sMinCurrent),
        0
    },

    //
    // The maximum allowable drive current during operation.  This is specified
    // in milli-amperes, ranging from 0 to 10A.
    //
    {
        PARAM_MAX_CURRENT,
        2,
        0,
        15000,
        100,
        (unsigned char *)&(g_sParameters.sMaxCurrent),
        0
    },

    //
    // The target drive current during operation.  This is specified
    // in milli-amperes, ranging from 0 to 10A.
    //
    {
        PARAM_TARGET_CURRENT,
        2,
        0,
        15000,
        100,
        (unsigned char *)&(g_sParameters.sTargetCurrent),
        0
    },

    //
    // The minimum allowable bus voltage during operation.  This is specified
    // in millivolts, ranging from 1 V to 40 V.
    //
    {
        PARAM_MIN_BUS_VOLTAGE,
        4,
        0,
        50000,
        100,
        (unsigned char *)&(g_sParameters.ulMinVBus),
        0
    },

    //
    // The maximum allowable bus voltage during operation.  This is specified
    // in millivolts, ranging from 1 V to 40 V.
    //
    {
        PARAM_MAX_BUS_VOLTAGE,
        4,
        0,
        50000,
        100,
        (unsigned char *)&(g_sParameters.ulMaxVBus),
        0
    },

    //
    // The P coefficient for the frequency adjust PI controller.
    //
    {
        PARAM_SPEED_P,
        4,
        0x80000000,
        0x7fffffff,
        1,
        (unsigned char *)&(g_sParameters.lFAdjP),
        0
    },

    //
    // The I coefficient for the frequency adjust PI controller.
    //
    {
        PARAM_SPEED_I,
        4,
        0x80000000,
        0x7fffffff,
        1,
        (unsigned char *)&g_lFAdjI,
        UIFAdjI
    },

    //
    // The voltage at which the brake circuit is applied.  This is specified in
    // millivolts, ranging from 1 V to 40 V.
    //
    {
        PARAM_BRAKE_ON_VOLTAGE,
        4,
        1000,
        40000,
        100,
        (unsigned char *)&(g_sParameters.ulBrakeOnV),
        0
    },

    //
    // The voltage at which the brake circuit is disengaged.  This is specified
    // in millivolts, ranging from 1 V to 40 V.
    //
    {
        PARAM_BRAKE_OFF_VOLTAGE,
        4,
        1000,
        40000,
        100,
        (unsigned char *)&(g_sParameters.ulBrakeOffV),
        0
    },

    //
    // This indicates if the on-board user interface should be utilized.  When
    // one, the on-board user interface is active, and when zero it is not.
    //
    {
        PARAM_USE_ONBOARD_UI,
        1,
        0,
        1,
        1,
        (unsigned char *)&g_ulUIUseOnboard,
        0
    },

    //
    // The amount of time to precharge the bootstrap capacitor on the high side
    // gate driver before starting the motor drive, specified in milliseconds.
    //
    {
        PARAM_PRECHARGE_TIME,
        1,
        0,
        255,
        1,
        &(g_sParameters.ucPrechargeTime),
        0
    },

    //
    // This indicates if dynamic braking should be utilized.  When one, dynamic
    // braking is active, and when zero it is not.
    //
    {
        PARAM_USE_DYNAM_BRAKE,
        1,
        0,
        1,
        1,
        &g_ucDynamicBrake,
        UIDynamicBrake
    },

    //
    // The maximum amount of time to apply dynamic braking, specified in
    // milliseconds.
    //
    {
        PARAM_MAX_BRAKE_TIME,
        4,
        0,
        60 * 1000,
        1,
        (unsigned char *)&(g_sParameters.ulBrakeMax),
        0
    },

    //
    // The time at which dynamic braking can be reapplied after entering its
    // cooling mode, specified in milliseconds.  Note that the cooling time is
    // the maximum braking time minus this parameter.
    //
    {
        PARAM_BRAKE_COOL_TIME,
        4,
        0,
        60 * 1000,
        1,
        (unsigned char *)&(g_sParameters.ulBrakeCool),
        0
    },

    //
    // The fault status flags.
    //
    {
        PARAM_FAULT_STATUS,
        1,
        0,
        255,
        1,
        (unsigned char *)&g_ulFaultFlags,
        MainClearFaults
    },

    //
    // The motor status.
    //
    {
        PARAM_MOTOR_STATUS,
        1,
        0,
        0,
        0,
        &g_ucMotorStatus,
        0
    },

    //
    // The voltage at which the deceleration rate is reduced.  This is
    // specified in volts, raning from 1 V to 40 V.
    //
    {
        PARAM_DECEL_VOLTAGE,
        4,
        0,
        50000,
        100,
        (unsigned char *)&(g_sParameters.ulDecelV),
        0
    },

    //
    // The maximum allowable ambient temperature.  This is specified in degrees
    // Celsius, ranging from 0 to 85 C.
    //
    {
        PARAM_MAX_TEMPERATURE,
        1,
        0,
        85,
        1,
        &(g_sParameters.ucMaxTemperature),
        0
    },

    //
    // The motor current at which the acceleration rate is reduced.  This is
    // specified in milli-amperes, ranging from 0 A to 10 A.
    //
    {
        PARAM_ACCEL_CURRENT,
        2,
        0,
        15000,
        100,
        (unsigned char *)&(g_sParameters.sAccelCurrent),
        0
    },

    //
    // The current decay mode.
    //
    {
        PARAM_DECAY_MODE,
        1,
        0,
        1,
        1,
        &g_ucDecayMode,
        UIDecayMode,
    },

    //
    // The current value of the GPIO data input(s).
    //
    {
        PARAM_GPIO_DATA,
        4,
        0,
        0xffffffff,
        1,
        (unsigned char *)&g_ulGPIOData,
        0,
    },

    //
    // The current number of packets received on the CAN interface.
    //
    {
        PARAM_CAN_RX_COUNT,
        4,
        0,
        0xffffffff,
        1,
        (unsigned char *)&g_ulCANRXCount,
        0,
    },

    //
    // The current number of packets transmitted on the CAN interface.
    //
    {
        PARAM_CAN_TX_COUNT,
        4,
        0,
        0xffffffff,
        1,
        (unsigned char *)&g_ulCANTXCount,
        0,
    },

    //
    // The current number of packets received on the Ethernet interface.
    //
    {
        PARAM_ETH_RX_COUNT,
        4,
        0,
        0xffffffff,
        1,
        (unsigned char *)&g_ulEthernetRXCount,
        0,
    },

    //
    // The current number of packets transmitted on the Ethernet interface.
    //
    {
        PARAM_ETH_TX_COUNT,
        4,
        0,
        0xffffffff,
        1,
        (unsigned char *)&g_ulEthernetTXCount,
        0,
    },

    //
    // The Ethernet TCP Connection Timeout.
    //
    {
        PARAM_ETH_TCP_TIMEOUT,
        4,
        0,
        0xffffffff,
        1,
        (unsigned char *)&g_sParameters.ulConnectionTimeout,
        UIConnectionTimeout,
    },

    //
    // The skip count for Back EMF zero crossing detection hold-off.
    //
    {
        PARAM_BEMF_SKIP_COUNT,
        1,
        1,
        100,
        1,
        &(g_sParameters.ucBEMFSkipCount),
        0,
    },

    //
    // The startup count for sensorless mode.
    //
    {
        PARAM_STARTUP_COUNT,
        2,
        0,
        0xffff,
        1,
        (unsigned char *)&(g_sParameters.usStartupCount),
        0,
    },

    //
    // The starting voltage for sensorless startup.
    //
    {
        PARAM_STARTUP_STARTV,
        4,
        0,
        50000,
        1,
        (unsigned char *)&(g_sParameters.ulSensorlessStartVoltage),
        0,
    },

    //
    // The ending voltage for sensorless startup.
    //
    {
        PARAM_STARTUP_ENDV,
        4,
        0,
        50000,
        1,
        (unsigned char *)&(g_sParameters.ulSensorlessEndVoltage),
        0,
    },

    //
    // The starting speed for sensorless startup.
    //
    {
        PARAM_STARTUP_STARTSP,
        4,
        0,
        60000,
        1,
        (unsigned char *)&(g_sParameters.ulSensorlessStartSpeed),
        0,
    },

    //
    // The ending speed for sensorless startup.
    //
    {
        PARAM_STARTUP_ENDSP,
        4,
        0,
        60000,
        1,
        (unsigned char *)&(g_sParameters.ulSensorlessEndSpeed),
        MainSetSpeed,
    },

    //
    // The target motor power.  This is specified in milliWatts, ranging from
    // 0 to 360 W.
    //
    {
        PARAM_TARGET_POWER,
        4,
        0,
        360000,
        1,
        (unsigned char *)&(g_sParameters.ulTargetPower),
        MainSetPower
    },
    
    //
    // The minimum motor power.  This is specified in milliwats, ranging from
    // 0 to 360 W.
    //
    {
        PARAM_MIN_POWER,
        4,
        0,
        360000,
        1,
        (unsigned char *)&(g_sParameters.ulMinPower),
        0
    },

    //
    // The maximum motor power.  This is specified in milliwats ranging from
    // 0 to 360 W.
    //
    {
        PARAM_MAX_POWER,
        4,
        0,
        360000,
        1,
        (unsigned char *)&(g_sParameters.ulMaxPower),
        UIPAdjI
    },

    //
    // The P coefficient for the power adjust PI controller.
    //
    {
        PARAM_POWER_P,
        4,
        0x80000000,
        0x7fffffff,
        1,
        (unsigned char *)&(g_sParameters.lPAdjP),
        0
    },

    //
    // The I coefficient for the power adjust PI controller.
    //
    {
        PARAM_POWER_I,
        4,
        0x80000000,
        0x7fffffff,
        1,
        (unsigned char *)&g_lPAdjI,
        UIPAdjI
    },

    //
    // The power acceleration rate for the motor drive.  This is specified in
    // milliwatts per second, ranging from 1 mW/sec to 50000 mW/sec.
    //
    {
        PARAM_ACCEL_POWER,
        2,
        1,
        50000,
        1,
        (unsigned char *)&(g_sParameters.usAccelPower),
        0
    },

    //
    // The power deceleration rate for the motor drive.  This is specified in
    // milliwatts per second, ranging from 1 mW/sec to 50000 mW/sec.
    //
    {
        PARAM_DECEL_POWER,
        2,
        1,
        50000,
        1,
        (unsigned char *)&(g_sParameters.usDecelPower),
        0
    },

    //
    // The control mode for the motor (speed/power).
    //
    {
        PARAM_CONTROL_MODE,
        1,
        0,
        1,
        1,
        &g_ucControlType,
        UIControlType
    },

    //
    // The ending speed for sensorless startup.
    //
    {
        PARAM_STARTUP_RAMP,
        2,
        0,
        0xffff,
        1,
        (unsigned char *)&(g_sParameters.usSensorlessRampTime),
        0,
    },

    //
    // The Back EMF Threshold Voltage for sensorless startup, specified in
    // millivolts.
    //
    {
        PARAM_STARTUP_THRESH,
        2,
        0,
        0xffff,
        1,
        (unsigned char *)&(g_sParameters.usSensorlessBEMFThresh),
        0,
    },
};

//*****************************************************************************
//
//! The number of motor drive parameters.  This is used by the user
//! interface module.
//
//*****************************************************************************
const unsigned long g_ulUINumParameters = (sizeof(g_sUIParameters) /
                                           sizeof(g_sUIParameters[0]));

//*****************************************************************************
//
//! An array of structures describing the Brushless DC motor drive real-time
//! data items to the serial user interface module.
//
//*****************************************************************************
const tUIRealTimeData g_sUIRealTimeData[] =
{
    //
    // The current through phase A of the motor.  This is a signed 16-bit
    // value providing the current in milli-amperes.
    //
    {
        DATA_PHASE_A_CURRENT,
        2,
        (unsigned char *)&(g_psPhaseCurrent[0])
    },

    //
    // The current through phase B of the motor.  This is a signed 16-bit
    // value providing the current in milli-amperes.
    //
    {
        DATA_PHASE_B_CURRENT,
        2,
        (unsigned char *)&(g_psPhaseCurrent[1])
    },

    //
    // The current through phase C of the motor.  This is a signed 16-bit
    // value providing the current in milli-amperes.
    //
    {
        DATA_PHASE_C_CURRENT,
        2,
        (unsigned char *)&(g_psPhaseCurrent[2])
    },

    //
    // The current through the entire motor.  This is a signed 16-bit
    // value providing the current in milli-amperes.
    //
    {
        DATA_MOTOR_CURRENT,
        2,
        (unsigned char *)&g_sMotorCurrent
    },

    //
    // The voltage of the DC bus.  This is a 32-bit value providing the voltage
    // in milli-volts.
    //
    {
        DATA_BUS_VOLTAGE,
        4,
        (unsigned char *)&g_ulBusVoltage
    },

    //
    // The frequency of the rotor.  This is a 16-bit value providing the
    // motor speed in RPM.
    //
    {
        DATA_ROTOR_SPEED,
        4,
        (unsigned char *)&g_ulMeasuredSpeed
    },

    //
    // The processor usage.  This is an 8-bit value providing the percentage
    // between 0 and 100.
    //
    {
        DATA_PROCESSOR_USAGE,
        1,
        &g_ucCPUUsage
    },

    //
    // The state of the motor drive.
    //
    {
        DATA_MOTOR_STATUS,
        1,
        &g_ucMotorStatus
    },

    //
    // The fault status flags.
    //
    {
        DATA_FAULT_STATUS,
        1,
        (unsigned char *)&g_ulFaultFlags
    },

    //
    // The ambient temperature of the microcontroller.  This is an 8-bit value
    // providing the temperature in Celsius.
    //
    {
        DATA_TEMPERATURE,
        2,
        (unsigned char *)&g_sAmbientTemp
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_ANALOG_INPUT,
        2,
        (unsigned char *)&g_usAnalogInputVoltage
    },

    //
    // The average power being consumed by the motor.  This is a 32-bit value
    // in milli-watts.
    //
    {
        DATA_MOTOR_POWER,
        4,
        (unsigned char *)&g_ulMotorPower
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_DEBUG_INFO,
        4,
        (unsigned char *)&g_ulDebugInfo[0]
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_DEBUG_INFO,
        4,
        (unsigned char *)&g_ulDebugInfo[1]
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_DEBUG_INFO,
        4,
        (unsigned char *)&g_ulDebugInfo[2]
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_DEBUG_INFO,
        4,
        (unsigned char *)&g_ulDebugInfo[3]
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_DEBUG_INFO,
        4,
        (unsigned char *)&g_ulDebugInfo[4]
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_DEBUG_INFO,
        4,
        (unsigned char *)&g_ulDebugInfo[5]
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_DEBUG_INFO,
        4,
        (unsigned char *)&g_ulDebugInfo[6]
    },

    //
    // The analog input voltage.  This is a 16-bit value providing the analog
    // input voltage in milli-volts.
    //
    {
        DATA_DEBUG_INFO,
        4,
        (unsigned char *)&g_ulDebugInfo[7]
    },
};

//*****************************************************************************
//
//! The number of motor drive real-time data items.  This is used by the serial
//! user interface module.
//
//*****************************************************************************
const unsigned long g_ulUINumRealTimeData = (sizeof(g_sUIRealTimeData) /
                                             sizeof(g_sUIRealTimeData[0]));

//*****************************************************************************
//
//! An array of structures describing the on-board switches.
//
//*****************************************************************************
const tUIOnboardSwitch g_sUISwitches[] =
{
    //
    // The run/stop/mode button.  Pressing the button will cycle between
    // stopped and running, and holding the switch for five seconds will toggle
    // between sine wave and space vector modulation.
    //
    {
        PIN_SWITCH_PIN_BIT,
        UI_INT_RATE * 5,
        UIButtonPress,
        0,
        UIButtonHold
    }
};

//*****************************************************************************
//
//! The number of switches in the g_sUISwitches array.  This value is
//! automatically computed based on the number of entries in the array.
//
//*****************************************************************************
#define NUM_SWITCHES            (sizeof(g_sUISwitches) /   \
                                 sizeof(g_sUISwitches[0]))

//*****************************************************************************
//
//! The number of switches on this target.  This value is used by the on-board
//! user interface module.
//
//*****************************************************************************
const unsigned long g_ulUINumButtons = NUM_SWITCHES;

//*****************************************************************************
//
//! This is the count of the number of samples during which the switches have
//! been pressed; it is used to distinguish a switch press from a switch
//! hold.  This array is used by the on-board user interface module.
//
//*****************************************************************************
unsigned long g_pulUIHoldCount[NUM_SWITCHES];

//*****************************************************************************
//
//! This is the board id, read once from the configuration switches at startup.
//
//*****************************************************************************
unsigned char g_ucBoardID = 0;

//*****************************************************************************
//
//! The running count of system clock ticks.
//
//*****************************************************************************
static unsigned long g_ulUITickCount = 0;

//*****************************************************************************
//
//! Updates the Ethernet TCP connection timeout.
//!
//! This function is called when the variable controlling the presence of an
//! encoder is updated.  The value is then reflected into the usFlags member
//! of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIConnectionTimeout(void)
{
    //
    // Update the encoder presence flag in the flags variable.
    //
    g_ulConnectionTimeoutParameter = g_sParameters.ulConnectionTimeout;
}

//*****************************************************************************
//
//! Updates the encoder presence bit of the motor drive.
//!
//! This function is called when the variable controlling the presence of an
//! encoder is updated.  The value is then reflected into the usFlags member
//! of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIEncoderPresent(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // Not allowed to change encoder settings while the motor is
        // running.
        //
        g_ucEncoder =  HWREGBITH(&(g_sParameters.usFlags), FLAG_ENCODER_BIT);

        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // Update the encoder presence flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_ENCODER_BIT) = g_ucEncoder;
}

//*****************************************************************************
//
//! Updates the control mode bit for the motor dive.
//!
//! This function is called when the variable controlling the motor control
//! variable (speed/power) is updated.  The value is then reflected into the
//! usFlags member of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIControlType(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // Not allowed to change control type while motor is running.
        //
        g_ucControlType = g_sParameters.ucControlType;

        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // Update the encoder presence flag in the flags variable.
    //
    g_sParameters.ucControlType = g_ucControlType;
}

//*****************************************************************************
//
//! Updates the sensor type bit of the motor drive.
//!
//! This function is called when the variable controlling the type of sensor
//! is updated.  The value is then reflected into the usFlags member of
//! #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UISensorType(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // Not allowed to change sensor type while motor is running.
        //
        g_ucSensorType = HWREGBITH(&(g_sParameters.usFlags),
                                   FLAG_SENSOR_TYPE_BIT);

        g_ucSensorType |= (HWREGBITH(&(g_sParameters.usFlags),
                                    FLAG_SENSOR_SPACE_BIT) << 1);
        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // Update the sensor type flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_SENSOR_TYPE_BIT) =
        (g_ucSensorType & 0x01);

    //
    // Update the sensor spacingb flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_SENSOR_SPACE_BIT) =
        ((g_ucSensorType >> 1) & 0x01);

    //
    // Reconfigure the Hall sensor support routines.
    //
    HallConfigure();

    //
    // Reconfigure the ADC support routines.
    //
    ADCConfigure();
}

//*****************************************************************************
//
//! Updates the sensor polarity bit of the motor drive.
//!
//! This function is called when the variable controlling the polarity of
//! the sensor is updated.  The value is then reflected into the usFlags
//! member of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UISensorPolarity(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // The sensor polarity can not be changed when the motor drive is
        // running, so revert the sensor polarity variable back to the value
        // in the flags.
        //
        g_ucSensorPolarity = HWREGBITH(&(g_sParameters.usFlags),
                FLAG_SENSOR_POLARITY_BIT);
        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // Update the sensor type flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_SENSOR_POLARITY_BIT) =
        g_ucSensorPolarity;
}

//*****************************************************************************
//
//! Updates the modulation waveform type bit in the motor drive.
//!
//! This function is called when the variable controlling the modulation
//! waveform type is updated.  The value is then reflected into the usFlags
//! member of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIModulationType(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // The modulation type can not changed when the motor drive is running
        // (that could be catastrophic!), so revert the modulation type
        // variable back to the value in the flags.
        //
        g_ucModulationType = g_sParameters.ucModulationType;

        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // Update the modulation waveform type flag in the flags variable.
    //
    g_sParameters.ucModulationType = g_ucModulationType;

    //
    // Reconfigure the Hall sensor support routines.
    //
    HallConfigure();

    //
    // Reconfigure the ADC support routines.
    //
    ADCConfigure();
}

//*****************************************************************************
//
//! Updates the motor drive direction bit.
//!
//! This function is called when the variable controlling the motor drive
//! direction is updated.  The value is then reflected into the usFlags
//! member of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIDirectionSet(void)
{
    //
    // Update the direction flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_DIR_BIT) = g_ucDirection;

    //
    // Change the direction of the motor drive.
    //
    MainSetDirection(g_ucDirection ? false : true);
}

//*****************************************************************************
//
//! Updates the PWM frequency of the motor drive.
//!
//! This function is called when the variable controlling the PWM frequency of
//! the motor drive is updated.  The value is then reflected into the usFlags
//! member of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIPWMFrequencySet(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // The modulation type can not changed when the motor drive is running
        // (that could be catastrophic!), so revert the modulation type
        // variable back to the value in the flags.
        //
        g_ucFrequency = g_sParameters.usFlags & FLAG_PWM_FREQUENCY_MASK;
        if(g_ucFrequency > 3)
        {
            g_ucFrequency = (g_ucFrequency & 0x3) + 4;
        }

        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // Map the UI parameter value to actual frequency value.
    //
    switch(g_ucFrequency)
    {
        case 0:
            g_sParameters.usFlags =
                ((g_sParameters.usFlags & ~FLAG_PWM_FREQUENCY_MASK) |
                 FLAG_PWM_FREQUENCY_8K);
            break;

        case 1:
            g_sParameters.usFlags =
                ((g_sParameters.usFlags & ~FLAG_PWM_FREQUENCY_MASK) |
                 FLAG_PWM_FREQUENCY_12K);
            break;

        case 2:
            g_sParameters.usFlags =
                ((g_sParameters.usFlags & ~FLAG_PWM_FREQUENCY_MASK) |
                 FLAG_PWM_FREQUENCY_16K);
            break;

        case 4:
            g_sParameters.usFlags =
                ((g_sParameters.usFlags & ~FLAG_PWM_FREQUENCY_MASK) |
                 FLAG_PWM_FREQUENCY_25K);
            break;

        case 5:
            g_sParameters.usFlags =
                ((g_sParameters.usFlags & ~FLAG_PWM_FREQUENCY_MASK) |
                 FLAG_PWM_FREQUENCY_40K);
            break;

        case 6:
            g_sParameters.usFlags =
                ((g_sParameters.usFlags & ~FLAG_PWM_FREQUENCY_MASK) |
                 FLAG_PWM_FREQUENCY_50K);
            break;

        case 7:
            g_sParameters.usFlags =
                ((g_sParameters.usFlags & ~FLAG_PWM_FREQUENCY_MASK) |
                 FLAG_PWM_FREQUENCY_80K);
            break;

        case 3:
        default:
            g_sParameters.usFlags =
                ((g_sParameters.usFlags & ~FLAG_PWM_FREQUENCY_MASK) |
                 FLAG_PWM_FREQUENCY_20K);
            break;
    }

    //
    // Change the PWM frequency.
    //
    MainSetPWMFrequency();
}

//*****************************************************************************
//
//! Sets the update rate of the motor drive.
//!
//! This function is called when the variable specifying the update rate of the
//! motor drive is updated.  This allows the motor drive to perform a
//! synchronous change of the update rate to avoid discontinuities in the
//! output waveform.
//!
//! \return None.
//
//*****************************************************************************
static void
UIUpdateRate(void)
{
    //
    // Set the update rate of the motor drive.
    //
    PWMSetUpdateRate(g_ucUpdateRate);
}

//*****************************************************************************
//
//! Updates the I coefficient of the frequency PI controller.
//!
//! This function is called when the variable containing the I coefficient of
//! the frequency PI controller is updated.  The value is then reflected into
//! the parameter block.
//!
//! \return None.
//
//*****************************************************************************
static void
UIFAdjI(void)
{
    //
    // Update the frequency PI controller.
    //
    MainUpdateFAdjI(g_lFAdjI);
}

//*****************************************************************************
//
//! Updates the I coefficient of the power PI controller.
//!
//! This function is called when the variable containing the I coefficient of
//! the power PI controller is updated.  The value is then reflected into
//! the parameter block.
//!
//! \return None.
//
//*****************************************************************************
static void
UIPAdjI(void)
{
    //
    // Update the frequency PI controller.
    //
    MainUpdatePAdjI(g_lPAdjI);
}

//*****************************************************************************
//
//! Updates the dynamic brake bit of the motor drive.
//!
//! This function is called when the variable controlling the dynamic braking
//! is updated.  The value is then reflected into the usFlags member of
//! #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIDynamicBrake(void)
{
    //
    // Update the dynamic braking flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_BRAKE_BIT) = g_ucDynamicBrake;
}

//*****************************************************************************
//
//! Updates the decay mode bit of the motor drive.
//!
//! This function is called when the variable controlling the decay mode
//! is updated.  The value is then reflected into the usFlags member of
//! #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIDecayMode(void)
{
    //
    // Update the decay mode flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_DECAY_BIT) = g_ucDecayMode;
}

//*****************************************************************************
//
//! Starts the motor drive.
//!
//! This function is called by the serial user interface when the run command
//! is received.  The motor drive will be started as a result; this is a no
//! operation if the motor drive is already running.
//!
//! \return None.
//
//*****************************************************************************
void
UIRun(void)
{
    //
    // Start the motor drive.
    //
    MainRun();
}

//*****************************************************************************
//
//! Stops the motor drive.
//!
//! This function is called by the serial user interface when the stop command
//! is received.  The motor drive will be stopped as a result; this is a no
//! operation if the motor drive is already stopped.
//!
//! \return None.
//
//*****************************************************************************
void
UIStop(void)
{
    //
    // Stop the motor drive.
    //
    MainStop();
}

//*****************************************************************************
//
//! Emergency stops the motor drive.
//!
//! This function is called by the serial user interface when the emergency
//! stop command is received.
//!
//! \return None.
//
//*****************************************************************************
void
UIEmergencyStop(void)
{
    //
    // Emergency stop the motor drive.
    //
    MainEmergencyStop();

    //
    // Indicate that the emergency stop fault has occurred.
    //
    MainSetFault(FAULT_EMERGENCY_STOP);
}

//*****************************************************************************
//
//! Loads the motor drive parameter block from flash.
//!
//! This function is called by the serial user interface when the load
//! parameter block function is called.  If the motor drive is running, the
//! parameter block is not loaded (since that may result in detrimental
//! changes, such as changing the motor drive from sine to trapezoid).
//! If the motor drive is not running and a valid parameter block exists in
//! flash, the contents of the parameter block are loaded from flash.
//!
//! \return None.
//
//*****************************************************************************
void
UIParamLoad(void)
{
    unsigned char *pucBuffer;
    unsigned long ulIdx;

    //
    // Return without doing anything if the motor drive is running.
    //
    if(MainIsRunning())
    {
        return;
    }

    //
    // Get a pointer to the latest parameter block in flash.
    //
    pucBuffer = FlashPBGet();

    //
    // See if a parameter block was found in flash.
    //
    if(pucBuffer)
    {
        //
        // Loop through the words of the parameter block to copy its contents
        // from flash to SRAM.
        //
        for(ulIdx = 0; ulIdx < (sizeof(tDriveParameters) / 4); ulIdx++)
        {
            ((unsigned long *)&g_sParameters)[ulIdx] =
                ((unsigned long *)pucBuffer)[ulIdx];
        }

        //
        // Check for older versions of the parameter block and supply any
        // new parameters (as needed).
        //
        if(g_sParameters.ucVersion < 4)
        {
            g_sParameters.ucBEMFSkipCount = 3;
            g_sParameters.usStartupCount = 1000;
            g_sParameters.ulSensorlessStartVoltage = 1200;
            g_sParameters.ulSensorlessEndVoltage = 3600;
            g_sParameters.ulSensorlessStartSpeed = 400;
            g_sParameters.ulSensorlessEndSpeed = 1000;
            g_sParameters.usSensorlessRampTime = 500;
            g_sParameters.lPAdjP = (65536 * 8);
            g_sParameters.lPAdjI = 1000;
            g_sParameters.ulMinPower = 0;
            g_sParameters.ulMaxPower = 360000;
            g_sParameters.ulTargetPower = 0;
            g_sParameters.ulTargetSpeed = 3000;
            g_sParameters.usAccelPower = 1000;
            g_sParameters.usDecelPower = 1000;
            g_sParameters.ucModulationType = MOD_TYPE_TRAPEZOID;
            g_sParameters.ucControlType = CONTROL_TYPE_SPEED;
            HWREGBITH(&(g_sParameters.usFlags), FLAG_SENSOR_SPACE_BIT) = 0;
            g_sParameters.usSensorlessBEMFThresh = 500;
        }

        //
        // Check for older versions of the parameter block and supply any
        // new parameters (as needed).
        //
        if(g_sParameters.ucVersion < 5)
        {
            g_sParameters.ucVersion = 5;
            g_sParameters.ucBEMFSkipCount = 3;
            g_sParameters.ucNumPoles = ((g_sParameters.ucNumPoles + 1) * 2);
        }
    }

    //
    // Set the local variables (used by the serial interface) based on the
    // values in the parameter block values.
    //
    g_ucEncoder = HWREGBITH(&(g_sParameters.usFlags), FLAG_ENCODER_BIT);
    g_ucControlType = g_sParameters.ucControlType;
    g_ucModulationType = g_sParameters.ucModulationType;
    g_ucDirection = HWREGBITH(&(g_sParameters.usFlags), FLAG_DIR_BIT);
    g_ucFrequency = g_sParameters.usFlags & FLAG_PWM_FREQUENCY_MASK;
    if(g_ucFrequency > 3)
    {
        g_ucFrequency = (g_ucFrequency & 0x3) + 4;
    }
    g_ucUpdateRate = g_sParameters.ucUpdateRate;
    g_lFAdjI = g_sParameters.lFAdjI;
    g_lPAdjI = g_sParameters.lPAdjI;
    g_ucDynamicBrake = HWREGBITH(&(g_sParameters.usFlags), FLAG_BRAKE_BIT);
    g_ucSensorType = HWREGBITH(&(g_sParameters.usFlags), FLAG_SENSOR_TYPE_BIT);
    g_ucSensorType |= (HWREGBITH(&(g_sParameters.usFlags),
                                 FLAG_SENSOR_SPACE_BIT) << 1);



    g_ucDecayMode =  HWREGBITH(&(g_sParameters.usFlags), FLAG_DECAY_BIT);
    g_ucSensorPolarity =
        HWREGBITH(&(g_sParameters.usFlags), FLAG_SENSOR_POLARITY_BIT);

    //
    // Loop through all of the parameters.
    //
    for(ulIdx = 0; ulIdx < g_ulUINumParameters; ulIdx++)
    {
        //
        // If there is an update function for this parameter, then call it now
        // since the parameter value may have changed as a result of the load.
        //
        if(g_sUIParameters[ulIdx].pfnUpdate)
        {
            g_sUIParameters[ulIdx].pfnUpdate();
        }
    }
}

//*****************************************************************************
//
//! Saves the motor drive parameter block to flash.
//!
//! This function is called by the serial user interface when the save
//! parameter block function is called.  The parameter block is written to
//! flash for use the next time a load occurs (be it from an explicit request
//! or a power cycle of the drive).
//!
//! \return None.
//
//*****************************************************************************
void
UIParamSave(void)
{
    //
    // Return without doing anything if the motor drive is running.
    //
    if(MainIsRunning())
    {
        return;
    }

    //
    // Save the parameter block to flash.
    //
    FlashPBSave((unsigned char *)&g_sParameters);
}

//*****************************************************************************
//
//! Starts a firmware upgrade.
//!
//! This function is called by the serial user interface when a firmware
//! upgrade has been requested.  This will branch directly to the boot loader
//! and relinquish all control, never returning.
//!
//! \return None.
//
//*****************************************************************************
void
UIUpgrade(void)
{
    //
    // Call the main control loop firmware upgrade function.
    //
    MainUpgrade();
}

//*****************************************************************************
//
//! Handles button presses.
//!
//! This function is called when a press of the on-board push button has been
//! detected.  If the motor drive is running, it will be stopped.  If it is
//! stopped, the direction will be reversed and the motor drive will be
//! started.
//!
//! \return None.
//
//*****************************************************************************
void
UIButtonPress(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // Stop the motor drive.
        //
        MainStop();
    }
    else
    {
        //
        // Reverse the motor drive direction.
        //
        g_ucDirection ^= 1;
        UIDirectionSet();

        //
        // Start the motor drive.
        //
        MainRun();
    }
}

//*****************************************************************************
//
//! Handles button holds.
//!
//! This function is called when a hold of the on-board push button has been
//! detected.  The modulation type of the motor will be toggled between sine
//! wave and space vector modulation, but only if a three phase motor is in
//! use.
//!
//! \return None.
//
//*****************************************************************************
static void
UIButtonHold(void)
{
    //
    // Toggle the modulation type.  UIModulationType() will take care of
    // forcing sine wave modulation for single phase motors.
    //
    //g_ucModulation ^= 1;
    //UIModulationType();
}

//*****************************************************************************
//
//! Sets the blink rate for an LED.
//!
//! \param ulIdx is the number of the LED to configure.
//! \param usRate is the rate to blink the LED.
//! \param usPeriod is the amount of time to turn on the LED.
//!
//! This function sets the rate at which an LED should be blinked.  A blink
//! period of zero means that the LED should be turned off, and a blink period
//! equal to the blink rate means that the LED should be turned on.  Otherwise,
//! the blink rate determines the number of user interface interrupts during
//! the blink cycle of the LED, and the blink period is the number of those
//! user interface interrupts during which the LED is turned on.
//!
//! \return None.
//
//*****************************************************************************
static void
UILEDBlink(unsigned long ulIdx, unsigned short usRate, unsigned short usPeriod)
{
    //
    // Clear the blink rate for this LED.
    //
    g_pusBlinkRate[ulIdx] = 0;

    //
    // A blink period of zero means that the LED should be turned off.
    //
    if(usPeriod == 0)
    {
        //
        // Turn off the LED.
        //
        GPIOPinWrite(g_pulLEDBase[ulIdx], g_pucLEDPin[ulIdx],
                     (ulIdx == 0) ? g_pucLEDPin[0] : 0);
    }

    //
    // A blink rate equal to the blink period means that the LED should be
    // turned on.
    //
    else if(usRate == usPeriod)
    {
        //
        // Turn on the LED.
        //
        GPIOPinWrite(g_pulLEDBase[ulIdx], g_pucLEDPin[ulIdx],
                     (ulIdx == 0) ? 0 : g_pucLEDPin[ulIdx]);
    }

    //
    // Otherwise, the LED should be blinked at the given rate.
    //
    else
    {
        //
        // Save the blink rate and period for this LED.
        //
        g_pusBlinkRate[ulIdx] = usRate;
        g_pusBlinkPeriod[ulIdx] = usPeriod;
    }
}

//*****************************************************************************
//
//! Sets the blink rate for the run LED.
//!
//! \param usRate is the rate to blink the run LED.
//! \param usPeriod is the amount of time to turn on the run LED.
//!
//! This function sets the rate at which the run LED should be blinked.  A
//! blink period of zero means that the LED should be turned off, and a blink
//! period equal to the blink rate means that the LED should be turned on.
//! Otherwise, the blink rate determines the number of user interface
//! interrupts during the blink cycle of the run LED, and the blink period
//! is the number of those user interface interrupts during which the LED is
//! turned on.
//!
//! \return None.
//
//*****************************************************************************
void
UIRunLEDBlink(unsigned short usRate, unsigned short usPeriod)
{
    //
    // The run LED is the first LED.
    //
    UILEDBlink(0, usRate, usPeriod);
}

//*****************************************************************************
//
//! Sets the blink rate for the fault LED.
//!
//! \param usRate is the rate to blink the fault LED.
//! \param usPeriod is the amount of time to turn on the fault LED.
//!
//! This function sets the rate at which the fault LED should be blinked.  A
//! blink period of zero means that the LED should be turned off, and a blink
//! period equal to the blink rate means that the LED should be turned on.
//! Otherwise, the blink rate determines the number of user interface
//! interrupts during the blink cycle of the fault LED, and the blink period
//! is the number of those user interface interrupts during which the LED is
//! turned on.
//!
//! \return None.
//
//*****************************************************************************
void
UIFaultLEDBlink(unsigned short usRate, unsigned short usPeriod)
{
    //
    // The fault LED is the second LED.
    //
    UILEDBlink(1, usRate, usPeriod);
}

//*****************************************************************************
//
//! This function returns the current number of system ticks.
//!
//! \return The number of system timer ticks.
//
//*****************************************************************************
unsigned long
UIGetTicks(void)
{
    unsigned long ulTime1;
    unsigned long ulTime2;
    unsigned long ulTicks;

    //
    // We read the SysTick value twice, sandwiching taking the snapshot of
    // the tick count value. If the second SysTick read gives us a higher
    // number than the first read, we know that it wrapped somewhere between
    // the two reads so our tick count value is suspect.  If this occurs,
    // we go round again. Note that it is not sufficient merely to read the
    // values with interrupts disabled since the SysTick counter keeps
    // counting regardless of whether or not the wrap interrupt has been
    // serviced.
    //
    do
    {
        ulTime1 = TimerValueGet(TIMER1_BASE, TIMER_A);
        ulTicks = g_ulUITickCount;
        ulTime2 = TimerValueGet(TIMER1_BASE, TIMER_A);
    }
    while(ulTime2 > ulTime1);

    //
    // Calculate the number of ticks
    //
    ulTime1 = ulTicks + (SYSTEM_CLOCK / TIMER1A_INT_RATE) - ulTime2;

    //
    // Return the value.
    //
    return(ulTime1);
}

//*****************************************************************************
//
//! Handles the Timer1A interrupt.
//!
//! This function is called when Timer1A asserts its interrupt.  It is
//! responsible for keeping track of system time.  This should be the highest
//! priority interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
Timer1AIntHandler(void)
{
    //
    // Clear the Timer interrupt.
    //
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Increment the running count of timer ticks, based on the Timer1A Tick
    // interrupt rate.
    //
    g_ulUITickCount += (SYSTEM_CLOCK / TIMER1A_INT_RATE);
}


//*****************************************************************************
//
//! Handles the SysTick interrupt.
//!
//! This function is called when SysTick asserts its interrupt.  It is
//! responsible for handling the on-board user interface elements (push button
//! and potentiometer) if enabled, and the processor usage computation.
//!
//! \return None.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulIdx, ulCount;

    //
    // Run the Hall module tick handler.
    //
    HallTickHandler();

    //
    // Run the ADC module tick handler.
    //
    ADCTickHandler();

    //
    // Run the UI Ethernet tick handler.
    //
    UIEthernetTick(UI_TICK_MS);

    //
    // Convert the ADC Analog Input reading to milli-volts.  Each volt at the
    // ADC input corresponds to ~1.714 volts at the Analog Input.
    //
    ulCount = ADCReadAnalog();
    g_usAnalogInputVoltage = (((g_usAnalogInputVoltage * 3) +
        (((ulCount * 3000 * 240) / 140) / 1024)) / 4);

    //
    // Read the on-board switch and pass its current value to the switch
    // debouncer, only if the onboard user interface is enabled.
    //
    if(g_ulUIUseOnboard)
    {
        UIOnboardSwitchDebouncer(GPIOPinRead(PIN_SWITCH_PORT, PIN_SWITCH_PIN));
    }

    //
    // Read the config switch settings into the GPIO data variable.
    //
    g_ulGPIOData = ((GPIOPinRead(PIN_CFG0_PORT,
                    (PIN_CFG0_PIN | PIN_CFG1_PIN | PIN_CFG2_PIN)) >> 2) &
                    0x07);

    //
    // Read the encoder input pins into the GPIO data variable.
    //
    g_ulGPIOData |=
        (((GPIOPinRead(PIN_ENCA_PORT, PIN_ENCA_PIN) >> 4) & 1) << 8);
    g_ulGPIOData |=
        (((GPIOPinRead(PIN_ENCB_PORT, PIN_ENCB_PIN) >> 7) & 1) << 9);
    g_ulGPIOData |=
        (((GPIOPinRead(PIN_INDEX_PORT, PIN_INDEX_PIN) >> 2) & 1) << 10);

    //
    // Compute the new value for the processor usage.
    //
    g_ucCPUUsage = (CPUUsageTick() + 32768) / 65536;

    //
    // Increment the blink counter.
    //
    g_ulBlinkCount++;

    //
    // Loop through the two LEDs.
    //
    for(ulIdx = 0; ulIdx < 2; ulIdx++)
    {
        //
        // See if this LED is enabled for blinking.
        //
        if(g_pusBlinkRate[ulIdx] != 0)
        {
            //
            // Get the count in terms of the clock for this LED.
            //
            ulCount = g_ulBlinkCount % g_pusBlinkRate[ulIdx];

            //
            // The LED should be turned on when the count is zero.
            //
            if(ulCount == 0)
            {
                GPIOPinWrite(g_pulLEDBase[ulIdx], g_pucLEDPin[ulIdx],
                             (ulIdx == 0) ? 0 : g_pucLEDPin[ulIdx]);
            }

            //
            // The LED should be turned off when the count equals the period.
            //
            if(ulCount == g_pusBlinkPeriod[ulIdx])
            {
                GPIOPinWrite(g_pulLEDBase[ulIdx], g_pucLEDPin[ulIdx],
                             (ulIdx == 0) ? g_pucLEDPin[0] : 0);
            }
        }
    }

    //
    // Send real-time data, if appropriate.
    //
    UIEthernetSendRealTimeData();
}

//*****************************************************************************
//
//! Initializes the user interface.
//!
//! This function initializes the user interface modules (on-board and serial),
//! preparing them to operate and control the motor drive.
//!
//! \return None.
//
//*****************************************************************************
void
UIInit(void)
{
    volatile int iLoop;

    //
    //
    // Make the push button pin be a GPIO input.
    //
    GPIOPinTypeGPIOInput(PIN_SWITCH_PORT, PIN_SWITCH_PIN);
    GPIOPadConfigSet(PIN_SWITCH_PORT, PIN_SWITCH_PIN, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    //
    // Make the LEDs be GPIO outputs and turn them off.
    //
    GPIOPinTypeGPIOOutput(PIN_LEDRUN_PORT, PIN_LEDRUN_PIN);
    GPIOPinTypeGPIOOutput(PIN_LEDFAULT_PORT, PIN_LEDFAULT_PIN);
    GPIOPinWrite(PIN_LEDRUN_PORT, PIN_LEDRUN_PIN, 0);
    GPIOPinWrite(PIN_LEDFAULT_PORT, PIN_LEDFAULT_PIN, 0);

    //
    // Configure and read the configuration switches and store the values
    // for future reference.
    // 
    GPIOPinTypeGPIOInput(PIN_CFG0_PORT,
                         (PIN_CFG0_PIN | PIN_CFG1_PIN | PIN_CFG2_PIN));
    GPIOPadConfigSet(PIN_CFG0_PORT,
                    (PIN_CFG0_PIN | PIN_CFG1_PIN | PIN_CFG2_PIN),
                     GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);
    for(iLoop = 0; iLoop < 10000; iLoop++)
    {
    }
    g_ucBoardID = ((GPIOPinRead(PIN_CFG0_PORT,
                   (PIN_CFG1_PIN | PIN_CFG0_PIN)) >> 2) & 0x03);

    //
    // Ensure that the UART pins are configured appropriately.
    //
    GPIOPinTypeUART(PIN_UART0RX_PORT, PIN_UART0RX_PIN);
    GPIOPinTypeUART(PIN_UART0TX_PORT, PIN_UART0TX_PIN);
    
    //
    // Initialize the Ethernet user interface.
    //
    UIEthernetInit(GPIOPinRead(PIN_SWITCH_PORT, PIN_SWITCH_PIN) ?
                   true : false);

    //
    // Initialize the CAN user interface.
    //
    UICANInit();

    //
    // Initialize the on-board user interface.
    //
    UIOnboardInit(GPIOPinRead(PIN_SWITCH_PORT, PIN_SWITCH_PIN), 0);

    //
    // Initialize the processor usage routine.
    //
    CPUUsageInit(SYSTEM_CLOCK, UI_INT_RATE, 2);

    //
    // Configure SysTick to provide a periodic user interface interrupt.
    //
    SysTickPeriodSet(SYSTEM_CLOCK / UI_INT_RATE);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Configure and enable a timer to provide a periodic interrupt.
    //
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, (SYSTEM_CLOCK / TIMER1A_INT_RATE));
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER1A);
    TimerEnable(TIMER1_BASE, TIMER_A);

    //
    // Load the parameter block from flash if there is a valid one.
    //
    UIParamLoad();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
