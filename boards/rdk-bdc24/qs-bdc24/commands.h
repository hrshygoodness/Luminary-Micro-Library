//*****************************************************************************
//
// commands.h - Command queue handling code and defines.
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

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

//*****************************************************************************
//
// The commands that can be sent via the command queue.
//
//*****************************************************************************
#define COMMAND_FORCE_NEUTRAL   0x00
#define COMMAND_VOLTAGE_MODE    0x01
#define COMMAND_VOLTAGE_SET     0x02
#define COMMAND_VOLTAGE_RATE    0x03
#define COMMAND_SPEED_MODE      0x11
#define COMMAND_SPEED_SET       0x12
#define COMMAND_SPEED_SRC_SET   0x13
#define COMMAND_SPEED_P_SET     0x14
#define COMMAND_SPEED_I_SET     0x15
#define COMMAND_SPEED_D_SET     0x16
#define COMMAND_POS_MODE        0x21
#define COMMAND_POS_SET         0x22
#define COMMAND_POS_SRC_SET     0x23
#define COMMAND_POS_P_SET       0x24
#define COMMAND_POS_I_SET       0x25
#define COMMAND_POS_D_SET       0x26
#define COMMAND_CURRENT_MODE    0x31
#define COMMAND_CURRENT_SET     0x32
#define COMMAND_CURRENT_P_SET   0x33
#define COMMAND_CURRENT_I_SET   0x34
#define COMMAND_CURRENT_D_SET   0x35
#define COMMAND_NUM_BRUSHES     0x41
#define COMMAND_ENCODER_LINES   0x42
#define COMMAND_POT_TURNS       0x43
#define COMMAND_BRAKE_COAST_SET 0x44
#define COMMAND_POS_LIMIT_MODE  0x45
#define COMMAND_POS_LIMIT_FWD   0x46
#define COMMAND_POS_LIMIT_REV   0x47
#define COMMAND_MAX_VOLTAGE     0x48
#define COMMAND_VCOMP_MODE      0x51
#define COMMAND_VCOMP_SET       0x52
#define COMMAND_VCOMP_IN_RAMP   0x53
#define COMMAND_VCOMP_COMP_RAMP 0x54

//*****************************************************************************
//
// Forces the motor controller into neutral, regardless of the control mode.
//
//*****************************************************************************
#define CommandForceNeutral()                                                 \
        CommandSend(COMMAND_FORCE_NEUTRAL, 0, 0, 0)

//*****************************************************************************
//
// Enables or disables voltage control mode.
//
//*****************************************************************************
#define CommandVoltageMode(bEnable)                                           \
        CommandSend(COMMAND_VOLTAGE_MODE, bEnable, 0, 0)

//*****************************************************************************
//
// Sets the desired voltage for voltage control mode.
//
//*****************************************************************************
#define CommandVoltageSet(lVoltage)                                           \
        CommandSend(COMMAND_VOLTAGE_SET, lVoltage, 0, 0)

//*****************************************************************************
//
// Sets the rate of change for voltage control mode.
//
//*****************************************************************************
#define CommandVoltageRateSet(ulRate)                                         \
        CommandSend(COMMAND_VOLTAGE_RATE, ulRate, 0, 0)

//*****************************************************************************
//
// Enables or disables speed control mode.
//
//*****************************************************************************
#define CommandSpeedMode(bEnable)                                             \
        CommandSend(COMMAND_SPEED_MODE, bEnable, 0, 0)

//*****************************************************************************
//
// Sets the desired speed for speed control mode.
//
//*****************************************************************************
#define CommandSpeedSet(lSpeed)                                               \
        CommandSend(COMMAND_SPEED_SET, lSpeed, 0, 0)

//*****************************************************************************
//
// Sets the speed reference source.
//
//*****************************************************************************
#define CommandSpeedSrcSet(ulSrc)                                             \
        CommandSend(COMMAND_SPEED_SRC_SET, ulSrc, 0, 0)

//*****************************************************************************
//
// Sets the P coefficient of the speed control PID controller.
//
//*****************************************************************************
#define CommandSpeedPSet(lPGain)                                              \
        CommandSend(COMMAND_SPEED_P_SET, lPGain, 0, 0)

//*****************************************************************************
//
// Sets the I coefficient of the speed control PID controller.
//
//*****************************************************************************
#define CommandSpeedISet(lIGain)                                              \
        CommandSend(COMMAND_SPEED_I_SET, lIGain, 0, 0)

//*****************************************************************************
//
// Sets the D coefficient of the speed control PID controller.
//
//*****************************************************************************
#define CommandSpeedDSet(lDGain)                                              \
        CommandSend(COMMAND_SPEED_D_SET, lDGain, 0, 0)

//*****************************************************************************
//
// Enables or disables position control mode.
//
//*****************************************************************************
#define CommandPositionMode(bEnable, lStartingPosition)                       \
        CommandSend(COMMAND_POS_MODE, bEnable, lStartingPosition, 0)

//*****************************************************************************
//
// Sets the desired position for position control mode.
//
//*****************************************************************************
#define CommandPositionSet(lPosition)                                         \
        CommandSend(COMMAND_POS_SET, lPosition, 0, 0)

//*****************************************************************************
//
// Sets the position reference source.
//
//*****************************************************************************
#define CommandPositionSrcSet(ulSrc)                                          \
        CommandSend(COMMAND_POS_SRC_SET, ulSrc, 0, 0)

//*****************************************************************************
//
// Sets the P coefficient of the position control PID controller.
//
//*****************************************************************************
#define CommandPositionPSet(lPGain)                                           \
        CommandSend(COMMAND_POS_P_SET, lPGain, 0, 0)

//*****************************************************************************
//
// Sets the I coefficient of the position control PID controller.
//
//*****************************************************************************
#define CommandPositionISet(lIGain)                                           \
        CommandSend(COMMAND_POS_I_SET, lIGain, 0, 0)

//*****************************************************************************
//
// Sets the D coefficient of the position control PID controller.
//
//*****************************************************************************
#define CommandPositionDSet(lDGain)                                           \
        CommandSend(COMMAND_POS_D_SET, lDGain, 0, 0)

//*****************************************************************************
//
// Enables or disables current control mode.
//
//*****************************************************************************
#define CommandCurrentMode(bEnable)                                           \
        CommandSend(COMMAND_CURRENT_MODE, bEnable, 0, 0)

//*****************************************************************************
//
// Sets the desired current for current control mode.
//
//*****************************************************************************
#define CommandCurrentSet(lCurrent)                                           \
        CommandSend(COMMAND_CURRENT_SET, lCurrent, 0, 0)

//*****************************************************************************
//
// Sets the P coefficient of the current control PID controller.
//
//*****************************************************************************
#define CommandCurrentPSet(lP)                                                \
        CommandSend(COMMAND_CURRENT_P_SET, lP, 0, 0)

//*****************************************************************************
//
// Sets the I coefficient of the current control PID controller.
//
//*****************************************************************************
#define CommandCurrentISet(lI)                                                \
        CommandSend(COMMAND_CURRENT_I_SET, lI, 0, 0)

//*****************************************************************************
//
// Sets the D coefficient of the current control PID controller.
//
//*****************************************************************************
#define CommandCurrentDSet(lD)                                                \
        CommandSend(COMMAND_CURRENT_D_SET, lD, 0, 0)

//*****************************************************************************
//
// Sets the number of brushes in the motor, which determines the number of
// commutations per revolution.
//
//*****************************************************************************
#define CommandNumBrushesSet(ulCount)                                         \
        CommandSend(COMMAND_NUM_BRUSHES, ulCount, 0, 0)

//*****************************************************************************
//
// Sets the number of lines per revolution in the encoder.
//
//*****************************************************************************
#define CommandEncoderLinesSet(ulCount)                                       \
        CommandSend(COMMAND_ENCODER_LINES, ulCount, 0, 0)

//*****************************************************************************
//
// Sets the number of turns in the potentiometer.
//
//*****************************************************************************
#define CommandPotTurnsSet(ulCount)                                           \
        CommandSend(COMMAND_POT_TURNS, ulCount, 0, 0)

//*****************************************************************************
//
// Sets the brake/coast configuration.
//
//*****************************************************************************
#define CommandBrakeCoastSet(ulState)                                         \
        CommandSend(COMMAND_BRAKE_COAST_SET, ulState, 0, 0)

//*****************************************************************************
//
// Enables or disables the soft limit switches.
//
//*****************************************************************************
#define CommandPositionLimitMode(bEnable)                                     \
        CommandSend(COMMAND_POS_LIMIT_MODE, bEnable, 0, 0)

//*****************************************************************************
//
// Set the configuration of the forward soft limit switch.
//
//*****************************************************************************
#define CommandPositionLimitForwardSet(lLimit, bLessThan)                     \
        CommandSend(COMMAND_POS_LIMIT_FWD, lLimit, bLessThan, 0)

//*****************************************************************************
//
// Set the configuration of the reverse soft limit switch.
//
//*****************************************************************************
#define CommandPositionLimitReverseSet(lLimit, bLessThan)                     \
        CommandSend(COMMAND_POS_LIMIT_REV, lLimit, bLessThan, 0)

//*****************************************************************************
//
// Set the configuration of the maximum output voltage.
//
//*****************************************************************************
#define CommandMaxVoltageSet(sVoltage)                                        \
        CommandSend(COMMAND_MAX_VOLTAGE, sVoltage, 0, 0)

//*****************************************************************************
//
// Enables or disabled voltage compensation control mode.
//
//*****************************************************************************
#define CommandVCompMode(bEnable)                                             \
        CommandSend(COMMAND_VCOMP_MODE, bEnable, 0, 0)

//*****************************************************************************
//
// Sets the desired voltage for voltage compensation control mode.
//
//*****************************************************************************
#define CommandVCompSet(lVoltage)                                             \
        CommandSend(COMMAND_VCOMP_SET, lVoltage, 0, 0)

//*****************************************************************************
//
// Sets the rate of change for the input to voltage compensation control mode.
//
//*****************************************************************************
#define CommandVCompInRampSet(ulRate)                                         \
        CommandSend(COMMAND_VCOMP_IN_RAMP, ulRate, 0, 0)

//*****************************************************************************
//
// Sets the rate of change for the output in voltage compensation control mode.
//
//*****************************************************************************
#define CommandVCompCompRampSet(ulRate)                                       \
        CommandSend(COMMAND_VCOMP_COMP_RAMP, ulRate, 0, 0)

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern unsigned long CommandSend(unsigned long ulCmd, unsigned long ulParam1,
                                 unsigned long ulParam2,
                                 unsigned long ulParam3);
extern void CommandQueueProcess(unsigned long bInFault);

#endif // __COMMANDS_H__
