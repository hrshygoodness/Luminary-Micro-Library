//*****************************************************************************
//
// data-acq.c - Data acquisition functions used by the oscilloscope
//              application.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_adc.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "data-acq.h"

//*****************************************************************************
//
// Resources used by the data acquisition module.
//
//*****************************************************************************
#define DATA_ACQ_PERIPH_TIMER   SYSCTL_PERIPH_TIMER1
#define DATA_ACQ_PERIPH_ADC     SYSCTL_PERIPH_ADC0
#define DATA_ACQ_PERIPH_GPIO    SYSCTL_PERIPH_GPIOE

#define DATA_ACQ_TIMER_BASE     TIMER1_BASE
#define DATA_ACQ_ADC_BASE       ADC0_BASE
#define DATA_ACQ_GPIO_BASE      GPIO_PORTE_BASE

//
// The GPIO pins that are muxed to the ADC0/1/2/3 inputs.
//
#define DATA_ACQ_GPIO_PINS      (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | \
                                 GPIO_PIN_7)

//*****************************************************************************
//
// This array contains all fixed hardware sampling rates supported.
// Rates below 125KHz are supported using a timer to trigger the ADC capture
// so all values below this frequency can be supported.
//
//*****************************************************************************
#define NUM_HARDWARE_SAMPLE_RATES \
                                4
static unsigned long g_pulDataAcqSampleRates[NUM_HARDWARE_SAMPLE_RATES] =
{
    1000000,
     500000,
     250000,
     125000
};

//*****************************************************************************
//
// Internal type and label definitions
//
//*****************************************************************************

//*****************************************************************************
//
// Instance data for a single capture channel.
//
//*****************************************************************************
typedef struct
{
    //
    // The number of samples to capture per trigger event.
    //
    unsigned long ulMaxSamples;

    //
    // The number of samples remaining to be captured for this trigger event.
    //
    unsigned long ulSamplesToCapture;

    //
    // The trigger offset relative to the start of capture in samples.  This
    // value will be 0 if the trigger is at the start of capture or
    // ulMaxSamples if the trigger is at the end of the buffer.
    //
    unsigned long ulTrigPos;

    //
    // The write index indicates the next entry that will be written with
    // a captured ADC sample.
    //
    unsigned long ulWriteIndex;

    //
    // The sample rate used to capture the data that this structure refers
    // to.
    //
    unsigned long ulSampleRate;

    //
    // A pointer to the start of the channel's sample capture buffer.
    //
    unsigned short *pusSampleBuffer;

    //
    // The current state of the channel.
    //
    tDataAcqState eState;

    //
    // Flag indicating whether or not we have captured enough data into the
    // buffer to wrap.
    //
    tBoolean bWrapped;

    //
    // Flag indicating whether we are working in single or dual channel mode
    //
    tBoolean bDualMode;

    //
    // Flag indicating whether we ahve just captured a channel 1 or channel 2
    // sample.
    //
    tBoolean bTriggerCheck;

    //
    // Flag indicating the sample order.
    //
    tBoolean bChannel1First;
}
tChannelInst;

//*****************************************************************************
//
// Number of words to use in the temporary buffer used when reading data
// from the ADC.  Although the largest ADC FIFO is only 8 words deep, we add
// some padding here to allow us to read more samples that may become
// available while the ISR is running.
//
//*****************************************************************************
#define ADC_SAMPLE_BUFFER_SIZE  16

//*****************************************************************************
//
// Globals related to triggering and sample capture rate.
//
//*****************************************************************************
static unsigned long g_ulSampleRate;
static tBoolean g_bTimerTrigger;
static unsigned short g_usTriggerLevel;
static tBoolean g_bTriggerStateLast;
static unsigned long g_ulTriggerPos;
static tTriggerType g_eTrigger;
static tTriggerType g_eUserTrigger;
static volatile tBoolean g_bTriggered;
static tBoolean g_bDualMode;
static tBoolean g_bAbortCapture;
static tBoolean g_bTriggerChannel1;

//*****************************************************************************
//
// Channel instance data structure.
//
//*****************************************************************************
static tChannelInst g_sChannelInst;

//*****************************************************************************
//
// Useful macro which converts a frequency in Hz to the number of system
// clock ticks that, when set as a general purpose timer period, will generate
// timeouts at the required frequency.
//
//*****************************************************************************
#define HZ_TO_TICKS(hz)         (ROM_SysCtlClockGet() / (hz))

//*****************************************************************************
//
// Private functions.
//
//*****************************************************************************

//*****************************************************************************
//
// \internal
//
// Resets all pointers related to a capture buffer in preparation for a new
// capture request.
//
// This function configures the capture buffer for a single channel, setting
// all internal pointers and counts to the state used to indicate an empty
// buffer.  Note that it is assumed the client has ensured that eChannel is not
// CHANNEL_BOTH prior to calling this function.
//
// \return None.
//
//*****************************************************************************
static void
ResetBufferPointers()
{
    g_sChannelInst.bWrapped = false;
    g_sChannelInst.ulWriteIndex = 0;
    g_sChannelInst.ulSamplesToCapture = g_sChannelInst.ulMaxSamples;
    g_sChannelInst.bTriggerCheck = true;
    g_sChannelInst.bDualMode = g_bDualMode;
}

//*****************************************************************************
//
// \internal
//
// Calculate the index of the trigger sample in capture buffer.
//
// \param pInst points to the instance data whose sample buffer trigger
// pointer is to be calculated.
//
// This function calculates the index of the channel 1 data sample
// that corresponds to the point where the trigger event was detected.
//
// If the instance represents either an idle or non-triggered capture, the
// returned value will be 0xFFFFFFFF to indicate that no trigger event has yet
// been detected.
//
// If the instance represents a completed capture or one that is triggered and
// in progress, the returned index will represent the channel 1 sample in the
// buffer at which the trigger event was detected.
//
// \return The index of the sample in the ring buffer at which a trigger
// event was detected.
//
//*****************************************************************************
static unsigned long
GetTriggerIndex(tChannelInst *pInst)
{
    long lTrig;

    //
    // If we have not triggered, return NULL.
    //
    if((pInst->eState != DATA_ACQ_COMPLETE) &&
       (pInst->eState != DATA_ACQ_TRIGGERED))
    {
        lTrig = 0xFFFFFFFF;
    }
    else
    {
        //
        // We are triggered so calculate the trigger index.
        //
        lTrig = (long)pInst->ulWriteIndex;
        lTrig -= ((pInst->ulMaxSamples - pInst->ulTrigPos) -
                  pInst->ulSamplesToCapture);

        //
        // Handle the wrap case
        //
        if(lTrig < 0)
        {
            lTrig += pInst->ulMaxSamples;
        }
    }

    //
    // Return the calculated index to the caller.
    //
    return((unsigned long)lTrig);
}

//*****************************************************************************
//
// \internal
//
// Determine the start of the data samples we want to keep based on the
// outstanding sample count and trigger offset in the supplied instance
// structure.
//
// \param pInst points to the instance data whose sample buffer start index
// is to be calculated.
//
// This function calculates the index of the first sample of data in the
// channel sample buffer that we want to keep after a trigger event has been
// generated.
//
// If the instance represents either an idle or non-triggered capture, the
// returned index will be the start of the buffer if data has not wrapped
// or the current write index if it has (since the write index will then
// point to the oldest sample read).
//
// If the instance represents a completed capture or one that is triggered and
// in progress, the returned start index will represent the first data
// that is to be saved prior to the trigger point based on the trigger offset
// stored in the instance data structure.
//
// \return The index of the first valid sample value in the ring buffer.
//
//*****************************************************************************
static unsigned long
GetStartIndex(tChannelInst *pInst)
{
    unsigned long ulStart;

    //
    // If we have not triggered, return the start of the buffer if it has not
    // yet wrapped, otherwise return the current write pointer since this
    // will point to the oldest sample still in the buffer.
    //
    if((pInst->eState != DATA_ACQ_COMPLETE) &&
       (pInst->eState != DATA_ACQ_TRIGGERED))
    {
        //
        // Has the buffer wrapped yet?
        //
        if(pInst->bWrapped == false)
        {
            //
            // No - the first valid sample (if there are any) must be at the
            // start of the buffer.
            //
            ulStart = 0;
        }
        else
        {
            //
            // We have wrapped.  The oldest sample is at the current write
            // position.
            //
            ulStart = pInst->ulWriteIndex;
        }
    }
    else
    {
        //
        // We are triggered so calculate a pointer to the first valid data
        // in the buffer based on the trigger point and trigger offset.
        //
        ulStart = pInst->ulWriteIndex;
        ulStart += pInst->ulSamplesToCapture;

        //
        // Handle the wrap case
        //
        if(ulStart >= pInst->ulMaxSamples)
        {
            ulStart -= pInst->ulMaxSamples;
        }
    }

    //
    // Return the calculated start index to the caller.
    //
    return(ulStart);
}

//*****************************************************************************
//
// \internal
//
// Handles interrupts from the GPIO peripheral used to signal capture aborts.
//
// This function is used to abort a data acquisition capture request which has
// not triggered.  At the highest capture rates, the ADC interrupt handler
// uses all the available CPU bandwidth so no cycles are available to handle
// the user interface.  If it so happens that the trigger level is set such
// that no trigger event is detected, this can cause the application to lock
// up.  To prevent this, we set the direction pushbutton GPIOs to interrupt at
// very high priority and force a trigger to occur if the interrupt fires.
// This allows a user to abort a capture request and give the user interface
// handling code an opportunity to run before another capture is initiated.
//
// \return None.
//
//*****************************************************************************
void
DataAcquisitionAbortIntHandler(void)
{
    //
    // Read and clear the interrupt sources.
    //
    GPIOPinIntClear(ABORT_GPIO_BASE, GPIOPinIntStatus(ABORT_GPIO_BASE, true));

    //
    // Flag the fact that an abort occurred and force the system to trigger
    // by temporarily setting the trigger type to TRIGGER_ALWAYS.
    //
    g_eTrigger = TRIGGER_ALWAYS;
    g_bAbortCapture = true;

    //
    // Now that we have signalled the abort, turn off the interrupt.  It will
    // be turned on again next time a capture request is made.
    //
    GPIOPinIntDisable(ABORT_GPIO_BASE, ABORT_GPIO_PINS);
}

//*****************************************************************************
//
// \internal
//
// Handles all interrupts from ADC Sequence 0.
//
// This function is responsible for reading ADC samples from sequence 0's
// FIFO and storing them in the sample buffer.  The function is also
// responsible for detecting trigger events and updating the state as
// required when a trigger is detected.
//
// This handler supports both single and dual channel capture.  In dual
// channel mode, samples from each channel are interleaved in the capture
// buffer with the channel 1 sample leading the channel 2 sample.
//
// Trigger event detection is performed every two samples even when capturing
// only one channel of data.  This is due primarily to the fact that the
// trigger search code path through the ISR is longer than a single sample
// period at the highest supported sampling rates and, if we check for
// trigger on every sample, the ADC FIFO will overflow.  As an aside, if you
// duplicate this ISR and remove the small amount of code specific to the
// dual channel handling (the bTriggerCheck toggling and checks), you can
// successfully check for trigger on every sample at 1Ms/S.
//
// \note The timing in the DATA_ACQ_TRIGGER_SEARCH state is right on the
// hairy edge.  If you even think about adding any code to this function, you
// can expect ADC FIFO overflows when sampling at 1Ms/S.
//
// \return None.
//
//*****************************************************************************
void
DataAcquisitionADCSeq0IntHandler(void)
{
    tBoolean bTriggerState;

    //
    // Clear the ADC interrupt
    //
    HWREG(DATA_ACQ_ADC_BASE + ADC_O_ISC) = ADC_ISC_IN0;

    //
    // We keep processing data from the ADC as long as the FIFO has data to
    // process.  This can lead to an extremely long ISR but, if we don't do
    // this, we will overflow the ADC FIFO when running at maximum rates.
    //
    do
    {
        switch(g_sChannelInst.eState)
        {
            //
            // We are triggered so just copy the new samples straight into the
            // ring buffer.
            //
            case DATA_ACQ_TRIGGERED:
            {
                //
                // Read samples from the FIFO until it is empty.  We do this
                // using local code rather than a call to DriverLib to save
                // overhead.  At high sample rates, we have a very short time
                // to do this before we get an overflow.
                //
                while(!(HWREG(DATA_ACQ_ADC_BASE + ADC_O_SSFSTAT0) &
                        ADC_SSFSTAT0_EMPTY) &&
                        g_sChannelInst.ulSamplesToCapture)
                {
                    //
                    // Read the FIFO and copy it to the destination.
                    //
                    g_sChannelInst.pusSampleBuffer[
                        g_sChannelInst.ulWriteIndex++] =
                                (unsigned short)((HWREG(DATA_ACQ_ADC_BASE +
                                                     ADC_O_SSFIFO0)) & 0x3FF);

                    //
                    // Toggle the phase flag so that we know which sample we
                    // are reading next.
                    //
                    g_sChannelInst.bTriggerCheck ^= 1;

                    //
                    // Decrement the count of samples still to be read.
                    //
                    g_sChannelInst.ulSamplesToCapture--;

                    //
                    // Wrap the write index if necessary
                    //
                    if(g_sChannelInst.ulWriteIndex ==
                       g_sChannelInst.ulMaxSamples)
                    {
                        g_sChannelInst.ulWriteIndex = 0;
                    }
                }
                break;
            }

            //
            // We are building up samples prior to looking for a trigger so
            // just copy samples from the FIFO to the destination until
            // buffering is complete.
            //
            case DATA_ACQ_BUFFERING:
            {
                //
                // Read samples from the FIFO until it is empty.
                //
                while(!(HWREG(DATA_ACQ_ADC_BASE + ADC_O_SSFSTAT0) &
                        ADC_SSFSTAT0_EMPTY) &&
                        (g_sChannelInst.ulSamplesToCapture > 0))
                {
                    //
                    // Read the FIFO and copy it to the destination.
                    //
                    g_sChannelInst.pusSampleBuffer[
                       g_sChannelInst.ulWriteIndex] =
                         (unsigned short)((HWREG(DATA_ACQ_ADC_BASE +
                                                 ADC_O_SSFIFO0)) & 0x3FF);

                    //
                    // Set the current trigger condition based on the sample
                    // just read if that sample was from channel 1.
                    //
                    if(g_sChannelInst.bTriggerCheck)
                    {
                        g_bTriggerStateLast =
                          (g_sChannelInst.pusSampleBuffer[
                           g_sChannelInst.ulWriteIndex] <
                           g_usTriggerLevel) ? true : false;
                    }

                    //
                    // Toggle the phase flag so that we know which sample we
                    // are reading next.
                    //
                    g_sChannelInst.bTriggerCheck ^= 1;

                    //
                    // Update our write pointer.
                    //
                    g_sChannelInst.ulWriteIndex++;

                    //
                    // Decrement the count of samples still to be read.
                    //
                    g_sChannelInst.ulSamplesToCapture--;

                    //
                    // We don't need to deal with wrapping of the write index
                    // within this loop since this would imply that the trigger
                    // position was set to some index outside the buffer and
                    // this is illegal.  We do check once on exit to cater for
                    // the case where the trigger position is right at the
                    // end of the buffer, though.
                    //

                }

                //
                // If we finished buffering data, switch to active trigger
                // search and reset the number of samples to read to the
                // maximum, indicating that we should continue reading forever
                // until we find a trigger.  Note that we need to check for
                // write pointer wrap here too since, if the trigger position
                // is set to the very end of the buffer, we will wrap just as
                // we exit buffering state.
                //
                if(g_sChannelInst.ulSamplesToCapture == 0)
                {
                    //
                    // Wrap the write index if necessary
                    //
                    if(g_sChannelInst.ulWriteIndex ==
                       g_sChannelInst.ulMaxSamples)
                    {
                        g_sChannelInst.ulWriteIndex = 0;
                        g_sChannelInst.bWrapped = true;
                    }

                    //
                    // Set up for the next state - active trigger search.
                    //
                    g_sChannelInst.ulSamplesToCapture =
                        g_sChannelInst.ulMaxSamples;
                    g_sChannelInst.eState = DATA_ACQ_TRIGGER_SEARCH;

                    break;
                }
            }
            break;

            //
            // We are actively searching for a trigger event in the sample
            // stream.  This is the state with the most processing required
            // and, hence, is the critical loop in the acquisition code.
            //
            case DATA_ACQ_TRIGGER_SEARCH:
            {
                while(!(HWREG(DATA_ACQ_ADC_BASE + ADC_O_SSFSTAT0) &
                        ADC_SSFSTAT0_EMPTY))
                {
                    //
                    // Read the FIFO and copy it to the destination.
                    //
                    g_sChannelInst.pusSampleBuffer[
                        g_sChannelInst.ulWriteIndex] =
                            (unsigned short)((HWREG(DATA_ACQ_ADC_BASE +
                                              ADC_O_SSFIFO0)) & 0x3FF);

                    //
                    // Set the current trigger condition based on the sample
                    // just read if this was a channel 1 sample.
                    //
                    if(g_sChannelInst.bTriggerCheck)
                    {
                        bTriggerState =
                          (g_sChannelInst.pusSampleBuffer[
                           g_sChannelInst.ulWriteIndex] <
                           g_usTriggerLevel) ? true : false;

                        //
                        // Because we ensure that at least 1 sample is read in
                        // DATA_ACQ_BUFFERING state, we guarantee that
                        // g_bTriggerStateLast is set by this time.
                        //
                        // We consider the trigger state from the last sample
                        // and the current one.  The state flag is true if the
                        // sample in question is below the trigger level and
                        // false otherwise.  For any trigger to occur, the two
                        // flags must be different.  For rising and falling
                        // edge triggers, the actual state of one of the flags
                        // is also important.
                        //
                        if(bTriggerState ^ g_bTriggerStateLast)
                        {
                            //
                            // The flags are different so we may have a
                            // trigger event.
                            //
                            switch(g_eTrigger)
                            {
                                //
                                // If this is a rising edge, the previous state
                                // must have been below the trigger level.  If
                                // this is the case then g_bTriggerStateLast
                                // will be true.
                                //
                                case TRIGGER_RISING:
                                    g_bTriggered = g_bTriggerStateLast;
                                break;

                                //
                                // If this is a falling edge, the current state
                                // must be below the trigger level.  If this
                                // is the case then bTriggerState will be true.
                                //
                                case TRIGGER_FALLING:
                                    g_bTriggered = bTriggerState;
                                break;

                                //
                                // If we are triggering on level alone, the
                                // direction of the trigger level crossing is
                                // irrelevant so we always trigger on a
                                // difference in the two states.
                                //
                                case TRIGGER_LEVEL:
                                    g_bTriggered = true;
                                break;

                                //
                                // Just to keep the compiler happy.  We are
                                // never in this state when using
                                // TRIGGER_ALWAYS.
                                //
                                case TRIGGER_ALWAYS:
                                    break;
                            }
                        }
                        else
                        {
                            //
                            // Although we should never get into this state
                            // when using TRIGGER_ALWAYS, we allow the
                            // client to change the trigger mode while
                            // capture is ongoing so we need to handle it here
                            // too.
                            //
                            if(g_eTrigger == TRIGGER_ALWAYS)
                            {
                                g_bTriggered = true;
                            }
                        }

                        //
                        // Remember the trigger state for this sample since
                        // we will need it next time round.
                        //
                        g_bTriggerStateLast = bTriggerState;
                    }

                    //
                    // Toggle our phase flag.
                    //
                    g_sChannelInst.bTriggerCheck ^= 1;

                    //
                    // Update our write index.
                    //
                    g_sChannelInst.ulWriteIndex++;

                    //
                    // Wrap the write index if necessary
                    //
                    if(g_sChannelInst.ulWriteIndex ==
                       g_sChannelInst.ulMaxSamples)
                    {
                        g_sChannelInst.ulWriteIndex = 0;
                        g_sChannelInst.bWrapped = true;
                    }

                    //
                    // If we triggered, set the new state appropriately
                    //
                    if(g_bTriggered)
                    {
                        g_sChannelInst.eState = DATA_ACQ_TRIGGERED;

                        //
                        // In this case, we need to capture 1 more sample than
                        // may be expected.  Trigger is detected on the channel
                        // 1 sample, before we read the matching channel 2
                        // sample.  If we don't grab an extra sample, we end
                        // up with an uneven number of samples for each
                        // channel.
                        //
                        g_sChannelInst.ulSamplesToCapture =
                            (g_sChannelInst.ulMaxSamples -
                             g_sChannelInst.ulTrigPos) + 1;

                        break;
                    }
                }
            }
            break;

            //
            // Just to keep the compiler happy.
            //
            default:
            {
                return;
            }
        }
    }
    while(!(HWREG(DATA_ACQ_ADC_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY) &&
          g_sChannelInst.ulSamplesToCapture);

    //
    // Determine whether or not we have completed capturing all the
    // required data.
    //
    if(g_sChannelInst.ulSamplesToCapture == 0)
    {
        //
        // Disable the ADC sequence.
        //
        ADCSequenceDisable(DATA_ACQ_ADC_BASE, 0);

        //
        // Set the channel state to indicate completion.
        //
        g_sChannelInst.eState = DATA_ACQ_COMPLETE;

        //
        // Turn off the abort interrupt
        //
        GPIOPinIntDisable(ABORT_GPIO_BASE, ABORT_GPIO_PINS);
    }
}

//*****************************************************************************
//
// \internal
//
// Clear global state variables used to monitor triggering.
//
// This function sets the global state variables used in trigger detection
// to indicate that capture has not been triggered.
//
// \return None.
//
//*****************************************************************************
static void
ClearTrigger(void)
{
    //
    // Revert to the user's choice of trigger mode.  This is necessary since
    // the abort handler causes the capture to complete by forcing the
    // trigger mode temporarily to TRIGGER_ALWAYS.
    //
    g_eTrigger = g_eUserTrigger;

    if(g_eTrigger == TRIGGER_ALWAYS)
    {
        //
        // If no trigger is required (TRIGGER_ALWAYS) just set things up
        // so that we are already triggered.
        //
        g_bTriggered = true;

        //
        // Mark the channel as already triggered.
        //
        g_sChannelInst.eState = DATA_ACQ_TRIGGERED;
        g_sChannelInst.ulSamplesToCapture = g_sChannelInst.ulMaxSamples;
    }
    else
    {
        //
        // For all other trigger modes, we need to set the intial state
        // and sample count appropriately.
        //

        //
        // Check that the trigger position set is within the capture buffer
        // and, if not, adjust it so that it is.
        //
        if(g_ulTriggerPos >= g_sChannelInst.ulMaxSamples)
        {
            //
            // The trigger position is outside the buffer.  Move it to the
            // very end of the buffer instead.  The buffer size must be
            // even (enforced in DataAcquisitionSetCaptureBuffer).
            //
            g_ulTriggerPos = g_sChannelInst.ulMaxSamples -
                             (g_sChannelInst.bDualMode ? 2 : 1);
        }
        else
        {
            //
            // The trigger position must be on the second or later
            // sample since we need at least 1 sample before we can
            // determine if the trigger level has been crossed.  Adjust
            // the position if required to ensure this constraint is met
            // in dual channel mode.
            //
            if(g_sChannelInst.bDualMode && (g_ulTriggerPos == 1))
            {
                //
                // Move the trigger position to immediately after the first
                // set of channel 1 and channel 2 samples.
                //
                g_ulTriggerPos = 2;
            }
        }

        //
        // If in dual mode, ensure that the trigger position is on a
        // channel 1 sample position.  These are all even indices.
        //
        if(g_sChannelInst.bDualMode)
        {
            g_sChannelInst.ulTrigPos = g_ulTriggerPos & ~1;
        }
        else
        {
            //
            // In single channel mode, the trigger point can be anywhere
            // within the buffer.
            //
            g_sChannelInst.ulTrigPos = g_ulTriggerPos;
        }

        //
        // Remember that we are not triggered.
        //
        g_bTriggered = false;

        //
        // Set the initial state depending upon the trigger position.
        //
        g_sChannelInst.eState = DATA_ACQ_BUFFERING;
        g_sChannelInst.ulSamplesToCapture = g_sChannelInst.ulTrigPos;
    }

    //
    // Take a snapshot of the rate being used for this capture.  We need to
    // associate this with the capture itself to allow the application to
    // change the rate between captures without affecting data that has
    // already been sampled.
    //
    g_sChannelInst.ulSampleRate = g_ulSampleRate;
}

//*****************************************************************************
//
// \internal
//
// Prepare the ADC and channel instance data prior to starting a new capture.
//
// This function prepares the ADC and channel instance data for the capture of
// another block of data.  After this function returns, the client need only
// enable the relevant ADC sequence to start the capture process.
//
// \return None.
//
//*****************************************************************************
static void
PrepareChannelForCapture()
{
    unsigned long pulData[8];

    //
    // Disable ADC sequence interrupts.
    //
    ADCIntDisable(DATA_ACQ_ADC_BASE, 0);

    //
    // Disable the ADC sequence itself.
    //
    ADCSequenceDisable(DATA_ACQ_ADC_BASE, 0);

    //
    // Flush any old data from the ADC sequence FIFO.
    //
    while(ADCSequenceDataGet(DATA_ACQ_ADC_BASE, 0, pulData));

    //
    // Clear any pending ADC sequence interrupts.
    //
    ADCIntClear(DATA_ACQ_ADC_BASE, 0);

    //
    // Clear any outstanding overflow or underflow errors.
    //
    ADCSequenceUnderflowClear(DATA_ACQ_ADC_BASE, 0);
    ADCSequenceOverflowClear(DATA_ACQ_ADC_BASE, 0);

    //
    // Reset the capture buffer to remove any old data.
    //
    ResetBufferPointers();

    //
    // Clear the trigger for this channel.  This will set the initial
    // channel state and sample counter depending upon the trigger mode and
    // position.  It will also update the capture rate snapshot in the
    // channel instance data.
    //
    ClearTrigger();

    //
    // Record the sample order so that the display code knows which sample
    // is for which channel.
    //
    g_sChannelInst.bChannel1First = g_bTriggerChannel1;

    //
    // Enable the abort function.
    // Remember to clear any pending interrupts which may have occurred
    // since the last time we armed the abort mechanism.  If we don't do
    // this, typically we get an immediate abort resulting in one wasted
    // capture cycle.
    //
    g_bAbortCapture = false;
    GPIOPinIntClear(ABORT_GPIO_BASE, ABORT_GPIO_PINS);
    GPIOPinIntEnable(ABORT_GPIO_BASE, ABORT_GPIO_PINS);

    //
    // Enable the ADC sequence interrupt.
    //
    ADCIntEnable(DATA_ACQ_ADC_BASE, 0);
}

//*****************************************************************************
//
// \internal
//
// Sets up the ADC sequences used given the sample timing method specified.
//
// \param bTimerTrigger should be set to \b true when the sample capture rate
// is to be triggered by a timer or \b false if capture is to proceed at
// the rate defined by a previous call to SysCtlADCSpeedSet().
// \param ulSampleRate is the desired sample capture rate in hertz.
// \param bDualMode is set to \b true if we are to capture data from two
// channels or \b false for single channel capture.
//
// This function configures the ADC sequences appropriately for capture at
// the specified rate and using the sample timing method specified by
// parameter \e bTimerTrigger.  In cases where \e bTimerTrigger is \b false, it
// is assumed that the caller has previously called SysCtlADCSpeedSet() to
// set the desired sample rate.
//
// \return None.
//
//*****************************************************************************
static void
ConfigureADCSequences(tBoolean bTimerTrigger, unsigned long ulSampleRate,
                      tBoolean bDualMode, tBoolean bChannel1Trigger)
{
    unsigned long ulSample1;
    unsigned long ulSample2;

    //
    // Fix up the ADC inputs depending upon the channel being used to trigger.
    // The trigger channel is always the first sample in the pair when doing
    // dual channel capture.
    //
    ulSample1 = bChannel1Trigger ? ADC_CTL_CH0 : ADC_CTL_CH1;
    ulSample2 = bChannel1Trigger ? ADC_CTL_CH1 : ADC_CTL_CH0;

    //
    // Disable the ADC sequences before we configure them.
    //
    ADCSequenceDisable(DATA_ACQ_ADC_BASE, 0);
    ADCSequenceDisable(DATA_ACQ_ADC_BASE, 1);

    //
    // Disable hardware oversampling.
    //
    ADCHardwareOversampleConfigure(DATA_ACQ_ADC_BASE, 0);

    //
    // Configure the sequence we will be using, setting the sequence to
    // trigger either using timer or always (when the rate is controlled by
    // the ADC clock rather than a timer trigger).
    //
    ADCSequenceConfigure(DATA_ACQ_ADC_BASE, 0, (bTimerTrigger ?
                         ADC_TRIGGER_TIMER : ADC_TRIGGER_ALWAYS), 0);

    //
    // We don't use the other sequences so set them to something benign and low
    // priority.
    //
    ADCSequenceConfigure(DATA_ACQ_ADC_BASE, 1, ADC_TRIGGER_PROCESSOR, 1);
    ADCSequenceConfigure(DATA_ACQ_ADC_BASE, 2, ADC_TRIGGER_PROCESSOR, 2);
    ADCSequenceConfigure(DATA_ACQ_ADC_BASE, 3, ADC_TRIGGER_PROCESSOR, 3);

    //
    // Configure the sequence.  We use sequence 1 and, in dual channel mode,
    // set it up to sample the two different inputs one after the other.  In
    // single channel mode, we merely use the same input for all samples.
    //
    if(bTimerTrigger)
    {
        //
        // If we are using the timer to control capture rate, we grab a single
        // sample in the sequence and interrupt immediately.  We can't grab
        // multiple samples since the pacing is controlled by the ADC clock
        // rather than the timer in this case.  The timer event triggers the
        // sequence not each individual sample capture within it.
        //
        // Note that, since we are using differential input mode, we use
        // ADC input 0 for channel 1 and ADC input 1 for channel 2.  ADC pins
        // 0 and 1 correspond to the first differential input and ADC inputs
        // 2 and 3 correspond to the second differential input.
        //

        if(bDualMode)
        {
            //
            // In dual mode, we capture 2 samples back-to-back when using the
            // timer to trigger capture.  This actually makes life somewhat
            // more tricky since we can't assume that the final interleaved
            // samples in the buffer are each offset by one sample period.  If
            // we are using the timer, they are offset by 1 ADC clock period
            // instead.
            //
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 0,
                                     (ADC_CTL_D | ulSample1));
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 1,
                                     (ADC_CTL_D | ulSample2 | ADC_CTL_IE |
                                      ADC_CTL_END));
        }
        else
        {
            //
            // In single channel mode we capture a single sample per timer
            // interrupt.
            //
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 0,
                                     (ADC_CTL_D | ulSample1 | ADC_CTL_IE |
                                      ADC_CTL_END));
        }
    }
    else
    {
        //
        // If we are not using the timer event but are capturing at the raw
        // ADC clock rate, we set the sequence up to capture 4 fewer samples
        // than the FIFO will hold.  Since we are triggering all the time,
        // this gives us a small buffer in case interrupt latency means we
        // don't read a sample before the next sample conversion completes.
        //

        //
        // Sequence 0 has an 8 entry FIFO, hence we configure 4 sequence steps.
        //
        if(bDualMode)
        {
            //
            // Dual channel mode.  Capture interleaved samples from the two
            // inputs.
            //
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 0,
                                     (ADC_CTL_D | ulSample1));
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 1,
                                     (ADC_CTL_D | ulSample2));
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 2,
                                     (ADC_CTL_D | ulSample1));
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 3,
                                     (ADC_CTL_D | ulSample2 | ADC_CTL_IE |
                                      ADC_CTL_END));
        }
        else
        {
            //
            // Single channel mode.  Capture everything from a single input.
            //
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 0,
                                     (ADC_CTL_D | ulSample1));
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 1,
                                     (ADC_CTL_D | ulSample1));
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 2,
                                     (ADC_CTL_D | ulSample1));
            ADCSequenceStepConfigure(DATA_ACQ_ADC_BASE, 0, 3,
                                     (ADC_CTL_D | ulSample1 | ADC_CTL_IE |
                                      ADC_CTL_END));
        }
    }

    //
    // If we have been asked to trigger using a timer, set the timers up.
    //
    if(bTimerTrigger)
    {
        //
        // Make sure the timer is stopped and in its reset state.
        //
        SysCtlPeripheralReset(DATA_ACQ_PERIPH_TIMER);

        //
        // Set up the timer as a single 32 bit periodic counter.
        //
        TimerConfigure(DATA_ACQ_TIMER_BASE, TIMER_CFG_PERIODIC);

        //
        // Set both timers to generate trigger events.
        //
        TimerControlTrigger(DATA_ACQ_TIMER_BASE, TIMER_A, true);

        //
        // Load the timer with the required period to trigger at the
        // the desired rate.
        //
        TimerLoadSet(DATA_ACQ_TIMER_BASE, TIMER_A, HZ_TO_TICKS(ulSampleRate));

        //
        // Start the timer running
        //
        TimerEnable(DATA_ACQ_TIMER_BASE, TIMER_A);
    }
}

//*****************************************************************************
//
// \internal
//
// Sets the ADC sample rate and determines whether to use timer-triggered
// capture or ADC-clock rate capture depending upon the passed sample rate
// and whether we are to capture one or two channels.
//
// \param ulRate is the desired sample capture rate for each channel hertz.
// \param bDualMode is set to \b true if we are to capture data from two
// channels or \b false for single channel capture.
//
// This function checks for valid sample rates and, if found, determines
// the best way to trigger the ADC to achieve the requested rate on either
// one or two channels.
//
// Two different ADC triggering methods may be employed.  For the fastest
// rates, the ADC is run at the requested sample rate (or twice the rate if
// capturing two channels) and is instructed to capture samples as quickly as
// it can.  For slower rates, a timer is used to trigger the ADC at the
// desired rate.  In this case, when dual channel mode is in use, a pair of
// samples is captured on each timer trigger.
//
// \return Returns \b true on success or \b false if an illegal or
// unsupported capture rate is provided.
//
//*****************************************************************************
static tBoolean
SetSampleRate(unsigned long ulRate, tBoolean bDualMode)
{
    //
    // Check for valid sample rates.
    //
    switch(ulRate)
    {
        //
        // Cases which use self-timed capture
        // In these cases, we set the ADC rate and capture at full speed
        // using the appropriate sequence.
        //
        case 1000000:
        {
            //
            // 1MHz sampling is only available for single channel capture.
            //
            if(bDualMode)
            {
                return(false);
            }

            SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);
            g_bTimerTrigger = false;
            break;
        }

        case 500000:
        {
            SysCtlADCSpeedSet(bDualMode ?
                              SYSCTL_ADCSPEED_1MSPS :
                              SYSCTL_ADCSPEED_500KSPS);
            g_bTimerTrigger = false;
            break;
        }

        case 250000:
        {
            SysCtlADCSpeedSet(bDualMode ?
                              SYSCTL_ADCSPEED_500KSPS :
                              SYSCTL_ADCSPEED_250KSPS);
            g_bTimerTrigger = false;
            break;
        }

        case 125000:
        {
            //
            // At 125KHz, we use the ADC clock if capturing a single
            // channel but a timer trigger if capturing 2 channels.
            //
            SysCtlADCSpeedSet(bDualMode ?
                              SYSCTL_ADCSPEED_1MSPS :
                              SYSCTL_ADCSPEED_125KSPS);
            g_bTimerTrigger = bDualMode;
            break;
        }

        //
        // The default case catches all invalid sample rates and all cases
        // where we need to use the timer to trigger capture.
        //
        default:
        {
            //
            // First look for invalid cases.
            //
            if(ulRate > 125000)
            {
                //
                // We don't currently handle rates above 125000 other than the
                // fixed capture rates supported by the ADC hardware itself.
                //
                return(false);
            }

            //
            // If we get here, the sample rate is something we can handle using
            // the timer.
            //
            g_bTimerTrigger = true;

            //
            // Set the ADC clock to be as fast as possible to minimize the
            // time difference between the two channel samples in dual channel
            // mode.  The speed used here needs to be at least twice as fast
            // as the requested capture rate to ensure we don't overflow.
            //
            SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);

            break;
        }
    }

    //
    // All failing cases exit before this point so we know the sample rate is
    // supported.  Save it then set things up given the new rate.
    //
    g_ulSampleRate = ulRate;

    //
    // Set up the ADC sequences for each channel depending upon whether we are
    // using the timer or ADC clock to time the sampling.
    //
    ConfigureADCSequences(g_bTimerTrigger, g_ulSampleRate, bDualMode,
                          g_bTriggerChannel1);

    //
    // All is well if we get this far.
    //
    return(true);
}

//*****************************************************************************
//
// Public functions.
//
//*****************************************************************************

//*****************************************************************************
//
// Initializes the oscilloscope data acquisition module.
//
// This function enables peripherals and configures input pins as required
// to capture oscilloscope data.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
DataAcquisitionInit(void)
{
    //
    // Initialize our instance data.
    //
    g_sChannelInst.eState = DATA_ACQ_IDLE;
    g_sChannelInst.ulMaxSamples = 0;

    ResetBufferPointers();

    //
    // Make sure we have an ADC and the appropriate timer then enable them.
    //
    if(!SysCtlPeripheralPresent(DATA_ACQ_PERIPH_ADC) ||
       !SysCtlPeripheralPresent(DATA_ACQ_PERIPH_TIMER))
    {
        //
        // Oops - we are running on silicon which doesn't have an ADC or timer
        // available!
        //
        return(false);
    }
    else
    {
        //
        // Enable the ADC and timer block we need.
        //
        SysCtlPeripheralEnable(DATA_ACQ_PERIPH_ADC);
        SysCtlPeripheralEnable(DATA_ACQ_PERIPH_TIMER);
    }

    //
    // On DustDevil, with dual mode pads, we need to set the ADC inputs to
    // analog.
    //
    SysCtlPeripheralEnable(DATA_ACQ_PERIPH_GPIO);

    //
    // Set the ADC pins for analog input mode.
    //
    GPIOPinTypeADC(DATA_ACQ_GPIO_BASE, DATA_ACQ_GPIO_PINS);

    //
    // Set the default triggering mode.
    //
    g_eTrigger = TRIGGER_ALWAYS;
    g_eUserTrigger = g_eTrigger;
    g_usTriggerLevel = 0;
    g_bTriggerChannel1 = true;

    //
    // Set the ADC to run at 500KHz
    //
    g_ulSampleRate = 500000;
    SysCtlADCSpeedSet(SYSCTL_ADCSPEED_500KSPS);

    //
    // Default to single channel capture.
    //
    g_bDualMode = false;

    //
    // Set the intial ADC configuration based on the default sample rate.
    //
    ConfigureADCSequences(false, g_ulSampleRate, g_bDualMode,
                          g_bTriggerChannel1);

    //
    // Enable the ADC sequence interrupts at the NVIC level.
    //
    IntEnable(INT_ADC0SS0);

    //
    // Configure the interrupt that we will use to abort capture requests.
    // We enable the master interrupt for the appropriate GPIO module but
    // leave the individual pin interrupts disabled until we know we want
    // to use them.
    //
    SysCtlPeripheralEnable(ABORT_GPIO_PERIPH);
    GPIOIntTypeSet(ABORT_GPIO_BASE, ABORT_GPIO_PINS, GPIO_FALLING_EDGE);
    GPIOPinIntClear(ABORT_GPIO_BASE, ABORT_GPIO_PINS);
    GPIOPinIntDisable(ABORT_GPIO_BASE, ABORT_GPIO_PINS);
    IntEnable(ABORT_GPIO_INT);

    return(true);
}

//*****************************************************************************
//
// Sets the sample rate to be used when capturing oscilloscope data.
//
// \param ulSamplesPerSecond is the number of samples of data to capture
// on each channel per second.
// \param bDualMode is \b true if capturing two channels or \b false for
// single channel capture.
//
// This function enables a client to set the oscilloscope sample rate.  Samples
// are captured at this rate following a call to DataAcquisitionRequestCapture
// and the detection of the required triggering event.
//
// Valid values for \e ulSamplesPerSecond are 1000000, 500000, 250000 and any
// number less than or equal to 125000.
//
// \note When using both channels simultaneously, the maximum supported
// sample rate is 500000.
//
// \return Returns \b true on success or \b false if an invalid sample rate is
// supplied or if a capture is currently active or waiting to start on either
// of the channels.
//
//*****************************************************************************
tBoolean
DataAcquisitionSetRate(unsigned long ulSamplesPerSecond, tBoolean bDualMode)
{
    //
    // We can't change the rate while a capture is pending or active.
    //
    if((g_sChannelInst.eState != DATA_ACQ_COMPLETE) &&
       (g_sChannelInst.eState != DATA_ACQ_IDLE) &&
       (g_sChannelInst.eState != DATA_ACQ_ERROR))
    {
        //
        // This channel is currently active so we can't allow a rate change
        // just now.
        //
        return(false);
    }

    //
    // Remember the capture mode setting.
    //
    g_bDualMode = bDualMode;

    return(SetSampleRate(ulSamplesPerSecond, g_bDualMode));
}

//*****************************************************************************
//
// Sets the type of trigger event to be used in future data sampling.
//
// \param bChannel 1 is \b true to trigger on the signal for channel 1 or
// \b false to trigger on the channel 2 signal.
// \param eType is the type of trigger event to set.  Valid values are
// TRIGGER_LEVEL, TRIGGER_RISING, TRIGGER_FALLING or TRIGGER_ALWAYS.
// \param ulTrigPos defines the position of the trigger in the captured
// sample data buffer relative to the earliest captured sample.  A value of 1
// indicates that the trigger point is to be the earliest sample so all
// returned samples are after the trigger.  A value of (buffer size / 2) places
// the trigger in the center of the captured data and a value of
// (buffer size - 1) places the trigger at the right edge of the capture
// window resulting in all data samples returned being prior to the trigger
// event.  A value of 0 is not valid.
// \param usLevel indicates the trigger level as an ADC sample value.  Valid
// values are 0 <= \e sLevel < 0x400.
//
// This function sets the trigger event that will be used to start
// oscilloscope data capture and also where this trigger will appear in the
// returned data samples.
//
// \note Any trigger events detected before ulTrigPos samples have been
// acquired are ignored.
//
// \note The trigger position is expressed in terms of buffer samples not
// channel samples.  In dual channel mode, therefore, a given trigger position
// will result in half as many samples for each channel captured prior to
// the trigger point than the same trigger position set when in single
// channel mode.  The caller must take this into account when switching modes.
//
// \return Returns \b true on success or \b false if an invalid trigger type
// or level is supplied.
//
//*****************************************************************************
tBoolean
DataAcquisitionSetTrigger(tTriggerType eType, unsigned long ulTrigPos,
                          unsigned short usLevel)
{
    //
    // Check for valid trigger types
    //
    if((eType != TRIGGER_LEVEL) && (eType != TRIGGER_RISING) &&
       (eType != TRIGGER_FALLING) && (eType != TRIGGER_ALWAYS))
    {
        return(false);
    }

    //
    // Check for valid trigger positions.  We check for triggers outside the
    // buffer when a capture request is made since we don't want to enforce
    // an order on setting the buffers vs. setting the default trigger mode.
    //
    if(ulTrigPos == 0)
    {
        return(false);
    }

    //
    // We can't change the trigger position while a capture is pending or
    // active.  We do, however, allow the trigger type and level to be
    // changed.
    //
    if((g_sChannelInst.eState != DATA_ACQ_COMPLETE) &&
       (g_sChannelInst.eState != DATA_ACQ_IDLE) &&
       (ulTrigPos != g_ulTriggerPos))
    {
        //
        // This channel is currently active so we can't allow a trigger
        // change just now.
        //
        return(false);
    }

    //
    // Remember the new trigger setup.
    //
    g_eTrigger = eType;
    g_eUserTrigger = eType;
    g_usTriggerLevel = usLevel;
    g_ulTriggerPos = ulTrigPos;

    return(true);
}

//*****************************************************************************
//
// Sets the channel which is to be used for triggering.
//
// \param bChannel 1 is \b true to trigger on the signal for channel 1 or
// \b false to trigger on the channel 2 signal.
//
// This function sets the channel that is scanned for the required trigger
// event during capture.  This call must be made only when no data capture is
// ongoing.
//
// \return Returns \b true on success or \b false if capture is currently
// ongoing.
//
//*****************************************************************************
tBoolean
DataAcquisitionSetTriggerChannel(tBoolean bChannel1)
{
    //
    // Make sure the data acquisition is currently idle.
    //
    if((g_sChannelInst.eState != DATA_ACQ_COMPLETE) &&
       (g_sChannelInst.eState != DATA_ACQ_IDLE) &&
       (g_sChannelInst.eState != DATA_ACQ_ERROR))
    {
        //
        // We are currently capturing data so can't change the trigger channel.
        //
        return(false);
    }

    g_bTriggerChannel1 = bChannel1;

    //
    // Reprogram the ADC sequence to allow triggering on the required channel.
    //
    ConfigureADCSequences(g_bTimerTrigger, g_ulSampleRate, g_bDualMode,
                          g_bTriggerChannel1);

    return(true);
}

//*****************************************************************************
//
// Gets the channel which is to be used for triggering.
//
// This function allows the current trigger channel to be queried.
//
// \return Returns \b true if channel 1 is being used to trigger or or \b false
// if channel 2 is being used.
//
//*****************************************************************************
tBoolean
DataAcquisitionGetTriggerChannel(void)
{
    return(g_bTriggerChannel1);
}

//*****************************************************************************
//
// Gets information on the current trigger parameters.
//
// \param peType is a pointer which will be written with the current trigger
// type of trigger, TRIGGER_LEVEL, TRIGGER_RISING, TRIGGER_FALLING or
// TRIGGER_ALWAYS.
// \param pulTrigPos is a pointer which will be written with the current
// trigger position.  A value of 0 indicates that the trigger point is at the
// earliest sample.
// \param pusLevel is a pointer which will be written with the current
// trigger level.  The value returned is an ADC sample value in the range
// 0 <= \e *pusLevel < 0x400.
//
// This function allows a client to query the current triggering parameters
// which will be used to capture data following the next call to
// DataAcquisitionRequestCapture().
//
// \return Returns \b true on success or \b false if a NULL pointer is passed
// for any parameter.
//
//*****************************************************************************
tBoolean
DataAcquisitionGetTrigger(tTriggerType *peType, unsigned long *pulTrigPos,
                          unsigned short *pusLevel)
{
    //
    // Check for valid parameters.
    //
    if(!peType || !pulTrigPos || !pusLevel)
    {
        return(false);
    }

    //
    // Return the requested information.
    //
    *peType = g_eUserTrigger;
    *pusLevel = g_usTriggerLevel;
    *pulTrigPos = g_ulTriggerPos;

    return(true);
}

//*****************************************************************************
//
// Sets the buffer into which samples will be captured.
//
// \param ulNumSamples indicates the size of the buffer pointed to by /f
// pusSampleBuffer.  It must be an even number.
// \param pusSampleBuffer is a pointer to the capture buffer.  This buffer must
// be at least (sizeof(unsigned short) * \e ulNumSamples) bytes in length.
//
// This function sets the buffer into which captured samples will be written
// once the data acquisition module detects a trigger event and begins
// capturing data.
//
// The buffer is used as a circular buffer so the caller must take care to
// query the appropriate start and trigger indices when retrieving data to
// ensure that sample order is correctly maintained.
//
// In dual channel mode, samples from the channels are interleaved within
// the same buffer.  The first sample of each pair is from channel 1 and the
// second from channel 2.  In this mode, the overall sample rate remains
// as set using DataAcquisitionSetRate with each channel sampled at half
// that rate.  Note that the samples in a channel 1/channel 2 pair are not
// coincident in time but are 1 sample period apart.
//
// \return Returns \b true on success or \b false if an invalid parameter is
// is supplied or an outstaning capture request exists.
//
//*****************************************************************************
tBoolean
DataAcquisitionSetCaptureBuffer(unsigned long ulNumSamples,
                                unsigned short *pusSampleBuffer)
{
    //
    // Check for valid parameters
    //
    if((ulNumSamples == 0) || (ulNumSamples & 1) ||
       (pusSampleBuffer == (unsigned short *)0))
    {
        return(false);
    }

    //
    // Make sure the channel is not currently capturing samples.
    //
    if((g_sChannelInst.eState != DATA_ACQ_COMPLETE) &&
        (g_sChannelInst.eState != DATA_ACQ_IDLE) &&
        (g_sChannelInst.eState != DATA_ACQ_ERROR))
    {
        //
        // This channel is currently active so we can't allow a buffer change
        // just now.
        //
        return(false);
    }

    //
    // The channel is idle so update the buffer pointer.
    //
    g_sChannelInst.pusSampleBuffer = pusSampleBuffer;
    g_sChannelInst.ulMaxSamples = ulNumSamples;

    //
    // Reset all the buffer indices
    //
    ResetBufferPointers();

    return(true);
}

//*****************************************************************************
//
// Requests capture of data on one or both channels following the next trigger
// event.
//
// This function sets up the hardware to begin capturing samples on the next
// trigger event, as specified in a previous call to function
// DataAcquisitionSetTrigger().  Prior to making this call, the client must
// ensure that DataAcquisitionSetCaptureBuffer() has been called.
//
// If no prior call to DataAcquisitionSetRate() has been made, samples will
// be captured at 500KHz.
//
// If no prior call to DataAcquisitionSetTrigger() has been made,
// TRIGGER_ALWAYS is assumed.
//
// \return Returns \b true on success or \b false if no capture buffer has
// been set for an affected channel.
//
//*****************************************************************************
tBoolean
DataAcquisitionRequestCapture(void)
{
    //
    // Make sure we are not currently capturing on the required channel.
    //
    if((g_sChannelInst.eState != DATA_ACQ_COMPLETE) &&
        (g_sChannelInst.eState != DATA_ACQ_IDLE) &&
        (g_sChannelInst.eState != DATA_ACQ_ERROR))
    {
        //
        // Channel 1 is currently active so we can't allow a new capture
        // request just now.
        //
        return(false);
    }

    //
    // Prepare the channel for capture.
    //
    PrepareChannelForCapture();

    //
    // Enable the ADC sequence to start the capture process.
    //
    ADCSequenceEnable(DATA_ACQ_ADC_BASE, 0);

    return(true);
}

//*****************************************************************************
//
// Cancels any pending capture request for the specified channel.
//
// This function cancels any pending capture request for the specified
// channel(s) and returns the channel to DATA_ACQ_IDLE state.  If the capture
// was in progress when this call is made, it is stopped and all samples
// flushed.
//
// \return None.
//
//*****************************************************************************
void
DataAcquisitionRequestCancel()
{
    //
    // Disable ADC sequence interrupts.
    //
    ADCIntDisable(DATA_ACQ_ADC_BASE, 0);

    //
    // Disable the ADC sequence itself.
    //
    ADCSequenceDisable(DATA_ACQ_ADC_BASE, 0);

    //
    // Mark the channel as idle
    //
    g_sChannelInst.eState = DATA_ACQ_IDLE;
}

//*****************************************************************************
//
// Requests the status of a capture channel.
//
// \param psStatus points to a structure whose members will be written with
// information on the current state of a capture request.
//
// This function requests the current status of data acquisition.
// If the status returned is DATA_ACQ_TRIGGERED or DATA_ACQ_COMPLETE then
// \e ulSamplesCaptured will contain the total number of valid samples that
// have been written to the channel's capture buffer.
//
// To support trigger point offsets within the data, the capture buffer is
// managed as a circular buffer.  The client must be careful to take note of
// the returned \e ulStartIndex value since this represents the entry
// within the buffer at which valid data samples begin.  The client must also
// take care to handle data wrapping past the end of the buffer.  The distance
// between \e ulStartIndex and \e ulTriggerIndex is defined by the
// \e ulTrigPos parameter passed to DataAcquisitionSetTrigger().
//
// The following diagram illustrates a case where capture has been
// triggered and /f ulTrigPos is non zero:
//
//                        |<       ulSamplesCaptured      >|
//  ------------------------------------------------------------
//  |                     X\\\\\\\\\\X\\\\\\\\\\\\\\\\\\\\\|   |
//  ------------------------------------------------------------
//  |                     |          |
//  |                     |           - ulTriggerIndex
//  |                      - ulStartIndex
//   - pusSampleBuffer passed to DataAcquisitionSetCaptureBuffer()
//
// The same set of samples captured across a buffer wrap would look as
// follows:
//
//       >|                               |<  ulSamplesCaptured
//  ------------------------------------------------------------
//  |\\\\\|                               X\\\\\\\\\\X\\\\\\\\\\|
//  ------------------------------------------------------------
//  |                                     |          |
//  |                                     |           - ulTriggerIndex
//  |                                      - ulStartIndex
//   - pusSampleBuffer passed to DataAcquisitionSetCaptureBuffer()
//
// When the module is configured to capture two channels of data, the
// buffer organization is identical except that two 16-bit samples are
// written for every capture period.  In these cases, the channel 1 sample
// appears at the lower address and is immediately followed by a channel 2
// sample.  The fields \e ulSampleOffsetuS and \e ulSamplePerioduS can be used
// to determine the temporal relationship between samples from each channel.
//
// Channel 1 sample 0 |                     |
// .                   > ulSampleOffsetuS   |
// Channel 2 sample 0 |                     |
// .                                         > ulSamplePerioduS
// .                                        |
// .                                        |
// Channel 1 sample 1 |                     |
// .                   > ulSampleOffsetuS
// Channel 2 sample 1 |
//
// \return Returns \b true on success or \b false if an invalid parameter was
// specified.
//
//*****************************************************************************
tBoolean
DataAcquisitionGetStatus(tDataAcqCaptureStatus *psStatus)
{
    //
    // Check for valid parameters
    //
    if(!psStatus)
    {
        //
        // We were passed a NULL pointer so return an error.
        //
        return(false);
    }

    //
    // Get the relevant values from the channel instance structure.  Do this
    // with interrupts disabled to ensure that our pointers are correct and
    // that we don't get inconsistent values if an ADC interrupt occurs just as
    // we are reading the state.
    //
    IntMasterDisable();
    psStatus->eState = g_sChannelInst.eState;
    psStatus->ulStartIndex = GetStartIndex(&g_sChannelInst);
    psStatus->ulTriggerIndex = GetTriggerIndex(&g_sChannelInst);
    psStatus->ulSamplesCaptured = (g_sChannelInst.ulMaxSamples -
                                   g_sChannelInst.ulSamplesToCapture);
    IntMasterEnable();

    //
    // Other fields in the structure are derived from values that are not
    // changed during the interrupt handler so we can safely set them outside
    // the critical section.
    //
    psStatus->bDualMode = g_sChannelInst.bDualMode;
    psStatus->pusBuffer = g_sChannelInst.pusSampleBuffer;
    psStatus->ulMaxSamples = g_sChannelInst.ulMaxSamples;
    psStatus->bBSampleFirst = !g_sChannelInst.bChannel1First;

    //
    // The sample period is merely the reciprocal of the rate originally
    // passed to DataAcquisitionSetRate.
    //
    psStatus->ulSamplePerioduS = 1000000 / g_sChannelInst.ulSampleRate;

    //
    // The channel 1 - channel 2 sample offset time is one ADC clock period.
    // If we are using the timer trigger method, this is alway 1 microsecond
    // since we run the ADC at 1MHz in these cases.  If we are capturing at
    // the full ADC clock rate, the offset is half the sample period.
    //
    psStatus->ulSampleOffsetuS = (g_bTimerTrigger ?
                                  1 : (psStatus->ulSamplePerioduS / 2));

    return(true);
}

//*****************************************************************************
//
// Determines whether sample capture has completed or not.
//
// This function allows a client to determine if a previously-scheduled
// capture request has completed or not.
//
// \return Returns \b true if capture has completed either successfully or
// due to an error or \b false if sampel capture is ongoing.
//
//*****************************************************************************
tBoolean
DataAcquisitionIsComplete(void)
{
    return((g_sChannelInst.eState == DATA_ACQ_COMPLETE) ||
           (g_sChannelInst.eState == DATA_ACQ_IDLE) ||
           (g_sChannelInst.eState == DATA_ACQ_ERROR));
}

//*****************************************************************************
//
// Determines whether an overflow, underflow or abort has occurred since the
// last time these errors were cleared.
//
// \param eChannels indicates the channels whose error status is to be
// returned and, optionally, cleared.
// \param bClear should be set to \b true to clear any flagged errors.  If
// \b false, any errors remain flagged after the call completes.
// \param pbOverflow points to storage that will we written with \b true if
// an overflow error was detected or \b false otherwise.
// \param pbUnderflow points to storage that will we written with \b true if
// an underflow error was detected or \b false otherwise.
//
// This function returns flags indicating whether or not an ADC FIFO overflow
// or underflow has occurred since the last time these errors were cleared.
// The caller may optionally chose to clear any errors detected.  If neither of
// these errors is flagged but the return code is \b false, this indicates that
// the previous capture request was aborted by the user.
//
// \note Note that the overflow and underflow error flags are cleared during
// processing of DataAcquisitionRequestCapture().
//
// \return Returns \b true if any overflow or underflow error has occurred
// since these errors were last cleared or if the last capture request was
// aborted by the user, or \b false if no error has been detected.
//
//*****************************************************************************
tBoolean
DataAcquisitionDidErrorOccur(tBoolean bClear, tBoolean *pbOverflow,
                             tBoolean *pbUnderflow)
{
    long lOverflow;
    long lUnderflow;

    //
    // Check for valid parameters.
    //
    ASSERT((pbOverflow != (tBoolean *)0) && (pbUnderflow != (tBoolean *)0));

    //
    // Set the default return condition (no errors)
    //
    *pbOverflow = false;
    *pbUnderflow = false;

    //
    // Get the current error status
    //
    lOverflow = ADCSequenceOverflow(DATA_ACQ_ADC_BASE, 0);
    lUnderflow = ADCSequenceUnderflow(DATA_ACQ_ADC_BASE, 0);

    //
    // If requested, clear the error flags in the hardware.
    //
    if(bClear)
    {
        ADCSequenceOverflowClear(DATA_ACQ_ADC_BASE, 0);
        ADCSequenceUnderflowClear(DATA_ACQ_ADC_BASE, 0);
    }

    //
    // If an overflow was detected on this channel, set the output flag.
    //
    if(lOverflow)
    {
        *pbOverflow = true;
    }

    //
    // If an underflow was detected on this channel, set the output flag.
    //
    if(lUnderflow)
    {
        *pbUnderflow = true;
    }

    //
    // Set the main return code depending upon whether we are flagging any
    // specific errors.
    //
    if(*pbUnderflow || *pbOverflow)
    {
        return(true);
    }
    else
    {
        return(g_bAbortCapture);
    }
}

//*****************************************************************************
//
// Determines the closest supported sample rate equal to or lower than the
// desired rate passed.
//
// \param ulSampleRate is the client's desired sample rate in Hz.
// \param bDualMode is \b true if the client wishes to capture two channels of
// data or \b false for single channel capture.
//
// This function returns a suitable sample rate as close as possible to the
// desired rate passed by the client.  If an exact match is not possible, the
// nearest lower supported rate is returned.
//
// \return Returns a supported sample rate in Hz.
//
//*****************************************************************************
unsigned long
DataAcquisitionGetClosestRate(unsigned long ulSampleRate, tBoolean bDualMode)
{
    unsigned long ulLoop;
    unsigned long ulRate;

    ulRate = ulSampleRate;

    //
    // We now look through the list of supported sample rates and adjust our
    // calculated value if it falls between any of the hardware rates.
    //
    for(ulLoop = 0; ulLoop < NUM_HARDWARE_SAMPLE_RATES; ulLoop++)
    {
        if(ulRate > g_pulDataAcqSampleRates[ulLoop])
        {
            //
            // Catch 1 special case - we can't support the highest sample
            // rate in 2 channel mode.
            //
            if(!bDualMode || (ulLoop != 0))
            {
                ulRate = g_pulDataAcqSampleRates[ulLoop];
                break;
            }
        }
    }

    //
    // If we drop out of the previous loop after checking all rates, this
    // means that the frequency we calculated is below the hardware capture
    // rate cutoff and, hence, doesn't need adjusted.
    //
    return(ulRate);
}
