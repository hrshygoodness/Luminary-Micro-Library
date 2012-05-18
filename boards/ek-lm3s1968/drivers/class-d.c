//*****************************************************************************
//
// class-d.c - Audio driver for the Class-D amplifier on the EK-LM3S1968.
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
// This is part of revision 8555 of the EK-LM3S1968 Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup class_d_api
//! @{
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "drivers/class-d.h"

//*****************************************************************************
//
// The number of clocks per PWM period.
//
//*****************************************************************************
static unsigned long g_ulClassDPeriod;

//*****************************************************************************
//
// A set of flags indicating the mode of the Class-D audio driver.
//
//*****************************************************************************
static volatile unsigned long g_ulClassDFlags;
#define CLASSD_FLAG_STARTUP     0
#define CLASSD_FLAG_SHUTDOWN    1
#define CLASSD_FLAG_ADPCM       2
#define CLASSD_FLAG_PCM         3

//*****************************************************************************
//
// A pointer to the audio buffer being played.  The validity of this pointer,
// and the expected contents of this buffer, are defined by the flag set in
// g_ulClassDFlags.
//
//*****************************************************************************
static const unsigned char *g_pucClassDBuffer;

//*****************************************************************************
//
// The length of the audio buffer, in bytes.  When performing the startup ramp,
// this is the number of steps left in the ramp.
//
//*****************************************************************************
static unsigned long g_ulClassDLength;

//*****************************************************************************
//
// The volume to playback the audio stream.  This will be a value between 0
// (for silence) and 256 (for full volume).
//
//*****************************************************************************
static long g_lClassDVolume = 256;

//*****************************************************************************
//
// The previous and current audio samples, used for interpolating from 8 KHz
// to 64 KHz audio.
//
//*****************************************************************************
static unsigned short g_pusClassDSamples[2];

//*****************************************************************************
//
// The audio step, which corresponds to the current interpolation point between
// the previous and current audio samples.  The upper bits (that is, 31 through
// 3) are used to determine the sub-sample within a byte of input (for ADPCM
// and DPCM).
//
//*****************************************************************************
static unsigned long g_ulClassDStep;

//*****************************************************************************
//
// The current step index for the ADPCM decoder.  This selects a differential
// value from g_pusADPCMStep.
//
//*****************************************************************************
static long g_lClassDADPCMStepIndex;

//*****************************************************************************
//
// The adjustment to the step index based on the value of an encoded sample.
// The sign bit is ignored when using this table (that is, only the lower three
// bits are used).
//
//*****************************************************************************
static const signed char g_pcADPCMIndex[8] =
{
    -1, -1, -1, -1, 2, 4, 6, 8
};

//*****************************************************************************
//
// The differential values for the ADPCM decoder.  One of these is selected
// based on the step index.
//
//*****************************************************************************
static const unsigned short g_pusADPCMStep[89] =
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41,
    45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209,
    230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876,
    963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767
};

//*****************************************************************************
//
//! Handles the PWM1 interrupt.
//!
//! This function responds to the PWM1 interrupt, updating the duty cycle of
//! the output waveform in order to produce sound.  It is the application's
//! responsibility to ensure that this function is called in response to the
//! PWM1 interrupt, typically by installing it in the vector table as the
//! handler for the PWM1 interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
ClassDPWMHandler(void)
{
    long lStep, lNibble, lDutyCycle;

    //
    // Clear the PWM interrupt.
    //
    PWMGenIntClear(PWM0_BASE, PWM_GEN_1, PWM_INT_CNT_ZERO);

    //
    // See if the startup ramp is in progress.
    //
    if(HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_STARTUP))
    {
        //
        // Decrement the ramp count.
        //
        g_ulClassDStep--;

        //
        // Increase the pulse width of the two outputs by one clock.
        //
        PWMDeadBandEnable(PWM0_BASE, PWM_GEN_1, 0, g_ulClassDStep);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,
                         (g_ulClassDPeriod - g_ulClassDStep) / 2);

        //
        // See if this was the last step of the ramp.
        //
        if(g_ulClassDStep == 0)
        {
            //
            // Indicate that the startup ramp has completed.
            //
            HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_STARTUP) = 0;
        }

        //
        // There is nothing further to be done.
        //
        return;
    }

    //
    // See if the shutdown ramp is in progress.
    //
    if(HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_SHUTDOWN))
    {
        //
        // See if this was the last step of the ramp.
        //
        if(g_ulClassDStep == (g_ulClassDPeriod - 2))
        {
            //
            // Disable the PWM2 and PWM3 output signals.
            //
            PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, false);

            //
            // Clear the Class-D audio flags.
            //
            g_ulClassDFlags = 0;

            //
            // Disable the PWM interrupt.
            //
            IntDisable(INT_PWM0_1);
        }
        else
        {
            //
            // Increment the ramp count.
            //
            g_ulClassDStep++;

            //
            // Decrease the pulse width of the two outputs by one clock.
            //
            PWMDeadBandEnable(PWM0_BASE, PWM_GEN_1, 0, g_ulClassDStep);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,
                             (g_ulClassDPeriod - g_ulClassDStep) / 2);
        }

        //
        // There is nothing further to be done.
        //
        return;
    }

    //
    // Compute the value of the PCM sample based on the blended average of the
    // previous and current samples.  It should be noted that linear
    // interpolation does not produce the best results with audio (it produces
    // a significant amount of harmonic aliasing) but it is fast.
    //
    lDutyCycle = (((g_pusClassDSamples[0] * (8 - (g_ulClassDStep & 7))) +
                   (g_pusClassDSamples[1] * (g_ulClassDStep & 7))) / 8);

    //
    // Adjust the magnitude of the sample based on the current volume.  Since a
    // multiplicative volume control is implemented, the volume value will
    // result in nearly linear volume adjustment if it is squared.
    //
    lDutyCycle = (((lDutyCycle - 32768) * g_lClassDVolume * g_lClassDVolume) /
                  65536) + 32768;

    //
    // Set the PWM duty cycle based on this PCM sample.
    //
    lDutyCycle = (g_ulClassDPeriod * lDutyCycle) / 65536;
    if(lDutyCycle > (g_ulClassDPeriod - 2))
    {
        lDutyCycle = g_ulClassDPeriod - 2;
    }
    if(lDutyCycle < 2)
    {
        lDutyCycle = 2;
    }
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, lDutyCycle);

    //
    // Increment the audio step.
    //
    g_ulClassDStep++;

    //
    // See if the next sample has been reached.
    //
    if((g_ulClassDStep & 7) == 0)
    {
        //
        // Copy the current sample to the previous sample.
        //
        g_pusClassDSamples[0] = g_pusClassDSamples[1];

        //
        // See if there is more input data.
        //
        if(g_ulClassDLength == 0)
        {
            //
            // All input data has been played, so start a shutdown to avoid a
            // pop.
            //
            g_ulClassDFlags = 0;
            HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_SHUTDOWN) = 1;
            g_ulClassDStep = 0;
        }

        //
        // See if an ADPCM stream is being played.
        //
        else if(HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_ADPCM))
        {
            //
            // See which nibble should be decoded.
            //
            if((g_ulClassDStep & 8) == 0)
            {
                //
                // Extract the lower nibble from the current byte, and skip to
                // the next byte.
                //
                lNibble = *g_pucClassDBuffer++;

                //
                // Decrement the count of bytes to be decoded.
                //
                g_ulClassDLength--;
            }
            else
            {
                //
                // Extract the upper nibble from the current byte.
                //
                lNibble = *g_pucClassDBuffer >> 4;
            }

            //
            // Compute the sample delta based on the current nibble and step
            // size.
            //
            lStep = ((((2 * (lNibble & 7)) + 1) *
                      g_pusADPCMStep[g_lClassDADPCMStepIndex]) / 16);

            //
            // Add or subtract the delta to the previous sample value, clipping
            // if necessary.
            //
            if(lNibble & 8)
            {
                lStep = g_pusClassDSamples[0] - lStep;
                if(lStep < 0)
                {
                    lStep = 0;
                }
            }
            else
            {
                lStep = g_pusClassDSamples[0] + lStep;
                if(lStep > 65535)
                {
                    lStep = 65535;
                }
            }

            //
            // Store the generated sample.
            //
            g_pusClassDSamples[1] = lStep;

            //
            // Adjust the step size index based on the current nibble, clipping
            // the value if required.
            //
            g_lClassDADPCMStepIndex += g_pcADPCMIndex[lNibble & 7];
            if(g_lClassDADPCMStepIndex < 0)
            {
                g_lClassDADPCMStepIndex = 0;
            }
            if(g_lClassDADPCMStepIndex > 88)
            {
                g_lClassDADPCMStepIndex = 88;
            }
        }

        //
        // See if a 8-bit PCM stream is being played.
        //
        else if(HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_PCM))
        {
            //
            // Read the next sample from the input stream.
            //
            g_pusClassDSamples[1] = *g_pucClassDBuffer++ * 256;

            //
            // Decrement the count of samples to be played.
            //
            g_ulClassDLength--;
        }

        //
        // This should never be reached, where the Class-D audio flags are set
        // to something that is not recognized.  In this case, start a shutdown
        // to avoid a pop.
        //
        else
        {
            g_ulClassDFlags = 0;
            HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_SHUTDOWN) = 1;
            g_ulClassDStep = 0;
        }
    }
}

//*****************************************************************************
//
//! Initializes the Class-D audio driver.
//!
//! \param ulPWMClock is the rate of the clock supplied to the PWM module.
//!
//! This function initializes the Class-D audio driver, preparing it to output
//! audio data to the speaker.
//!
//! The PWM module clock should be as high as possible; lower clock rates
//! reduces the quality of the produced audio.  For the best quality audio, the
//! PWM module should be clocked at 50 MHz.
//!
//! \note In order for the Class-D audio driver to function properly, the
//! Class-D audio driver interrupt handler (ClassDPWMHandler()) must be
//! installed into the vector table for the PWM1 interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
ClassDInit(unsigned long ulPWMClock)
{
    //
    // Enable the peripherals used by the Class-D audio driver.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);

    //
    // Set GPIO H0 and H1 as PWM pins.  They are used to output the PWM2 and
    // PWM3 signals.
    //
    GPIOPinTypePWM(GPIO_PORTH_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                     GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);

    //
    // Compute the PWM period based on the system clock.
    //
    g_ulClassDPeriod = ulPWMClock / 64000;

    //
    // Set the PWM period to 64 KHz.
    //
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1,
                    PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, g_ulClassDPeriod);

    //
    // Start the PWM outputs with one clock long pulses.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 1);
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_1, 0, g_ulClassDPeriod - 2);

    //
    // Disable the PWM2 and PWM3 output signals.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, false);

    //
    // Enable the PWM generator.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);

    //
    // Clear the Class-D audio flags.
    //
    g_ulClassDFlags = 0;

    //
    // Enable the PWM generator interrupt.
    //
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_1, PWM_INT_CNT_ZERO);
}

//*****************************************************************************
//
//! Plays a buffer of 8 KHz, 8-bit, unsigned PCM data.
//!
//! \param pucBuffer is a pointer to the buffer containing 8-bit, unsigned PCM
//! data.
//! \param ulLength is the number of bytes in the buffer.
//!
//! This function starts playback of a stream of 8-bit, unsigned PCM data.
//! Since the data is unsigned, a value of 128 represents the mid-point of the
//! speaker's travel (that is, corresponds to no DC offset).
//!
//! \return None.
//
//*****************************************************************************
void
ClassDPlayPCM(const unsigned char *pucBuffer, unsigned long ulLength)
{
    //
    // Return without playing the buffer if something is already playing.
    //
    if(g_ulClassDFlags)
    {
        return;
    }

    //
    // Save a pointer to the buffer.
    //
    g_pucClassDBuffer = pucBuffer;
    g_ulClassDLength = ulLength;

    //
    // Initialize the sample buffer with silence.
    //
    g_pusClassDSamples[0] = 32768;
    g_pusClassDSamples[1] = 32768;

    //
    // Set the length of the startup processing.
    //
    g_ulClassDStep = g_ulClassDPeriod - 2;

    //
    // Start playback of a PCM stream.
    //
    HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_STARTUP) = 1;
    HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_PCM) = 1;

    //
    // Enable the PWM2 and PWM3 output signals.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, true);

    //
    // Enable the PWM interrupt.
    //
    IntEnable(INT_PWM0_1);
}

//*****************************************************************************
//
//! Plays a buffer of 8 KHz IMA ADPCM data.
//!
//! \param pucBuffer is a pointer to the buffer containing the IMA ADPCM
//! encoded data.
//! \param ulLength is the number of bytes in the buffer.
//!
//! This function starts playback of a stream of IMA ADPCM encoded data.  The
//! data is decoded as needed and therefore does not require a large buffer in
//! SRAM.  This provides a 2:1 compression ratio relative to raw 8-bit PCM with
//! little to no loss in audio quality.
//!
//! \return None.
//
//*****************************************************************************
void
ClassDPlayADPCM(const unsigned char *pucBuffer, unsigned long ulLength)
{
    //
    // Return without playing the buffer if something is already playing.
    //
    if(g_ulClassDFlags)
    {
        return;
    }

    //
    // Save a pointer to the buffer.
    //
    g_pucClassDBuffer = pucBuffer;
    g_ulClassDLength = ulLength;

    //
    // Initialize the sample buffer with silence.
    //
    g_pusClassDSamples[0] = 32768;
    g_pusClassDSamples[1] = 32768;

    //
    // Initialize the ADPCM step index.
    //
    g_lClassDADPCMStepIndex = 0;

    //
    // Set the length of the startup processing.
    //
    g_ulClassDStep = g_ulClassDPeriod - 2;

    //
    // Start playback of an ADPCM stream.
    //
    HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_STARTUP) = 1;
    HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_ADPCM) = 1;

    //
    // Enable the PWM2 and PWM3 output signals.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, true);

    //
    // Enable the PWM interrupt.
    //
    IntEnable(INT_PWM0_1);
}

//*****************************************************************************
//
//! Determines if the Class-D audio driver is busy.
//!
//! This function determines if the Class-D audio driver is busy, either
//! performing the startup or shutdown ramp for the speaker or playing an audio
//! stream.
//!
//! \return Returns \b true if the Class-D audio driver is busy and \b false
//! otherwise.
//
//*****************************************************************************
tBoolean
ClassDBusy(void)
{
    //
    // The Class-D audio driver is busy if the Class-D audio flags are not
    // zero.
    //
    return(g_ulClassDFlags != 0);
}

//*****************************************************************************
//
//! Stops playback of the current audio stream.
//!
//! This function immediately stops playback of the current audio stream.  As
//! a result, the output is changed directly to the mid-point, possibly
//! resulting in a pop or click.  It is then ramped down to no output,
//! eliminating the current draw through the Class-D amplifier and speaker.
//!
//! \return None.
//
//*****************************************************************************
void
ClassDStop(void)
{
    //
    // See if playback is in progress.
    //
    if((g_ulClassDFlags != 0) &&
       (HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_SHUTDOWN) == 0))
    {
        //
        // Temporarily disable the PWM interrupt.
        //
        IntDisable(INT_PWM0_1);

        //
        // Clear the Class-D flags and set the shutdown flag (to try to avoid
        // a pop, though one may still occur based on the current position of
        // the output waveform).
        //
        g_ulClassDFlags = 0;
        HWREGBITW(&g_ulClassDFlags, CLASSD_FLAG_SHUTDOWN) = 1;

        //
        // Set the shutdown step to the first.
        //
        g_ulClassDStep = 0;

        //
        // Reenable the PWM interrupt.
        //
        IntEnable(INT_PWM0_1);
    }
}

//*****************************************************************************
//
//! Sets the volume of the audio playback.
//!
//! \param ulVolume is the volume of the audio playback, specified as a value
//! between 0 (for silence) and 256 (for full volume).
//!
//! This function sets the volume of the audio playback.  Setting the volume to
//! 0 will mute the output, while setting the volume to 256 will play the audio
//! stream without any volume adjustment (that is, full volume).
//!
//! \return None.
//
//*****************************************************************************
void
ClassDVolumeSet(unsigned long ulVolume)
{
    //
    // Set the volume mulitplier to be used.
    //
    g_lClassDVolume = ulVolume;
}

//*****************************************************************************
//
//! Increases the volume of the audio playback.
//!
//! \param ulVolume is the amount by which to increase the volume of the audio
//! playback, specified as a value between 0 (for no adjustment) and 256
//! maximum adjustment).
//!
//! This function increases the volume of the audio playback relative to the
//! current volume.
//!
//! \return None.
//
//*****************************************************************************
void
ClassDVolumeUp(unsigned long ulVolume)
{
    //
    // Compute the new volume, limiting to the maximum if required.
    //
    ulVolume = g_lClassDVolume + ulVolume;
    if(ulVolume > 256)
    {
        ulVolume = 256;
    }

    //
    // Set the new volume.
    //
    g_lClassDVolume = ulVolume;
}

//*****************************************************************************
//
//! Decreases the volume of the audio playback.
//!
//! \param ulVolume is the amount by which to decrease the volume of the audio
//! playback, specified as a value between 0 (for no adjustment) and 256
//! maximum adjustment).
//!
//! This function decreases the volume of the audio playback relative to the
//! current volume.
//!
//! \return None.
//
//*****************************************************************************
void
ClassDVolumeDown(unsigned long ulVolume)
{
    //
    // Compute the new volume, limiting to the minimum if required.
    //
    ulVolume = g_lClassDVolume - ulVolume;
    if(ulVolume & 0x80000000)
    {
        ulVolume = 0;
    }

    //
    // Set the new volume.
    //
    g_lClassDVolume = ulVolume;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
