//*****************************************************************************
//
// analog.c - Analog input driver for the Intelligent Display Module.
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
// This is part of revision 8555 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup analog_api
//! @{
//
//*****************************************************************************

#include "inc/hw_adc.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "drivers/analog.h"

//*****************************************************************************
//
// The current readings from the analog input channels.
//
//*****************************************************************************
short g_psAnalogValues[4];

//*****************************************************************************
//
// This structure describes the characteristics of the analog input channels.
//
//*****************************************************************************
static struct
{
    //
    // The trigger level for this channel.
    //
    unsigned short usLevel;

    //
    // The amount of hysteresis to apply to this channel.
    //
    char cHysteresis;

    //
    // The current debounced state of this channel.  If the MSB is set, then
    // the input is above the trigger level; otherwise, it is below the trigger
    // level.  The LSBs constitute a count that is used when the input level
    // crosses the trigger level; after the count reaches an appropriate value,
    // the input is considered to have crossed the trigger level.
    //
    unsigned char ucState;

    //
    // A pointer to the callback function that is called whenever this channel
    // is below the trigger level (in other words, this function is called on
    // every ADC interrupt when the channel is below the trigger level).
    //
    tAnalogCallback *pfnOnBelow;

    //
    // A pointer to the callback function that is called whenever this channel
    // is above the trigger level.
    //
    tAnalogCallback *pfnOnAbove;

    //
    // A pointer to the callback function that is called whenever this channel
    // transitions from below the trigger level to above the trigger level.
    //
    tAnalogCallback *pfnOnRisingEdge;

    //
    // A pointer to the callback function that is called whenever this channel
    // transitions from above the trigger level to below the trigger level.
    //
    tAnalogCallback *pfnOnFallingEdge;
}
g_psAnalogChannels[4];

//*****************************************************************************
//
//! Handles the ADC sample sequence two interrupt.
//!
//! This function is called when the ADC sample sequence two generates an
//! interrupt.  It will read the new ADC readings, perform debouncing on the
//! analog inputs, and call the appropriate callbacks based on the new
//! readings.
//!
//! \return None.
//
//*****************************************************************************
void
AnalogIntHandler(void)
{
    unsigned long ulIdx;

    //
    // Clear the ADC sample sequence interrupt.
    //
    HWREG(ADC0_BASE + ADC_O_ISC) = 1 << 2;

    //
    // Read the four samples from the ADC FIFO.
    //
    for(ulIdx = 0; ulIdx < 4; ulIdx++)
    {
        g_psAnalogValues[ulIdx] = HWREG(ADC0_BASE + ADC_O_SSFIFO2);
    }

    //
    // Loop through the four channels.
    //
    for(ulIdx = 0; ulIdx < 4; ulIdx++)
    {
        //
        // See if the debounced state of this channel is currently above the
        // trigger level.
        //
        if(g_psAnalogChannels[ulIdx].ucState & 0x80)
        {
            //
            // See if the current reading on this channel is below the trigger
            // level.
            //
            if(g_psAnalogValues[ulIdx] <
               (g_psAnalogChannels[ulIdx].usLevel -
                g_psAnalogChannels[ulIdx].cHysteresis))
            {
                //
                // Increment the state count.
                //
                g_psAnalogChannels[ulIdx].ucState++;

                //
                // See if the previous two ADC readings were also below the
                // trigger level.
                //
                if(g_psAnalogChannels[ulIdx].ucState == 0x83)
                {
                    //
                    // Three consecutive ADC readings were below the trigger
                    // level, so change the debounced state to below the
                    // trigger.
                    //
                    g_psAnalogChannels[ulIdx].ucState = 0x00;

                    //
                    // A falling edge was just detected, so call the falling
                    // edge callback if one exists.
                    //
                    if(g_psAnalogChannels[ulIdx].pfnOnFallingEdge)
                    {
                        g_psAnalogChannels[ulIdx].pfnOnFallingEdge(ulIdx);
                    }
                }
            }
            else
            {
                //
                // The current reading is above the trigger level, so reset the
                // state count.
                //
                g_psAnalogChannels[ulIdx].ucState = 0x80;
            }
        }
        else
        {
            //
            // The debounced state is currently below the trigger level, so see
            // if the current reading on this channel is above the trigger
            // level.
            //
            if(g_psAnalogValues[ulIdx] >
               (g_psAnalogChannels[ulIdx].usLevel +
                g_psAnalogChannels[ulIdx].cHysteresis))
            {
                //
                // Increment the state count.
                //
                g_psAnalogChannels[ulIdx].ucState++;

                //
                // See if the previous two ADC readings were also above the
                // trigger level.
                //
                if(g_psAnalogChannels[ulIdx].ucState == 0x03)
                {
                    //
                    // Three consecutive ADC readings were above the trigger
                    // level, so change the debounced state to above the
                    // trigger.
                    //
                    g_psAnalogChannels[ulIdx].ucState = 0x80;

                    //
                    // A rising edge was just detected, so call the rising edge
                    // callback if one exists.
                    //
                    if(g_psAnalogChannels[ulIdx].pfnOnRisingEdge)
                    {
                        g_psAnalogChannels[ulIdx].pfnOnRisingEdge(ulIdx);
                    }
                }
            }
            else
            {
                //
                // The current reading is below the trigger level, so reset the
                // state count.
                //
                g_psAnalogChannels[ulIdx].ucState = 0x00;
            }
        }

        //
        // If the debounced state is above the trigger level and there is a
        // callback function for the level being above the trigger, then call
        // it now.
        //
        if((g_psAnalogChannels[ulIdx].ucState & 0x80) &&
           (g_psAnalogChannels[ulIdx].pfnOnAbove))
        {
            g_psAnalogChannels[ulIdx].pfnOnAbove(ulIdx);
        }

        //
        // If the debounced state is below the trigger level, adn there is a
        // callback function for the level being below the trigger, then call
        // it now.
        //
        if(!(g_psAnalogChannels[ulIdx].ucState & 0x80) &&
           (g_psAnalogChannels[ulIdx].pfnOnBelow))
        {
            g_psAnalogChannels[ulIdx].pfnOnBelow(ulIdx);
        }
    }
}

//*****************************************************************************
//
//! Initializes the analog input driver.
//!
//! This function initializes the analog input driver, starting the sampling
//! process and disabling all channel callbacks.  Once called, the ADC2
//! interrupt will be asserted periodically; the AnalogIntHandler() function
//! should be called in response to this interrupt.  It is the application's
//! responsibility to install AnalogIntHandler() in the application's vector
//! table.
//!
//! \return None.
//
//*****************************************************************************
void
AnalogInit(void)
{
    unsigned long ulIdx;

    //
    // Clear out the analog channel information.
    //
    for(ulIdx = 0; ulIdx < 4; ulIdx++)
    {
        g_psAnalogChannels[ulIdx].usLevel = 0;
        g_psAnalogChannels[ulIdx].cHysteresis = 0;
        g_psAnalogChannels[ulIdx].ucState = 0;
        g_psAnalogChannels[ulIdx].pfnOnBelow = 0;
        g_psAnalogChannels[ulIdx].pfnOnAbove = 0;
        g_psAnalogChannels[ulIdx].pfnOnRisingEdge = 0;
        g_psAnalogChannels[ulIdx].pfnOnFallingEdge = 0;
    }

    //
    // Enable the peripherals used by the analog inputs.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    //
    // Configure the ADC sample sequence used to read the analog inputs.
    //
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_TIMER, 1);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 1, ADC_CTL_CH1);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 2, ADC_CTL_CH2);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 3,
                             ADC_CTL_CH3 | ADC_CTL_END | ADC_CTL_IE);
    ADCSequenceEnable(ADC0_BASE, 2);

    //
    // Enable the ADC sample sequence interrupt.
    //
    ADCIntEnable(ADC0_BASE, 2);
    IntEnable(INT_ADC0SS2);

    //
    // See if the ADC trigger timer has been configured, and configure it only
    // if it has not been configured yet.
    //
    if((HWREG(TIMER0_BASE + TIMER_O_CTL) & TIMER_CTL_TAEN) == 0)
    {
        //
        // Configure the timer to trigger the sampling of the analog inputs
        // every millisecond.
        //
        TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR |
                                     TIMER_CFG_A_PERIODIC |
                                     TIMER_CFG_B_PERIODIC));
        TimerLoadSet(TIMER0_BASE, TIMER_A, (SysCtlClockGet() / 1000) - 1);
        TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

        //
        // Enable the timer.  At this point, the analog inputs will be sampled
        // once per millisecond.
        //
        TimerEnable(TIMER0_BASE, TIMER_A);
    }
}

//*****************************************************************************
//
//! Sets the trigger level for an analog channel.
//!
//! \param ulChannel is the channel to modify.
//! \param usLevel is the trigger level for this channel.
//! \param cHysteresis is the hysteresis to apply to the trigger level for this
//! channel.
//!
//! This function sets the trigger level and hysteresis for an analog input
//! channel.  The hysteresis allows for filtering of noise on the analog input.
//! The actual level to transition from ``below'' the trigger level to
//! ``above'' the trigger level is the trigger level plus the hysteresis.
//! Similarly, the actual level to transition from ``above'' the trigger level
//! to ``below'' the trigger level is the trigger level minus the hysteresis.
//!
//! \return None.
//
//*****************************************************************************
void
AnalogLevelSet(unsigned long ulChannel, unsigned short usLevel,
               char cHysteresis)
{
    //
    // Check the arguments.
    //
    ASSERT(ulChannel < 4);
    ASSERT(usLevel < 1024);
    ASSERT((usLevel > cHysteresis) && ((usLevel + cHysteresis) < 1023));

    //
    // Save the trigger level and hysteresis for this channel.
    //
    g_psAnalogChannels[ulChannel].usLevel = usLevel;
    g_psAnalogChannels[ulChannel].cHysteresis = cHysteresis;

    //
    // Reset the state counter.
    //
    g_psAnalogChannels[ulChannel].ucState &= 0x80;
}

//*****************************************************************************
//
//! Sets the function to be called when the analog input is above the trigger
//! level.
//!
//! \param ulChannel is the channel to modify.
//! \param pfnOnAbove is a pointer to the function to be called whenever the
//! analog input is above the trigger level.
//!
//! This function sets the function that should be called whenever the analog
//! input is above the trigger level (in other words, while the analog input is
//! above the trigger level, the callback will be called every millisecond).
//! Specifying a function address of 0 will cancel a previous callback function
//! (meaning that no function will be called when the analog input is above the
//! trigger level).
//!
//! \return None.
//
//*****************************************************************************
void
AnalogCallbackSetAbove(unsigned long ulChannel, tAnalogCallback *pfnOnAbove)
{
    //
    // Check the arguments.
    //
    ASSERT(ulChannel < 4);

    //
    // Save the pointer to the callback function.
    //
    g_psAnalogChannels[ulChannel].pfnOnAbove = pfnOnAbove;
}

//*****************************************************************************
//
//! Sets the function to be called when the analog input is below the trigger
//! level.
//!
//! \param ulChannel is the channel to modify.
//! \param pfnOnBelow is a pointer to the function to be called whenever the
//! analog input is below the trigger level.
//!
//! This function sets the function that should be called whenever the analog
//! input is below the trigger level (in other words, while the analog input is
//! below the trigger level, the callback will be called every millisecond).
//! Specifying a function address of 0 will cancel a previous callback function
//! (meaning that no function will be called when the analog input is below the
//! trigger level).
//!
//! \return None.
//
//*****************************************************************************
void
AnalogCallbackSetBelow(unsigned long ulChannel, tAnalogCallback *pfnOnBelow)
{
    //
    // Check the arguments.
    //
    ASSERT(ulChannel < 4);

    //
    // Save the pointer to the callback function.
    //
    g_psAnalogChannels[ulChannel].pfnOnBelow = pfnOnBelow;
}

//*****************************************************************************
//
//! Sets the function to be called when the analog input transitions from below
//! to above the trigger level.
//!
//! \param ulChannel is the channel to modify.
//! \param pfnOnRisingEdge is a pointer to the function to be called when the
//! analog input transitions from below to above the trigger level.
//!
//! This function sets the function that should be called whenever the analog
//! input transitions from below to above the trigger level.  Specifying a
//! function address of 0 will cancel a previous callback function (meaning
//! that no function will be called when the analog input transitions from
//! below to above the trigger level).
//!
//! \return None.
//
//*****************************************************************************
void
AnalogCallbackSetRisingEdge(unsigned long ulChannel,
                            tAnalogCallback *pfnOnRisingEdge)
{
    //
    // Check the arguments.
    //
    ASSERT(ulChannel < 4);

    //
    // Save the pointer to the callback function.
    //
    g_psAnalogChannels[ulChannel].pfnOnRisingEdge = pfnOnRisingEdge;
}

//*****************************************************************************
//
//! Sets the function to be called when the analog input transitions from above
//! to below the trigger level.
//!
//! \param ulChannel is the channel to modify.
//! \param pfnOnFallingEdge is a pointer to the function to be called when the
//! analog input transitions from above to below the trigger level.
//!
//! This function sets the function that should be called whenever the analog
//! input transitions from above to below the trigger level.  Specifying a
//! function address of 0 will cancel a previous callback function (meaning
//! that no function will be called when the analog input transitions from
//! above to below the trigger level).
//!
//! \return None.
//
//*****************************************************************************
void
AnalogCallbackSetFallingEdge(unsigned long ulChannel,
                             tAnalogCallback *pfnOnFallingEdge)
{
    //
    // Check the arguments.
    //
    ASSERT(ulChannel < 4);

    //
    // Save the pointer to the callback function.
    //
    g_psAnalogChannels[ulChannel].pfnOnFallingEdge = pfnOnFallingEdge;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
