//*****************************************************************************
//
// sound.c - Sound driver for the IDM-SBC reference design kit.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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

//*****************************************************************************
//
//! \addtogroup sound_api
//! @{
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "inc/hw_udma.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_i2s.h"
#include "driverlib/i2s.h"
#include "driverlib/udma.h"
#include "drivers/wm8510.h"
#include "drivers/sound.h"

//*****************************************************************************
//
// The current volume of the music/sound effects.
//
//*****************************************************************************
static unsigned char g_ucVolume = 100;

//*****************************************************************************
//
// A pointer to the song currently being played, if any.  The value of this
// variable is used to determine whether or not a song is being played.  Since
// each entry is a short, the maximum length of the song is 65536 / 200
// seconds, which is around 327 seconds.
//
//*****************************************************************************
static const unsigned short *g_pusMusic = 0;

//*****************************************************************************
//
// Interrupt values for tone generation.
//
//*****************************************************************************
static unsigned long g_ulFrequency;
static unsigned long g_ulDACStep;
static unsigned long g_ulSize;
static unsigned long g_ulTicks;
static unsigned short g_ulMusicCount;
static unsigned short g_ulMusicSize;
#define SAMPLE_RATE             48000

#define SAMPLE_LEFT_UP          0x00000001
#define SAMPLE_RIGHT_UP         0x00000002

//*****************************************************************************
//
// Sawtooth state information, this allows for a phase difference between left
// and right waveforms.
//
//*****************************************************************************
volatile struct
{
    int iSample;
    unsigned long ulFlags;
} g_sSample;

#define NUM_SAMPLES             512
static unsigned long g_pulTxBuf[NUM_SAMPLES];

//*****************************************************************************
//
// I2S MCLK enables and divisors for 48KHz.
//
//*****************************************************************************
#define I2S_RX_8MHZ_48KHZ      ((SYSCTL_I2SMCLKCFG_RXEN) |                  \
                                ((32 << 4) | (8 << 0)) << 16)
#define I2S_TX_8MHZ_48KHZ      ((SYSCTL_I2SMCLKCFG_TXEN) |                  \
                                (32 << 4) | (8 << 0))

#define I2S_RX_8MHZ_44KHZ      ((SYSCTL_I2SMCLKCFG_RXEN) |                  \
                                ((35 << 4) | (7 << 0)) << 16)
#define I2S_TX_8MHZ_44KHZ      ((SYSCTL_I2SMCLKCFG_TXEN) |                  \
                                (35 << 4) | (7 << 0))

#define I2S_RX_8MHZ_22KHZ      ((SYSCTL_I2SMCLKCFG_RXEN) |                  \
                                ((70 << 4) | (13 << 0)) << 16)
#define I2S_TX_8MHZ_22KHZ      ((SYSCTL_I2SMCLKCFG_TXEN) |                  \
                                (70 << 4) | (13 << 0))

#define I2S_RX_8MHZ_11KHZ      ((SYSCTL_I2SMCLKCFG_RXEN) |                  \
                                ((141 << 4) | (11 << 0)) << 16)
#define I2S_TX_8MHZ_11KHZ      ((SYSCTL_I2SMCLKCFG_TXEN) |                  \
                                (141 << 4) | (11 << 0))

//*****************************************************************************
//
// I2S Pin definitions.
//
//*****************************************************************************
#define I2S0_LRCTX_PERIPH       (SYSCTL_PERIPH_GPIOE)
#define I2S0_LRCTX_PORT         (GPIO_PORTE_BASE)
#define I2S0_LRCTX_PIN          (GPIO_PIN_4)

#define I2S0_SCLKTX_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define I2S0_SCLKTX_PORT        (GPIO_PORTB_BASE)
#define I2S0_SCLKTX_PIN         (GPIO_PIN_6)

#define I2S0_SDATX_PERIPH       (SYSCTL_PERIPH_GPIOE)
#define I2S0_SDATX_PORT         (GPIO_PORTE_BASE)
#define I2S0_SDATX_PIN          (GPIO_PIN_5)

#define I2S0_MCLKTX_PERIPH      (SYSCTL_PERIPH_GPIOF)
#define I2S0_MCLKTX_PORT        (GPIO_PORTF_BASE)
#define I2S0_MCLKTX_PIN         (GPIO_PIN_1)

//*****************************************************************************
//
// Buffer management structures and defines.
//
//*****************************************************************************

#define NUM_BUFFERS             2

static struct
{
    //
    // Pointer to the buffer.
    //
    const unsigned long *pulData;

    //
    // Size of the buffer.
    //
    unsigned long ulSize;

    //
    // Callback function for this buffer.
    //
    tBufferCallback pfnBufferCallback;
}
g_sBuffers[NUM_BUFFERS];

//*****************************************************************************
//
// A set of flags.  The flag bits are defined as follows:
//
//     1 -> A TX DMA transfer is pending.
//
//*****************************************************************************
#define FLAG_TX_PENDING         1
static volatile unsigned long g_ulDMAFlags;

//*****************************************************************************
//
// The buffer index that is currently playing.
//
//*****************************************************************************
static unsigned long g_ulPlaying;

static unsigned long g_ulSampleRate;
static unsigned short g_usChannels;
static unsigned short g_usBitsPerSample;

//*****************************************************************************
//
// This function is used to generate a pattern to fill the TX buffer.
//
//*****************************************************************************
static unsigned long
PatternNext(void)
{
    int iSample;

    if(g_sSample.ulFlags & SAMPLE_LEFT_UP)
    {
        g_sSample.iSample += g_ulDACStep;
        if(g_sSample.iSample >= 32767)
        {
            g_sSample.ulFlags &= ~SAMPLE_LEFT_UP;
            g_sSample.iSample = 32768 - g_ulDACStep;
        }
    }
    else
    {
        g_sSample.iSample -= g_ulDACStep;
        if(g_sSample.iSample <= -32768)
        {
            g_sSample.ulFlags |= SAMPLE_LEFT_UP;
            g_sSample.iSample = g_ulDACStep - 32768;
        }
    }

    //
    // Copy the sample to prevent a compiler warning on the return line.
    //
    iSample = g_sSample.iSample;
    return((iSample & 0xffff) | (iSample << 16));
}

//*****************************************************************************
//
// Generate the next tone.
//
//*****************************************************************************
static unsigned long
SoundNextTone(void)
{
    int iIdx;

    g_sSample.iSample = 0;
    g_sSample.ulFlags = SAMPLE_LEFT_UP;

    //
    // Set the frequency.
    //
    g_ulFrequency = g_pusMusic[g_ulMusicCount + 1];

    //
    // Calculate the step size for each sample.
    //
    g_ulDACStep = ((65536 * 2 * g_ulFrequency) / SAMPLE_RATE);

    //
    // How big is the buffer that needs to be restarted.
    //
    g_ulSize = (SAMPLE_RATE/g_ulFrequency);

    //
    // Cap the size in a somewhat noisy way.  This will affect frequencies below
    // 93.75 Hz or 48000/NUM_SAMPLES.
    //
    if(g_ulSize > NUM_SAMPLES)
    {
        g_ulSize = NUM_SAMPLES;
    }

    //
    // Move on to the next value.
    //
    g_ulMusicCount += 2;

    //
    // Stop if there are no more entries in the list.
    //
    if(g_ulMusicCount < g_ulMusicSize)
    {
        g_ulTicks = (g_pusMusic[g_ulMusicCount] * g_ulFrequency) / 1000;
    }
    else
    {
        g_ulTicks = 0;
    }

    //
    // Fill the buffer with the new tone.
    //
    for(iIdx = 0; iIdx < g_ulSize; iIdx++)
    {
        g_pulTxBuf[iIdx] = PatternNext();
    }

    //
    // This should be the size in bytes and not words.
    //
    g_ulSize <<= 2;

    return(g_ulTicks);
}

//*****************************************************************************
//
// Handles playback of the single buffer when playing tones.
//
//*****************************************************************************
static void
BufferCallback(const void *pvBuffer, unsigned long ulEvent)
{
    if((ulEvent & BUFFER_EVENT_FREE) && (g_ulTicks != 0))
    {
        //
        // Kick off another request for a buffer playback.
        //
        SoundBufferPlay(pvBuffer, g_ulSize, BufferCallback);

        //
        // Count down before stopping.
        //
        g_ulTicks--;
    }
    else
    {
        //
        // Stop requesting transfers.
        //
        I2STxDisable(I2S0_BASE);
    }
}

//*****************************************************************************
//
//! Disables the sound output.
//!
//! This function disables the sound output, muting the speaker and cancelling
//! any playback that may be in progress.
//!
//! \return None.
//
//*****************************************************************************
void
SoundDisable(void)
{
    //
    // Cancel any song or sound effect playback that may be in progress.
    //
    g_pusMusic = 0;

    //
    // Indicate that there are no more pending transfers.
    //
    HWREGBITW(&g_ulDMAFlags, FLAG_TX_PENDING) = 0;
}

//*****************************************************************************
//
//! Initializes the sound output.
//!
//! This function prepares the sound driver to play songs or sound effects.  It
//! must be called before any other sound function.  The sound driver uses
//! uDMA and the caller must ensure that the uDMA peripheral is enabled and
//! its control table configured prior to making this call.
//!
//! \return None
//
//*****************************************************************************
void
SoundInit(void)
{
    //
    // Set the current active buffer to zero.
    //
    g_ulPlaying = 0;

    //
    // Enable and reset the peripheral.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2S0);

    //
    // Select alternate functions for all of the I2S pins.
    //
    ROM_SysCtlPeripheralEnable(I2S0_SCLKTX_PERIPH);
    GPIOPinTypeI2S(I2S0_SCLKTX_PORT, I2S0_SCLKTX_PIN);

    ROM_SysCtlPeripheralEnable(I2S0_LRCTX_PERIPH);
    GPIOPinTypeI2S(I2S0_LRCTX_PORT, I2S0_LRCTX_PIN);

    ROM_SysCtlPeripheralEnable(I2S0_SDATX_PERIPH);
    GPIOPinTypeI2S(I2S0_SDATX_PORT, I2S0_SDATX_PIN);

    ROM_SysCtlPeripheralEnable(I2S0_MCLKTX_PERIPH);
    GPIOPinTypeI2S(I2S0_MCLKTX_PORT, I2S0_MCLKTX_PIN);

    //
    // Initialize the DAC.
    //
    WM8510Init();

    //
    // Set the intial volume level
    //
    WM8510VolumeSet(g_ucVolume);

    //
    // Set the FIFO trigger limit
    //
    I2STxFIFOLimitSet(I2S0_BASE, 4);

    //
    // Clear out all pending interrupts.
    //
    I2SIntClear(I2S0_BASE, I2S_INT_TXERR | I2S_INT_TXREQ );

    //
    // Enable the I2S interrupt on the NVIC
    //
    ROM_IntEnable(INT_I2S0);

    //
    // Disable all uDMA attributes.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_I2S0TX, UDMA_ATTR_ALL);
}

//*****************************************************************************
//
//! Handles the I2S sound interrupt.
//!
//! This function services the I2S interrupt and ensures that DMA buffers are
//! correctly handled to ensure smooth flow of audio data to the DAC.
//!
//! \return None.
//
//*****************************************************************************
void
SoundIntHandler(void)
{
    unsigned long ulStatus;
    unsigned long *pulTemp;

    //
    // Get the interrupt status and clear any pending interrupts.
    //
    ulStatus = I2SIntStatus(I2S0_BASE, 1);

    //
    // Clear out any interrupts.
    //
    I2SIntClear(I2S0_BASE, ulStatus);

    //
    // Handle the TX channel interrupt
    //
    if(HWREGBITW(&g_ulDMAFlags, FLAG_TX_PENDING))
    {
        //
        // If the TX DMA is done, then call the callback if present.
        //
        if(ROM_uDMAChannelModeGet(UDMA_CHANNEL_I2S0TX | UDMA_PRI_SELECT) ==
           UDMA_MODE_STOP)
        {
            //
            // Save a temp pointer so that the current pointer can be set to
            // zero before calling the callback.
            //
            pulTemp = (unsigned long *)g_sBuffers[0].pulData;

            //
            // If at the mid point then refill the first half of the buffer.
            //
            if((g_sBuffers[0].pfnBufferCallback) &&
               (g_sBuffers[0].pulData != 0))
            {
                g_sBuffers[0].pulData = 0;
                g_sBuffers[0].pfnBufferCallback(pulTemp, BUFFER_EVENT_FREE);
            }
        }

        //
        // If the TX DMA is done, then call the callback if present.
        //
        if(ROM_uDMAChannelModeGet(UDMA_CHANNEL_I2S0TX | UDMA_ALT_SELECT) ==
           UDMA_MODE_STOP)
        {
            //
            // Save a temp pointer so that the current pointer can be set to
            // zero before calling the callback.
            //
            pulTemp = (unsigned long *)g_sBuffers[1].pulData;

            //
            // If at the mid point then refill the first half of the buffer.
            //
            if((g_sBuffers[1].pfnBufferCallback) &&
               (g_sBuffers[1].pulData != 0))
            {
                g_sBuffers[1].pulData = 0;
                g_sBuffers[1].pfnBufferCallback(pulTemp, BUFFER_EVENT_FREE);
            }
        }

        //
        // If no more buffers are pending then clear the flag.
        //
        if((g_sBuffers[0].pulData == 0) && (g_sBuffers[1].pulData == 0))
        {
            HWREGBITW(&g_ulDMAFlags, FLAG_TX_PENDING) = 0;
        }
    }
}

//*****************************************************************************
//
//! Starts playback of a song.
//!
//! \param pusSong is a pointer to the song data structure.
//! \param ulLength is the length of the song data structure in bytes.
//!
//! This function starts the playback of a song or sound effect.  If a song
//! or sound effect is already being played, its playback is cancelled and the
//! new song is started.
//!
//! \return None.
//
//*****************************************************************************
void
SoundPlay(const unsigned short *pusSong, unsigned long ulLength)
{
    SoundSetFormat(48000, 16, 2);

    //
    // Save the music buffer.
    //
    g_ulMusicCount = 0;
    g_ulMusicSize = ulLength * 2;
    g_pusMusic = pusSong;
    g_ulPlaying = 0;

    g_sBuffers[0].pulData = 0;
    g_sBuffers[1].pulData = 0;

    if(SoundNextTone() != 0)
    {
        SoundBufferPlay(g_pulTxBuf, g_ulSize, BufferCallback);
        SoundBufferPlay(g_pulTxBuf, g_ulSize, BufferCallback);
    }
}

//*****************************************************************************
//
//! Configures the I2S peripheral for the given audio data format.
//!
//! \param ulSampleRate is the sample rate of the audio to be played in
//! samples per second.
//! \param usBitsPerSample is the number of bits in each audio sample.
//! \param usChannels is the number of audio channels, 1 for mono, 2 for stereo.
//!
//! This function configures the I2S peripheral in preparation for playing
//! audio data of a particular format.
//!
//! \return None.
//
//*****************************************************************************
void SoundSetFormat(unsigned long ulSampleRate, unsigned short usBitsPerSample,
                    unsigned short usChannels)
{
    unsigned long ulFormat;

    //
    // Save these values for use when configuring I2S.
    //
    g_ulSampleRate = ulSampleRate;
    g_usChannels = usChannels;
    g_usBitsPerSample = usBitsPerSample;

    I2SMasterClockSelect(I2S0_BASE, 0);

    //
    // Set the sample rate.
    //
    switch(g_ulSampleRate)
    {
        case 48000:
        {
            HWREG(SYSCTL_I2SMCLKCFG) = I2S_TX_8MHZ_48KHZ | I2S_RX_8MHZ_48KHZ;
            break;
        }
        case 44100:
        {
            HWREG(SYSCTL_I2SMCLKCFG) = I2S_TX_8MHZ_44KHZ | I2S_RX_8MHZ_44KHZ;
            break;
        }
        case 22222:
        case 22050:
        {
            HWREG(SYSCTL_I2SMCLKCFG) = I2S_TX_8MHZ_22KHZ | I2S_RX_8MHZ_22KHZ;
            break;
        }
        default:
        case 11025:
        {
            HWREG(SYSCTL_I2SMCLKCFG) = I2S_TX_8MHZ_11KHZ | I2S_RX_8MHZ_11KHZ;
            break;
        }
    }

    ulFormat = I2S_CONFIG_FORMAT_I2S | I2S_CONFIG_CLK_MASTER |
               I2S_CONFIG_EMPTY_ZERO | I2S_CONFIG_WIRE_SIZE_32;

    if(g_usChannels == 1)
    {
        if(g_usBitsPerSample == 8)
        {
            ulFormat |= I2S_CONFIG_MODE_MONO | I2S_CONFIG_SAMPLE_SIZE_8;
        }
        else
        {
            ulFormat |= I2S_CONFIG_MODE_COMPACT_16 | I2S_CONFIG_SAMPLE_SIZE_16;
        }
    }
    else
    {
        if(g_usBitsPerSample == 8)
        {
            ulFormat |= I2S_CONFIG_MODE_COMPACT_8 | I2S_CONFIG_SAMPLE_SIZE_8;
        }
        else
        {
            ulFormat |= I2S_CONFIG_MODE_COMPACT_16 | I2S_CONFIG_SAMPLE_SIZE_16;
        }
    }

    //
    // Configure the I2S TX and RX blocks.
    //
    I2STxConfigSet(I2S0_BASE, ulFormat);
    I2SRxConfigSet(I2S0_BASE, ulFormat);
    I2SMasterClockSelect(I2S0_BASE, I2S_TX_MCLK_INT | I2S_RX_MCLK_INT);
}

//*****************************************************************************
//
//! Starts playback of a block of PCM audio samples.
//!
//! \param pvData is a pointer to the audio data to play.
//! \param ulLength is the length of the data in bytes.
//! \param pfnCallback is a function to call when this buffer has be played.
//!
//! This function starts the playback of a block of PCM audio samples.  If
//! playback of another buffer is currently ongoing, its playback is cancelled
//! and the buffer starts playing immediately.
//!
//! \return 0 if the buffer was accepted, returns non-zero if there was no
//! space available for this buffer.
//
//*****************************************************************************
unsigned long
SoundBufferPlay(const void *pvData, unsigned long ulLength,
                tBufferCallback pfnCallback)
{
    unsigned long ulChannel;
    unsigned long ulDMASetting;

    //
    // Must disable I2S interrupts during this time to prevent state problems.
    //
    ROM_IntDisable(INT_I2S0);

    //
    // Save the buffer information.
    //
    g_sBuffers[g_ulPlaying].pulData = (const unsigned long *)pvData;
    g_sBuffers[g_ulPlaying].ulSize = ulLength;
    g_sBuffers[g_ulPlaying].pfnBufferCallback = pfnCallback;

    //
    // Configure the I2S TX DMA channel.
    // Program it to only use burst transfer.  The arb size is 8 to
    // match the FIFO trigger level (set above).
    // The transfer size is 32 bits, from the TX buffer to the
    // TX FIFO.
    //
    ROM_uDMAChannelAttributeEnable(UDMA_CHANNEL_I2S0TX,
                                  (UDMA_ATTR_USEBURST |
                                   UDMA_ATTR_HIGH_PRIORITY));

    if(g_ulPlaying)
    {
        ulChannel = UDMA_CHANNEL_I2S0TX | UDMA_ALT_SELECT;
    }
    else
    {
        ulChannel = UDMA_CHANNEL_I2S0TX | UDMA_PRI_SELECT;
    }

    if(g_usChannels == 1)
    {
        if(g_usBitsPerSample == 8)
        {
            ulDMASetting = UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                           UDMA_DST_INC_NONE | UDMA_ARB_4;
        }
        else
        {
            ulDMASetting = UDMA_SIZE_16 | UDMA_SRC_INC_16 |
                           UDMA_DST_INC_NONE | UDMA_ARB_4;

            //
            // Modify the DMA transfer is 16 bits.
            //
            g_sBuffers[g_ulPlaying].ulSize >>= 1;
        }
    }
    else
    {
        if(g_usBitsPerSample == 8)
        {
            ulDMASetting = UDMA_SIZE_16 | UDMA_SRC_INC_16 |
                           UDMA_DST_INC_NONE | UDMA_ARB_4;
            //
            // Modify the DMA transfer is 16 bits.
            //
            g_sBuffers[g_ulPlaying].ulSize >>= 1;
        }
        else
        {
            ulDMASetting = UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                            UDMA_DST_INC_NONE | UDMA_ARB_4;

            //
            // Modify the DMA transfer is 32 bits.
            //
            g_sBuffers[g_ulPlaying].ulSize >>= 2;
        }
    }

    ROM_uDMAChannelControlSet(ulChannel, ulDMASetting);

    ROM_uDMAChannelTransferSet(ulChannel,
                               UDMA_MODE_PINGPONG,
                               (unsigned long *)g_sBuffers[g_ulPlaying].pulData,
                               (void *)(I2S0_BASE + I2S_O_TXFIFO),
                           g_sBuffers[g_ulPlaying].ulSize);

    //
    // Enable the TX channel.  At this point the uDMA controller will
    // start servicing the request from the I2S, and the transmit side
    // should start running.
    //
    ROM_uDMAChannelEnable(UDMA_CHANNEL_I2S0TX);

    //
    // Indicate that there is still a pending transfer.
    //
    HWREGBITW(&g_ulDMAFlags, FLAG_TX_PENDING) = 1;

    //
    // Toggle the next side to use.
    //
    g_ulPlaying ^= 1;

    //
    //
    I2STxEnable(I2S0_BASE);

    //
    // Re-enable I2S interrupts.
    //
    ROM_IntEnable(INT_I2S0);

    return(0);
}

//*****************************************************************************
//
//! Sets the volume of the music/sound effect playback.
//!
//! \param ulPercent is the volume percentage, which must be between 0%
//! (silence) and 100% (full volume), inclusive.
//!
//! This function sets the volume of the sound output to a value between
//! silence (0%) and full volume (100%).
//!
//! \return None.
//
//*****************************************************************************
void
SoundVolumeSet(unsigned long ulPercent)
{
    WM8510VolumeSet(ulPercent);
}

//*****************************************************************************
//
//! Decreases the volume.
//!
//! \param ulPercent is the amount to decrease the volume, specified as a
//! percentage between 0% (silence) and 100% (full volume), inclusive.
//!
//! This function adjusts the audio output down by the specified percentage.
//! The adjusted volume will not go below 0% (silence).
//!
//! \return None.
//
//*****************************************************************************
void
SoundVolumeDown(unsigned long ulPercent)
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
    // Set the new volume.
    //
    SoundVolumeSet(g_ucVolume);
}

//*****************************************************************************
//
//! Returns the current volume level.
//!
//! This function returns the current volume, specified as a percentage between
//! 0% (silence) and 100% (full volume), inclusive.
//!
//! \return Returns the current volume.
//
//*****************************************************************************
unsigned char
SoundVolumeGet(void)
{
    //
    // Return the current Audio Volume.
    //
    return(g_ucVolume);
}

//*****************************************************************************
//
//! Increases the volume.
//!
//! \param ulPercent is the amount to increase the volume, specified as a
//! percentage between 0% (silence) and 100% (full volume), inclusive.
//!
//! This function adjusts the audio output up by the specified percentage.  The
//! adjusted volume will not go above 100% (full volume).
//!
//! \return None.
//
//*****************************************************************************
void
SoundVolumeUp(unsigned long ulPercent)
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
    // Set the new volume.
    //
    SoundVolumeSet(g_ucVolume);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
