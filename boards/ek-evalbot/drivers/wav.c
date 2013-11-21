//*****************************************************************************
//
// wav.c - Functions allowing playback of wav audio files.
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
//*****************************************************************************
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_ints.h"
#include "inc/hw_i2s.h"
#include "driverlib/gpio.h"
#include "driverlib/i2s.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "utils/ustdlib.h"
#include "dac.h"
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

//*****************************************************************************
//
// Audio buffer size and flags definitions  for indicating state of the buffer.
//
//*****************************************************************************
#define AUDIO_BUFFER_SIZE       4096

#define BUFFER_BOTTOM_EMPTY     0x00000001
#define BUFFER_TOP_EMPTY        0x00000002
#define BUFFER_PLAYING          0x00000004

//*****************************************************************************
//
// Allocate the audio clip buffer.
//
//*****************************************************************************
static unsigned char g_pucBuffer[AUDIO_BUFFER_SIZE];
static unsigned char *g_pucDataPtr;
static unsigned long g_ulMaxBufferSize;

//*****************************************************************************
//
// Holds the state of the audio buffer.
//
//*****************************************************************************
static volatile unsigned long g_ulFlags;

//*****************************************************************************
//
// Variables used to track playback position and time.
//
//*****************************************************************************
static unsigned long g_ulBytesPlayed;
static unsigned long g_ulBytesRemaining;
static unsigned short g_usMinutes;
static unsigned short g_usSeconds;

//*****************************************************************************
//
// This function is called by the sound driver when a buffer has been
// played.  It marks the buffer (top half or bottom half) as free.
//
//*****************************************************************************
static void
BufferCallback(void *pvBuffer, unsigned long ulEvent)
{
    //
    // Handle buffer-free event
    //
    if(ulEvent & BUFFER_EVENT_FREE)
    {
        //
        // If pointing at the start of the buffer, then this is the first
        // half, so mark it as free.
        //
        if(pvBuffer == g_pucBuffer)
        {
            g_ulFlags |= BUFFER_BOTTOM_EMPTY;
        }

        //
        // Otherwise it must be the second half of the buffer that is free.
        //
        else
        {
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

    //
    // Loop through the entire array, and convert the 8-bit unsigned data
    // to 8-bit signed.
    //
    for(ulIdx = 0; ulIdx < ulSize; ulIdx++)
    {
        *pucBuffer = ((short)(*pucBuffer)) - 128;
        pucBuffer++;
    }
}

//*****************************************************************************
//
//! Opens a wav file and parses its header information.
//!
//! \param pulAddress is a pointer to the start of the wav format data in
//! memory.
//! \param pWaveHeader is a pointer to a caller-supplied tWaveHeader structure
//! that will be populated by the function.
//!
//! This function is used to parse a wav header and populate a caller-supplied
//! header structure in preparation for playback.  It can also be used to test
//! if a clip is in wav format or not.
//!
//! \return \b WAVE_OK if the file was parsed successfully,
//! \b WAVE_INVALID_RIFF if RIFF information is not supported,
//! \b WAVE_INVALID_CHUNK if the chunk size is not supported,
//! \b WAVE_INVALID_FORMAT if the format is not supported.
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
    // Get a pointer to the RIFF tag.
    //
    pulBuffer = (unsigned long *)pulAddress;

    //
    // Check for valid RIFF.
    //
    if((pulBuffer[0] != RIFF_CHUNK_ID_RIFF) || (pulBuffer[2] != RIFF_TAG_WAVE))
    {
        return(WAVE_INVALID_RIFF);
    }

    //
    // Check for valid chunk.
    //
    if(pulBuffer[3] != RIFF_CHUNK_ID_FMT)
    {
        return(WAVE_INVALID_CHUNK);
    }

    //
    // Read the chunk size and verify it is okay.
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

    //
    // Populate the caller-supplied header structure with the wav format
    // information.
    //
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
    // 1024 samples per ping-pong buffer due to uDMA.
    //
    ulBytesPerSample = (pWaveHeader->usBitsPerSample *
                        pWaveHeader->usNumChannels) >> 3;

    //
    // Cap the maximum buffer size used to be the smaller of 1024 samples,
    // which is the limit of the uDMA controller, or the size of the
    // audio buffer.
    //
    if(((AUDIO_BUFFER_SIZE >> 1) / ulBytesPerSample) > 1024)
    {
        g_ulMaxBufferSize = 1024 * ulBytesPerSample;
    }
    else
    {
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

    //
    // Calculate the duration of the clip
    //
    g_usSeconds = pWaveHeader->ulDataSize / pWaveHeader->ulAvgByteRate;
    g_usMinutes = g_usSeconds / 60;
    g_usSeconds -= g_usMinutes * 60;

    //
    // Set the data pointer to the start of the data
    //
    g_pucDataPtr = (unsigned char *)&pulBuffer[2];

    //
    // Set the number of data bytes in the file.
    //
    g_ulBytesRemaining = pWaveHeader->ulDataSize;

    //
    // Adjust the average bit rate for 8 bit mono files.
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

    //
    // Made it to here ... return success.
    //
    return(WAVE_OK);
}

//*****************************************************************************
//
//! Change the playback state to stop the playback.
//!
//! This function changes the state of playback to ``stopped'' so that the
//! audio clip will stop playing.  It does not stop the clip immediately but
//! instead changes an internal flag so that the audio clip will stop at the
//! next buffer update.  This allows this function to be called from the
//! context of an interrupt handler.
//!
//! \return None.
//
//*****************************************************************************
void
WaveStop(void)
{
    //
    // Clear the flag that indicates audio is playing.
    //
    g_ulFlags &= ~BUFFER_PLAYING;
}

//*****************************************************************************
//
// Read next block of data from audio clip.
//
// \param pWaveHeader is a pointer to wave header data for this audio clip.
// \param pucBuffer is a pointer to a buffer where the next block of data
// will be copied.
//
// This function reads the next block of data from the playing audio clip
// and stores it in the caller-supplied buffer.  If required the data will
// be converted in-place from 8-bit unsigned to 8-bit signed.
//
// \return The number of bytes read from the audio clip and stored in the
// buffer.
//
//*****************************************************************************
static unsigned long
WaveRead(tWaveHeader *pWaveHeader, unsigned char *pucBuffer)
{
    int i;
    unsigned long ulBytesToRead;

    //
    // Either read a half buffer or just the bytes remaining if we are at the
    // end of the file.
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
    // Copy data from the playing audio clip to the caller-supplied buffer.
    // This buffer will be in SRAM which is required in case the data needs
    // 8-bit sign conversion, and also so that the buffer can be handled by
    // uDMA.
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

    //
    // Return the number of bytes that were read from the audio clip into
    // the buffer.
    //
    return(ulBytesToRead);
}

//*****************************************************************************
//
//! Initialize and start playing a wav file.
//!
//! \param pWaveHeader is the wave header structure containing the clip
//! format information.
//!
//! This function will prepare a wave audio clip for playing, using the wave
//! format information passed in the structure pWaveHeader, which was
//! previously populated by a call to WaveOpen().  Once this function is used
//! to prepare the clip for playing, then WavePlayContinue() should be used
//! to cause the clip to actually play on the audio output.
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

    //
    // Enable the Class D amp.  It's turned off when idle to save power.
    //
    SoundClassDEn();
}

//*****************************************************************************
//
//! Continues playback of a wave file previously passed to WavePlayStart().
//!
//! \param pWaveHeader points to the structure containing information on the
//! wave file being played.
//!
//! This function must be called periodically (at least every 40mS) after
//! WavePlayStart() to continue playback of an audio wav file.  It does any
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
    ROM_IntDisable(INT_I2S0);

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
        // Disable the Class D amp to save power.
        //
        SoundClassDDis();
        bRetcode = true;
    }

    //
    // Reenable the I2S interrupt now that we're finished mucking with
    // state and flags.
    //
    ROM_IntEnable(INT_I2S0);

    return(bRetcode);
}

//*****************************************************************************
//
//! Formats a text string showing elapsed and total playing time.
//!
//! \param pWaveHeader is a pointer to the current wave file's header
//!  information.  This structure will have been populated by a previous call
//!  to WaveOpen().
//! \param pcTime points to storage for the returned string.
//! \param ulSize is the size of the buffer pointed to by \e pcTime.
//!
//! This function may be called periodically by an application during the time
//! that a wave file is playing.  It formats a text string containing the
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
//! Returns the current playback status of the wave file.
//!
//! This function may be called to determine whether a wave file is currently
//! playing.
//!
//! \return Returns \b true if a wave file us currently playing or \b false if
//! no file is playing.
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
