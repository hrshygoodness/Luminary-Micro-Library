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
// This is part of revision 8555 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#include <limits.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/cpu_usage.h"
#include "utils/flash_pb.h"
#include "blinker.h"
#include "commands.h"
#include "stepcfg.h"
#include "stepper.h"
#include "ui.h"
#include "uiparms.h"
#include "ui_common.h"
#include "ui_onboard.h"
#include "ui_serial.h"

//*****************************************************************************
//
//! \page ui_intro Introduction
//!
//! There are two user interfaces for the the stepper motor application.
//! One uses an on-board potentiometer and push button for basic control of
//! the motor and two LEDs for basic status feedback, and the other uses the
//! serial port to provide complete control of all aspects of the motor drive
//! as well as monitoring of real-time performance data.
//!
//! The on-board user interface consists of a potentiometer, push button, and
//! two LEDs. The on-board interface operates in two modes: speed mode
//! and position mode. In speed mode the potentiometer controls the speed
//! at which the motor runs. The button can be used to start and stop the
//! motor, reversing direction each time. In position mode, the potentiometer
//! controls the position of the motor. Motion commands are sent to the
//! stepper every time the potentiometer is moved, so that the motor tracks
//! the potentiometer position.
//!
//! The initial mode is speed mode. The mode is indicated by blinking on
//! the Mode LED. It blinks one time for speed mode, and twice for
//! position mode. When the RDK board is reset, the Mode LED will blink
//! one time, indicating speed mode. The mode can be changed by holding
//! down the user button for 5 seconds.
//!
//! In speed mode, the motor is started running by a single press and release
//! of the user button. The speed can be changed while the motor is running
//! by changing the position of the potentiometer knob. If the button is
//! pressed again, the motor will stop. Each time it is pressed it will
//! start or stop, reversing direction each time it starts. The motor speed
//! ranges from 10 steps/second up to about 1000 steps/second at the extremes
//! of the potentiometer range.
//!
//! In position mode, the motor is enabled for motion. When the potentiometer
//! knob is moved, the motor will move to track the knob position. If the
//! button is pressed, the motor will be disabled and will not move with
//! the knob. If it is pressed again, then the motor will be re-enabled.
//! The potentiometer reading is scaled so that one turn of the potentiometer
//! is about the same as one revolution of the motor. The result is a
//! one-to-one response between the position of the knob and the motor.
//!
//! As the motor turns, in either mode, the Status LED blinks at a
//! rate corresponding to the motor speed. If a fault occurs, due to
//! a current limit, then the status LED will blink rapidly. If this
//! happens, the fault can be cleared by holding down the user button for
//! 5 seconds, and the Status LED will stop blinking.
//!
//! A periodic interrupt, the SysTick timer, is used to poll the state of
//! the push button and perform debouncing. It is also used to read the
//! ADC values for the potentiometer position as well as the bus voltage
//! and processor temperature. The SysTick interrupt initiates motion
//! commands when the position of the potentiometer changes.
//!
//! When the serial interface is used, the on-board interface is (typically)
//! disabled, and the button and potentiometer have no effect. However,
//! the Status LED is still used as a motor speed indicator.
//!
//! The serial user interface is handled entirely by the serial user interface
//! module. The only thing provided here is the list of parameters and
//! real-time data items, plus a set of helper functions that are required in
//! order to properly set the values of some of the parameters.
//!
//! This user interface (and the accompanying serial and on-board user
//! interface modules) is more complicated and consumes more program space than
//! would typically exist in a real motor drive application. The added
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
//! An absolute value macro, provided here to avoid the need to pull in
//! the library.
//!
//! \param n a signed value
//!
//! This macro returns the absolute value of whatever value is passed to it.
//!
//! \return Returns the absolute value of the input value.
//
//*****************************************************************************
#define ABS(n) ((n) < 0 ? -(n) : (n))

//*****************************************************************************
//
//! The rate at which the user interface interrupt occurs (SysTick).
//
//*****************************************************************************
#define UI_INT_RATE             100

//*****************************************************************************
//
//! The minimum value that can be read from the potentiometer. This
//! corresponds to the value read when the potentiometer is all the way
//! to the left.
//
//*****************************************************************************
#define UI_POT_MIN              0

//*****************************************************************************
//
//! The maximum value that can be read from the potentiometer. This
//! corresponds to the value read when the potentiometer is all the way
//! to the right.
//
//*****************************************************************************
#define UI_POT_MAX              1023

//*****************************************************************************
//
//! Definition and mode for the on-board user interface.
//
//*****************************************************************************
typedef enum
{
    //
    //! Defines speed mode, where the motor runs at a speed determined
    //! by the position of the knob.
    //
    UI_MODE_SPEED,

    //
    //! Defines position mode, there the motor moves to a position to
    //! match the position of the knob.
    //
    UI_MODE_POSITION,

    //
    //! The number of user interface modes.
    //
    NUM_UI_MODES
}
tUIMode;

//*****************************************************************************
//
//! Holds the state of the on-board interface: UI_MODE_SPEED, or
//! UI_MODE_POSITION.
//
//*****************************************************************************
static tUIMode eUIMode = UI_MODE_SPEED;

//*****************************************************************************
//
//! A flag that is used to keep track of motor direction when using the
//! on-board interface in speed mode.
//
//*****************************************************************************
unsigned char bReverse = 1;

//*****************************************************************************
//
//! The value of the last read potentiometer position.
//
//*****************************************************************************
static unsigned long ulPotPosition = 0;

//*****************************************************************************
//
//! A pointer to the motion status information that is returned from
//! the positioner module.
//
//*****************************************************************************
static tStepperStatus *pStepperStatus;

//*****************************************************************************
//
// Forward declarations for functions declared within this source file for use
// in the parameter and real-time data structures.
//
//*****************************************************************************
static void UIButtonPress(void);
static void UIButtonHold(void);

//*****************************************************************************
//
//! An array of structures describing the on-board switches.
//
//*****************************************************************************
const tUIOnboardSwitch g_sUISwitches[] =
{
    //
    // The run/stop/mode button. Pressing the button will cycle between
    // stopped and running, and holding the switch for five seconds will toggle
    // between speed mode and position mode.
    //
    {
        USER_BUTTON_PIN_NUM,// the pin number
        UI_INT_RATE * 3,    // the time needed to trigger a hold
        UIButtonPress,      // press function
        0,                  // release function
        UIButtonHold        // hold function
    }
};

//*****************************************************************************
//
//! The number of switches in the g_sUISwitches array. This value is
//! automatically computed based on the number of entries in the array.
//
//*****************************************************************************
#define NUM_SWITCHES            (sizeof(g_sUISwitches) /   \
                                 sizeof(g_sUISwitches[0]))

//*****************************************************************************
//
//! The number of switches on this target. This value is used by the on-board
//! user interface module.
//
//*****************************************************************************
const unsigned long g_ulUINumButtons = NUM_SWITCHES;

//*****************************************************************************
//
//! This is the count of the number of samples during which the switches have
//! been pressed; it is used to distinguish a switch press from a switch
//! hold. This array is used by the on-board user interface module.
//
//*****************************************************************************
unsigned long g_pulUIHoldCount[NUM_SWITCHES];

//*****************************************************************************
//
//! Sets up fault conditions whenever a fault triggering parameter is set.
//!
//! This function is called by the serial user interface whenever the
//! maximum current parameter is changed. This parameter is used to set up
//! a fault trigger, which will trip if the current exceeds the specified
//! value.
//!
//! \return None.
//
//*****************************************************************************
void
UISetFaultParms(void)
{
    //
    // Call the stepper API function to set the current fault parameter
    // using the value that was set from the UI.
    //
    StepperSetFaultParms(g_sParameters.usMaxCurrent);
}

//*****************************************************************************
//
//! Clears a fault condition.
//!
//! Once a fault condition occurs, the motor will  not run until the fault
//! is cleared. This function clears any faults.
//!
//! \return None.
//
//*****************************************************************************
void
UIClearFaults(void)
{
    //
    // Call the stepper API function to clear all pending faults.
    //
    StepperClearFaults();
}

//*****************************************************************************
//
//! Commands the stepper drive to make a motion.
//!
//! This function is called by the serial user interface whenever any of the
//! following parameters are changed: target position, speed and acceleration
//! or deceleration rates. This will immediately change the motor motion
//! to match the new parameters.
//!
//! If the motor was not first enabled, then this will have no effect.
//!
//! \return None.
//
//*****************************************************************************
void
UISetMotion(void)
{
    //
    // Send the motion command.
    //
    StepperSetMotion(g_lTargetPos, g_sParameters.usSpeed,
                     g_sParameters.usAccel, g_sParameters.usDecel);

    //
    // Read back the target position, it will not have changed if the
    // motor wasnt enabled.
    //
    g_lTargetPos = pStepperStatus->lTargetPos;
}

//*****************************************************************************
//
//! Sets the chopper blanking intervals.
//!
//! This function is called by the serial user interface whenever the
//! chopper blanking parameters are changed.
//!
//! \return None.
//
//*****************************************************************************
void
UISetChopperBlanking(void)
{
    //
    // Call the stepper API function to set the off blanking time,
    // using the value that was just set from the UI.
    //
    StepperSetBlankingTime(g_sParameters.usBlankOffTime);

    //
    // The off blanking time is always accepted, so there is no need
    // to read it back to check it.
    //
}

//*****************************************************************************
//
//! Sets the motor parameters related to winding current.
//!
//! This function is called by the serial user interface whenever the
//! motor drive or holding current parameters are changed.
//!
//! \return None.
//
//*****************************************************************************
void
UISetMotorParms(void)
{
    //
    // Call the stepper API function to set the motor parameters
    // for current, using the values that were just set from the UI.
    //
    StepperSetMotorParms(g_sParameters.usDriveCurrent,
                         g_sParameters.usHoldCurrent, g_usBusVoltage,
                         g_sParameters.usResistance);

    //
    // The motor parameters are always accepted, so there is no need to
    // read back to verify.
    //
}

//*****************************************************************************
//
//! Sets the motor control mode (method).
//!
//! This function is called by the serial user interface whenever the
//! motor control mode is changed.
//!
//! \return None.
//
//*****************************************************************************
void
UISetControlMode(void)
{
    //
    // Call the stepper API function to set the current control mode,
    // using the value that was just set from the UI.
    //
    StepperSetControlMode(g_sParameters.ucControlMode);

    //
    // The control mode can only be set when the motor is not running,
    // so read it back in case it was not changed.
    //
    g_sParameters.ucControlMode = pStepperStatus->ucControlMode;
}

//*****************************************************************************
//
//! Sets the current decay control mode.
//!
//! This function is called by the serial user interface whenever the
//! current decay mode is changed.
//!
//! \return None.
//
//*****************************************************************************
void
UISetDecayMode(void)
{
    //
    // Call the stepper API function to set the current decay mode,
    // using the value that was just set from the UI.
    //
    StepperSetDecayMode(g_sParameters.ucDecayMode);

    //
    // The decay mode is always accepted, so there is no need to read
    // it back to verify it.
    //
}

//*****************************************************************************
//
//! Sets the step size mode.
//!
//! This function is called by the serial user interface whenever the
//! step size mode is changed.
//!
//! \return None.
//
//*****************************************************************************
void
UISetStepMode(void)
{
    //
    // Call the stepper API function to set the step size mode,
    // using the value that was just set from the UI.
    //
    StepperSetStepMode(g_sParameters.ucStepMode);

    //
    // The step mode can only be set when the motor is not running,
    // so read it back in case it was not changed.
    //
    g_sParameters.ucStepMode = pStepperStatus->ucStepMode;
}

//*****************************************************************************
//
//! Sets the PWM fixed on time.
//!
//! This function is called by the serial user interface whenever the
//! fixed on time parameter is changed.
//!
//! \return None.
//
//*****************************************************************************
void
UISetFixedOnTime(void)
{
    //
    // Call the stepper API function to set the fixed rise time,
    // using the value that was just set from the UI.
    //
    StepperSetFixedOnTime(g_sParameters.usFixedOnTime);

    //
    // The fixed on time can be updated any time, so there is no
    // need to read it back to verify it.
    //
}

//*****************************************************************************
//
//! Sets the PWM frequency.
//!
//! This function is called by the serial user interface whenever the
//! PWM frequency parameter is changed.
//!
//! \return None.
//
//*****************************************************************************
void
UISetPWMFreq(void)
{
    //
    // Call the stepper API function to set the PWM frequency,
    // using the value that was just set from the UI.
    //
    StepperSetPWMFreq(g_sParameters.usPWMFrequency);

    //
    // The PWM frequency can only be set when the motor is not running,
    // so read it back in case it was not changed.
    //
    g_sParameters.usPWMFrequency = pStepperStatus->usPWMFrequency;
}

//*****************************************************************************
//
//! Switch User Interface modes between on-board and off-board
//!
//! This function is called by the serial user interface when the
//! on-board interface enable flag is changed. If the on-board
//! interface is being disabled, that means the user is using the
//! off-board interface. If the on-board interface was being used,
//! then the target and actual positions will have non-zero values, which
//! could cause confusion when the PC based GUI program is started.
//! So the target and actual positions are reset to 0 if the motor
//! is not moving.
//!
//! \return None.
//
//*****************************************************************************
void
UIOnBoard(void)
{
    //
    // If the on-board interface is being disabled, then reset the
    // target and actual positions back to 0, so they start out at
    // 0 for the user interface.
    //
    if(!g_bUIUseOnboard)
    {
        //
        // Get the motion status so we know what the motor is doing.
        //
        pStepperStatus = StepperGetMotorStatus();

        //
        // Only change the target and actual parameters if the motor
        // is not moving.
        //
        if(pStepperStatus->ucMotorStatus == MOTOR_STATUS_STOP)
        {
            //
            // Enable the stepper so the target position can be changed.
            //
            StepperEnable();

            //
            // Set the requested target position and actual to 0
            //
            g_lTargetPos = 0;
            StepperResetPosition(0);

            //
            // Send a motion command to get the target pos back to 0.
            //
            UISetMotion();

            //
            // Disable the motor in prep for off-board UI.
            //
            StepperDisable();
        }
    }
}

//*****************************************************************************
//
//! Enables the motor drive.
//!
//! This function is called by the serial user interface when the run command
//! is received. The motor drive will be enabled as a result; this is a no
//! operation if the motor drive is already running.
//!
//! There is no immediate response when the motor is enabled, it must
//! be followed by a motion command in order to make the motor move.
//!
//! \return None.
//
//*****************************************************************************
void
UIRun(void)
{
    //
    // Call the stepper API function to enable the motor for operation.
    //
    StepperEnable();
}

//*****************************************************************************
//
//! Stops the motor drive.
//!
//! This function is called by the serial user interface when the stop command
//! is received. The motor drive will be stopped as a result; this is a no
//! operation if the motor drive is already stopped. After this, the motor
//! will be disabled, and must be re-enabled before another motion command
//! can be issued.
//!
//! This will cause a controlled stop of the motor.
//!
//! \return None.
//
//*****************************************************************************
void
UIStop(void)
{
    //
    // Call the stepper API function to disable the motor operation.
    //
    StepperDisable();
}

//*****************************************************************************
//
//! Emergency stops the motor drive.
//!
//! This function is called by the serial user interface when the emergency
//! stop command is received. This will immediately remove all power from
//! the motor. It is not a controlled stop, and the position information
//! will be lost. The motor will be disabled, and must be re-enabled before
//! it can be used again.
//!
//! \return None.
//
//*****************************************************************************
void
UIEmergencyStop(void)
{
    //
    // Call the stepper API function to urgently stop the motor. The
    // motor will be left disabled.
    //
    StepperEmergencyStop();
}

//*****************************************************************************
//
//! Loads the motor drive parameter block from flash.
//!
//! This function is called by the serial user interface when the load
//! parameter block function is called. If the motor drive is running, the
//! parameter block is not loaded. If the motor drive is not running and a
//! valid parameter block exists in flash, the contents of the parameter
//! block are loaded from flash.
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
    // Get the motion status so we know what the motor is doing.
    //
    pStepperStatus = StepperGetMotorStatus();

    //
    // Return without doing anything if the motor drive is running.
    //
    if(pStepperStatus->ucMotorStatus != MOTOR_STATUS_STOP)
    {
        return;
    }

    //
    // Get a pointer to the latest parameter block in flash, and return without
    // doing anything if there is not one.
    //
    pucBuffer = FlashPBGet();
    if(!pucBuffer)
    {
        return;
    }

    //
    // Loop through the words of the parameter block to copy its contents from
    // flash to SRAM.
    //
    for(ulIdx = 0; ulIdx < (sizeof(tDriveParameters) / 4); ulIdx++)
    {
        ((unsigned long *)&g_sParameters)[ulIdx] =
            ((unsigned long *)pucBuffer)[ulIdx];
    }

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

    //
    // Make sure the correct control mode is set last, after all the
    // parameters are loaded and updated.
    //
    UISetControlMode();
}

//*****************************************************************************
//
//! Saves the motor drive parameter block to flash.
//!
//! This function is called by the serial user interface when the save
//! parameter block function is called. The parameter block is written to
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
    // Save the parameter block to flash.
    //
    FlashPBSave((unsigned char *)&g_sParameters);
}

//*****************************************************************************
//
//! Update the firmware using the bootloader.
//!
//! This function is called by the serial user interface when the firmware
//! update command is received.
//!
//! \return None.
//
//*****************************************************************************
void
UIUpgrade(void)
{
    //
    // Emergency stop the motor drive.
    //
    StepperEmergencyStop();

    //
    // Disable all processor interrupts. Instead of disabling them one at a
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
    BlinkStart(STATUS_LED, 0, 1, 1);
    BlinkStart(MODE_LED, 0, 1, 1);
    BlinkHandler();

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
    // Return control to the boot loader. This is a call to the SVC handler in
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
//! detected.
//!
//! \return None.
//
//*****************************************************************************
static void
UIButtonPress(void)
{
    //
    // Get the motion status so we know what the motor is doing.
    //
    pStepperStatus = StepperGetMotorStatus();

    //
    // If in Speed mode, then a button press will cause the motor
    // to start moving (if not moving), or stop if it was moving.
    // The direction is also reversed each time.
    //
    if(eUIMode == UI_MODE_SPEED)
    {
        //
        // If the motor is stopped, then flip the direction flag,
        // and then start the motor running in the correct direction.
        // Since in speed mode, the motor should run continuously,
        // a very large move is sent to the motor.
        //
        if(pStepperStatus->ucMotorStatus == MOTOR_STATUS_STOP)
        {
            bReverse = !bReverse;
            if(bReverse)
            {
                g_lTargetPos = INT_MIN + pStepperStatus->lPosition;
            }
            else
            {
                g_lTargetPos = INT_MAX + pStepperStatus->lPosition;
            }
            StepperEnable();
            StepperSetMotion(g_lTargetPos, ulPotPosition,
                     g_sParameters.usAccel, g_sParameters.usDecel);
        }

        //
        // If the motor was already running, then just make it stop.
        //
        else
        {
            StepperDisable();
        }
    }

    //
    // If in position mode, then the button press just enables
    // or disables the motor running.
    //
    else if(eUIMode == UI_MODE_POSITION)
    {
        if(pStepperStatus->bEnabled)
        {
            StepperDisable();
        }
        else
        {
            StepperEnable();
            StepperResetPosition(ulPotPosition * 256);
        }
    }
}

//*****************************************************************************
//
//! Handles button holds.
//!
//! This function is called when a hold of the on-board push button has been
//! detected. This causes the stepper on-board interface to switch modes.
//!
//! \return None.
//
//*****************************************************************************
static void
UIButtonHold(void)
{
    //
    // Disable the motor first, to stop it if it is running.
    //
    StepperDisable();

    //
    // Wait until the motor is stopped before proceeding.
    //
    do
    {
        pStepperStatus = StepperGetMotorStatus();
    } while(pStepperStatus->ucMotorStatus != MOTOR_STATUS_STOP);

    //
    // If there is a pending fault, then clear the fault and just return,
    // without changing modes.
    //
    if(pStepperStatus->ucFaultFlags)
    {
        UIClearFaults();
        return;
    }

    //
    // Advance to the next mode, and cause the LED to blink the
    // mode number.
    //
    eUIMode++;
    if(eUIMode == NUM_UI_MODES)
    {
        eUIMode = UI_MODE_SPEED;
    }
    BlinkStart(MODE_LED, UI_INT_RATE / 2, UI_INT_RATE / 2,
               eUIMode + 1);

    //
    // For position mode, reset the current position and the
    // target position both to 0, so the motor starts out not
    // needing to move. Leave the motor enabled.
    //
    if(eUIMode == UI_MODE_POSITION)
    {
        g_lTargetPos = (ulPotPosition / 5) * 256;
        StepperResetPosition(g_lTargetPos);
        StepperEnable();
    }

    //
    // For speed mode, set the reverse flag. On a button press
    // the flag will be notted, so this will ensure that it starts
    // in the "forward" direction.
    //
    else if(eUIMode == UI_MODE_SPEED)
    {
        bReverse = 1;
    }
}

//*****************************************************************************
//
//! Handles the SysTick interrupt.
//!
//! This function is called when SysTick asserts its interrupt. It is
//! responsible for handling the on-board user interface elements (push button
//! and potentiometer) if enabled, and the processor usage computation.
//!
//! \return None.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    static unsigned int uDataUpdate = UI_INT_RATE / 10;
    static unsigned long ulLastPotPosition = 0;
    static unsigned long ulLastBlink = 0;
    static long lLastPotDelta = 0;
    static unsigned short usLastBusVoltage = 0;
    static unsigned char bFaultBlink = 0;
    unsigned long ulBlink;
    unsigned long ulADCCounts[8]; // 0-pot, 1-busV, 2-temperature
    long lSamples;
    long lPotDelta;

    //
    // Get the motor status. This will update the fields in the
    // stepper status structure.
    //
    pStepperStatus = StepperGetMotorStatus();

    //
    // Compute the average current, which is what gets reported through
    // the UI.
    //
    g_usMotorCurrent = (pStepperStatus->usCurrent[0] +
                       pStepperStatus->usCurrent[1] + 1) / 2;

    //
    // Get the previous ADC samples, and start a new acquisition
    //
    lSamples = ADCSequenceDataGet(ADC0_BASE, UI_ADC_SEQUENCER, ulADCCounts);
    ADCProcessorTrigger(ADC0_BASE, UI_ADC_SEQUENCER);

    //
    // Read the bus voltage and convert to millivolts,
    // and the CPU temperature and convert to C
    //
    if(lSamples == 3)
    {
        g_usBusVoltage = (unsigned short)((ulADCCounts[1] * 81300) / 1023);
        g_sAmbientTemp = (59960 - (ulADCCounts[2] * 100)) / 356;
    }

    //
    // If the bus voltage has changed more than 0.5 volts, then call
    // the UISetMotorParms() function, which will force an update of
    // the PWM duty cycle calculation. This will insure that the PWM
    // duty cycle is correct if the bus voltage changes.
    //
    if(ABS((int)g_usBusVoltage - (int)usLastBusVoltage) > 500)
    {
        UISetMotorParms();
        usLastBusVoltage = g_usBusVoltage;
    }

    //
    // Periodically send real time data
    //
    if(!uDataUpdate--)
    {
        uDataUpdate = UI_INT_RATE / 20;
        UISerialSendRealTimeData();
    }

    //
    // If a fault just occurred, then start the status LED blinking
    // at a rapid rate.
    //
    if(pStepperStatus->ucFaultFlags && !bFaultBlink)
    {
        BlinkStart(STATUS_LED, 4, 4, INT_MAX);
        bFaultBlink = 1;
    }

    //
    // Else, if a fault was just cleared, then stop the status LED blinking.
    //
    else if(!pStepperStatus->ucFaultFlags && bFaultBlink)
    {
        BlinkStart(STATUS_LED, 1, 1, 1);
        bFaultBlink = 0;
    }

    //
    // Otherwise, as normal, blink the status LED according to the number
    // of steps moved.
    //
    else
    {
        //
        // Blink the status LED every so many steps.
        //
        ulBlink = pStepperStatus->lPosition / ((2000 * 256) / 8);
        if(ulBlink != ulLastBlink)
        {
            BlinkStart(STATUS_LED, UI_INT_RATE / 32, 1, 1);
            ulLastBlink = ulBlink;
        }
    }

    //
    // Periodically call the blinker state machine.
    //
    BlinkHandler();

    //
    // Compute the new value for the processor usage.
    //
    g_ucCPUUsage = (CPUUsageTick() + 32768) / 65536;

    //
    // Do the following only if the on-board UI is enabled.
    //
    if(g_bUIUseOnboard == 1)
    {
        //
        // Filter the potentiometer value. Make sure there is valid
        // ADC data.
        //
        if(lSamples == 3)
        {
            ulPotPosition = UIOnboardPotentiometerFilter(ulADCCounts[0]);
        }
        else
        {
            //
            // If ADC reading is suspect, then just use the last previous
            // reading.
            //
            ulPotPosition = ulLastPotPosition;
        }

        //
        // Set the minimum value to be use as 10. This sets the minimum
        // speed for speed mode to 10.
        //
        if(ulPotPosition < 10)
        {
            ulPotPosition = 10;
        }

        //
        // Read the on-board switch and pass its value to the switch
        // debouncer. This will drive the button press functions
        // UIButtonPress() and UIButtonHold() which will be called
        // back if the button press or hold occurred.
        //
        UIOnboardSwitchDebouncer(GPIOPinRead(USER_BUTTON_PORT,
                                             USER_BUTTON_PIN));

        //
        // Find the difference from the last pot measurement.
        //
        lPotDelta = ulPotPosition - ulLastPotPosition;

        //
        // Check if the direction is reversed. The following is used
        // to ensure that the pot reading does not actually cause
        // a change in direction (in position mode) due just to jitter
        // in the potentiometer reading. Only a change above a certain
        // value will be accepted. If the direction is not changed,
        // then no filtering is done.
        //
        if((lPotDelta != 0) && ((lPotDelta * lLastPotDelta) < 0))
        {
            //
            // If the direction is reversed, but the change is less than a
            // certain amount, then don't accept the new potentiometer reading,
            // and set it back to its previous value.
            //
            if(ABS(lPotDelta) < 20)
            {
                ulPotPosition = ulLastPotPosition;
            }

            //
            // else, if the direction is reversed and changed more than a
            // certain amount, then just leave the new pot reading as is,
            // and remember the difference.
            //
            else
            {
                lLastPotDelta = lPotDelta;
            }
        }

        //
        // Else, the direction of the pot move was not reversed. Remember
        // the (signed) difference. The new pot value will be used below.
        //
        else
        {
            lLastPotDelta = lPotDelta;
        }

        //
        // Check to see if the potentiometer has moved.
        //
        if(ulPotPosition != ulLastPotPosition)
        {
            ulLastPotPosition = ulPotPosition;

            //
            // In position mode, the pot indicates the motor target
            // position, so update the target position and command the
            // motor to move.
            //
            if(eUIMode == UI_MODE_POSITION)
            {
                //
                // Make sure that after conversion to steps, the target
                // position actually changed, before sending a new motion
                // command.
                // The pot position is adjusted so that one revolution
                // of the pot matches 200 steps, the number of steps in
                // the motor. This gives the position mode a 1-1 feel,
                // as the pot is turned, the motor turns a corresponding
                // amount.
                //
                if(((ulPotPosition * 256 * 100) / 507) != g_lTargetPos)
                {
                    g_lTargetPos = (ulPotPosition * 256 * 100) / 507;
                    UISetMotion();
                }
            }

            //
            // In speed mode, the pot indicates the motor speed, so
            // command the motor to move using the new speed.
            //
            else if(eUIMode == UI_MODE_SPEED)
            {
                StepperSetMotion(g_lTargetPos, ulPotPosition,
                     g_sParameters.usAccel, g_sParameters.usDecel);
            }
        }

    }
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
    unsigned long ulSysTickVal;

    //
    // Enable the GPIO peripherals needed for the button and LEDs
    //
    SysCtlPeripheralEnable(USER_BUTTON_GPIO_PERIPH);
    SysCtlPeripheralEnable(LED_GPIO_PERIPH);

    //
    // Set up button GPIO as input, and LEDs as outputs, and turn them off
    //
    GPIODirModeSet(USER_BUTTON_PORT, USER_BUTTON_PIN, GPIO_DIR_MODE_IN);
    GPIODirModeSet(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_DIR_MODE_OUT);
    GPIODirModeSet(MODE_LED_PORT, MODE_LED_PIN, GPIO_DIR_MODE_OUT);
    GPIOPinWrite(STATUS_LED_PORT, STATUS_LED_PIN, 0);
    GPIOPinWrite(MODE_LED_PORT, MODE_LED_PIN, 0);

    //
    // Set up the LED blinking function
    //
    BlinkInit(STATUS_LED, STATUS_LED_PORT, STATUS_LED_PIN);
    BlinkInit(MODE_LED, MODE_LED_PORT, MODE_LED_PIN);
    BlinkStart(MODE_LED, UI_INT_RATE / 2, UI_INT_RATE / 2, eUIMode + 1);

    //
    // Enable the ADC peripheral, needed for potentiometer
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC0);

    //
    // Set the ADC to run at the maximum rate of 500 ksamples.
    //
    HWREG(SYSCTL_RCGC0) |= 0x00000200;
    HWREG(SYSCTL_SCGC0) |= 0x00000200;

    //
    // Program sequencer for collecting ADC sample for potentiometer
    // position, bus voltage, and temperature sensor.
    //
    ADCSequenceConfigure(ADC0_BASE, UI_ADC_SEQUENCER, ADC_TRIGGER_PROCESSOR,
                         UI_ADC_PRIORITY);
    ADCSequenceStepConfigure(ADC0_BASE, UI_ADC_SEQUENCER, 0, POT_ADC_CHAN);
    ADCSequenceStepConfigure(ADC0_BASE, UI_ADC_SEQUENCER, 1, BUSV_ADC_CHAN);
    ADCSequenceStepConfigure(ADC0_BASE, UI_ADC_SEQUENCER, 2,
                             ADC_CTL_TS | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, UI_ADC_SEQUENCER);
    ADCProcessorTrigger(ADC0_BASE, UI_ADC_SEQUENCER);   // take initial sample

    //
    // initialize the lower level,
    // positioner, which handles computing all the motion control
    //
    StepperInit();

    //
    // Get a pointer to the stepper status.
    //
    pStepperStatus = StepperGetMotorStatus();

    //
    // Force an update of all the parameters (sets defaults).
    //
    UISetPWMFreq();
    UISetChopperBlanking();
    UISetMotorParms();
    UISetControlMode();
    UISetDecayMode();
    UISetStepMode();
    UISetFixedOnTime();

    //
    // Initialize the flash parameter block driver.
    //
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, FLASH_PB_SIZE);

    //
    // Initialize the serial user interface.
    //
    UISerialInit();
    IntPrioritySet(INT_UART0, UI_SER_INT_PRI);

    //
    // Make sure that the UART doesnt get put to sleep
    //
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);

    //
    // Initialize the on-board user interface.
    //
    UIOnboardInit(GPIOPinRead(USER_BUTTON_PORT, USER_BUTTON_PIN), 0);

    //
    // Initialize the processor usage routine.
    //
    CPUUsageInit(SysCtlClockGet(), UI_INT_RATE, 2);

    //
    // Configure SysTick to provide a periodic user interface interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / UI_INT_RATE);
    SysTickIntEnable();
    SysTickEnable();
    IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRI);

    //
    // A delay is needed to let the current sense line discharge after
    // reset, before the current fault parameter is configured.  The
    // two loops below let the systick roll around once before proceeding.
    //
    ulSysTickVal = SysTickValueGet();

    //
    // Wait for systick to reach 0 and roll over to top.
    //
    while(SysTickValueGet() <= ulSysTickVal)
    {
    }

    //
    // Wait for systick to get back to the starting value.
    //
    while(SysTickValueGet() > ulSysTickVal)
    {
    }

    //
    // Now set the current fault parameter (after the delay above).
    //
    UISetFaultParms();

    //
    // Load stored parameters from flash, if any are available.
    //
    UIParamLoad();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
