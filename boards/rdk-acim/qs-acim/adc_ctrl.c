//*****************************************************************************
//
// adc_ctrl.c - ADC control routines.
//
// Copyright (c) 2007-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6852 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_adc.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "adc_ctrl.h"
#include "brake.h"
#include "main.h"
#include "pins.h"
#include "ui.h"

//*****************************************************************************
//
//! \page adc_ctrl_intro Introduction
//!
//! The ADC is used to monitor the motor current, DC bus voltage, and ambient
//! temperature of the microcontroller.  Each of these values is sampled every
//! PWM period based on a trigger from the PWM module, which allows the motor
//! current to be measured when the low side switch for each phase is turned
//! on.
//!
//! Each reading from the ADC is passed through a single-pole IIR low pass
//! filter.  This helps to reduce the effects of high frequency noise (such as
//! switching noise) on the sampled data.  A coefficient of 0.75 is used to
//! simplify the integer math (requiring only a multiplication by 3, an
//! addition, and a division by four).
//!
//! The measured current in each motor phase is passed through a peak detect
//! that resets every cycle of the output motor drive waveforms.  The peak
//! value is then divided by the square root of 2 (approximated by 1.4) in
//! order to obtain the RMS current of each phase of the motor.  The RMS
//! current of the motor is the average of the RMS current though each phase.
//!
//! The individual motor phase RMS currents, motor RMS current, DC bus voltage,
//! and ambient temperature are used outside this module.
//!
//! The code for handling the ADC is contained in <tt>adc_ctrl.c</tt>, with
//! <tt>adc_ctrl.h</tt> containing the definitions for the variables and
//! functions exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup adc_ctrl_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! An array containing the raw low pass filtered ADC readings.  This is
//! maintained in a raw form since it is required as an input to the next
//! iteration of the IIR low pass filter.
//
//*****************************************************************************
static unsigned short g_pusFilteredData[5];

//*****************************************************************************
//
//! An array containing the maximum phase currents seen during the last half
//! cycle of each phase.  This is used to perform a peak detect on the phase
//! currents.
//
//*****************************************************************************
static unsigned short g_pusPhaseMax[3];

//*****************************************************************************
//
//! The RMS current passing through the three phases of the motor, specified in
//! amperes as an unsigned 8.8 fixed point value.
//
//*****************************************************************************
volatile unsigned short g_pusPhaseCurrentRMS[3];

//*****************************************************************************
//
//! The total RMS current passing through the motor, specified in amperes as an
//! unsigned 8.8 fixed point value.
//
//*****************************************************************************
volatile unsigned short g_usMotorCurrent;

//*****************************************************************************
//
//! The DC bus voltage, specified in volts.
//
//*****************************************************************************
volatile unsigned short g_usBusVoltage;

//*****************************************************************************
//
//! The ambient case temperature of the microcontroller, specified in degrees
//! Celsius.
//
//*****************************************************************************
volatile short g_sAmbientTemp;

//*****************************************************************************
//
//! The angle of the motor drive on the previous ADC interrupt.
//
//*****************************************************************************
static unsigned long g_ulPrevAngle;

//*****************************************************************************
//
//! Handles the ADC sample sequence zero interrupt.
//!
//! This function is called when sample sequence zero asserts an interrupt.  It
//! handles clearing the interrupt and processing the new ADC data in the FIFO.
//!
//! \return None.
//
//*****************************************************************************
void
ADC0IntHandler(void)
{
    unsigned long ulIdx;
    unsigned short pusADCData[8];

    //
    // Clear the ADC interrupt.
    //
    ADCIntClear(ADC0_BASE, 0);

    //
    // Read the samples from the ADC FIFO.
    //
    ulIdx = 0;
    while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY) &&
          (ulIdx < 8))
    {
        //
        // Read the next sample.
        //
        pusADCData[ulIdx] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);

        //
        // Increment the count of samples read.
        //
        ulIdx++;
    }

    //
    // See if five samples were read.
    //
    if(ulIdx != 5)
    {
        //
        // Since there were not precisely five samples in the FIFO, it is not
        // known what analog signal is represented by each sample.  Therefore,
        // return without doing any processing on these samples.
        //
        return;
    }

    //
    // Filter the new samples.
    //
    for(ulIdx = 0; ulIdx < 5; ulIdx++)
    {
        //
        // Pass the new data sample through a single pole IIR low pass filter
        // with a coefficient of 0.75.
        //
        g_pusFilteredData[ulIdx] = (((g_pusFilteredData[ulIdx] * 3) +
                                     pusADCData[ulIdx]) / 4);
    }

    //
    // Convert the ADC DC bus reading to volts.  Each volt at the ADC input
    // corresponds to 150 volts of bus voltage.
    //
    g_usBusVoltage = ((unsigned long)g_pusFilteredData[3] * 450) / 1024;

    //
    // Convert the ADC junction temperature reading to ambient case temperature
    // in Celsius.
    //
    g_sAmbientTemp = (59960 - (g_pusFilteredData[4] * 100)) / 356;

    //
    // See if the motor drive is running.
    //
    if(!MainIsRunning())
    {
        //
        // Since the motor drive is not running, there is no current through
        // the motor.
        //
        g_pusPhaseCurrentRMS[0] = 0;
        g_pusPhaseCurrentRMS[1] = 0;
        g_pusPhaseCurrentRMS[2] = 0;
        g_usMotorCurrent = 0;

        //
        // There is nothing further to be done since the motor is not running.
        //
        return;
    }

    //
    // See if the drive angle just crossed zero in either direction.
    //
    if(((g_ulAngle > 0xf0000000) && (g_ulPrevAngle < 0x10000000)) ||
       ((g_ulAngle < 0x10000000) && (g_ulPrevAngle > 0xf0000000)))
    {
        //
        // Loop through the three phases of the motor drive.
        //
        for(ulIdx = 0; ulIdx < 3; ulIdx++)
        {
            //
            // Convert the maximum reading detected during the last cycle into
            // amperes.  The phase current is measured as the voltage dropped
            // across a 0.04 Ohm resistor, so current is 25 times the voltage.
            // This is then passed through an op amp that multiplies the value
            // by 11.  The resulting phase current is put into a 8.8 fixed
            // point representation and must therefore be multiplied by 256.
            // This is the peak current, which is then divided by 1.4 to get
            // the RMS current.  Since the ADC reading is 0 to 1023 for
            // voltages between 0 V and 3 V, the final equation is:
            //
            //     A = R * (25 / 11) * (3 / 1024) * (10 / 14) * 256
            //
            // Reducing the constants results in R * 375 / 308.
            //
            g_pusPhaseCurrentRMS[ulIdx] = (((long)g_pusPhaseMax[ulIdx] * 375) /
                                           308);

            //
            // Reset the maximum phase current seen to zero to prepare for the
            // next cycle.
            //
            g_pusPhaseMax[ulIdx] = 0;
        }

        //
        // See if this is a single phase or three phase motor.
        //
        if(HWREGBITH(&(g_sParameters.usFlags), FLAG_MOTOR_TYPE_BIT) ==
           FLAG_MOTOR_TYPE_1PHASE)
        {
            //
            // Average the RMS current of the two phases to get the RMS motor
            // current.
            //
            pusADCData[0] = g_pusPhaseCurrentRMS[0];
            pusADCData[1] = g_pusPhaseCurrentRMS[1];
            g_usMotorCurrent = ((pusADCData[0] + pusADCData[1]) / 2);
        }
        else
        {
            //
            // Average the RMS current of the three phases to get the RMS motor
            // current.
            //
            pusADCData[0] = g_pusPhaseCurrentRMS[0];
            pusADCData[1] = g_pusPhaseCurrentRMS[1];
            pusADCData[2] = g_pusPhaseCurrentRMS[2];
            g_usMotorCurrent = ((pusADCData[0] + pusADCData[1] +
                                 pusADCData[2]) / 3);
        }
    }

    //
    // Loop through the three phases of the motor drive.
    //
    for(ulIdx = 0; ulIdx < 3; ulIdx++)
    {
        //
        // See if this ADC reading is larger than any previous ADC reading.
        //
        if(g_pusFilteredData[ulIdx] > g_pusPhaseMax[ulIdx])
        {
            //
            // Save this ADC reading as the maximum.
            //
            g_pusPhaseMax[ulIdx] = g_pusFilteredData[ulIdx];
        }
    }

    //
    // Save the current motor drive angle for the next set of samples.
    //
    g_ulPrevAngle = g_ulAngle;
}

//*****************************************************************************
//
//! Initializes the ADC control routines.
//!
//! This function initializes the ADC module and the control routines,
//! preparing them to monitor currents and voltages on the motor drive.
//!
//! \return None.
//
//*****************************************************************************
void
ADCInit(void)
{
    //
    // Set the speed of the ADC to 1 million samples per second.
    //
    SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);

    //
    // Configure sample sequence zero to capture all three motor phase
    // currents, the DC bus voltage, and the internal junction temperature.
    // The sample sequence is triggered by the signal from the PWM module.
    //
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PWM0, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, PIN_I_PHASEU);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, PIN_I_PHASEV);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 2, PIN_I_PHASEW);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 3, PIN_VSENSE);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 4,
                             ADC_CTL_END | ADC_CTL_IE | ADC_CTL_TS);

    //
    // Enable sample sequence zero and its interrupt.
    //
    ADCSequenceEnable(ADC0_BASE,0);
    ADCIntEnable(ADC0_BASE, 0);
    IntEnable(INT_ADC0SS0);
}

//*****************************************************************************
//
// Close the Doxyen group.
//! @}
//
//*****************************************************************************
