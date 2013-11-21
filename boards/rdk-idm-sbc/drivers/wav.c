//*****************************************************************************
//
// wav.c - Functions allowing playback of wav audio clips.
//
// Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_i2s.h"
#include "driverlib/gpio.h"
#include "driverlib/i2s.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "driverlib/debug.h"
#include "driverlib/rom_map.h"
#include "utils/ustdlib.h"
#include "wm8510.h"
#include "sound.h"
#include "wav.h"

//*****************************************************************************
//
//! \addtogroup wav_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Basic wav file RIFF header information used to open and read a wav file.
//
//*****************************************************************************
#define RIFF_CHUNK_ID_RIFF      0x46464952
#define RIFF_CHUNK_ID_FMT       0x20746d66
#define RIFF_CHUNK_ID_DATA      0x61746164

#define RIFF_TAG_WAVE           0x45564157

#define RIFF_FORMAT_UNKNOWN     0x0000
#define RIFF_FORMAT_PCM         0x0001
#define RIFF_FORMAT_MSADPCM     0x0002
#define RIFF_FORMAT_IMAADPCM    0x0011

#define AUDIO_BUFFER_SIZE       4096

#define BUFFER_BOTTOM_EMPTY     0x00000001
#define BUFFER_TOP_EMPTY        0x00000002
#define BUFFER_PLAYING          0x00000004

//
// State information for keep track of time.
//
static unsigned long g_ulBytesPlayed;

//
// Buffer management and flags.
//
static unsigned char g_pucBuffer[AUDIO_BUFFER_SIZE];
unsigned char *g_pucDataPtr;
unsigned long g_ulMaxBufferSize;

//
// Flags used in the g_ulFlags global variable.
//
static volatile unsigned long g_ulFlags;

//
// Globals used to track playback position.
//
static unsigned long g_ulBytesRemaining;
static unsigned short g_usMinutes;
static unsigned short g_usSeconds;

char g_pcTime[40] = "";

//*****************************************************************************
//
// Handles release of audio buffers.
//
//*****************************************************************************
void
BufferCallback(const void *pvBuffer, unsigned long ulEvent)
{
    if(ulEvent & BUFFER_EVENT_FREE)
    {
        if(pvBuffer == g_pucBuffer)
        {
            //
            // Flag if the first half is free.
            //
            g_ulFlags |= BUFFER_BOTTOM_EMPTY;
        }
        else
        {
            //
            // Flag if the second half is free.
            //
            g_ulFlags |= BUFFER_TOP_EMPTY;
        }

        //
        // Update the byte count.
        //
        g_ulBytesPlayed += AUDIO_BUFFER_SIZE >> 1;
    }
}

//*****************************************************************************
//
// Converts unsigned 8 bit data to signed data in place.
//
// \param pucBuffer is a pointer to the input data buffer.
// \param ulSize is the size of the input data buffer.
//
// This function converts the contents of  an 8 bit unsigned buffer to 8 bit
// signed data in place so that the buffer can be passed into the I2S controller
// for playback.
//
// \return None.
//
//*****************************************************************************
static void
WaveConvert8Bit(unsigned char *pucBuffer, unsigned long ulSize)
{
    unsigned long ulIdx;

    for(ulIdx = 0; ulIdx < ulSize; ulIdx++)
    {
        //
        // In place conversion of 8 bit unsigned to 8 bit signed.
        //
        *pucBuffer = ((short)(*pucBuffer)) - 128;
        pucBuffer++;
    }
}

//*****************************************************************************
//
//! Opens a wav audio clip and parses its header information.
//!
//! \param pulAddress is a pointer to the start of the wave audio clip data in
//!        memory.
//! \param pWaveHeader points to a tWaveHeader structure that will be populated
//!        based on values parsed from the wave audio clip data.
//!
//! This function should be used to parse a wave audio clip in preparation for
//! playback.  If \e WAVE_OK is returned, the audio data is valid and in a
//! format that is supported by the driver.  Valid wave audio clips must
//! contain uncompressed mono or stereo PCM samples.
//!
//! \return Returns \e WAVE_OK on success, \e WAVE_INVALID_RIFF if RIFF
//! information is not supported, \e WAVE_INVALID_CHUNK if chunk size is not
//! supported or \e WAVE_INVALID_FORMAT if data format is not supported.
//
//*****************************************************************************
tWaveReturnCode
WaveOpen(unsigned long *pulAddress, tWaveHeader *pWaveHeader)
{
    unsigned long *pulBuffer;
    unsigned short *pusBuffer;
    unsigned long ulChunkSize;
    unsigned long ulBytesPerSample;

    //
    // Look for RIFF tag.
    //
    pulBuffer = (unsigned long *)pulAddress;

    if((pulBuffer[0] != RIFF_CHUNK_ID_RIFF) || (pulBuffer[2] != RIFF_TAG_WAVE))
    {
        return(WAVE_INVALID_RIFF);
    }

    if(pulBuffer[3] != RIFF_CHUNK_ID_FMT)
    {
        return(WAVE_INVALID_CHUNK);
    }

    //
    // Read the format chunk size.
    //
    ulChunkSize = pulBuffer[4];

    if(ulChunkSize > 16)
    {
        return(WAVE_INVALID_CHUNK);
    }

    //
    // Read the next chunk header.
    //
    pulBuffer = (unsigned long *)&pulAddress[5];
    pusBuffer = (unsigned short *)pulBuffer;

    pWaveHeader->usFormat = pusBuffer[0];
    pWaveHeader->usNumChannels =  pusBuffer[1];
    pWaveHeader->ulSampleRate = pulBuffer[1];
    pWaveHeader->ulAvgByteRate = pulBuffer[2];
    pWaveHeader->usBitsPerSample = pusBuffer[7];

    //
    // Reset the byte count.
    //
    g_ulBytesPlayed = 0;

    //
    // Calculate the Maximum buffer size based on format.  There can only be
    // 1024 samples per ping pong buffer due to uDMA.
    //
    ulBytesPerSample = (pWaveHeader->usBitsPerSample *
                        pWaveHeader->usNumChannels) >> 3;

    if(((AUDIO_BUFFER_SIZE >> 1) / ulBytesPerSample) > 1024)
    {
        //
        // The maximum number of DMA transfers was more than 1024 so limit
        // it to 1024 transfers.
        //
        g_ulMaxBufferSize = 1024 * ulBytesPerSample;
    }
    else
    {
        //
        // The maximum number of DMA transfers was not more than 1024.
        //
        g_ulMaxBufferSize = AUDIO_BUFFER_SIZE >> 1;
    }

    //
    // Only mono and stereo supported.
    //
    if(pWaveHeader->usNumChannels > 2)
    {
        return(WAVE_INVALID_FORMAT);
    }

    //
    // Read the next chunk header.
    //
    pulBuffer = (unsigned long *)&pulAddress[5] + (ulChunkSize / 4);
    if(pulBuffer[0] != RIFF_CHUNK_ID_DATA)
    {
        return(WAVE_INVALID_CHUNK);
    }

    //
    // Save the size of the data.
    //
    pWaveHeader->ulDataSize = pulBuffer[1];

    g_usSeconds = pWaveHeader->ulDataSize / pWaveHeader->ulAvgByteRate;
    g_usMinutes = g_usSeconds / 60;
    g_usSeconds -= g_usMinutes * 60;

    g_pucDataPtr = (unsigned char *)&pulBuffer[2];

    //
    // Set the number of data bytes in the clip.
    //
    g_ulBytesRemaining = pWaveHeader->ulDataSize;

    //
    // Adjust the average bit rate for 8 bit mono clips.
    //
    if((pWaveHeader->usNumChannels == 1) && (pWaveHeader->usBitsPerSample == 8))
    {
        pWaveHeader->ulAvgByteRate <<=1;
    }

    //
    // Set the format of the playback in the sound driver.
    //
    SoundSetFormat(pWaveHeader->ulSampleRate, pWaveHeader->usBitsPerSample,
                   pWaveHeader->usNumChannels);

    return(WAVE_OK);
}

//*****************************************************************************
//
//! Stops playback of a wave audio clip.
//!
//! This function may be used to stop playback of any audio clip that is
//! currently being played.
//!
//! \return None.
//
//*****************************************************************************
void
WaveStop(void)
{
    //
    // Stop playing audio.
    //
    g_ulFlags &= ~BUFFER_PLAYING;
}

//*****************************************************************************
//
// This function will handle reading the correct amount from the wav array and
// will also handle converting 8 bit unsigned to 8 bit signed if necessary.
//
// Argument(s) : pWaveHeader is a pointer to the current wave data's header
//                  information.
//               pucBuffer is a pointer to the input data buffer.
//
// Return(s)   : The number of bytes read.
//
//*****************************************************************************
static unsigned long
WaveRead(tWaveHeader *pWaveHeader, unsigned char *pucBuffer)
{
    int i;
    unsigned long ulBytesToRead;

    //
    // Either read a half buffer or just the bytes remaining if we are at the
    // end of the clip.
    //
    if(g_ulBytesRemaining < g_ulMaxBufferSize)
    {
        ulBytesToRead = g_ulBytesRemaining;
    }
    else
    {
        ulBytesToRead = g_ulMaxBufferSize;
    }

    //
    // Copy data from flash to SRAM.  This is needed in case 8-bit audio is
    // used.  In this case, the data must be converted from unsigned to signed.
    //
    for(i = 0; i < ulBytesToRead; i++)
    {
        pucBuffer[i] = g_pucDataPtr[i];
    }

    //
    // Decrement the number of data bytes remaining to be read.
    //
    g_ulBytesRemaining -= ulBytesToRead;

    //
    // Update the global data pointer keeping track of the location in flash.
    //
    g_pucDataPtr += ulBytesToRead;

    //
    // Need to convert the audio from unsigned to signed if 8 bit
    // audio is used.
    //
    if(pWaveHeader->usBitsPerSample == 8)
    {
        WaveConvert8Bit(pucBuffer, ulBytesToRead);
    }

    return(ulBytesToRead);
}

//*****************************************************************************
//
//! Starts playback of an audio clip.
//!
//! \param pWaveHeader points to a structure containing information on the wave
//! audio data that is to be played.
//!
//! This function will start playback of the wave audio data passed in via the
//! psWaveHeader parameter.  The WaveOpen() function should previously have
//! been called to parse the wave data and populate the pWaveHeader structure.
//!
//! \return None.
//
//*****************************************************************************
void
WavePlayStart(tWaveHeader *pWaveHeader)
{
    //
    // Mark both buffers as empty.
    //
    g_ulFlags = BUFFER_BOTTOM_EMPTY | BUFFER_TOP_EMPTY;

    //
    // Indicate that the application is about to start playing.
    //
    g_ulFlags |= BUFFER_PLAYING;
}

//*****************************************************************************
//
//! Continues playback of an audio clip previously passed to WavePlayStart().
//!
//! \param pWaveHeader points to the structure containing information on the
//! audio clip being played.
//!
//! This function must be called periodically (at least every 40mS) after
//! WavePlayStart() to continue playback of a wave audio clip.  It does any
//! necessary housekeeping to keep the DAC supplied with audio data and returns
//! a value indicating when the playback has completed.
//!
//! \return Returns \b true when playback is complete or \b false if more audio
//! data still remains to be played.
//
//*****************************************************************************
tBoolean
WavePlayContinue(tWaveHeader *pWaveHeader)
{
    tBoolean bRetcode;
    unsigned long ulCount;

    //
    // Assume we're not finished until we learn otherwise.
    //
    bRetcode = false;

    //
    // Set a value that we can use to tell whether or not we processed any
    // new data.
    //
    ulCount = 0xFFFFFFFF;

    //
    // Must disable I2S interrupts during this time to prevent state problems.
    //
    IntDisable(INT_I2S0);

    //
    // If the refill flag gets cleared then fill the requested side of the
    // buffer.
    //
    if(g_ulFlags & BUFFER_BOTTOM_EMPTY)
    {
        //
        // Read out the next buffer worth of data.
        //
        ulCount = WaveRead(pWaveHeader, g_pucBuffer);

        //
        // Start the playback for a new buffer.
        //
        SoundBufferPlay(g_pucBuffer, ulCount, BufferCallback);

        //
        // Bottom half of the buffer is now not empty.
        //
        g_ulFlags &= ~BUFFER_BOTTOM_EMPTY;
    }

    if(g_ulFlags & BUFFER_TOP_EMPTY)
    {
        //
        // Read out the next buffer worth of data.
        //
        ulCount = WaveRead(pWaveHeader, &g_pucBuffer[AUDIO_BUFFER_SIZE >> 1]);

        //
        // Start the playback for a new buffer.
        //
        SoundBufferPlay(&g_pucBuffer[AUDIO_BUFFER_SIZE >> 1],
                        ulCount, BufferCallback);

        //
        // Top half of the buffer is now not empty.
        //
        g_ulFlags &= ~BUFFER_TOP_EMPTY;
    }

    //
    // Audio playback is done once the count is below a full buffer.
    //
    if((ulCount < g_ulMaxBufferSize) || (g_ulBytesRemaining == 0))
    {
        //
        // No longer playing audio.
        //
        g_ulFlags &= ~BUFFER_PLAYING;

        //
        // Wait for the buffer to empty.
        //
        while(g_ulFlags != (BUFFER_TOP_EMPTY | BUFFER_BOTTOM_EMPTY))
        {
        }

        //
        // Tell the caller we're finished.
        //
        bRetcode = true;
    }

    //
    // Reenable the I2S interrupt now that we're finished mucking with
    // state and flags.
    //
    IntEnable(INT_I2S0);

    return(bRetcode);
}

//*****************************************************************************
//
//! Formats a text string showing elapsed and total playing time.
//!
//! \param pWaveHeader is a pointer to the current wave audio clip's header
//!  information.  This structure will have been populated by a previous call
//!  to WaveOpen().
//! \param pcTime points to storage for the returned string.
//! \param ulSize is the size of the buffer pointed to by \e pcTime.
//!
//! This function may be called periodically by an application during the time
//! that a wave audio clip is playing.  It formats a text string containing the
//! current playback time and the total length of the audio clip.  The
//! formatted string may be up to 12 bytes in length containing the
//! terminating NULL so, to prevent truncation, \e ulSize must be 12 or larger.
//!
//! \return None.
//
//*****************************************************************************
void
WaveGetTime(tWaveHeader *pWaveHeader, char *pcTime, unsigned long ulSize)
{
    unsigned long ulSeconds;
    unsigned long ulMinutes;

    //
    // Calculate the integer number of minutes and seconds.
    //
    ulSeconds = g_ulBytesPlayed / pWaveHeader->ulAvgByteRate;
    ulMinutes = ulSeconds / 60;
    ulSeconds -= ulMinutes * 60;

    //
    // If for some reason the seconds go over, clip to the right size.
    //
    if(ulSeconds > g_usSeconds)
    {
        ulSeconds = g_usSeconds;
    }

    //
    // Print the time string in the format mm.ss/mm.ss
    //
    usnprintf((char *)pcTime, ulSize, "%d:%02d/%d:%02d", ulMinutes,
              ulSeconds, g_usMinutes, g_usSeconds);
}

//*****************************************************************************
//
//! Returns the current playback status of the wave audio clip.
//!
//! This function may be called to determine whether a wave audio clip is
//! currently playing.
//!
//! \return Returns \b true if a clip is currently playing or \b false if
//! no clip is playing.
//
//*****************************************************************************
tBoolean
WavePlaybackStatus(void)
{
    return((g_ulFlags & BUFFER_PLAYING) ? true : false);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
