//*****************************************************************************
//
// servo_if.c - Driver for the servo (PWM) input interface.
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

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/systick.h"
#include "driverlib/watchdog.h"
#include "shared/can_proto.h"
#include "commands.h"
#include "constants.h"
#include "controller.h"
#include "led.h"
#include "param.h"
#include "pins.h"
#include "servo_if.h"

//*****************************************************************************
//
// The servo input calibration routine is idle (in other words, there is no
// calibration active).
//
//*****************************************************************************
#define CALIBRATE_STATE_IDLE    0

//*****************************************************************************
//
// The servo input calibration routine is sampling the values of the servo
// input.
//
//*****************************************************************************
#define CALIBRATE_STATE_ACTIVE  1

//*****************************************************************************
//
// The servo input calibration routine is adjusting the servo input
// interpretation based on the collected calibration samples.
//
//*****************************************************************************
#define CALIBRATE_STATE_ADJUST  2

//*****************************************************************************
//
// The time of the rising edge of the previous servo pulse.
//
//*****************************************************************************
static unsigned long g_ulLastServoPulseRiseTime;

//*****************************************************************************
//
// The time of the rising edge of the current servo pulse.
//
//*****************************************************************************
static unsigned long g_ulServoPulseRiseTime;

//*****************************************************************************
//
// The minimum servo pulse width seen during calibration.
//
//*****************************************************************************
static unsigned long g_ulMinPulseWidth;

//*****************************************************************************
//
// The maximum servo pulse width seen during calibration.
//
//*****************************************************************************
static unsigned long g_ulMaxPulseWidth;

//*****************************************************************************
//
// The width of the most recent servo pulse during calibration.
//
//*****************************************************************************
static unsigned long g_ulLastPulseWidth;

//*****************************************************************************
//
// The state of the calibration routine.
//
//*****************************************************************************
static unsigned long g_ulCalibrate = CALIBRATE_STATE_IDLE;

//*****************************************************************************
//
// This function takes the width of an input servo pulse and determines the
// voltage command that corresponds to the input pulse.
//
//*****************************************************************************
static long
ServoIFPulseInterpret(unsigned long ulWidth)
{
    //
    // See if the servo pulse width is less than the neutral pulse width.
    //
    if(ulWidth < g_sParameters.ulServoNeutralWidth)
    {
        //
        // Compute the scaled voltage command based on the negative width of
        // the servo calibration.
        //
        ulWidth = ((((g_sParameters.ulServoNeutralWidth - ulWidth) * 32768) -
                    (g_sParameters.ulServoNegativeWidth / 2)) /
                   g_sParameters.ulServoNegativeWidth);

        //
        // Return the voltage command with limiting.
        //
        if(ulWidth > 32768)
        {
            return(-32768);
        }
        else
        {
            return(0 - ulWidth);
        }
    }
    else
    {
        //
        // Compute the scaled voltage command based on the positive width of
        // the servo calibration.
        //
        ulWidth = ((((ulWidth - g_sParameters.ulServoNeutralWidth) * 32767) +
                    (g_sParameters.ulServoPositiveWidth / 2)) /
                   g_sParameters.ulServoPositiveWidth);

        //
        // Return the voltage command with limiting.
        //
        if(ulWidth > 32767)
        {
            return(32767);
        }
        else
        {
            return(ulWidth);
        }
    }
}

//*****************************************************************************
//
// This function is called each time there is an input edge interrupt on the
// servo input.  The time of the edge is taken and compared against the times
// of previous edges in order to compute with width and period of the input
// servo signal.
//
//*****************************************************************************
void
ServoIFIntHandler(void)
{
    unsigned long ulNow, ulPeriod, ulWidth;

    //
    // Save the current time.
    //
    ulNow = ROM_SysTickValueGet();

    //
    // Clear the servo interrupt.
    //
    ROM_GPIOPinIntClear(SERVO_PORT, SERVO_PIN);

    //
    // See if this was a rising or falling edge of the servo input.  On the
    // board, the servo signal passes through an inverting opto isolator, so
    // the sense of the servo signal is inverted here as well.
    //
    if(ROM_GPIOPinRead(SERVO_PORT, SERVO_PIN) == 0)
    {
        //
        // On a rising edge, simply save the time of the edge.
        //
        g_ulLastServoPulseRiseTime = g_ulServoPulseRiseTime;
        g_ulServoPulseRiseTime = ulNow;
    }
    else
    {
        //
        // On a falling edge, compute the period of the servo input.  See if
        // the SysTick timer wrapped during the servo input period.
        //
        if(g_ulLastServoPulseRiseTime > g_ulServoPulseRiseTime)
        {
            //
            // The two edges occurred during the same SysTick period.
            //
            ulPeriod = g_ulLastServoPulseRiseTime - g_ulServoPulseRiseTime;
        }
        else
        {
            //
            // The two edges occurred across a wrapped SysTick period.
            //
            ulPeriod = (16777216 - g_ulServoPulseRiseTime +
                        g_ulLastServoPulseRiseTime);
        }

        //
        // Compute the pulse width of the servo input.  See if the SysTick
        // timer wrapped during the servo input pulse.
        //
        if(g_ulServoPulseRiseTime > ulNow)
        {
            //
            // The rising and falling edges are in the same SysTick period.
            //
            ulWidth = g_ulServoPulseRiseTime - ulNow;
        }
        else
        {
            //
            // The rising and falling edges occurred across a wrapped SysTick
            // period.
            //
            ulWidth = (16777216 - ulNow) + g_ulServoPulseRiseTime;
        }

        //
        // See if the servo pulse is valid.  The period and pulse width must be
        // within reasonable bounds.
        //
        if((ulPeriod >= SERVO_MIN_PERIOD) &&
           (ulPeriod <= SERVO_MAX_PERIOD) &&
           (ulWidth >= SERVO_MIN_PULSE_WIDTH) &&
           (ulWidth <= SERVO_MAX_PULSE_WIDTH))
        {
            //
            // Indicate that the servo link is good.
            //
            ControllerLinkGood(LINK_TYPE_SERVO);

            //
            // See if calibration mode is active.
            //
            if(g_ulCalibrate == CALIBRATE_STATE_IDLE)
            {
                //
                // Calibration mode is not active, so interpret the input pulse
                // and send a voltage command.
                //
                CommandVoltageSet(ServoIFPulseInterpret(ulWidth));
            }

            //
            // See if calibration mode is active.
            //
            else if(g_ulCalibrate == CALIBRATE_STATE_ACTIVE)
            {
                //
                // See if the width of this pulse is shorter than the shortest
                // pulse seen so far.  If so, then save the width of this pulse
                // as the shortest pulse.
                //
                if(ulWidth < g_ulMinPulseWidth)
                {
                    g_ulMinPulseWidth = ulWidth;
                }

                //
                // See if the width of this pulse is longer than the longest
                // pulse seen so far.  If so, then save the width of this pulse
                // as the longest pulse.
                //
                if(ulWidth > g_ulMaxPulseWidth)
                {
                    g_ulMaxPulseWidth = ulWidth;
                }

                //
                // Save the width of this pulse.  The most recent pulse width
                // when calibration mode completes will be used as the neutral
                // pulse width.
                //
                g_ulLastPulseWidth = ulWidth;
            }
        }
        else
        {
            //
            // The servo pulse width or period was out of bounds, so indicate
            // that the servo link was lost.
            //
            ControllerLinkLost(LINK_TYPE_SERVO);
        }
    }

    //
    // Tell the controller that servo activity was detected.
    //
    ControllerWatchdog(LINK_TYPE_SERVO);
}

//*****************************************************************************
//
// This function prepares the servo input interface for operation.
//
//*****************************************************************************
void
ServoIFInit(void)
{
    //
    // Initialize the internal variables.
    //
    g_ulServoPulseRiseTime = 0;
    g_ulLastServoPulseRiseTime = 0;

    //
    // Configure the servo input as a GPIO input.
    //
    ROM_GPIODirModeSet(SERVO_PORT, SERVO_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(SERVO_PORT, SERVO_PIN, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPD);

    //
    // Configure the servo input to generate an interrupt on every rising or
    // falling edge.
    //
    ROM_GPIOIntTypeSet(SERVO_PORT, SERVO_PIN, GPIO_BOTH_EDGES);
    ROM_GPIOPinIntEnable(SERVO_PORT, SERVO_PIN);
    ROM_IntEnable(SERVO_INT);

    //
    // Enable and configure the SysTick timer for use as a time base that is
    // read when the servo input interrupt is handled.
    //
    ROM_SysTickPeriodSet(16777216);
    ROM_SysTickEnable();
}

//*****************************************************************************
//
// This function begins the servo calibration process.  During the calibration
// process, servo input pulses are logged but do not affect the state of the
// motor.  The following pulse width information is logged:
//
// * The width of the shortest pulse seen (the minimum pulse width).
// * The width of the longest pulse seen (the maximum pulse width).
// * The width of the most recent pulse seen (the neutral pulse width).
//
//*****************************************************************************
void
ServoIFCalibrationStart(void)
{
    //
    // Do not start the calibration process when there is an active fault, of
    // if the servo interface is not in use.
    //
    if(((ControllerFaultsActive() != 0) &&
        (ControllerFaultsActive() != LM_FAULT_COMM)) ||
       (ControllerLinkType() != LINK_TYPE_SERVO))
    {
        return;
    }

    //
    // Set the minimum and maximum pulse widths such that the first pulse seen
    // will be both the minimum and maximum.
    //
    g_ulMinPulseWidth = 0xffffffff;
    g_ulMaxPulseWidth = 0;

    //
    // Start calibration mode.
    //
    g_ulCalibrate = CALIBRATE_STATE_ACTIVE;

    //
    // Put the drive into neutral.
    //
    CommandVoltageSet(0);

    //
    // Indicate that the servo calibration process is under way via the LED.
    //
    LEDCalibrateStart();
}

//*****************************************************************************
//
// This function ends the servo calibration process.  The servo pulses that
// were logged are analyzed to determine they are valid, and if so they are
// used to compute the new calibration parameters for the servo input.
//
// The servo pulses are considered to be valid if all of the following are
// true:
//
// * The minimum pulse width is less the the neutral pulse width.
// * The neutral pulse width is less than the maximum pulse width.
// * The neutral pulse width is reasonably close to 1.5 ms.
// * The range from the minimum pulse width to the maximum pulse width is wide
//   enough.
// * The neutral pulse width is reasonably centered between the minimum and
//   maximum pulse widths.
//
// On a successful calibration, the new calibration values are written into the
// parameter block and stored into flash.
//
//*****************************************************************************
void
ServoIFCalibrationEnd(void)
{
    unsigned long ulDiff1, ulDiff2, ulDelta;

    //
    // Do nothing if the servo calibration process is not in progress.
    //
    if(g_ulCalibrate == CALIBRATE_STATE_IDLE)
    {
        return;
    }

    //
    // Stop calibration mode.
    //
    g_ulCalibrate = CALIBRATE_STATE_ADJUST;

    //
    // Fail the calibration if the detected pulse widths are not ordered
    // correctly (i.e. the minimum pulse width must be less than the neutral
    // pulse width which must be less than the maximum pulse width).
    //
    if((g_ulMinPulseWidth > g_ulLastPulseWidth) ||
       (g_ulLastPulseWidth > g_ulMaxPulseWidth))
    {
        LEDCalibrateFail();
        g_ulCalibrate = CALIBRATE_STATE_IDLE;
        return;
    }

    //
    // Fail the calibration if the neutral position is not close enough to
    // 1.5 ms.
    //
    if((g_ulLastPulseWidth < (SERVO_DEFAULT_NEU_WIDTH - SERVO_NEUTRAL_SLOP)) ||
       (g_ulLastPulseWidth > (SERVO_DEFAULT_NEU_WIDTH + SERVO_NEUTRAL_SLOP)))
    {
        LEDCalibrateFail();
        g_ulCalibrate = CALIBRATE_STATE_IDLE;
        return;
    }

    //
    // Compute the difference between the neutral pulse width and the minimum/
    // maximum pulse widths.
    //
    ulDiff1 = g_ulLastPulseWidth - g_ulMinPulseWidth;
    ulDiff2 = g_ulMaxPulseWidth - g_ulLastPulseWidth;

    //
    // Fail the calibration if the difference between the neutral pulse width
    // and the minimum/maximum pulse widths are too small.
    //
    if((ulDiff1 < SERVO_RANGE_MIN) || (ulDiff2 < SERVO_RANGE_MIN))
    {
        LEDCalibrateFail();
        g_ulCalibrate = CALIBRATE_STATE_IDLE;
        return;
    }

    //
    // Fail the calibration if the neutral pulse width is not reasonably
    // centered between the minimum and maximum pulse widths.
    //
    ulDelta = (ulDiff1 > ulDiff2) ? ulDiff1 - ulDiff2 : ulDiff2 - ulDiff1;
    if(ulDelta > SERVO_NEUTRAL_SLOP)
    {
        LEDCalibrateFail();
        g_ulCalibrate = CALIBRATE_STATE_IDLE;
        return;
    }

    //
    // Save the widths of the negative, neutral, and positive pulses.
    //
    g_sParameters.ulServoNegativeWidth = ulDiff1;
    g_sParameters.ulServoNeutralWidth = g_ulLastPulseWidth;
    g_sParameters.ulServoPositiveWidth = ulDiff2;

    //
    // Save the configuration.
    //
    ParamSave();

    //
    // Move the calibration process into the idle state.
    //
    g_ulCalibrate = CALIBRATE_STATE_IDLE;

    //
    // Indicate on the LED that the calibration was successful.
    //
    LEDCalibrateSuccess();
}

//*****************************************************************************
//
// This function aborts the servo calibration process, abandoning any results
// that may have been collected.
//
//*****************************************************************************
void
ServoIFCalibrationAbort(void)
{
    //
    // Force the calibration process into the idle state.
    //
    g_ulCalibrate = CALIBRATE_STATE_IDLE;
}
