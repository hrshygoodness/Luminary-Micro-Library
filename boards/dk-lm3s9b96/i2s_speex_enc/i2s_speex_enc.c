//*****************************************************************************
//
// i2s_speex_enc.c - Example program that records the line input from the
// codec encodes it using Speex and then decodes it and plays it back out the
// output of the codec.
//
// Copyright (c) 2010-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "driverlib/uart.h"
#include "driverlib/i2s.h"
#include "driverlib/rom.h"
#include "drivers/sound.h"
#include "drivers/set_pinout.h"
#include "utils/cmdline.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "third_party/bget/bget.h"
#include <speex/speex.h>
#include "utils/speexlib.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>I2S Record and Playback with Speex codec (i2s_speex_enc)</h1>
//!
//! This example application demonstrates recording audio from the codec's ADC,
//! transferring the audio over the I2S receive interface to the
//! microcontroller, encoding the audio using a Speex encoder and then decoding
//! the audio and sending the audio back to the codec via the I2S transmit
//! interface.  A line level audio source should be fed into the LINE IN
//! audio jack which will be recorded with the codec's ADC and then played back
//! through both the HEADPHONE and LINE OUT jacks.  The application provides a
//! simple command line interface via the virtual COM port.  To get the list
//! of supported commands, connect a serial communication program to the
//! virtual COM port at 115200, no parity, 8 data bits, one stop bit.  The
//! "help" command will provide a list of valid commands.  The current commands
//! are "bypass" which will disable the Speex encoder and will pass the audio
//! directly from input to output unmodified but still using the I2S record
//! data.  This is useful for hearing the audio at the current audio resolution
//! without any encoding and decoding.  The "speex" command takes and integer
//! quality parameter that ranges from 0-4.  The larger the number the higher
//! the quality setting for the encoder.
//!
//! This application requires some modifications to the default jumpers on the
//! board in order for the audio record path to function correctly.  The
//! PD4/LD4 jumper should be removed and placed on the PD4/RXSD jumper to
//! receive I2S data.  The PD4/LD4 is located in the row of jumpers near the
//! LCD connector while the PD4/RXSD jumper is in the row near the audio jacks.
//!
//! \note Moving this jumper will cause the LCD to not function for other
//! applications so the jumper should be moved back to run other applications.
//!
//
//*****************************************************************************

//*****************************************************************************
//
// The DMA control structure table.
//
//*****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// This is the memory buffer that is provided to the bget allocation routine
// and is used when the Speex library allocates memory.
//
//*****************************************************************************
#define HEAP_SIZE_BYTES         0x7800
static unsigned long g_pulHeap[HEAP_SIZE_BYTES>>2];

//*****************************************************************************
//
// Volume setting for the application.
//
//*****************************************************************************
#define INITIAL_VOLUME_PERCENT  60

//*****************************************************************************
//
// Audio format, these can be changed to try different formats for streaming.
//
//*****************************************************************************
#define AUDIO_BITS              16
#define AUDIO_CHANNELS          1
#define AUDIO_RATE              (8000)

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// The size of the audio buffers used by the application.
//
//*****************************************************************************
#define RECORD_BUFFER_INC       (160 * 2)
#define RECORD_BUFFER_SIZE      (RECORD_BUFFER_INC * 3)

#define PLAY_BUFFER_INC         (160 * 2)
#define PLAY_BUFFER_SIZE        (PLAY_BUFFER_INC * 2)

#define ENC_BUFFER_SIZE         (160 * 2)

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];

//*****************************************************************************
//
// The various buffers used in the audio signal chain.  The g_pucRecBuffer
// is filled with audio from the I2S audio input and the g_pucEncBuffer is
// filled by encoding audio from the g_pucRecBuffer using the Speex encoder.
// The g_pucPlayBuffer buffer is filled by decoding data from the
// g_pucEncBuffer and passing it to the I2S audio output.
//
//*****************************************************************************
static unsigned char g_pucRecBuffer[RECORD_BUFFER_SIZE];
static unsigned char g_pucEncBuffer[ENC_BUFFER_SIZE];
static unsigned char g_pucPlayBuffer[PLAY_BUFFER_SIZE];
static unsigned char *g_pucEncode;
static unsigned char *g_pucPlay;

//
// The size of the last encoded buffer.
//
static unsigned long g_ulEncSize;

//
// Enable audio bypass mode.
//
#define FLAG_BYPASS             1

//
// The state flags for the application.
//
static unsigned long g_ulFlags;

//
// The current Speex quality setting.
//
static int g_iQuality;

static void PlayBufferCallback(void *pvBuffer, unsigned long ulEvent);
static void RecordBufferCallback(void *pvBuffer, unsigned long ulEvent);

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list
// of the available commands with a brief description.
//
//*****************************************************************************
int
Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    //
    // Print some header text.
    //
    UARTprintf("\nAvailable commands\n");
    UARTprintf("------------------\n");

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_sCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The
    // end of the table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        UARTprintf("%s%s\n", pEntry->pcCmd, pEntry->pcHelp);

        //
        // Advance to the next entry in the table.
        //
        pEntry++;
    }

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// Enable the Speex encoder/decoder and allow the quality setting to be set
// by a single decimal argument.
//
//*****************************************************************************
int
Cmd_speex(int argc, char *argv[])
{
    unsigned long ulTemp;

    //
    // Clear the pass through flag.
    //
    HWREGBITW(&g_ulFlags, FLAG_BYPASS) = 0;

    //
    // Check for an argument that is specifying a quality setting.
    //
    if(argc == 2)
    {
        //
        // Convert the value to a numeric value.
        //
        ulTemp = ustrtoul(argv[1], 0, 10);

        //
        // Rail the quality setting at 4.
        //
        if(ulTemp <= 4)
        {
            g_iQuality = (int)ulTemp;

            //
            // Set the quality setting.
            //
            SpeexEncodeQualitySet(g_iQuality);

            UARTprintf("Speex Encoder Quality set to %d\n", g_iQuality);
        }
        else
        {
            UARTprintf("Encoder Quality not changed, value must be (1-4).\n");
        }
    }

    return(0);
}

//*****************************************************************************
//
// Disable the Speex encoder/decoder to allow the audio to stream through
// unmodified.
//
//*****************************************************************************
int
Cmd_bypass(int argc, char *argv[])
{
    //
    // Set the pass through flag so that audio encode/decode is skipped.
    //
    HWREGBITW(&g_ulFlags, FLAG_BYPASS) = 1;

    return(0);
}

//*****************************************************************************
//
// Initialize all of the buffer state.
//
//*****************************************************************************
static void
InitBuffers(void)
{
    //
    // Move the read pointer one increment in front of the write pointer.
    //
    g_pucEncode = g_pucRecBuffer;

    //
    // Move the write pointer one increment in front of the read pointer.
    //
    g_pucPlay = g_pucPlayBuffer;
}

//*****************************************************************************
//
// This function is called to decode a buffer into a playable buffer.
//
//*****************************************************************************
void
Decode(void *pvBuffer, unsigned long ulSize)
{
    //
    // If in pass through mode then just copy the data to the play buffer.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_BYPASS))
    {
        memcpy(g_pucPlay, pvBuffer, PLAY_BUFFER_INC);
    }
    else
    {
        //
        // Decode the frame and write it out to play buffer.
        //
        SpeexDecode(pvBuffer, ulSize, g_pucPlay, PLAY_BUFFER_INC);
    }

    //
    // Schedule the new buffer to start playing.
    //
    SoundBufferPlay(g_pucPlay, PLAY_BUFFER_INC, PlayBufferCallback);

    //
    // Advance the play pointer.
    //
    g_pucPlay += PLAY_BUFFER_INC;

    //
    // Wrap the play pointer if necessary.
    //
    if(g_pucPlay >= &g_pucPlayBuffer[PLAY_BUFFER_SIZE])
    {
        g_pucPlay = g_pucPlayBuffer;
    }
}

//*****************************************************************************
//
// This function is called to take PCM audio and use the Speex encoder to
// encode it at the current quality setting.
//
//*****************************************************************************
void
Encode(void *pvBuffer)
{
    //
    // Only copy the data if the application is in pass through mode.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_BYPASS))
    {
        memcpy(g_pucEncBuffer, pvBuffer, RECORD_BUFFER_INC);
    }
    else
    {
        //
        // Just encode this buffer into the g_pucEncBuffer.
        //
        g_ulEncSize = SpeexEncode((short *)pvBuffer, RECORD_BUFFER_INC,
                                  (unsigned char *)g_pucEncBuffer,
                                  ENC_BUFFER_SIZE);
    }
}

//*****************************************************************************
//
// Handler for playing the buffer that has been filled from the audio in path.
//
//*****************************************************************************
void
PlayBufferCallback(void *pvBuffer, unsigned long ulEvent)
{
    if(ulEvent & BUFFER_EVENT_FREE)
    {
        //
        // Use the currently played buffer as the next to play.
        //
        g_pucPlay = pvBuffer;
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
    if(ulEvent & BUFFER_EVENT_FULL)
    {
        //
        // Encode the current buffer.
        //
        Encode(pvBuffer);

        //
        // Move the encode position up.
        //
        g_pucEncode += RECORD_BUFFER_INC;

        //
        // Wrap the encoder location.
        //
        if(g_pucEncode >= &g_pucRecBuffer[RECORD_BUFFER_SIZE])
        {
            g_pucEncode = g_pucRecBuffer;
        }

        //
        // Kick off another request for a buffer play back.
        //
        SoundBufferRead(g_pucEncode, RECORD_BUFFER_INC, RecordBufferCallback);

        //
        // Now decode the buffer.
        //
        Decode(g_pucEncBuffer, g_ulEncSize);
    }
}

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions,
// and brief description.
//
//*****************************************************************************
tCmdLineEntry g_sCmdTable[] =
{
    { "help",   Cmd_help,          " : Display list of commands" },
    { "h",      Cmd_help,       "    : alias for help" },
    { "?",      Cmd_help,       "    : alias for help" },
    { "speex",  Cmd_speex,  "  : Enable Speex with quality (0-4)" },
    { "bypass", Cmd_bypass,  "  : Bypass Speex encode/decode." },
    { 0, 0, 0 }
};

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
    int nStatus;

    //
    // Set the system clock to run at 80MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Give bget some memory to work with.
    //
    bpool(g_pulHeap, sizeof(g_pulHeap));

    //
    // Set the device pin out appropriately for this board.
    //
    PinoutSet();

    //
    // Configure the relevant pins such that UART0 owns them.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Open the UART for I/O
    //
    UARTStdioInit(0);

    UARTprintf("i2s_speex_enc\n");

    //
    // Configure and enable uDMA
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    ROM_uDMAControlBaseSet(&sDMAControlTable[0]);
    ROM_uDMAEnable();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Configure the I2S peripheral.
    //
    SoundInit(1);

    //
    // Set the format of the play back in the sound driver.
    //
    SoundSetFormat(AUDIO_RATE, AUDIO_BITS, AUDIO_CHANNELS);

    //
    // Print out some header information to the serial console.
    //
    UARTprintf("\ni2s_speex_enc Stellaris Example\n");
    UARTprintf("Streaming at %d %d bit ",SoundSampleRateGet(), AUDIO_BITS);

    if(AUDIO_CHANNELS == 2)
    {
        UARTprintf("Stereo\n");
    }
    else
    {
        UARTprintf("Mono\n");
    }

    //
    // Set the initial volume.
    //
    SoundVolumeSet(INITIAL_VOLUME_PERCENT);

    //
    // Initialize the Speex decoder.
    //
    SpeexDecodeInit();

    //
    // Set the default quality to 2.
    //
    g_iQuality = 2;

    //
    // Initialize the Speex encoder to Complexity of 1 and Quality 2.
    //
    SpeexEncodeInit(AUDIO_RATE, 1, g_iQuality);

    //
    // Initialize the audio buffers.
    //
    InitBuffers();

    //
    // Initialize the applications global state flags.
    //
    g_ulFlags = 0;

    //
    // Kick off a request for a buffer play back and advance the encoder
    // pointer.
    //
    SoundBufferRead(g_pucRecBuffer, RECORD_BUFFER_INC,
                    RecordBufferCallback);
    g_pucEncode += RECORD_BUFFER_INC;

    //
    // Kick off a second request for a buffer play back and advance the encode
    // pointer.
    //
    SoundBufferRead(&g_pucRecBuffer[RECORD_BUFFER_INC], RECORD_BUFFER_INC,
                    RecordBufferCallback);
    g_pucEncode += RECORD_BUFFER_INC;

    //
    // The rest of the handling occurs at interrupt time so the main loop will
    // simply stall here.
    //
    while(1)
    {
        //
        // Print a prompt to the console.  Show the CWD.
        //
        UARTprintf("\n> ");

        //
        // Get a line of text from the user.
        //
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));

        //
        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        //
        nStatus = CmdLineProcess(g_cCmdBuf);

        //
        // Handle the case of bad command.
        //
        if(nStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(nStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }

        //
        // Otherwise the command was executed.  Print the error
        // code if one was returned.
        //
        else if(nStatus != 0)
        {
            UARTprintf("Command returned error code \n");
        }
    }
}
