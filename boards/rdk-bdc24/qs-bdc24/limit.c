//*****************************************************************************
//
// limit.c - Supports the operation of the calibrate/user limit.
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
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "commands.h"
#include "constants.h"
#include "controller.h"
#include "limit.h"
#include "pins.h"

//*****************************************************************************
//
// The limit switch flags.
//
// The first enables the soft limit switches, the second indicates that the
// forward soft limit switch is a less than comparison instead of a greater
// than comparison, and the third indicates the same for the reverse soft limit
// switch.
//
// The last flag indicates that the Jaguar was put into Automatic Ramp mode on
// startup because it detected a cross-connected jumper on the hard limit
// switch inputs.
//
//*****************************************************************************
#define LIMIT_FLAG_POSITION_EN  8
#define LIMIT_FLAG_FWD_LT       9
#define LIMIT_FLAG_REV_LT       10
#define LIMIT_FLAG_AUTO_RAMP_EN 11
unsigned long g_ulLimitFlags;

//*****************************************************************************
//
// The position values for the forward and reverse soft limit switches.
//
//*****************************************************************************
static long g_lLimitForward;
static long g_lLimitReverse;

//*****************************************************************************
//
// This function initializes the limit switch inputs, preparing them to sense
// the state of the limit switches.
//
// The limit switches are normally closed switches that open when the switches
// are pressed.  When closed, the switches connect the input to ground.  When
// opened, the on-chip weak pull-up connects the input to Vdd.
//
// Before final initialization, this function can set the Voltage Mode ramp
// rate when it detects a cross-connected jumper across the limit swich inputs;
// essentially enabling an automatic voltage ramp without the use of CAN.
//
// This function should always be called sometime after ControllerInit()
// because it depends on the controller being initialized.
//
//*****************************************************************************
void
LimitInit(void)
{
    //
    // Configure the limit switch inputs with the weak pull-up enabled.
    //
#if LIMIT_FWD_PORT == LIMIT_REV_PORT
    ROM_GPIODirModeSet(LIMIT_FWD_PORT, LIMIT_FWD_PIN | LIMIT_REV_PIN,
                       GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(LIMIT_FWD_PORT, LIMIT_FWD_PIN | LIMIT_REV_PIN,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
#else
    ROM_GPIODirModeSet(LIMIT_FWD_PORT, LIMIT_FWD_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIODirModeSet(LIMIT_REV_PORT, LIMIT_REV_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(LIMIT_FWD_PORT, LIMIT_FWD_PIN, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOPadConfigSet(LIMIT_REV_PORT, LIMIT_REV_PIN, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
#endif

    //
    // Clear the limit flags.
    //
    g_ulLimitFlags = 0;

    //
    // Set the sticky limit flags since no limits have tripped on startup.
    //
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_FWD_OK) = 1;
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_REV_OK) = 1;
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_SFWD_OK) = 1;
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_SREV_OK) = 1;

    //
    // If both limit switch inputs are high, determine if there is a
    // cross-connected jumper.
    //
    if(ROM_GPIOPinRead(LIMIT_FWD_PORT, LIMIT_FWD_PIN) &&
       ROM_GPIOPinRead(LIMIT_REV_PORT, LIMIT_REV_PIN))
    {
        //
        // Configure forward limit pin as an open-drain output with a weak
        // pull-up.
        //
        ROM_GPIODirModeSet(LIMIT_FWD_PORT, LIMIT_FWD_PIN, GPIO_DIR_MODE_OUT);
        ROM_GPIOPadConfigSet(LIMIT_FWD_PORT, LIMIT_FWD_PIN, GPIO_STRENGTH_2MA,
                             GPIO_PIN_TYPE_OD_WPU);

        //
        // Set the forward limit pin low.
        //
        ROM_GPIOPinWrite(LIMIT_FWD_PORT, LIMIT_FWD_PIN, 0);

        //
        // Wait a bit.
        //
        SysCtlDelay(1000);

        //
        // If the reverse limit pin is now low too, a cross-connected jumper
        // was successfully detected.
        //
        if(ROM_GPIOPinRead(LIMIT_REV_PORT, LIMIT_REV_PIN) == 0)
        {
            //
            // Set the Automatic Ramp Flag.
            //
            HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_AUTO_RAMP_EN) = 1;

            //
            // Set Voltage Mode ramp to AUTO_RAMP_RATE.
            //
            ControllerVoltageRateSet(AUTO_RAMP_RATE);
        }

        //
        // Reconfigure the forward limit pin as an input with a weak pull-up.
        //
        ROM_GPIODirModeSet(LIMIT_FWD_PORT, LIMIT_FWD_PIN, GPIO_DIR_MODE_IN);
        ROM_GPIOPadConfigSet(LIMIT_FWD_PORT, LIMIT_FWD_PIN, GPIO_STRENGTH_2MA,
                             GPIO_PIN_TYPE_STD_WPU);
    }
}

//*****************************************************************************
//
// This function enables the soft limit switches, allowing them to affect the
// drive of the motor.
//
//*****************************************************************************
void
LimitPositionEnable(void)
{
    //
    // Enable the soft limit switches.
    //
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_POSITION_EN) = 1;
}

//*****************************************************************************
//
// This function disables the soft limit switches, preventing them from
// affecting the drive of the motor.
//
//*****************************************************************************
void
LimitPositionDisable(void)
{
    //
    // Disable the soft limit switches.
    //
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_POSITION_EN) = 0;
}

//*****************************************************************************
//
// This function determines if the soft limit switches are enabled.
//
//*****************************************************************************
unsigned long
LimitPositionActive(void)
{
    //
    // Determine if the soft limit switches are enabled.
    //
    return(HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_POSITION_EN));
}

//*****************************************************************************
//
// This function sets the position and comparison of the forward soft limit
// switch.
//
//*****************************************************************************
void
LimitPositionForwardSet(long lPosition, unsigned long ulLessThan)
{
    //
    // Save the position of the forward limit switch.
    //
    g_lLimitForward = lPosition;

    //
    // Save the comparison flag.
    //
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_FWD_LT) = ulLessThan ? 1 : 0;
}

//*****************************************************************************
//
// This function gets the position and comparison of the forward soft limit
// switch.
//
//*****************************************************************************
void
LimitPositionForwardGet(long *plPosition, unsigned long *pulLessThan)
{
    //
    // Return the position and comparison of the forward limit switch.
    //
    *plPosition = g_lLimitForward;
    *pulLessThan = HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_FWD_LT);
}

//*****************************************************************************
//
// This function sets the position and comparison of the reverse soft limit
// switch.
//
//*****************************************************************************
void
LimitPositionReverseSet(long lPosition, unsigned long ulLessThan)
{
    //
    // Save the positino of the reverse limit switch.
    //
    g_lLimitReverse = lPosition;

    //
    // Save the comparison flag.
    //
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_REV_LT) = ulLessThan ? 1 : 0;
}

//*****************************************************************************
//
// This function gets the position and comparison of the reverse soft limit
// switch.
//
//*****************************************************************************
void
LimitPositionReverseGet(long *plPosition, unsigned long *pulLessThan)
{
    //
    // Return the position and comparison of the reverse limit switch.
    //
    *plPosition = g_lLimitReverse;
    *pulLessThan = HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_REV_LT);
}

//*****************************************************************************
//
// When called periodically, this function samples the state of the hard limit
// switches and the state of the soft limit switches.  When one of the limit
// switches trips, a flag is set which is later used by the H-bridge driver
// to shut off the output drive.
//
//*****************************************************************************
void
LimitTick(void)
{
    unsigned long ulFlag, ulSoftFlag, ulLimitStatus;
    long lPosition;

    //
    // Get the current position.
    //
    if(HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_POSITION_EN) == 1)
    {
        lPosition = ControllerPositionGet();
    }
    else
    {
        lPosition = 0;
    }

    //
    // Set a local flag to one.  If a forward limit switch check fails, this
    // will be set to zero.  If it is still one after all forward limit switch
    // checks, then the forward limit switch is OK (meaning that it is
    // acceptable to drive the motor in the forward direction).
    //
    ulFlag = 1;
    ulSoftFlag = 1;

    //
    // If Automatic Ramp mode is enabled set the status to LIMIT_FWD_OK,
    // ignoring the hard limit. Otherwise, read the status of the forward
    // limit switch.
    //
    if(HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_AUTO_RAMP_EN))
    {
        ulLimitStatus = LIMIT_FWD_OK;
    }
    else
    {
        ulLimitStatus = ROM_GPIOPinRead(LIMIT_FWD_PORT, LIMIT_FWD_PIN);
    }

    //
    // See if the forward limit switch is engaged.
    //
    if(ulLimitStatus == LIMIT_FWD_OK)
    {
        //
        // See if the soft limit switches are enabled.
        //
        if(HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_POSITION_EN) == 1)
        {
            //
            // Compare the current position against the forward soft limit
            // switch position.
            //
            if(HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_FWD_LT) == 1)
            {
                if(lPosition > g_lLimitForward)
                {
                    ulSoftFlag = 0;
                }
            }
            else
            {
                if(lPosition < g_lLimitForward)
                {
                    ulSoftFlag = 0;
                }
            }
        }
    }
    else
    {
        //
        // The forward limit switch has opened, so it is not acceptable to run
        // the motor in the forward direction.
        //
        ulFlag = 0;
    }

    //
    // Set the forward limit switch flag based on the measured state of the
    // forward limit switches.
    //
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_FWD_OK) = (ulFlag & ulSoftFlag);
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_SFWD_OK) = ulSoftFlag;

    //
    // Clear the sticky flag if the limit switches have been tripped.
    //
    if((ulFlag == 0) || (ulSoftFlag == 0))
    {
        HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_FWD_OK) = 0;
    }

    //
    // Clear the sticky soft limit flag if the soft limits have been tripped.
    //
    if(ulSoftFlag == 0)
    {
        HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_SFWD_OK) = 0;
    }

    //
    // Set a local flag to one.  If a reverse limit switch check fails, this
    // will be set to zero.  If it is still one after all reverse limit switch
    // checks, then the reverse limit switch is OK (meaning that it is
    // acceptable to drive the motor in the reverse direction).
    //
    ulFlag = 1;
    ulSoftFlag = 1;

    //
    // If Automatic Ramp mode is enabled set the status to LIMIT_REV_OK,
    // ignoring the hard limit. Otherwise, read the status of the reverse
    // limit switch.
    //
    if(HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_AUTO_RAMP_EN))
    {
        ulLimitStatus = LIMIT_REV_OK;
    }
    else
    {
        ulLimitStatus = ROM_GPIOPinRead(LIMIT_REV_PORT, LIMIT_REV_PIN);
    }

    //
    // See if the reverse limit switch is engaged.
    //
    if(ulLimitStatus == LIMIT_REV_OK)
    {
        //
        // See if the soft limit switches are enabled.
        //
        if(HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_POSITION_EN) == 1)
        {
            //
            // Compare the current position against the reverse soft limit
            // switch position.
            //
            if(HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_REV_LT) == 1)
            {
                if(lPosition > g_lLimitReverse)
                {
                    ulSoftFlag = 0;
                }
            }
            else
            {
                if(lPosition < g_lLimitReverse)
                {
                    ulSoftFlag = 0;
                }
            }
        }
    }
    else
    {
        //
        // The reverse limit switch has opened, so it is not acceptable to run
        // the motor in the reverse direction.
        //
        ulFlag = 0;
    }

    //
    // Set the reverse limit switch flag based on the measured state of the
    // reverse limit switches.
    //
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_REV_OK) = (ulFlag & ulSoftFlag);
    HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_SREV_OK) = ulSoftFlag;

    //
    // Clear the sticky flag if the limit switches have been tripped.
    //
    if((ulFlag == 0) || (ulSoftFlag == 0))
    {
        HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_REV_OK) = 0;
    }

    //
    // Clear the sticky soft limit flag if the soft limits have been tripped.
    //
    if(ulSoftFlag == 0)
    {
        HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_SREV_OK) = 0;
    }
}
