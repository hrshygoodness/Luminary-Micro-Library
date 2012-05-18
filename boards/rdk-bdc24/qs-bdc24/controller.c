//*****************************************************************************
//
// controller.c - The motor controller.
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

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "shared/can_proto.h"
#include "adc_ctrl.h"
#include "button.h"
#include "commands.h"
#include "constants.h"
#include "controller.h"
#include "encoder.h"
#include "fan.h"
#include "hbridge.h"
#include "led.h"
#include "limit.h"
#include "math.h"
#include "message.h"
#include "pid.h"
#include "servo_if.h"

//*****************************************************************************
//
// The amount of time that the controller stays in neutral after a fault
// condition.
//
//*****************************************************************************
static unsigned long g_ulFaultTime = FAULT_TIME;

//*****************************************************************************
//
// A count down time for how long after a fault occurs before the fault
// condition is cleared and the controller reset.
//
//*****************************************************************************
static unsigned long g_ulFaultCounter = 0;

//*****************************************************************************
//
// The current drive voltage.  This is determined by the current control mode
// (voltage, voltage compensation, current, position, or speed).
//
//*****************************************************************************
static long g_lVoltage = 0;

//*****************************************************************************
//
// The target voltage for voltage control mode.
//
//*****************************************************************************
static long g_lVoltageTarget = 0;

//*****************************************************************************
//
// The rate at which the voltage is adjusted from the current value to the
// target value.  This is defined as the number of steps per millisecond.
//
//*****************************************************************************
static unsigned long g_ulVoltageRate = 0;

//*****************************************************************************
//
// The target speed for speed control mode.
//
//*****************************************************************************
static long g_lSpeedTarget = 0;

//*****************************************************************************
//
// The PID controller used in speed control mode.
//
//*****************************************************************************
static tPIDState g_sSpeedPID;

//*****************************************************************************
//
// The desired drive voltage for voltage compensation control mode.
//
//*****************************************************************************
static long g_lVCompVoltage = 0;

//*****************************************************************************
//
// The target voltage for voltage compensation control mode.
//
//*****************************************************************************
static long g_lVCompTarget = 0;

//*****************************************************************************
//
// The rate at which the target voltage is adjusted from the current value to
// the target value.  This is defined as the number of volts per millisecond,
// specified as an 8.8 fixed-point number.
//
//*****************************************************************************
static unsigned long g_ulVCompInRate = 0;

//*****************************************************************************
//
// The rate at which the output voltage is adjusted based on changes in the bus
// voltage.  This is defined as the number of volts per millisecond, specified
// as an 8.8 fixed-point number.
//
//*****************************************************************************
static unsigned long g_ulVCompCompRate = 0;

//*****************************************************************************
//
// The target encoder position in encoder position mode.
//
//*****************************************************************************
static long g_lPositionTarget = 0;

//*****************************************************************************
//
// The PID controller used in encoder position mode.
//
//*****************************************************************************
static tPIDState g_sPositionPID;

//*****************************************************************************
//
// The target current for current control mode.
//
//*****************************************************************************
static long g_lCurrentTarget = 0;

//*****************************************************************************
//
// The PID controller used in current control mode.
//
//*****************************************************************************
static tPIDState g_sCurrentPID;

//*****************************************************************************
//
// A set of flags that indicate the current status of the motor controller.
//
//*****************************************************************************
#define FLAG_FAULT              0
#define FLAG_LOST_LINK          1
#define FLAG_HAVE_LINK          2
#define FLAG_SERVO_LINK         3
#define FLAG_CAN_LINK           4
#define FLAG_UART_LINK          5
#define FLAG_VCOMP_MODE         6
#define FLAG_CURRENT_MODE       7
#define FLAG_SPEED_MODE         8
#define FLAG_POSITION_MODE      9
#define FLAG_SPEED_SRC_ENCODER  10
#define FLAG_SPEED_SRC_INV_ENC  11
#define FLAG_SPEED_SRC_QUAD_ENC 12
#define FLAG_POS_SRC_ENCODER    13
#define FLAG_POS_SRC_POT        14
#define FLAG_POWER_STATUS       15
#define FLAG_HALTED             16
static unsigned long g_ulFlags = 0;

//*****************************************************************************
//
// A set of flags that indicate the reason for the current fault condition.
//
//*****************************************************************************
static unsigned long g_ulFaultFlags = 0;

//*****************************************************************************
//
// The same set of flags, but they are sticky (they must be read to be
// cleared).
//
//*****************************************************************************
static unsigned long g_ulStickyFaultFlags = 0;

//*****************************************************************************
//
// Global counter for current faults.
//
//*****************************************************************************
static unsigned char g_ucFaultCountCurrent = 0;

//*****************************************************************************
//
// Global counter for Temperature faults.
//
//*****************************************************************************
static unsigned char g_ucFaultCountTemp = 0;

//*****************************************************************************
//
// Global counter for VBus faults.
//
//*****************************************************************************
static unsigned char g_ucFaultCountVBus = 0;

//*****************************************************************************
//
// Global counter for Gate faults.
//
//*****************************************************************************
static unsigned char g_ucFaultCountGate = 0;

//*****************************************************************************
//
// Global counter for COMM faults.
//
//*****************************************************************************
static unsigned char g_ucFaultCountCOMM = 0;

//*****************************************************************************
//
// The current state of the controller.
//
//*****************************************************************************
#define STATE_WAIT_FOR_LINK     0
#define STATE_RUN               1
#define STATE_FAULT             2
static unsigned long g_ulControllerState = STATE_WAIT_FOR_LINK;

//*****************************************************************************
//
// This function sets the time that the controller stays in neutral after a
// fault condition.
//
//*****************************************************************************
void
ControllerFaultTimeSet(unsigned long ulTime)
{
    //
    // The minimum fault time is 500ms.
    //
    if(ulTime < 500)
    {
        ulTime = 500;
    }

    //
    // Convert the fault time from milliseconds to controller ticks.
    //
    g_ulFaultTime = (ulTime * UPDATES_PER_SECOND) / 1000;
}

//*****************************************************************************
//
// This function gets the time that the controller stays in neutral after a
// fault condition.
//
//*****************************************************************************
unsigned long
ControllerFaultTimeGet(void)
{
    //
    // Convert the fault time from controller ticks to milliseconds.
    //
    return((g_ulFaultTime * 1000) / UPDATES_PER_SECOND);
}

//*****************************************************************************
//
// This function indicates that the control link is good.
//
//*****************************************************************************
void
ControllerLinkGood(unsigned long ulType)
{
    //
    // See if the control link is CAN.
    //
    if(ulType == LINK_TYPE_CAN)
    {
        //
        // Indicate that the control link is CAN.
        //
        HWREGBITW(&g_ulFlags, FLAG_SERVO_LINK) = 0;
        HWREGBITW(&g_ulFlags, FLAG_UART_LINK) = 0;
        HWREGBITW(&g_ulFlags, FLAG_CAN_LINK) = 1;
    }
    else if(ulType == LINK_TYPE_UART)
    {
        //
        // Indicate that the control link is UART.
        //
        HWREGBITW(&g_ulFlags, FLAG_SERVO_LINK) = 0;
        HWREGBITW(&g_ulFlags, FLAG_CAN_LINK) = 0;
        HWREGBITW(&g_ulFlags, FLAG_UART_LINK) = 1;
    }
    else if(ulType == LINK_TYPE_SERVO)
    {
        //
        // Indicate that the control link is servo.
        //
        HWREGBITW(&g_ulFlags, FLAG_CAN_LINK) = 0;
        HWREGBITW(&g_ulFlags, FLAG_UART_LINK) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SERVO_LINK) = 1;
    }

    //
    // Indicate that the control link is good.
    //
    HWREGBITW(&g_ulFlags, FLAG_HAVE_LINK) = 1;
}

//*****************************************************************************
//
// This function indicates that the control link has been lost.
//
//*****************************************************************************
void
ControllerLinkLost(unsigned long ulType)
{
    //
    // Clear the link type.
    //
    switch(ulType)
    {
        //
        // Clear the servo link flag.
        //
        case LINK_TYPE_SERVO:
        {
            HWREGBITW(&g_ulFlags, FLAG_SERVO_LINK) = 0;

            break;
        }

        //
        // Clear the CAN link flag.
        //
        case LINK_TYPE_CAN:
        {
            HWREGBITW(&g_ulFlags, FLAG_CAN_LINK) = 0;

            break;
        }

        //
        // Clear the UART link flag.
        //
        case LINK_TYPE_UART:
        {
            HWREGBITW(&g_ulFlags, FLAG_UART_LINK) = 0;

            break;
        }

        //
        // Clear all link flags.
        //
        default:
        {
            HWREGBITW(&g_ulFlags, FLAG_SERVO_LINK) = 0;
            HWREGBITW(&g_ulFlags, FLAG_CAN_LINK) = 0;
            HWREGBITW(&g_ulFlags, FLAG_UART_LINK) = 0;
            break;
        }
    }

    //
    // Indicate that the control link has been lost.
    //
    HWREGBITW(&g_ulFlags, FLAG_LOST_LINK) = 1;
}

//*****************************************************************************
//
// This function returns the type of control link that is in use.
//
//*****************************************************************************
unsigned long
ControllerLinkType(void)
{
    //
    // Determine the type of control link (if any) and return it.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_SERVO_LINK) == 1)
    {
        return(LINK_TYPE_SERVO);
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_CAN_LINK) == 1)
    {
        return(LINK_TYPE_CAN);
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_UART_LINK) == 1)
    {
        return(LINK_TYPE_UART);
    }
    else
    {
        return(LINK_TYPE_NONE);
    }
}

//*****************************************************************************
//
// This function returns whether or not the link is still active.
//
//*****************************************************************************
unsigned long
ControllerLinkActive(void)
{
    return(HWREGBITW(&g_ulFlags, FLAG_HAVE_LINK));
}

//*****************************************************************************
//
// This function handles whether or not the watchdog should be prevented from
// expiring based on the type of events that are passed in.
//
//*****************************************************************************
void
ControllerWatchdog(unsigned long ulType)
{
    //
    // Delay the watchdog expiring.
    //
    ROM_WatchdogIntClear(WATCHDOG0_BASE);
}

//*****************************************************************************
//
// This function is used to indicate that a fault has occurred.
//
//*****************************************************************************
void
ControllerFaultSignal(unsigned long ulFault)
{
    unsigned long ulIsSet;

    //
    // See if this fault is being set for the first time.
    //
    ulIsSet = g_ulFaultFlags & ulFault & ~(LM_FAULT_COMM);

    //
    // Save the new fault in the fault flags.
    //
    g_ulFaultFlags |= ulFault & ~(LM_FAULT_COMM);

    //
    // Also save the new fault in the sticky fault flags.
    //
    g_ulStickyFaultFlags |= ulFault;

    //
    // See if this fault was set for the first time.
    //
    if(ulIsSet == 0)
    {
        //
        // Increment the corresponding fault counter for the new fault.
        //
        switch(ulFault)
        {
            //
            // Increment the Current Fault counter and saturate at 255.
            //
            case LM_FAULT_CURRENT:
            {
                if(g_ucFaultCountCurrent < 255)
                {
                    g_ucFaultCountCurrent++;
                }

                break;
            }

            //
            // Increment the Temperature Fault counter and saturate at 255.
            //
            case LM_FAULT_TEMP:
            {
                if(g_ucFaultCountTemp < 255)
                {
                    g_ucFaultCountTemp++;
                }

                break;
            }

            //
            // Increment the VBus Fault counter and saturate at 255.
            //
            case LM_FAULT_VBUS:
            {
                if(g_ucFaultCountVBus < 255)
                {
                    g_ucFaultCountVBus++;
                }

                break;
            }

            //
            // Increment the Gate Fault counter and saturate at 255.
            //
            case LM_FAULT_GATE_DRIVE:
            {
                if(g_ucFaultCountGate < 255)
                {
                    g_ucFaultCountGate++;
                }

                break;
            }

            //
            // Increment the COMM Fault counter and saturate at 255.
            //
            case LM_FAULT_COMM:
            {
                if(g_ucFaultCountCOMM < 255)
                {
                    g_ucFaultCountCOMM++;
                }

                break;
            }

            //
            // An invalid fault condition, don't increment anything.
            //
            default:
            {
                break;
            }
        }
    }

    //
    // Indicate that there has been a fault.
    //
    if(ulFault != LM_FAULT_COMM)
    {
        HWREGBITW(&g_ulFlags, FLAG_FAULT) = 1;
    }
}

//*****************************************************************************
//
// The function returns the set of faults that are currently active (if any).
//
//*****************************************************************************
unsigned long
ControllerFaultsActive(void)
{
    //
    // Return the set of currently active faults.
    //
    return(g_ulFaultFlags);
}

//*****************************************************************************
//
// The function returns the set of sticky fault flags that are currently active
// (if any) and then clears them.
//
//*****************************************************************************
unsigned long
ControllerStickyFaultsActive(unsigned long bClear)
{
    unsigned long ulTemp;

    //
    // Read the sticky fault flags.
    //
    ulTemp = g_ulStickyFaultFlags;

    //
    // If requested, clear the sticky flags now that they have been read.
    //
    if(bClear)
    {
        g_ulStickyFaultFlags = 0;
    }

    //
    // Return the status of the sticky flags before they were cleared.
    //
    return(ulTemp);
}

//*****************************************************************************
//
// This function returns the number of current faults that have occurred.
//
//*****************************************************************************
unsigned char
ControllerCurrentFaultsGet(void)
{
    //
    // Return the value of the current fault counter.
    //
    return(g_ucFaultCountCurrent);
}

//*****************************************************************************
//
// This function returns the number of temperature faults that have occurred.
//
//*****************************************************************************
unsigned char
ControllerTemperatureFaultsGet(void)
{
    //
    // Return the value of the temperature fault counter.
    //
    return(g_ucFaultCountTemp);
}

//*****************************************************************************
//
// This function returns the number of Vbus faults that have occurred.
//
//*****************************************************************************
unsigned char
ControllerVBusFaultsGet(void)
{
    //
    // Return the value of the Vbus fault counter.
    //
    return(g_ucFaultCountVBus);
}

//*****************************************************************************
//
// This function returns the number of gate faults that have occurred.
//
//*****************************************************************************
unsigned char
ControllerGateFaultsGet(void)
{
    //
    // Return the value of the gate fault counter.
    //
    return(g_ucFaultCountGate);
}

//*****************************************************************************
//
// This function returns the number of communication faults that have occurred.
//
//*****************************************************************************
unsigned char
ControllerCommunicationFaultsGet(void)
{
    //
    // Return the value of the communication fault counter.
    //
    return(g_ucFaultCountCOMM);
}

//*****************************************************************************
//
// The function resets the specified fault counters to zero.
//
//*****************************************************************************
void
ControllerFaultCountReset(unsigned long ulCounters)
{
    //
    // Set the indicated fault counters to zero.
    //
    if(ulCounters & 1)
    {
        g_ucFaultCountCurrent = 0;
    }
    if(ulCounters & 2)
    {
        g_ucFaultCountTemp = 0;
    }
    if(ulCounters & 4)
    {
        g_ucFaultCountVBus = 0;
    }
    if(ulCounters & 8)
    {
        g_ucFaultCountGate = 0;
    }
    if(ulCounters & 16)
    {
        g_ucFaultCountCOMM = 0;
    }
}

//*****************************************************************************
//
// This function is used to force the motor controller into neutral.
//
//*****************************************************************************
void
ControllerForceNeutral(void)
{
    //
    // Reset the current voltage to neutral.
    //
    g_lVoltage = 0;
    HBridgeVoltageSet(0);

    //
    // Reset the target voltage to neutral.
    //
    g_lVoltageTarget = 0;

    //
    // Reset the target voltage compensation to neutral.
    //
    g_lVCompVoltage = 0;
    g_lVCompTarget = 0;

    //
    // Reset the speed target to zero and reset the PID controller.
    //
    g_lSpeedTarget = 0;
    PIDReset(&g_sSpeedPID);

    //
    // Reset the position PID controller.
    //
    g_lPositionTarget = ControllerPositionGet();
    PIDReset(&g_sPositionPID);

    //
    // Reset the current target to zero and reset the PID controller.
    //
    g_lCurrentTarget = 0;
    PIDReset(&g_sCurrentPID);
}

//*****************************************************************************
//
// Return the current motor controller control mode.
//
//*****************************************************************************
unsigned char
ControllerControlModeGet(void)
{
    unsigned char ucRet;

    //
    // Check for the current control mode.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_VCOMP_MODE))
    {
        ucRet = LM_STATUS_CMODE_VCOMP;
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_CURRENT_MODE))
    {
        ucRet = LM_STATUS_CMODE_CURRENT;
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_SPEED_MODE))
    {
        ucRet = LM_STATUS_CMODE_SPEED;
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_POSITION_MODE))
    {
        ucRet = LM_STATUS_CMODE_POS;
    }
    else
    {
        ucRet = LM_STATUS_CMODE_VOLT;
    }

    return(ucRet);
}

//*****************************************************************************
//
// This function returns the current output voltage.
//
//*****************************************************************************
long
ControllerVoltageGet(void)
{
    //
    // Return the current output voltage.
    //
    return(g_lVoltage);
}

//*****************************************************************************
//
// This function is used to enable or disable voltage control mode.
//
//*****************************************************************************
void
ControllerVoltageModeSet(unsigned long bEnable)
{
    //
    // Force the controller into neutral.
    //
    ControllerForceNeutral();

    //
    // See if voltage control mode is being enabled.
    //
    if(bEnable)
    {
        //
        // Disable the other control modes, which enables voltage control mode
        // since it is the default.
        //
        HWREGBITW(&g_ulFlags, FLAG_VCOMP_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_CURRENT_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_POSITION_MODE) = 0;
    }
}

//*****************************************************************************
//
// This function is used to set the target output voltage when in voltage
// control mode.
//
//*****************************************************************************
void
ControllerVoltageSet(long lVoltage)
{
    //
    // If the voltage is less than the start of the reverse voltage plateau,
    // then set the voltage to full reverse.
    //
    if(lVoltage < (-32768 + REVERSE_PLATEAU))
    {
        lVoltage = -32768;
    }

    //
    // If the voltage is within the neutral voltage plateau, then set the
    // voltage to neutral.
    //
    if((lVoltage >= (0 - NEUTRAL_PLATEAU)) &&
       (lVoltage <= NEUTRAL_PLATEAU))
    {
        lVoltage = 0;
    }

    //
    // If the voltage is greater than the start of the forward voltage plateau,
    // then set the voltage to full forward.
    //
    if(lVoltage >= (32767 - FORWARD_PLATEAU))
    {
        lVoltage = 32767;
    }

    //
    // Save the target voltage.
    //
    g_lVoltageTarget = lVoltage;
}

//*****************************************************************************
//
// This function returns the target output voltage when in voltage control
// mode.
//
//*****************************************************************************
long
ControllerVoltageTargetGet(void)
{
    //
    // Return the target output voltage.
    //
    return(g_lVoltageTarget);
}

//*****************************************************************************
//
// This function sets the rate of change of the output voltage when in voltage
// control mode.
//
//*****************************************************************************
void
ControllerVoltageRateSet(unsigned long ulRate)
{
    //
    // Save the voltage change rate.
    //
    g_ulVoltageRate = ulRate;
}

//*****************************************************************************
//
// This function returns the rate of change of the output voltage when in
// voltage control mode.
//
//*****************************************************************************
unsigned long
ControllerVoltageRateGet(void)
{
    //
    // Return the voltage change rate.
    //
    return(g_ulVoltageRate);
}

//*****************************************************************************
//
// This function is used to enable or disable speed control mode.
//
//*****************************************************************************
void
ControllerSpeedModeSet(unsigned long bEnable)
{
    //
    // Force the controller into neutral.
    //
    ControllerForceNeutral();

    //
    // See if speed mode is being enabled.
    //
    if(bEnable)
    {
        //
        // Enable speed mode and disable the other control modes.
        //
        HWREGBITW(&g_ulFlags, FLAG_VCOMP_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_CURRENT_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_MODE) = 1;
        HWREGBITW(&g_ulFlags, FLAG_POSITION_MODE) = 0;
    }
    else
    {
        //
        // Disable speed mode.
        //
        HWREGBITW(&g_ulFlags, FLAG_SPEED_MODE) = 0;
    }
}

//*****************************************************************************
//
// This function is used to set the target speed when in speed control mode.
//
//*****************************************************************************
void
ControllerSpeedSet(long lSpeed)
{
    //
    // Save the target speed.
    //
    g_lSpeedTarget = lSpeed;
}

//*****************************************************************************
//
// This function is used to get the target speed when in speed control mode.
//
//*****************************************************************************
long
ControllerSpeedTargetGet(void)
{
    //
    // Return the target speed.
    //
    return(g_lSpeedTarget);
}

//*****************************************************************************
//
// This function is used to get the current speed of the motor.
//
//*****************************************************************************
long
ControllerSpeedGet(void)
{
    //
    // Get the current speed from the currently configured speed sense source.
    //
    if((HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_ENCODER) == 1) ||
       (HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_INV_ENC) == 1))
    {
        return(EncoderVelocityGet(0));
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_QUAD_ENC) == 1)
    {
        return(EncoderVelocityGet(1));
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// This function is used to set the speed sense source.
//
//*****************************************************************************
void
ControllerSpeedSrcSet(unsigned long ulSrc)
{
    //
    // Set the speed sense source flag based on the specified speed sense
    // source.
    //
    if(ulSrc == LM_REF_ENCODER)
    {
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_ENCODER) = 1;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_INV_ENC) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_QUAD_ENC) = 0;
    }
    else if(ulSrc == LM_REF_INV_ENCODER)
    {
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_ENCODER) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_INV_ENC) = 1;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_QUAD_ENC) = 0;
    }
    else if(ulSrc == LM_REF_QUAD_ENCODER)
    {
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_ENCODER) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_INV_ENC) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_QUAD_ENC) = 1;
    }
    else
    {
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_ENCODER) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_INV_ENC) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_QUAD_ENC) = 0;
    }
}

//*****************************************************************************
//
// This function is used to get the speed sense source.
//
//*****************************************************************************
unsigned long
ControllerSpeedSrcGet(void)
{
    //
    // Return the speed sense source.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_ENCODER) == 1)
    {
        return(LM_REF_ENCODER);
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_INV_ENC) == 1)
    {
        return(LM_REF_INV_ENCODER);
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_QUAD_ENC) == 1)
    {
        return(LM_REF_QUAD_ENCODER);
    }
    else
    {
        return(0xffffffff);
    }
}

//*****************************************************************************
//
// This function sets the P gain of the PID controller for speed control mode.
//
//*****************************************************************************
void
ControllerSpeedPGainSet(long lPGain)
{
    //
    // Set the P gain on the speed PID controller.
    //
    PIDGainPSet(&g_sSpeedPID, lPGain);
}

//*****************************************************************************
//
// This function gets the P gain of the PID controller for speed control mode.
//
//*****************************************************************************
long
ControllerSpeedPGainGet(void)
{
    //
    // Return the P gain on the speed PID controller.
    //
    return(g_sSpeedPID.lPGain);
}

//*****************************************************************************
//
// This function sets the I gain of the PID controller for speed control mode.
//
//*****************************************************************************
void
ControllerSpeedIGainSet(long lIGain)
{
    long lTemp;

    //
    // Set the I gain and readjust the integrator min/max on the speed PID
    // controller.
    //
    lTemp = MathDiv16x16(32767 * 256, lIGain);
    if(lTemp < 0)
    {
        lTemp = 0 - lTemp;
    }
    PIDGainISet(&g_sSpeedPID, lIGain, lTemp, 0 - lTemp);
}

//*****************************************************************************
//
// This function gets the I gain of the PID controller for speed control mode.
//
//*****************************************************************************
long
ControllerSpeedIGainGet(void)
{
    //
    // Return the I gain on the speed PID controller.
    //
    return(g_sSpeedPID.lIGain);
}

//*****************************************************************************
//
// This function sets the D gain of the PID controller for speed control mode.
//
//*****************************************************************************
void
ControllerSpeedDGainSet(long lDGain)
{
    //
    // Set the D gain on the speed PID controller.
    //
    PIDGainDSet(&g_sSpeedPID, lDGain);
}

//*****************************************************************************
//
// This function gets the D gain of the PID controller for speed control mode.
//
//*****************************************************************************
long
ControllerSpeedDGainGet(void)
{
    //
    // Return the D gain on the speed PID controller.
    //
    return(g_sSpeedPID.lDGain);
}

//*****************************************************************************
//
// This function is used to enable or disable voltage compensation control
// mode.
//
//*****************************************************************************
void
ControllerVCompModeSet(unsigned long bEnable)
{
    //
    // Force the controller into neutral.
    //
    ControllerForceNeutral();

    //
    // See if voltage compensation control mode is being enabled.
    //
    if(bEnable)
    {
        //
        // Enable voltage compensation control mode and disable the other
        // control modes.
        //
        HWREGBITW(&g_ulFlags, FLAG_VCOMP_MODE) = 1;
        HWREGBITW(&g_ulFlags, FLAG_CURRENT_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_POSITION_MODE) = 0;
    }
    else
    {
        //
        // Disable voltage compensation control mode.
        //
        HWREGBITW(&g_ulFlags, FLAG_VCOMP_MODE) = 0;
    }
}

//*****************************************************************************
//
// This function is used to set the target voltage when in voltage compensation
// control mode.
//
//*****************************************************************************
void
ControllerVCompSet(long lVoltage)
{
    //
    // Save the target voltage.
    //
    g_lVCompTarget = lVoltage;
}

//*****************************************************************************
//
// This function returns the target output voltage when in voltage compensation
// control mode.
//
//*****************************************************************************
long
ControllerVCompTargetGet(void)
{
    //
    // Return the target output voltage.
    //
    return(g_lVCompTarget);
}

//*****************************************************************************
//
// This function sets the rate of change of the target voltage when in voltage
// compensation control mode.
//
//*****************************************************************************
void
ControllerVCompInRateSet(unsigned long ulRate)
{
    //
    // Save the target voltage change rate.
    //
    g_ulVCompInRate = ulRate;
}

//*****************************************************************************
//
// This function returns the rate of change of the target voltage when in
// voltage compensation control mode.
//
//*****************************************************************************
unsigned long
ControllerVCompInRateGet(void)
{
    //
    // Return the target voltage change rate.
    //
    return(g_ulVCompInRate);
}

//*****************************************************************************
//
// This function sets the rate of change of the output voltage when in voltage
// compensation control mode.
//
//*****************************************************************************
void
ControllerVCompCompRateSet(unsigned long ulRate)
{
    //
    // Save the output voltage change rate.
    //
    g_ulVCompCompRate = ulRate;
}

//*****************************************************************************
//
// This function returns the rate of change of the output voltage when in
// voltage compensation control mode.
//
//*****************************************************************************
unsigned long
ControllerVCompCompRateGet(void)
{
    //
    // Return the output voltage change rate.
    //
    return(g_ulVCompCompRate);
}

//*****************************************************************************
//
// This function is used to enable or disable position control mode.
//
//*****************************************************************************
void
ControllerPositionModeSet(unsigned long bEnable, long lStartingPosition)
{
    //
    // Force the controller into neutral.
    //
    ControllerForceNeutral();

    //
    // See if position mode is being enabled.
    //
    if(bEnable)
    {
        //
        // Enable position control mode and disable the other control modes.
        //
        HWREGBITW(&g_ulFlags, FLAG_VCOMP_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_CURRENT_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_POSITION_MODE) = 1;

        //
        // Provide the ``start'' position to the encoder interface.
        //
        EncoderPositionSet(lStartingPosition);

        //
        // Set the default target position to the current position, based on
        // the currently selected position sense source.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_POS_SRC_ENCODER) == 1)
        {
            g_lPositionTarget = lStartingPosition;
        }
        else if(HWREGBITW(&g_ulFlags, FLAG_POS_SRC_POT) == 1)
        {
            g_lPositionTarget = ADCPotPosGet();
        }
        else
        {
            g_lPositionTarget = 0;
        }
    }
    else
    {
        //
        // Disable position mode.
        //
        HWREGBITW(&g_ulFlags, FLAG_POSITION_MODE) = 0;
    }
}

//*****************************************************************************
//
// This function is used to set the target position when in position control
// mode.
//
//*****************************************************************************
void
ControllerPositionSet(long lPosition)
{
    //
    // Save the target position.
    //
    g_lPositionTarget = lPosition;
}

//*****************************************************************************
//
// This fucntion is used to get the target position when in position control
// mode.
//
//*****************************************************************************
long
ControllerPositionTargetGet(void)
{
    //
    // Return the target position.
    //
    return(g_lPositionTarget);
}

//*****************************************************************************
//
// This function is used to get the current position of the motor.
//
//*****************************************************************************
long
ControllerPositionGet(void)
{
    //
    // Get the current position from the currently configured position sense
    // source.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_POS_SRC_ENCODER) == 1)
    {
        return(EncoderPositionGet());
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_POS_SRC_POT) == 1)
    {
        return(ADCPotPosGet());
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// This function is used to set the position sense source.
//
//*****************************************************************************
void
ControllerPositionSrcSet(unsigned long ulSrc)
{
    //
    // Set the position sense source flag based on the specified position sense
    // source.
    //
    if((ulSrc == LM_REF_ENCODER) || (ulSrc == LM_REF_INV_ENCODER) ||
       (ulSrc == LM_REF_QUAD_ENCODER))
    {
        HWREGBITW(&g_ulFlags, FLAG_POS_SRC_ENCODER) = 1;
        HWREGBITW(&g_ulFlags, FLAG_POS_SRC_POT) = 0;
    }
    else if(ulSrc == LM_REF_POT)
    {
        HWREGBITW(&g_ulFlags, FLAG_POS_SRC_ENCODER) = 0;
        HWREGBITW(&g_ulFlags, FLAG_POS_SRC_POT) = 1;
    }
    else
    {
        HWREGBITW(&g_ulFlags, FLAG_POS_SRC_ENCODER) = 0;
        HWREGBITW(&g_ulFlags, FLAG_POS_SRC_POT) = 0;
    }
}

//*****************************************************************************
//
// This function is used to get the position sense source.
//
//*****************************************************************************
unsigned long
ControllerPositionSrcGet(void)
{
    //
    // Return the position sense source.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_POS_SRC_ENCODER) == 1)
    {
        return(LM_REF_ENCODER);
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_POS_SRC_POT) == 1)
    {
        return(LM_REF_POT);
    }
    else
    {
        return(0xffffffff);
    }
}

//*****************************************************************************
//
// This function sets the P gain of the PID controller for position control
// mode.
//
//*****************************************************************************
void
ControllerPositionPGainSet(long lPGain)
{
    //
    // Set the P gain on the position PID controller.
    //
    PIDGainPSet(&g_sPositionPID, lPGain);
}

//*****************************************************************************
//
// This function gets the P gain of the PID controller for position control
// mode.
//
//*****************************************************************************
long
ControllerPositionPGainGet(void)
{
    //
    // Return the P gain on the position PID controller.
    //
    return(g_sPositionPID.lPGain);
}

//*****************************************************************************
//
// This function sets the I gain of the PID controller for position control
// mode.
//
//*****************************************************************************
void
ControllerPositionIGainSet(long lIGain)
{
    long lTemp;

    //
    // Set the I gain and readjust the integrator min/max on the position PID
    // controller.
    //
    lTemp = MathDiv16x16(32767 * 256, lIGain);
    if(lTemp < 0)
    {
        lTemp = 0 - lTemp;
    }
    PIDGainISet(&g_sPositionPID, lIGain, lTemp, 0 - lTemp);
}

//*****************************************************************************
//
// This function gets the I gain of the PID controller for position control
// mode.
//
//*****************************************************************************
long
ControllerPositionIGainGet(void)
{
    //
    // Return the I gain on the position PID controller.
    //
    return(g_sPositionPID.lIGain);
}

//*****************************************************************************
//
// This function sets the D gain of the PID controller for position control
// mode.
//
//*****************************************************************************
void
ControllerPositionDGainSet(long lDGain)
{
    //
    // Set the D gain on the position PID controller.
    //
    PIDGainDSet(&g_sPositionPID, lDGain);
}

//*****************************************************************************
//
// This function gets the D gain of the PID controller for position control
// mode.
//
//*****************************************************************************
long
ControllerPositionDGainGet(void)
{
    //
    // Return the D gain on the position PID controller.
    //
    return(g_sPositionPID.lDGain);
}

//*****************************************************************************
//
// This function is used to enable or disable current control mode.
//
//*****************************************************************************
void
ControllerCurrentModeSet(unsigned long bEnable)
{
    //
    // Force the controller into neutral.
    //
    ControllerForceNeutral();

    //
    // See if current control mode is being enabled.
    //
    if(bEnable)
    {
        //
        // Enable current control mode and disable the other control modes.
        //
        HWREGBITW(&g_ulFlags, FLAG_VCOMP_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_CURRENT_MODE) = 1;
        HWREGBITW(&g_ulFlags, FLAG_SPEED_MODE) = 0;
        HWREGBITW(&g_ulFlags, FLAG_POSITION_MODE) = 0;
    }
    else
    {
        //
        // Disable current control mode.
        //
        HWREGBITW(&g_ulFlags, FLAG_CURRENT_MODE) = 0;
    }
}

//*****************************************************************************
//
// This function is used to set the target current when in current control
// mode.
//
//*****************************************************************************
void
ControllerCurrentSet(long lCurrent)
{
    //
    // Save the target current.
    //
    g_lCurrentTarget = lCurrent;
}

//*****************************************************************************
//
// This function is used to get the target current when in current control
// mode.
//
//*****************************************************************************
long
ControllerCurrentTargetGet(void)
{
    //
    // Return the target current.
    //
    return(g_lCurrentTarget);
}

//*****************************************************************************
//
// This function sets the P gain of the PID controller for current control
// mode.
//
//*****************************************************************************
void
ControllerCurrentPGainSet(long lPGain)
{
    //
    // Set the P gain on the current PID controller.
    //
    PIDGainPSet(&g_sCurrentPID, lPGain);
}

//*****************************************************************************
//
// This function gets the P gain of the PID controller for current control
// mode.
//
//*****************************************************************************
long
ControllerCurrentPGainGet(void)
{
    //
    // Return the P gain on the current PID controller.
    //
    return(g_sCurrentPID.lPGain);
}

//*****************************************************************************
//
// This function sets the I gain of the PID controller for current control
// mode.
//
//*****************************************************************************
void
ControllerCurrentIGainSet(long lIGain)
{
    long lTemp;

    //
    // Set the I gain and readjust the integrator min/max on the current PID
    // controller.
    //
    lTemp = MathDiv16x16(32767 * 256, lIGain);
    if(lTemp < 0)
    {
        lTemp = 0 - lTemp;
    }
    PIDGainISet(&g_sCurrentPID, lIGain, lTemp, 0 - lTemp);
}

//*****************************************************************************
//
// This function gets the I gain of the PID controller for current control
// mode.
//
//*****************************************************************************
long
ControllerCurrentIGainGet(void)
{
    //
    // Return the I gain on the current PID controller.
    //
    return(g_sCurrentPID.lIGain);
}

//*****************************************************************************
//
// This function sets the D gain of the PID controller for current control
// mode.
//
//*****************************************************************************
void
ControllerCurrentDGainSet(long lDGain)
{
    //
    // Set the D gain on the current PID controller.
    //
    PIDGainDSet(&g_sCurrentPID, lDGain);
}

//*****************************************************************************
//
// This function gets the D gain of the PID controller for current control
// mode.
//
//*****************************************************************************
long
ControllerCurrentDGainGet(void)
{
    //
    // Return the D gain on the current PID controller.
    //
    return(g_sCurrentPID.lDGain);
}

//*****************************************************************************
//
// This function handles the periodic processing for voltage control mode.
//
//*****************************************************************************
static void
ControllerVoltageMode(void)
{
    //
    // There is only something to be done if the target voltage does not match
    // the current output voltage.
    //
    if(g_lVoltage != g_lVoltageTarget)
    {
        //
        // If output voltage ramping is disabled or the target voltage is
        // neutral, then go directly to the target voltage.
        //
        if((g_ulVoltageRate == 0) ||
           ((g_lVoltageTarget >= (0 - NEUTRAL_PLATEAU)) &&
            (g_lVoltageTarget <= NEUTRAL_PLATEAU)))
        {
            g_lVoltage = g_lVoltageTarget;
        }

        //
        // See if the current output voltage is less than the target voltage.
        //
        else if(g_lVoltage < g_lVoltageTarget)
        {
            //
            // Increment the output voltage by the ramp rate and do not allow
            // it to exceed the target voltage.
            //
            g_lVoltage += g_ulVoltageRate;
            if(g_lVoltage > g_lVoltageTarget)
            {
                g_lVoltage = g_lVoltageTarget;
            }
        }
        else
        {
            //
            // Decrement the output voltage by the ramp rate and do not allow
            // it to exceed the target voltage.
            //
            g_lVoltage -= g_ulVoltageRate;
            if(g_lVoltage < g_lVoltageTarget)
            {
                g_lVoltage = g_lVoltageTarget;
            }
        }

        //
        // Send the new output voltage to the H-bridge.
        //
        HBridgeVoltageSet(g_lVoltage);
    }
}

//*****************************************************************************
//
// This function handles the periodic processing for voltage compensation
// control mode.
//
//*****************************************************************************
static void
ControllerVCompMode(void)
{
    long lVBus, lVolts, lComp;

    //
    // See if the desired output voltage needs to be adjusted towards the
    // target voltage.
    //
    if(g_lVCompVoltage != g_lVCompTarget)
    {
        //
        // If target voltage ramping is disabled then go directly to the target
        // voltage.
        //
        if(g_ulVCompInRate == 0)
        {
            g_lVCompVoltage = g_lVCompTarget;
        }

        //
        // See if the desired output voltage is less than the target output
        // voltage.
        //
        else if(g_lVCompVoltage < g_lVCompTarget)
        {
            //
            // Increment the desired output voltage by the ramp rate and do not
            // allow it to exceed the target voltage.
            //
            g_lVCompVoltage += g_ulVCompInRate;
            if(g_lVCompVoltage > g_lVCompTarget)
            {
                g_lVCompVoltage = g_lVCompTarget;
            }
        }
        else
        {
            //
            // Decrement the desired output voltage by the ramp rate and do not
            // allow it to exceed the target voltage.
            //
            g_lVCompVoltage -= g_ulVCompInRate;
            if(g_lVCompVoltage < g_lVCompTarget)
            {
                g_lVCompVoltage = g_lVCompTarget;
            }
        }
    }

    //
    // Get the current bus voltage.
    //
    lVBus = ADCVBusGet();

    //
    // Compute the desired output PWM width in order to produce the desired
    // output voltage, and limit it to the available range.
    //
    lVolts = (g_lVCompVoltage * 32768) / lVBus;
    if(lVolts < -32768)
    {
        lVolts = -32768;
    }
    if(lVolts > 32767)
    {
        lVolts = 32767;
    }

    //
    // See if the output voltage needs to be adjusted towards the desired
    // output voltage.
    //
    if(g_lVoltage != lVolts)
    {
        //
        // Convert the compensation ramp rate to output PWM width.
        //
        lComp = (g_ulVCompCompRate * 32768) / lVBus;

        //
        // If output voltage ramping is disabled then go directly to the
        // desired voltage.
        //
        if(lComp == 0)
        {
            g_lVoltage = lVolts;
        }

        //
        // See if the current output voltage is less than the desired voltage.
        //
        else if(g_lVoltage < lVolts)
        {
            //
            // Increment the output voltage by the ramp rate and do not allow
            // it to exceed the target voltage.
            //
            g_lVoltage += lComp;
            if(g_lVoltage > lVolts)
            {
                g_lVoltage = lVolts;
            }
        }
        else
        {
            //
            // Decrement the output voltage by the ramp rate and do not allow
            // it to exceed the target voltage.
            //
            g_lVoltage -= lComp;
            if(g_lVoltage < lVolts)
            {
                g_lVoltage = lVolts;
            }
        }

        //
        // Send the new output voltage to the H-bridge.
        //
        HBridgeVoltageSet(g_lVoltage);
    }
}

//*****************************************************************************
//
// This function handles the periodic processing for current control mode.
//
//*****************************************************************************
static void
ControllerCurrentMode(void)
{
    long lTemp;

    //
    // If the target current is zero, then the output voltage is zero also.
    //
    if(g_lCurrentTarget == 0)
    {
        g_lVoltage = 0;
    }
    else
    {
        //
        // Get the winding current.
        //
        lTemp = ADCCurrentGet();
        if(g_lCurrentTarget < 0)
        {
            lTemp = 0 - lTemp;
        }

        //
        // Compute the error between the target current and the winding
        // current.
        //
        lTemp = g_lCurrentTarget - lTemp;

        //
        // Run the PID controller, with the output being the output voltage
        // that should be driven to the motor.
        //
        lTemp = PIDUpdate(&g_sCurrentPID, lTemp * 256) / 256;

        //
        // Limit the output voltage to the valid values.
        //
        if(lTemp < -32768)
        {
            lTemp = -32768;
        }
        if(lTemp > 32767)
        {
            lTemp = 32767;
        }

        //
        // Do not drive the motor if the desired output voltage is the opposite
        // polarity of the target current (in other words, do not drive the
        // motor forward if the target current is reverse), or if the desired
        // output voltage is within the neutral plateau.
        //
        if(((g_lCurrentTarget < 0) && (lTemp > 0)) ||
           ((g_lCurrentTarget > 0) && (lTemp < 0)) ||
           ((lTemp >= (0 - NEUTRAL_PLATEAU)) && (lTemp <= NEUTRAL_PLATEAU)))
        {
            lTemp = 0;
        }

        //
        // Set the new output voltage.
        //
        g_lVoltage = lTemp;
    }

    //
    // Send the new output voltage to the H-bridge.
    //
    HBridgeVoltageSet(g_lVoltage);
}

//*****************************************************************************
//
// This function handles the periodic processing for speed control mode.
//
//*****************************************************************************
static void
ControllerSpeedMode(void)
{
    long lTemp;

    //
    // If the target speed is zero, then the output voltage is zero also.
    //
    if(g_lSpeedTarget == 0)
    {
        g_lVoltage = 0;
    }
    else
    {
        //
        // Get the motor speed.
        //
        if((HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_ENCODER) == 1) ||
           (HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_INV_ENC) == 1))
        {
            lTemp = EncoderVelocityGet(0);
            if(g_lSpeedTarget < 0)
            {
                lTemp = 0 - lTemp;
            }
        }
        else if(HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_QUAD_ENC) == 1)
        {
            lTemp = EncoderVelocityGet(1);
        }
        else
        {
            lTemp = 0;
        }

        //
        // Compute the error between the target speed and the motor speed.
        //
        lTemp = g_lSpeedTarget - lTemp;

        //
        // Run the PID controller, with the output being the output voltage
        // that should be driven to the motor.
        //
        lTemp = PIDUpdate(&g_sSpeedPID, lTemp) / 256;

        //
        // Limit the output voltage to the valid values.
        //
        if(lTemp < -32768)
        {
            lTemp = -32768;
        }
        if(lTemp > 32767)
        {
            lTemp = 32767;
        }

        //
        // See if a single-channel encoder is being used.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_ENCODER) == 1)
        {
            //
            // Do not drive the motor if the desired output voltage is the
            // opposite polarity of the target speed (in other words, do not
            // drive the motor forward if the target speed is reverse).
            //
            if(((g_lSpeedTarget < 0) && (lTemp > 0)) ||
               ((g_lSpeedTarget > 0) && (lTemp < 0)))
            {
                lTemp = 0;
            }
        }

        //
        // See if an inverted single-channel encoder is being used.
        //
        else if(HWREGBITW(&g_ulFlags, FLAG_SPEED_SRC_INV_ENC) == 1)
        {
            //
            // Do not drive the motor if the desired output voltage is the same
            // polarity as the target speed (in other words, do not drive the
            // motor forward if the target speed is forward).
            //
            if(((g_lSpeedTarget > 0) && (lTemp > 0)) ||
               ((g_lSpeedTarget < 0) && (lTemp < 0)))
            {
                lTemp = 0;
            }
        }

        //
        // Set the new output voltage.
        //
        g_lVoltage = lTemp;
    }

    //
    // Send the new output voltage to the H-bridge.
    //
    HBridgeVoltageSet(g_lVoltage);
}

//*****************************************************************************
//
// This function handles the periodic processing for position control mode.
//
//*****************************************************************************
static void
ControllerPositionMode(void)
{
    long lTemp;

    //
    // Get the motor position.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_POS_SRC_ENCODER) == 1)
    {
        lTemp = EncoderPositionGet();
    }
    else if(HWREGBITW(&g_ulFlags, FLAG_POS_SRC_POT) == 1)
    {
        lTemp = ADCPotPosGet();
    }
    else
    {
        lTemp = 0;
    }

    //
    // Compute the error between the target position and the motor position.
    //
    lTemp = g_lPositionTarget - lTemp;

    //
    // Run the PID controller, with the output being the output voltage that
    // should be driven to the motor.
    //
    lTemp = PIDUpdate(&g_sPositionPID, lTemp) / 256;

    //
    // Limit the output voltage to the valid values.
    //
    if(lTemp < -32768)
    {
        lTemp = -32768;
    }
    if(lTemp > 32767)
    {
        lTemp = 32767;
    }

    //
    // Do not drive the motor if the desired output voltage is within the
    // neutral plateau.
    //
    if((lTemp >= (0 - NEUTRAL_PLATEAU)) && (lTemp <= NEUTRAL_PLATEAU))
    {
        lTemp = 0;
    }

    //
    // Set the new output voltage.
    //
    g_lVoltage = lTemp;

    //
    // Send the new output voltage to the H-bridge.
    //
    HBridgeVoltageSet(g_lVoltage);
}

//*****************************************************************************
//
// This function returns the current power status.
//
//*****************************************************************************
unsigned long
ControllerPowerStatus(void)
{
    //
    // Return the state of the power status flag.
    //
    return(HWREGBITW(&g_ulFlags, FLAG_POWER_STATUS));
}

//*****************************************************************************
//
// This function clears the power status.
//
//*****************************************************************************
void
ControllerPowerStatusClear(void)
{
    //
    // Clear the power status flag.
    //
    HWREGBITW(&g_ulFlags, FLAG_POWER_STATUS) = 0;
}

//*****************************************************************************
//
// This function set the halted flag.
//
//*****************************************************************************
void
ControllerHaltSet(void)
{
    //
    // Set the halted flag.
    //
    HWREGBITW(&g_ulFlags, FLAG_HALTED) = 1;
}

//*****************************************************************************
//
// This function clears the halted flag.
//
//*****************************************************************************
void
ControllerHaltClear(void)
{
    //
    // Clear the halted flag.
    //
    HWREGBITW(&g_ulFlags, FLAG_HALTED) = 0;
}

//*****************************************************************************
//
// This function gets the value of the halted flag.
//
//*****************************************************************************
unsigned long
ControllerHalted(void)
{
    //
    // Return the state of the halted flag.
    //
    return(HWREGBITW(&g_ulFlags, FLAG_HALTED));
}

//*****************************************************************************
//
// This function initializes the controller.
//
//*****************************************************************************
void
ControllerInit(void)
{
    //
    // Initialize the PID controllers.
    //
    PIDInitialize(&g_sCurrentPID, 0, 0, 0, 0, 0);
    PIDInitialize(&g_sSpeedPID, 0, 0, 0, 0, 0);
    PIDInitialize(&g_sPositionPID, 0, 0, 0, 0, 0);

    //
    // Set the power status flag to indicate that the device was just powered
    // up.
    //
    HWREGBITW(&g_ulFlags, FLAG_POWER_STATUS) = 1;
}

//*****************************************************************************
//
// This function is called to handle the timer interrupt from the PWM module,
// which generates the 1 ms system timing.
//
//*****************************************************************************
void
ControllerIntHandler(void)
{
    //
    // Clear the interrupt source.
    //
    ROM_PWMGenIntClear(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_ZERO);

    //
    // Always check the buttons.
    //
    ButtonTick();

    //
    // Determine the state of the controller.
    //
    switch(g_ulControllerState)
    {
        //
        // The controller is waiting for a valid link.
        //
        case STATE_WAIT_FOR_LINK:
        {
            //
            // Do not acknowledge a valid link until the ADC calibration is
            // complete.
            //
            if(ADCCalibrationDone() == 0)
            {
                break;
            }

            //
            // If a fault has been signaled, then turn off the fault flag and
            // reset the fault counter.
            //
            if(HWREGBITW(&g_ulFlags, FLAG_FAULT) == 1)
            {
                HWREGBITW(&g_ulFlags, FLAG_FAULT) = 0;
                g_ulFaultCounter = g_ulFaultTime;
            }

            //
            // See if the fault counter is active.
            //
            if(g_ulFaultCounter)
            {
                //
                // Decrement the fault counter.
                //
                g_ulFaultCounter--;

                //
                // Clear the fault(s) if the fault counter has reached zero.
                //
                if(g_ulFaultCounter == 0)
                {
                    g_ulFaultFlags = 0;
                }
            }

            //
            // See if a valid link has been detected.
            //
            if(HWREGBITW(&g_ulFlags, FLAG_HAVE_LINK) == 1)
            {
                //
                // Indicate that the link has not been lost.
                //
                HWREGBITW(&g_ulFlags, FLAG_LOST_LINK) = 0;

                //
                // If the fault counter is active, then go to the fault state.
                // Otherwise, go to the run state.
                //
                if(g_ulFaultCounter != 0)
                {
                    g_ulControllerState = STATE_FAULT;
                }
                else
                {
                    g_ulControllerState = STATE_RUN;
                }
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The controller is actively running.
        //
        case STATE_RUN:
        {
            //
            // Process any commands in the queue.
            //
            CommandQueueProcess(0);

            //
            // Don't allow the voltage to change if the controller is
            // halted.
            //
            if(ControllerHalted() == 0)
            {
                //
                // Call the appropriate control method based on the control mode
                // that is currently enabled.
                //
                if(HWREGBITW(&g_ulFlags, FLAG_VCOMP_MODE))
                {
                    ControllerVCompMode();
                }
                else if(HWREGBITW(&g_ulFlags, FLAG_CURRENT_MODE))
                {
                    ControllerCurrentMode();
                }
                else if(HWREGBITW(&g_ulFlags, FLAG_SPEED_MODE))
                {
                    ControllerSpeedMode();
                }
                else if(HWREGBITW(&g_ulFlags, FLAG_POSITION_MODE))
                {
                    ControllerPositionMode();
                }
                else
                {
                    ControllerVoltageMode();
                }
            }

            //
            // See if the control link has been lost.
            //
            if(HWREGBITW(&g_ulFlags, FLAG_LOST_LINK) == 1)
            {
                //
                // Indicate that the link is not valid.
                //
                HWREGBITW(&g_ulFlags, FLAG_HAVE_LINK) = 0;

                //
                // Abandon any activity that is currently active.
                //
                ControllerForceNeutral();
                ServoIFCalibrationAbort();

                //
                // Signal a COMM fault now that the link has been lost.
                //
                ControllerFaultSignal(LM_FAULT_COMM);

                //
                // Wait for the control link to be restored.
                //
                g_ulControllerState = STATE_WAIT_FOR_LINK;
            }

            //
            // See if a fault has occurred.
            //
            if(HWREGBITW(&g_ulFlags, FLAG_FAULT) == 1)
            {
                //
                // Clear the fault flag.
                //
                HWREGBITW(&g_ulFlags, FLAG_FAULT) = 0;

                //
                // Abandon any activity that is currently active.
                //
                ControllerForceNeutral();
                ServoIFCalibrationAbort();

                //
                // Reset the fault counter and go to the fault state.
                //
                g_ulFaultCounter = g_ulFaultTime;
                g_ulControllerState = STATE_FAULT;
            }

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The controller is waiting for a fault to clear.
        //
        case STATE_FAULT:
        {
            //
            // If a fault has been signaled, then turn off the fault flag and
            // reset the fault counter.
            //
            if(HWREGBITW(&g_ulFlags, FLAG_FAULT) == 1)
            {
                HWREGBITW(&g_ulFlags, FLAG_FAULT) = 0;
                g_ulFaultCounter = g_ulFaultTime;
            }

            //
            // Process any commands in the queue, but ignore any commands that
            // would result in the motor being driven.
            //
            CommandQueueProcess(1);

            //
            // Decrement the fault counter.
            //
            g_ulFaultCounter--;

            //
            // See if the fault counter has reached zero.
            //
            if(g_ulFaultCounter == 0)
            {
                //
                // Clear the fault(s).
                //
                g_ulFaultFlags = 0;

                //
                // Reset the gate driver to clear any latched fault conditions.
                //
                HBridgeGateDriverReset();

                //
                // If there is not a control link, then go to the wait for link
                // state.  Otherwise, go to the run state.
                //
                if(HWREGBITW(&g_ulFlags, FLAG_LOST_LINK) == 1)
                {
                    HWREGBITW(&g_ulFlags, FLAG_HAVE_LINK) = 0;
                    g_ulControllerState = STATE_WAIT_FOR_LINK;
                }
                else
                {
                    g_ulControllerState = STATE_RUN;
                }
            }

            //
            // This state has been handled.
            //
            break;
        }
    }

    //
    // Update the limit switches.
    //
    LimitTick();

    //
    // Update the encoder.
    //
    EncoderTick();

    //
    // Update the fan.
    //
    FanTick();

    //
    // Update the LED status
    //
    LEDTick();

    //
    // Call the message tick function.
    //
    MessageTick();
}
