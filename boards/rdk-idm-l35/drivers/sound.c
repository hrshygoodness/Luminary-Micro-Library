//*****************************************************************************
//
// sound.c - Sound driver for the Intelligent Display Module.
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
// This is part of revision 8555 of the RDK-IDM-L35 Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup sound_api
//! @{
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "drivers/sound.h"

//*****************************************************************************
//
// The frequency of the system clock, which drives the timers.
//
//*****************************************************************************
static unsigned long g_ulSystemClock;

//*****************************************************************************
//
// The current volume of the music/sound effects.
//
//*****************************************************************************
static unsigned char g_ucVolume = 100;

//*****************************************************************************
//
// A pointer to the song currently being played, if any.  The value of this
// variable is used to determine whether or not a song is being played.  Since
// each entry is a short, the maximum length of the song is 65536 / 200
// seconds, which is around 327 seconds.
//
//*****************************************************************************
static const unsigned short *g_pusMusic = 0;

//*****************************************************************************
//
// The number of bytes in the array describing the song currently being played.
//
//*****************************************************************************
static unsigned short g_usMusicLength;

//*****************************************************************************
//
// The count of clock ticks into the song being played.
//
//*****************************************************************************
static unsigned short g_usMusicCount;

//*****************************************************************************
//
// Mutes the audio output and sets the PWM generator to an inaudible frequency.
//
//*****************************************************************************
static void
SoundMute(void)
{
    //
    // Disable the timer that is generating the PWM output.
    //
    TimerDisable(TIMER2_BASE, TIMER_A);

    //
    // Make the output pin be a GPIO output and make it be low.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0);
}

//*****************************************************************************
//
//! Disables the sound output.
//!
//! This function disables the sound output, muting the speaker and cancelling
//! any playback that may be in progress.
//!
//! \return None.
//
//*****************************************************************************
void
SoundDisable(void)
{
    //
    // Cancel any song or sound effect playback that may be in progress.
    //
    g_pusMusic = 0;

    //
    // Mute the output.
    //
    SoundMute();
}

//*****************************************************************************
//
//! Enables the sound output.
//!
//! This function enables the sound output, preparing it to play music or sound
//! effects.
//!
//! \return None.
//
//*****************************************************************************
void
SoundEnable(void)
{
    //
    // Set the timer to produce 40 kHz, which is well beyond the limits of
    // human hearing.
    //
    TimerLoadSet(TIMER2_BASE, TIMER_A, (g_ulSystemClock / 40000) - 1);

    //
    // Restore the output volume.
    //
    SoundVolumeSet(g_ucVolume);

    //
    // Enable the PWM output timer.
    //
    TimerEnable(TIMER2_BASE, TIMER_A);

    //
    // Configure the speaker GPIO pin as a timer PWM pin.
    //
    GPIOPinTypeTimer(GPIO_PORTC_BASE, GPIO_PIN_7);
}

//*****************************************************************************
//
//! Sets the sound output frequency.
//!
//! \param ulFrequency is the desired sound output frequency.
//!
//! This function sets the frequency of the output sound.  This change will
//! take effect immediately and will remain in effect until changed (either
//! explicitly by another call or implicitly by the playback of a sound).
//!
//! \return None.
//
//*****************************************************************************
void
SoundFrequencySet(unsigned long ulFrequency)
{
    //
    // Set the PWM frequency to the requested frequency.
    //
    TimerLoadSet(TIMER2_BASE, TIMER_A, (g_ulSystemClock / ulFrequency) - 1);
}

//*****************************************************************************
//
//! Initializes the sound output.
//!
//! This function prepares the sound driver to play songs or sound effects.  It
//! must be called before any other sound functions.  The sound driver uses
//! timer 2 subtimer A to produce the PWM output, and timer 2 subtimer B to be
//! the time base for the playback of sound effects.  It is the responsility of
//! the application to ensure that SoundIntHandler() is called when the timer 2
//! subtimer B interrupt occurs (typically by placing a pointer to this
//! function in the appropriate location in the processor's vector table).
//!
//! \return None.
//
//*****************************************************************************
void
SoundInit(void)
{
    //
    // Get the current clock frequency, which is used to determine the counter
    // values that result in a desired output audio frequency.
    //
    g_ulSystemClock = SysCtlClockGet();

    //
    // Enable the GPIO and timer peripherals used to produce sound.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);

    //
    // Configure the GPIO pin connected to the speaker as a GPIO output and set
    // it low (producing silence).
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0);

    //
    // Configure the second timer as a 16-bit pair, with the first producing a
    // PWM output and the other in periodic mode.
    //
    TimerConfigure(TIMER2_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM |
                                 TIMER_CFG_B_PERIODIC));

    //
    // Invert the PWM output, so that the match value relative to zero always
    // defines the amount of time the PWM signal is high (regardless of the
    // current load value).
    //
    TimerControlLevel(TIMER2_BASE, TIMER_A, true);
}

//*****************************************************************************
//
//! Handles the sound timer interrupt.
//!
//! This function provides periodic updates to the PWM output in order to
//! produce a sound effect.  It is called when the timer 2 subtimer B interrupt
//! occurs.
//!
//! \return None.
//
//*****************************************************************************
void
SoundIntHandler(void)
{
    unsigned long ulLoop;

    //
    // Clear the timer interrupt.
    //
    TimerIntClear(TIMER2_BASE, TIMER_TIMB_TIMEOUT);

    //
    // See if a song is being played.
    //
    if(g_pusMusic)
    {
        //
        // Find the correct position within the array describing the song.
        //
        for(ulLoop = 0; ulLoop < g_usMusicLength; ulLoop += 2)
        {
            if(g_pusMusic[ulLoop] > g_usMusicCount)
            {
                break;
            }
        }

        //
        // See if the end of the song has been reached.
        //
        if(ulLoop == g_usMusicLength)
        {
            //
            // The song is over, so mute the output.
            //
            SoundMute();

            //
            // Indicate that there is no longer a song being played.
            //
            g_pusMusic = 0;
        }
        else if(g_usMusicCount == g_pusMusic[ulLoop - 2])
        {
            //
            // Set the PWM frequency to the requested frequency.
            //
            TimerLoadSet(TIMER2_BASE, TIMER_A,
                         (g_ulSystemClock / g_pusMusic[ulLoop - 1]) - 1);
        }

        //
        // Increment the music counter.
        //
        g_usMusicCount++;
    }
}

//*****************************************************************************
//
//! Starts playback of a song.
//!
//! \param pusSong is a pointer to the song data structure.
//! \param ulLength is the length of the song data structure in bytes.
//!
//! This function starts the playback of a song or sound effect.  If a song
//! or sound effect is already being played, its playback is cancelled and the
//! new song is started.
//!
//! \return None.
//
//*****************************************************************************
void
SoundPlay(const unsigned short *pusSong, unsigned long ulLength)
{
    //
    // Stop the playback of any previous song or sound effect.
    //
    g_pusMusic = 0;

    //
    // Save the length of the song and start the song counter at zero.
    //
    g_usMusicLength = ulLength;
    g_usMusicCount = 0;

    //
    // Save the pointer to the song data.  At this point, the interrupt handler
    // could be called and commence the actual playback.
    //
    g_pusMusic = pusSong;

    //
    // Unmute the audio volume.
    //
    SoundEnable();

    //
    // See if the periodic timer has been initialized.
    //
    if(TimerPrescaleGet(TIMER2_BASE, TIMER_B) != 3)
    {
        //
        // Set the prescale on the periodic timer to 4.
        //
        TimerPrescaleSet(TIMER2_BASE, TIMER_B, 3);

        //
        // Set the periodic timer to produce an interrupt at 200 Hz.
        //
        TimerLoadSet(TIMER2_BASE, TIMER_B, (g_ulSystemClock / (200 * 4)) - 1);

        //
        // Enable the periodic timer.
        //
        TimerEnable(TIMER2_BASE, TIMER_B);

        //
        // Enable the periodic timer's interrupt.
        //
        TimerIntEnable(TIMER2_BASE, TIMER_TIMB_TIMEOUT);
        IntEnable(INT_TIMER2B);
    }
}

//*****************************************************************************
//
//! Sets the volume of the music/sound effect playback.
//!
//! \param ulPercent is the volume percentage, which must be between 0%
//! (silence) and 100% (full volume), inclusive.
//!
//! This function sets the volume of the sound output to a value between
//! silence (0%) and full volume (100%).
//!
//! \return None.
//
//*****************************************************************************
void
SoundVolumeSet(unsigned long ulPercent)
{
    //
    // Check the argument.
    //
    ASSERT(ulPercent <= 100);

    //
    // Set the timer match value based on the volume.
    //
    TimerMatchSet(TIMER2_BASE, TIMER_A, ulPercent * 26);

    //
    // Save the volume for future use (such as un-muting).
    //
    g_ucVolume = ulPercent;
}

//*****************************************************************************
//
//! Decreases the volume.
//!
//! \param ulPercent is the amount to decrease the volume, specified as a
//! percentage between 0% (silence) and 100% (full volume), inclusive.
//!
//! This function adjusts the audio output down by the specified percentage.
//! The adjusted volume will not go below 0% (silence).
//!
//! \return None.
//
//*****************************************************************************
void
SoundVolumeDown(unsigned long ulPercent)
{
    //
    // Do not let the volume go below 0%.
    //
    if(g_ucVolume < ulPercent)
    {
        //
        // Set the volume to the minimum.
        //
        g_ucVolume = 0;
    }
    else
    {
        //
        // Decrease the volume by the specified amount.
        //
        g_ucVolume -= ulPercent;
    }

    //
    // Set the new volume.
    //
    SoundVolumeSet(g_ucVolume);
}

//*****************************************************************************
//
//! Returns the current volume level.
//!
//! This function returns the current volume, specified as a percentage between
//! 0% (silence) and 100% (full volume), inclusive.
//!
//! \return Returns the current volume.
//
//*****************************************************************************
unsigned char
SoundVolumeGet(void)
{
    //
    // Return the current Audio Volume.
    //
    return(g_ucVolume);
}

//*****************************************************************************
//
//! Increases the volume.
//!
//! \param ulPercent is the amount to increase the volume, specified as a
//! percentage between 0% (silence) and 100% (full volume), inclusive.
//!
//! This function adjusts the audio output up by the specified percentage.  The
//! adjusted volume will not go above 100% (full volume).
//!
//! \return None.
//
//*****************************************************************************
void
SoundVolumeUp(unsigned long ulPercent)
{
    //
    // Increase the volume by the specified amount.
    //
    g_ucVolume += ulPercent;

    //
    // Do not let the volume go above 100%.
    //
    if(g_ucVolume > 100)
    {
        //
        // Set the volume to the maximum.
        //
        g_ucVolume = 100;
    }

    //
    // Set the new volume.
    //
    SoundVolumeSet(g_ucVolume);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
