//*****************************************************************************
//
// commands.c - Command queue handling code.
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

#include "adc_ctrl.h"
#include "commands.h"
#include "controller.h"
#include "encoder.h"
#include "hbridge.h"
#include "limit.h"

//*****************************************************************************
//
// The number of entries in the command queue.
//
//*****************************************************************************
#define COMMAND_QUEUE_SIZE      32

//*****************************************************************************
//
// This structure defines an entry in the command queue.
//
//*****************************************************************************
typedef struct
{
    //
    // The command being sent.  This will be one of the COMMAND_* values.
    //
    unsigned long ulCmd;

    //
    // The first parameter of the command.  This value is dependent upon the
    // command being sent.
    //
    unsigned long ulParam1;

    //
    // The second parameter of the command.  This value is dependent upon the
    // command being sent.
    //
    unsigned long ulParam2;

    //
    // The third parameter of the command.  This value is dependent upon the
    // command being sent.
    //
    unsigned long ulParam3;
}
tCommandQueue;

//*****************************************************************************
//
// The command queue.  By convention, one execution context writes commands
// into the queue and another execution context reads commands from the queue.
// Because of this convention, no access control mechanism is required.
//
//*****************************************************************************
static tCommandQueue g_psCommandQueue[COMMAND_QUEUE_SIZE];

//*****************************************************************************
//
// The index into the command queue of the next command to be read.  If this is
// equal to the write index, then the queue is empty.
//
//*****************************************************************************
static unsigned long g_ulCommandQueueRead = 0;

//*****************************************************************************
//
// The index into the command queue of the next command to be written.  If this
// is one less than the read index (modulo the buffer size), then the queue is
// full.
//
//*****************************************************************************
static unsigned long g_ulCommandQueueWrite = 0;

//*****************************************************************************
//
// This function adds a command to the command queue.  If the queue is full,
// the command is not added and a failure is returned.
//
//*****************************************************************************
unsigned long
CommandSend(unsigned long ulCmd, unsigned long ulParam1,
            unsigned long ulParam2, unsigned long ulParam3)
{
    //
    // Return a failure if the command queue is full.
    //
    if(((g_ulCommandQueueWrite + 1) % COMMAND_QUEUE_SIZE) ==
       g_ulCommandQueueRead)
    {
        return(0);
    }

    //
    // Add this command to the next slot in the command queue.
    //
    g_psCommandQueue[g_ulCommandQueueWrite].ulCmd = ulCmd;
    g_psCommandQueue[g_ulCommandQueueWrite].ulParam1 = ulParam1;
    g_psCommandQueue[g_ulCommandQueueWrite].ulParam2 = ulParam2;
    g_psCommandQueue[g_ulCommandQueueWrite].ulParam3 = ulParam3;

    //
    // Increment the command queue write pointer.
    //
    g_ulCommandQueueWrite = (g_ulCommandQueueWrite + 1) % COMMAND_QUEUE_SIZE;

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// This function processes all the commands in the command queue.  Certain
// commands are ignored while the controller is in a fault condition.
//
//*****************************************************************************
void
CommandQueueProcess(unsigned long bInFault)
{
    tCommandQueue *pCommand;

    //
    // Loop while there are commands in the command queue.
    //
    while(g_ulCommandQueueRead != g_ulCommandQueueWrite)
    {
        //
        // Get a pointer to this command.
        //
        pCommand = &(g_psCommandQueue[g_ulCommandQueueRead]);

        //
        // See which command has been received.
        //
        switch(pCommand->ulCmd)
        {
            //
            // Force the motor controller into neutral.
            //
            case COMMAND_FORCE_NEUTRAL:
            {
                //
                // Set the output to neutral.
                //
                ControllerForceNeutral();

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Switch the controller into voltage control mode.
            //
            case COMMAND_VOLTAGE_MODE:
            {
                //
                // Set the voltage control mode.
                //
                ControllerVoltageModeSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the target voltage for voltage control mode.
            //
            case COMMAND_VOLTAGE_SET:
            {
                //
                // Ignore this command if in a fault condition.
                //
                if(!bInFault)
                {
                    //
                    // Set the new target voltage.
                    //
                    ControllerVoltageSet((long)pCommand->ulParam1);
                }

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the rate of change for voltage control mode.
            //
            case COMMAND_VOLTAGE_RATE:
            {
                //
                // Set the rate of change for voltage control mode.
                //
                ControllerVoltageRateSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Switch the controller into speed control mode.
            //
            case COMMAND_SPEED_MODE:
            {
                //
                // Set the speed control mode.
                //
                ControllerSpeedModeSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the target speed for speed control mode.
            //
            case COMMAND_SPEED_SET:
            {
                //
                // Ignore this command if in a fault condition.
                //
                if(!bInFault)
                {
                    //
                    // Set the new target speed.
                    //
                    ControllerSpeedSet((long)pCommand->ulParam1);
                }

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the speed reference source.
            //
            case COMMAND_SPEED_SRC_SET:
            {
                //
                // Set the new speed reference source.
                //
                ControllerSpeedSrcSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the P gain on the speed controller.
            //
            case COMMAND_SPEED_P_SET:
            {
                //
                // Set the P gain of the speed PID controller.
                //
                ControllerSpeedPGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the I gain on the speed controller.
            //
            case COMMAND_SPEED_I_SET:
            {
                //
                // Set the I gain of the speed PID controller.
                //
                ControllerSpeedIGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the D gain on the speed controller.
            //
            case COMMAND_SPEED_D_SET:
            {
                //
                // Set the D gain of the speed PID controller.
                //
                ControllerSpeedDGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Switch the controller into position mode.
            //
            case COMMAND_POS_MODE:
            {
                //
                // Set the position mode.
                //
                ControllerPositionModeSet(pCommand->ulParam1,
                                          pCommand->ulParam2);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the target position for position mode.
            //
            case COMMAND_POS_SET:
            {
                //
                // Ignore this command if in a fault condition.
                //
                if(!bInFault)
                {
                    //
                    // Set the new target position.
                    //
                    ControllerPositionSet((long)pCommand->ulParam1);
                }

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the position reference source.
            //
            case COMMAND_POS_SRC_SET:
            {
                //
                // Set the new position reference source.
                //
                ControllerPositionSrcSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the P gain on the position controller.
            //
            case COMMAND_POS_P_SET:
            {
                //
                // Set the P gain of the position PID controller.
                //
                ControllerPositionPGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the I gain on the position controller.
            //
            case COMMAND_POS_I_SET:
            {
                //
                // Set the I gain of the position PID controller.
                //
                ControllerPositionIGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the D gain on the position controller.
            //
            case COMMAND_POS_D_SET:
            {
                //
                // Set the D gain of the position PID controller.
                //
                ControllerPositionDGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Switch the controller into current control mode.
            //
            case COMMAND_CURRENT_MODE:
            {
                //
                // Set the current control mode.
                //
                ControllerCurrentModeSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the target current for current control mode.
            //
            case COMMAND_CURRENT_SET:
            {
                //
                // Ignore this command if in a fault condition.
                //
                if(!bInFault)
                {
                    //
                    // Set the new target current.
                    //
                    ControllerCurrentSet((long)pCommand->ulParam1);
                }

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the P gain on the current controller.
            //
            case COMMAND_CURRENT_P_SET:
            {
                //
                // Set the P gain of the current PID controller.
                //
                ControllerCurrentPGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the I gain on the current controller.
            //
            case COMMAND_CURRENT_I_SET:
            {
                //
                // Set the I gain of the current PID controller.
                //
                ControllerCurrentIGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the D gain on the current controller.
            //
            case COMMAND_CURRENT_D_SET:
            {
                //
                // Set the D gain of the current PID controller.
                //
                ControllerCurrentDGainSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the number of brushes in the motor, which determines the
            // number of commutations per revolution for sensor-less speed
            // detection.
            //
            case COMMAND_NUM_BRUSHES:
            {
                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the number of lines per revolution in the encoder.
            //
            case COMMAND_ENCODER_LINES:
            {
                //
                // Set the number of lines per revolution.
                //
                EncoderLinesSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the number of turns in the potentiometer.
            //
            case COMMAND_POT_TURNS:
            {
                //
                // Set the number of turns in the potentiometer.
                //
                ADCPotTurnsSet((long)pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the brake/coast state.
            //
            case COMMAND_BRAKE_COAST_SET:
            {
                //
                // Set the brake/coast state.
                //
                HBridgeBrakeCoastSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Enable or disable the position limit "switches".
            //
            case COMMAND_POS_LIMIT_MODE:
            {
                //
                // See if the position limit "switches" should be enabled or
                // disabled.
                //
                if(pCommand->ulParam1)
                {
                    LimitPositionEnable();
                }
                else
                {
                    LimitPositionDisable();
                }

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the position of the forward position limit "switch".
            //
            case COMMAND_POS_LIMIT_FWD:
            {
                //
                // Set the forward limit "switch".
                //
                LimitPositionForwardSet(pCommand->ulParam1,
                                        pCommand->ulParam2);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the position of the reverse position limit "switch".
            //
            case COMMAND_POS_LIMIT_REV:
            {
                //
                // Set the reverse limit "switch".
                //
                LimitPositionReverseSet(pCommand->ulParam1,
                                        pCommand->ulParam2);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the maximum output voltage.
            //
            case COMMAND_MAX_VOLTAGE:
            {
                //
                // Set the maximum H-bridge output voltage.
                //
                HBridgeVoltageMaxSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Switch the controller into voltage compensation control mode.
            //
            case COMMAND_VCOMP_MODE:
            {
                //
                // Set the voltage compensation control mode.
                //
                ControllerVCompModeSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the target voltage for voltage compensation control mode.
            //
            case COMMAND_VCOMP_SET:
            {
                //
                // Ignore this command if in a fault condition.
                //
                if(!bInFault)
                {
                    //
                    // Set the new target voltage.
                    //
                    ControllerVCompSet((long)pCommand->ulParam1);
                }

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the rate of set point change for voltage compensation
            // control mode.
            //
            case COMMAND_VCOMP_IN_RAMP:
            {
                //
                // Set the rate of set point change for voltage compensation
                // control mode.
                //
                ControllerVCompInRateSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Set the rate of compensation change for voltage compensation
            // control mode.
            //
            case COMMAND_VCOMP_COMP_RAMP:
            {
                //
                // Set the rate of compensation change for voltage compensation
                // control mode.
                //
                ControllerVCompCompRateSet(pCommand->ulParam1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Ignore any commands that are not recognized.
            //
            default:
            {
                break;
            }
        }

        //
        // Increment the command queue read pointer.
        //
        g_ulCommandQueueRead = (g_ulCommandQueueRead + 1) % COMMAND_QUEUE_SIZE;
    }
}
