//*****************************************************************************
//
// CTS_HAL.c - Capacative Sense Library Hardware Abstraction Layer. 
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "drivers/CTS_structure.h"
#include "drivers/CTS_HAL.h"

//*****************************************************************************
//
// Function to get a capacitance reading from a single-pin RC-based capacitive
// sensor. This function assumes that the capacitive sensor is charged by a
// GPIO, and then discharged through a resistor to GND. The same GPIO used to
// perform the charging will be used as an input during discharge to sense when
// the capacitor's voltage has decreased to at least VIL. The function will
// execute several cycles of this, and report how long the process took in
// units of SysTick counts. This count should be proportional to the current
// capacitance of the sensor, and the resistance of the discharge resistor.
//
//*****************************************************************************
void
CapSenseSystickRC(const tSensor *group, unsigned long *counts)
{
    unsigned char ucIndex;

    //
    // Run an RC based capacitance measurement routine on each element in the
    // sensor array.
    //
    for(ucIndex = 0; ucIndex < (group->ucNumElements); ucIndex++)
    {
        //
        // Reset Systick to zero (probably not necessary for most cases, but we
        // want to make sure that Systick doesn't roll over.)
        //
        HWREG(NVIC_ST_CURRENT) = 0x0;

        //
        // Grab a count number from the capacitive sensor element.
        //
        counts[ucIndex] = 
            CapSenseElementSystickRC(group->Element[ucIndex]->ulGPIOPort,
                                     group->Element[ucIndex]->ulGPIOPin,
                                     group->ulNumSamples);
                                             
    }
}

//*****************************************************************************
//
//
//
//
//*****************************************************************************
unsigned long
CapSenseElementSystickRC(unsigned long ulGPIOPort, unsigned long ulGPIOPin,
                         unsigned long ulNumSamples)
{
    volatile unsigned long ulStartTime;
    volatile unsigned long ulEndTime;
    unsigned long ulIndex;
    unsigned long ulGPIODataReg;
    unsigned long ulGPIODirReg;

    //
    // Save off our GPIO information.
    //
    ulGPIODataReg = (ulGPIOPort + (GPIO_O_DATA + (ulGPIOPin << 2)));
    ulGPIODirReg = ulGPIOPort + GPIO_O_DIR;

    //
    // Grab the time
    //
    ulStartTime = HWREG(NVIC_ST_CURRENT);
    
    //
    // Loop until we have the requisite number of samples.
    //
    for(ulIndex = 0; ulIndex < ulNumSamples; ulIndex++)
    {
        //
        // Drive up.
        //
        //GPIOPinWrite(ulGPIOPort, ulGPIOPin, ulGPIOPin);
        HWREG(ulGPIODataReg) = 0xFF;

        //
        // Configure as input
        //
        //GPIOPinTypeGPIOInput(ulGPIOPort, ulGPIOPin);
        HWREG(ulGPIODirReg) = (HWREG(ulGPIODirReg) & ~(ulGPIOPin));

        //
        // Wait until the capacitor drains away to ground
        //
        //while(GPIOPinRead(ulGPIOPort, ulGPIOPin))
        while(HWREG(ulGPIODataReg))
        {
        }

        //
        // Configure as an output.
        //
        //GPIOPinTypeGPIOOutput(ulGPIOPort, ulGPIOPin);
        HWREG(ulGPIODirReg) = (HWREG(ulGPIODirReg) | ulGPIOPin);
    }
    
    //
    // Grab the time, and calculate the difference
    //
    ulEndTime = HWREG(NVIC_ST_CURRENT);

    //
    // Return the time difference
    //
    return(ulStartTime - ulEndTime);
}
