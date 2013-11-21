//*****************************************************************************
//
// motor_demo.c - Example to run the EVALBOT motors
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
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ethernet.h"
#include "driverlib/ethernet.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "drivers/display96x16x1.h"
#include "drivers/io.h"
#include "drivers/motor.h"
#include "drivers/sensors.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Motor Demo (motor_demo)</h1>
//!
//! This example application demonstrates the use of the motors.  The buttons
//! and bump sensors are used to start, stop, and reverse the motors.  It uses
//! the system tick timer (SysTick) as a time reference for button debouncing
//! and blinking LEDs.
//!
//! When this example is running, the display shows a message identifying the
//! example.  The two user buttons on the right are used to control
//! the motors.  The top button controls the left motor and the bottom button
//! controls the right motor.  When a motor control button is pressed, the
//! motor runs in the forward direction.  When the button is pressed a second
//! time, the motor pauses.  Pressing the button a third time causes the motor
//! to run in reverse.  This cycle can be repeated by continuing to press the
//! button.
//!
//! When a motor is running, either forward or reverse, pressing the bump
//! sensor pauses the motor.  When the bump sensor is released, the motor
//! resumes running in the same direction.
//
//*****************************************************************************

//*****************************************************************************
//
// Defines the possible states for the motor state machine.
//
//*****************************************************************************
typedef enum
{
    STATE_STOPPED,
    STATE_RUNNING,
    STATE_PAUSED
} tMotorState;

//*****************************************************************************
//
// Counter for the 10 ms system clock ticks.  Used for tracking time.
//
//*****************************************************************************
static volatile unsigned long g_ulTickCount;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    for(;;)
    {
    }
}
#endif

//*****************************************************************************
//
// The interrupt handler for the SysTick timer.  This handler will increment a
// tick counter to keep track of time, toggle the LEDs, and call the button
// and bump sensor debouncers.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Increment the tick counter.
    //
    g_ulTickCount++;

    //
    // Every 100 ticks (1 second), toggle the LEDs
    //
    if((g_ulTickCount % 100) == 0)
    {
        LED_Toggle(BOTH_LEDS);
    }

    //
    // Call the user button and bump sensor debouncing functions, which must
    // be called periodically.
    //
    PushButtonDebouncer();
    BumpSensorDebouncer();
}

//*****************************************************************************
//
// The main application.  It configures the board and then enters a loop
// to process the button and sensor presses and run the motor.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulPHYMR0;
    tBoolean bButtonWasPressed[2] = { false, false };
    tBoolean bReverse[2] = { true, true };
    tMotorState sMotorState[2] = { STATE_STOPPED, STATE_STOPPED };

    //
    // Set the clocking to directly from the crystal
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Since the Ethernet is not used, power down the PHY to save battery.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    ulPHYMR0 = ROM_EthernetPHYRead(ETH_BASE, PHY_MR0);
    ROM_EthernetPHYWrite(ETH_BASE, PHY_MR0, ulPHYMR0 | PHY_MR0_PWRDN);

    //
    // Initialize the board display
    //
    Display96x16x1Init(true);

    //
    // Print a simple message to the display
    //
    Display96x16x1StringDraw("MOTOR", 29, 0);
    Display96x16x1StringDraw("DEMO", 31, 1);

    //
    // Initialize the LED driver, then turn one LED on
    //
    LEDsInit();
    LED_On(LED_1);

    //
    // Initialize the buttons driver
    //
    PushButtonsInit();

    //
    // Initialize the bump sensor driver
    //
    BumpSensorsInit();

    //
    // Initialize the motor driver
    //
    MotorsInit();

    //
    // Set up and enable the SysTick timer to use as a time reference.
    // It will be set up for a 10 ms tick.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 100);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enter loop to run forever, processing button presses to make the
    // motors run, pause, and stop.
    //
    for(;;)
    {
        unsigned long ulMotor;

        //
        // Process a state machine for each motor
        //
        for(ulMotor = 0; ulMotor < 2; ulMotor++)
        {
            tBoolean bButtonIsPressed;
            tBoolean bBumperIsPressed;

            //
            // Get the current button and bump sensor state for this motor
            //
            bButtonIsPressed = !PushButtonGetDebounced(ulMotor);
            bBumperIsPressed = !BumpSensorGetDebounced(ulMotor);

            switch(sMotorState[ulMotor])
            {
                //
                // STOPPED state - the motor is not running, and is waiting for
                // a button press to start running
                //
                case STATE_STOPPED:
                {
                    //
                    // If the button for this motor is pressed (and it was
                    // not pressed before), then start the motor running.
                    //
                    if(bButtonIsPressed && !bButtonWasPressed[ulMotor])
                    {
                        //
                        // Change the direction from whatever it was before
                        //
                        bReverse[ulMotor] ^= 1;
                        MotorDir(ulMotor, bReverse[ulMotor]);

                        //
                        // Set the motor duty cycle to 50%.
                        // Duty cycle is specified as 8.8 fixed point.
                        //
                        MotorSpeed(ulMotor, 50 << 8);

                        //
                        // Start the motor running
                        //
                        MotorRun(ulMotor);

                        //
                        // Change to running state
                        //
                        sMotorState[ulMotor] = STATE_RUNNING;
                    }
                    break;
                }

                //
                // RUNNING state - wait for button press to stop the
                // motor, or bumper press to pause the motor
                //
                case STATE_RUNNING:
                {
                    //
                    // If the button for this motor is pressed (and it was
                    // not pressed before), then stop the motor.
                    //
                    if(bButtonIsPressed && !bButtonWasPressed[ulMotor])
                    {
                        MotorStop(ulMotor);

                        //
                        // Change to stopped state
                        //
                        sMotorState[ulMotor] = STATE_STOPPED;
                    }

                    //
                    // Check to see if the bump sensor is pressed
                    //
                    else if(bBumperIsPressed)
                    {
                        //
                        // Stop the motor
                        //
                        MotorStop(ulMotor);

                        //
                        // Change to the paused state
                        //
                        sMotorState[ulMotor] = STATE_PAUSED;
                    }
                    break;
                }

                //
                // PAUSED state - wait for bumper to be released
                //
                case STATE_PAUSED:
                {
                    //
                    // Check to see if bump sensor is released
                    //
                    if(!bBumperIsPressed)
                    {
                        //
                        // Resume the motor running at the previous speed
                        // and direction
                        //
                        MotorRun(ulMotor);

                        //
                        // Change state back to running
                        //
                        sMotorState[ulMotor] = STATE_RUNNING;
                    }
                    break;
                }

                //
                // default is an error.  Stop the motors and go to
                // stopped state
                //
                default:
                {
                    MotorStop(ulMotor);
                    sMotorState[ulMotor] = STATE_STOPPED;
                    break;
                }
            } // end switch

            //
            // Remember if the button was pressed for the next pass through
            // the state machine.
            ///
            bButtonWasPressed[ulMotor] = bButtonIsPressed;

        } // end for(ulMotor ...)
    } // end for(;;)
}

