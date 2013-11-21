//*****************************************************************************
//
// usb_host_audio.c - Main routine for the USB host audio example.
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F232 Firmware Package.
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
#include "driverlib/pin_map.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/slider.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "utils/ustdlib.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "drivers/usb_sound.h"
#include "drivers/wavfile.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/buttons.h"
#include "drivers/slidemenuwidget.h"
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
// A structure that holds a mapping between an FRESULT numerical code,
// and a string representation.  FRESULT codes are returned from the FatFs
// FAT file system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT fresult;
    char *pcResultStr;
}
tFresultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A table that holds a mapping between the numerical FRESULT code and
// it's name as a string.  This is used for looking up error codes and
// providing a human-readable string.
//
//*****************************************************************************
tFresultString g_sFresultStrings[] =
{
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_RW_ERROR),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_MKFS_ABORTED)
};

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES (sizeof(g_sFresultStrings) / sizeof(tFresultString))

//*****************************************************************************
//
// Error reasons returned by ChangeToDirectory().
//
//*****************************************************************************
#define NAME_TOO_LONG_ERROR 1
#define OPENDIR_ERROR       2

//*****************************************************************************
//
// Define a pair of buffers that are used for holding path information.
// The buffer size must be large enough to hold the longest expected
// full path name, including the file name, and a trailing null character.
// The initial path is set to root "/".
//
//*****************************************************************************
#define PATH_BUF_SIZE   80
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";
static char g_cTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// A set of string pointers that are used for showing status on the display.
// Five lines of text are accommodated, which is the reasonable limit for
// this display.
//
//*****************************************************************************
static char *g_pcStatusLines[5];

//*****************************************************************************
//
// A variable to track the current level in the directory tree.  The root
// level is level 0.
//
//*****************************************************************************
static unsigned long g_ulLevel;

//*****************************************************************************
//
// A variable to track the current level in the directory tree.  The root
// level is level 0.
//
//*****************************************************************************
static unsigned long g_ulLevel;

//*****************************************************************************
//
// Declare a pair of off-screen buffers and associated display structures.
// These are used by the slide menu widget for animated menu effects.
//
//*****************************************************************************
#define OFFSCREEN_BUF_SIZE GrOffScreen4BPPSize(96, 40)
static unsigned char g_pucOffscreenBufA[OFFSCREEN_BUF_SIZE];
static unsigned char g_pucOffscreenBufB[OFFSCREEN_BUF_SIZE];
static tDisplay g_sOffscreenDisplayA;
static tDisplay g_sOffscreenDisplayB;

//*****************************************************************************
//
// Create a palette that is used by the on-screen menus and anything else that
// uses the (above) off-screen buffers.  This palette should contain any
// colors that are used by any widget using the offscreen buffers.  There can
// be up to 16 colors in this palette.
//
//*****************************************************************************
static unsigned long g_pulPalette[] =
{
    ClrBlack,
    ClrWhite,
    ClrDarkBlue,
    ClrLightBlue,
    ClrRed,
    ClrDarkGreen,
    ClrYellow,
    ClrBlue
};
#define NUM_PALETTE_ENTRIES (sizeof(g_pulPalette) / sizeof(unsigned long))

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Audio buffering definitions, these are optimized to deal with USB audio.
//
//*****************************************************************************
#define AUDIO_TRANSFER_SIZE     (192)
#define AUDIO_BUFFERS           (16)
#define AUDIO_BUFFER_SIZE       (AUDIO_TRANSFER_SIZE * AUDIO_BUFFERS)
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

//
// Play Screen is being displayed
//
#define FLAGS_PLAY_SCREEN       5

//*****************************************************************************
//
// These are the global .wav file states used by the application.
//
//*****************************************************************************
tWavFile g_sWavFile;
tWavHeader g_sWavHeader;

//*****************************************************************************
//
// Define the maximum number of files that can appear at any directory level.
// This is used for allocating space for holding the file information.
// Define the maximum depth of subdirectories, also used to allocating space
// for directory structures.
// Define the maximum number of characters allowed to be stored for a file
// name.
//
//*****************************************************************************
#define MAX_FILES_PER_MENU 64
#define MAX_SUBDIR_DEPTH 32
#define MAX_FILENAME_STRING_LEN 16

//*****************************************************************************
//
// Widget definitions
//
//*****************************************************************************

//*****************************************************************************
//
// Declare a set of menu items and matching strings that are used to hold
// file information.  There are two alternating sets.  Two are needed because
// the file information must be retained for the current directory, and the
// new directory (up or down the tree).
//
//*****************************************************************************
static char g_cFileNames[2][MAX_FILES_PER_MENU][MAX_FILENAME_STRING_LEN];
static tSlideMenuItem g_sFileMenuItems[2][MAX_FILES_PER_MENU];

//*****************************************************************************
//
// Declare a set of menus, one for each level of directory.
//
//*****************************************************************************
static tSlideMenu g_sFileMenus[MAX_SUBDIR_DEPTH];

//*****************************************************************************
//
// Define the slide menu widget.  This is the wigdet that is used for
// displaying the file information.
//
//*****************************************************************************
SlideMenu(g_sFileMenuWidget, WIDGET_ROOT, 0, 0, &g_sCFAL96x64x16, 0, 12, 96, 40,
          &g_sOffscreenDisplayA, &g_sOffscreenDisplayB, 16,
          ClrWhite, ClrRed, ClrBlack, &g_sFontFixed6x8, &g_sFileMenus[0],
          0);

//*****************************************************************************
//
// The canvas widgets for the wav file information.
//
//*****************************************************************************
extern tCanvasWidget g_sWaveInfoBackground;

char g_pcVolume[16] = "";
Canvas(g_sWaveInfoVolume, &g_sWaveInfoBackground, 0, 0,
       &g_sCFAL96x64x16, 0, 42, 96, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrRed, ClrWhite, ClrWhite, g_pFontFixed6x8,
       g_pcVolume, 0, 0);

char g_pcTime[16] = "";
Canvas(g_sWaveInfoTime, &g_sWaveInfoBackground, &g_sWaveInfoVolume, 0,
       &g_sCFAL96x64x16, 0, 32, 96, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrRed, ClrWhite, ClrWhite, g_pFontFixed6x8,
       g_pcTime, 0, 0);

char g_pcFormat[16] = "";
Canvas(g_sWaveInfoSample, &g_sWaveInfoBackground, &g_sWaveInfoTime, 0,
       &g_sCFAL96x64x16, 0, 22, 96, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_OPAQUE, ClrRed, ClrWhite, ClrWhite, g_pFontFixed6x8,
       g_pcFormat, 0, 0);

char g_pcFileName[16] = "";
Canvas(g_sWaveInfoFileName, &g_sWaveInfoBackground, &g_sWaveInfoSample, 0,
       &g_sCFAL96x64x16, 0, 12, 96, 10,
       CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE,
       ClrRed, ClrWhite, ClrWhite, g_pFontFixed6x8,
       g_pcFileName, 0, 0);

#define INITIAL_VOLUME_PERCENT 20
unsigned long g_ulCurrentVolume;

//*****************************************************************************
//
// The canvas widget acting as the background for the wav file information.
//
//*****************************************************************************
Canvas(g_sWaveInfoBackground, WIDGET_ROOT, 0,
       &g_sWaveInfoFileName, &g_sCFAL96x64x16, 0, 12, 96, 40,
       CANVAS_STYLE_OUTLINE | CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite,
       g_pFontFixed6x8, 0, 0, 0);

//*****************************************************************************
//
// The status line.
//
//*****************************************************************************
#define STATUS_TEXT_SIZE        40
char g_psStatusText[STATUS_TEXT_SIZE] = "";
Canvas(g_sStatus, WIDGET_ROOT, 0, 0,
       &g_sCFAL96x64x16, 0, 64 - 12, 96, 12,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontFixed6x8, g_psStatusText, 0, 0);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, 0, 0,
       &g_sCFAL96x64x16, 0, 0, 96, 12,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontFixed6x8, "usb-host-audio", 0, 0);

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
// This function is used to update the current volume being using for play
// back.
//
//*****************************************************************************
static void
DisplayVolume(void)
{
    //
    // Print the volume string in the format Volume dd%
    //
    usprintf(g_pcVolume, "Volume %0d%%", g_ulCurrentVolume);

    //
    // Display the updated time on the screen.
    //
    WidgetPaint((tWidget *)&g_sWaveInfoVolume);
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
        ulSeconds = g_ulBytesPlayed / g_sWavHeader.ulAvgByteRate;
        ulMinutes = ulSeconds / 60;
        ulSeconds -= ulMinutes * 60;

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
    // Change the status text on the button to Stopped.
    //
    strcpy(g_psStatusText, "Stopped");
    WidgetPaint((tWidget *)&g_sStatus);
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
// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
//
//*****************************************************************************
static const char *
StringFromFresult(FRESULT fresult)
{
    unsigned int uIdx;

    //
    // Enter a loop to search the error code table for a matching
    // error code.
    //
    for(uIdx = 0; uIdx < NUM_FRESULT_CODES; uIdx++)
    {
        //
        // If a match is found, then return the string name of the
        // error code.
        //
        if(g_sFresultStrings[uIdx].fresult == fresult)
        {
            return(g_sFresultStrings[uIdx].pcResultStr);
        }
    }

    //
    // At this point no matching code was found, so return a
    // string indicating unknown error.
    //
    return("UNKNOWN ERR");
}

//*****************************************************************************
//
// This function shows a status screen.  It draws a banner at the top of the
// screen with the name of the application, and then up to 5 lines of text
// in the remaining screen area.  The caller specifies a pointer to a set of
// strings, and the count of number of lines of text (up to 5).  This
// function attempts to vertically center the strings on the display.
//
//*****************************************************************************
static void
ShowStatusScreen(char *pcStatus[], unsigned long ulCount)
{
    tContext sContext;
    tRectangle sRect;
    unsigned int uIdx;
    long lY;

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sCFAL96x64x16);

    //
    // Fill the rest of the display with black, to clear whatever was there
    // before.
    //
    sRect.sXMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.sYMin = 12;
    sRect.sYMax = 63 - 12;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);

    //
    // Change foreground for white text.
    //
    GrContextForegroundSet(&sContext, ClrWhite);

    //
    // Cap the max number of status lines to 4
    //
    ulCount = (ulCount > 4) ? 4 : ulCount;

    //
    // Compute the starting Y coordinate based on the number of lines.
    //
    lY = 36 - (ulCount * 5);

    //
    // Display the status lines
    //
    GrContextFontSet(&sContext, g_pFontFixed6x8);
    for(uIdx = 0; uIdx < ulCount; uIdx++)
    {
        GrStringDrawCentered(&sContext, pcStatus[uIdx], -1,
                             GrContextDpyWidthGet(&sContext) / 2, lY, 0);
        lY += 10;
    }
}

//*****************************************************************************
//
// Initializes the file system module.
//
// \param None.
//
// This function initializes the third party FAT implementation.
//
// \return Returns \e true on success or \e false on failure.
//
//*****************************************************************************
static tBoolean
FileInit(void)
{
    //
    // Mount the file system, using logical disk 0.
    //
    if(f_mount(0, &g_sFatFs) != FR_OK)
    {
        return(false);
    }
    return(true);
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

            //
            // Change the text to reflect the change.
            //
            strcpy(g_psStatusText, "Ready");
            WidgetPaint((tWidget *)&g_sStatus);

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

            //
            // Display the SD card found message again.  This
            // should replace the slide menu.
            //
            g_pcStatusLines[0] = "SD Card Found";
            ShowStatusScreen(g_pcStatusLines, 1);

            break;
        }
        case SOUND_EVENT_UNKNOWN_DEV:
        {
            if(ulParam == 1)
            {
                //
                // Unknown device connected.
                //
                strcpy(g_psStatusText, "Unknown Device");
                WidgetPaint((tWidget *)&g_sStatus);
            }
            else
            {
                //
                // Unknown device disconnected.
                //
                strcpy(g_psStatusText, "No Device");
                WidgetPaint((tWidget *)&g_sStatus);
            }

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
// This function is called to read the contents of the current directory from
// the USB stick and populate a set of menu items, one for each file in the
// directory.  A subdirectory within the directory counts as a file item.
//
// This function returns the number of file items that were found, or 0 if
// there is any error detected.
//
//*****************************************************************************
static unsigned long
PopulateFileList(unsigned long ulLevel)
{
    unsigned long ulItemCount;
    FRESULT fresult;

    //
    // Open the current directory for access.
    //
    fresult = f_opendir(&g_sDirObject, g_cCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        g_pcStatusLines[0] = "Error from";
        g_pcStatusLines[1] = "SD Card";
        g_pcStatusLines[2] = (char *)StringFromFresult(fresult);
        ShowStatusScreen(g_pcStatusLines, 3);
        return(0);
    }

    //
    // Initialize the count of files in this directory
    //
    ulItemCount = 0;

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
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
            g_pcStatusLines[0] = "Error from";
            g_pcStatusLines[1] = "USB disk";
            g_pcStatusLines[2] = (char *)StringFromFresult(fresult);
            ShowStatusScreen(g_pcStatusLines, 3);
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
        // Add the information to a menu item
        //
        if(ulItemCount < MAX_FILES_PER_MENU)
        {
            tSlideMenuItem *pMenuItem;

            //
            // Get a pointer to the current menu item.  Use the directory
            // level to determine which of the two sets of menu items to use.
            // (ulLevel & 1].  This lets us alternate between the current
            // set of menu items and the new set (up or down the tree).
            //
            pMenuItem = &g_sFileMenuItems[ulLevel & 1][ulItemCount];

            //
            // Add the file name to the menu item
            //
            usnprintf(g_cFileNames[ulLevel & 1][ulItemCount],
                      MAX_FILENAME_STRING_LEN, "%s", g_sFileInfo.fname);
            pMenuItem->pcText = g_cFileNames[ulLevel & 1][ulItemCount];

            //
            // If this is a directory, then add the next level menu so that
            // when displayed it will be showed with a submenu option
            // (next level down in directory tree).  Otherwise it is a file
            // so clear the child menu so that there is no submenu option
            // shown.
            //
            pMenuItem->pChildMenu = (g_sFileInfo.fattrib & AM_DIR) ?
                                    &g_sFileMenus[ulLevel + 1] : 0;

            //
            // Move to the next entry in the item array we use to populate the
            // list box.
            //
            ulItemCount++;
        }
    }

    //
    // Made it to here, return the count of files that were populated.
    //
    return(ulItemCount);
}

//*****************************************************************************
//
// This function is used to change to a new directory in the file system.
// It takes a parameter that specifies the directory to make the current
// working directory.
// Path separators must use a forward slash "/".  The directory parameter
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
// In cases of error, the pulReason parameter will be written with one of
// the following values:
//
//  NAME_TOO_LONG_ERROR - combination of paths are too long for the buffer
//  OPENDIR_ERROR - there is some problem opening the new directory
//
//*****************************************************************************
static FRESULT
ChangeToDirectory(char *pcDirectory, unsigned long *pulReason)
{
    unsigned int uIdx;
    FRESULT fresult;

    //
    // Copy the current working path into a temporary buffer so
    // it can be manipulated.
    //
    strcpy(g_cTmpBuf, g_cCwdBuf);

    //
    // If the first character is /, then this is a fully specified
    // path, and it should just be used as-is.
    //
    if(pcDirectory[0] == '/')
    {
        //
        // Make sure the new path is not bigger than the cwd buffer.
        //
        if(strlen(pcDirectory) + 1 > sizeof(g_cCwdBuf))
        {
            *pulReason = NAME_TOO_LONG_ERROR;
            return(FR_OK);
        }

        //
        // If the new path name (in argv[1])  is not too long, then
        // copy it into the temporary buffer so it can be checked.
        //
        else
        {
            strncpy(g_cTmpBuf, pcDirectory, sizeof(g_cTmpBuf));
        }
    }

    //
    // If the argument is .. then attempt to remove the lowest level
    // on the CWD.
    //
    else if(!strcmp(pcDirectory, ".."))
    {
        //
        // Get the index to the last character in the current path.
        //
        uIdx = strlen(g_cTmpBuf) - 1;

        //
        // Back up from the end of the path name until a separator (/)
        // is found, or until we bump up to the start of the path.
        //
        while((g_cTmpBuf[uIdx] != '/') && (uIdx > 1))
        {
            //
            // Back up one character.
            //
            uIdx--;
        }

        //
        // Now we are either at the lowest level separator in the
        // current path, or at the beginning of the string (root).
        // So set the new end of string here, effectively removing
        // that last part of the path.
        //
        g_cTmpBuf[uIdx] = 0;
    }

    //
    // Otherwise this is just a normal path name from the current
    // directory, and it needs to be appended to the current path.
    //
    else
    {
        //
        // Test to make sure that when the new additional path is
        // added on to the current path, there is room in the buffer
        // for the full new path.  It needs to include a new separator,
        // and a trailing null character.
        //
        if(strlen(g_cTmpBuf) + strlen(pcDirectory) + 1 + 1 > sizeof(g_cCwdBuf))
        {
            *pulReason = NAME_TOO_LONG_ERROR;
            return(FR_INVALID_OBJECT);
        }

        //
        // The new path is okay, so add the separator and then append
        // the new directory to the path.
        //
        else
        {
            //
            // If not already at the root level, then append a /
            //
            if(strcmp(g_cTmpBuf, "/"))
            {
                strcat(g_cTmpBuf, "/");
            }

            //
            // Append the new directory to the path.
            //
            strcat(g_cTmpBuf, pcDirectory);
        }
    }

    //
    // At this point, a candidate new directory path is in cTmpBuf.
    // Try to open it to make sure it is valid.
    //
    fresult = f_opendir(&g_sDirObject, g_cTmpBuf);

    //
    // If it can't be opened, then it is a bad path.  Return an error.
    //
    if(fresult != FR_OK)
    {
        *pulReason = OPENDIR_ERROR;
        return(fresult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD.
    //
    else
    {
        strncpy(g_cCwdBuf, g_cTmpBuf, sizeof(g_cCwdBuf));
    }

    //
    // Return success.
    //
    return(FR_OK);
}

//*****************************************************************************
//
// Sends a button/key press message to the slide menu widget that is showing
// files.
//
//*****************************************************************************
static void
SendWidgetKeyMessage(unsigned long ulMsg)
{
    WidgetMessageQueueAdd(WIDGET_ROOT,
                          ulMsg, (unsigned long)&g_sFileMenuWidget, 0, 1, 1);
}

//*****************************************************************************
//
// This function performs actions that are common whenever the directory
// level is changed up or down.  It populates the correct menu structure with
// the list of files in the directory.
//
//*****************************************************************************
static tBoolean
ProcessDirChange(char *pcDir, unsigned long ulLevel)
{
    FRESULT fresult;
    unsigned long ulReason;
    unsigned long ulFileCount;

    //
    // Attempt to change to the new directory.
    //
    fresult = ChangeToDirectory(pcDir, &ulReason);

    //
    // If the directory change was successful, populate the
    // list of files for the new subdirectory.
    //
    if((fresult == FR_OK) && (ulLevel < MAX_SUBDIR_DEPTH))
    {
        //
        // Get a pointer to the current menu for this CWD.
        //
        tSlideMenu *pMenu = &g_sFileMenus[ulLevel];

        //
        // Populate the menu items with the file list for the new CWD.
        //
        ulFileCount = PopulateFileList(ulLevel);

        //
        // Initialize the file menu with the list of menu items,
        // which are just files and dirs in the root directory
        //
        pMenu->pSlideMenuItems = g_sFileMenuItems[ulLevel & 1];
        pMenu->ulItems = ulFileCount;

        //
        // Set the parent directory, if there is one.  If at level 0
        // (CWD is root), then there is no parent directory.
        //
        if(ulLevel)
        {
            pMenu->pParent = &g_sFileMenus[ulLevel - 1];
        }
        else
        {
            pMenu->pParent = 0;
        }

        //
        // If we are descending into a new subdir, then initialize the other
        // menu item fields to default values.
        //
        if(ulLevel > g_ulLevel)
        {
            pMenu->ulCenterIndex = 0;
            pMenu->ulFocusIndex = 0;
            pMenu->bMultiSelectable = 0;
        }

        //
        // Return a success indication
        //
        return(true);
    }

    //
    // Directory change was not successful
    //
    else
    {
        //
        // Return failure indication
        //
        return(false);
    }
}

//*****************************************************************************
//
// The program main function.  It performs initialization, then handles wav
// file playback.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulTemp;
    unsigned long ulRetcode;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Configure the required pins for USB operation.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    ROM_GPIOPinConfigure(GPIO_PG4_USB0EPEN);
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTG_BASE, GPIO_PIN_4);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the buttons driver.
    //
    ButtonsInit();

    //
    // Initialize two offscreen displays and assign the palette.  These
    // buffers are used by the slide menu widget to allow animation effects.
    //
    GrOffScreen4BPPInit(&g_sOffscreenDisplayA, g_pucOffscreenBufA, 96, 40);
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplayA, g_pulPalette, 0,
                              NUM_PALETTE_ENTRIES);
    GrOffScreen4BPPInit(&g_sOffscreenDisplayB, g_pucOffscreenBufB, 96, 40);
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplayB, g_pulPalette, 0,
                              NUM_PALETTE_ENTRIES);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sStatus);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sFileMenuWidget);

    //
    // Initialize the status text.
    //
    strcpy(g_psStatusText, "No Device");

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
    // Determine whether or not an SD Card is installed.  If not, print a
    // warning and have the user install one and restart.
    //
    ulRetcode = disk_initialize(0);

    if(ulRetcode != RES_OK)
    {
        g_pcStatusLines[0] = "No SD Card Found";
        g_pcStatusLines[1] = "Please insert";
        g_pcStatusLines[2] = "a card and";
        g_pcStatusLines[3] = "reset the board.";
        ShowStatusScreen(g_pcStatusLines, 4);
        return (1);
    }
    else
    {
        g_pcStatusLines[0] = "SD Card Found";
        ShowStatusScreen(g_pcStatusLines, 1);

        //
        // Mount the file system, using logical disk 0.
        //
        f_mount(0, &g_sFatFs);
        if(!FileInit())
        {
            return(1);
        }
    }

    //
    // Not playing anything right now.
    //
    g_ulFlags = 0;
    g_ulSysTickCount = 0;
    g_ulLastTick = 0;
    g_ulCurrentVolume = INITIAL_VOLUME_PERCENT;

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
        unsigned long ulLastTickCount = 0;

        //
        // On connect change the device state to ready.
        //
        if(HWREGBITW(&g_ulFlags, FLAGS_DEVICE_CONNECT))
        {
            HWREGBITW(&g_ulFlags, FLAGS_DEVICE_CONNECT) = 0;

            //
            // Getting here means the device is ready.
            // Reset the CWD to the root directory.
            //
            g_cCwdBuf[0] = '/';
            g_cCwdBuf[1] = 0;

            //
            // Set the initial directory level to the root
            //
            g_ulLevel = 0;

            //
            // We need to reset the indexes of the root menu to 0, so that
            // it will start at the top of the file list, and reset the
            // slide menu widget to start with the root menu.
            //
            g_sFileMenus[g_ulLevel].ulCenterIndex = 0;
            g_sFileMenus[g_ulLevel].ulFocusIndex = 0;
            SlideMenuMenuSet(&g_sFileMenuWidget, &g_sFileMenus[g_ulLevel]);

            //
            // Initiate a directory change to the root.  This will
            // populate a menu structure representing the root directory.
            //
            if(ProcessDirChange("/", g_ulLevel))
            {
                //
                // Request a repaint so the file menu will be shown
                //
                WidgetPaint(WIDGET_ROOT);
            }
            else
            {
                g_pcStatusLines[0] = "ERROR";
                g_pcStatusLines[1] = "Unable to change";
                g_pcStatusLines[2] = "directory.";
                ShowStatusScreen(g_pcStatusLines, 3);
                return(1);
            }

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

                if(ulTemp == 44100)
                {
                    usprintf(g_psStatusText, "44.1 kHz Ready");
                }
                else if(ulTemp == 48000)
                {
                    usprintf(g_psStatusText, "48 kHz Ready");
                }

                HWREGBITW(&g_ulFlags, FLAGS_DEVICE_READY) = 1;
            }
            else
            {
                strcpy(g_psStatusText, "Not Supported");
                return (1);
            }

            //
            // Set initial volume.
            //
            USBSoundVolumeSet(g_ulCurrentVolume);

            //
            // Update the status line.
            //
            WidgetPaint((tWidget *)&g_sStatus);
        }

        //
        // Process occurrence of timer tick.  Check for user input
        // once each tick.
        //
        if((g_ulSysTickCount != ulLastTickCount) &&
           HWREGBITW(&g_ulFlags, FLAGS_DEVICE_READY) &&
           (HWREGBITW(&g_ulFlags, FLAGS_PLAY_SCREEN) == 0))
        {
            unsigned char ucButtonState;
            unsigned char ucButtonChanged;

            ulLastTickCount = g_ulSysTickCount;

            //
            // Get the current debounced state of the buttons.
            //
            ucButtonState = ButtonsPoll(&ucButtonChanged, 0);

            //
            // If select button or right button is pressed, then we
            // are trying to descend into another directory
            //
            if(BUTTON_PRESSED(SELECT_BUTTON,
                              ucButtonState, ucButtonChanged) ||
               BUTTON_PRESSED(RIGHT_BUTTON,
                              ucButtonState, ucButtonChanged))
            {
                unsigned long ulNewLevel;
                unsigned long ulItemIdx;
                char *pcItemName;

                //
                // Get a pointer to the current menu for this CWD.
                //
                tSlideMenu *pMenu = &g_sFileMenus[g_ulLevel];

                //
                // Get the highlighted index in the current file list.
                // This is the currently highlighted file or dir
                // on the display.  Then get the name of the file at
                // this index.
                //
                ulItemIdx = SlideMenuFocusItemGet(pMenu);
                pcItemName = pMenu->pSlideMenuItems[ulItemIdx].pcText;

                //
                // Make sure we are not yet past the maximum tree
                // depth.
                //
                if(g_ulLevel < MAX_SUBDIR_DEPTH)
                {
                    //
                    // Potential new level is one greater than the
                    // current level.
                    //
                    ulNewLevel = g_ulLevel + 1;

                    //
                    // Process the directory change to the new
                    // directory.  This function will populate a menu
                    // structure with the files and subdirs in the new
                    // directory.
                    //
                    if(ProcessDirChange(pcItemName, ulNewLevel))
                    {
                        //
                        // If the change was successful, then update
                        // the level.
                        //
                        g_ulLevel = ulNewLevel;

                        //
                        // Now that all the prep is done, send the
                        // KEY_RIGHT message to the widget and it will
                        // "slide" from the previous file list to the
                        // new file list of the CWD.
                        //
                        SendWidgetKeyMessage(WIDGET_MSG_KEY_RIGHT);

                    }

                    //
                    // We have selected a file to play.  Display
                    // the file information and if it is a valid
                    // wav file, allow playback.
                    //
                    else
                    {

                        //
                        // Update the file name information.
                        //
                        strncpy(g_pcFileName, pcItemName, 16);

                        if(WavOpen(g_pcFileName, &g_sWavFile) == 0)
                        {
                            //
                            // Read the .wav file format.
                            //
                            WavGetFormat(&g_sWavFile, &g_sWavHeader);

                            //
                            // Print the formatted string so that it can be
                            // displayed.
                            //
                            usprintf(g_pcFormat, "%d Hz %d bit ",
                                     g_sWavHeader.ulSampleRate / 1000,
                                     g_sWavHeader.usBitsPerSample);

                            //
                            // Concatenate the number of channels.
                            //
                            if(g_sWavHeader.usNumChannels == 1)
                            {
                                strcat(g_pcFormat, "Mo");
                            }
                            else
                            {
                                strcat(g_pcFormat, "St");
                            }

                            //
                            // Calculate the minutes and seconds in the file.
                            //
                            g_usSeconds = g_sWavHeader.ulDataSize /
                                          g_sWavHeader.ulAvgByteRate;
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

                            //
                            // Update the volume information
                            //
                            DisplayVolume();
                        }
                        else
                        {
                            //
                            // Set the time and volume strings to null strings.
                            //
                            g_pcTime[0] = 0;
                            g_pcVolume[0] = 0;

                            //
                            // Print message about invalid wav format
                            //
                            usprintf(g_pcFormat, "Invalid Wav");
                        }

                        //
                        // Update the file name line.
                        //
                        WidgetPaint((tWidget *)&g_sWaveInfoBackground);

                        //
                        // Set a flag to change the button functions.
                        //
                        HWREGBITW(&g_ulFlags, FLAGS_PLAY_SCREEN) = 1;
                    }
                }
            }

            //
            // If the UP button is pressed, just pass it to the widget
            // which will handle scrolling the list of files.
            //
            if(BUTTON_PRESSED(UP_BUTTON, ucButtonState, ucButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_UP);
            }

            //
            // If the DOWN button is pressed, just pass it to the widget
            // which will handle scrolling the list of files.
            //
            if(BUTTON_PRESSED(DOWN_BUTTON, ucButtonState, ucButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_DOWN);
            }

            //
            // If the LEFT button is pressed, then we are attempting
            // to go up a level in the file system.
            //
            if(BUTTON_PRESSED(LEFT_BUTTON, ucButtonState, ucButtonChanged))
            {
                unsigned long ulNewLevel;

                //
                // Make sure we are not already at the top of the
                // directory tree (at root).
                //
                if(g_ulLevel)
                {
                    //
                    // Potential new level is one less than the
                    // current level.
                    //
                    ulNewLevel = g_ulLevel - 1;

                    //
                    // Process the directory change to the new
                    // directory.  This function will populate a menu
                    // structure with the files and subdirs in the new
                    // directory.
                    //
                    if(ProcessDirChange("..", ulNewLevel))
                    {
                        //
                        // If the change was successful, then update
                        // the level.
                        //
                        g_ulLevel = ulNewLevel;

                        //
                        // Now that all the prep is done, send the
                        // KEY_LEFT message to the widget and it will
                        // "slide" from the previous file list to the
                        // new file list of the CWD.
                        //
                        SendWidgetKeyMessage(WIDGET_MSG_KEY_LEFT);
                    }
                }
            }
        }

        //
        // If we are in the play back screen, change the function of the
        // buttons.
        //
        if((g_ulSysTickCount != ulLastTickCount) &&
           HWREGBITW(&g_ulFlags, FLAGS_DEVICE_READY) &&
           (HWREGBITW(&g_ulFlags, FLAGS_PLAY_SCREEN)))
        {
            unsigned char ucButtonState;
            unsigned char ucButtonChanged;

            ulLastTickCount = g_ulSysTickCount;

            //
            // Get the current debounced state of the buttons.
            //
            ucButtonState = ButtonsPoll(&ucButtonChanged, 0);

            //
            // If the left button is pressed, we need to return to
            // the file menu.
            //
            if(BUTTON_PRESSED(LEFT_BUTTON, ucButtonState, ucButtonChanged))
            {
                //
                // It is possible we have already started playback.
                //
                WaveStop();

                //
                // Redraw the menu over the play screen.
                //
                HWREGBITW(&g_ulFlags, FLAGS_PLAY_SCREEN) = 0;
                WidgetPaint(WIDGET_ROOT);
            }

            //
            // If the select or right button is pressed, we will
            // start play back of the wav file.
            //
            if(BUTTON_PRESSED(SELECT_BUTTON,
                              ucButtonState, ucButtonChanged) ||
               BUTTON_PRESSED(RIGHT_BUTTON,
                              ucButtonState, ucButtonChanged))
            {
                //
                // If we are stopped, then start playing.
                //
                if(HWREGBITW(&g_ulFlags, FLAGS_PLAYING) == 0)
                {
                    HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 1;
                    //
                    // Don't play anything but 16 bit audio since most USB
                    // devices do not support 8 bit formats.
                    //
                    if(g_sWavHeader.usBitsPerSample != 16)
                    {
                        return(1);
                    }

                    //
                    // See if this is a valid .wav file that can be opened.
                    //
                    if(WavOpen(g_pcFileName, &g_sWavFile) == 0)
                    {
                        //
                        // Change the text on the button to playing.
                        //
                        strcpy(g_psStatusText, "Playing");
                        WidgetPaint((tWidget *)&g_sStatus);

                        //
                        // Indicate that wave play back should start.
                        //
                        HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 1;
                    }
                    else
                    {
                        HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 0;
                        return(1);
                    }

                    //
                    // Initialize the read and write pointers.
                    //
                    g_pucRead = g_pucAudioBuffer;
                    g_pucWrite = g_pucAudioBuffer;
                }

                //
                // Stop play back if we are playing.
                //
                else
                {
                    WaveStop();
                }
            }

            //
            // If the UP button is pressed, increase the volume by 5%.
            //
            if(BUTTON_PRESSED(UP_BUTTON, ucButtonState, ucButtonChanged))
            {
                if((g_ulCurrentVolume >= 0) && (g_ulCurrentVolume < 100))
                {
                    g_ulCurrentVolume += 5;
                    USBSoundVolumeSet(g_ulCurrentVolume);
                    DisplayVolume();
                }
            }

            //
            // If the DOWN button is pressed, decrease the volume by 5%.
            //
            if(BUTTON_PRESSED(DOWN_BUTTON, ucButtonState, ucButtonChanged))
            {
                if((g_ulCurrentVolume <= 100) && (g_ulCurrentVolume > 0))
                {
                    g_ulCurrentVolume -= 5;
                    USBSoundVolumeSet(g_ulCurrentVolume);
                    DisplayVolume();
                }
            }
        }

        //
        // Handle the case when the wave file is playing.
        //
        if(HWREGBITW(&g_ulFlags, FLAGS_PLAYING))
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
                        WaveStop();
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
                        WaveStop();
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
}

