//*****************************************************************************
//
// ui.h - Prototypes for the user interface and compiler defines.
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
// This is part of revision 10636 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#ifndef __UI_H__
#define __UI_H__

//*****************************************************************************
//
// This file contains the basic user interface prototypes, along with the
// compiler defines that are used to cusomize the basic BLDC code for a
// specific motor type and drive configuration.
//
// A typical development scenario would be to use the qs-bldc application,
// along with the BLDC GUI to experiment with the motor control parameters.
// Once the proper set of parameters have been determined for the targeted
// motor, the parameters can be defined in this file and the application
// can be recompiled for the targeted motor.
//
// NOTE:  Once the basic bldc application has been programmed into the board,
// the BLDC GUI will not be functional until the qs-bldc application along
// with the qs-bl_bldc boot loader has been restored to the board (using a
// JTAG programmer).
//
// The parameters in this file are organized in the same groupings as the 
// BLDC GUI configuration panels.
//
//*****************************************************************************




//*****************************************************************************
//
// The following set of parameters correspond to the parameters that are
// configurable on the "Main" panel of the BLDC GUI.
//
//*****************************************************************************
//
// The direction in which to drive the motor.
//
#define UI_PARAM_DIRECTION          DIRECTION_FORWARD
//#define UI_PARAM_DIRECTION          DIRECTION_BACKWARD


//
// The target speed setting in RPM.
//
#define UI_PARAM_TARGET_SPEED       3000

//
// The target power setting in milliwatts.
//
#define UI_PARAM_TARGET_POWER       0




//*****************************************************************************
//
// The following set of parameters correspond to the parameters that are
// configurable on the "Drive Configuration" tab of the "Configuration" panel
// of the BLDC GUI.
//
//*****************************************************************************
//
// The type of motor drive algorithm that should be used.
//
#define UI_PARAM_MODULATION         MODULATION_TRAPEZOID
//#define UI_PARAM_MODULATION         MODULATION_SENSORLESS
//#define UI_PARAM_MODULATION         MODULATION_SINE

//
// The control mode for the motor drive algorithm.
//
#define UI_PARAM_CONTROL_MODE       CONTROL_MODE_SPEED
//#define UI_PARAM_CONTROL_MODE       CONTROL_MODE_POWER

//
// The minimum current through the motor drive during operation, specified
// in milli-amperes.  A value of zero indicated that this parameter is
// not used.
//
#define UI_PARAM_MIN_CURRENT        0

//
// The maximum current through the motor drive during operation, specified
// in milli-amperes.  A value of zero indicates that this parameter is
// not used.
//
#define UI_PARAM_MAX_CURRENT        10000

//
// The motor current limit for motor operation, specified in milliamps.  A
// value of zero indicates that this parameter is not used.
//
#define UI_PARAM_TARGET_CURRENT     0

//
// The minimum speed of the motor drive, specified in RPM.
//
#define UI_PARAM_MIN_SPEED          200

//
// The maximum speed of the motor drive, specified in RPM.
//
#define UI_PARAM_MAX_SPEED          12000

//
// The rate of acceleration, specified in RPM per second.
//
#define UI_PARAM_ACCEL              5000

//
// The rate of deceleration, specified in RPM per second.
//
#define UI_PARAM_DECEL              5000

//
// The P coefficient of the speed adjust PID controller.
//
#define UI_PARAM_SPEED_P            (65536 * 2)

//
// The I coefficient of the speed adjust PID controller.
//
#define UI_PARAM_SPEED_I            1000

//
// The minimum power setting in milliwatts.
//
#define UI_PARAM_MIN_POWER          0

//
// The maximum power setting in milliwatts.
//
#define UI_PARAM_MAX_POWER          100000

//
// The rate of acceleration, specified in milliwatts per second.
//
#define UI_PARAM_ACCEL_POWER        1000

//
// The rate of deceleration, specified in milliwatts per second.
//
#define UI_PARAM_DECEL_POWER        1000

//
// The P coefficient of the power adjust PID controller.
//
#define UI_PARAM_POWER_P            (65536 * 2)

//
// The I coefficient of the power adjust PID controller.
//
#define UI_PARAM_POWER_I            1000




//*****************************************************************************
//
// The following set of parameters correspond to the parameters that are
// configurable on the "Bus / Temp Configuration" tab of the "Configuration"
// panel of the BLDC GUI.
//
//*****************************************************************************
//
// The minimum bus voltage during operation, specified in millivolts.
//
#define UI_PARAM_MIN_BUS_VOLTAGE    100

//
// The maximum bus voltage during operation, specified in millivolts.
//
#define UI_PARAM_MAX_BUS_VOLTAGE    40000

//
// The DC bus voltage at which the deceleration rate is reduced, specified
// in millivolts.
//
#define UI_PARAM_DECEL_VOLTAGE      36000

//
// The motor current at which the acceleration rate is reduced, specified
// in milli-amperes.
//
#define UI_PARAM_ACCEL_CURRENT      2000

//
// The flag to enable dynamic braking resistor.
//
#define UI_PARAM_USE_DYNAM_BRAKE    BRAKE_ON
//#define UI_PARAM_USE_DYNAM_BRAKE    BRAKE_OFF

//
// The amount of time (assuming continuous application) that the dynamic
// braking can be utilized, specified in milliseconds.
//
#define UI_PARAM_MAX_BRAKE_TIME     (60 * 1000)

//
// The amount of accumulated time that the dynamic brake can have before
// the cooling period will end, specified in milliseconds.
//
#define UI_PARAM_BRAKE_COOL_TIME    (55 * 1000)
//
// The bus voltage at which the braking circuit is engaged, specified in
// millivolts.
//
#define UI_PARAM_BRAKE_ON_VOLTAGE   38000

//
// The bus voltage at which the braking circuit is disengaged, specified
// in millivolts.
//
#define UI_PARAM_BRAKE_OFF_VOLTAGE  37000

//
// The maximum ambient temperature of the microcontroller, specified in
// degrees Celsius.
//
#define UI_PARAM_MAX_TEMPERATURE    85





//*****************************************************************************
//
// The following set of parameters correspond to the parameters that are
// configurable on the "PWM Configuration" tab of the "Configuration"
// panel of the BLDC GUI.
//
//*****************************************************************************
//
// The PWM frequency to use when driving the motor.
//
//#define UI_PARAM_PWM_FREQUENCY      PWM_FREQUENCY_8K
//#define UI_PARAM_PWM_FREQUENCY      PWM_FREQUENCY_12K
//#define UI_PARAM_PWM_FREQUENCY      PWM_FREQUENCY_16K
#define UI_PARAM_PWM_FREQUENCY      PWM_FREQUENCY_20K
//#define UI_PARAM_PWM_FREQUENCY      PWM_FREQUENCY_25K
//#define UI_PARAM_PWM_FREQUENCY      PWM_FREQUENCY_40K
//#define UI_PARAM_PWM_FREQUENCY      PWM_FREQUENCY_50K
//#define UI_PARAM_PWM_FREQUENCY      PWM_FREQUENCY_80K

//
// The dead time between inverting the high and low side of a motor phase,
// specified in 20 ns periods.
//
#define UI_PARAM_PWM_DEAD_TIME      3

//
// The amount of time to precharge the bootstrap capacitor on the high
// side gate drivers, specified in milliseconds.
//
#define UI_PARAM_PRECHARGE_TIME     3

//
// The PWM decay mode to use when driving the motor.
//
//#define UI_PARAM_DECAY_MODE         DECAY_SLOW
#define UI_PARAM_DECAY_MODE         DECAY_FAST

//
// The minimum width of a PWM pulse, specified in 0.1 us periods.
//
#define UI_PARAM_PWM_MIN_PULSE      25

//
// The rate at which the PWM pulse width is updated, specified in the
// number of PWM periods (add 1).
//
#define UI_PARAM_PWM_UPDATE         0





//*****************************************************************************
//
// The following set of parameters correspond to the parameters that are
// configurable on the "Motor Configuration" tab of the "Configuration"
// panel of the BLDC GUI.
//
//*****************************************************************************
//
// The type of hall sensor used for the bldc motor.
//
#define UI_PARAM_SENSOR_TYPE        SENSOR_TYPE_GPIO
//#define UI_PARAM_SENSOR_TYPE        SENSOR_TYPE_LINEAR
//#define UI_PARAM_SENSOR_TYPE        SENSOR_TYPE_GPIO_60

//
// The polarity of the hall sensor used for the BLDC motor.
//
#define UI_PARAM_SENSOR_POLARITY    SENSOR_POLARITY_HIGH
//#define UI_PARAM_SENSOR_POLARITY    SENSOR_POLARITY_LOW

//
// The flag to indicate if an optical encoder is present.
//
#define UI_PARAM_ENCODER_PRESENT    ENCODER_ABSENT
//#define UI_PARAM_ENCODER_PRESENT    ENCODER_PRESENT

//
// The number of lines in the (optional) optical encoder.
//
#define UI_PARAM_NUM_LINES          1000

//
// The number of poles.
//
#define UI_PARAM_NUM_POLES          2

//
// The skip count for Back EMF zero crossing detection hold-off.
//
#define UI_PARAM_BEMF_SKIP_COUNT    3

//
// The number of milliseconds to hold in sensorless startup.
//
#define UI_PARAM_STARTUP_COUNT      500

//
// The starting voltage for sensorless startup in millivolts.
//
#define UI_PARAM_STARTUP_STARTV     1200

//
// The starting speed for sensorless startup in RPM.
//
#define UI_PARAM_STARTUP_STARTSP    400

//
// The ending voltage for sensorless startup in millivolts.
//
#define UI_PARAM_STARTUP_ENDV       3600

//
// The ending speed for sensorless startup in RPM.
//
#define UI_PARAM_STARTUP_ENDSP      1500

//
// The open-loop sensorless ramp time, specified in milliseconds.
//
#define UI_PARAM_STARTUP_RAMP       500

//
// The sensorless startup BEMF threshold voltage, specified in millivolts.
//
#define UI_PARAM_STARTUP_THRESH     500




//*****************************************************************************
//
// This is the end of user configurable parameters.  Nothing below this line
// should be changed.
//
//*****************************************************************************




//*****************************************************************************
//
// The value for UI_PARAM_MODULATION that indicates that the motor is being
// driven with trapezoid modulation, using hall sensors.
//
//*****************************************************************************
#define MODULATION_TRAPEZOID        0

//*****************************************************************************
//
// The value for UI_PARAM_MODULATION that indicates that the motor is being
// driven with trapezoid modulation, in sensorless mode.
//
//*****************************************************************************
#define MODULATION_SENSORLESS       1

//*****************************************************************************
//
// The value for UI_PARAM_MODULATION that indicates that the motor is being
// driven with sinusoidal modulation, using hall sensors for position
// sensing.
//
//*****************************************************************************
#define MODULATION_SINE             2

//*****************************************************************************
//
// The value for UI_PARAM_CONTROL_MODE that indicates that the motor is being
// driven using speed as the closed loop control target.
//
//*****************************************************************************
#define CONTROL_MODE_SPEED          0

//*****************************************************************************
//
// The value for UI_PARAM_CONTROL_MODE that indicates that the motor is being
// driven using power as the closed loop control target.
//
//*****************************************************************************
#define CONTROL_MODE_POWER          1

//*****************************************************************************
//
// The value for UI_PARAM_PWM_FREQUENCY that indicates that the PWM frequency
// is 8 KHz.
//
//*****************************************************************************
#define PWM_FREQUENCY_8K            0

//*****************************************************************************
//
// The value for UI_PARAM_PWM_FREQUENCY that indicates that the PWM frequency
// is 12.5 KHz.
//
//*****************************************************************************
#define PWM_FREQUENCY_12K           1

//*****************************************************************************
//
// The value for UI_PARAM_PWM_FREQUENCY that indicates that the PWM frequency
// is 16 KHz.
//
//*****************************************************************************
#define PWM_FREQUENCY_16K           2

//*****************************************************************************
//
// The value for UI_PARAM_PWM_FREQUENCY that indicates that the PWM frequency
// is 20 KHz.
//
//*****************************************************************************
#define PWM_FREQUENCY_20K           3

//*****************************************************************************
//
// The value for UI_PARAM_PWM_FREQUENCY that indicates that the PWM frequency
// is 25 KHz.
//
//*****************************************************************************
#define PWM_FREQUENCY_25K           4

//*****************************************************************************
//
// The value for UI_PARAM_PWM_FREQUENCY that indicates that the PWM frequency
// is 40 KHz.
//
//*****************************************************************************
#define PWM_FREQUENCY_40K           5

//*****************************************************************************
//
// The value for UI_PARAM_PWM_FREQUENCY that indicates that the PWM frequency
// is 50 KHz.
//
//*****************************************************************************
#define PWM_FREQUENCY_50K           6

//*****************************************************************************
//
// The value for UI_PARAM_PWM_FREQUENCY that indicates that the PWM frequency
// is 80 KHz.
//
//*****************************************************************************
#define PWM_FREQUENCY_80K           7

//*****************************************************************************
//
// The value for UI_PARAM_DECAY_MODE that indicates that the motor is to
// be driven with fast decay in trapezoid mode.
//
//*****************************************************************************
#define DECAY_FAST                  0

//*****************************************************************************
//
// The value for UI_PARAM_DECAY_MODE that indicates that the motor is to
// be driven with slow decay in trapezoid mode.
//
//*****************************************************************************
#define DECAY_SLOW                  1

//*****************************************************************************
//
// The value for UI_PARAM_DIRECTION that indicates that the motor is to be
// driven in the forward direction.
//
//*****************************************************************************
#define DIRECTION_FORWARD           0

//*****************************************************************************
//
// The value for UI_PARAM_DIRECTION that indicates that the motor is to be
// driven in the backward direction.
//
//*****************************************************************************
#define DIRECTION_BACKWARD          1

//*****************************************************************************
//
// The value for UI_PARAM_ENCODER_PRESENT that indicates that the encoder is
// absent.
//
//*****************************************************************************
#define ENCODER_ABSENT              0

//*****************************************************************************
//
// The value for UI_PARAM_ENCODER_PRESENT that indicates that the encoder is
// present.
//
//*****************************************************************************
#define ENCODER_PRESENT             1

//*****************************************************************************
//
// The value for UI_PARAM_USE_DYNAM_BRAKE that indicates that the dynamic
// brake is disabled.
//
//*****************************************************************************
#define BRAKE_OFF                   0

//*****************************************************************************
//
// The value for UI_PARAM_USE_DYNAM_BRAKE that indicates that the dynamic
// brake is enabled.
//
//*****************************************************************************
#define BRAKE_ON                    1

//*****************************************************************************
//
// The value for UI_PARAM_SENSOR_TYPE that indicates that the Hall Effect
// sensor(s) are digital GPIO inputs with 120 degree spacing.
//
//*****************************************************************************
#define SENSOR_TYPE_GPIO            0

//*****************************************************************************
//
// The value for UI_PARAM_SENSOR_TYPE that indicates that the Hall Effect
// sensor(s) are Analog/Linear ADC inputs with 120 degree spacing.
//
//*****************************************************************************
#define SENSOR_TYPE_LINEAR          1

//*****************************************************************************
//
// The value for UI_PARAM_SENSOR_TYPE that indicates that the Hall Effect
// sensor(s) are digital GPIO inputs with 60 degree spacing.
//
//*****************************************************************************
#define SENSOR_TYPE_GPIO_60         2

//*****************************************************************************
//
// The value for UI_PARAM_SENSOR_TYPE that indicates that the Hall Effect
// sensor(s) are Analog/Linear ADC inputs with 120 degree spacing.
//
//*****************************************************************************
#define SENSOR_TYPE_LINEAR_60       3

//*****************************************************************************
//
// The value for UI_PARAM_SENSOR_POLARITY that indicates that the
// Hall Effect sensor(s) are configured as active low.
//
//*****************************************************************************
#define SENSOR_POLARITY_LOW         1

//*****************************************************************************
//
// The value for UI_PARAM_SENSOR_POLARITY that indicates that the
// Hall Effect sensor(s) are configured as active high.
//
//*****************************************************************************
#define SENSOR_POLARITY_HIGH        0

//*****************************************************************************
//
// Close the Doxygen group.
// @}
//
//*****************************************************************************

//*****************************************************************************
//
// Prototypes for the globals exported from the user interface.
//
//*****************************************************************************
extern void UIRunLEDBlink(unsigned short usRate, unsigned short usPeriod);
extern void UIFaultLEDBlink(unsigned short usRate, unsigned short usPeriod);
extern void SysTickIntHandler(void);
extern void UIInit(void);
extern unsigned long UIGetTicks(void);

#endif // __UI_H__
