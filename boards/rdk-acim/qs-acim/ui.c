//*****************************************************************************
//
// ui.c - User interface module.
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
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/cpu_usage.h"
#include "utils/flash_pb.h"
#include "adc_ctrl.h"
#include "commands.h"
#include "faults.h"
#include "inrush.h"
#include "main.h"
#include "pins.h"
#include "pwm_ctrl.h"
#include "speed_sense.h"
#include "ui.h"
#include "ui_common.h"
#include "ui_onboard.h"
#include "ui_serial.h"

//*****************************************************************************
//
//! \page ui_intro Introduction
//!
//! There are two user interfaces for the the AC induction motor application.
//! One uses an on-board potentiometer and push button for basic control of
//! the motor and four LEDs for basic status feedback, and the other uses the
//! serial port to provide complete control of all aspects of the motor drive
//! as well as monitoring of real-time performance data.
//!
//! The on-board user interface consists of a potentiometer, push button, and
//! four LEDs.  The potentiometer is not directly sampled; it controls the
//! frequency of an oscillator whose output is passed through the isolation
//! barrier.  The potentiometer value is determined by measuring the time
//! between edges from the oscillator.  The potentiometer controls the
//! frequency of the motor drive, and the push button cycles between run
//! forward, stop, run backward, stop.  Holding the push button for five
//! seconds while the motor drive is stopped will toggle between sine wave
//! modulation and space vector modulation.
//!
//! The ``Run'' LED flashes the entire time the application is running.  The
//! LED is off most of the time if the motor drive is stopped and on most of
//! the time if it is running.  The ``Fault'' LED is normally off but flashes
//! at a fast rate when a fault occurs.  Also, it flashes slowly when the
//! in-rush current limiter is operating on application startup.  The ``S1''
//! LED is on when the dynamic brake is active and off when it is not active.
//! And the ``S2'' LED is on when space vector modulation is being used and off
//! when sine wave modulation is being used.
//!
//! A periodic interrupt is used to poll the state of the push button and
//! perform debouncing.  A separate edge-triggered GPIO interrupt is used to
//! measure the time between edges from the potentiometer-controlled
//! oscillator.
//!
//! The serial user interface is entirely handled by the serial user interface
//! module.  The only thing provided here is the list of parameters and
//! real-time data items, plus a set of helper functions that are required in
//! order to properly set the values of some of the parameters.
//!
//! This user interface (and the accompanying serial and on-board user
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

//*****************************************************************************
//
//! The minimum value that can be read from the potentiometer.  This
//! corresponds to the value read when the wiper is all the way to the left.
//
//*****************************************************************************
#define UI_POT_MIN              70

//*****************************************************************************
//
//! The maximum value that can be read from the potentiometer.  This
//! corresponds to the value read when the wiper is all the way to the right.
//
//*****************************************************************************
#define UI_POT_MAX              355

//*****************************************************************************
//
// Forward declarations for functions declared within this source file for use
// in the parameter and real-time data structures.
//
//*****************************************************************************
static void UILoopMode(void);
static void UIEncoderPresent(void);
static void UIModulationType(void);
static void UIDirectionSet(void);
static void UIPWMFrequencySet(void);
static void UIUpdateRate(void);
static void UIMotorType(void);
static void UIFAdjI(void);
static void UIBusComp(void);
static void UIVfRange(void);
static void UIDynamicBrake(void);
static void UIDCBrake(void);
static void UIButtonPress(void);
static void UIButtonHold(void);

//*****************************************************************************
//
//! The blink rate of the four LEDs on the board; this is the number of user
//! interface interrupts for an entire blink cycle.  The run LED is the first
//! entry of the array, the fault LED is the second entry of the array, the
//! status1 LED is the third entry of the array, and the status2 LED is the
//! fourth entry of the array.
//
//*****************************************************************************
static unsigned short g_pusBlinkRate[4] =
{
    0, 0, 0, 0
};

//*****************************************************************************
//
//! The blink period of the four LEDs on the board; this is the number of user
//! interface interrupts for which the LED will be turned on.  The run LED is
//! the first entry of the array, the fault LED is the second entry of the
//! array, the status1 LED is the third entry of the array, and the status2 LED
//! is the fourth entry of the array.
//
//*****************************************************************************
static unsigned short g_pusBlinkPeriod[4];

//*****************************************************************************
//
//! The count of count of user interface interrupts that have occurred.  This
//! is used to determine when to toggle the LEDs that are blinking.
//
//*****************************************************************************
static unsigned long g_ulBlinkCount = 0;

//*****************************************************************************
//
//! This array contains the base address of the GPIO blocks for the four LEDs
//! on the board.
//
//*****************************************************************************
static const unsigned long g_pulLEDBase[4] =
{
    PIN_LEDRUN_PORT,
    PIN_LEDFAULT_PORT,
    PIN_LEDSTATUS1_PORT,
    PIN_LEDSTATUS2_PORT
};

//*****************************************************************************
//
//! This array contains the pin numbers of the four LEDs on the board.
//
//*****************************************************************************
static const unsigned char g_pucLEDPin[4] =
{
    PIN_LEDRUN_PIN,
    PIN_LEDFAULT_PIN,
    PIN_LEDSTATUS1_PIN,
    PIN_LEDSTATUS2_PIN
};

//*****************************************************************************
//
//! The specification of open-loop or closed-loop mode of the motor drive.
//! This variable is used by the serial interface as a staging area before the
//! value gets placed into the flags in the parameter block by UILoopMode().
//
//*****************************************************************************
static unsigned char g_ucLoop;

//*****************************************************************************
//
//! The specification of the encoder presence on the motor.  This variable is
//! used by the serial interface as a staging area before the value gets
//! placed into the flags in the parameter block by UIEncoderPresent().
//
//*****************************************************************************
static unsigned char g_ucEncoder;

//*****************************************************************************
//
//! The specification of the modulation waveform type for the motor drive.
//! This variable is used by the serial interface as a staging area before the
//! value gets placed into the flags in the parameter block by
//! UIModulationType().
//
//*****************************************************************************
static unsigned char g_ucModulation;

//*****************************************************************************
//
//! The specification of the motor drive direction.  This variable is used by
//! the serial interface as a staging area before the value gets placed into
//! the flags in the parameter block by UIDirectionSet().
//
//*****************************************************************************
static unsigned char g_ucDirection;

//*****************************************************************************
//
//! The specification of the PWM frequency for the motor drive.  This variable
//! is used by the serial interface as a staging area before the value gets
//! placed into the flags in the parameter block by UIPWMFrequencySet().
//
//*****************************************************************************
static unsigned char g_ucFrequency;

//*****************************************************************************
//
//! The specification of the update rate for the motor drive.  This variable is
//! used by the serial interface as a staging area before the value gets
//! updated in a synchronous manner by UIUpdateRate().
//
//*****************************************************************************
static unsigned char g_ucUpdateRate;

//*****************************************************************************
//
//! The specification of the type of motor connected to the motor drive.  This
//! variable is used by the serial interface as a staging area before the value
//! gets placed into the flags in the parameter block by UIMotorType().
//
//*****************************************************************************
static unsigned char g_ucType;

//*****************************************************************************
//
//! The I coefficient of the frequency PI controller.  This variable is used by
//! the serial interface as a staging area before the value gets placed into
//! the parameter block by UIFAdjI().
//
//*****************************************************************************
static long g_lFAdjI;

//*****************************************************************************
//
//! A boolean that is true when the on-board user interface should be active
//! and false when it should not be.
//
//*****************************************************************************
static unsigned long g_ulUIUseOnboard = 1;

//*****************************************************************************
//
//! A boolean that is true when the DC bus voltage compensation should be
//! active and false when it should not be.  This variable is used by the
//! serial interface as a staging area before the value gets placed into the
//! flags in the parameter block by UIBusComp().
//
//*****************************************************************************
static unsigned char g_ucBusComp;

//*****************************************************************************
//
//! A boolean that is true when the V/f table ranges from 0 Hz to 400 Hz and
//! false when it ranges from 0 Hz to 100 Hz.  This variable is used by the
//! serial interface as a staging area before the value gets placed into the
//! flags in the parameter block by UIVfRange().
//
//*****************************************************************************
static unsigned char g_ucVfRange;

//*****************************************************************************
//
//! A boolean that is true when dynamic braking should be utilized.  This
//! variable is used by the serial interface as a staging area before the value
//! gets placed into the flags in the parameter block by UIDynamicBrake().
//
//*****************************************************************************
static unsigned char g_ucDynamicBrake;

//*****************************************************************************
//
//! A boolean that is true when DC injection braking should be utilized.  This
//! variable is used by the serial interface as a staging area before the value
//! gets placed into the flags in the parameter block by UIDCBrake().
//
//*****************************************************************************
static unsigned char g_ucDCBrake;

//*****************************************************************************
//
//! The processor usage for the most recent measurement period.  This is a
//! value between 0 and 100, inclusive.
//
//*****************************************************************************
unsigned char g_ucCPUUsage = 0;

//*****************************************************************************
//
//! The time between the last two edges on the potentiometer input.  The
//! potentiometer controls a variable frequency oscillator whose output is
//! passed through the electrical isolation barrier; measuring the time between
//! edges provides an approximation of the value of the potentiometer.
//
//*****************************************************************************
static unsigned long g_ulUIPotEdgeTime = 0;

//*****************************************************************************
//
//! The value of the SysTick timer when the most recent edge was received on
//! the potentiometer.  When a new edge is detected, this is used to determine
//! the time between the edges.
//
//*****************************************************************************
static unsigned long g_ulUIPotPreviousTime;

//*****************************************************************************
//
//! This structure instance contains the configuration values for the AC
//! induction motor drive.
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
    1,

    //
    // The minimum pulse width (ucMinPulseWidth).
    //
    10,

    //
    // The PWM dead time (ucDeadTime).
    //
    100,

    //
    // The PWM update rate (ucUpdateRate).
    //
    0,

    //
    // The number of poles (ucNumPoles).
    //
    1,

    //
    // The acceleration rate (ucAccel).
    //
    40,

    //
    // The deceleration rate (ucDecel).
    //
    40,

    //
    // The minimum motor drive current (ucMinCurrent).
    //
    1,

    //
    // The maximum motor drive current (ucMaxCurrent).
    //
    48,

    //
    // The precharge time (ucPrechargeTime).
    //
    2,

    //
    // The maximum ambient microcontroller temperature (ucMaxTemperature).
    //
    85,

    //
    // The flags (usFlags).
    //
    (FLAG_PWM_FREQUENCY_20K |
     (FLAG_MOTOR_TYPE_3PHASE << FLAG_MOTOR_TYPE_BIT) |
     (FLAG_LOOP_OPEN << FLAG_LOOP_BIT) |
     (FLAG_DRIVE_SINE << FLAG_DRIVE_BIT) |
     (FLAG_DIR_FORWARD << FLAG_DIR_BIT) |
     (FLAG_ENCODER_PRESENT << FLAG_ENCODER_BIT) |
     (FLAG_VF_RANGE_400 << FLAG_VF_RANGE_BIT) |
     (FLAG_BUS_COMP_ON << FLAG_BUS_COMP_BIT) |
     (FLAG_BRAKE_ON << FLAG_BRAKE_BIT) |
     (FLAG_DC_BRAKE_ON << FLAG_DC_BRAKE_BIT)),

    //
    // The number of encoder lines (usNumEncoderLines).
    //
    7,

    //
    // The minimum motor frequency (usMinFrequency).
    //
    600,

    //
    // The maximum motor frequency (usMaxFrequency).
    //
    3400,

    //
    // The minimum DC bus voltage (usMinVBus).
    //
    250,

    //
    // The maximum DC bus voltage (usMaxVBus).
    //
    390,

    //
    // The brake engage voltage (usBrakeOnV).
    //
    360,

    //
    // The brake disengage voltage (usBrakeOffV).
    //
    350,

    //
    // The DC injection braking voltage (usDCBrakeV).
    //
    24,

    //
    // The DC injection braking time (usDCBrakeTime).
    //
    200,

    //
    // The DC bus voltage at which the deceleration rate is reduced (usDecelV).
    //
    350,

    //
    // The V/f table (usVFTable).
    //
    {
        4200,
        5200,
        6200,
        7200,
        8300,
        9700,
        11500,
        13400,
        15200,
        17050,
        18900,
        20750,
        22550,
        24400,
        26250,
        28100,
        29900,
        31750,
        31750,
        31750,
        31750
    },

    //
    // The frequency adjust P coefficient (lFAdjP).
    //
    32768,

    //
    // The frequency adjust I coefficient (lFAdjI).
    //
    128,

    //
    // The brake maximum time (ulBrakeMax).
    //
    60 * 1000,

    //
    // The brake cooling time (ulBrakeCool).
    //
    55 * 1000,

    //
    // The motor current at which the acceleration rate is reduced
    // (ucAccelCurrent).
    //
    48
};

//*****************************************************************************
//
//! The current drive frequency.  This is updated by the speed control routine
//! as it ramps the speed of the motor drive.
//
//*****************************************************************************
unsigned short g_usCurrentFrequency;

//*****************************************************************************
//
//! The target drive frequency.
//
//*****************************************************************************
unsigned short g_usTargetFrequency;

//*****************************************************************************
//
//! The target type for this drive.  This is used by the serial user interface
//! module.
//
//*****************************************************************************
const unsigned long g_ulUITargetType = RESP_ID_TARGET_ACIM;

//*****************************************************************************
//
//! The version of the firmware.  Changing this value will make it much more
//! difficult for Texas Instruments support personnel to determine the firmware
//! in use when trying to provide assistance; it should only be changed after
//! careful consideration.
//
//*****************************************************************************
const unsigned short g_usFirmwareVersion = 10636;

//*****************************************************************************
//
//! An array of structures describing the AC induction motor drive parameters
//! to the serial user interface module.
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
    // The minimum motor speed.  This is specified in 1/10th of a Hz, ranging
    // from 0 Hz to 400 Hz.
    //
    {
        PARAM_MIN_SPEED,
        2,
        0,
        4000,
        1,
        (unsigned char *)&(g_sParameters.usMinFrequency),
        0
    },

    //
    // The maximum motor speed.  This is specified in 1/10th of a Hz, ranging
    // from 0 Hz to 400 Hz.
    //
    {
        PARAM_MAX_SPEED,
        2,
        0,
        4000,
        1,
        (unsigned char *)&(g_sParameters.usMaxFrequency),
        UIFAdjI
    },

    //
    // The target motor speed.  This is specified in 1/10th of a Hz, ranging
    // from 0 Hz to 400 Hz.
    //
    {
        PARAM_TARGET_SPEED,
        2,
        0,
        4000,
        1,
        (unsigned char *)&(g_usTargetFrequency),
        MainSetFrequency
    },

    //
    // The current motor speed.  This is specified in 1/10th of a Hz, ranging
    // from 0 Hz to 400 Hz.  This is a read-only parameter.
    //
    {
        PARAM_CURRENT_SPEED,
        2,
        0,
        4000,
        0,
        (unsigned char *)&g_usCurrentFrequency,
        0
    },

    //
    // The acceleration rate for the motor drive.  This is specified in Hz per
    // second, ranging from 1 Hz/sec^2 to 100 Hz/sec^2.
    //
    {
        PARAM_ACCEL,
        1,
        1,
        100,
        1,
        &(g_sParameters.ucAccel),
        0
    },

    //
    // The deceleration rate for the motor drive.  This is specified in Hz per
    // second, ranging from 1 Hz/sec^2 to 100 Hz/sec^2.
    //
    {
        PARAM_DECEL,
        1,
        1,
        100,
        1,
        &(g_sParameters.ucDecel),
        0
    },

    //
    // Selection of open-loop or closed-loop mode.  This is a boolean that is
    // true for closed-loop mode.
    //
    {
        PARAM_CLOSED_LOOP,
        1,
        0,
        1,
        1,
        &g_ucLoop,
        UILoopMode
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
    // The type of modulation to be used to drive the motor.  When the value is
    // zero, sine wave modulation is used.  When the value is one, space vector
    // modulation is used.
    //
    {
        PARAM_MODULATION,
        1,
        0,
        1,
        1,
        &g_ucModulation,
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
    // The V/f table mapping motor drive frequency to motor drive voltage.
    // Each entry of this table is a 1.15 fixed-point number providing the
    // amplitude of the drive.
    //
    {
        PARAM_VF_TABLE,
        42,
        0,
        0,
        1,
        (unsigned char *)&(g_sParameters.usVFTable),
        0
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
        3,
        1,
        &g_ucFrequency,
        UIPWMFrequencySet
    },

    //
    // The dead-time between switching off one side of a motor phase (high or
    // low) and turning on the other.  This is specified in nanoseconds,
    // ranging from 20 ns to 5120 ns.
    //
    {
        PARAM_PWM_DEAD_TIME,
        1,
        100,
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
    // microsecond, ranging from 0 us to 5 us.
    //
    {
        PARAM_PWM_MIN_PULSE,
        1,
        0,
        50,
        1,
        &(g_sParameters.ucMinPulseWidth),
        PWMSetMinPulseWidth
    },

    //
    // The type of motor connected to the motor drive.  When this is zero, a
    // three phase motor is in use.  When this is one, a single phase motor is
    // in use.
    //
    {
        PARAM_MOTOR_TYPE,
        1,
        0,
        1,
        1,
        &g_ucType,
        UIMotorType
    },

    //
    // The number of poles in the motor.  This ranges from 1 to 256 poles.
    //
    {
        PARAM_NUM_POLES,
        1,
        0,
        255,
        1,
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
    // in 1/10ths of an ampere, ranging from 0 A to 5 A.
    //
    {
        PARAM_MIN_CURRENT,
        1,
        0,
        50,
        1,
        &(g_sParameters.ucMinCurrent),
        0
    },

    //
    // The maximum allowable drive current during operation.  This is specified
    // in 1/10ths of an ampere, ranging from 0 A to 5 A.
    //
    {
        PARAM_MAX_CURRENT,
        1,
        0,
        50,
        1,
        &(g_sParameters.ucMaxCurrent),
        0
    },

    //
    // The minimum allowable bus voltage during operation.  This is specified
    // in volts, ranging from 1 V to 400 V.
    //
    {
        PARAM_MIN_BUS_VOLTAGE,
        2,
        1,
        400,
        1,
        (unsigned char *)&(g_sParameters.usMinVBus),
        0
    },

    //
    // The maximum allowable bus voltage during operation.  This is specified
    // in volts, ranging from 1 V to 400 V.
    //
    {
        PARAM_MAX_BUS_VOLTAGE,
        2,
        1,
        400,
        1,
        (unsigned char *)&(g_sParameters.usMaxVBus),
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
    // volts, ranging from 1 V to 400 V.
    //
    {
        PARAM_BRAKE_ON_VOLTAGE,
        2,
        1,
        400,
        1,
        (unsigned char *)&(g_sParameters.usBrakeOnV),
        0
    },

    //
    // The voltage at which the brake circuit is disengaged.  This is specified
    // in volts, ranging from 1 V to 400 V.
    //
    {
        PARAM_BRAKE_OFF_VOLTAGE,
        2,
        1,
        400,
        1,
        (unsigned char *)&(g_sParameters.usBrakeOffV),
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
    // This indicates if the DC bus voltage compensation should be utilized.
    // When one, DC bus voltage compensation is active, and when zero it is
    // not.
    //
    {
        PARAM_USE_BUS_COMP,
        1,
        0,
        1,
        1,
        &g_ucBusComp,
        UIBusComp
    },

    //
    // This indicates the range of the V/f table.  When zero, the V/f table
    // ranges from 0 Hz to 100 Hz in 5 Hz increments; when one, the V/f table
    // ranges from 0 Hz to 400 Hz in 20 Hz increments.
    //
    {
        PARAM_VF_RANGE,
        1,
        0,
        1,
        1,
        &g_ucVfRange,
        UIVfRange
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
    // This indicates if DC braking should be utilized.  When one, DC braking
    // is active, and when zero it is not.
    //
    {
        PARAM_USE_DC_BRAKE,
        1,
        0,
        1,
        1,
        &g_ucDCBrake,
        UIDCBrake
    },

    //
    // The DC voltage to be applied during DC braking, specified in volts.
    //
    {
        PARAM_DC_BRAKE_V,
        2,
        0,
        160,
        1,
        (unsigned char *)&(g_sParameters.usDCBrakeV),
        0
    },

    //
    // The amount of time to apply DC braking, specified in milliseconds.
    //
    {
        PARAM_DC_BRAKE_TIME,
        2,
        0,
        65535,
        1,
        (unsigned char *)&(g_sParameters.usDCBrakeTime),
        0
    },

    //
    // The voltage at which the deceleration rate is reduced.  This is
    // specified in volts, raning from 1 V to 400 V.
    //
    {
        PARAM_DECEL_VOLTAGE,
        2,
        1,
        400,
        1,
        (unsigned char *)&(g_sParameters.usDecelV),
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
    // specified in 1/10ths of an ampere, ranging from 0 A to 5 A.
    //
    {
        PARAM_ACCEL_CURRENT,
        1,
        0,
        50,
        1,
        &(g_sParameters.ucAccelCurrent),
        0
    }
};

//*****************************************************************************
//
//! The number of motor drive parameters.  This is used by the serial user
//! interface module.
//
//*****************************************************************************
const unsigned long g_ulUINumParameters = (sizeof(g_sUIParameters) /
                                           sizeof(g_sUIParameters[0]));

//*****************************************************************************
//
//! An array of structures describing the AC induction motor drive real-time
//! data items to the serial user interface module.
//
//*****************************************************************************
const tUIRealTimeData g_sUIRealTimeData[] =
{
    //
    // The RMS current through phase U of the motor.  This is an 8.8 fixed
    // point value providing the current in amperes.
    //
    {
        DATA_PHASE_A_CURRENT,
        2,
        (unsigned char *)&(g_pusPhaseCurrentRMS[0])
    },

    //
    // The RMS current through phase V of the motor.  This is an 8.8 fixed
    // point value providing the current in amperes.
    //
    {
        DATA_PHASE_B_CURRENT,
        2,
        (unsigned char *)&(g_pusPhaseCurrentRMS[1])
    },

    //
    // The RMS current through phase W of the motor.  This is an 8.8 fixed
    // point value providing the current in amperes.
    //
    {
        DATA_PHASE_C_CURRENT,
        2,
        (unsigned char *)&(g_pusPhaseCurrentRMS[2])
    },

    //
    // The RMS current through the entire motor.  This is an 8.8 fixed point
    // value providing the current in amperes.
    //
    {
        DATA_MOTOR_CURRENT,
        2,
        (unsigned char *)&g_usMotorCurrent
    },

    //
    // The voltage of the DC bus.  This is a 16-bit value providing the voltage
    // in volts.
    //
    {
        DATA_BUS_VOLTAGE,
        2,
        (unsigned char *)&g_usBusVoltage
    },

    //
    // The frequency of the motor drive.  This is a 16-bit value providing the
    // frequency in 1/10 of a Hz.
    //
    {
        DATA_STATOR_SPEED,
        2,
        (unsigned char *)&g_usCurrentFrequency
    },

    //
    // The frequency of the rotor.  This is a 16-bit value providing the
    // frequency in 1/10 of a Hz.
    //
    {
        DATA_ROTOR_SPEED,
        2,
        (unsigned char *)&g_usRotorFrequency
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
    }
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
//! Updates the open-/closed-loop mode bit of the motor drive.
//!
//! This function is called when the variable controlling open-/closed-loop
//! mode of the motor drive is updated.  The value is then reflected into the
//! usFlags member of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UILoopMode(void)
{
    //
    // If there is no encoder then closed-loop mode is not possible.
    //
    if(HWREGBITH(&(g_sParameters.usFlags), FLAG_ENCODER_BIT) ==
       FLAG_ENCODER_ABSENT)
    {
        g_ucLoop = 0;
    }

    //
    // Set the open-/closed-loop mode for the motor drive.
    //
    MainSetLoopMode(g_ucLoop);
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
    // Update the encoder presence flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_ENCODER_BIT) = g_ucEncoder;

    //
    // Update the open-/closed-loop mode state of the motor drive.  If the
    // encoder is not present, then closed-loop mode is not possible.
    //
    UILoopMode();
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
        g_ucModulation = HWREGBITH(&(g_sParameters.usFlags), FLAG_DRIVE_BIT);

        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // If the motor drive is configured for a single phase motor, then only
    // allow sine wave modulation to be used.
    //
    if(g_ucType == 1)
    {
        g_ucModulation = 0;
    }

    //
    // Update the modulation waveform type flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_DRIVE_BIT) = g_ucModulation;

    //
    // Turn on the second status light if using space vector modulation.
    //
    if(g_ucModulation == 1)
    {
        UIStatus2LEDBlink(1, 1);
    }
    else
    {
        UIStatus2LEDBlink(0, 0);
    }
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
    // If the motor drive is configured for a single phase motor, then only
    // allow the direction to be forward.
    //
    if(g_ucType == 1)
    {
        g_ucDirection = 0;
    }

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
    // See if the value is zero.
    //
    if(g_ucFrequency == 0)
    {
        //
        // Set the frequency to 8 KHz.
        //
        g_sParameters.usFlags = ((g_sParameters.usFlags &
                                  ~FLAG_PWM_FREQUENCY_MASK) |
                                 FLAG_PWM_FREQUENCY_8K);
    }

    //
    // See if the value is one.
    //
    else if(g_ucFrequency == 1)
    {
        //
        // Set the frequency to 12.5 KHz.
        //
        g_sParameters.usFlags = ((g_sParameters.usFlags &
                                  ~FLAG_PWM_FREQUENCY_MASK) |
                                 FLAG_PWM_FREQUENCY_12K);
    }

    //
    // See if the value is two.
    //
    else if(g_ucFrequency == 2)
    {
        //
        // Set the frequency to 16 KHz.
        //
        g_sParameters.usFlags = ((g_sParameters.usFlags &
                                  ~FLAG_PWM_FREQUENCY_MASK) |
                                 FLAG_PWM_FREQUENCY_16K);
    }

    //
    // Otherwise, assume that the value is three.
    //
    else
    {
        //
        // Set the frequency to 20 KHz.
        //
        g_sParameters.usFlags = ((g_sParameters.usFlags &
                                  ~FLAG_PWM_FREQUENCY_MASK) |
                                 FLAG_PWM_FREQUENCY_20K);
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
//! Updates the type of motor connected to the motor drive.
//!
//! This function is called when the variable specifying the type of motor
//! connected to the motor drive is updated.  This value is then reflected into
//! the usFlags member of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIMotorType(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // The motor type can not changed when the motor drive is running (that
        // would be catastrophic!), so revert the motor type variable back to
        // the value in the flags.
        //
        g_ucType = HWREGBITH(&(g_sParameters.usFlags), FLAG_MOTOR_TYPE_BIT);
 
        //
        // There is nothing further to do.
        //
        return;
    }

    //
    // Update the motor type flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_MOTOR_TYPE_BIT) = g_ucType;

    //
    // See if the motor type was changed to single phase.
    //
    if(g_ucType == 1)
    {
        //
        // Single phase motors can only be driven forward with sine wave
        // modulation, so force the motor drive to those conditions.
        //
        UIDirectionSet();
        UIModulationType();
    }
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
//! Updates the DC bus compensation bit of the motor drive.
//!
//! This function is called when the variable controlling the DC bus
//! compensation is updated.  The value is then reflected into the usFlags
//! member of #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIBusComp(void)
{
    //
    // Update the bus voltage compensation flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_BUS_COMP_BIT) = g_ucBusComp;
}

//*****************************************************************************
//
//! Updates the V/f table range of the motor drive.
//!
//! This function is called when the variable controlling the V/f table range
//! is updated.  The value is then reflected into the usFlags member of
//! #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIVfRange(void)
{
    //
    // Update the V/f range flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_VF_RANGE_BIT) = g_ucVfRange;
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
//! Update the DC brake bit of the motor drive.
//!
//! This function is called when the variable controlling the DC braking is
//! updated.  The value is then reflected into the usFlags member of
//! #g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
static void
UIDCBrake(void)
{
    //
    // Update the DC braking flag in the flags variable.
    //
    HWREGBITH(&(g_sParameters.usFlags), FLAG_DC_BRAKE_BIT) = g_ucDCBrake;
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
//! stop command is received.  In the case of an AC induction motor, an
//! emergency stop is treated as a "protect the motor drive" command;
//! mechanical braking must be utilized in an emergency stop situation.
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
//! changes, such as changing the motor type from three phase to single phase).
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
        // See if this is a version zero parameter block.
        //
        if(g_sParameters.ucVersion == 0)
        {
            //
            // Fill in default values for the new parameters added to the
            // version one parameter block.
            //
            g_sParameters.ucAccelCurrent = 48;

            //
            // Set the parameter block version to one.
            //
            g_sParameters.ucVersion = 1;
        }
    }

    //
    // Set the local variables (used by the serial interface) based on the
    // values in the parameter block values.
    //
    g_ucLoop = HWREGBITH(&(g_sParameters.usFlags), FLAG_LOOP_BIT);
    g_ucEncoder = HWREGBITH(&(g_sParameters.usFlags), FLAG_ENCODER_BIT);
    g_ucModulation = HWREGBITH(&(g_sParameters.usFlags), FLAG_DRIVE_BIT);
    g_ucDirection = HWREGBITH(&(g_sParameters.usFlags), FLAG_DIR_BIT);
    g_ucFrequency = g_sParameters.usFlags & FLAG_PWM_FREQUENCY_MASK;
    g_ucUpdateRate = g_sParameters.ucUpdateRate;
    g_ucType = HWREGBITH(&(g_sParameters.usFlags), FLAG_MOTOR_TYPE_BIT);
    g_lFAdjI = g_sParameters.lFAdjI;
    g_ucBusComp = HWREGBITH(&(g_sParameters.usFlags), FLAG_BUS_COMP_BIT);
    g_ucVfRange = HWREGBITH(&(g_sParameters.usFlags), FLAG_VF_RANGE_BIT);
    g_ucDynamicBrake = HWREGBITH(&(g_sParameters.usFlags), FLAG_BRAKE_BIT);
    g_ucDCBrake = HWREGBITH(&(g_sParameters.usFlags), FLAG_DC_BRAKE_BIT);

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
//! \return Never returns.
//
//*****************************************************************************
void
UIUpgrade(void)
{
    //
    // Emergency stop the motor drive.
    //
    MainEmergencyStop();

    //
    // Disable all processor interrupts.  Instead of disabling them one at a
    // time (and possibly missing an interrupt if new sources are added), a
    // direct write to NVIC is done to disable all peripheral interrupts.
    //
    HWREG(NVIC_DIS0) = 0xffffffff;

    //
    // Also disable the SysTick interrupt.
    //
    SysTickIntDisable();

    //
    // Turn off all the on-board LEDs.
    //
    UIRunLEDBlink(0, 0);
    UIFaultLEDBlink(0, 0);
    UIStatus1LEDBlink(0, 0);
    UIStatus2LEDBlink(0, 0);

    //
    // Stop running from the PLL.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_6MHZ);

    //
    // Reconfigure the UART for 115,200, 8-N-1 operation with new clock.
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE |
                         UART_CONFIG_STOP_ONE));

    //
    // Adjust the PWM drive to the in-rush relay to account for the slower
    // processor clock rate.
    //
    InRushRelayAdjust();

    //
    // Return control to the boot loader.  This is a call to the SVC handler in
    // the boot loader.
    //
    (*((void (*)(void))(*(unsigned long *)0x2c)))();

    //
    // Control should never return here, but just in case it does.
    //
    while(1)
    {
    }
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
static void
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
    g_ucModulation ^= 1;
    UIModulationType();
}

//*****************************************************************************
//
//! Handles the GPIO port D interrupt.
//!
//! This function is called when GPIO port D asserts its interrupt.  GPIO port
//! D is configured to generate an interrupt on either edge of the signal from
//! the potentiometer oscillator.  The time between the current edge and the
//! previous edge is computed.
//!
//! \return None.
//
//*****************************************************************************
void
GPIODIntHandler(void)
{
    unsigned long ulCurrentTime;

    //
    // Clear the GPIO interrupt.
    //
    GPIOPinIntClear(PIN_POTENTIOMETER_PORT, PIN_POTENTIOMETER_PIN);

    //
    // Get the current value of the SysTick timer.
    //
    ulCurrentTime = SysTickValueGet();

    //
    // See if the SysTick timer rolled over.
    //
    if(ulCurrentTime > g_ulUIPotPreviousTime)
    {
        //
        // The SysTick timer rolled over, so compute the time based on the roll
        // over.
        //
        g_ulUIPotEdgeTime = ((SYSTEM_CLOCK / UI_INT_RATE) +
                             g_ulUIPotPreviousTime - ulCurrentTime);
    }
    else
    {
        //
        // The SysTick timer did not roll over, so the timer difference is the
        // difference in the two readings.
        //
        g_ulUIPotEdgeTime = g_ulUIPotPreviousTime - ulCurrentTime;
    }

    //
    // Save the current time as the previous edge time.
    //
    g_ulUIPotPreviousTime = ulCurrentTime;
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
//! Sets the blink rate for the status1 LED.
//!
//! \param usRate is the rate to blink the status1 LED.
//! \param usPeriod is the amount of time to turn on the status1 LED.
//!
//! This function sets the rate at which the status1 LED should be blinked.  A
//! blink period of zero means that the LED should be turned off, and a blink
//! period equal to the blink rate means that the LED should be turned on.
//! Otherwise, the blink rate determines the number of user interface
//! interrupts during the blink cycle of the status1 LED, and the blink period
//! is the number of those user interface interrupts during which the LED is
//! turned on.
//!
//! \return None.
//
//*****************************************************************************
void
UIStatus1LEDBlink(unsigned short usRate, unsigned short usPeriod)
{
    //
    // The status1 LED is the third LED.
    //
    UILEDBlink(2, usRate, usPeriod);
}

//*****************************************************************************
//
//! Sets the blink rate for the status2 LED.
//!
//! \param usRate is the rate to blink the status2 LED.
//! \param usPeriod is the amount of time to turn on the status2 LED.
//!
//! This function sets the rate at which the status2 LED should be blinked.  A
//! blink period of zero means that the LED should be turned off, and a blink
//! period equal to the blink rate means that the LED should be turned on.
//! Otherwise, the blink rate determines the number of user interface
//! interrupts during the blink cycle of the status2 LED, and the blink period
//! is the number of those user interface interrupts during which the LED is
//! turned on.
//!
//! \return None.
//
//*****************************************************************************
void
UIStatus2LEDBlink(unsigned short usRate, unsigned short usPeriod)
{
    //
    // The status2 LED is the fourth LED.
    //
    UILEDBlink(3, usRate, usPeriod);
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
    // See if the on-board user interface is enabled.
    //
    if(g_ulUIUseOnboard == 1)
    {
        //
        // Filter the potentiometer value.
        //
        ulCount = UIOnboardPotentiometerFilter(g_ulUIPotEdgeTime / 512);

        //
        // If the potentiometer value is outside the valid range then clip it
        // to the valid range.  There is some guard-banding here to account for
        // component variations and ensure that the full frequency range is
        // available.
        //
        if(ulCount < UI_POT_MIN)
        {
            ulCount = UI_POT_MIN;
        }
        if(ulCount > UI_POT_MAX)
        {
            ulCount = UI_POT_MAX;
        }

        //
        // Set the target motor drive frequency based on the filtered
        // potentiometer value.
        //
        g_usTargetFrequency = ((((ulCount - UI_POT_MIN) *
                                 (g_sParameters.usMaxFrequency -
                                  g_sParameters.usMinFrequency)) /
                                (UI_POT_MAX - UI_POT_MIN)) +
                               g_sParameters.usMinFrequency);
        MainSetFrequency();

        //
        // Read the on-board switch and pass its current value to the switch
        // debouncer.
        //
        UIOnboardSwitchDebouncer(GPIOPinRead(PIN_SWITCH_PORT, PIN_SWITCH_PIN));
    }

    //
    // Compute the new value for the processor usage.
    //
    g_ucCPUUsage = (CPUUsageTick() + 32768) / 65536;

    //
    // Increment the blink counter.
    //
    g_ulBlinkCount++;

    //
    // Loop through the four LEDs.
    //
    for(ulIdx = 0; ulIdx < 4; ulIdx++)
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
    UISerialSendRealTimeData();
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
    //
    // Make the push button and potentiometer oscillator pins be GPIO inputs.
    //
    GPIODirModeSet(PIN_SWITCH_PORT, PIN_SWITCH_PIN, GPIO_DIR_MODE_IN);
    GPIODirModeSet(PIN_POTENTIOMETER_PORT, PIN_POTENTIOMETER_PIN,
                   GPIO_DIR_MODE_IN);

    //
    // Make the LEDs be GPIO outputs and turn them off.
    //
    GPIODirModeSet(PIN_LEDRUN_PORT, PIN_LEDRUN_PIN, GPIO_DIR_MODE_OUT);
    GPIODirModeSet(PIN_LEDFAULT_PORT, PIN_LEDFAULT_PIN, GPIO_DIR_MODE_OUT);
    GPIODirModeSet(PIN_LEDSTATUS1_PORT, PIN_LEDSTATUS1_PIN, GPIO_DIR_MODE_OUT);
    GPIODirModeSet(PIN_LEDSTATUS2_PORT, PIN_LEDSTATUS2_PIN, GPIO_DIR_MODE_OUT);
    GPIOPinWrite(PIN_LEDRUN_PORT, PIN_LEDRUN_PIN, PIN_LEDRUN_PIN);
    GPIOPinWrite(PIN_LEDFAULT_PORT, PIN_LEDFAULT_PIN, 0);
    GPIOPinWrite(PIN_LEDSTATUS1_PORT, PIN_LEDSTATUS1_PIN, 0);
    GPIOPinWrite(PIN_LEDSTATUS2_PORT, PIN_LEDSTATUS2_PIN, 0);

    //
    // Configure the potentiometer oscillator pin to interrupt on both edges,
    // and enable the GPIO interrupt.
    //
    GPIOIntTypeSet(PIN_POTENTIOMETER_PORT, PIN_POTENTIOMETER_PIN,
                   GPIO_BOTH_EDGES);
    GPIOPinIntEnable(PIN_POTENTIOMETER_PORT, PIN_POTENTIOMETER_PIN);
    IntEnable(INT_GPIOD);

    //
    // Initialize the serial user interface.
    //
    UISerialInit();

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
