//*****************************************************************************
//
// adc_ctrl.c - Supports the operation of the ADC.
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

#include "inc/hw_adc.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "shared/can_proto.h"
#include "adc_ctrl.h"
#include "constants.h"
#include "controller.h"
#include "hbridge.h"
#include "math.h"
#include "pins.h"

//*****************************************************************************
//
// This function converts a current value into the corresponding ADC reading.
//
//*****************************************************************************
#define CURRENT_TO_ADC(ulC)     ((unsigned long)((((ulC) * 1944) - 1072) /    \
                                                 65536))

//*****************************************************************************
//
// This function converts an ADC reading value into current in Amperes, as an
// 8.8 fixed-point value.
//
//*****************************************************************************
#define ADC_TO_CURRENT(ulA)     ((unsigned long)((((ulA) * 2202991) +         \
                                                  10081885) / 65536))

//*****************************************************************************
//
// The maximum value of the current counter.  If the current counter reaches
// this value, an over-current fault will be triggered.
//
//*****************************************************************************
#define CURRENT_COUNTER_MAX     ((CURRENT_TO_ADC(CURRENT_SHUTOFF_LEVEL) -     \
                                  CURRENT_TO_ADC(CURRENT_MINIMUM_LEVEL)) *    \
                                 (CURRENT_TO_ADC(CURRENT_SHUTOFF_LEVEL) -     \
                                  CURRENT_TO_ADC(CURRENT_MINIMUM_LEVEL)) *    \
                                 CURRENT_SHUTOFF_TIME)

//*****************************************************************************
//
// Converts a bus voltage to an ADC reading.
//
//*****************************************************************************
#define VBUS_TO_ADC(v)          (((v) * 1024) / (36 * 256))

//*****************************************************************************
//
// Converts an ADC reading to a bus voltage.
//
//*****************************************************************************
#define ADC_TO_VBUS(a)          (((a) * 36 * 256) / 1024)

//*****************************************************************************
//
// Converts a temperature to an ADC reading.
//
//*****************************************************************************
#define TEMPERATURE_TO_ADC(t)   (((131 * 256) - (t)) / 49)

//*****************************************************************************
//
// Converts an ADC reading to a temperature in (C).
//
//*****************************************************************************
#define ADC_TO_TEMPERATURE(a)   ((131 * 256) - (a * 49))

//*****************************************************************************
//
// The location of each measurement element within the ADC data buffer.
//
//*****************************************************************************
#define WINDING_CURRENT         0
#define VBUS                    1
#define ANALOG_IN               2
#define TEMP_SENSOR             3

//*****************************************************************************
//
// The buffer where the ADC data is stored.
//
//*****************************************************************************
static unsigned short g_pusADCData[4];

//*****************************************************************************
//
// A counter that tracks the current load on the board.
//
//*****************************************************************************
static unsigned long g_ulCurrentCounter = 0;

//*****************************************************************************
//
// The number of turns in the potentiometer.
//
//*****************************************************************************
static long g_lPotTurns = 1;

//*****************************************************************************
//
// A set of buckets used for accumulating winding current readings.
//
//*****************************************************************************
static unsigned short g_pusBuckets[8] = { 0 };

//*****************************************************************************
//
// The bucket that is currently being used to accumulate winding current
// readings.
//
//*****************************************************************************
static unsigned long g_ulBucket = 0;

//*****************************************************************************
//
// The averaged winding current.
//
//*****************************************************************************
static unsigned short g_usCurrent = 0;

//*****************************************************************************
//
// The averaged winding current when the motor is not being driven.  This is
// used to cancel out any zero-current error that may be present (due to
// component inaccuracies).
//
//*****************************************************************************
static unsigned short g_usCurrentZero;

//*****************************************************************************
//
// The count of consecutive samples where the bus voltage is below the shutdown
// threshold.
//
//*****************************************************************************
static unsigned long g_ulVBusTimeout = 0;

//*****************************************************************************
//
// The previous state of the temperature fault, which adjusts the temperature
// compare value in order to provide hysteresis.
//
//*****************************************************************************
static unsigned long g_ulTemperatureFault = 0;

//*****************************************************************************
//
// Initializes the ADC.
//
//*****************************************************************************
void
ADCInit(void)
{
    //
    // Set the ADC speed to 1 Msps.
    //
    ROM_SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);

    //
    // Configure the GPIOs used with the analog inputs.
    //
    GPIOPinTypeADC(ADC_POSITION_PORT, ADC_POSITION_PIN);
    GPIOPinTypeADC(ADC_VBUS_PORT, ADC_VBUS_PIN);
    GPIOPinTypeADC(ADC_CURRENT_PORT, ADC_CURRENT_PIN);

    //
    // Configure the ADC sample sequence that is triggered by PWM
    // generator 2
    //
    ROM_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PWM0, 0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CURRENT_CH);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_VBUS_CH);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_POSITION_CH);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 3,
                                 (ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END));
    ROM_ADCSequenceEnable(ADC0_BASE, 0);

    //
    // Enable interrupts
    //
    ROM_ADCIntEnable(ADC0_BASE, 0);
    ROM_IntEnable(INT_ADC0SS0);

    //
    // Set the zero current to indicate that the initial sampling has just
    // begun.
    //
    g_usCurrentZero = 0xffff;
}

//*****************************************************************************
//
// This function will set the number of turns in the potentiometer, which is
// used to translate voltage readings from the ADC into the potentiometer
// position.
//
// If the number of turns is positive, increasing ADC readings will result in
// increasing position values.  If the number of turns is negative, increasing
// ADC readings will result in decreasing position values.
//
//*****************************************************************************
void
ADCPotTurnsSet(long lTurns)
{
    //
    // Save the number of turns in the potentiometer.
    //
    g_lPotTurns = lTurns;
}

//*****************************************************************************
//
// This function will get the number of turns in the potentiometer.
//
//*****************************************************************************
long
ADCPotTurnsGet(void)
{
    //
    // Return the number of turns in the potentiometer.
    //
    return(g_lPotTurns);
}

//*****************************************************************************
//
// This function computes the current position of the potentiometer based on
// the current ADC reading.  The value will be returned as a signed 16.16
// fixed-point value that represents the number of revolutions.
//
//*****************************************************************************
long
ADCPotPosGet(void)
{
    //
    // Convert the ADC reading into the revolution position of the
    // potentiometer and return it.
    //
    return((g_pusADCData[ANALOG_IN] * g_lPotTurns * 65536) / 1023);
}

//*****************************************************************************
//
// This function returns the current bus voltage, specified as an unsigned 8.8
// fixed-point value that represents the voltage.
//
//*****************************************************************************
unsigned long
ADCVBusGet(void)
{
    //
    // Convert the ADC reading into a voltage and return it.
    //
    return(ADC_TO_VBUS(g_pusADCData[VBUS]));
}

//*****************************************************************************
//
// This function returns the motor winding current, specified as an unsigned
// 16.16 fixed-point value that represents the current in Amperes.
//
//*****************************************************************************
unsigned long
ADCCurrentGet(void)
{
    unsigned long ulRet;

    //
    // If the ADC reading is less than the zero value, then the current reading
    // is zero.
    //
    if(g_usCurrent < g_usCurrentZero)
    {
        ulRet = 0;
    }
    else
    {
        //
        // Convert the ADC reading into Amperes.
        //
        ulRet = ADC_TO_CURRENT(g_usCurrent - g_usCurrentZero);

        //
        // If the current is less than 1 Amp, or is negative, then set the
        // current reading to zero.
        //
        if((ulRet < 256) || (ulRet & 0x80000000))
        {
            ulRet = 0;
        }
    }

    //
    // Return the computed winding current.
    //
    return(ulRet);
}

//*****************************************************************************
//
// This function returns the current ambient temperature, specified as an
// unsigned 8.8 fixed-point value that represents the temperature in degrees
// Celcius.
//
//*****************************************************************************
unsigned long
ADCTemperatureGet(void)
{
    //
    // Convert the ADC reading into a temperature and return it.
    //
    return(ADC_TO_TEMPERATURE(g_pusADCData[TEMP_SENSOR]));
}

//*****************************************************************************
//
// This function determines if the ADC zero-current calibration has completed.
//
//*****************************************************************************
unsigned long
ADCCalibrationDone(void)
{
    //
    // If the zero-current value is valid, then calibration has completed.
    //
    if((g_usCurrentZero & 0xf000) == 0)
    {
        return(1);
    }

    //
    // If the calibration is starting and the bucket 0 has been filled (since
    // another bucket is currently being filled), then move to the calibration
    // waiting state.
    //
    if((g_usCurrentZero == 0xffff) && ((g_ulBucket >> 16) != 0))
    {
        g_usCurrentZero = 0xfffe;
    }

    //
    // If waiting for the calibration to complete and bucket 0 is being filled
    // (meaning all buckets have been filled), then save the current reading as
    // the zero current reading and indicate that calibration has completed.
    //
    if((g_usCurrentZero == 0xfffe) && ((g_ulBucket >> 16) == 0))
    {
        g_usCurrentZero = g_usCurrent;
        return(1);
    }

    //
    // Calibration is still in progress.
    //
    return(0);
}

//*****************************************************************************
//
// This function is called each time the ADC sample sequence completes.
//
//*****************************************************************************
void
ADCIntHandler(void)
{
    unsigned short pusADCData[8];
    unsigned long ulIdx;

    //
    // Clear the ADC interrupt.
    //
    ROM_ADCIntClear(ADC0_BASE, 0);

    //
    // If running on Rev A0 silicon, clear the PWM trigger interrupt sources.
    // This is a workaround to allow them to retrigger the ADC.  Since this
    // workaround is otherwise harmless, it is done unconditionally (to avoid
    // checking the silicon revision within this interrupt handler).
    //
    ROM_PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_BD);

    //
    // Call the H-bridge update function.
    //
    HBridgeTick();

    //
    // Drain the ADC of conversions.
    //
    if(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)
    {
        return;
    }
    pusADCData[0] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    if(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)
    {
        return;
    }
    pusADCData[1] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    if(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)
    {
        return;
    }
    pusADCData[2] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    if(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)
    {
        return;
    }
    pusADCData[3] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    if(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
    {
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }
        return;
    }
    g_pusADCData[0] = pusADCData[0];
    g_pusADCData[1] = pusADCData[1];
    g_pusADCData[2] = pusADCData[2];
    g_pusADCData[3] = pusADCData[3];

    //
    // Put this winding current reading into the current bucket.
    //
    g_pusBuckets[g_ulBucket >> 16] += g_pusADCData[WINDING_CURRENT];
    g_ulBucket++;

    //
    // See if this bucket is full.
    //
    if((g_ulBucket & 0xffff) == 16)
    {
        //
        // Compute the new averaged winding current reading.
        //
        g_usCurrent = ((g_pusBuckets[0] + g_pusBuckets[1] + g_pusBuckets[2] +
                        g_pusBuckets[3] + g_pusBuckets[4] + g_pusBuckets[5] +
                        g_pusBuckets[6] + g_pusBuckets[7]) / (8 * 16));

        //
        // Advance to the next bucket.
        //
        g_ulBucket = (g_ulBucket + 0x10000) & 0x00070000;
        g_pusBuckets[g_ulBucket >> 16] = 0;
    }

    //
    // Track the current usage over time.  The amount of time spent above the
    // nominal level and the excess over the nominal level is added to a
    // counter that will cause an over current error when the counter gets too
    // high.  The amount of time spent below the nominal level is subtracted
    // from the counter in the same manner.  First, see if the current is above
    // or below the nominal level.
    //
    ulIdx = g_pusADCData[WINDING_CURRENT];
    if(ulIdx > CURRENT_TO_ADC(CURRENT_NOMINAL_LEVEL))
    {
        //
        // Compute the square of the different between the current reading and
        // the minimum level.
        //
        ulIdx -= CURRENT_TO_ADC(CURRENT_MINIMUM_LEVEL);
        ulIdx *= ulIdx;

        //
        // Add this value to the winding current counter and return a winding
        // current error if it has exceeded the maximum value.
        //
        g_ulCurrentCounter += ulIdx;
        if(g_ulCurrentCounter > CURRENT_COUNTER_MAX)
        {
            ControllerFaultSignal(LM_FAULT_CURRENT);
        }
    }
    else
    {
        //
        // Compute the square of the difference between the current reading and
        // the nominal level.
        //
        ulIdx = CURRENT_TO_ADC(CURRENT_NOMINAL_LEVEL) - ulIdx;
        ulIdx *= ulIdx;

        //
        // Subtract this value from the winding current counter, saturating at
        // zero.
        //
        if(ulIdx > g_ulCurrentCounter)
        {
            g_ulCurrentCounter = 0;
        }
        else
        {
            g_ulCurrentCounter -= ulIdx;
        }
    }

    //
    // If the temperature exceeds the set limit, shut the system down.
    //
    ulIdx = ADC_TO_TEMPERATURE(g_pusADCData[TEMP_SENSOR]);
    if(((g_ulTemperatureFault == 0) &&
        (ulIdx > (SHUTDOWN_TEMPERATURE + SHUTDOWN_TEMPERATURE_HYSTERESIS))) ||
       ((g_ulTemperatureFault == 1) &&
        (ulIdx > (SHUTDOWN_TEMPERATURE - SHUTDOWN_TEMPERATURE_HYSTERESIS))))
    {
        //
        // Shut the motor down, the temperature has exceeded the limit.
        //
        ControllerFaultSignal(LM_FAULT_TEMP);

        //
        // Indicate that the system is in temperature fault so that the
        // hysteresis can be properly applied to come out of temperature fault.
        //
        g_ulTemperatureFault = 1;
    }
    else
    {
        //
        // Indicate that the system is not in temperature fault so that the
        // hysteresis can be properly applied to go into temperature fault.
        //
        g_ulTemperatureFault = 0;
    }

    //
    // If the Vbus drops below the set limit, shut the system down.
    //
    if(g_pusADCData[VBUS] < VBUS_TO_ADC(SHUTDOWN_VOLTAGE))
    {
        //
        // Increment the count of consecutive samples where Vbus is below the
        // set limit.
        //
        g_ulVBusTimeout++;

        //
        // See there have been too many consecutive samples where Vbus is below
        // the set limit.
        //
        if(g_ulVBusTimeout >= SHUTDOWN_VOLTAGE_TIME)
        {
            //
            // Shut the motor down, the Vbus has dropped too much.
            //
            ControllerFaultSignal(LM_FAULT_VBUS);
        }
    }
    else
    {
        //
        // Reset the count of consecutive samples where Vbus is below the set
        // limit since it is presently above the set limit.
        //
        g_ulVBusTimeout = 0;
    }
}
