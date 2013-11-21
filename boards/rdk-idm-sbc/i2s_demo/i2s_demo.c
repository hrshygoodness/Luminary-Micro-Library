//******************************************************************************
//
// i2s_demo.c - Example program for playing wav files from an SD card.
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
// This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
//
//******************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "driverlib/systick.h"
#include "driverlib/i2s.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/slider.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "utils/ustdlib.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/locator.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"

//******************************************************************************
//
//! \addtogroup example_list
//! <h1>I2S example application using SD Card FAT file system (i2s_demo)</h1>
//!
//! This example application demonstrates playing wav files from an SD card that
//! is formatted with a FAT file system.  The application will only look in the
//! root directory of the SD card and display all files that are found.  Files
//! can be selected to show their format and then played if the application
//! determines that they are a valid .wav file.
//!
//! For additional details about FatFs, see the following site:
//! http://elm-chan.org/fsw/ff/00index_e.html
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.
//
//******************************************************************************

//******************************************************************************
//
// The DMA control structure table.
//
//******************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//******************************************************************************
//
// The following are data structures used by FatFs.
//
//******************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;

//******************************************************************************
//
// The number of SysTick ticks per second.
//
//******************************************************************************
#define TICKS_PER_SECOND 100

//*****************************************************************************
//
// A signal used to tell the main loop to transfer control to the boot loader
// so that a firmware update can be performed over Ethernet.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

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
// are stored in format "filename.ext".
//
//******************************************************************************
#define MAX_FILENAME_STRING_LEN (8 + 1 + 3 + 1)
char g_pcFilenames[NUM_LIST_STRINGS][MAX_FILENAME_STRING_LEN];

//******************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//******************************************************************************
void OnListBoxChange(tWidget *pWidget, short usSelected);
void OnSliderChange(tWidget *pWidget, long lValue);
void OnBtnPlay(tWidget *pWidget);
static int PopulateFileListBox(tBoolean bRedraw);
void WaveStop(void);

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
        0, 30, 125, 180, LISTBOX_STYLE_OUTLINE, ClrBlack, ClrDarkBlue,
        ClrSilver, ClrWhite, ClrWhite, g_pFontCmss12, g_ppcDirListStrings,
        NUM_LIST_STRINGS, 0, OnListBoxChange);

//******************************************************************************
//
// The button used to play/stop a selected file.
//
//******************************************************************************
char g_psPlayText[5] = "Play";
RectangularButton(g_sPlayBtn, &g_sPlayBackground, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 160, 150, 95, 34,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlack, ClrBlue, ClrWhite, ClrWhite,
                   g_pFontCm20, g_psPlayText, 0, 0, 0, 0, OnBtnPlay);

//*****************************************************************************
//
// The canvas widget used to display the MAC address.
//
//*****************************************************************************
#define SIZE_MAC_ADDR_BUFFER 32
char g_pcMACString[SIZE_MAC_ADDR_BUFFER];
Canvas(g_sMACAddr, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 230, 160, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontFixed6x8, g_pcMACString, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the IP address.
//
//*****************************************************************************
#define SIZE_IP_ADDR_BUFFER 24
char g_pcIPString[SIZE_IP_ADDR_BUFFER];
Canvas(g_sIPAddr, WIDGET_ROOT, &g_sMACAddr, 0,
       &g_sKitronix320x240x16_SSD2119, 160, 230, 160, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontFixed6x8, g_pcIPString, 0, 0);

//******************************************************************************
//
// The canvas widget acting as the background to the play/stop button.
//
//******************************************************************************
Canvas(g_sPlayBackground, WIDGET_ROOT, &g_sIPAddr, 0,
       &g_sKitronix320x240x16_SSD2119, 160, 150, 95, 34,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//******************************************************************************
//
// The canvas widgets for the wav file information.
//
//******************************************************************************
extern tCanvasWidget g_sWaveInfoBackground;

char g_pcTime[40]="";
Canvas(g_sWaveInfoTime, &g_sWaveInfoBackground, 0, 0,
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
Canvas(g_sWaveInfoBackground, WIDGET_ROOT, &g_sPlayBackground,
       &g_sWaveInfoFileName, &g_sKitronix320x240x16_SSD2119, 130, 30, 155, 80,
       CANVAS_STYLE_OUTLINE | CANVAS_STYLE_FILL, ClrBlack, ClrWhite, ClrWhite,
       g_pFontCmss12, 0, 0, 0);

//******************************************************************************
//
// The slider widget used for volume control.
//
//******************************************************************************
#define INITIAL_VOLUME_PERCENT 60
Slider(g_sSlider, &g_sListBackground, &g_sWaveInfoBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 290, 30, 30, 180, 0, 100,
       INITIAL_VOLUME_PERCENT, (SL_STYLE_FILL | SL_STYLE_BACKG_FILL |
       SL_STYLE_OUTLINE | SL_STYLE_VERTICAL), ClrGreen, ClrBlack, ClrWhite,
       ClrWhite, ClrWhite, 0, 0, 0, 0, OnSliderChange);

//******************************************************************************
//
// The canvas widget acting as the background to the left portion of the
// display.
//
//******************************************************************************
Canvas(g_sListBackground, WIDGET_ROOT, &g_sSlider, &g_sDirList,
       &g_sKitronix320x240x16_SSD2119, 10, 60, 120, 200,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//******************************************************************************
//
// The heading containing the application title.
//
//******************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sListBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "i2s demo", 0, 0);

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
static unsigned long g_pulBuffer[(AUDIO_BUFFER_SIZE / sizeof(unsigned long))];
static unsigned char *g_pucBuffer = (unsigned char *)g_pulBuffer;
unsigned long g_ulMaxBufferSize;

//
// Flags used in the g_ulFlags global variable.
//
#define BUFFER_BOTTOM_EMPTY     0x00000001
#define BUFFER_TOP_EMPTY        0x00000002
#define BUFFER_PLAYING          0x00000004
static volatile unsigned long g_ulFlags;

//
// Globals used to track playback position.
//
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
static tWaveHeader sWaveHeader;

//*****************************************************************************
//
// This function is called by the swupdate module whenever it receives a
// signal indicating that a remote firmware update request is being made.  This
// notification occurs in the context of the Ethernet interrupt handler so it
// is vital that you DO NOT transfer control to the boot loader directly from
// this function (since the boot loader does not like being entered in interrupt
// context).
//
//*****************************************************************************
void
SoftwareUpdateRequestCallback(void)
{
    //
    // Set the flag that tells the main task to transfer control to the boot
    // loader.
    //
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// Initialize the Ethernet hardware and lwIP TCP/IP stack and set up to listen
// for remote firmware update requests.
//
//*****************************************************************************
unsigned long
TCPIPStackInit(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACAddr[6];

    //
    // Configure the Ethernet LEDs on PF2 and PF3.
    //  LED0        Bit 3   Output
    //  LED1        Bit 2   Output
    //
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Get the MAC address from the UART0 and UART1 registers in NV ram.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV ram into a MAC address
    // array.
    //
    pucMACAddr[0] = ulUser0 & 0xff;
    pucMACAddr[1] = (ulUser0 >> 8) & 0xff;
    pucMACAddr[2] = (ulUser0 >> 16) & 0xff;
    pucMACAddr[3] = ulUser1 & 0xff;
    pucMACAddr[4] = (ulUser1 >> 8) & 0xff;
    pucMACAddr[5] = (ulUser1 >> 16) & 0xff;

    //
    // Format this address into a string and display it.
    //
    usnprintf(g_pcMACString, SIZE_MAC_ADDR_BUFFER,
              "MAC: %02X-%02X-%02X-%02X-%02X-%02X",
              pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
              pucMACAddr[4], pucMACAddr[5]);

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM-SBC i2s_demo");

    //
    // Start monitoring for the special packet that tells us a software
    // download is being requested.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Return our initial IP address.  This is 0 for now since we have not
    // had one assigned yet.
    //
    return(0);
}

//*****************************************************************************
//
// Check to see if the IP address has changed and, if so, update the
// display.
//
//*****************************************************************************
unsigned long IPAddressChangeCheck(unsigned long ulCurrentIP)
{
    unsigned long ulIPAddr;

    //
    // What is our current IP address?
    //
    ulIPAddr = lwIPLocalIPAddrGet();

    //
    // Has the IP address changed?
    //
    if(ulIPAddr != ulCurrentIP)
    {
        //
        // Yes - the address changed so update the display.
        //
        usprintf(g_pcIPString, "IP: %d.%d.%d.%d",
                 ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                 (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
        WidgetPaint((tWidget *)&g_sIPAddr);
    }

    //
    // Return our current IP address.
    //
    return(ulIPAddr);
}

//******************************************************************************
//
// Handler for bufffers being released.
//
//******************************************************************************
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

//******************************************************************************
//
// This function can be used to test if a file is a wav file or not and will
// also return the wav file header information in the pWaveHeader structure.
// If the file is a wav file then the psFileObject pointer will contain an
// open file pointer to the wave file ready to be passed into the WavePlay()
// function.
//
//******************************************************************************
FRESULT
WaveOpen(FIL *psFileObject, const char *pcFileName, tWaveHeader *pWaveHeader)
{
    unsigned long *pulBuffer;
    unsigned short *pusBuffer;
    unsigned long ulChunkSize;
    unsigned short usCount;
    unsigned long ulBytesPerSample;
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

    return(FR_OK);
}

//******************************************************************************
//
// This closes out the wav file.
//
//******************************************************************************
void
WaveClose(FIL *psFileObject)
{
    //
    // Close out the file.
    //
    f_close(psFileObject);
}

//******************************************************************************
//
// Convert an 8 bit unsigned buffer to 8 bit signed buffer in place so that it
// can be passed into the i2s playback.
//
//******************************************************************************
void
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
void
DisplayTime(void)
{
    unsigned long ulSeconds;
    unsigned long ulMinutes;

    //
    // Only display on the screen once per second.
    //
    if(g_ulBytesPlayed >= g_ulNextUpdate)
    {
        //
        // Set the next update time to one second later.
        //
        g_ulNextUpdate = g_ulBytesPlayed + sWaveHeader.ulAvgByteRate;

        //
        // Calculate the integer number of minutes and seconds.
        //
        ulSeconds = g_ulBytesPlayed/sWaveHeader.ulAvgByteRate;
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

//******************************************************************************
//
// This function is used to update the file information area of the screen.
//
//******************************************************************************
void
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
    else if(WaveOpen(&g_sFileObject, g_pcFilenames[sSelected],
                     &sWaveHeader) == FR_OK)
    {

        //
        // Update the file name information.
        //
        strncpy(g_pcFileName, g_pcFilenames[sSelected], 16);

        //
        // Print the formatted string so that it can be displayed.
        //
        usprintf(g_pcFormat, "%d Hz %d bit ", sWaveHeader.ulSampleRate,
                 sWaveHeader.usBitsPerSample);

        //
        // Concatenate the number of channels.
        //
        if(sWaveHeader.usNumChannels == 1)
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
        DisplayTime();
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
    WidgetPaint((tWidget *)&g_sSlider);
}

//******************************************************************************
//
// This function will handle stopping the playback of audio.  It will not do
// this immediately but will defer stopping audio at a later time.  This allows
// this function to be called from an interrupt handler.
//
//******************************************************************************
void
WaveStop(void)
{
    //
    // Stop playing audio.
    //
    g_ulFlags &= ~BUFFER_PLAYING;
}

//******************************************************************************
//
// This function will handle reading the correct amount from the wav file and
// will also handle converting 8 bit unsigned to 8 bit signed if necessary.
//
//******************************************************************************
unsigned short
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
// This will play the file passed in via the psFileObject parameter based on
// the format passed in the pWaveHeader structure.  The WaveOpen() function
// can be used to properly fill the pWaveHeader and psFileObject structures.
//
//******************************************************************************
unsigned long
WavePlay(FIL *psFileObject, tWaveHeader *pWaveHeader)
{
    static unsigned short usCount;

    //
    // Mark both buffers as empty.
    //
    g_ulFlags = BUFFER_BOTTOM_EMPTY | BUFFER_TOP_EMPTY;

    //
    // Set the format of the playback in the sound driver.
    //
    SoundSetFormat(pWaveHeader->ulSampleRate, pWaveHeader->usBitsPerSample,
                   pWaveHeader->usNumChannels);

    //
    // Indicate that the application is about to start playing.
    //
    g_ulFlags |= BUFFER_PLAYING;

    while(!g_bFirmwareUpdate)
    {
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
            usCount = WaveRead(psFileObject, pWaveHeader, g_pucBuffer);

            //
            // Start the playback for a new buffer.
            //
            SoundBufferPlay(g_pucBuffer, usCount, BufferCallback);

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
            usCount = WaveRead(psFileObject, pWaveHeader,
                               &g_pucBuffer[AUDIO_BUFFER_SIZE >> 1]);

            //
            // Start the playback for a new buffer.
            //
            SoundBufferPlay(&g_pucBuffer[AUDIO_BUFFER_SIZE >> 1],
                            usCount, BufferCallback);

            //
            // Top half of the buffer is now not empty.
            //
            g_ulFlags &= ~BUFFER_TOP_EMPTY;

            //
            // Update the current time display.
            //
            DisplayTime();
        }

        //
        // If something reset this while playing then stop playing and break
        // out of the loop.
        //
        if((g_ulFlags & BUFFER_PLAYING) == 0)
        {
            //
            // Change the text to indicate that the button is now for play.
            //
            strcpy(g_psPlayText, "Play");
            WidgetPaint((tWidget *)&g_sPlayBtn);

            //
            // Update the new file information if necessary.
            //
            UpdateFileInfo();

            break;
        }

        //
        // Audio playback is done once the count is below a full buffer.
        //
        if((usCount < g_ulMaxBufferSize) || (g_ulBytesRemaining == 0))
        {
            //
            // Change the text to indicate that the button is now for play.
            //
            strcpy(g_psPlayText, "Play");
            WidgetPaint((tWidget *)&g_sPlayBtn);

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

            break;
        }

        //
        // Must disable I2S interrupts during this time to prevent state
        // problems.
        //
        IntEnable(INT_I2S0);

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }

    //
    // Close out the file.
    //
    WaveClose(psFileObject);

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
void OnListBoxChange(tWidget *pWidget, short usSelected)
{
    //
    // Update only if playing a file.
    //
    if((g_ulFlags & BUFFER_PLAYING) == 0)
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

//******************************************************************************
//
// The "Play/Stop" button widget callback function.
//
// This function is called whenever someone presses the "Play/Stop" button.
//
//******************************************************************************
void OnBtnPlay(tWidget *pWidget)
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
            if(WaveOpen(&g_sFileObject, g_pcFilenames[sSelected], &sWaveHeader)
                   == FR_OK)
            {
                //
                // Change the text on the button to Stop.
                //
                strcpy(g_psPlayText, "Stop");
                WidgetPaint((tWidget *)&g_sPlayBtn);

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
void
OnSliderChange(tWidget *pWidget, long lValue)
{
    SoundVolumeSet(lValue);
}

//******************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a
// timer tick every 10 ms for internal timing purposes.  We also call the
// TCP/IP stack timer function.
//
//******************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();

    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
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
            // Ignore directories.
            //
            if((g_sFileInfo.fattrib & AM_DIR) == 0)
            {
                strncpy(g_pcFilenames[ulItemCount], g_sFileInfo.fname,
                         MAX_FILENAME_STRING_LEN);
                ListBoxTextAdd(&g_sDirList, g_pcFilenames[ulItemCount]);
            }
        }

        //
        // Ignore directories.
        //
        if((g_sFileInfo.fattrib & AM_DIR) == 0)
        {
            //
            // Move to the next entry in the item array we use to populate the
            // list box.
            //
            ulItemCount++;
        }
    }

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//******************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//******************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//******************************************************************************
//
// The program main function.  It performs initialization, then handles wav
// file playback.
//
//******************************************************************************
int
main(void)
{
    FRESULT fresult;
    unsigned long ulIPAddr;

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
    // Configure and enable uDMA
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    ROM_uDMAControlBaseSet(&sDMAControlTable[0]);
    ROM_uDMAEnable();

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
    Kitronix320x240x16_SSD2119Init();

    //
    // Turn on the display backlight at full brightness
    //
    Kitronix320x240x16_SSD2119BacklightOn(255);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Initialize the Ethernet hardware and lwIP TCP/IP stack.
    //
    ulIPAddr = TCPIPStackInit();

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Set some initial strings.
    //
    ListBoxTextAdd(&g_sDirList, "Initializing...");

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

    //
    // Configure the I2S peripheral.
    //
    SoundInit();

    //
    // Set the initial volume to something sensible.  Beware - if you make the
    // mistake of using 24 ohm headphones and setting the volume to 100% you
    // may find it is rather too loud!
    //
    SoundVolumeSet(INITIAL_VOLUME_PERCENT);

    //
    // Continue reading and processing commands from the user until a remote
    // firmware update request is received.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // If wav playback has started let the WavePlay routine take over main
        // routine.
        //
        if(g_ulFlags & BUFFER_PLAYING)
        {
            //
            // Try to play Wav file.
            //
            WavePlay(&g_sFileObject, &sWaveHeader);
        }

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();

        //
        // Check to see if we have been assigned an IP address and, if so,
        // update the display.
        //
        ulIPAddr = IPAddressChangeCheck(ulIPAddr);
    }

    //
    // If we drop out, a remote firmware update request has been received.
    // Let the user know what is going on then transfer control to the boot
    // loader.
    //
    CanvasTextSet(&g_sHeading, "Updating Firmware");
    WidgetPaint((tWidget *)&g_sHeading);
    WidgetMessageQueueProcess();

    //
    // Transfer control to the bootloader.
    //
    SoftwareUpdateBegin();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
