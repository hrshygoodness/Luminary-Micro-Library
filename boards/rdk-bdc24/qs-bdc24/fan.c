//*****************************************************************************
//
// fan.c - Driver for handling the fan.
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
#include "adc_ctrl.h"
#include "constants.h"
#include "controller.h"
#include "fan.h"
#include "pins.h"

//*****************************************************************************
//
// A flag that indicates if the fan is on or off.
//
//*****************************************************************************
static unsigned long g_ulFanState;

//*****************************************************************************
//
// The count of ticks before the fan can be turned off.  This is set to the
// timeout value whenever the motor is not in neutral and is decremented when
// the motor is in neutral.  If the count reaches zero, the fan can be turned
// off (if the temperature is low enough).
//
//*****************************************************************************
static unsigned long g_ulFanTime;

//*****************************************************************************
//
// This function initializes the fan interface, preparing it to manage the
// operation of the fan.
//
//*****************************************************************************
void
FanInit(void)
{
    //
    // Configure the GPIO as an output and enable the 8 mA drive.
    //
    ROM_GPIODirModeSet(FAN_PORT, FAN_PIN, GPIO_DIR_MODE_OUT);
    ROM_GPIOPadConfigSet(FAN_PORT, FAN_PIN, GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);

    //
    // Turn on the fan to test it.
    //
    g_ulFanState = 1;
    ROM_GPIOPinWrite(FAN_PORT, FAN_PIN, FAN_ON);

    //
    // Set the fan time to FAN_TEST_TIME.
    //
    g_ulFanTime = FAN_TEST_TIME;
}

//*****************************************************************************
//
// This function is called on a periodic basis to manage the fan.  The fan is
// turned on whenever the motor is not in neutral or if the ambient temperature
// gets too high.  It is turned off when the ambient temperature falls enough
// and the motor has been in neutral for an adequate period of time.  The time
// delay between going to neutral and turning off the fan serves two purposes:
// it continues to cool the FETs for a period of time after the motor stops,
// and it keeps the fan running continuously in the cases where the motor is
// simply passing through neutral for a brief period.
//
// This function should be called UPDATES_PER_SECOND times per second.
//
//*****************************************************************************
void
FanTick(void)
{
    unsigned long ulTemperature;
    long lVoltage;

    //
    // Get the current output voltage and ambient temperature.
    //
    lVoltage = ControllerVoltageGet();
    ulTemperature = ADCTemperatureGet();

    //
    // See if the fan is currently on.
    //
    if(g_ulFanState)
    {
        //
        // Decrement the fan time if has not reached zero.
        //
        if(g_ulFanTime)
        {
            g_ulFanTime--;
        }

        //
        // If the fan delay time has expired, the ambient temperature is low
        // enough, and the motor is in neutral, then turn off the fan.
        //
        if((g_ulFanTime == 0) &&
           (ulTemperature < (FAN_TEMPERATURE - FAN_HYSTERESIS)) &&
           (lVoltage == 0))
        {
            //
            // Turn off the fan.
            //
            g_ulFanState = 0;
            ROM_GPIOPinWrite(FAN_PORT, FAN_PIN, FAN_OFF);
        }

        //
        // If the motor is not in neutral, reset the fan time to the desired
        // delay.
        //
        if(lVoltage != 0)
        {
            g_ulFanTime = FAN_COOLING_TIME;
        }
    }
    else
    {
        //
        // See if the ambient temperature is too high or the motor is not in
        // neutral.
        //
        if((ulTemperature > (FAN_TEMPERATURE + FAN_HYSTERESIS)) ||
           (lVoltage != 0))
        {
            //
            // Turn on the fan.
            //
            g_ulFanState = 1;
            ROM_GPIOPinWrite(FAN_PORT, FAN_PIN, FAN_ON);

            //
            // See if the motor is in neutral.
            //
            if(lVoltage != 0)
            {
                //
                // The motor is not in neutral, so set the fan time to the
                // desired delay.
                //
                g_ulFanTime = FAN_COOLING_TIME;
            }
            else
            {
                //
                // The fan is being turned on because the ambient temperature
                // has gotten too high, so the fan time is set to zero so that
                // it will turn off immediately when the ambient temperature
                // drops enough.
                //
                g_ulFanTime = 0;
            }
        }
    }
}
