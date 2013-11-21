//****************************************************************************
//
// sound_task.c - Sound playing task for the qs-autonomous example.
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//****************************************************************************

#include "inc/hw_types.h"
#include "drivers/wav.h"
#include "drivers/sound.h"
#include "utils/scheduler.h"

//****************************************************************************
//
// Defines the possible states of the sound task state machine.
//
//****************************************************************************
#define SOUND_STATE_IDLE 0
#define SOUND_STATE_PLAYING 1

//****************************************************************************
//
// Points to the currently playing audio clip, and the next one to be played.
//
//****************************************************************************
const unsigned char *pucNowPlaying;
const unsigned char *pucNextPlaying;

//****************************************************************************
//
// Holds the wave header information for the currently opened audio clip.
//
//****************************************************************************
tWaveHeader sSoundEffectHeader;

//****************************************************************************
//
// This function is the sound "task" that is called periodically by the
// scheduler from the app main processing loop.  This task plays audio clips
// that are queued up bby the SoundTaskPlay() function.
//
//****************************************************************************
void
SoundTask(void *pvParam)
{
    static unsigned long ulState = SOUND_STATE_IDLE;

    //
    // Process the state machine
    //
    switch(ulState)
    {
        //
        // IDLE - not playing a sound
        //
        case SOUND_STATE_IDLE:
        {
            //
            // If there is a new clip ready to play, then start playing it.
            //
            if(pucNextPlaying)
            {
                //
                // Set the current playing clip to match the new clip, and
                // clear the "next" clip pointer.
                //
                pucNowPlaying = pucNextPlaying;
                pucNextPlaying = 0;

                //
                // Open the new clip as a wave file
                //
                if(WaveOpen((unsigned long *)pucNowPlaying,
                            &sSoundEffectHeader) == WAVE_OK)
                {
                    //
                    // If the clip opened okay as a wave file, then start the
                    // clip playing and change our state to PLAYING
                    //
                    ulState = SOUND_STATE_PLAYING;
                    WavePlayStart(&sSoundEffectHeader);
                }

                //
                // Otherwise the clip was not opened successfully in which
                // case clear the current playing clip and remain in IDLE state
                //
                else
                {
                    pucNowPlaying = 0;
                }
            }
            break;
        }

        //
        // PLAYING - a clip is playing
        //
        case SOUND_STATE_PLAYING:
        {
            //
            // Call the function to continue wave playing.  This function must
            // be called periodically, and it will keep the wave driver playing
            // the audio clip until it is finished.
            //
            if(WavePlayContinue(&sSoundEffectHeader))
            {
                //
                // Clip is done playing, so clear the playing clip pointer and
                // set the state back to IDLE.
                //
                pucNowPlaying = 0;
                ulState = SOUND_STATE_IDLE;
            }
            break;
        }

        //
        // default state is an error.  Clear the current and next clip pointers
        // and set the state back to IDLE
        //
        default:
        {
            pucNowPlaying = 0;
            pucNextPlaying = 0;
            ulState = SOUND_STATE_IDLE;
            break;
        }
    }
}

//****************************************************************************
//
// This function perform initializations needed to run the sound task.  It
// should be called during system initialization.
//
//****************************************************************************
void
SoundTaskInit(void *pvParam)
{
    SoundInit();
}

//****************************************************************************
//
// This function is used to queue a wave format audio clip for playing.
//
//****************************************************************************
void
SoundTaskPlay(const unsigned char *pucSound)
{
    //
    // Set the "next" pointer to point at the requested clip.
    //
    pucNextPlaying = pucSound;
}
