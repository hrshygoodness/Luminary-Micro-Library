//*****************************************************************************
//
// commands.h - Definitions used by the serial communication protocol.
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
// This is part of revision 9453 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup ui_serial_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The value of the <tt>{tag}</tt> byte for a command packet.
//
//*****************************************************************************
#define TAG_CMD        0xff

//*****************************************************************************
//
//! The value of the <tt>{tag}</tt> byte for a status packet.
//
//*****************************************************************************
#define TAG_STATUS     0xfe

//*****************************************************************************
//
//! The value of the <tt>{tag}</tt> byte for a real-time data packet.
//
//*****************************************************************************
#define TAG_DATA       0xfd

//*****************************************************************************
//
//! This command is used to determine the type of motor driven by the board.
//! In this context, the type of motor is a broad statement; for example,
//! both single-phase and three-phase AC induction motors can be driven by a
//! single AC induction motor board (not simultaneously, of course).
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_ID_TARGET {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x05 CMD_ID_TARGET {type} {checksum}
//! \endverbatim
//!
//! - <tt>{type}</tt> identifies the motor drive type; will be one of
//! #RESP_ID_TARGET_BLDC, #RESP_ID_TARGET_STEPPER, or #RESP_ID_TARGET_ACIM.
//
//*****************************************************************************
#define CMD_ID_TARGET           0x00

//*****************************************************************************
//
//! The response returned by the #CMD_ID_TARGET command for a BLDC motor drive.
//
//*****************************************************************************
#define RESP_ID_TARGET_BLDC     0x00

//*****************************************************************************
//
//! The response returned by the #CMD_ID_TARGET command for a stepper motor
//! drive.
//
//*****************************************************************************
#define RESP_ID_TARGET_STEPPER  0x01

//*****************************************************************************
//
//! The response returned by the #CMD_ID_TARGET command for an AC induction
//! motor drive.
//
//*****************************************************************************
#define RESP_ID_TARGET_ACIM     0x02

//*****************************************************************************
//
//! Starts an upgrade of the firmware on the target. There is no response
//! to this command; once received, the target will return to the control
//! of the Stellaris boot loader and its serial protocol.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_UPGRADE {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     <none>
//! \endverbatim
//
//*****************************************************************************
#define CMD_UPGRADE             0x01

//*****************************************************************************
//
//! This command is used to discover the motor drive board(s) that may be
//! connected to the networked communication channel (e.g. CAN, Ethernet).
//! This command is similar to the CMD_ID_TARGET command, but intended for
//! networked operation.  Additional parameters are available in the response
//! that will allow the networked device to provide board-specific information
//! (e.g. configuration switch settings) that can be used to identify which
//! board is to be selected for operation.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_DISCOVER_TARGET {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x0A CMD_DISCOVER_TARGET {type} {id} {remote-ip} {checksum}
//! \endverbatim
//!
//! - <tt>{type}</tt> identifies the motor drive type; will be one of
//! #RESP_ID_TARGET_BLDC, #RESP_ID_TARGET_STEPPER, or #RESP_ID_TARGET_ACIM.
//! - <tt>{id}</tt> is a board-specific identification value; will typically
//! be the setting read from a set of configuration switches on the board.
//! - <tt>{config}</tt> is used to provide additional (if needed) board
//! configuration information.  The interpretation of this field will vary
//! with the board type.
//
//*****************************************************************************
#define CMD_DISCOVER_TARGET     0x02

//*****************************************************************************
//
//! Gets a list of the parameters supported by this motor drive.  This
//! command returns a list of parameter numbers, in no particular order;
//! each will be one of the \b PARAM_xxx values.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_GET_PARAMS {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS {length} CMD_GET_PARAMS {param} [{param} ...] {checksum}
//! \endverbatim
//!
//! - <tt>{param}</tt> is a list of one or more \b PARAM_xxx values.
//
//*****************************************************************************
#define CMD_GET_PARAMS          0x10

//*****************************************************************************
//
//! Gets the description of a parameter.  The size of the parameter value,
//! the minimum and maximum values for the parameter, and the step between
//! valid values for the parameter.  If the minimum, maximum, and step
//! values don't make sense for a parameter, they may be omitted from
//! the response, leaving only the size.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x05 CMD_GET_PARAM_DESC {param} {checksum}
//! \endverbatim
//!
//! - <tt>{param}</tt> is one of the \b PARAM_xxx values.
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS {length} CMD_GET_PARAM_DESC {size} {min} [{min} ...]
//!         {max} [{max} ...] {step} [{step} ...] {checksum}
//! \endverbatim
//!
//! - <tt>{size}</tt> is the size of the parameter in bytes.
//! - <tt>{min}</tt> is the minimum valid value for this parameter.  The number
//!   of bytes for this value is determined by the size of the parameter.
//! - <tt>{max}</tt> is the maximum valid value for this parameter.  The number
//!   of bytes for this value is determined by the size of the parameter.
//! - <tt>{step}</tt> is the increment between valid values for this parameter.
//!   It should be the case that ``min + (step * N) = max'' for some positive
//!   integer N.  The number of bytes for this value is determined by the size
//!   of the parameter.
//
//*****************************************************************************
#define CMD_GET_PARAM_DESC      0x11

//*****************************************************************************
//
//! Gets the value of a parameter.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x05 CMD_GET_PARAM_VALUE {param} {checksum}
//! \endverbatim
//!
//! - <tt>{param}</tt> is the parameter whose value should be returned; must
//!   be one of the parameters returned by #CMD_GET_PARAMS.
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS {length} CMD_GET_PARAM_VALUE {value} [{value} ...]
//!         {checksum}
//! \endverbatim
//!
//! - <tt>{value}</tt> is the current value of the parameter.  All bytes of the
//!   value will always be returned.
//
//*****************************************************************************
#define CMD_GET_PARAM_VALUE     0x12

//*****************************************************************************
//
//! Sets the value of a parameter.  For parameters that have values larger
//! than a single byte, not all bytes of the parameter value need to be
//! supplied; value bytes that are not supplied (that is, the more
//! significant bytes) are treated as if a zero was transmitted.  If more bytes
//! than required for the parameter value are supplied, the extra bytes are
//! ignored.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD {length} CMD_SET_PARAM_VALUE {param} {value} [{value} ...]
//!         {checksum}
//! \endverbatim
//!
//! - <tt>{param}</tt> is the parameter whose value should be set; must be one
//!   of the parameters returned by #CMD_GET_PARAMS.
//! - <tt>{value}</tt> is the new value for the parameter.
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_SET_PARAM_VALUE {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_SET_PARAM_VALUE     0x13

//*****************************************************************************
//
//! Loads the most recent parameter set from flash, causing the current
//! parameter values to be lost.  This can be used to recover from parameter
//! changes that do not work very well.  For example, if a set of parameter
//! changes are made during experimentation and they turn out to cause the
//! motor to perform poorly, this will restore the last-saved
//! parameter set (which is presumably, but not necessarily, of better
//! quality).
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_LOAD_PARAMS {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_LOAD_PARAMS {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_LOAD_PARAMS         0x14

//*****************************************************************************
//
//! Saves the current parameter set to flash.  Only the most recently
//! saved parameter set is available for use, and it contains the
//! default settings of all the parameters at power-up.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_SAVE_PARAMS {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_SAVE_PARAMS {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_SAVE_PARAMS         0x15

//*****************************************************************************
//
//! Gets a list of the real-time data items supported by this motor drive.
//! This command returns a list of real-time data item numbers, in no
//! particular order, along with the size of the data item; each
//! data item will be one of the \b DATA_xxx values.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_GET_DATA_ITEMS {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS {length} CMD_GET_DATA_ITEMS {item} {size}
//!         [{item} {size} ...] {checksum}
//! \endverbatim
//!
//! - <tt>{item}</tt> is a list of one or more \b DATA_xxx values.
//! - <tt>{size}</tt> is the size of the data item immediately preceding.
//
//*****************************************************************************
#define CMD_GET_DATA_ITEMS      0x20

//*****************************************************************************
//
//! Adds a real-time data item to the real-time data output stream.  To
//! avoid a change in the real-time data output stream at an unexpected
//! time, this command should only be issued when the real-time data output
//! stream is disabled.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x05 CMD_ENABLE_DATA_ITEM {item} {checksum}
//! \endverbatim
//!
//! - <tt>{item}</tt> is the real-time data item to be added to the real-time
//!   data output stream; must be one of the \b DATA_xxx values.
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_ENABLE_DATA_ITEM {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_ENABLE_DATA_ITEM    0x21

//*****************************************************************************
//
//! Removes a real-time data item from the real-time data output
//! stream.  To avoid a change in the real-time data output stream at an
//! unexpected time, this command should only be issued when the real-time
//! data output stream is disabled.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x05 CMD_DISABLE_DATA_ITEM {item} {checksum}
//! \endverbatim
//!
//! - <tt>{item}</tt> is the real-time data item to be removed from the
//!   real-time data output stream; must be one of the \b DATA_xxx values.
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_DISABLE_DATA_ITEM {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_DISABLE_DATA_ITEM   0x22

//*****************************************************************************
//
//! Starts the real-time data output stream.  Only those values that have
//! been added to the output stream will be provided, and it will continue
//! to run (regardless of any other motor drive state) until stopped.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_START_DATA_STREAM {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_START_DATA_STREAM {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_START_DATA_STREAM   0x23

//*****************************************************************************
//
//! Stops the real-time data output stream.  The output stream should be
//! stopped before real-time data items are added to or removed from the
//! stream to avoid unexpected changes in the stream data (it will all
//! be valid data, there is simply no easy way to know what real-time data
//! items are in a #TAG_DATA packet if changes are made while the output stream
//! is running).
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_STOP_DATA_STREAM {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_STOP_DATA_STREAM {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_STOP_DATA_STREAM    0x24

//*****************************************************************************
//
//! Starts the motor running based on the current parameter set, if it is
//! not already running.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_RUN {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_RUN {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_RUN                 0x30

//*****************************************************************************
//
//! Stops the motor, if it is not already stopped.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_STOP {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_STOP {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_STOP                0x31

//*****************************************************************************
//
//! Stops the motor, if it is not already stopped.  This may take more
//! aggressive action than #CMD_STOP at the cost of precision.  For
//! example, for a stepper motor, the stop command would ramp the speed down
//! before stopping the motor while emergency stop would stop stepping
//! immediately; in the later case, it is possible that the motor will spin
//! a couple of additional steps, so position accuracy is sacrificed.  This
//! is needed for safety reasons.
//!
//! <i>Command:</i>
//! \verbatim
//!     TAG_CMD 0x04 CMD_EMERGENCY_STOP {checksum}
//! \endverbatim
//!
//! <i>Response:</i>
//! \verbatim
//!     TAG_STATUS 0x04 CMD_EMERGENCY_STOP {checksum}
//! \endverbatim
//
//*****************************************************************************
#define CMD_EMERGENCY_STOP      0x32

//*****************************************************************************
//
//! Specifies the version of the firmware on the motor drive.
//
//*****************************************************************************
#define PARAM_FIRMWARE_VERSION  0x00

//*****************************************************************************
//
//! Specifies the rate at which the real-time data is provided by the
//! motor drive.
//
//*****************************************************************************
#define PARAM_DATA_RATE         0x01

//*****************************************************************************
//
//! Specifies the minimum speed at which the motor can be run.
//
//*****************************************************************************
#define PARAM_MIN_SPEED         0x02

//*****************************************************************************
//
//! Specifies the maximum speed at which the motor can be run.
//
//*****************************************************************************
#define PARAM_MAX_SPEED         0x03

//*****************************************************************************
//
//! Specifies the desired speed of the the motor.
//
//*****************************************************************************
#define PARAM_TARGET_SPEED      0x04

//*****************************************************************************
//
//! Contains the current speed of the motor.  This is a read-only value
//! and matches the corresponding real-time data item.
//
//*****************************************************************************
#define PARAM_CURRENT_SPEED     0x05

//*****************************************************************************
//
//! Specifies the rate at which the speed of the motor is changed when
//! increasing its speed.
//
//*****************************************************************************
#define PARAM_ACCEL             0x06

//*****************************************************************************
//
//! Specifies the rate at which the speed of the motor is changed when
//! decreasing its speed.
//
//*****************************************************************************
#define PARAM_DECEL             0x07

//*****************************************************************************
//
//! Specifies the target position of the motor.
//
//*****************************************************************************
#define PARAM_TARGET_POS        0x08

//*****************************************************************************
//
//! Contains the current position of the motor.  This is a read-only value
//! and matches the corresponding real-time data item.
//
//*****************************************************************************
#define PARAM_CURRENT_POS       0x09

//*****************************************************************************
//
//! Selects between open-loop and closed-loop mode of the motor drive.
//
//*****************************************************************************
#define PARAM_CLOSED_LOOP       0x0a

//*****************************************************************************
//
//! Indicates whether or not an encoder feedback is present on the motor.
//!  Things that require the encoder feedback in order to operate
//! (for example, closed-loop speed control) will be automatically disabled
//! when there is no encoder feedback present.
//
//*****************************************************************************
#define PARAM_ENCODER_PRESENT   0x0b

//*****************************************************************************
//
//! Specifies the type of waveform modulation to be used to drive the motor.
//
//*****************************************************************************
#define PARAM_MODULATION        0x0c

//*****************************************************************************
//
//! Specifies the direction of rotation for the motor.
//
//*****************************************************************************
#define PARAM_DIRECTION         0x0d

//*****************************************************************************
//
//! Specifies the mapping of motor drive frequency to motor drive voltage
//! (commonly referred to as the V/f table).
//
//*****************************************************************************
#define PARAM_VF_TABLE          0x0e

//*****************************************************************************
//
//! Specifies the base PWM frequency used to generate the motor drive
//! waveforms.
//
//*****************************************************************************
#define PARAM_PWM_FREQUENCY     0x0f

//*****************************************************************************
//
//! Specifies the dead time between the high- and low-side PWM signals for
//! a motor phase when using complimentary PWM outputs.
//
//*****************************************************************************
#define PARAM_PWM_DEAD_TIME     0x10

//*****************************************************************************
//
//! Specifies the rate at which the PWM duty cycle is updated.
//
//*****************************************************************************
#define PARAM_PWM_UPDATE        0x11

//*****************************************************************************
//
//! Specifies the minimum width of a PWM pulse; pulses shorter than this
//! value (either positive or negative) are removed from the output.
//! A high pulse shorter than this value will result in the PWM signal
//! remaining low, and a low pulse shorter than this value will result in the
//! PWM signal remaining high.
//
//*****************************************************************************
#define PARAM_PWM_MIN_PULSE     0x12

//*****************************************************************************
//
//! Specifies the wiring configuration of the motor.  For example, for an
//! AC induction motor, this could be one phase or three phase;
//! for a stepper motor, this could be unipolar or bipolar.
//
//*****************************************************************************
#define PARAM_MOTOR_TYPE        0x13

//*****************************************************************************
//
//! Specifies the number of pole pairs in the motor.
//
//*****************************************************************************
#define PARAM_NUM_POLES         0x14

//*****************************************************************************
//
//! Specifies the number of lines in the (optional) optical encoder attached
//! to the motor.
//
//*****************************************************************************
#define PARAM_NUM_LINES         0x15

//*****************************************************************************
//
//! Specifies the minimum current supplied to the motor when operating.  If
//! the current drops below this value, then an undercurrent alarm is asserted.
//
//*****************************************************************************
#define PARAM_MIN_CURRENT       0x16

//*****************************************************************************
//
//! Specifies the maximum current supplied to the motor when operating.  If
//! the current goes above this value, then an overcurrent alarm is asserted.
//
//*****************************************************************************
#define PARAM_MAX_CURRENT       0x17

//*****************************************************************************
//
//! Specifies the minimum bus voltage when the motor is operating. If the
//! bus voltage drops below this value, then an undervoltage alarm is asserted.
//
//*****************************************************************************
#define PARAM_MIN_BUS_VOLTAGE   0x18

//*****************************************************************************
//
//! Specifies the maximum bus voltage when the motor is operating. If the
//! bus voltage goes above this value, then an overvoltage alarm is asserted.
//
//*****************************************************************************
#define PARAM_MAX_BUS_VOLTAGE   0x19

//*****************************************************************************
//
//! Specifies the P coefficient for the PI controller used to adjust the
//! motor speed to track to the requested speed.
//
//*****************************************************************************
#define PARAM_SPEED_P           0x1a

//*****************************************************************************
//
//! Specifies the I coefficient for the PI controller used to adjust the
//! motor speed to track to the requested speed.
//
//*****************************************************************************
#define PARAM_SPEED_I           0x1b

//*****************************************************************************
//
//! Specifies the bus voltage at which the brake circuit is first applied.
//! If the bus voltage goes above this value, then the brake circuit is
//! engaged.
//
//*****************************************************************************
#define PARAM_BRAKE_ON_VOLTAGE  0x1c

//*****************************************************************************
//
//! Specifies the bus voltage at which the brake circuit is disengaged.
//! If the brake circuit is engaged and the bus voltage drops below this
//! value, then the brake circuit is disengaged.
//
//*****************************************************************************
#define PARAM_BRAKE_OFF_VOLTAGE 0x1d

//*****************************************************************************
//
//! Specifies whether the on-board user interface should be active or inactive.
//
//*****************************************************************************
#define PARAM_USE_ONBOARD_UI    0x1e

//*****************************************************************************
//
//! Specifies the amount of time to precharge the bridge before starting the
//! motor drive.
//
//*****************************************************************************
#define PARAM_PRECHARGE_TIME    0x1f

//*****************************************************************************
//
//! Specifies whether DC bus voltage compensation should be performed.
//
//*****************************************************************************
#define PARAM_USE_BUS_COMP      0x20

//*****************************************************************************
//
//! Specifies the range of the V/f table.
//
//*****************************************************************************
#define PARAM_VF_RANGE          0x21

//*****************************************************************************
//
//! Specifies the motor control mode.
//
//*****************************************************************************
#define PARAM_CONTROL_MODE      0x22

//*****************************************************************************
//
//! Specifies the motor winding current decay mode.
//
//*****************************************************************************
#define PARAM_DECAY_MODE        0x23

//*****************************************************************************
//
//! Specifies the motor stepping mode.
//
//*****************************************************************************
#define PARAM_STEP_MODE         0x24

//*****************************************************************************
//
//! Specifies the fixed on duration for application of motor winding current.
//
//*****************************************************************************
#define PARAM_FIXED_ON_TIME     0x25

//*****************************************************************************
//
//! Specifies the winding resistance.
//
//*****************************************************************************
#define PARAM_RESISTANCE        0x26

//*****************************************************************************
//
//! Specifies the blanking time after the current is removed.
//
//*****************************************************************************
#define PARAM_BLANK_OFF         0x27

//*****************************************************************************
//
//! Specifies the motor winding holding current.
//
//*****************************************************************************
#define PARAM_HOLDING_CURRENT   0x28

//*****************************************************************************
//
//! Specifies whether dynamic braking should be performed.
//
//*****************************************************************************
#define PARAM_USE_DYNAM_BRAKE   0x29

//*****************************************************************************
//
//! Specifies the maximum time that dynamic braking can be performed (in
//! order to prevent circuit or motor damage).
//
//*****************************************************************************
#define PARAM_MAX_BRAKE_TIME    0x2a

//*****************************************************************************
//
//! Specifies the time at which the dynamic braking leaves cooling mode
//! if entered.
//
//*****************************************************************************
#define PARAM_BRAKE_COOL_TIME   0x2b

//*****************************************************************************
//
//! Provides the fault status of the motor drive.  This value matches the
//! corresponding real-time data item; writing it will clear all latched
//! fault status.
//
//*****************************************************************************
#define PARAM_FAULT_STATUS      0x2c

//*****************************************************************************
//
//! Provides the status of the motor drive, indicating the operating mode
//! of the drive.  This value will be one of #MOTOR_STATUS_STOP,
//! #MOTOR_STATUS_RUN, #MOTOR_STATUS_ACCEL, or #MOTOR_STATUS_DECEL.
//
//*****************************************************************************
#define PARAM_MOTOR_STATUS      0x2d

//*****************************************************************************
//
//! Specifies whether DC injection braking should be performed.
//
//*****************************************************************************
#define PARAM_USE_DC_BRAKE      0x2e

//*****************************************************************************
//
//! Specifies the voltage to be applied during DC injection braking.
//
//*****************************************************************************
#define PARAM_DC_BRAKE_V        0x2f

//*****************************************************************************
//
//! Specifies the amount of time to apply DC injection braking.
//
//*****************************************************************************
#define PARAM_DC_BRAKE_TIME     0x30

//*****************************************************************************
//
//! Specifies the bus voltage at which the deceleration of the motor drive
//! is reduced in order to control increases in the bus voltage.
//
//*****************************************************************************
#define PARAM_DECEL_VOLTAGE     0x31

//*****************************************************************************
//
//! Specifies the target running current of the motor.
//
//*****************************************************************************
#define PARAM_TARGET_CURRENT    0x32

//*****************************************************************************
//
//! Specifies the maximum ambient temperature of the microcontroller.  If
//! the ambient temperature goes above this value, then an overtemperature
//! alarm is asserted.
//
//*****************************************************************************
#define PARAM_MAX_TEMPERATURE   0x33

//*****************************************************************************
//
//! Specifies the motor current at which the acceleration of the motor drive is
//! reduced in order to control increases in the motor current.
//
//*****************************************************************************
#define PARAM_ACCEL_CURRENT     0x34

//*****************************************************************************
//
//! Indicates whether or not Hall Effect sensor feedback is present on the
//! motor.  Things that require the sensor feedback in order to operate
//! (for example, closed-loop speed control) will be automatically disabled
//! when there is no sensor feedback present.
//
//*****************************************************************************
#define PARAM_SENSOR_PRESENT    0x35

//*****************************************************************************
//
//! Indicates the type of Hall Effect sensor feedback that is present on the
//! motor.  The Hall Effect sensor can be the Digital/GPIO type that is 
//! typically used, or can be the Analog/Linear type.
//
//*****************************************************************************
#define PARAM_SENSOR_TYPE       0x36

//*****************************************************************************
//
//! Indicates the value(s) of the various GPIO signals on the motor drive
//! board.
//
//*****************************************************************************
#define PARAM_GPIO_DATA         0x37

//*****************************************************************************
//
//! Indicates the number of CAN messages that have been received on the
//! CAN bus.
//
//*****************************************************************************
#define PARAM_CAN_RX_COUNT      0x38

//*****************************************************************************
//
//! Indicates the number of CAN messages that have been transmitted on the
//! CAN bus.
//
//*****************************************************************************
#define PARAM_CAN_TX_COUNT      0x39

//*****************************************************************************
//
//! Indicates the number of Ethernet messages that have been received on the
//! Ethernet interface.
//
//*****************************************************************************
#define PARAM_ETH_RX_COUNT      0x3a

//*****************************************************************************
//
//! Indicates the number of Ethernet messages that have been transmitted on
//! the Ethernet interface.
//
//*****************************************************************************
#define PARAM_ETH_TX_COUNT      0x3b

//*****************************************************************************
//
//! The timeout for an IDLE TCP connection
//
//*****************************************************************************
#define PARAM_ETH_TCP_TIMEOUT   0x3c

//*****************************************************************************
//
//! Indicates the polarity of the GPIO/Digital Hall Sensor Inputs.
//
//*****************************************************************************
#define PARAM_SENSOR_POLARITY   0x3d

//*****************************************************************************
//
//! Indicates the duty cycle for startup phase.
//
//*****************************************************************************
#define PARAM_STARTUP_DUTY      0x3e

//*****************************************************************************
//
//! Indicates the startup count for sensorless operation.
//
//*****************************************************************************
#define PARAM_STARTUP_COUNT     0x3f

//*****************************************************************************
//
//! Contains the starting voltage for sensorless startup operation.
//
//*****************************************************************************
#define PARAM_STARTUP_STARTV    0x40

//*****************************************************************************
//
//! Contains the ending voltage for sensorless startup operation.
//
//*****************************************************************************
#define PARAM_STARTUP_ENDV      0x41

//*****************************************************************************
//
//! Contains the starting speed for sensorless startup operation.
//
//*****************************************************************************
#define PARAM_STARTUP_STARTSP   0x42

//*****************************************************************************
//
//! Contains the ending speed for sensorless startup operation.
//
//*****************************************************************************
#define PARAM_STARTUP_ENDSP     0x43

//*****************************************************************************
//
//! Specifies the target power supplied to the motor when operating.
//
//*****************************************************************************
#define PARAM_TARGET_POWER      0x44

//*****************************************************************************
//
//! Contains the current power of the motor.  This is a read-only value
//! and matches the corresponding real-time data item.
//
//*****************************************************************************
#define PARAM_CURRENT_POWER     0x45

//*****************************************************************************
//
//! Specifies the minimum power at which the motor can be run.
//
//*****************************************************************************
#define PARAM_MIN_POWER         0x46

//*****************************************************************************
//
//! Specifies the maximum power at which the motor can be run.
//
//*****************************************************************************
#define PARAM_MAX_POWER         0x47

//*****************************************************************************
//
//! Specifies the P coefficient for the PI controller used to adjust the
//! motor power to track to the requested power.
//
//*****************************************************************************
#define PARAM_POWER_P           0x48

//*****************************************************************************
//
//! Specifies the I coefficient for the PI controller used to adjust the
//! motor power to track to the requested power.
//
//*****************************************************************************
#define PARAM_POWER_I           0x49

//*****************************************************************************
//
//! Specifies the rate at which the power of the motor is changed when
//! increasing its power.
//
//*****************************************************************************
#define PARAM_ACCEL_POWER       0x4A

//*****************************************************************************
//
//! Specifies the rate at which the power of the motor is changed when
//! decreasing its power.
//
//*****************************************************************************
#define PARAM_DECEL_POWER       0x4B

//*****************************************************************************
//
//! Contains the length of time for the open-loop sensorless startup.
//
//*****************************************************************************
#define PARAM_STARTUP_RAMP      0x4C

//*****************************************************************************
//
//! Contains the back EMF threshhold voltage for sensorless startup.
//
//*****************************************************************************
#define PARAM_STARTUP_THRESH    0x4D

//*****************************************************************************
//
//! Contains the skip count for BEMF zero crossing detect hold-off.
//
//*****************************************************************************
#define PARAM_BEMF_SKIP_COUNT   0x4E

//*****************************************************************************
//
//! This real-time data item provides the current through phase A of the motor.
//
//*****************************************************************************
#define DATA_PHASE_A_CURRENT    0x00

//*****************************************************************************
//
//! This real-time data item provides the current through phase B of the motor.
//
//*****************************************************************************
#define DATA_PHASE_B_CURRENT    0x01

//*****************************************************************************
//
//! This real-time data item provides the current through phase C of the motor.
//
//*****************************************************************************
#define DATA_PHASE_C_CURRENT    0x02

//*****************************************************************************
//
//! This real-time data item provides the current through the motor (that is,
//! the sum of the phases).
//
//*****************************************************************************
#define DATA_MOTOR_CURRENT      0x03

//*****************************************************************************
//
//! This real-time data item provides the bus voltage.
//
//*****************************************************************************
#define DATA_BUS_VOLTAGE        0x04

//*****************************************************************************
//
//! This real-time data item provides the position of the motor.
//
//*****************************************************************************
#define DATA_MOTOR_POSITION     0x05

//*****************************************************************************
//
//! This real-time data item provides the speed of the motor drive.  This will
//! only be available for asynchronous motors, where the rotor speed does not
//! match the stator speed.
//
//*****************************************************************************
#define DATA_STATOR_SPEED       0x06

//*****************************************************************************
//
//! This real-time data item provides the speed of the rotor (in other words,
//! the motor shaft).  For asynchronous motors, this will differ from the
//! stator speed.
//
//*****************************************************************************
#define DATA_ROTOR_SPEED        0x07

//*****************************************************************************
//
//! This real-time data item provides the percentage of the processor that is
//! being utilized.
//
//*****************************************************************************
#define DATA_PROCESSOR_USAGE    0x08

//*****************************************************************************
//
//! This real-time data item provides the current operating mode of the motor
//! drive.  This value will be one of #MOTOR_STATUS_STOP, #MOTOR_STATUS_RUN,
//! #MOTOR_STATUS_ACCEL, or #MOTOR_STATUS_DECEL.
//
//*****************************************************************************
#define DATA_MOTOR_STATUS       0x09

//*****************************************************************************
//
//! This real-time data item provides the direction the motor drive is running.
//
//*****************************************************************************
#define DATA_DIRECTION          0x0a

//*****************************************************************************
//
//! This real-time data item provides the current fault status of the motor
//! drive.
//
//*****************************************************************************
#define DATA_FAULT_STATUS       0x0b

//*****************************************************************************
//
//! This real-time data item provides the ambient temperature of the
//! microcontroller.
//
//*****************************************************************************
#define DATA_TEMPERATURE        0x0c

//*****************************************************************************
//
//! This real-time data item provides the ambient temperature of the
//! microcontroller.
//
//*****************************************************************************
#define DATA_ANALOG_INPUT       0x0d

//*****************************************************************************
//
//! This real-time data item provides application-specific debug information.
//! The format of this data will vary from one application to the next.  It
//! is the responsibility of the user to ensure that the motor drive board and
//! host application are in sync with the data format.
//
//*****************************************************************************
#define DATA_DEBUG_INFO         0x0e

//*****************************************************************************
//
//! This real-time data item provides the power supplied to the motor.
//
//*****************************************************************************
#define DATA_MOTOR_POWER        0x0f

//*****************************************************************************
//
//! The number of real-time data items.
//
//*****************************************************************************
#define DATA_NUM_ITEMS          0x10

//*****************************************************************************
//
//! This is the motor status when the motor drive is stopped.
//
//*****************************************************************************
#define MOTOR_STATUS_STOP       0x00

//*****************************************************************************
//
//! This is the motor status when the motor drive is running at a fixed speed.
//
//*****************************************************************************
#define MOTOR_STATUS_RUN        0x01

//*****************************************************************************
//
//! This is the motor status when the motor drive is accelerating.
//
//*****************************************************************************
#define MOTOR_STATUS_ACCEL      0x02

//*****************************************************************************
//
//! This is the motor status when the motor drive is decelerating.
//
//*****************************************************************************
#define MOTOR_STATUS_DECEL      0x03

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
