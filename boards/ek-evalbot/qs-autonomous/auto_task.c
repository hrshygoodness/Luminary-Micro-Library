//****************************************************************************
//
// auto_task.c - Autonomous task for EVALBOT
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//****************************************************************************

#include "inc/hw_types.h"
#include "utils/scheduler.h"
#include "drivers/motor.h"
#include "drivers/sensors.h"
#include "drivers/io.h"
#include "utils/uartstdio.h"
#include "drive_task.h"
#include "random.h"
#include "sound_task.h"
#include "sounds.h"

//****************************************************************************
//
// Define the minimum driving duration time, and the allowed random variation,
// in timer ticks.
//
//****************************************************************************
#define DRIVE_DURATION_MIN 700      // 7 seconds
#define DRIVE_DURATION_VAR 1300     // +13 seconds

//****************************************************************************
//
// Define the minimum turning duration, and the allowed random variation,
// in timer ticks.
//
//****************************************************************************
#define TURN_DURATION_MIN 200      // 2 seconds
#define TURN_DURATION_VAR 200      // +2 seconds

//****************************************************************************
//
// Define the wheel speed, in RPM, for driving and turning.
//
//****************************************************************************
#define AUTO_FORWARD_SPEED  40
#define AUTO_TURN_SPEED     25

//****************************************************************************
//
// Define the possible states for the EVALBOT autonomous state machine
//
//****************************************************************************
typedef enum
{
    EVALBOT_STATE_IDLE,
    EVALBOT_STATE_DRIVING,
    EVALBOT_STATE_TURNING,
} tEvalbotState;

//****************************************************************************
//
// This "task" is called periodically from the scheduler in the main app.
// It runs a state machine that tracks the EVALBOT motion, and changes
// the motion in reaction to external inputs.
//
//****************************************************************************
void
AutoTask(void *pvParam)
{
    static tEvalbotState sState = EVALBOT_STATE_IDLE;
    static unsigned long ulLastTicks = 0;
    static unsigned long ulDurationTicks = 0;

    //
    // Process according to the current state
    //
    switch(sState)
    {
        //
        // IDLE - in this state, the EVALBOT is waiting for a press of
        // button 1, which will start the motion.
        //
        case EVALBOT_STATE_IDLE:
        {
            //
            // Check for press of button 1
            //
            if(!PushButtonGetStatus(BUTTON_1))
            {
                //
                // Whenever there is an external event, add to the entropy
                // of the random number generator.
                //
                RandomAddEntropy(SchedulerTickCountGet());
                RandomSeed();

                //
                // Get the current tick, which will be used for measuring
                // the duration of time in a state.
                //
                ulLastTicks = SchedulerTickCountGet();

                //
                // Generate a random duration in timer ticks.
                //
                ulDurationTicks = (RandomNumber() % DRIVE_DURATION_VAR) +
                                  DRIVE_DURATION_MIN;

                //
                // Command the drive to start running forward, update the
                // state machine, and notify action via serial port
                //
                DriveRun(MOTOR_DRIVE_FORWARD, AUTO_FORWARD_SPEED);
                sState = EVALBOT_STATE_DRIVING;
                UARTprintf("button 1 - driving for %u\n", ulDurationTicks);
            }
            break;
        }

        //
        // DRIVING - in this state the EVALBOT is driving forward.  It is
        // waiting for one of the bump sensors, which will initiate a turn, or
        // the user to press button 2 (to stop), or for the driving duration
        // to time out, initiating a random turn.
        //
        case EVALBOT_STATE_DRIVING:
        {
            //
            // Check for left bumper sensor
            //
            if(!BumpSensorGetStatus(LEFT_SIDE))
            {
                //
                // Play bumper sound
                //
                SoundTaskPlay(g_pcBumpSound);

                //
                // Update random entropy
                //
                RandomAddEntropy(SchedulerTickCountGet());
                RandomSeed();

                //
                // Save current tick, and compute random turn duration
                //
                ulLastTicks = SchedulerTickCountGet();
                ulDurationTicks = (RandomNumber() % TURN_DURATION_VAR) +
                                  TURN_DURATION_MIN;

                //
                // Command the drive to start turning, update the
                // state machine, and notify action via serial port
                //
                DriveRun(MOTOR_DRIVE_TURN_RIGHT, AUTO_TURN_SPEED);
                sState = EVALBOT_STATE_TURNING;
                UARTprintf("left sensor - turn right for %u\n", ulDurationTicks);
            }

            //
            // Else, check for right bumper sensor
            //
            else if(!BumpSensorGetStatus(RIGHT_SIDE))
            {
                //
                // Play bumper sound
                //
                SoundTaskPlay(g_pcBumpSound);

                //
                // Update random entropy
                //
                RandomAddEntropy(SchedulerTickCountGet());
                RandomSeed();

                //
                // Save current tick, and compute random turn duration
                //
                ulLastTicks = SchedulerTickCountGet();
                ulDurationTicks = (RandomNumber() % TURN_DURATION_VAR) +
                                  TURN_DURATION_MIN;

                //
                // Command the drive to start turning, update the
                // state machine, and notify action via serial port
                //
                DriveRun(MOTOR_DRIVE_TURN_LEFT, AUTO_TURN_SPEED);
                sState = EVALBOT_STATE_TURNING;
                UARTprintf("right sensor - turn left for %u\n", ulDurationTicks);
            }

            //
            // Else, check for user press of button 2
            //
            else if(!PushButtonGetStatus(BUTTON_2))
            {
                //
                // Update random entropy
                //
                RandomAddEntropy(SchedulerTickCountGet());
                RandomSeed();

                //
                // Command the drive to stop the motion
                //
                DriveRun(MOTOR_DRIVE_FORWARD, 0);
                DriveStop();

                //
                // Update the state machine and notify action via serial port
                //
                sState = EVALBOT_STATE_IDLE;
                UARTprintf("button 2 - stopping\n");
            }

            //
            // Else, check to see if the (random) drive duration has timed
            // out
            //
            else if(SchedulerElapsedTicksGet(ulLastTicks) > ulDurationTicks)
            {
                //
                // Save current tick, and compute random turn duration
                //
                ulLastTicks = SchedulerTickCountGet();
                ulDurationTicks = (RandomNumber() % TURN_DURATION_VAR) +
                                  TURN_DURATION_MIN;

                //
                // Command the drive to turn, randomly selecting left or
                // right turn
                //
                DriveRun((RandomNumber() & 1) ?
                              MOTOR_DRIVE_TURN_LEFT : MOTOR_DRIVE_TURN_RIGHT,
                              AUTO_TURN_SPEED);

                //
                // Update the state and notify the user via serial port
                //
                sState = EVALBOT_STATE_TURNING;
                UARTprintf("random turn - turn for %u\n", ulDurationTicks);
            }

            //
            // Done with this state
            //
            break;
        }

        //
        // In this state, EVALBOT is turning.  It will continue to turn
        // until the randomly chosen turn duration has elapsed, at which time
        // it will then resume driving forward
        //
        case EVALBOT_STATE_TURNING:
        {
            if(SchedulerElapsedTicksGet(ulLastTicks) > ulDurationTicks)
            {
                //
                // Get the current tick, and determine a random drive duration
                //
                ulLastTicks = SchedulerTickCountGet();
                ulDurationTicks = (RandomNumber() % DRIVE_DURATION_VAR) +
                                  DRIVE_DURATION_MIN;

                //
                // Command the drive to go forward, update the state machine
                // and notify action via serial port
                //
                DriveRun(MOTOR_DRIVE_FORWARD, AUTO_FORWARD_SPEED);
                sState = EVALBOT_STATE_DRIVING;
                UARTprintf("done turning, forward for %u\n", ulDurationTicks);
            }

            //
            // Done with this state
            //
            break;
        }
    }
}

//****************************************************************************
//
// This function is used to perform any needed initialization for the
// autonomous driving "task".
//
//****************************************************************************
void
AutoTaskInit(void *pvParam)
{
    PushButtonsInit();
    BumpSensorsInit();
}
