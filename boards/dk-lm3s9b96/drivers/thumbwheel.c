//*****************************************************************************
//
// thumbwheel.c - Functions to read the thumbwheel potentiometer.
//
// Copyright (c) 2009-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_adc.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"

//*****************************************************************************
//
//! \addtogroup thumbwheel_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// A pointer to the function to receive messages from the touch screen driver
// when events occur on the touch screen (debounced presses, movement while
// pressed, and debounced releases).
//
//*****************************************************************************
static void (*g_pfnThumbHandler)(unsigned short usThumbwheelmV);

//*****************************************************************************
//
// Handles the ADC interrupt for the thumbwheel.
//
// This function is called when the ADC sequence that samples the thumbwheel
// potentiometer has completed its acquisition.
//
// \return None.
//
//*****************************************************************************
void
ThumbwheelIntHandler(void)
{
    unsigned short ulVoltage;
    unsigned short usPotValue;

    //
    // Clear the ADC sample sequence interrupt.
    //
    HWREG(ADC0_BASE + ADC_O_ISC) = 1 << 2;

    //
    // Read the raw ADC value.
    //
    usPotValue = (unsigned short)HWREG(ADC0_BASE + ADC_O_SSFIFO2);

    //
    // Convert to millivolts.
    //
    ulVoltage = ((unsigned long)usPotValue * 3000) / 1024;

    //
    // Call the owner with the new value.
    //
    if(g_pfnThumbHandler)
    {
        g_pfnThumbHandler((unsigned short)ulVoltage);
    }
}

//*****************************************************************************
//
//! Initializes the ADC to read the thumbwheel potentiometer.
//!
//! This function configures ADC sequence 2 to sample the thumbwheel
//! potentiometer under processor trigger control.
//!
//! \return None.
//
//*****************************************************************************
void
ThumbwheelInit(void)
{
    //
    // Enable the peripherals used by the touch screen interface.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Configure the pin which the thumbwheel is attached to.
    //
    GPIOPinTypeADC(GPIO_PORTB_BASE, GPIO_PIN_4);

    //
    // Configure the ADC sample sequence used to read the thumbwheel
    //
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0,
                             ADC_CTL_CH10 | ADC_CTL_END | ADC_CTL_IE);
    ADCSequenceEnable(ADC0_BASE, 2);

    //
    // Enable the ADC sample sequence interrupt.
    //
    ADCIntEnable(ADC0_BASE, 2);
    IntEnable(INT_ADC0SS2);
}

//*****************************************************************************
//
//! Triggers the ADC to capture a single sample from the thumbwheel.
//!
//! This function triggers the ADC and starts the process of capturing a single
//! sample from the thumbwheel potentiometer.  Once the sample is available,
//! the user-supplied callback function, provided via a call to
//! ThumbwheelCallbackSet{}, will be called with the result.
//!
//! \return None.
//
//*****************************************************************************
void
ThumbwheelTrigger(void)
{
    ADCProcessorTrigger(ADC0_BASE, 2);
}

//*****************************************************************************
//
//! Sets the function that will be called when the thumbwheel has been sampled.
//!
//! \param pfnCallback is a pointer to the function to be called when
//! a thumbwheel sample is available.
//!
//! This function sets the address of the function to be called when a new
//! thumbwheel sample is available
//!
//! \return None.
//
//*****************************************************************************
void
ThumbwheelCallbackSet(void (*pfnCallback)(unsigned short usThumbwheelmV))
{
    //
    // Save the pointer to the callback function.
    //
    g_pfnThumbHandler = pfnCallback;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
