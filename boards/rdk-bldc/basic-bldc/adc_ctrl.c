//*****************************************************************************
//
// adc_ctrl.c - ADC control routines.
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
// This is part of revision 9453 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_adc.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "adc_ctrl.h"
#include "main.h"
#include "pins.h"
#include "pwm_ctrl.h"
#include "trapmod.h"
#include "ui.h"

//*****************************************************************************
//
// \page adc_ctrl_intro Introduction
//
// Depending on the mode of operation, the ADC is used to monitor the motor
// phase current, motor phase back EMF voltage, linear hall sensor voltage,
// DC bus voltage, analog input voltage, and ambient temperature of the
// microcontroller.  Each of these values is sampled every PWM period based
// on a trigger from the PWM module.  Multiple ADC sequences are used to
// allow optimization of CPU usage.
//
// Readings from the ADC may be passed through a single-pole IIR low pass
// filter.  This helps to reduce the effects of high frequency noise (such as
// switching noise) on the sampled data.  A coefficient of 0.75 is used to
// simplify the integer math (requiring only a multiplication by 3, an
// addition, and a division by four).
//
// The individual motor phase RMS currents, motor RMS current, DC bus voltage,
// and ambient temperature are used outside this module.
//
// The code for handling the ADC is contained in <tt>adc_ctrl.c</tt>, with
// <tt>adc_ctrl.h</tt> containing the definitions for the variables and
// functions exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
// \defgroup adc_ctrl_api Definitions
// @{
//
//*****************************************************************************

//*****************************************************************************
//
// Forward reference.
//
//*****************************************************************************
static void ADC0IntIdle(void);

//*****************************************************************************
//
// The interrupt handler for the ADC0 mode.
//
//*****************************************************************************
static void (* g_pfnADC0Handler)(void) = ADC0IntIdle;

//*****************************************************************************
//
// The time of the ADC Sequence 0 Interrupt.
//
//*****************************************************************************
static unsigned long g_ulADC0Time;

//*****************************************************************************
//
// The bit number of the flag in #g_ulADCFlags that indicates that the
// next edge should be ignored by the Back EMF speed calculation code.  This
// is used at startup since there is no previous edge time to be used to
// calculate the time between edges.
//
//*****************************************************************************
#define FLAG_SKIP_BIT           0

//*****************************************************************************
//
// The bit number of the flag in #g_ulADCFlags that indicates that an edge
// has been seen (in the speed processing code).  This will prevent the
// #ADCTickHandler from resetting the rotor speed.
//
//*****************************************************************************
#define FLAG_EDGE_BIT           1

//*****************************************************************************
//
// The bit number of the flag in #g_ulADCFlags that indicates that the
// next edge should be ignored by the Linear Hall speed calculation code.
// This is used at startup since there is no previous edge time to be used to
// calculate the time between edges.
//
//*****************************************************************************
#define FLAG_SKIP_LINEAR_BIT    2

//*****************************************************************************
//
// The bit number of the flag in #g_ulADCFlags that indicates that a
// zero-crossing has been detected in the Back EMF processing.
//
//*****************************************************************************
#define FLAG_BEMF_EDGE_BIT      3

//*****************************************************************************
//
// A set of flags that provide status and control of the ADC Control
// module.
//
//*****************************************************************************
static unsigned long g_ulADCFlags = 
    ((1 << FLAG_SKIP_BIT) |
     (1 << FLAG_SKIP_LINEAR_BIT));

//*****************************************************************************
//
// This array contains the raw data read from the ADC sequence (prior to
// any filtering that may be applied).
//
//*****************************************************************************
static unsigned short g_pusADC0DataRaw[8];

//*****************************************************************************
//
// The Back EMF Voltage ADC count value.
//
//*****************************************************************************
static unsigned short g_pusBEMFVoltageCount[3];

//*****************************************************************************
//
// The Phase Current ADC count value.
//
//*****************************************************************************
static unsigned short g_pusPhaseCurrentCount[3];

//*****************************************************************************
//
// The Bus Voltage ADC count value.  This value is passed through an IIR
// filter with a coefficient of .875.
//
//*****************************************************************************
static unsigned short g_usBusVoltageCount;

//*****************************************************************************
//
// The Ambient Temperature ADC count value.  This value is passed through an
// IIR filter with a coefficient of .875.
//
//*****************************************************************************
static unsigned short g_usAmbientTempCount;

//*****************************************************************************
//
// The current passing through the three phases of the motor, specified in
// milliamperes as a signed value.
//
//*****************************************************************************
short g_psPhaseCurrent[3];

//*****************************************************************************
//
// An array containing the maximum phase currents seen during the last half
// cycle of each phase.  This is used to perform a peak detect on the phase
// currents.
//
//*****************************************************************************
static unsigned short g_pusPhaseMax[3];

//*****************************************************************************
//
// The index for the phase current being processed in the ADC sequence
// handler.
//
//*****************************************************************************
static unsigned char g_ucPhaseCurrentIndex = 0;

//*****************************************************************************
//
// The index for the phase current previously processed in the ADC sequence
// handler.
//
//*****************************************************************************
static unsigned char g_ucPreviousPhaseCurrentIndex = 0;

//*****************************************************************************
//
// The total current passing through the motor, specified in milliamperes as
// a signed value.
//
//*****************************************************************************
short g_sMotorCurrent;

//*****************************************************************************
//
// The average motor power, specified in milliwatts.
//
//*****************************************************************************
unsigned long g_ulMotorPower;

//*****************************************************************************
//
// The DC bus voltage, specified in millivolts.
//
//*****************************************************************************
unsigned long g_ulBusVoltage;

//*****************************************************************************
//
// The Phase Back EMF voltage, specified in millivolts.
//
//*****************************************************************************
unsigned long g_ulPhaseBEMFVoltage;

//*****************************************************************************
//
// The state of the Back EMF processing state machine.
//
//*****************************************************************************
static unsigned char g_ucBEMFState = 0;

//*****************************************************************************
//
// The time at which the last Back EMF speed edge occurred.
//
//*****************************************************************************
static unsigned long g_ulBEMFSpeedPrevious = 0;

//*****************************************************************************
//
// The time at which the last Back EMF edge occurred.
//
//*****************************************************************************
static unsigned long g_ulBEMFEdgePrevious = 0;

//*****************************************************************************
//
// The rotor speed as measured by the BEMF processing code.
//
//*****************************************************************************
unsigned long g_ulBEMFRotorSpeed = 0;

//*****************************************************************************
//
// The next Back EMF Hall state value.
//
//*****************************************************************************
unsigned long g_ulBEMFNextHall = 0;

//*****************************************************************************
//
// The Hall state value as determined by the Back EMF processing code.
//
//*****************************************************************************
unsigned long g_ulBEMFHallValue = 0;

//*****************************************************************************
//
// The ambient case temperature of the microcontroller, specified in degrees
// Celsius.
//
//*****************************************************************************
unsigned char g_ucAmbientTemp;

//*****************************************************************************
//
// The Linear Hall Sensor ADC values (scaled).
//
//*****************************************************************************
static unsigned short g_pusLinearHallSensor[3];

//*****************************************************************************
//
// The Linear Hall Sensor ADC Maximum Value.
//
//*****************************************************************************
static unsigned short g_pusLinearHallMax[3] = {1023, 1023, 1023};

//*****************************************************************************
//
// The Linear Hall Sensor ADC Minimum Value.
//
//*****************************************************************************
static unsigned short g_pusLinearHallMin[3] = {0, 0, 0};

//*****************************************************************************
//
// The Hall state value as determined by the linear Hall sensor processing
// code.
//
//*****************************************************************************
unsigned long g_ulLinearHallValue = 0;

//*****************************************************************************
//
// The time at which the last linear Hall sensor speed edge occurred.
//
//*****************************************************************************
static unsigned long g_ulLinearSpeedPrevious = 0;

//*****************************************************************************
//
// The rotor speed as measured by the linear Hall sensor processing code.
//
//*****************************************************************************
unsigned long g_ulLinearRotorSpeed = 0;

//*****************************************************************************
//
// The previous Hall state value as determined by the linear Hall sensor
// processing code.
//
//*****************************************************************************
static unsigned long g_ulLinearLastHall = 0;

//*****************************************************************************
//
// The angle of the motor drive on the previous ADC interrupt.
//
//*****************************************************************************
static unsigned long g_ulPrevAngle;

//*****************************************************************************
//
// The average commuation period for Sensorless operation.
//
//*****************************************************************************
static unsigned long g_ulBEMFPeriod;

//*****************************************************************************
//
// The minimum Back EMF ADC reading.
//
//*****************************************************************************
static unsigned long g_ulPhaseBEMFCountMin = 1023;

//*****************************************************************************
//
// The maximum Back EMF ADC reading.
//
//*****************************************************************************
static unsigned long g_ulPhaseBEMFCountMax = 0;

//*****************************************************************************
//
// The number of ADC readings to skip before BEMF edge detection.
//
//*****************************************************************************
static unsigned char g_ucBEMFSkipCount = UI_PARAM_BEMF_SKIP_COUNT;

//*****************************************************************************
//
// The calculation for Bus Voltage as read on the ADC input pin.  Defined
// here as a macro since it is used in several routines in this file.
// DC Bus Voltage is measured across a divider circuit using a 390K
// and 10K resistor.  The ADC reading is 0 to 1023 for voltages between
// between 0 and 3V.  This results in the following calculation:
// BV = R * ((390 + 10)/10) * (3 / 1024) * 1000
// ...
// BV = R * 1875 / 16
//
//*****************************************************************************
#define BUS_VOLTAGE_CALC(r)                                                  \
    do                                                                       \
    {                                                                        \
        g_usBusVoltageCount = (((g_usBusVoltageCount * 7) + (r)) / 8);       \
        g_ulBusVoltage = (((unsigned long)g_usBusVoltageCount * 1875) / 16); \
    } while(0)

//*****************************************************************************
//
// The calculation for ambient temperature as read from the internal
// temperature sensor.  Defined here as a macro since it is used in
// several places.
//
//*****************************************************************************
#define AMBIENT_TEMP_CALC(r)                                                 \
    do                                                                       \
    {                                                                        \
        g_usAmbientTempCount = (((g_usAmbientTempCount * 7) + (r)) / 8);     \
        g_ucAmbientTemp = ((59960 - (g_usAmbientTempCount * 100)) / 356);    \
    } while(0)

//*****************************************************************************
//
// The calculation for phase current.  Defined here as a macro since it is
// used in several places.  The phase current is measured as the voltage
// dropped across a 0.018 Ohm resistor, so current is ~55.6 times the voltage.
// This is then passed through an op amp with a gain of (1 + (390/140)).
// There is also a DC bias on the input of the op amp of 0.3 volts.
//
// The equation for the current then becomes:
//
// mA = ((R * (3 / 1024) / (1 + (390K / 140K))) - 0.3) * (1000 / 18) *
//      1000 * (10 / 14)
//
// Moving the DC bias to the outermost part of the equation, converting to
// current.
//
// mA = [R * (3 / 1024) / (1 + (390K / 140K)) * (1000 / 18) * 1000] - 16667.
//
// Reducing ...
//
// mA = [R * 3E6 / (1024 * (1 + (390K / 140K)) * 18)] - 16667.
// mA = [R * 3E6 / 69778] - 16667.
//
//*****************************************************************************
#define PHASE_CURRENT_CALC(r, u, l)             \
    do                                          \
    {                                           \
        (u) = (((r) * 3000000) / 69778);        \
        (l) = (long)(u) - (long)16667;          \
    } while (0)

//*****************************************************************************
//
// Handles the ADC sample sequence for idle (default) mode.
//
// This functions processes the ADC sequence zero for the idle mode.  The
// sequence has been programmed to read a  bus voltage and ambient
// temperature.
//
// \return None.
//
//*****************************************************************************
static void
ADC0IntIdle(void)
{
    volatile unsigned long ulTemp;

    //
    // Read the samples from the ADC FIFO.
    //
    g_pusADC0DataRaw[0] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[1] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);

    //
    // Reset the sequence if an overflow, underflow, or if the fifo is
    // NOT empty after reading what should have been all of the data.
    //
    if((HWREG(ADC0_BASE + ADC_O_OSTAT) & ADC_OSTAT_OV0) ||
       (HWREG(ADC0_BASE + ADC_O_USTAT) & ADC_USTAT_UV0) ||
       (!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)))
    {
        //
        // Disable the sequence.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~ADC_ACTSS_ASEN0;

        //
        // Drain the Sequence FIFO.
        //
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            //
            // Read the next sample.
            //
            ulTemp = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }

        //
        // Clear any overflow/underflow conditions that might exist.
        //
        HWREG(ADC0_BASE + ADC_O_OSTAT) = ADC_OSTAT_OV0;
        HWREG(ADC0_BASE + ADC_O_USTAT) = ADC_USTAT_UV0;

        //
        // Renable the sequence and return.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= ADC_ACTSS_ASEN0;
        return;
    }

    //
    // Filter and convert the Bus Voltage ADC count to a millivolt value.
    //
    BUS_VOLTAGE_CALC(g_pusADC0DataRaw[0]);

    //
    // Filter and convert the Ambient Temp ADC count to a Celsius value.
    //
    AMBIENT_TEMP_CALC(g_pusADC0DataRaw[1]);
}

//*****************************************************************************
//
// Handles the ADC sample sequence for trapezoid mode.
//
// This functions processes the ADC sequence zero for trapezoid mode.  The
// sequence has been programmed to read a single value of Phase current,
// along with bus voltage and ambient temperature.
//
// \return None.
//
//*****************************************************************************
static void
ADC0IntTrap(void)
{
    unsigned long ulTemp;
    long lTemp;
    static unsigned short usPhaseCurrentMax = 0;
    static unsigned long ulLastPWMEnable = 0;
    unsigned long ulPWMEnable;
    unsigned long ulTime;
    unsigned long ulBEMF;
    unsigned long ulIPhase;
    static unsigned char ucNextHallValue[] =
        {5, 2, 3, 4, 6, 1, 1, 6, 2, 5, 4, 3};
    static unsigned long ulCount = 0;

    //
    // Reset/Reconfigure the sequence if a change in PWM output drive
    // state is detected.
    //
    ulPWMEnable = HWREG(PWM0_BASE + PWM_O_ENABLE);
    if(ulPWMEnable != ulLastPWMEnable)
    {
        //
        // Disable the sequence.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~ADC_ACTSS_ASEN0;

        //
        // Drain the Sequence FIFO.
        //
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            //
            // Read the next sample.
            //
            ulTemp = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }

        //
        // Clear any overflow/underflow conditions that might exist.
        //
        HWREG(ADC0_BASE + ADC_O_OSTAT) = ADC_OSTAT_OV0;
        HWREG(ADC0_BASE + ADC_O_USTAT) = ADC_USTAT_UV0;

        //
        // If the low side of Phase A is active, enable current readings
        // on the Phase A ADC current input signal.
        //
        if(ulPWMEnable & 0x2)
        {
            ulIPhase = PIN_IPHASEA;
            g_ucPhaseCurrentIndex = 0;
        }

        //
        // If the low side of Phase B is active, enable current readings
        // on the Phase B ADC current input signal.
        //
        else if(ulPWMEnable & 0x8)
        {
            ulIPhase = PIN_IPHASEB;
            g_ucPhaseCurrentIndex = 1;
        }

        //
        // Here, assume Phase C is active, enable current readings
        // on the Phase C ADC current input signal.
        //
        else
        {
            ulIPhase = PIN_IPHASEC;
            g_ucPhaseCurrentIndex = 2;
        }
        
        //
        // Based on the new PWM output state, determine which Back EMF
        // detection state we should be in.
        //
        // ------+---------------------------+---------------------------+----+
        // Phase |                           |                           |    |
        // Drive | Forward                   | Reverse                   |F,R |
        // ------+---------------------------+---------------------------+----+
        // B+ C- | Fall Phase A, Rise Hall A | Rise Phase A, Fall Hall B |0,6 |
        // B- C+ | Rise Phase A, Fall Hall A | Fall Phase A, Rise Hall B |1,7 |
        // A- C+ | Fall Phase B, Rise Hall B | Rise Phase B, Fall Hall C |2,8 |
        // A+ C- | Rise Phase B, Fall Hall B | Fall Phase B, Rise Hall C |3,9 |
        // A+ B- | Fall Phase C, Rise Hall C | Rise Phase C, Fall Hall A |4,10|
        // A- B+ | Rise Phase C, Fall Hall C | Fall Phase C, Rise Hall A |5,11|
        // ------+---------------------------+---------------------------+----+
        //
        if((ulPWMEnable & 0x03) == 0)
        {
            ulBEMF = PIN_VBEMFA;
            g_ucBEMFState = (ulPWMEnable & 0x08) ? 1 : 0 ;
        }
        else if((ulPWMEnable & 0x0c) == 0)
        {
            ulBEMF = PIN_VBEMFB;
            g_ucBEMFState = (ulPWMEnable & 0x02) ? 2 : 3 ;
        }
        else
        {
            ulBEMF = PIN_VBEMFC;
            g_ucBEMFState = (ulPWMEnable & 0x02) ? 5 : 4 ;
        }
        if(MainIsReverse())
        {
            g_ucBEMFState += 6;
        }

        //
        // Reset the Back EMF edge flags for next detection.
        //
        HWREGBITW(&g_ulADCFlags, FLAG_BEMF_EDGE_BIT) = 0;
        
        //
        // Reset the Back EMF detection skip counter.
        //
        g_ucBEMFSkipCount = UI_PARAM_BEMF_SKIP_COUNT;

        //
        // Reprogram the Back EMF and Phase Current sequence entries.
        //
        ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ulBEMF);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ulIPhase);

        //
        // Save the PWM output state.
        //
        ulLastPWMEnable = ulPWMEnable;

        //
        // Enable the sequence and return.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= ADC_ACTSS_ASEN0;
        return;
    }

    //
    // Read the samples from the ADC FIFO.
    //
    g_pusADC0DataRaw[0] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[1] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[2] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[3] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);

    //
    // Reset the sequence if an overflow, underflow, or if the fifo is
    // NOT empty after reading what should have been all of the data.
    //
    if((HWREG(ADC0_BASE + ADC_O_OSTAT) & ADC_OSTAT_OV0) ||
       (HWREG(ADC0_BASE + ADC_O_USTAT) & ADC_USTAT_UV0) ||
       (!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)))
    {
        //
        // Disable the sequence.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~ADC_ACTSS_ASEN0;

        //
        // Drain the Sequence FIFO.
        //
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            //
            // Read the next sample.
            //
            ulTemp = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }

        //
        // Clear any overflow/underflow conditions that might exist.
        //
        HWREG(ADC0_BASE + ADC_O_OSTAT) = ADC_OSTAT_OV0;
        HWREG(ADC0_BASE + ADC_O_USTAT) = ADC_USTAT_UV0;

        //
        // Renable the sequence and return.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= ADC_ACTSS_ASEN0;
        return;
    }

    //
    // Save the raw data, filtering as needed.
    //
    g_pusBEMFVoltageCount[0] = g_pusADC0DataRaw[0];

    //
    // Filter and convert the Bus Voltage ADC count to a millivolt value.
    //
    BUS_VOLTAGE_CALC(g_pusADC0DataRaw[2]);

    //
    // Filter and convert the Ambient Temp ADC count to a Celsius value.
    //
    AMBIENT_TEMP_CALC(g_pusADC0DataRaw[3]);

    //
    // See if the motor drive is running.
    //
    if(!MainIsRunning())
    {
        //
        // Since the motor drive is not running, there is no current through
        // the motor, nor is there any Back EMF voltage.
        //
        g_psPhaseCurrent[0] = 0;
        g_psPhaseCurrent[1] = 0;
        g_psPhaseCurrent[2] = 0;
        g_sMotorCurrent = 0;
        g_ulMotorPower = 0;

        //
        // Update the min/max based on current ADC sample.
        //
        if(g_pusBEMFVoltageCount[0] < g_ulPhaseBEMFCountMin)
        {
            g_ulPhaseBEMFCountMin = g_pusBEMFVoltageCount[0];
        }
        if(g_pusBEMFVoltageCount[0] > g_ulPhaseBEMFCountMax)
        {
            g_ulPhaseBEMFCountMax = g_pusBEMFVoltageCount[0];
        }

        //
        // Dynamically adjust the min/max values periodically.
        //
        ulCount = ((ulCount + 1) % 10);
        if(ulCount == 0)
        {
            g_ulPhaseBEMFCountMax--;
            g_ulPhaseBEMFCountMin++;
        }
    
        //
        // Make sure Max and Min are not the same.
        //
        if(g_ulPhaseBEMFCountMax <= g_ulPhaseBEMFCountMin)
        {
            g_ulPhaseBEMFCountMax = g_ulPhaseBEMFCountMin + 1;
        }
    
        //
        // Calculate the Back EMF voltage range.
        //
        g_ulPhaseBEMFVoltage = (((g_ulPhaseBEMFVoltage * 7) +
            (((g_ulPhaseBEMFCountMax - g_ulPhaseBEMFCountMin) * 120000) /
             1024)) / 8);

        //
        // Reset the Back EMF Commutation Period Average.
        //
        g_ulBEMFPeriod = 0;

        //
        // If the motor is NOT running, there is nothing more to do here.
        //
        return;
    }

    //
    // Track the maximum phase current reading over the commutation.
    //
    if(usPhaseCurrentMax < g_pusADC0DataRaw[1])
    {
        usPhaseCurrentMax = g_pusADC0DataRaw[1];
    }

    //
    // If we have changed phases, calculate the phase current average.
    //
    if(g_ucPhaseCurrentIndex != g_ucPreviousPhaseCurrentIndex)
    {
        //
        // Calculate the phase current.
        //
        PHASE_CURRENT_CALC(usPhaseCurrentMax, ulTemp, lTemp);
        usPhaseCurrentMax = 0;

        //
        // Take the RMS reading (because the current is somewhat periodic when 
        // PWM duty cycle is < 100%).
        //
        //lTemp = ((lTemp * 181) / 256);

        //
        // Accumulate the average.
        //
        g_sMotorCurrent = (short)((((long)g_sMotorCurrent * 3) + lTemp) / 4);
        g_psPhaseCurrent[g_ucPhaseCurrentIndex] = g_sMotorCurrent;

        //
        // Do the power calculation.
        //
        if(g_sMotorCurrent > 0)
        {
            ulTemp = (g_ulBusVoltage * g_ulTrapDutyCycle);
            ulTemp = (ulTemp / 10000);
            ulTemp = (ulTemp * (unsigned long)g_sMotorCurrent);
            ulTemp = ulTemp / 1000;
            g_ulMotorPower = (((g_ulMotorPower * 15) + ulTemp) / 16);
        }
        else
        {
            g_ulMotorPower = 0;
        }
        g_ucPreviousPhaseCurrentIndex = g_ucPhaseCurrentIndex;
    }

    //
    // If Back EMF trigger point has been found, there is nothing
    // more to do until the PWM drive signals change.
    //
    if(HWREGBITW(&g_ulADCFlags, FLAG_BEMF_EDGE_BIT) == 1)
    {
        return;
    }

    //
    // Check to see if we are still in the skip mode.  This skip prevents us
    // from detecting a false BEMF zero crossing.
    //
    if(g_ucBEMFSkipCount)
    {
        g_ucBEMFSkipCount--;
        return;
    }

    //
    // Check for Back EMF Trigger Point.
    //
    switch(g_ucBEMFState)
    {
        case 0:
        case 2:
        case 4:
        case 7:
        case 9:
        case 11:
            if(g_pusBEMFVoltageCount[0] < (g_usBusVoltageCount / 2))
            {
                HWREGBITW(&g_ulADCFlags, FLAG_BEMF_EDGE_BIT) = 1;
                g_ulBEMFNextHall = ucNextHallValue[g_ucBEMFState];
            }
            break;

        case 1:
        case 3:
        case 5:
        case 6:
        case 8:
        case 10:
            if(g_pusBEMFVoltageCount[0] > (g_usBusVoltageCount / 2))
            {
                HWREGBITW(&g_ulADCFlags, FLAG_BEMF_EDGE_BIT) = 1;
                g_ulBEMFNextHall = ucNextHallValue[g_ucBEMFState];
            }
            break;
    }

    //
    // If we detected an edge, start a timer to trigger a commutation.
    //
    if(HWREGBITW(&g_ulADCFlags, FLAG_BEMF_EDGE_BIT) == 1)
    {
        if(MainIsRunning() && !MainIsStartup())
        {
            //
            // Calculate the period of this commutation.
            //
            ulTime = g_ulADC0Time - g_ulBEMFEdgePrevious;

            //
            // Accomodate jitter by adjusting the period based on
            // the average speed.
            //
            ulTemp = (((3 * g_ulBEMFPeriod) - ulTime) / 2);

            //
            // Assume that the BEMF detection occurs at the half-way point
            // between a commutation
            //
            ulTemp = (ulTemp / 2);

            //
            // Account for latency from BEMF sample time to the TIMER
            // program/enable time.
            // ~3.5us (3500ns) from BEMF sample to ADC sequence interrupt.
            // ~350 processor clocks of latency from start of interrupt to
            // here.
            //
            ulTemp = (ulTemp - (3500 / SYSTEM_CLOCK_WIDTH));
            ulTemp = (ulTemp - 350);

            //
            // Allow for the fact that the zero-crossing may have occurred
            // at any point between the current sample and the previous
            // sample.  For now, assume one half the PWM period.
            //
            ulTemp = (ulTemp -
                (((g_ulPWMWidth * PWM_CLOCK_WIDTH) / SYSTEM_CLOCK_WIDTH) / 2));

            //
            // Program and enable the timer.
            //
            HWREG(TIMER0_BASE + TIMER_O_TAILR) = ulTemp;
            HWREG(TIMER0_BASE + TIMER_O_CTL) |=
                (TIMER_A & (TIMER_CTL_TAEN | TIMER_CTL_TBEN));
        }
        g_ulBEMFEdgePrevious = g_ulADC0Time;
    }

    //
    // Compute the new speed from the time between edges.
    //
    if((HWREGBITW(&g_ulADCFlags, FLAG_BEMF_EDGE_BIT) == 1) &&
       ((g_ucBEMFState == 0) || (g_ucBEMFState ==11)))
    {
        //
        // Set the flag to indicate that we have seen an edge.
        // 
        HWREGBITW(&g_ulADCFlags, FLAG_EDGE_BIT) = 1;

        //
        // See if this edge should be skipped.
        //
        if(HWREGBITW(&g_ulADCFlags, FLAG_SKIP_BIT))
        {
            //
            // This edge should be skipped, but an edge time now exists so the
            // next edge should not be skipped.
            //
            HWREGBITW(&g_ulADCFlags, FLAG_SKIP_BIT) = 0;

            //
            // Save the time of the current edge.
            //
            g_ulBEMFSpeedPrevious = g_ulADC0Time;

            //
            // There is nothing further to be done.
            //
            return;
        }
            
        //
        // Compute the time between this edge and the previous edge.
        //
        ulTime = g_ulADC0Time - g_ulBEMFSpeedPrevious;

        //
        // Save the time of the current edge.
        //
        g_ulBEMFSpeedPrevious = g_ulADC0Time;

        //
        // Compute the new speed from the time between edges, running it
        // through a low pass filter with a coefficient of .875.
        //
        ulTemp = (SYSTEM_CLOCK * 60U);
        ulTemp = (ulTemp / ulTime);
        ulTemp = (ulTemp / (UI_PARAM_NUM_POLES / 2));
        g_ulBEMFRotorSpeed = (((g_ulBEMFRotorSpeed * 7) + ulTemp) / 8);

        //
        // Accumulate average Commutation Period.
        //
        g_ulBEMFPeriod = (((g_ulBEMFPeriod * 3) + (ulTime / 6)) / 4);
    }
}

//*****************************************************************************
//
// Handles the ADC sample sequence for trapezoid mode, with linear hall
// sensors.
//
// This functions processes the ADC sequence zero for trapezoid mode.  The
// sequence has been programmed to read a single value of Phase current, the
// linear hall sensor inputs, along with bus voltage and ambient temperature.
//
// \return None.
//
//*****************************************************************************
static void
ADC0IntTrapLinear(void)
{
    unsigned long ulTemp;
    long lTemp;
    static unsigned short usPhaseCurrentMax = 0;
    static unsigned long ulLastPWMEnable = 0;
    unsigned long ulPWMEnable;
    static unsigned long ulLinearCount = 0;
    unsigned long ulTime;
    unsigned long ulIdx;

    //
    // Reset/Reconfigure the sequence if a change in PWM output drive
    // state is detected.
    //
    ulPWMEnable = HWREG(PWM0_BASE + PWM_O_ENABLE);
    if(ulPWMEnable != ulLastPWMEnable)
    {
        unsigned long ulIPhase;

        //
        // Disable the sequence.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~ADC_ACTSS_ASEN0;

        //
        // Drain the Sequence FIFO.
        //
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            //
            // Read the next sample.
            //
            ulTemp = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }

        //
        // Clear any overflow/underflow conditions that might exist.
        //
        HWREG(ADC0_BASE + ADC_O_OSTAT) = ADC_OSTAT_OV0;
        HWREG(ADC0_BASE + ADC_O_USTAT) = ADC_USTAT_UV0;

        //
        // Save the PWM output state.
        //
        ulLastPWMEnable = ulPWMEnable;

        //
        // If the low side of Phase A is active, enable current readings
        // on the Phase A ADC current input signal.
        //
        if(ulPWMEnable & 0x2)
        {
            ulIPhase = PIN_IPHASEA;
            g_ucPhaseCurrentIndex = 0;
        }

        //
        // If the low side of Phase B is active, enable current readings
        // on the Phase B ADC current input signal.
        //
        else if(ulPWMEnable & 0x8)
        {
            ulIPhase = PIN_IPHASEB;
            g_ucPhaseCurrentIndex = 1;
        }

        //
        // Here, assume Phase C is active, enable current readings
        // on the Phase C ADC current input signal.
        //
        else
        {
            ulIPhase = PIN_IPHASEC;
            g_ucPhaseCurrentIndex = 2;
        }
        
        //
        // Reprogram the sequence to read Back EMF voltage first (with the
        // PWM Pulse Active) and 5 Phase current readings to follow.
        //
        ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ulIPhase);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 1, PIN_VBEMFA);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 2, PIN_VBEMFB);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 3, PIN_VBEMFC);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 4, PIN_VSENSE);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 5,
                                 ADC_CTL_END | ADC_CTL_IE | ADC_CTL_TS);

        //
        // Enable the sequence and return.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= ADC_ACTSS_ASEN0;
        return;
    }

    //
    // Read the samples from the ADC FIFO.
    //
    g_pusADC0DataRaw[0] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[1] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[2] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[3] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[4] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[5] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);

    //
    // Reset the sequence if an overflow, underflow, or if the fifo is
    // NOT empty after reading what should have been all of the data.
    //
    if((HWREG(ADC0_BASE + ADC_O_OSTAT) & ADC_OSTAT_OV0) ||
       (HWREG(ADC0_BASE + ADC_O_USTAT) & ADC_USTAT_UV0) ||
       (!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)))
    {
        //
        // Disable the sequence.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~ADC_ACTSS_ASEN0;

        //
        // Drain the Sequence FIFO.
        //
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            //
            // Read the next sample.
            //
            ulTemp = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }

        //
        // Clear any overflow/underflow conditions that might exist.
        //
        HWREG(ADC0_BASE + ADC_O_OSTAT) = ADC_OSTAT_OV0;
        HWREG(ADC0_BASE + ADC_O_USTAT) = ADC_USTAT_UV0;

        //
        // Renable the sequence and return.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= ADC_ACTSS_ASEN0;
        return;
    }

    //
    // Filter and convert the Bus Voltage ADC count to a millivolt value.
    //
    BUS_VOLTAGE_CALC(g_pusADC0DataRaw[4]);

    //
    // Filter and convert the Ambient Temp ADC count to a Celsius value.
    //
    AMBIENT_TEMP_CALC(g_pusADC0DataRaw[5]);

    //
    // Expand the Linear Hall Sensor data to full 10-bit range.
    // 
    for(ulIdx = 0, ulLinearCount++; ulIdx < 3; ulIdx++)
    {
        unsigned short usRange, usMin;

        //
        // Adjust the min/max values inward to keep the values dynamic.
        //
        if(((ulLinearCount % 2000) == 0) && MainIsRunning())
        {
            g_pusLinearHallMax[ulIdx]--;
            g_pusLinearHallMin[ulIdx]++;
        }

        //
        // Find max/min hall sensor values.
        //
        if(g_pusADC0DataRaw[1 + ulIdx] > g_pusLinearHallMax[ulIdx])
        {
            g_pusLinearHallMax[ulIdx] = g_pusADC0DataRaw[1 + ulIdx];
        }
        if(g_pusADC0DataRaw[1 + ulIdx] < g_pusLinearHallMin[ulIdx])
        {
            g_pusLinearHallMin[ulIdx] = g_pusADC0DataRaw[1 + ulIdx];
        }

        //
        // Adjust the ADC values to full-scale 10-bit ADC values.
        //
        usRange = g_pusLinearHallMax[ulIdx];
        usRange -= g_pusLinearHallMin[ulIdx];
        if(!usRange)
        {
            usRange++;
        }
        usMin = g_pusLinearHallMin[ulIdx];
        g_pusLinearHallSensor[ulIdx] = (unsigned short)
            (((unsigned long)(g_pusADC0DataRaw[1 + ulIdx] - usMin) * 1023) /
            usRange);
    }
    
    //
    // Convert Linear Hall data to a Hall Sensor Value (A)
    //
    if(g_pusLinearHallSensor[0] > 614)
    {
        g_ulLinearHallValue |= 0x01;
    }
    else if (g_pusLinearHallSensor[0] < 410)
    {
        g_ulLinearHallValue &= ~0x01;
    }

    //
    // Convert Linear Hall data to a Hall Sensor Value (B)
    //
    if(g_pusLinearHallSensor[1] > 614)
    {
        g_ulLinearHallValue |= 0x02;
    }
    else if (g_pusLinearHallSensor[1] < 410)
    {
        g_ulLinearHallValue &= ~0x02;
    }

    //
    // Convert Linear Hall data to a Hall Sensor Value (C)
    //
    if(g_pusLinearHallSensor[2] > 614)
    {
        g_ulLinearHallValue |= 0x04;
    }
    else if (g_pusLinearHallSensor[2] < 410)
    {
        g_ulLinearHallValue &= ~0x04;
    }

    //
    // Check if the linear hall value has changed.
    //
    if(g_ulLinearHallValue != g_ulLinearLastHall)
    {
        //
        // Commutate the motor, if we are in the appropriate mode to do so.
        //
        TrapModulate(g_ulLinearHallValue);

        //
        // Save the current linear hall value.
        //
        g_ulLinearLastHall = g_ulLinearHallValue;

        //
        // Compute the new speed from the time between edges if the Hall
        // state value has changed and the motor has completed one electrical
        // revolution.
        //
        if(g_ulLinearHallValue == 5)
        {
            //
            // Set the flag to indicate that we have seen an edge.
            // 
            HWREGBITW(&g_ulADCFlags, FLAG_EDGE_BIT) = 1;

            //
            // See if this edge should be skipped.
            //
            if(HWREGBITW(&g_ulADCFlags, FLAG_SKIP_LINEAR_BIT))
            {
                //
                // This edge should be skipped, but an edge time now exists so
                // the next edge should not be skipped.
                //
                HWREGBITW(&g_ulADCFlags, FLAG_SKIP_LINEAR_BIT) = 0;

                //
                // Save the time of the current edge.
                //
                g_ulLinearSpeedPrevious = g_ulADC0Time;

                //
                // There is nothing further to be done.
                //
                return;
            }
            
            //
            // Compute the time between this edge and the previous edge.
            //
            ulTime = g_ulADC0Time - g_ulLinearSpeedPrevious;

            //
            // Save the time of the current edge.
            //
            g_ulLinearSpeedPrevious = g_ulADC0Time;

            //
            // Compute the new speed from the time between edges, running it
            // through a low pass filter with a coefficient of .875.
            //
            ulTemp = (SYSTEM_CLOCK * 60U);
            ulTemp = (ulTemp / ulTime);
            ulTemp = (ulTemp / (UI_PARAM_NUM_POLES / 2));
            g_ulLinearRotorSpeed = (((g_ulLinearRotorSpeed * 7) + ulTemp) / 8);
        }
    }

    //
    // See if the motor drive is running.
    //
    if(!MainIsRunning())
    {
        //
        // Since the motor drive is not running, there is no current through
        // the motor, nor is there any Back EMF voltage.
        //
        g_psPhaseCurrent[0] = 0;
        g_psPhaseCurrent[1] = 0;
        g_psPhaseCurrent[2] = 0;
        g_sMotorCurrent = 0;
        g_ulMotorPower = 0;

        //
        // There is nothing further to be done since the motor is not running.
        //
        return;
    }

    //
    // Track the maximum phase current reading over the commutation.
    //
    if(usPhaseCurrentMax < g_pusADC0DataRaw[0])
    {
        usPhaseCurrentMax = g_pusADC0DataRaw[0];
    }

    //
    // If we have changed phases, calculate the phase current average.
    //
    if(g_ucPhaseCurrentIndex != g_ucPreviousPhaseCurrentIndex)
    {
        //
        // Calculate the phase current.
        //
        PHASE_CURRENT_CALC(usPhaseCurrentMax, ulTemp, lTemp);
        usPhaseCurrentMax = 0;

        //
        // Take the RMS reading (because the current is somewhat periodic when 
        // PWM duty cycle is < 100%).
        //
        //lTemp = ((lTemp * 181) / 256);

        //
        // Accumulate the average.
        //
        g_sMotorCurrent = (short)((((long)g_sMotorCurrent * 3) + lTemp) / 4);
        g_psPhaseCurrent[g_ucPhaseCurrentIndex] = g_sMotorCurrent;

        //
        // Do the power calculation.
        //
        if(g_sMotorCurrent > 0)
        {
            ulTemp = (g_ulBusVoltage * g_ulTrapDutyCycle);
            ulTemp = (ulTemp / 10000);
            ulTemp = (ulTemp * (unsigned long)g_sMotorCurrent);
            ulTemp = ulTemp / 1000;
            g_ulMotorPower = (((g_ulMotorPower * 15) + ulTemp) / 16);
        }
        else
        {
            g_ulMotorPower = 0;
        }
        g_ucPreviousPhaseCurrentIndex = g_ucPhaseCurrentIndex;
    }
}

//*****************************************************************************
//
// Handles the ADC sample sequence for sinusoid mode.
//
// This functions processes the ADC sequence zero for trapezoid mode.  The
// sequence has been programmed to read a single value of Phase current,
// along with bus voltage and ambient temperature.
//
// \return None.
//
//*****************************************************************************
static void
ADC0IntSine(void)
{
    unsigned long ulTemp;
    long lTemp;
    unsigned long ulIdx;

    //
    // Read the samples from the ADC FIFO.
    //
    g_pusADC0DataRaw[0] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[1] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[2] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[3] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[4] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);

    //
    // Reset the sequence if an overflow, underflow, or if the fifo is
    // NOT empty after reading what should have been all of the data.
    //
    if((HWREG(ADC0_BASE + ADC_O_OSTAT) & ADC_OSTAT_OV0) ||
       (HWREG(ADC0_BASE + ADC_O_USTAT) & ADC_USTAT_UV0) ||
       (!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)))
    {
        //
        // Disable the sequence.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~ADC_ACTSS_ASEN0;

        //
        // Drain the Sequence FIFO.
        //
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            //
            // Read the next sample.
            //
            ulTemp = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }

        //
        // Clear any overflow/underflow conditions that might exist.
        //
        HWREG(ADC0_BASE + ADC_O_OSTAT) = ADC_OSTAT_OV0;
        HWREG(ADC0_BASE + ADC_O_USTAT) = ADC_USTAT_UV0;

        //
        // Renable the sequence and return.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= ADC_ACTSS_ASEN0;
        return;
    }

    //
    // Save the raw data, filtering as needed.
    //
    g_pusPhaseCurrentCount[0] = (((g_pusPhaseCurrentCount[0] * 3) +
                                g_pusADC0DataRaw[0]) / 4);
    g_pusPhaseCurrentCount[1] = (((g_pusPhaseCurrentCount[1] * 3) +
                                g_pusADC0DataRaw[1]) / 4);
    g_pusPhaseCurrentCount[2] = (((g_pusPhaseCurrentCount[2] * 3) +
                                g_pusADC0DataRaw[2]) / 4);

    //
    // Filter and convert the Bus Voltage ADC count to a millivolt value.
    //
    BUS_VOLTAGE_CALC(g_pusADC0DataRaw[3]);

    //
    // Filter and convert the Ambient Temp ADC count to a Celsius value.
    //
    AMBIENT_TEMP_CALC(g_pusADC0DataRaw[4]);

    //
    // See if the motor drive is running.
    //
    if(!MainIsRunning())
    {
        //
        // Since the motor drive is not running, there is no current through
        // the motor, nor is there any Back EMF voltage.
        //
        g_psPhaseCurrent[0] = 0;
        g_psPhaseCurrent[1] = 0;
        g_psPhaseCurrent[2] = 0;
        g_sMotorCurrent = 0;
        g_ulMotorPower = 0;

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
            // Calculate the phase current.
            //
            PHASE_CURRENT_CALC(g_pusPhaseMax[ulIdx], ulTemp, lTemp);

            //
            // Take the RMS reading.
            //
            lTemp = ((lTemp * 181) / 256);

            //
            // Save the value.
            //
            g_psPhaseCurrent[ulIdx] = lTemp;

            //
            // Reset the maximum phase current seen to zero to prepare for the
            // next cycle.
            //
            g_pusPhaseMax[ulIdx] = 0;
        }

        //
        // Average the RMS current of the three phases to get the RMS motor
        // current.
        //
        lTemp = (long)g_psPhaseCurrent[0];
        lTemp += (long)g_psPhaseCurrent[1];
        lTemp += (long)g_psPhaseCurrent[2];
        g_sMotorCurrent = (short)(lTemp / 3);

        if(g_sMotorCurrent > 0)
        {
            ulTemp = (g_ulBusVoltage * g_ulTrapDutyCycle);
            ulTemp = (ulTemp / 10000);
            ulTemp = (ulTemp * (unsigned long)g_sMotorCurrent);
            ulTemp = ulTemp / 1000;
            g_ulMotorPower = (((g_ulMotorPower * 15) + ulTemp) / 16);
        }
        else
        {
            g_ulMotorPower = 0;
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
        if(g_pusPhaseCurrentCount[ulIdx] > g_pusPhaseMax[ulIdx])
        {
            //
            // Save this ADC reading as the maximum.
            //
            g_pusPhaseMax[ulIdx] = g_pusPhaseCurrentCount[ulIdx];
        }
    }

    //
    // Save the current motor drive angle for the next set of samples.
    //
    g_ulPrevAngle = g_ulAngle;
}

//*****************************************************************************
//
// Handles the ADC sample sequence for sinusoid mode.
//
// This functions processes the ADC sequence zero for trapezoid mode.  The
// sequence has been programmed to read a single value of Phase current,
// along with bus voltage and ambient temperature.
//
// \return None.
//
//*****************************************************************************
static void
ADC0IntSineLinear(void)
{
    unsigned long ulTemp;
    long lTemp;
    static unsigned long ulLinearCount = 0;
    unsigned long ulTime;
    unsigned long ulIdx;

    //
    // Read the samples from the ADC FIFO.
    //
    g_pusADC0DataRaw[0] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[1] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[2] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[3] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[4] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[5] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[6] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    g_pusADC0DataRaw[7] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);

    //
    // Reset the sequence if an overflow, underflow, or if the fifo is
    // NOT empty after reading what should have been all of the data.
    //
    if((HWREG(ADC0_BASE + ADC_O_OSTAT) & ADC_OSTAT_OV0) ||
       (HWREG(ADC0_BASE + ADC_O_USTAT) & ADC_USTAT_UV0) ||
       (!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)))
    {
        //
        // Disable the sequence.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~ADC_ACTSS_ASEN0;

        //
        // Drain the Sequence FIFO.
        //
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            //
            // Read the next sample.
            //
            ulTemp = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }

        //
        // Clear any overflow/underflow conditions that might exist.
        //
        HWREG(ADC0_BASE + ADC_O_OSTAT) = ADC_OSTAT_OV0;
        HWREG(ADC0_BASE + ADC_O_USTAT) = ADC_USTAT_UV0;

        //
        // Renable the sequence and return.
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= ADC_ACTSS_ASEN0;
        return;
    }

    //
    // Save the raw data, filtering as needed.
    //
    g_pusPhaseCurrentCount[0] = (((g_pusPhaseCurrentCount[0] * 3) +
                                g_pusADC0DataRaw[0]) / 4);
    g_pusPhaseCurrentCount[1] = (((g_pusPhaseCurrentCount[1] * 3) +
                                g_pusADC0DataRaw[1]) / 4);
    g_pusPhaseCurrentCount[2] = (((g_pusPhaseCurrentCount[2] * 3) +
                                g_pusADC0DataRaw[2]) / 4);

    //
    // Filter and convert the Bus Voltage ADC count to a millivolt value.
    //
    BUS_VOLTAGE_CALC(g_pusADC0DataRaw[6]);

    //
    // Filter and convert the Ambient Temp ADC count to a Celsius value.
    //
    AMBIENT_TEMP_CALC(g_pusADC0DataRaw[7]);

    //
    // Expand the Linear Hall Sensor data to full 10-bit range.
    // 
    for(ulIdx = 0, ulLinearCount++; ulIdx < 3; ulIdx++)
    {
        unsigned short usRange, usMin;

        //
        // Adjust the min/max values inward to keep the values dynamic.
        //
        if(((ulLinearCount % 2000) == 0) && MainIsRunning())
        {
            g_pusLinearHallMax[ulIdx]--;
            g_pusLinearHallMin[ulIdx]++;
        }

        //
        // Find max/min hall sensor values.
        //
        if(g_pusADC0DataRaw[3 + ulIdx] > g_pusLinearHallMax[ulIdx])
        {
            g_pusLinearHallMax[ulIdx] = g_pusADC0DataRaw[3 + ulIdx];
        }
        if(g_pusADC0DataRaw[3 + ulIdx] < g_pusLinearHallMin[ulIdx])
        {
            g_pusLinearHallMin[ulIdx] = g_pusADC0DataRaw[3 + ulIdx];
        }

        //
        // Adjust the ADC values to full-scale 10-bit ADC values.
        //
        usRange = g_pusLinearHallMax[ulIdx];
        usRange -= g_pusLinearHallMin[ulIdx];
        if(!usRange)
        {
            usRange++;
        }
        usMin = g_pusLinearHallMin[ulIdx];
        g_pusLinearHallSensor[ulIdx] = (unsigned short)
            (((unsigned long)(g_pusADC0DataRaw[3 + ulIdx] - usMin) * 1023) /
            usRange);
    }
    
    //
    // Convert Linear Hall data to a Hall Sensor Value (A)
    //
    if(g_pusLinearHallSensor[0] > 614)
    {
        g_ulLinearHallValue |= 0x01;
    }
    else if (g_pusLinearHallSensor[0] < 410)
    {
        g_ulLinearHallValue &= ~0x01;
    }

    //
    // Convert Linear Hall data to a Hall Sensor Value (B)
    //
    if(g_pusLinearHallSensor[1] > 614)
    {
        g_ulLinearHallValue |= 0x02;
    }
    else if (g_pusLinearHallSensor[1] < 410)
    {
        g_ulLinearHallValue &= ~0x02;
    }

    //
    // Convert Linear Hall data to a Hall Sensor Value (C)
    //
    if(g_pusLinearHallSensor[2] > 614)
    {
        g_ulLinearHallValue |= 0x04;
    }
    else if (g_pusLinearHallSensor[2] < 410)
    {
        g_ulLinearHallValue &= ~0x04;
    }

    //
    // Check if the linear hall value has changed.
    //
    if(g_ulLinearHallValue != g_ulLinearLastHall)
    {
        //
        // Save the current linear hall value.
        //
        g_ulLinearLastHall = g_ulLinearHallValue;

        //
        // Compute the new speed from the time between edges if the Hall
        // state value has changed and the motor has completed one electrical
        // revolution.
        //
        if(g_ulLinearHallValue == 5)
        {
            //
            // Set the flag to indicate that we have seen an edge.
            // 
            HWREGBITW(&g_ulADCFlags, FLAG_EDGE_BIT) = 1;

            //
            // See if this edge should be skipped.
            //
            if(HWREGBITW(&g_ulADCFlags, FLAG_SKIP_LINEAR_BIT))
            {
                //
                // This edge should be skipped, but an edge time now exists so
                // the next edge should not be skipped.
                //
                HWREGBITW(&g_ulADCFlags, FLAG_SKIP_LINEAR_BIT) = 0;

                //
                // Save the time of the current edge.
                //
                g_ulLinearSpeedPrevious = g_ulADC0Time;

                //
                // There is nothing further to be done.
                //
                return;
            }
            
            //
            // Compute the time between this edge and the previous edge.
            //
            ulTime = g_ulADC0Time - g_ulLinearSpeedPrevious;

            //
            // Save the time of the current edge.
            //
            g_ulLinearSpeedPrevious = g_ulADC0Time;

            //
            // Compute the new speed from the time between edges, running it
            // through a low pass filter with a coefficient of .875.
            //
            ulTemp = (SYSTEM_CLOCK * 60U);
            ulTemp = (ulTemp / ulTime);
            ulTemp = (ulTemp / (UI_PARAM_NUM_POLES / 2));
            g_ulLinearRotorSpeed = (((g_ulLinearRotorSpeed * 7) + ulTemp) / 8);
        }
    }

    //
    // See if the motor drive is running.
    //
    if(!MainIsRunning())
    {
        //
        // Since the motor drive is not running, there is no current through
        // the motor, nor is there any Back EMF voltage.
        //
        g_psPhaseCurrent[0] = 0;
        g_psPhaseCurrent[1] = 0;
        g_psPhaseCurrent[2] = 0;
        g_sMotorCurrent = 0;
        g_ulMotorPower = 0;

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
            // Calculate the phase current.
            //
            PHASE_CURRENT_CALC(g_pusPhaseMax[ulIdx], ulTemp, lTemp);

            //
            // Take the RMS reading.
            //
            lTemp = ((lTemp * 181) / 256);

            //
            // Save the value.
            //
            g_psPhaseCurrent[ulIdx] = lTemp;

            //
            // Reset the maximum phase current seen to zero to prepare for the
            // next cycle.
            //
            g_pusPhaseMax[ulIdx] = 0;
        }

        //
        // Average the RMS current of the three phases to get the RMS motor
        // current.
        //
        lTemp = (long)g_psPhaseCurrent[0];
        lTemp += (long)g_psPhaseCurrent[1];
        lTemp += (long)g_psPhaseCurrent[2];
        g_sMotorCurrent = (short)(lTemp / 3);
    }

    //
    // Loop through the three phases of the motor drive.
    //
    for(ulIdx = 0; ulIdx < 3; ulIdx++)
    {
        //
        // See if this ADC reading is larger than any previous ADC reading.
        //
        if(g_pusPhaseCurrentCount[ulIdx] > g_pusPhaseMax[ulIdx])
        {
            //
            // Save this ADC reading as the maximum.
            //
            g_pusPhaseMax[ulIdx] = g_pusPhaseCurrentCount[ulIdx];
        }
    }

    //
    // Save the current motor drive angle for the next set of samples.
    //
    g_ulPrevAngle = g_ulAngle;
}

//*****************************************************************************
//
// Handles the ADC sample sequence zero interrupt.
//
// This function is called when sample sequence zero asserts an interrupt.  It
// handles clearing the interrupt and processing any sequence overflow
// conditions.  Then, depending on the modulation scheme that is active, the
// appropriate sub-handler is called.
//
// \return None.
//
//*****************************************************************************
void
ADC0IntHandler(void)
{
    //
    // Get the time for this interrupt.
    //
    g_ulADC0Time = UIGetTicks();

    //
    // Clear the ADC interrupt.
    //
    HWREG(ADC0_BASE + ADC_O_ISC) = ADC_ISC_IN0;

    //
    // Process the sequence based on the ADC mode.
    //
    g_pfnADC0Handler();
}

//*****************************************************************************
//
// Configure the ADC sequence based on the ADC mode of operation.
//
// This will parse set the ADC mode of operation (based on motor drive
// parameters, and will reconfigure the ADC sequences accordingly.
//
// \return None.
//
//*****************************************************************************
void
ADCConfigure(void)
{
    //
    // Disable the ADC sequence and interrupts for safe reconfiguration of 
    // the ADC sequences.
    //
    IntDisable(INT_ADC0SS0);
    ADCIntDisable(ADC0_BASE, 0);
    ADCSequenceDisable(ADC0_BASE, 0);

    //
    // Ensure that this sequence is the highest priority sequence
    // (in the event that other ADC sequences are being used
    // elsewhere in the system).
    //
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PWM0, 0);

    //
    // If modulation type is sensorless, there is only one ADC
    // configuration available.
    //
    if(UI_PARAM_MODULATION == MODULATION_SENSORLESS)
    {
        //
        // Program the interrupt handler.
        //
        g_pfnADC0Handler = ADC0IntTrap;

        //
        // Program the sequence.
        //
        ADCSequenceStepConfigure(ADC0_BASE, 0, 0, PIN_VBEMFA);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 1, PIN_IPHASEB);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 2, PIN_VSENSE);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 3,
                                 ADC_CTL_END | ADC_CTL_IE | ADC_CTL_TS);
    }

    //
    // If modulation type is trapezoid, configure the appropriate sequence
    // for digital or linear hall sensors.
    //
    else if(UI_PARAM_MODULATION == MODULATION_TRAPEZOID)
    {
        if((UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_LINEAR) ||
           (UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_LINEAR_60))
        {
            //
            // Program the interrupt handler.
            //
            g_pfnADC0Handler = ADC0IntTrapLinear;

            //
            // Program the sequence.
            // (note:  the linear hall inputs share ADC pin with
            // BEMF inputs, and must be jumpered correctly on the board)
            //
            ADCSequenceStepConfigure(ADC0_BASE, 0, 0, PIN_IPHASEB);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 1, PIN_VBEMFA);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 2, PIN_VBEMFB);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 3, PIN_VBEMFC);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 4, PIN_VSENSE);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 5,
                                     ADC_CTL_END | ADC_CTL_IE | ADC_CTL_TS);
        }
        else
        {
            //
            // Program the interrupt handler.
            //
            g_pfnADC0Handler = ADC0IntTrap;

            //
            // Program the sequence.
            //
            ADCSequenceStepConfigure(ADC0_BASE, 0, 0, PIN_VBEMFA);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 1, PIN_IPHASEB);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 2, PIN_VSENSE);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 3,
                                     ADC_CTL_END | ADC_CTL_IE | ADC_CTL_TS);
        }
    }

    //
    // If modulation type is sinusoid, configure the appropriate sequence
    // for digital or linear hall sensors.
    //
    else if(UI_PARAM_MODULATION == MODULATION_SINE)
    {
        if((UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_LINEAR) ||
           (UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_LINEAR_60))
        {
            //
            // Program the interrupt handler.
            //
            g_pfnADC0Handler = ADC0IntSineLinear;

            //
            // Program the sequence.
            // (note:  the linear hall inputs share ADC pin with
            // BEMF inputs, and must be jumpered correctly on the board)
            //
            ADCSequenceStepConfigure(ADC0_BASE, 0, 0, PIN_IPHASEA);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 1, PIN_IPHASEB);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 2, PIN_IPHASEC);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 3, PIN_VBEMFA);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 4, PIN_VBEMFB);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 5, PIN_VBEMFC);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 6, PIN_VSENSE);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 7,
                                     ADC_CTL_END | ADC_CTL_IE | ADC_CTL_TS);
        }
        else
        {
            //
            // Program the interrupt handler.
            //
            g_pfnADC0Handler = ADC0IntSine;

            //
            // Program the sequence.
            // (note:  the linear hall inputs share ADC pin with
            // BEMF inputs, and must be jumpered correctly on the board)
            //
            ADCSequenceStepConfigure(ADC0_BASE, 0, 0, PIN_IPHASEA);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 1, PIN_IPHASEB);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 2, PIN_IPHASEC);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 3, PIN_VSENSE);
            ADCSequenceStepConfigure(ADC0_BASE, 0, 4,
                                     ADC_CTL_END | ADC_CTL_IE | ADC_CTL_TS);
        }
    }

    //
    // Here, there is some type of mistake, so just configure the ADC
    // sequences for "idle" mode.
    //
    else
    {
        //
        // Program the interrupt handler.
        //
        g_pfnADC0Handler = ADC0IntIdle;

        //
        // Program the sequence to read bus voltage and ambient
        // temperature.
        //
        ADCSequenceStepConfigure(ADC0_BASE, 0, 0, PIN_VSENSE);
        ADCSequenceStepConfigure(ADC0_BASE, 0, 1,
                                 ADC_CTL_END | ADC_CTL_IE | ADC_CTL_TS);
    }

    //
    // Reset the phase Back EMF voltage to 0.
    //
    g_ulPhaseBEMFVoltage = 0;
    g_ulPhaseBEMFCountMin = 1023;
    g_ulPhaseBEMFCountMax = 0;

    //
    // Reset the skip count default.
    //
    g_ucBEMFSkipCount = UI_PARAM_BEMF_SKIP_COUNT;

    //
    // Set the phase current index to phase B.
    //
    g_ucPhaseCurrentIndex = 1;

    //
    // Renable the ADC sequence code.
    //
    ADCSequenceEnable(ADC0_BASE, 0);
    ADCIntEnable(ADC0_BASE, 0);
    IntEnable(INT_ADC0SS0);
}

//*****************************************************************************
//
// Initializes the ADC control routines.
//
// This function initializes the ADC module and the control routines,
// preparing them to monitor currents and voltages on the motor drive.
//
// \return None.
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
    // Configure the primary ADC sequence based on the default settings.
    //
    ADCConfigure();

    //
    // Configure, enable, and prime sequence three to read the analog input
    // with a sofware trigger and lowest priority.
    //
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 3);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0,
                             ADC_CTL_END | ADC_CTL_IE | PIN_VANALOG);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCProcessorTrigger(ADC0_BASE, 3);
}

//*****************************************************************************
//
// Read the Analog input ADC value
//
// This function will read the analog input value from sequence three, and
// retrigger the sequence for the next read.
//
// \return The ADC count value for the analog input if one is available,
// otherwise, all ones.
//
//*****************************************************************************
unsigned long 
ADCReadAnalog(void)
{
    unsigned long ulData;

    //
    // If the FIFO is empty, set an invalid value.
    //
    if(HWREG(ADC0_BASE + ADC_O_SSFSTAT3) & ADC_SSFSTAT3_EMPTY)
    {
        ulData = 0xFFFFFFFF;
    }

    //
    // Read the value.
    //
    else
    {
        ulData = HWREG(ADC0_BASE + ADC_O_SSFIFO3);
    }

    //
    // Retrigger the ADC sequence.
    //
    ADCProcessorTrigger(ADC0_BASE, 3);

    //
    // Return the value.
    //
    return(ulData);
}

//*****************************************************************************
//
// Handles the ADC System Tick.
//
// This function is called by the system tick handler.  It's primary
// purpose is to reset the motor speed to 0 if no "Hall" edges have
// been detected for some period of time.
//
// \return None.
//
//*****************************************************************************
void
ADCTickHandler(void)
{
    //
    // See if an edge was seen during this tick period.
    //
    if(HWREGBITW(&g_ulADCFlags, FLAG_EDGE_BIT) == 1)
    {
        //
        // An edge was seen, so clear the flag so the next period can be
        // checked as well.
        //
        HWREGBITW(&g_ulADCFlags, FLAG_EDGE_BIT) = 0;

        //
        // There is nothing more to do here, so return.
        //
        return;
    }

    //
    // Check to see if time since the last edge is to large for the
    // Back EMF motor speed value.
    //
    if((UIGetTicks() - g_ulBEMFSpeedPrevious) > (SYSTEM_CLOCK / 5))
    {
        //
        // No edge was seen, so set the rotor speed to zero.
        //
        g_ulBEMFRotorSpeed = 0;

        //
        // Since the amount of time the rotor is stopped is indeterminate,
        // skip the first edge when the rotor starts rotating again.
        //
        HWREGBITW(&g_ulADCFlags, FLAG_SKIP_BIT) = 1;
    }

    //
    // Check to see if time since the last edge is to large for the
    // linear Hall sensor motor speed value.
    //
    if((UIGetTicks() - g_ulLinearSpeedPrevious) > (SYSTEM_CLOCK / 5))
    {
        //
        // No edge was seen, so set the rotor speed to zero.
        //
        g_ulLinearRotorSpeed = 0;

        //
        // Since the amount of time the rotor is stopped is indeterminate,
        // skip the first edge when the rotor starts rotating again.
        //
        HWREGBITW(&g_ulADCFlags, FLAG_SKIP_LINEAR_BIT) = 1;
    }
}

//*****************************************************************************
//
// Close the Doxyen group.
// @}
//
//*****************************************************************************
