//*****************************************************************************
//
// audio.c - Routines for playing music and sound effects.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_types.h"
#include "driverlib/pwm.h"
#include "audio.h"
#include "globals.h"

//*****************************************************************************
//
// The current volume of the music/sound effects.
//
//*****************************************************************************
static unsigned char g_ucVolume = 50;

//*****************************************************************************
//
// A pointer to the song currently being played, if any.  The value of this
// variable is used to determine whether or not a song is being played.  Since
// each entry is a short, the maximum length of the song is 65536 / 300
// seconds, which is around 218 seconds.
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
// A pointer to the sound effect currently being played, if any.  The value of
// this variable is used to determine whether or nto a song is being played.
// Each entry of this array represents 1/300th of a second.
//
//*****************************************************************************
static const unsigned short *g_pusSoundEffect = 0;

//*****************************************************************************
//
// The number of entries in the array describing the sound effect currently
// being played.
//
//*****************************************************************************
static unsigned short g_usSoundLength;

//*****************************************************************************
//
// The cound of clock ticks into the sound effect being played.
//
//*****************************************************************************
static unsigned short g_usSoundCount;

//*****************************************************************************
//
// Mutes the audio output and sets the PWM generator to an inaudible frequency.
//
//*****************************************************************************
static void
AudioMute(void)
{
    //
    // Disable the PWM output.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, false);
    PWMOutputInvert(PWM0_BASE, PWM_OUT_1_BIT, false);

    //
    // Set the PWM frequency to 40 KHz, beyond the range of human hearing.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, g_ulSystemClock / (SILENCE * 8));
    PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT);
}

//*****************************************************************************
//
// Sets the volume of the music/sound effect playback.
//
//*****************************************************************************
void
AudioVolume(unsigned long ulPercent)
{
    //
    // If the volume is below two then simply mute the output.
    //
    if(ulPercent < 2)
    {
        PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, false);
        PWMOutputInvert(PWM0_BASE, PWM_OUT_1_BIT, false);
    }
    else
    {
        //
        // Set the PWM compare register based on the requested volume.  Since
        // this value is relative to zero, it is correct for any PWM frequency.
        //
        HWREG(PWM0_BASE + PWM_GEN_0_OFFSET + PWM_O_X_CMPB) = ulPercent;
        PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT);

        //
        // Turn on the output since it might have been muted previously.
        //
        PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);
        PWMOutputInvert(PWM0_BASE, PWM_OUT_1_BIT, true);
    }

    //
    // Save the volume for future use (such as un-muting).
    //
    g_ucVolume = ulPercent & 0xff;
}

//*****************************************************************************
//
// Adjusts the audio output up by the specified percentage.
//
//*****************************************************************************
void
AudioVolumeUp(unsigned long ulPercent)
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
    // Set the actual volume if something is playing.
    //
    if(g_pusMusic || g_pusSoundEffect)
    {
        AudioVolume(g_ucVolume);
    }
}

//*****************************************************************************
//
// Adjusts the audio output down by the specified percentage.
//
//*****************************************************************************
void
AudioVolumeDown(unsigned long ulPercent)
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
    // Set the actual volume if something is playing.
    //
    if(g_pusMusic || g_pusSoundEffect)
    {
        AudioVolume(g_ucVolume);
    }
}

//*****************************************************************************
//
// Returns the current volume level.
//
//*****************************************************************************
unsigned char
AudioVolumeGet(void)
{
    //
    // Return the current Audio Volume.
    //
    return(g_ucVolume);
}

//*****************************************************************************
//
// Provides periodic updates to the PWM output in order to produce a sound
// effect or play a song.
//
//*****************************************************************************
void
AudioHandler(void)
{
    unsigned long ulLoop;

    //
    // See if a song is being played.
    //
    if(g_pusMusic)
    {
        //
        // Increment the music counter.
        //
        g_usMusicCount++;

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
            AudioMute();

            //
            // Indicate that there is no longer a song being played.
            //
            g_pusMusic = 0;
        }
        else
        {
            //
            // Set the PWM frequency to the requested frequency.
            //
            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0,
                            (g_ulSystemClock / (g_pusMusic[ulLoop - 1] * 8)));
            PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT);
        }
    }

    //
    // Otherwise, see if a sound effect is being played.
    //
    else if(g_pusSoundEffect)
    {
        //
        // See if the end of the sound effect has been reached.
        //
        if(g_usSoundCount == g_usSoundLength)
        {
            //
            // The sound effect is over, so mute the output.
            //
            AudioMute();

            //
            // Indicate that there is no longer a sound effect being played.
            //
            g_pusSoundEffect = 0;
        }
        else
        {
            //
            // Set the PWM frequency to the next frequency in the sound effect.
            //
            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0,
                            (g_ulSystemClock /
                             (g_pusSoundEffect[g_usSoundCount] * 8)));
            PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT);
        }

        //
        // Increment the sound effect counter.
        //
        g_usSoundCount++;
    }
}

//*****************************************************************************
//
// Turns off audio playback.
//
//*****************************************************************************
void
AudioOff(void)
{
    //
    // Mute the output.
    //
    AudioMute();

    //
    // Cancel any song or sound effect playback that may be in progress.
    //
    g_pusMusic = 0;
    g_pusSoundEffect = 0;
}

//*****************************************************************************
//
// Configures the PWM module for producing audio.
//
//*****************************************************************************
void
AudioOn(void)
{
    //
    // Turn off the PWM generator 0 outputs.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT | PWM_OUT_1_BIT, false);
    PWMGenDisable(PWM0_BASE, PWM_GEN_0);

    //
    // Configure the PWM generator.  Up/down counting mode is used simply to
    // gain an additional bit of range and resolution.
    //
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_SYNC);

    //
    // Mute the audio output.
    //
    AudioMute();

    //
    // Enable the generator.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}

//*****************************************************************************
//
// Starts playback of a song.
//
//*****************************************************************************
void
AudioPlaySong(const unsigned short *pusSong, unsigned long ulLength)
{
    //
    // Stop the playback of any previous song or sound effect.
    //
    g_pusMusic = 0;
    g_pusSoundEffect = 0;

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
    AudioVolume(g_ucVolume);
}

//*****************************************************************************
//
// Starts playback of a sound effect.
//
//*****************************************************************************
void
AudioPlaySound(const unsigned short *pusSound, unsigned long ulLength)
{
    //
    // Stop the playback of any previous song or sound effect.
    //
    g_pusMusic = 0;
    g_pusSoundEffect = 0;

    //
    // Save the length of the sound effect and start the sound effect counter
    // at zero.
    //
    g_usSoundLength = ulLength;
    g_usSoundCount = 0;

    //
    // Save the pointer to the sound effect data.  At this point, the interrupt
    // handler could be called and commence the actual playback.
    //
    g_pusSoundEffect = pusSound;

    //
    // Unmute the audio volume.
    //
    AudioVolume(g_ucVolume);
}
