//*****************************************************************************
//
// data-acq.h - Prototypes and data types related to the data acquisition
//              module of the oscilloscope application.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#ifndef __DATA_ACQ_H__
#define __DATA_ACQ_H__

//*****************************************************************************
//
// The GPIO module and pins corresponding to the pushbuttons that are to be
// used to abort an untriggered capture.  This is set to correspond to the
// directional pushbutton on the board.
//
//*****************************************************************************
#define ABORT_GPIO_PERIPH       SYSCTL_PERIPH_GPIOB
#define ABORT_GPIO_BASE         GPIO_PORTB_BASE
#define ABORT_GPIO_PINS         (GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | \
                                 GPIO_PIN_6)
#define ABORT_GPIO_INT          INT_GPIOB

//*****************************************************************************
//
// Defines and macros to aid in conversion between ADC sample values and
// voltage levels.
//
//*****************************************************************************

//*****************************************************************************
//
// Voltage offsets and scale constants
//
//*****************************************************************************
#define ADC_MAX_MV              33000
#define ADC_OFFSET_VOLTAGE      16500
#define ADC_NUM_BITS            10

//*****************************************************************************
//
//! Convert the ADC output to millivolts.
//
//*****************************************************************************
#define ADC_SAMPLE_TO_MV(x)     (((ADC_MAX_MV * (long)(x)) >> ADC_NUM_BITS) - \
                                 ADC_OFFSET_VOLTAGE)

//*****************************************************************************
//
//! Convert a voltage level to an equivalent ADC output value.
//
//*****************************************************************************
#define MV_TO_ADC_SAMPLE(x)     ((((x) + ADC_OFFSET_VOLTAGE) << \
                                  ADC_NUM_BITS) / ADC_MAX_MV)

//*****************************************************************************
//
//! Determine the "temporal index" of a buffer index.  This macro converts
//! an index into a circular capture buffer into an index relative to the
//! start position (in other words, a count of samples since the start of
//! the capture).
//
//*****************************************************************************
#define DISTANCE_FROM_START(start, target, limit)       \
        (((target) >= (start)) ? ((target) - (start)) : \
         (((limit) - (start)) + (target)))

//*****************************************************************************
//
// Data Type Definitions.
//
//*****************************************************************************

//*****************************************************************************
//
//! Enumerated type defining the various trigger events that can be set.
//
//*****************************************************************************
typedef enum
{
    //
    //! Trigger at a particular voltage level regardless of signal edge.
    //
    TRIGGER_LEVEL,

    //
    //! Trigger on a rising edge in the signal.
    //
    TRIGGER_RISING,

    //
    //! Trigger on a falling edge in the signal.
    //
    TRIGGER_FALLING,

    //
    //! Trigger immediately on the next call to DataAcquisitionRequestCapture.
    //
    TRIGGER_ALWAYS
}
tTriggerType;

//*****************************************************************************
//
//! Enumerated type defining the various states that a a capture request may
//! be in.
//
//*****************************************************************************
typedef enum
{
    //
    //! Channel is actively searching for a trigger event.
    //
    DATA_ACQ_TRIGGER_SEARCH,

    //
    //! Channel is idle.
    //
    DATA_ACQ_IDLE,

    //
    //! Channel is buffering samples before looking for a trigger event.
    //
    DATA_ACQ_BUFFERING,

    //
    //! Channel has been triggered and is capturing data.
    //
    DATA_ACQ_TRIGGERED,

    //
    //! Channel has been triggered and has completed capturing data.
    //
    DATA_ACQ_COMPLETE,

    //
    //! An error occurred while the channel was attempting to capture data.
    //
    DATA_ACQ_ERROR,

    //
    //! A dummy state purely to give us a count of the members of the enum.
    //
    DATA_ACQ_STATE_COUNT
}
tDataAcqState;

//*****************************************************************************
//
//! Structure used in reporting the status of an ongoing or completed
//! capture request.
//
//*****************************************************************************
typedef struct
{
    //
    //! The current state of the data acquisition module.
    //
    tDataAcqState eState;

    //
    //! The mode of operation - \e true for dual channel or \e false for
    //! single channel.
    //
    tBoolean bDualMode;

    //
    //! The order of the samples stored in the buffer when capturing two
    //! channel data.
    //
    tBoolean bBSampleFirst;

    //
    //! The number of valid samples contained within the buffer at the time
    //! the call is made.
    //
    unsigned long ulSamplesCaptured;

    //
    //! A pointer to the start of the ring buffer being used to hold captured
    //! samples.
    //
    unsigned short *pusBuffer;

    //
    //! The total number of samples that the buffer pointed to by pusBuffer
    //! could contain.
    //
    unsigned long ulMaxSamples;

    //! The index of the oldest valid sample in the buffer.
    //
    unsigned long ulStartIndex;

    //
    //! The index the channel 1 sample in the ring buffer which corresponds
    //! to the trigger point.  If no trigger has been detected, the contents
    //! of this field are undefined and should be ignored.
    //
    unsigned long ulTriggerIndex;

    //
    //! A pair of samples, one each for channel 1 and 2 are captured one ADC
    //! clock apart rather than at exactly the same time.  This field contains
    //! information on the number of microseconds between a channel 1 sample
    //! (even indices in the capture buffer) and its following channel 2
    //! sample (odd indices in the buffer).  This information may be used to
    //! correctly position the channel 2 waveform on the display.
    //!
    //! When single channel capture is configured, this value will be 0.
    //
    unsigned long ulSampleOffsetuS;

    //
    //! This field contains the time difference between consecutive samples
    //! from the same channel expressed in microseconds.
    //
    unsigned long ulSamplePerioduS;
}
tDataAcqCaptureStatus;

//*****************************************************************************
//
// Exported Function Prototypes.
//
//*****************************************************************************
extern tBoolean DataAcquisitionInit(void);
extern tBoolean DataAcquisitionSetRate(unsigned long ulSamplesPerSecond,
                                       tBoolean bDualMode);
extern tBoolean DataAcquisitionSetTrigger(tTriggerType eType,
                                          unsigned long ulTrigPos,
                                          unsigned short usLevel);
extern tBoolean DataAcquisitionSetTriggerChannel(tBoolean bChannel1);
extern tBoolean DataAcquisitionGetTriggerChannel(void);
extern tBoolean DataAcquisitionGetTrigger(tTriggerType *peType,
                                          unsigned long *pulTrigPos,
                                          unsigned short *pusLevel);
extern tBoolean
       DataAcquisitionSetCaptureBuffer(unsigned long ulNumSamples,
                                       unsigned short *pusSampleBuffer);
extern tBoolean DataAcquisitionRequestCapture(void);
extern void DataAcquisitionRequestCancel(void);
extern tBoolean DataAcquisitionIsComplete(void);
extern tBoolean DataAcquisitionGetStatus(tDataAcqCaptureStatus *psStatus);
extern tBoolean DataAcquisitionDidErrorOccur(tBoolean bClear,
                                             tBoolean *pbOverflow,
                                             tBoolean *pbUnderflow);
extern unsigned long DataAcquisitionGetClosestRate(unsigned long ulSampleRate,
                                                   tBoolean bDualMode);

#endif // __DATA_ACQ_H__
