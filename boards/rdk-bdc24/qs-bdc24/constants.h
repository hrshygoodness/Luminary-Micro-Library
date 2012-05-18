//*****************************************************************************
//
// constants.h - Constants used in the system.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

//*****************************************************************************
//
// The system clock rate.
//
//*****************************************************************************
#define SYSCLK                  16000000

//*****************************************************************************
//
// The frequency of the PWM output.  This should be an integral divisor of
// SYSCLK, though that is not absolutely required.
//
//*****************************************************************************
#define PWM_FREQUENCY           15625

//*****************************************************************************
//
// The minimum pulse width of the PWM outputs, used to prevent runt pulses
// being sent to the HBridge.
//
//*****************************************************************************
#define PWM_MIN_WIDTH           ((unsigned long)(SYSCLK * 0.0000022))

//*****************************************************************************
//
// The number of system clock ticks during each PWM output period.
//
//*****************************************************************************
#define SYSCLK_PER_PWM_PERIOD   (SYSCLK / PWM_FREQUENCY)

//*****************************************************************************
//
// The number of controller updates per second.
//
//*****************************************************************************
#define UPDATES_PER_SECOND      1000

//*****************************************************************************
//
// The number of system clock ticks during each controller update period.
//
//*****************************************************************************
#define SYSCLK_PER_UPDATE       (SYSCLK / UPDATES_PER_SECOND)

//*****************************************************************************
//
// The length of time the motor is held in neutral after a fault condition is
// detected, in controller update periods.
//
//*****************************************************************************
#define FAULT_TIME              (3 * UPDATES_PER_SECOND)

//*****************************************************************************
//
// The number of system clock ticks before the watchdog expires.  The watchdog
// is pet whenever a CAN message or servo signal is received, preventing it
// from expiring.  Expiration of the watchdog is an error condition.
//
//*****************************************************************************
#define WATCHDOG_PERIOD         (SYSCLK * 0.2)

//*****************************************************************************
//
// The length of time that the button must be sampled in the same state in
// order to debounce.
//
//*****************************************************************************
#define BUTTON_DEBOUNCE_COUNT   10

//*****************************************************************************
//
// The length of time that the button must be held in order to invoke the
// button hold function instead of the button press function.
//
//*****************************************************************************
#define BUTTON_HOLD_COUNT       (5 * UPDATES_PER_SECOND)

//*****************************************************************************
//
// The ambient temperature at which the motor controller will be forced into
// neutral.  This value is the fixed-point 8.8 temperature in degrees Celcius.
//
//*****************************************************************************
#define SHUTDOWN_TEMPERATURE    (60 * 256)

//*****************************************************************************
//
// The amount of hysteresis to apply to the ambient temperature at which the
// motor controller will be forced into neutral.  This amount is added to the
// ambient temperature setpoint when determining if the motor controller should
// be shut down and subtracted when determining if it should no longer be shut
// down.  This value is the fixed-point 8.8 temperature delta in degrees
// Celcius.
//
//*****************************************************************************
#define SHUTDOWN_TEMPERATURE_HYSTERESIS \
                                (1 * 256)

//*****************************************************************************
//
// The bus voltage at which the motor controller will be forced into neutral.
// This value is the fixed-point 24.8 voltage.
//
//*****************************************************************************
#define SHUTDOWN_VOLTAGE        (6 * 256)

//*****************************************************************************
//
// The amount of time the bus voltage must be below the shutdown voltage before
// the motor controller will be forced into neutral.
//
//*****************************************************************************
#define SHUTDOWN_VOLTAGE_TIME   ((unsigned long)(PWM_FREQUENCY * 0.1))

//*****************************************************************************
//
// The length of time the fan is left on after the motor is put into neutral.
// This time will be extended as required if the ambient temperature is too
// high.
//
//*****************************************************************************
#define FAN_COOLING_TIME        (10 * UPDATES_PER_SECOND)

//*****************************************************************************
//
// This is the time the fan should turn on to test that it is operational.
// Time is measured in UPDATES_PER_SECOND.
//
//*****************************************************************************
#define FAN_TEST_TIME           1000

//*****************************************************************************
//
// The ambient temperature at which the fan will be turned on.  The fan will be
// on if the ambient temperature exceeds this value, even if the motor is in
// neutral.  This value is the fixed-point 24.8 temperature in degrees Celcius.
//
//*****************************************************************************
#define FAN_TEMPERATURE         (40 * 256)

//*****************************************************************************
//
// The amount of hysteresis to apply to the ambient temperature at which the
// fan is turned on.  This amount is added to the ambient temperature setpoint
// when determining if the fan should be turned on and subtracted when
// determining if the fan should be turned off.  This value is the fixed-point
// 8.8 temperature delta in degrees Celcius.
//
//*****************************************************************************
#define FAN_HYSTERESIS          (2 * 256)

//*****************************************************************************
//
// The address of the last block of flash to be used for storing parameters.
// Since the end of flash is used for parameters, this is actually the first
// address past the end of flash.
//
//*****************************************************************************
#define FLASH_PB_END            0x20000

//*****************************************************************************
//
// The address of the first block of flash to be used for storing parameters.
//
//*****************************************************************************
#define FLASH_PB_START          (FLASH_PB_END - 0x800)

//*****************************************************************************
//
// The size of the parameter block to save.  This must be a power of 2, and
// should be large enough to contain the tParameters structure.
//
//*****************************************************************************
#define FLASH_PB_SIZE           64

//*****************************************************************************
//
// The default mininum servo pulse width, specified in system clocks.
//
//*****************************************************************************
#define SERVO_DEFAULT_MIN_WIDTH ((unsigned long)(SYSCLK * 0.000671325))

//*****************************************************************************
//
// The default neutral servo pulse width, specified in system clocks.
//
//*****************************************************************************
#define SERVO_DEFAULT_NEU_WIDTH ((unsigned long)(SYSCLK * 0.0015))

//*****************************************************************************
//
// The default maximum servo pulse width, specified in system clocks.
//
//*****************************************************************************
#define SERVO_DEFAULT_MAX_WIDTH ((unsigned long)(SYSCLK * 0.002328675))

//*****************************************************************************
//
// The width of the plateau at the full reverse end of the voltage curve.  Any
// voltage command within this plateau will result in the motor operating in
// full reverse.
//
//*****************************************************************************
#define REVERSE_PLATEAU         1024

//*****************************************************************************
//
// The width of the plateau around neutral in the voltage curve.  Any voltage
// command within this plateau will result in the motor staying in neutral.
//
//*****************************************************************************
#define NEUTRAL_PLATEAU         2048

//*****************************************************************************
//
// The width of the plateau at the full forward end of the voltage curve.  Any
// voltage command within this plateau will result in the motor operating in
// full forward.
//
//*****************************************************************************
#define FORWARD_PLATEAU         1024

//*****************************************************************************
//
// The maximum amount of deviation allowed in the neutral servo input pulse
// width when performing a calibration, specified in system clocks.
//
//*****************************************************************************
#define SERVO_NEUTRAL_SLOP      ((unsigned long)(SYSCLK * 0.0001))

//*****************************************************************************
//
// The minimum deviation in the servo input between neutral and the minimum and
// maximum pulses when performing a calibration, specified in system clocks.
//
//*****************************************************************************
#define SERVO_RANGE_MIN         ((unsigned long)(SYSCLK * 0.00025))

//*****************************************************************************
//
// The minimum servo input period, specified in system clocks.  A fault is
// triggered if the servo input period is less than this value.
//
//*****************************************************************************
#define SERVO_MIN_PERIOD        ((unsigned long)(SYSCLK * 0.005))

//*****************************************************************************
//
// The maximum servo input period, specified in system clocks.  A fault is
// triggered if the servo input period is greater than this value.
//
//*****************************************************************************
#define SERVO_MAX_PERIOD        ((unsigned long)(SYSCLK * 0.03))

//*****************************************************************************
//
// The minimum servo input pulse width, specified in system clocks.  A fault is
// triggered if the servo input pulse width is less than this value.
//
//*****************************************************************************
#define SERVO_MIN_PULSE_WIDTH   ((unsigned long)(SYSCLK * 0.0005))

//*****************************************************************************
//
// The maximum servo input pulse width, specified in system clocks.  A fault is
// triggered if the servo input pulse width is greater than this value.
//
//*****************************************************************************
#define SERVO_MAX_PULSE_WIDTH   ((unsigned long)(SYSCLK * 0.0025))

//*****************************************************************************
//
// The minimum output current of the motor controller, specified as an 8.8
// fixed-point value.  This value is simply the second control point on the
// exponential curve used to control the over-current shutdown time; it does
// not affect the output current capability of the motor controller in any way.
//
//*****************************************************************************
#define CURRENT_MINIMUM_LEVEL   (40 * 256)

//*****************************************************************************
//
// The nominal output current of the motor controller, specified as an 8.8
// fixed-point value.  The motor controller will supply up to this current
// level indefinitely.
//
//*****************************************************************************
#define CURRENT_NOMINAL_LEVEL   (50 * 256)

//*****************************************************************************
//
// The shutoff output current of the motor controller, specified as an 8.8
// fixed-point value.  The motor controller will shut off the outputs if the
// current is at this level for the shutoff amount of time.
//
//*****************************************************************************
#define CURRENT_SHUTOFF_LEVEL   (60 * 256)

//*****************************************************************************
//
// The amount of time the output current must be at the shutoff level before
// the output is shut off, specified as the number of PWM periods.
//
//*****************************************************************************
#define CURRENT_SHUTOFF_TIME    (2 * PWM_FREQUENCY)

//*****************************************************************************
//
// The maximum amount of time after an encoder input edge to wait for another
// input edge, specified in milliseconds.  If this time is exceeded, the
// measured speed is forced to zero.
//
//*****************************************************************************
#define ENCODER_WAIT_TIME       100

//*****************************************************************************
//
// The Voltage mode ramp rate in steps/ms when Automatic Ramp mode is enabled.
//
//*****************************************************************************
#define AUTO_RAMP_RATE          524

#endif // __CONSTANTS_H__
