//******************************************************************************
//
// i2s_filter.c - Example program that records the line input from the codec and
// plays it back out the output.
//
// Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
//******************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "driverlib/i2s.h"
#include "drivers/sound.h"
#include "drivers/set_pinout.h"

//******************************************************************************
//
//! \addtogroup example_list
//! <h1>I2S Record and Playback (i2s_filter)</h1>
//!
//! This example application demonstrates recording audio from the codec's ADC,
//! transferring the audio over the I2S receive interface to the
//! microcontroller, and then sending the audio back to the codec via the I2S
//! trasmit interface.  A line level audio source should be fed into the LINE IN
//! audio jack which will be recorded with the codec's ADC and then played back
//! through both the HEADPHONE and LINE OUT jacks.  This application requires
//! some modifications to the default jumpers on the board in order for the
//! audio record path to function correctly.  The PD4/LD4 jumper should be
//! removed and placed on the PD4/RXSD jumper to receive I2S data.  The PD4/LD4
//! is located in the row of jumpers near the LCD connector while the PD4/RXSD
//! jumper is in the row near the audio jacks.
//!
//! \note Moving this jumper will cause the LCD to not function for other
//! applications so the jumper should be moved back to run other applications.
//!
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
// Widget definitions
//
//******************************************************************************
#define INITIAL_VOLUME_PERCENT  100

//******************************************************************************
//
// Buffer management and flags.
//
//******************************************************************************
#define AUDIO_BUFFER_SIZE       4096
static unsigned char g_pucBuffer[AUDIO_BUFFER_SIZE];

//
// Flags used in the g_ulFlags global variable.
//
#define FLAG_RECORD_TOP         0x00000001
#define FLAG_PLAY_TOP           0x00000002
#define FLAG_RUNNING            0x00000004

static volatile unsigned long g_ulFlags;

//******************************************************************************
//
// Handler for playing the buffer that has been filled from the audio in path.
//
//******************************************************************************
void
PlayBufferCallback(void *pvBuffer, unsigned long ulEvent)
{
    if(ulEvent & BUFFER_EVENT_FREE)
    {
        if(g_ulFlags & FLAG_PLAY_TOP)
        {
            //
            // Switch so that next time the bottom half of the buffer is used.
            //
            g_ulFlags &= ~FLAG_PLAY_TOP;

            //
            // Kick off another request for a buffer playback.
            //
            SoundBufferPlay(g_pucBuffer, AUDIO_BUFFER_SIZE >> 1,
                            PlayBufferCallback);
        }
        else
        {
            //
            // Switch so that next time the top half of the buffer is used.
            //
            g_ulFlags |= FLAG_PLAY_TOP;

            //
            // Kick off another request for a buffer playback.
            //
            SoundBufferPlay(&g_pucBuffer[AUDIO_BUFFER_SIZE >> 1],
                            AUDIO_BUFFER_SIZE >> 1,
                            PlayBufferCallback);
        }
    }
}

//*****************************************************************************
//
// Handles when a previous request to fill a buffer has completed and prepares
// a new buffer to be recorded.
//
//*****************************************************************************
static void
RecordBufferCallback(void *pvBuffer, unsigned long ulEvent)
{
    //
    // Only handle the case where a full buffer has been provided.
    //
    if(ulEvent & BUFFER_EVENT_FULL)
    {
        if(g_ulFlags & FLAG_RECORD_TOP)
        {
            //
            // Switch to recording to the bottom buffer next event.
            //
            g_ulFlags &= ~FLAG_RECORD_TOP;

            SoundBufferRead(&g_pucBuffer[AUDIO_BUFFER_SIZE >> 1],
                            AUDIO_BUFFER_SIZE >> 1,
                            RecordBufferCallback);
        }
        else
        {
            //
            // Switch to recording to the top buffer next event.
            //
            g_ulFlags |= FLAG_RECORD_TOP;

            SoundBufferRead(g_pucBuffer, AUDIO_BUFFER_SIZE >> 1,
                            RecordBufferCallback);
        }
    }
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
    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);


    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Configure and enable uDMA
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    uDMAControlBaseSet(&sDMAControlTable[0]);
    uDMAEnable();

    //
    // Enable Interrupts
    //
    IntMasterEnable();

    //
    // Not playing anything right now.
    //
    g_ulFlags = FLAG_RUNNING | FLAG_RECORD_TOP;

    //
    // Configure the I2S peripheral.
    //
    SoundInit(1);

    //
    // Set the format of the playback in the sound driver.
    //
    SoundSetFormat(48000, 16, 2);

    //
    // Set the initial volume to something sensible.  Beware - if you make the
    // mistake of using 24 ohm headphones and setting the volume to 100% you
    // may find it is rather too loud!
    //
    SoundVolumeSet(INITIAL_VOLUME_PERCENT);

    //
    // Start receiving data.
    //
    SoundBufferRead(g_pucBuffer, AUDIO_BUFFER_SIZE >> 1, RecordBufferCallback);

    //
    // Start playback of data, intial data should be null.
    //
    SoundBufferPlay(&g_pucBuffer[AUDIO_BUFFER_SIZE >> 1],
                    AUDIO_BUFFER_SIZE >> 1, PlayBufferCallback);

    //
    // The rest of the handling occurs at interrupt time so the main loop will
    // simply stall here.
    //
    while(1)
    {
    }
}
