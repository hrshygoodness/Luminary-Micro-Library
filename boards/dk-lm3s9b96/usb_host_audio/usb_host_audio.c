//*****************************************************************************
//
// usb_host_audio.c - Main routine for the USB host audio example.
//
// Copyright (c) 2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/slider.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "utils/ustdlib.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/usb_sound.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "drivers/wavfile.h"
#include "usblib/usblib.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB host audio example application using SD Card FAT file system
//! (usb_host_audio)</h1>
//!
//! This example application demonstrates playing .wav files from an SD card
//! that is formatted with a FAT file system using USB host audio class.  The
//! application will only look in the root directory of the SD card and display
//! all files that are found.  Files can be selected to show their format and
//! then played if the application determines that they are a valid .wav file.
//! Only PCM format (uncompressed) files may be played.
//!
//! For additional details about FatFs, see the following site:
//! http://elm-chan.org/fsw/ff/00index_e.html
//
//*****************************************************************************

//*****************************************************************************
//
// Our running system tick counter and a global used to determine the time
// elapsed since last call to GetTickms().
//
//*****************************************************************************
static unsigned long g_ulSysTickCount;
static unsigned long g_ulLastTick;
static unsigned long GetTickms(void);

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;

//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define USB_INT_PRIORITY        0x00
#define SYSTICK_INT_PRIORITY    0x40
#define ADC3_INT_PRIORITY       0x80

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Storage for the filename listbox widget string table.
//
//*****************************************************************************
#define NUM_LIST_STRINGS 48
const char *g_ppcDirListStrings[NUM_LIST_STRINGS];

//*****************************************************************************
//
// Storage for the names of the files in the current directory.  Filenames
// are stored in format "filename.ext".
//
//*****************************************************************************
#define MAX_FILENAME_STRING_LEN (8 + 1 + 3 + 1)
char g_pcFilenames[NUM_LIST_STRINGS][MAX_FILENAME_STRING_LEN];

//*****************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//*****************************************************************************
static void OnListBoxChange(tWidget *pWidget, short usSelected);
static void OnBtnPlay(tWidget *pWidget);
static int PopulateFileListBox(tBoolean bRedraw);

//*****************************************************************************
//
// Audio buffering definitions, these are optimized to deal with USB audio.
//
//*****************************************************************************
#define AUDIO_TRANSFER_SIZE     (192)
#define AUDIO_BUFFERS           (16)
#define AUDIO_BUFFER_SIZE       (AUDIO_TRANSFER_SIZE*AUDIO_BUFFERS)
unsigned long g_ulTransferSize;
unsigned long g_ulBufferSize;

//*****************************************************************************
//
// The main audio buffer and it's pointers.
//
//*****************************************************************************
unsigned char g_pucAudioBuffer[AUDIO_BUFFER_SIZE];
unsigned char *g_pucRead;
unsigned char *g_pucWrite;

//*****************************************************************************
//
// Holds global flags for the system.
//
//*****************************************************************************
int g_ulFlags = 0;

//
// Currently streaming audio to the USB device.
//
#define FLAGS_PLAYING           1

//
// The last transfer has competed so a new one can be started.
//
#define FLAGS_TX_COMPLETE       2

//
// New audio device present.
//
#define FLAGS_DEVICE_CONNECT    3

//
// New audio device present.
//
#define FLAGS_DEVICE_READY      4

//*****************************************************************************
//
// These are the global .wav file states used by the application.
//
//*****************************************************************************
tWavFile g_sWavFile;
tWavHeader g_sWavHeader;

//*****************************************************************************
//
// Widget definitions
//
//*****************************************************************************

//*****************************************************************************
//
// The list box used to display directory contents.
//
//*****************************************************************************
extern tCanvasWidget g_sListBackground;
extern tCanvasWidget g_sPlayBackground;

ListBox(g_sDirList, &g_sListBackground, 0, 0,
        &g_sKitronix320x240x16_SSD2119,
        0, 30, 125, 180, LISTBOX_STYLE_OUTLINE, ClrBlack, ClrDarkBlue,
        ClrSilver, ClrWhite, ClrWhite, &g_sFontCmss12, g_ppcDirListStrings,
        NUM_LIST_STRINGS, 0, OnListBoxChange);


//*****************************************************************************
//
// The button used to play/stop a selected file.
//
//*****************************************************************************
char g_psPlayText[5] = "Play";
RectangularButton(g_sPlayBtn, &g_sPlayBackground, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 220, 180, 90, 30,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlack, ClrBlue, ClrWhite, ClrWhite,
                   &g_sFontCm20, g_psPlayText, 0, 0, 0, 0, OnBtnPlay);

//*****************************************************************************
//
// The canvas widget acting as the background to the play/stop button.
//
//*****************************************************************************
Canvas(g_sPlayBackground, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 190, 180, 90, 30,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The canvas widgets for the wav file information.
//
//*****************************************************************************
extern tCanvasWidget g_sWaveInfoBackground;

char g_pcTime[40]="";
Canvas(g_sWaveInfoTime, &g_sWaveInfoBackground, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 70, 140, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite, &g_sFontFixed6x8,
       g_pcTime, 0, 0);

char g_pcFormat[40]="";
Canvas(g_sWaveInfoSample, &g_sWaveInfoBackground, &g_sWaveInfoTime, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 55, 140, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite, &g_sFontCmss12,
       g_pcFormat, 0, 0);

char g_pcFileName[16]="";
Canvas(g_sWaveInfoFileName, &g_sWaveInfoBackground, &g_sWaveInfoSample, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 40, 140, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, ClrWhite, ClrWhite, &g_sFontCmss12,
       g_pcFileName, 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background for the wav file information.
//
//*****************************************************************************
Canvas(g_sWaveInfoBackground, WIDGET_ROOT, &g_sPlayBackground,
       &g_sWaveInfoFileName, &g_sKitronix320x240x16_SSD2119, 130, 30, 190, 80,
       CANVAS_STYLE_OUTLINE | CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite,
       &g_sFontCmss12, 0, 0, 0);

//*****************************************************************************
//
// The slider widget used for volume control.
//
//*****************************************************************************
#define INITIAL_VOLUME_PERCENT 60

//*****************************************************************************
//
// The canvas widget acting as the background to the left portion of the
// display.
//
//*****************************************************************************
Canvas(g_sListBackground, WIDGET_ROOT, &g_sWaveInfoBackground, &g_sDirList,
       &g_sKitronix320x240x16_SSD2119, 0, 30, 125, 180,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The status line.
//
//*****************************************************************************
#define STATUS_TEXT_SIZE        40
char g_psStatusText[STATUS_TEXT_SIZE] = "";
Canvas(g_sStatus, WIDGET_ROOT, &g_sListBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 240-24, 320, 24,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT |
        CANVAS_STYLE_TEXT_LEFT),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontCm20, g_psStatusText, 0, 0);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sStatus, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 24,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontCm20, "usb host audio", 0, 0);

//*****************************************************************************
//
// State information for keep track of time.
//
//*****************************************************************************
static unsigned long g_ulBytesPlayed;
static unsigned long g_ulNextUpdate;

//*****************************************************************************
//
// Globals used to track play back position.
//
//*****************************************************************************
static unsigned short g_usMinutes;
static unsigned short g_usSeconds;

//*****************************************************************************
//
// This function handles the callback from the USB audio device when a buffer
// has been played or a new buffer has been received.
//
//*****************************************************************************
static void
USBAudioOutCallback(void *pvBuffer, unsigned long ulEvent)
{
    //
    // If a buffer has been played then schedule a new one to play.
    //
    if((ulEvent == USB_EVENT_TX_COMPLETE) &&
       (HWREGBITW(&g_ulFlags, FLAGS_PLAYING)))
    {
        //
        // Indicate that a transfer was complete so that the non-interrupt
        // code can read in more data from the file.
        //
        HWREGBITW(&g_ulFlags, FLAGS_TX_COMPLETE) = 1;

        //
        // Increment the read pointer.
        //
        g_pucRead += g_ulTransferSize;

        //
        // Wrap the read pointer if necessary.
        //
        if(g_pucRead >= (g_pucAudioBuffer + g_ulBufferSize))
        {
            g_pucRead = g_pucAudioBuffer;
        }

        //
        // Increment the number of bytes that have been played.
        //
        g_ulBytesPlayed += g_ulTransferSize;

        //
        // Schedule a new USB audio buffer to be transmitted to the USB
        // audio device.
        //
        USBSoundBufferOut(g_pucRead, g_ulTransferSize, USBAudioOutCallback);
    }
}

//*****************************************************************************
//
// This function is used to tell when to update the play back times for a file.
// It will only update the screen at 1 second intervals but can be called more
// often with no result.
//
//*****************************************************************************
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
        g_ulNextUpdate = g_ulBytesPlayed + g_sWavHeader.ulAvgByteRate;

        //
        // Calculate the integer number of minutes and seconds.
        //
        ulSeconds = g_ulBytesPlayed/g_sWavHeader.ulAvgByteRate;
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
        WidgetPaint((tWidget *)&g_sWaveInfoTime);
    }
}

//*****************************************************************************
//
// This function is used to update the file information area of the screen.
//
//*****************************************************************************
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
        // Set the time and format strings to null strings.
        //
        g_pcTime[0] = 0;
        g_pcFormat[0] = 0;
    }
    //
    // If this is successful then the file passed in was a wav file.
    //
    else if(WavOpen(g_pcFilenames[sSelected],&g_sWavFile) == 0)
    {
        //
        // Read the .wav file format.
        //
        WavGetFormat(&g_sWavFile, &g_sWavHeader);

        //
        // Update the file name information.
        //
        strncpy(g_pcFileName, g_pcFilenames[sSelected], 16);

        //
        // Print the formatted string so that it can be displayed.
        //
        usprintf(g_pcFormat, "%d Hz %d bit ", g_sWavHeader.ulSampleRate,
                 g_sWavHeader.usBitsPerSample);

        //
        // Concatenate the number of channels.
        //
        if(g_sWavHeader.usNumChannels == 1)
        {
            strcat(g_pcFormat, "Mono");
        }
        else
        {
            strcat(g_pcFormat, "Stereo");
        }

        //
        // Calculate the minutes and seconds in the file.
        //
        g_usSeconds = g_sWavHeader.ulDataSize / g_sWavHeader.ulAvgByteRate;
        g_usMinutes = g_usSeconds / 60;
        g_usSeconds -= g_usMinutes * 60;

        //
        // Close the file, it will be re-opened on play.
        //
        WavClose(&g_sWavFile);

        //
        // Update the file time information.
        //
        DisplayTime(1);
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
    WidgetPaint((tWidget *)&g_sWaveInfoFileName);
    WidgetPaint((tWidget *)&g_sWaveInfoTime);
    WidgetPaint((tWidget *)&g_sWaveInfoSample);
}

//*****************************************************************************
//
// This function will handle stopping the play back of audio.  It will not do
// this immediately but will defer stopping audio at a later time.  This allows
// this function to be called from an interrupt handler.
//
//*****************************************************************************
static void
WaveStop(void)
{
    int iIdx;
    unsigned long *pulBuffer;

    //
    // Stop playing audio.
    //
    HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 0;

    //
    // Do unsigned long accesses to clear out the buffer.
    //
    pulBuffer = (unsigned long *)g_pucAudioBuffer;

    //
    // Zero out the buffer.
    //
    for(iIdx = 0; iIdx < (AUDIO_BUFFER_SIZE >> 2); iIdx++)
    {
        pulBuffer[iIdx] = 0;
    }

    //
    // Reset the number of bytes played and for a time update on the screen.
    //
    g_ulBytesPlayed = 0;
    DisplayTime(1);

    //
    // Change the text on the button to Stop.
    //
    strcpy(g_psPlayText, "Play");
    WidgetPaint((tWidget *)&g_sPlayBtn);
}

//*****************************************************************************
//
// This will play the file passed in via the psWaveFile parameter based on
// the format passed in the pWaveHeader structure.  The WavOpen() function
// can be used to properly fill the pWaveHeader and psWaveFile structures.
//
//*****************************************************************************
static void
WavePlay(tWavHeader *pWaveHeader)
{
    short sSelected;

    //
    // Don't play anything but 16 bit audio since most USB devices do not
    // support 8 bit formats.
    //
    if(pWaveHeader->usBitsPerSample != 16)
    {
        return;
    }

    //
    // Get the current selection from the list box.
    //
    sSelected = ListBoxSelectionGet(&g_sDirList);

    //
    // Is there any selection?
    //
    if(sSelected == -1)
    {
        HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 0;
        return;
    }

    //
    // See if this is a valid .wav file that can be opened.
    //
    if(WavOpen(g_pcFilenames[sSelected], &g_sWavFile) == 0)
    {
        //
        // Change the text on the button to Stop.
        //
        strcpy(g_psPlayText, "Stop");
        WidgetPaint((tWidget *)&g_sPlayBtn);

        //
        // Indicate that wave play back should start.
        //
        HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 1;
    }
    else
    {
        HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 0;
        return;
    }

    //
    // Initialize the read and write pointers.
    //
    g_pucRead = g_pucAudioBuffer;
    g_pucWrite = g_pucAudioBuffer;

    while(HWREGBITW(&g_ulFlags, FLAGS_PLAYING))
    {
        //
        // Handle the transmit complete event.
        //
        if(HWREGBITW(&g_ulFlags, FLAGS_TX_COMPLETE))
        {
            //
            // Clear the transmit complete flag.
            //
            HWREGBITW(&g_ulFlags, FLAGS_TX_COMPLETE) = 0;

            //
            // If the read pointer has reached the top of the buffer then
            // fill in the top half or bottom half of the audio buffer.
            //
            if(g_pucRead == g_pucAudioBuffer)
            {
                //
                // Read new data into the bottom half since the audio is
                // play back is reading from the top of the buffer.
                //
                if(WavRead(&g_sWavFile,
                           g_pucAudioBuffer + (g_ulBufferSize >> 1),
                           g_ulBufferSize >> 1) == 0)
                {
                    //
                    // No more data or error so stop playing.
                    //
                    break;
                }

                //
                // Move the write pointer to the top of the audio buffer.
                //
                g_pucWrite = g_pucAudioBuffer;
            }
            else if(g_pucRead == g_pucAudioBuffer + (g_ulBufferSize >> 1))
            {
                //
                // Read new data into the top half since the audio is
                // play back is reading from the bottom of the buffer.
                //
                if(WavRead(&g_sWavFile, g_pucAudioBuffer,
                           (g_ulBufferSize >> 1)) == 0)
                {
                    //
                    // No more data or error so stop playing.
                    //
                    break;
                }

                //
                // Move the write pointer to the middle of the audio buffer.
                //
                g_pucWrite =  g_pucAudioBuffer + (g_ulBufferSize >> 1);
            }

            //
            // Update the real display time.
            //
            DisplayTime(0);
        }

        //
        // Check for stalled audio if still playing audio.  The audio has
        // stalled if the buffers have become equal and needs to be restarted.
        //
        if((HWREGBITW(&g_ulFlags, FLAGS_PLAYING)) &&
           (g_pucRead == g_pucAudioBuffer))
        {
            USBSoundBufferOut(g_pucRead, g_ulTransferSize, USBAudioOutCallback);
        }

        //
        // Need to periodically call the USBMain() routine so that
        // non-interrupt gets a chance to run.
        //
        USBMain(GetTickms());

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }

    //
    // Stop audio play back and return.
    //
    WaveStop();

    return;
}

//*****************************************************************************
//
// The list box widget call back function.
//
// This function is called whenever someone changes the selected entry in the
// list box containing the files.
//
//*****************************************************************************
static void
OnListBoxChange(tWidget *pWidget, short usSelected)
{
    //
    // Update only if playing a file.
    //
    if(HWREGBITW(&g_ulFlags, FLAGS_PLAYING) == 0)
    {
        //
        // Update the file info area.
        //
        UpdateFileInfo();
    }
    else
    {
        //
        // Should never be playing if the selection changes.
        //
        WaveStop();
    }
}

//*****************************************************************************
//
// The "Play/Stop" button widget call back function.
//
// This function is called whenever someone presses the "Play/Stop" button.
//
//*****************************************************************************
static void
OnBtnPlay(tWidget *pWidget)
{
    //
    // Determine if this was a Play or Stop command.
    //
    if(HWREGBITW(&g_ulFlags, FLAGS_PLAYING))
    {
        //
        // If already playing then this was a press to stop play back.
        //
        WaveStop();
    }
    else if(HWREGBITW(&g_ulFlags, FLAGS_DEVICE_READY))
    {
        //
        // Indicate that wave play back should start if there is an audio
        // device present.
        //
        HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 1;
    }
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a
// timer tick every 10ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Increment the system tick count.
    //
    g_ulSysTickCount++;

    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();
}

//*****************************************************************************
//
// This function returns the number of ticks since the last time this function
// was called.
//
//*****************************************************************************
static unsigned long
GetTickms(void)
{
    unsigned long ulRetVal;
    unsigned long ulSaved;

    ulRetVal = g_ulSysTickCount;
    ulSaved = ulRetVal;

    if(ulSaved > g_ulLastTick)
    {
        ulRetVal = ulSaved - g_ulLastTick;
    }
    else
    {
        ulRetVal = g_ulLastTick - ulSaved;
    }

    //
    // This could miss a few milliseconds but the timings here are on a
    // much larger scale.
    //
    g_ulLastTick = ulSaved;

    //
    // Return the number of milliseconds since the last time this was called.
    //
    return(ulRetVal * MS_PER_SYSTICK);
}

//*****************************************************************************
//
// This function is called to read the contents of the current directory on
// the SD card and fill the list box containing the names of all files.
//
//*****************************************************************************
static int
PopulateFileListBox(tBoolean bRepaint)
{
    unsigned long ulItemCount;
    FRESULT fresult;

    //
    // Empty the list box on the display.
    //
    ListBoxClear(&g_sDirList);

    //
    // Make sure the list box will be redrawn next time the message queue
    // is processed.
    //
    if(bRepaint)
    {
        WidgetPaint((tWidget *)&g_sDirList);
    }

    //
    // Open the current directory for access.
    //
    fresult = f_opendir(&g_sDirObject, "/");

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        return(fresult);
    }

    ulItemCount = 0;

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
            return(fresult);
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
        // Add the information as a line in the listbox widget.
        //
        if(ulItemCount < NUM_LIST_STRINGS)
        {
            //
            // Ignore directories and non .wav files.
            //
            if(((g_sFileInfo.fattrib & AM_DIR) == 0) &&
               (WavOpen(g_sFileInfo.fname, &g_sWavFile) == 0))
            {
                //
                // Read the .wav file format.
                //
                WavGetFormat(&g_sWavFile, &g_sWavHeader);

                //
                // Only 16 bit files are supported.
                //
                if(g_sWavHeader.usBitsPerSample == 16)
                {
                    strncpy(g_pcFilenames[ulItemCount], g_sFileInfo.fname,
                             MAX_FILENAME_STRING_LEN);
                    ListBoxTextAdd(&g_sDirList, g_pcFilenames[ulItemCount]);

                    //
                    // Move to the next entry in the item array we use to
                    // populate the list box.
                    //
                    ulItemCount++;
                }
                WavClose(&g_sWavFile);
            }
        }
    }

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//*****************************************************************************
//
// This function handled global level events for the USB host audio.  This
// function was passed into the USBSoundInit() function.
//
//*****************************************************************************
static void
AudioEvent(unsigned long ulEvent, unsigned long ulParam)
{
    switch(ulEvent)
    {
        case SOUND_EVENT_READY:
        {
            //
            // Flag that a new audio device is present.
            //
            HWREGBITW(&g_ulFlags, FLAGS_DEVICE_CONNECT) = 1;

            break;
        }
        case SOUND_EVENT_DISCONNECT:
        {
            //
            // Device is no longer present.
            //
            HWREGBITW(&g_ulFlags, FLAGS_DEVICE_READY) = 0;
            HWREGBITW(&g_ulFlags, FLAGS_DEVICE_CONNECT) = 0;
            HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 0;

            //
            // Change the text to reflect the change.
            //
            strcpy(g_psStatusText, "No Device");
            WidgetPaint((tWidget *)&g_sStatus);

            break;
        }
        case SOUND_EVENT_UNKNOWN_DEV:
        {
            //
            // Unknown device connected.
            //
            strcpy(g_psStatusText, "Unknown Device");
            WidgetPaint((tWidget *)&g_sStatus);

            break;
        }
        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// The program main function.  It performs initialization, then handles wav
// file playback.
//
//*****************************************************************************
int
main(void)
{
    FRESULT fresult;
    unsigned long ulTemp;

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Set the interrupt priorities to give USB and System tick higher priority
    // than the ADC.  While playing the touch screen should have lower priority
    // to reduce audio drop out.
    //
    ROM_IntPriorityGroupingSet(4);
    ROM_IntPrioritySet(INT_USB0, USB_INT_PRIORITY);
    ROM_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);
    ROM_IntPrioritySet(INT_ADC0SS3, ADC3_INT_PRIORITY);

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Set some initial strings.
    //
    ListBoxTextAdd(&g_sDirList, "Initializing...");

    //
    // Initialize the status text.
    //
    strcpy(g_psStatusText, "No Device");

    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPlayBtn);

    //
    // Issue the initial paint request to the widgets then immediately call
    // the widget manager to process the paint message.  This ensures that the
    // display is drawn as quickly as possible and saves the delay we would
    // otherwise experience if we processed the paint message after mounting
    // and reading the SD card.
    //
    WidgetPaint(WIDGET_ROOT);
    WidgetMessageQueueProcess();

    //
    // Mount the file system, using logical disk 0.
    //
    fresult = f_mount(0, &g_sFatFs);
    if(fresult != FR_OK)
    {
        return(1);
    }

    //
    // Populate the list box with the contents of the root directory.
    //
    PopulateFileListBox(true);

    //
    // Not playing anything right now.
    //
    g_ulFlags = 0;

    g_ulSysTickCount = 0;
    g_ulLastTick = 0;

    //
    // Configure the USB host output.
    //
    USBSoundInit(0, AudioEvent);

    //
    // Enter an (almost) infinite loop for reading and processing commands from
    // the user.
    //
    while(1)
    {
        //
        // On connect change the device state to ready.
        //
        if(HWREGBITW(&g_ulFlags, FLAGS_DEVICE_CONNECT))
        {
            HWREGBITW(&g_ulFlags, FLAGS_DEVICE_CONNECT) = 0;

            //
            // Attempt to set the audio format to 44100 16 bit stereo by
            // default otherwise try 48000 16 bit stereo.
            //
            if(USBSoundOutputFormatSet(44100, 16, 2) == 0)
            {
                ulTemp = 44100;
            }
            else if(USBSoundOutputFormatSet(48000, 16, 2) == 0)
            {
                ulTemp = 48000;
            }
            else
            {
                ulTemp = 0;
            }

            //
            // If the audio device was support put the sample rate in the
            // status line.
            //
            if(ulTemp != 0)
            {
                //
                // Calculate the number of bytes per USB frame.
                //
                g_ulTransferSize = (ulTemp * 4) / 1000;

                //
                // Calculate the size of the audio buffer.
                //
                g_ulBufferSize = AUDIO_BUFFERS * g_ulTransferSize;

                //
                // Print the time string in the format mm.ss/mm.ss
                //
                usprintf(g_psStatusText, "Ready  %dHz 16 bit Stereo", ulTemp);

                HWREGBITW(&g_ulFlags, FLAGS_DEVICE_READY) = 1;
            }
            else
            {
                strcpy(g_psStatusText, "Unsupported Audio Device");
            }

            //
            // Update the status line.
            //
            WidgetPaint((tWidget *)&g_sStatus);
        }

        //
        // If wav play back has started let the WavePlay routine take over main
        // routine.
        //
        if(HWREGBITW(&g_ulFlags, FLAGS_PLAYING))
        {
            //
            // Try to play Wav file.
            //
            WavePlay(&g_sWavHeader);
        }

        USBMain(GetTickms());

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}
