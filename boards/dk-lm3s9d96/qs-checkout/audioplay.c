//*****************************************************************************
//
// audioplay.c - WAV file player function for the Tempest checkout application
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/udma.h"
#include "driverlib/interrupt.h"
#include "driverlib/i2s.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/pushbutton.h"
#include "grlib/imgbutton.h"
#include "grlib/canvas.h"
#include "grlib/slider.h"
#include "grlib/container.h"
#include "grlib/listbox.h"
#include "utils/ustdlib.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "images.h"
#include "file.h"
#include "gui_widgets.h"
#include "qs-checkout.h"
#include "grlib_demo.h"
#include "audioplay.h"

//******************************************************************************
//
// Storage for the filename listbox widget string table.
//
//******************************************************************************
#define NUM_LIST_STRINGS 48
const char *g_ppcDirListStrings[NUM_LIST_STRINGS];

//******************************************************************************
//
// Storage for the names of the files in the current directory.  Filenames
// are stored in format "0:/filename.ext".
//
//******************************************************************************
#define MAX_FILENAME_STRING_LEN (3 + 8 + 1 + 3 + 1)
char g_pcFilenames[NUM_LIST_STRINGS][MAX_FILENAME_STRING_LEN];

//******************************************************************************
//
// The following are data structures used by FatFs.
//
//******************************************************************************
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;

//******************************************************************************
//
// The total number of wave files contained in the list box.
//
//******************************************************************************
static unsigned long g_ulWavCount;

//******************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//******************************************************************************
static void OnListBoxChange(tWidget *pWidget, short usSelected);
static void OnSliderChange(tWidget *pWidget, long lValue);
static void OnBtnPlay(tWidget *pWidget);
static int PopulateFileListBox(tBoolean bRedraw);
static void WaveStop(void);
static void OnBtnAudioToHome(tWidget *pWidget);

//******************************************************************************
//
// Widget definitions
//
//******************************************************************************

//******************************************************************************
//
// The listbox used to display directory contents.
//
//******************************************************************************
extern tCanvasWidget g_sListBackground;
extern tCanvasWidget g_sPlayBackground;

ListBox(g_sDirList, &g_sListBackground, 0, 0,
        &g_sKitronix320x240x16_SSD2119,
        0, 30, 125, 174, LISTBOX_STYLE_OUTLINE, ClrBlack, ClrDarkBlue,
        ClrSilver, ClrWhite, ClrWhite, g_pFontCmss12, g_ppcDirListStrings,
        NUM_LIST_STRINGS, 0, OnListBoxChange);

//******************************************************************************
//
// The button used to play/stop a selected file.
//
//******************************************************************************
Canvas(g_sAudioLMSymbol, &g_sAudioScreen, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 130, 112, 155, 95,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_IMG),
       CLR_BACKGROUND, 0, 0, 0, 0, g_pucTISymbol_80x75, 0);

//******************************************************************************
//
// The button used to play/stop a selected file.
//
//******************************************************************************
char g_psPlayText[5] = "Play";
RectangularButton(g_sPlayBtn, &g_sPlayBackground, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 162, 210, 90, 24,
                  ( PB_STYLE_TEXT | PB_STYLE_IMG | PB_STYLE_RELEASE_NOTIFY),
                  0, 0, 0, CLR_TEXT,
                  g_pFontCmss18b, g_psPlayText, g_pucRedButton_90x24_Up,
                  g_pucRedButton_90x24_Down, 0, 0, OnBtnPlay);

//******************************************************************************
//
// The canvas widget acting as the background to the play/stop button.
//
//******************************************************************************
Canvas(g_sPlayBackground, &g_sAudioScreen, &g_sAudioLMSymbol, &g_sPlayBtn,
       &g_sKitronix320x240x16_SSD2119, 190, 210, 90, 30,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//******************************************************************************
//
// The canvas widgets for the wav file information.
//
//******************************************************************************
extern tCanvasWidget g_sWaveInfoBackground;

Canvas(g_sVolume, &g_sWaveInfoBackground, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 85, 42, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite, g_pFontFixed6x8,
       "Volume: ", 0, 0);

char g_pcVolume[6]="100%";
Canvas(g_sWaveVolume, &g_sWaveInfoBackground, &g_sVolume, 0,
       &g_sKitronix320x240x16_SSD2119, 184, 85, 40, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite, g_pFontFixed6x8,
       g_pcVolume, 0, 0);

char g_pcTime[40]="";
Canvas(g_sWaveInfoTime, &g_sWaveInfoBackground, &g_sWaveVolume, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 70, 140, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite, g_pFontFixed6x8,
       g_pcTime, 0, 0);

char g_pcFormat[40]="";
Canvas(g_sWaveInfoSample, &g_sWaveInfoBackground, &g_sWaveInfoTime, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 55, 140, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite, g_pFontCmss12,
       g_pcFormat, 0, 0);

char g_pcFileName[16]="";
Canvas(g_sWaveInfoFileName, &g_sWaveInfoBackground, &g_sWaveInfoSample, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 40, 140, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite, g_pFontCmss12,
       g_pcFileName, 0, 0);

//******************************************************************************
//
// The canvas widget acting as the background for the wav file information.
//
//******************************************************************************
Canvas(g_sWaveInfoBackground, &g_sAudioScreen, &g_sPlayBackground,
       &g_sWaveInfoFileName, &g_sKitronix320x240x16_SSD2119, 130, 30, 155, 80,
       CANVAS_STYLE_OUTLINE | CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite,
       g_pFontCmss12, 0, 0, 0);

//******************************************************************************
//
// The slider widget used for volume control.
//
//******************************************************************************
#define INITIAL_VOLUME_PERCENT 60
Slider(g_sSlider, &g_sAudioScreen, &g_sWaveInfoBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 290, 40, 20, 190, 0, 100,
       INITIAL_VOLUME_PERCENT, (SL_STYLE_IMG | SL_STYLE_BACKG_IMG |
       SL_STYLE_VERTICAL), 0, 0, 0, 0, 0, 0, 0, g_pucRedVertSlider190x20Image,
       g_pucGreenVertSlider190x20Image, OnSliderChange);

//******************************************************************************
//
// The canvas widget acting as the background to the left portion of the
// display.
//
//******************************************************************************
Canvas(g_sListBackground, &g_sAudioScreen, &g_sSlider, &g_sDirList,
       &g_sKitronix320x240x16_SSD2119, 0, 30, 125, 174,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The push button widget used to return to the main menu.
//
//*****************************************************************************
RectangularButton(g_sAudioHomeBtn, &g_sAudioScreen, &g_sListBackground, 0,
                  &g_sKitronix320x240x16_SSD2119, 10, 210, 90, 24,
                  ( PB_STYLE_TEXT | PB_STYLE_IMG | PB_STYLE_RELEASE_NOTIFY),
                   0, 0, 0, CLR_TEXT, g_pFontCmss18b, "Home",
                   g_pucRedButton_90x24_Up, g_pucRedButton_90x24_Down, 0, 0,
                   OnBtnAudioToHome);

//******************************************************************************
//
// State information for keep track of time.
//
//******************************************************************************
static unsigned long g_ulBytesPlayed;
static unsigned long g_ulNextUpdate;

//******************************************************************************
//
// Buffer management and flags.
//
//******************************************************************************
#define AUDIO_BUFFER_SIZE       4096
static unsigned char g_pucBuffer[AUDIO_BUFFER_SIZE];
unsigned long g_ulMaxBufferSize;

//******************************************************************************
//
// Flags used in the g_ulFlags global variable.
//
//******************************************************************************
#define BUFFER_BOTTOM_EMPTY     0x00000001
#define BUFFER_TOP_EMPTY        0x00000002
#define BUFFER_PLAYING          0x00000004
static volatile unsigned long g_ulFlags;

//******************************************************************************
//
// Globals used to track playback position.
//
//******************************************************************************
static unsigned long g_ulBytesRemaining;
static unsigned short g_usMinutes;
static unsigned short g_usSeconds;

//******************************************************************************
//
// Basic wav file RIFF header information used to open and read a wav file.
//
//******************************************************************************
#define RIFF_CHUNK_ID_RIFF      0x46464952
#define RIFF_CHUNK_ID_FMT       0x20746d66
#define RIFF_CHUNK_ID_DATA      0x61746164

#define RIFF_TAG_WAVE           0x45564157

#define RIFF_FORMAT_UNKNOWN     0x0000
#define RIFF_FORMAT_PCM         0x0001
#define RIFF_FORMAT_MSADPCM     0x0002
#define RIFF_FORMAT_IMAADPCM    0x0011

//******************************************************************************
//
// The wav file header information.
//
//******************************************************************************
typedef struct
{
    //
    // Sample rate in bytes per second.
    //
    unsigned long ulSampleRate;

    //
    // The average byte rate for the wav file.
    //
    unsigned long ulAvgByteRate;

    //
    // The size of the wav data in the file.
    //
    unsigned long ulDataSize;

    //
    // The number of bits per sample.
    //
    unsigned short usBitsPerSample;

    //
    // The wav file format.
    //
    unsigned short usFormat;

    //
    // The number of audio channels.
    //
    unsigned short usNumChannels;
}
tWaveHeader;
static tWaveHeader g_sWaveHeader;

//******************************************************************************
//
// Handler for buffers being released.
//
//******************************************************************************
static void
BufferCallback(void *pvBuffer, unsigned long ulEvent)
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

//******************************************************************************
//
// This function can be used to test if a file is a wav file or not and will
// also return the wav file header information in the pWaveHeader structure.
// If the file is a wav file then the psFileObject pointer will contain an
// open file pointer to the wave file ready to be passed into the WavePlay()
// function.
//
//******************************************************************************
static FRESULT
WaveOpen(FIL *psFileObject, const char *pcFileName, tWaveHeader *pWaveHeader)
{
    unsigned long *pulBuffer;
    unsigned short *pusBuffer;
    unsigned long ulChunkSize, ulBytesPerSample;
    unsigned short usCount;
    FRESULT Result;

    pulBuffer = (unsigned long *)g_pucBuffer;
    pusBuffer = (unsigned short *)g_pucBuffer;

    Result = f_open(psFileObject, pcFileName, FA_READ);
    if(Result != FR_OK)
    {
        return(Result);
    }

    //
    // Read the first 12 bytes.
    //
    Result = f_read(psFileObject, g_pucBuffer, 12, &usCount);
    if(Result != FR_OK)
    {
        f_close(psFileObject);
        return(Result);
    }

    //
    // Look for RIFF tag.
    //
    if((pulBuffer[0] != RIFF_CHUNK_ID_RIFF) || (pulBuffer[2] != RIFF_TAG_WAVE))
    {
        f_close(psFileObject);
        return(FR_INVALID_NAME);
    }

    //
    // Read the next chunk header.
    //
    Result = f_read(psFileObject, g_pucBuffer, 8, &usCount);
    if(Result != FR_OK)
    {
        f_close(psFileObject);
        return(Result);
    }

    if(pulBuffer[0] != RIFF_CHUNK_ID_FMT)
    {
        f_close(psFileObject);
        return(FR_INVALID_NAME);
    }

    //
    // Read the format chunk size.
    //
    ulChunkSize = pulBuffer[1];

    if(ulChunkSize > 16)
    {
        f_close(psFileObject);
        return(FR_INVALID_NAME);
    }

    //
    // Read the next chunk header.
    //
    Result = f_read(psFileObject, g_pucBuffer, ulChunkSize, &usCount);
    if(Result != FR_OK)
    {
        f_close(psFileObject);
        return(Result);
    }

    pWaveHeader->usFormat = pusBuffer[0];
    pWaveHeader->usNumChannels =  pusBuffer[1];
    pWaveHeader->ulSampleRate = pulBuffer[1];
    pWaveHeader->ulAvgByteRate = pulBuffer[2];
    pWaveHeader->usBitsPerSample = pusBuffer[7];

    //
    // Reset the byte count.
    //
    g_ulBytesPlayed = 0;
    g_ulNextUpdate = 0;

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
        f_close(psFileObject);
        return(FR_INVALID_NAME);
    }

    //
    // Read the next chunk header.
    //
    Result = f_read(psFileObject, g_pucBuffer, 8, &usCount);
    if(Result != FR_OK)
    {
        f_close(psFileObject);
        return(Result);
    }

    if(pulBuffer[0] != RIFF_CHUNK_ID_DATA)
    {
        f_close(psFileObject);
        return(Result);
    }

    //
    // Save the size of the data.
    //
    pWaveHeader->ulDataSize = pulBuffer[1];

    g_usSeconds = pWaveHeader->ulDataSize/pWaveHeader->ulAvgByteRate;
    g_usMinutes = g_usSeconds/60;
    g_usSeconds -= g_usMinutes*60;

    //
    // Set the number of data bytes in the file.
    //
    g_ulBytesRemaining = pWaveHeader->ulDataSize;

    //
    // Mark both buffers as empty.
    //
    g_ulFlags = BUFFER_BOTTOM_EMPTY | BUFFER_TOP_EMPTY;

    //
    // Decrement the number of data bytes remaining to be read.
    //
    g_ulBytesRemaining -= usCount;

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

    return(FR_OK);
}

//******************************************************************************
//
// Convert an 8 bit unsigned buffer to 8 bit signed buffer in place so that it
// can be passed into the i2s playback.
//
//******************************************************************************
static void
Convert8Bit(unsigned char *pucBuffer, unsigned long ulSize)
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

//******************************************************************************
//
// This function is used to tell when to update the playback times for a file.
// It will only update the screen at 1 second intervals but can be called more
// often with no result.
//
//******************************************************************************
static void
DisplayTime(unsigned long ulForceUpdate)
{
    unsigned long ulSeconds;
    unsigned long ulMinutes;

    //
    // Only display on the screen once per second.
    //
    if((g_ulBytesPlayed >= g_ulNextUpdate) || (ulForceUpdate != 0))
    {
        //
        // Set the next update time to one second later.
        //
        g_ulNextUpdate = g_ulBytesPlayed + g_sWaveHeader.ulAvgByteRate;

        //
        // Calculate the integer number of minutes and seconds.
        //
        ulSeconds = g_ulBytesPlayed/g_sWaveHeader.ulAvgByteRate;
        ulMinutes = ulSeconds/60;
        ulSeconds -= ulMinutes*60;

        //
        // Print the time string in the format mm.ss/mm.ss
        //
        usprintf(g_pcTime, "%2d:%02d/%d:%02d", ulMinutes,ulSeconds,
                 g_usMinutes, g_usSeconds);

        //
        // Display the updated time on the screen.
        //
        if(g_ulCurrentScreen == AUDIO_SCREEN)
        {
            WidgetPaint((tWidget *)&g_sWaveInfoTime);
        }
    }
}

//******************************************************************************
//
// Move to the next wave file in the list box and start it playing.
//
//******************************************************************************
static void
PlayNextFile(void)
{
    short sSelected;

    //
    // Get the current selection from the list box.
    //
    sSelected = ListBoxSelectionGet(&g_sDirList);

    //
    // Is there any selection?
    //
    if(sSelected == -1)
    {
        return;
    }
    else
    {
        //
        // Move to the next selection in the list box, cycling from the end
        // to the beginning if necessary.
        //
        sSelected = (sSelected == (g_ulWavCount - 1)) ? 0: (sSelected + 1);
        ListBoxSelectionSet(&g_sDirList, sSelected);

        //
        // Make sure the list box updates.
        //
        OnListBoxChange((tWidget *)&g_sDirList, sSelected);

        //
        // Play the new file if we can.
        //
        OnBtnPlay((tWidget *)&g_sPlayBtn);
    }
}

//******************************************************************************
//
// This function is used to update the file information area of the screen.
//
//******************************************************************************
static void
UpdateFileInfo(void)
{
    short sSelected;

    //
    // Get the current selection from the list box.
    //
    sSelected = ListBoxSelectionGet(&g_sDirList);

    //
    // Just return if nothing is currently selected.
    //
    if(sSelected == -1)
    {
        //
        // Clear all the strings related to the file or audio format.
        //
        g_pcTime[0] = 0;
        g_pcFormat[0] = 0;
        g_pcFileName[0] = 0;
    }
    //
    // If this is successful then the file passed in was a wav file.
    //
    else if(WaveOpen(&g_sFileObject, g_pcFilenames[sSelected],
                     &g_sWaveHeader) == FR_OK)
    {

        //
        // Update the file name information.
        //
        strncpy(g_pcFileName, g_pcFilenames[sSelected], 16);

        //
        // Print the formatted string so that it can be displayed.
        //
        usprintf(g_pcFormat, "%d Hz %d bit ", g_sWaveHeader.ulSampleRate,
                 g_sWaveHeader.usBitsPerSample);

        //
        // Concatenate the number of channels.
        //
        if(g_sWaveHeader.usNumChannels == 1)
        {
            strcat(g_pcFormat, "Mono");
        }
        else
        {
            strcat(g_pcFormat, "Stereo");
        }

        //
        // Close the file, it will be reopend on play.
        //
        f_close(&g_sFileObject);

        //
        // Update the real display time.
        //
        DisplayTime(0);
    }
    else
    {
        //
        // Update the file name information.
        //
        strncpy(g_pcFileName, g_pcFilenames[sSelected], 16);

        //
        // Set the time and format strings to null strings.
        //
        g_pcTime[0] = 0;
        g_pcFormat[0] = 0;
    }

    //
    // Update all to the files information on the screen.
    //
    if(g_ulCurrentScreen == AUDIO_SCREEN)
    {
        WidgetPaint((tWidget *)&g_sWaveInfoFileName);
        WidgetPaint((tWidget *)&g_sWaveInfoTime);
        WidgetPaint((tWidget *)&g_sWaveInfoSample);
        WidgetPaint((tWidget *)&g_sSlider);
    }
}

//******************************************************************************
//
// This function will handle stopping the playback of audio.  It will not do
// this immediately but will defer stopping audio at a later time.  This allows
// this function to be called from an interrupt handler.
//
//******************************************************************************
static void
WaveStop(void)
{
    //
    // Stop playing audio.
    //
    g_ulFlags &= ~BUFFER_PLAYING;

    //
    // Change the text to indicate that the button is now for play.
    //
    strcpy(g_psPlayText, "Play");
    if(g_ulCurrentScreen == AUDIO_SCREEN)
    {
        WidgetPaint((tWidget *)&g_sPlayBtn);
    }
}

//******************************************************************************
//
// This function will handle reading the correct amount from the wav file and
// will also handle converting 8 bit unsigned to 8 bit signed if necessary.
//
//******************************************************************************
static unsigned short
WaveRead(FIL *psFileObject, tWaveHeader *pWaveHeader, unsigned char *pucBuffer)
{
    unsigned long ulBytesToRead;
    unsigned short usCount;

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
    // Read in another buffer from the sd card.
    //
    if(f_read(&g_sFileObject, pucBuffer, ulBytesToRead, &usCount) != FR_OK)
    {
        return(0);
    }

    //
    // Decrement the number of data bytes remaining to be read.
    //
    g_ulBytesRemaining -= usCount;

    //
    // Need to convert the audio from unsigned to signed if 8 bit
    // audio is used.
    //
    if(pWaveHeader->usBitsPerSample == 8)
    {
        Convert8Bit(pucBuffer, usCount);
    }

    return(usCount);
}

//******************************************************************************
//
// This function should be called periodicaly while a file is playing to ensure
// that the appropriate buffers are kept fed.
//
//******************************************************************************
static unsigned long
WavePlay(tWaveHeader *pWaveHeader)
{
    static unsigned short usCount = 0;

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
        // Bottom half of the buffer is now not empty.
        //
        g_ulFlags &= ~BUFFER_BOTTOM_EMPTY;

        //
        // Read out the next buffer worth of data.
        //
        usCount = WaveRead(&g_sFileObject, pWaveHeader, g_pucBuffer);

        //
        // Start the playback for a new buffer.
        //
        SoundBufferPlay(g_pucBuffer, usCount, BufferCallback);
    }

    if(g_ulFlags & BUFFER_TOP_EMPTY)
    {
        //
        // Top half of the buffer is now not empty.
        //
        g_ulFlags &= ~BUFFER_TOP_EMPTY;

        //
        // Read out the next buffer worth of data.
        //
        usCount = WaveRead(&g_sFileObject, pWaveHeader,
                           &g_pucBuffer[AUDIO_BUFFER_SIZE >> 1]);

        //
        // Start the playback for a new buffer.
        //
        SoundBufferPlay(&g_pucBuffer[AUDIO_BUFFER_SIZE >> 1],
                        usCount, BufferCallback);

        //
        // Update the current time display.
        //
        DisplayTime(0);
    }

    //
    // If something reset this while playing then stop playing.
    //
    if((g_ulFlags & BUFFER_PLAYING) == 0)
    {
        //
        // Stop requesting transfers.
        //
        I2STxDisable(I2S0_BASE);

        //
        // Close out the file.
        //
        f_close(&g_sFileObject);

        //
        // Change the text to indicate that the button is now for play.
        //
        strcpy(g_psPlayText, "Play");
        if(g_ulCurrentScreen == AUDIO_SCREEN)
        {
            WidgetPaint((tWidget *)&g_sPlayBtn);
        }

        //
        // Update the file information panel now that we have stopped.
        //
        UpdateFileInfo();

        return(0);
    }

    //
    // Audio playback is done once the count is below a full buffer.
    //
    if((usCount < g_ulMaxBufferSize) || (g_ulBytesRemaining == 0))
    {
        //
        // Close out the file.
        //
        f_close(&g_sFileObject);

        //
        // Change the text to indicate that the button is now for play.
        //
        strcpy(g_psPlayText, "Play");
        if(g_ulCurrentScreen == AUDIO_SCREEN)
        {
            WidgetPaint((tWidget *)&g_sPlayBtn);
        }

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
        // Force update the current time display.
        //
        DisplayTime(1);

        //
        // Stop requesting transfers.
        //
        I2STxDisable(I2S0_BASE);

        //
        // Move on to the next wav file in the list (if one exists).
        //
        PlayNextFile();

        return(0);
    }

    //
    // Re-enable I2S interrupts now that we are finished playing with the
    // buffers.
    //
    ROM_IntEnable(INT_I2S0);

    return(0);
}

//******************************************************************************
//
// The listbox widget callback function.
//
// This function is called whenever someone changes the selected entry in the
// listbox containing the files.
//
//******************************************************************************
static void
OnListBoxChange(tWidget *pWidget, short usSelected)
{
    //
    // Update only if playing a file.
    //
    if(g_ulFlags & BUFFER_PLAYING)
    {
        //
        // We stop any file which is currently playing if the user selects a
        // new one.
        //
        WaveStop();
    }

    //
    // Update the file info area.
    //
    UpdateFileInfo();
}

//******************************************************************************
//
// The "Play/Stop" button widget callback function.
//
// This function is called whenever someone presses the "Play/Stop" button.
//
//******************************************************************************
static void
OnBtnPlay(tWidget *pWidget)
{
    short sSelected;

    //
    // If already playing then this was a press to stop playback.
    //
    if(g_ulFlags & BUFFER_PLAYING)
    {
        WaveStop();
    }
    else
    {
        //
        // Get the current selection from the list box.
        //
        sSelected = ListBoxSelectionGet(&g_sDirList);

        //
        // Is there any selection?
        //
        if(sSelected == -1)
        {
            return;
        }
        else
        {
            if(WaveOpen(&g_sFileObject, g_pcFilenames[sSelected], &g_sWaveHeader)
                   == FR_OK)
            {
                //
                // Change the text on the button to Stop.
                //
                strcpy(g_psPlayText, "Stop");
                if(g_ulCurrentScreen == AUDIO_SCREEN)
                {
                    WidgetPaint((tWidget *)&g_sPlayBtn);
                }

                //
                // Indicate that wave playback should start.
                //
                g_ulFlags |= BUFFER_PLAYING;
            }
        }
    }
}

//******************************************************************************
//
// Handle changes in the volume slider.
//
//******************************************************************************
static void
OnSliderChange(tWidget *pWidget, long lValue)
{
    //
    // Make sure the correct volume level is displayed.
    //
    usnprintf(g_pcVolume, 6, "%d%%", lValue);

    //
    // Display the updated volume on the screen.
    //
    if(g_ulCurrentScreen == AUDIO_SCREEN)
    {
        WidgetPaint((tWidget *)&g_sWaveVolume);
    }

    //
    // Set the volume at the audio DAC.
    //
    SoundVolumeSet(lValue);
}

//******************************************************************************
//
// This function is called to read the contents of the root directory on
// a given FAT logical drive and fill the listbox containing the names of all
// audio WAV files found.
//
//******************************************************************************
static int
FindWaveFilesOnDrive(const char *pcDrive, int iStartIndex)
{
    FRESULT fresult;
    int iCount;

    //
    // Open the current directory for access.
    //
    fresult = f_opendir(&g_sDirObject, pcDrive);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        return(0);
    }

    //
    // Start by inserting at the next entry in the list box.
    //
    iCount = iStartIndex;

    //
    // Enter loop to enumerate through all directory entries.
    //
    while(1)
    {
        //
        // Read an entry from the directory.
        //
        fresult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(fresult != FR_OK)
        {
            return(0);
        }

        //
        // If the file name is blank, then this is the end of the
        // listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

        //
        // Add the information as a line in the listbox widget if there is
        // space left and the filename contains ".wav".
        //
        if((g_ulWavCount < NUM_LIST_STRINGS) &&
           ((ustrstr(g_sFileInfo.fname, ".wav")) ||
            (ustrstr(g_sFileInfo.fname, ".WAV"))))
        {
            //
            // Ignore directories.
            //
            if((g_sFileInfo.fattrib & AM_DIR) == 0)
            {
                usnprintf(g_pcFilenames[iCount], MAX_FILENAME_STRING_LEN,
                          "%s%s", pcDrive, g_sFileInfo.fname);
                ListBoxTextAdd(&g_sDirList, g_pcFilenames[iCount]);

                //
                // Move on to the next entry in the list box.
                //
                iCount++;
            }
        }
    }

    //
    // Made it to here, return the number of files we found.
    //
    return(iCount);
}

//******************************************************************************
//
// This function is called to read the contents of the current directory on
// the SD card and fill the listbox containing the names of all files.
//
//******************************************************************************
static int
PopulateFileListBox(tBoolean bRepaint)
{
    //
    // Empty the list box on the display.
    //
    ListBoxClear(&g_sDirList);

    //
    // Make sure the list box will be redrawn next time the message queue
    // is processed.
    //
    if(bRepaint && (g_ulCurrentScreen == AUDIO_SCREEN))
    {
        WidgetPaint((tWidget *)&g_sDirList);
    }

    //
    // How many files can we find on the SD card (if present)?
    //
    g_ulWavCount = (unsigned long)FindWaveFilesOnDrive("0:/", 0);

    //
    // Now add the file we find on the USB stick (if it's there).
    //
    g_ulWavCount = (unsigned long)FindWaveFilesOnDrive("1:/", g_ulWavCount);

    //
    // Did we find any files at all?
    //
    return(g_ulWavCount ? 0 : FR_NO_FILE);
}

//*****************************************************************************
//
// A simple demonstration of the features of the Stellaris Graphics Library.
//
//*****************************************************************************
void
AudioPlayerInit(void)
{
    //
    // Set the initial volume to something sensible.  Beware - if you make the
    // mistake of using 24 ohm headphones and setting the volume to 100% you
    // may find it is rather too loud!
    //
    SoundVolumeSet(INITIAL_VOLUME_PERCENT);

    //
    // Make sure the correct volume level is displayed.
    //
    usnprintf(g_pcVolume, 6, "%d%%", INITIAL_VOLUME_PERCENT);

    //
    // Add the player widgets to the main application widget tree.
    //
    WidgetAdd((tWidget *)&g_sAudioScreen, (tWidget *)&g_sAudioHomeBtn);
}

//*****************************************************************************
//
// This handler is called whenever the "Image Viewer" button is released.  It
// sets up the widget tree to show the relevant controls.
//
//*****************************************************************************
void
OnBtnShowAudioScreen(tWidget *pWidget)
{
    //
    // Fill the list box with the available WAV files.
    //
    PopulateFileListBox(true);

    //
    // Clear the audio information box.
    //
    UpdateFileInfo();

    //
    // Switch to the image viewer screen.
    //
    ShowUIScreen(AUDIO_SCREEN);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);

    //
    // We are not playing anything just now.
    //
    g_ulFlags = 0;
}

//*****************************************************************************
//
// This handler is called whenever the "Home" button is released.  It stops
// any playback that is currently going on and returns the display to the home
// screen.
//
//*****************************************************************************
static void
OnBtnAudioToHome(tWidget *pWidget)
{
    //
    // If we are currently playing a file, stop playback.
    //
    if(g_ulFlags & BUFFER_PLAYING)
    {
        WaveStop();
    }

    //
    // Return to the main menu screen.
    //
    ShowUIScreen(HOME_SCREEN);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);
}

//*****************************************************************************
//
// This function is called periodically from the application's main loop.  If
// a new audio file is to be played, it plays the file and returns once it is
// done.
//
//*****************************************************************************
void
AudioProcess(void)
{
    //
    // If wav playback has started let the WavePlay routine take over main
    // routine.  This hijacks the main loop for the period of time it takes
    // to play the WAV file - not very friendly, sorry.
    //
    if(g_ulFlags & BUFFER_PLAYING)
    {
        //
        // Try to play Wav file.
        //
        WavePlay(&g_sWaveHeader);
    }
}
