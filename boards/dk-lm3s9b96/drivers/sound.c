//*****************************************************************************
//
// sound.c - Sound driver for the development board.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
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
#include "inc/hw_udma.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_i2s.h"
#include "driverlib/i2s.h"
#include "driverlib/udma.h"
#include "drivers/tlv320aic23b.h"
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
// I2S Pin definitions.
//
//*****************************************************************************
#define I2S0_LRCTX_PERIPH       (SYSCTL_PERIPH_GPIOE)
#define I2S0_LRCTX_PORT         (GPIO_PORTE_BASE)
#define I2S0_LRCTX_PIN          (GPIO_PIN_4)

#define I2S0_SDATX_PERIPH       (SYSCTL_PERIPH_GPIOE)
#define I2S0_SDATX_PORT         (GPIO_PORTE_BASE)
#define I2S0_SDATX_PIN          (GPIO_PIN_5)

#define I2S0_SCLKTX_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define I2S0_SCLKTX_PORT        (GPIO_PORTB_BASE)
#define I2S0_SCLKTX_PIN         (GPIO_PIN_6)

#define I2S0_SDARX_PERIPH       (SYSCTL_PERIPH_GPIOD)
#define I2S0_SDARX_PORT         (GPIO_PORTD_BASE)
#define I2S0_SDARX_PIN          (GPIO_PIN_4)

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
    unsigned long *pulData;

    //
    // Size of the buffer.
    //
    unsigned long ulSize;

    //
    // Callback function for this buffer.
    //
    tBufferCallback pfnBufferCallback;
}
g_sOutBuffers[NUM_BUFFERS],
g_sInBuffers[NUM_BUFFERS];

//*****************************************************************************
//
// Global used to remember the correct FIFO record address to read so that it
// is not calculated multiple times.
//
//*****************************************************************************
static void * g_pvFIFORecord;

//*****************************************************************************
//
// A set of flags.  The flag bits are defined as follows:
//
//     0 -> A RX DMA transfer is pending.
//     1 -> A TX DMA transfer is pending.
//
//*****************************************************************************
#define FLAG_RX_PENDING         0
#define FLAG_TX_PENDING         1
static volatile unsigned long g_ulDMAFlags;

//*****************************************************************************
//
// The buffer index that is currently playing.
//
//*****************************************************************************
static unsigned long g_ulPlaying;
static unsigned long g_ulRecording;

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
BufferCallback(void *pvBuffer, unsigned long ulEvent)
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
//! Initializes the sound output.
//!
//! \param ulEnableReceive is set to 1 to enable the receive portion of the I2S
//! controller and 0 to leave the I2S controller not configured.
//!
//! This function prepares the sound driver to play songs or sound effects.  It
//! must be called before any other sound function.  The sound driver uses
//! uDMA with the I2S controller so the caller must ensure that the uDMA
//! peripheral is enabled and its control table configured prior to making this
//! call.  The GPIO peripheral and pins used by the I2S interface are
//! controlled by the I2S0_*_PERIPH, I2S0_*_PORT and I2S0_*_PIN definitions.
//!
//! \return None
//
//*****************************************************************************
void
SoundInit(unsigned long ulEnableReceive)
{
    //
    // Set the current active buffer to zero.
    //
    g_ulPlaying = 0;
    g_ulRecording = 0;

    //
    // Enable and reset the peripheral.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2S0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2S0);

    //
    // Select alternate functions for all of the I2S pins.
    //
    SysCtlPeripheralEnable(I2S0_SCLKTX_PERIPH);
    GPIOPinTypeI2S(I2S0_SCLKTX_PORT, I2S0_SCLKTX_PIN);

    SysCtlPeripheralEnable(I2S0_MCLKTX_PERIPH);
    GPIOPinTypeI2S(I2S0_MCLKTX_PORT, I2S0_MCLKTX_PIN);

    SysCtlPeripheralEnable(I2S0_LRCTX_PERIPH);
    GPIOPinTypeI2S(I2S0_LRCTX_PORT, I2S0_LRCTX_PIN);

    SysCtlPeripheralEnable(I2S0_SDATX_PERIPH);
    GPIOPinTypeI2S(I2S0_SDATX_PORT, I2S0_SDATX_PIN);

    //
    // Initialize the DAC.
    //
    TLV320AIC23BInit();

    //
    // Set the FIFO trigger limit
    //
    I2STxFIFOLimitSet(I2S0_BASE, 4);

    //
    // Clear out all pending interrupts.
    //
    I2SIntClear(I2S0_BASE, I2S_INT_TXERR | I2S_INT_TXREQ);

    //
    // Disable all uDMA attributes.
    //
    uDMAChannelAttributeDisable(UDMA_CHANNEL_I2S0TX, UDMA_ATTR_ALL);

    //
    // Only enable the RX channel if requested.
    //
    if(ulEnableReceive)
    {
        //
        // Enable the I2S RX Data pin.
        //
        SysCtlPeripheralEnable(I2S0_SDARX_PERIPH);
        GPIOPinTypeI2S(I2S0_SDARX_PORT, I2S0_SDARX_PIN);

        //
        // Set the FIFO trigger limit
        //
        I2SRxFIFOLimitSet(I2S0_BASE, 4);

        //
        // Enable the I2S Rx controller.
        //
        I2STxRxEnable(I2S0_BASE);

        //
        // Disable all DMA attributes.
        //
        uDMAChannelAttributeDisable(UDMA_CHANNEL_I2S0RX, UDMA_ATTR_ALL);
    }
    else
    {
        //
        // Enable the I2S Tx controller.
        //
        I2STxEnable(I2S0_BASE);
    }

    //
    // Enable the I2S interrupt on the NVIC
    //
    IntEnable(INT_I2S0);
}

//*****************************************************************************
//
//! Handles the I2S sound interrupt.
//!
//! This function services the I2S interrupt and will call the callback function
//! provided with the buffer that was given to the SoundBufferPlay() or
//! SoundBufferRead() functions to handle emptying or filling the buffers and
//! starting up DMA transfers again.  It is solely the responsibility of the
//! callback functions to continuing sending or receiving data to or from the
//! audio codec.
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
    // Handle the RX channel interrupt
    //
    if(HWREGBITW(&g_ulDMAFlags, FLAG_RX_PENDING))
    {
        //
        // If the RX DMA is done, then start another one using the same
        // RX buffer.  This keeps the RX running continuously.
        //
        if(uDMAChannelModeGet(UDMA_CHANNEL_I2S0RX | UDMA_PRI_SELECT) ==
           UDMA_MODE_STOP)
        {
            //
            // Save a temp pointer so that the current pointer can be set to
            // zero before calling the callback.
            //
            pulTemp = g_sInBuffers[0].pulData;

            //
            // If at the mid point the refill the first half of the buffer.
            //
            if(g_sInBuffers[0].pfnBufferCallback)
            {
                g_sInBuffers[0].pulData = 0;

                g_sInBuffers[0].pfnBufferCallback(pulTemp, BUFFER_EVENT_FULL);
            }
        }
        else if(uDMAChannelModeGet(UDMA_CHANNEL_I2S0RX | UDMA_ALT_SELECT) ==
                UDMA_MODE_STOP)
        {
            //
            // Save a temp pointer so that the current pointer can be set to
            // zero before calling the callback.
            //
            pulTemp = g_sInBuffers[1].pulData;

            //
            // If at the mid point the refill the first half of the buffer.
            //
            if(g_sInBuffers[1].pfnBufferCallback)
            {
                g_sInBuffers[1].pulData = 0;
                g_sInBuffers[1].pfnBufferCallback(pulTemp, BUFFER_EVENT_FULL);
            }
        }

        //
        // If there are no more scheduled buffers then there are no more
        // pending DMA transfers.
        //
        if((g_sInBuffers[0].pulData == 0) && (g_sInBuffers[1].pulData == 0))
        {
            HWREGBITW(&g_ulDMAFlags, FLAG_RX_PENDING) = 0;
        }
    }

    //
    // Handle the TX channel interrupt
    //
    if(HWREGBITW(&g_ulDMAFlags, FLAG_TX_PENDING))
    {
        //
        // If the TX DMA is done, then call the callback if present.
        //
        if(uDMAChannelModeGet(UDMA_CHANNEL_I2S0TX | UDMA_PRI_SELECT) ==
           UDMA_MODE_STOP)
        {
            //
            // Save a temp pointer so that the current pointer can be set to
            // zero before calling the callback.
            //
            pulTemp = g_sOutBuffers[0].pulData;

            //
            // If at the mid point then refill the first half of the buffer.
            //
            if((g_sOutBuffers[0].pfnBufferCallback) &&
               (g_sOutBuffers[0].pulData != 0))
            {
                g_sOutBuffers[0].pulData = 0;
                g_sOutBuffers[0].pfnBufferCallback(pulTemp, BUFFER_EVENT_FREE);
            }
        }

        //
        // If the TX DMA is done, then call the callback if present.
        //
        if(uDMAChannelModeGet(UDMA_CHANNEL_I2S0TX | UDMA_ALT_SELECT) ==
           UDMA_MODE_STOP)
        {
            //
            // Save a temporary pointer so that the current pointer can be set
            // to zero before calling the callback.
            //
            pulTemp = g_sOutBuffers[1].pulData;

            //
            // If at the mid point then refill the first half of the buffer.
            //
            if((g_sOutBuffers[1].pfnBufferCallback) &&
               (g_sOutBuffers[1].pulData != 0))
            {
                g_sOutBuffers[1].pulData = 0;
                g_sOutBuffers[1].pfnBufferCallback(pulTemp, BUFFER_EVENT_FREE);
            }
        }

        //
        // If no more buffers are pending then clear the flag.
        //
        if((g_sOutBuffers[0].pulData == 0) && (g_sOutBuffers[1].pulData == 0))
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
//! or sound effect is already being played, its playback is canceled and the
//! new song is started.
//!
//! \return None.
//
//*****************************************************************************
void
SoundPlay(const unsigned short *pusSong, unsigned long ulLength)
{
    //
    // Set the format of the audio stream.
    //
    SoundSetFormat(48000, 16, 2);

    //
    // Save the music buffer.
    //
    g_ulMusicCount = 0;
    g_ulMusicSize = ulLength * 2;
    g_pusMusic = pusSong;
    g_ulPlaying = 0;

    g_sOutBuffers[0].pulData = 0;
    g_sOutBuffers[1].pulData = 0;

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
//! and recording audio data of a particular format.
//!
//! \note This function has a work around for the I2SMCLKCFG register errata.
//! This errata limits the low end of the MCLK at some bit sizes.  The absolute
//! limit is a divide of the System PLL by 256 or an MCLK minimum of
//! 400MHz/256 or 1.5625MHz.  This is overcome by increasing the number of
//! bits shifted out per sample and thus increasing the MCLK needed for a given
//! sample rate.  This uses the fact that the I2S codec used on the development
//! board s that will toss away extra bits that go to or from the codec.
//!
//! \return None.
//
//*****************************************************************************
void
SoundSetFormat(unsigned long ulSampleRate, unsigned short usBitsPerSample,
               unsigned short usChannels)
{
    unsigned long ulFormat;
    unsigned long ulDMASetting;
    unsigned long ulI2SErrata;

    //
    // Save these values for use when configuring I2S.
    //
    g_usChannels = usChannels;
    g_usBitsPerSample = usBitsPerSample;

    I2SMasterClockSelect(I2S0_BASE, 0);

    //
    // Always use have the controller be an I2S Master.
    //
    ulFormat = I2S_CONFIG_FORMAT_I2S | I2S_CONFIG_CLK_MASTER;

    //
    // Check if the missing divisor bits need to be taken into account.
    //
    if(CLASS_IS_TEMPEST && REVISION_IS_B1)
    {
        ulI2SErrata = 1;
    }
    else
    {
        ulI2SErrata = 0;
    }

    //
    // Mono or Stereo formats.
    //
    if(g_usChannels == 1)
    {
        //
        // 8 bit formats.
        //
        if(g_usBitsPerSample == 8)
        {
            //
            // On Tempest class devices rev B parts the divisor is
            // limited for lower samples rates (see errata).
            //
            if((ulI2SErrata != 0) && (ulSampleRate < 24400))
            {
                ulFormat |= I2S_CONFIG_WIRE_SIZE_32 | I2S_CONFIG_MODE_MONO |
                            I2S_CONFIG_SAMPLE_SIZE_8;
                usBitsPerSample = 32;
            }
            else
            {
                ulFormat |= I2S_CONFIG_WIRE_SIZE_8 | I2S_CONFIG_MODE_MONO |
                            I2S_CONFIG_SAMPLE_SIZE_8;
            }
        }
        else if(g_usBitsPerSample == 16)
        {
            //
            // On Tempest class devices rev B parts the divisor is
            // limited for lower samples rates (see errata).
            //
            if((ulI2SErrata != 0) && (ulSampleRate < 12200))
            {
                ulFormat |= I2S_CONFIG_WIRE_SIZE_32 | I2S_CONFIG_MODE_MONO |
                            I2S_CONFIG_SAMPLE_SIZE_16;

                usBitsPerSample = 32;
            }
            else
            {
                ulFormat |= I2S_CONFIG_WIRE_SIZE_16 | I2S_CONFIG_MODE_MONO |
                            I2S_CONFIG_SAMPLE_SIZE_16;
            }
        }
        else if(g_usBitsPerSample == 24)
        {
            ulFormat |= (I2S_CONFIG_WIRE_SIZE_24 | I2S_CONFIG_MODE_MONO |
                         I2S_CONFIG_SAMPLE_SIZE_24);
        }
        else
        {
            ulFormat |= (I2S_CONFIG_WIRE_SIZE_32 | I2S_CONFIG_MODE_MONO |
                         I2S_CONFIG_SAMPLE_SIZE_32);
        }
    }
    else
    {
        if(g_usBitsPerSample == 8)
        {
            //
            // On Tempest class devices rev B parts the divisor is
            // limited for lower samples rates (see errata).
            //
            if((ulI2SErrata != 0) && (ulSampleRate < 12200))
            {
                ulFormat |= I2S_CONFIG_WIRE_SIZE_32 |
                            I2S_CONFIG_MODE_COMPACT_8 |
                            I2S_CONFIG_SAMPLE_SIZE_8;

                usBitsPerSample = 32;
            }
            else
            {
                ulFormat |= I2S_CONFIG_WIRE_SIZE_8 |
                            I2S_CONFIG_MODE_COMPACT_8 |
                            I2S_CONFIG_SAMPLE_SIZE_8;
            }
        }
        else if(g_usBitsPerSample == 16)
        {
            if((ulI2SErrata != 0) && (ulSampleRate < 12200))
            {
                ulFormat |= I2S_CONFIG_WIRE_SIZE_32 |
                            I2S_CONFIG_MODE_COMPACT_16 |
                            I2S_CONFIG_SAMPLE_SIZE_16;
                usBitsPerSample = 32;
            }
            else
            {
                ulFormat |= I2S_CONFIG_WIRE_SIZE_16 |
                            I2S_CONFIG_MODE_COMPACT_16 |
                            I2S_CONFIG_SAMPLE_SIZE_16;
            }
        }
        else if(g_usBitsPerSample == 24)
        {
            ulFormat |= (I2S_CONFIG_WIRE_SIZE_24 | I2S_CONFIG_MODE_DUAL |
                         I2S_CONFIG_SAMPLE_SIZE_24);
        }
        else
        {
            ulFormat |= (I2S_CONFIG_WIRE_SIZE_32 | I2S_CONFIG_MODE_DUAL |
                         I2S_CONFIG_SAMPLE_SIZE_32);
        }
    }

    //
    // Configure the I2S TX format.
    //
    I2STxConfigSet(I2S0_BASE, ulFormat);

    //
    // This is needed on Rev B parts due to errata.
    //
    if(ulI2SErrata)
    {
        ulFormat = (ulFormat & ~I2S_CONFIG_FORMAT_MASK) | I2S_CONFIG_FORMAT_LEFT_JUST;
    }

    //
    // Configure the I2S RX format.
    //
    I2SRxConfigSet(I2S0_BASE, ulFormat);

    //
    // Internally both are masters but the pins may not be driven out.
    //
    I2SMasterClockSelect(I2S0_BASE, I2S_TX_MCLK_INT | I2S_RX_MCLK_INT);

    //
    // Set the MCLK rate and save it for conversion back to sample rate.
    // The multiply by 8 is due to a 4X oversample rate plus a factor of two
    // since the data is always stereo on the I2S interface.
    //
    g_ulSampleRate = SysCtlI2SMClkSet(0, ulSampleRate * usBitsPerSample * 8);

    //
    // Convert the MCLK rate to sample rate.
    //
    g_ulSampleRate = g_ulSampleRate / (usBitsPerSample * 8);

    //
    // Configure the I2S TX DMA channel to use high priority burst transfer.
    //
    uDMAChannelAttributeEnable(UDMA_CHANNEL_I2S0TX,
                               UDMA_ATTR_USEBURST | UDMA_ATTR_HIGH_PRIORITY);

    //
    // Set the DMA channel configuration.
    //
    if(g_usChannels == 1)
    {
        //
        // Handle Mono formats.
        //
        if(g_usBitsPerSample == 8)
        {
            //
            // The transfer size is 8 bits from the TX buffer to the TX FIFO.
            //
            ulDMASetting = (UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                            UDMA_DST_INC_NONE | UDMA_ARB_2);
        }
        else
        {
            //
            // The transfer size is 16 bits from the TX buffer to the TX FIFO.
            //
            ulDMASetting = (UDMA_SIZE_16 | UDMA_SRC_INC_16 |
                            UDMA_DST_INC_NONE | UDMA_ARB_2);
        }
    }
    else
    {
        //
        // Handle Stereo formats.
        //
        if(g_usBitsPerSample == 8)
        {
            //
            // The transfer size is 16 bits(stereo 8 bits) from the TX buffer
            // to the TX FIFO.
            //
            ulDMASetting = (UDMA_SIZE_16 | UDMA_SRC_INC_16 |
                            UDMA_DST_INC_NONE | UDMA_ARB_2);
        }
        else
        {
            //
            // The transfer size is 32 bits(stereo 16 bits) from the TX buffer
            // to the TX FIFO.
            //
            ulDMASetting = (UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                            UDMA_DST_INC_NONE | UDMA_ARB_2);
        }
    }

    //
    // Configure the DMA settings for this channel.
    //
    uDMAChannelControlSet(UDMA_CHANNEL_I2S0TX | UDMA_PRI_SELECT, ulDMASetting);
    uDMAChannelControlSet(UDMA_CHANNEL_I2S0TX | UDMA_ALT_SELECT, ulDMASetting);

    //
    // Set the DMA channel configuration.
    //
    if(g_usChannels == 1)
    {
        //
        // Handle stereo recording.
        //
        if(g_usBitsPerSample == 8)
        {
            //
            // The transfer size is 8 bits from the RX FIFO to
            // RX buffer.
            //
            ulDMASetting = (UDMA_SIZE_8 | UDMA_DST_INC_8 |
                            UDMA_SRC_INC_NONE | UDMA_ARB_2);

            //
            // Only read the most significant byte of the I2S FIFO.
            //
            g_pvFIFORecord = (void *)(I2S0_BASE + I2S_O_RXFIFO + 3);
        }
        else
        {
            //
            // The transfer size is 16 bits from the RX FIFO to
            // RX buffer.
            //
            ulDMASetting = (UDMA_SIZE_16 | UDMA_DST_INC_16 |
                            UDMA_SRC_INC_NONE | UDMA_ARB_2);

            //
            // Only read the most significant 16 bits of the I2S FIFO.
            //
            g_pvFIFORecord = (void *)(I2S0_BASE + I2S_O_RXFIFO + 2);
        }
    }
    else
    {
        //
        // Handle stereo recording.
        //
        if(g_usBitsPerSample == 8)
        {
            //
            // The transfer size is 16 bits(stereo 8 bits) from the RX FIFO to
            // RX buffer.
            //
            ulDMASetting = (UDMA_SIZE_16 | UDMA_DST_INC_16 |
                            UDMA_SRC_INC_NONE | UDMA_ARB_2);

            //
            // Only read the most significant 16 bits of the I2S FIFO.
            //
            g_pvFIFORecord = (void *)(I2S0_BASE + I2S_O_RXFIFO);
        }
        else
        {
            //
            // The transfer size is 32 bits(stereo 16 bits) from the RX FIFO to
            // RX buffer.
            //
            ulDMASetting = (UDMA_SIZE_32 | UDMA_DST_INC_32 |
                            UDMA_SRC_INC_NONE | UDMA_ARB_2);

            //
            // Only read the most significant byte of the I2S FIFO.
            //
            g_pvFIFORecord = (void *)(I2S0_BASE + I2S_O_RXFIFO);
        }
    }

    //
    // Configure the DMA settings for this channel.
    //
    uDMAChannelControlSet(UDMA_CHANNEL_I2S0RX | UDMA_PRI_SELECT, ulDMASetting);
    uDMAChannelControlSet(UDMA_CHANNEL_I2S0RX | UDMA_ALT_SELECT, ulDMASetting);
}

//*****************************************************************************
//
//! Returns the current sample rate.
//!
//! This function returns the sample rate that was set by a call to
//! SoundSetFormat().  This is needed to retrieve the exact sample rate that is
//! in use in case the requested rate could not be matched exactly.
//!
//! \return The current sample rate in samples/second.
//
//*****************************************************************************
unsigned long
SoundSampleRateGet(void)
{
    return(g_ulSampleRate);
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
//! playback of another buffer is currently ongoing, its playback is canceled
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

    //
    // Must disable I2S interrupts during this time to prevent state problems.
    //
    IntDisable(INT_I2S0);

    //
    // Save the buffer information.
    //
    g_sOutBuffers[g_ulPlaying].pulData = (unsigned long *)pvData;
    g_sOutBuffers[g_ulPlaying].ulSize = ulLength;
    g_sOutBuffers[g_ulPlaying].pfnBufferCallback = pfnCallback;

    //
    // Handle which half of the ping-pong DMA is in use.
    //
    if(g_ulPlaying)
    {
        ulChannel = UDMA_CHANNEL_I2S0TX | UDMA_ALT_SELECT;
    }
    else
    {
        ulChannel = UDMA_CHANNEL_I2S0TX | UDMA_PRI_SELECT;
    }

    //
    // Set the DMA channel configuration.
    //
    if(g_usChannels == 1)
    {
        //
        // Handle Mono formats.
        //
        if(g_usBitsPerSample == 16)
        {
            //
            // The transfer size is 16 bits from the TX buffer to the TX FIFO.
            // Modify the DMA transfer size at it is units not bytes.
            //
            g_sOutBuffers[g_ulPlaying].ulSize >>= 1;
        }
    }
    else
    {
        //
        // Handle Stereo formats.
        //
        if(g_usBitsPerSample == 8)
        {
            //
            // The transfer size is 16 bits(stereo 8 bits) from the TX buffer
            // to the TX FIFO.  Modify the DMA transfer size at it is units
            // not bytes.
            //
            g_sOutBuffers[g_ulPlaying].ulSize >>= 1;
        }
        else
        {
            //
            // The transfer size is 32 bits(stereo 16 bits) from the TX buffer
            // to the TX FIFO. Modify the DMA transfer size at it is units not
            // bytes.
            //
            g_sOutBuffers[g_ulPlaying].ulSize >>= 2;
        }
    }

    //
    // Set the addresses and the DMA mode to ping-pong.
    //
    uDMAChannelTransferSet(ulChannel,
                           UDMA_MODE_PINGPONG,
                           (unsigned long *)g_sOutBuffers[g_ulPlaying].pulData,
                           (void *)(I2S0_BASE + I2S_O_TXFIFO),
                           g_sOutBuffers[g_ulPlaying].ulSize);

    //
    // Enable the TX channel.  At this point the uDMA controller will
    // start servicing the request from the I2S, and the transmit side
    // should start running.
    //
    uDMAChannelEnable(UDMA_CHANNEL_I2S0TX);

    //
    // Indicate that there is still a pending transfer.
    //
    HWREGBITW(&g_ulDMAFlags, FLAG_TX_PENDING) = 1;

    //
    // Toggle which ping-pong DMA setting is in use.
    //
    g_ulPlaying ^= 1;

    //
    // Enable the I2S controller to start transmitting.
    //
    I2STxEnable(I2S0_BASE);

    //
    // Re-enable I2S interrupts.
    //
    IntEnable(INT_I2S0);

    return(0);
}

//*****************************************************************************
//
//! Starts recording from the audio input.
//!
//! \param pvData is a pointer to store the data as it is received.
//! \param ulSize is the size of the buffer pointed to by pvData in bytes.
//! \param pfnCallback is a function to call when this buffer has been filled.
//!
//! This function initiates a request for the I2S controller to receive data
//! from an I2S codec.
//!
//! \return 0 if the buffer was accepted, returns non-zero if there was no
//! space available for this buffer.
//
//*****************************************************************************
unsigned long
SoundBufferRead(void *pvData, unsigned long ulSize,
                tBufferCallback pfnCallback)
{
    unsigned long ulChannel;

    //
    // Must disable I2S interrupts during this time to prevent state problems.
    //
    IntDisable(INT_I2S0);

    //
    // Save the buffer information.
    //
    g_sInBuffers[g_ulRecording].pulData = (unsigned long *)pvData;
    g_sInBuffers[g_ulRecording].ulSize = ulSize;
    g_sInBuffers[g_ulRecording].pfnBufferCallback = pfnCallback;

    //
    // Configure the I2S TX DMA channel to use high priority burst mode.
    //
    uDMAChannelAttributeEnable(UDMA_CHANNEL_I2S0RX,
                               (UDMA_ATTR_USEBURST |
                                UDMA_ATTR_HIGH_PRIORITY));

    //
    // Handle which half of the ping-pong DMA is in use.
    //
    if(g_ulRecording)
    {
        ulChannel = UDMA_CHANNEL_I2S0RX | UDMA_ALT_SELECT;
    }
    else
    {
        ulChannel = UDMA_CHANNEL_I2S0RX | UDMA_PRI_SELECT;
    }

    if(g_usChannels == 1)
    {
        //
        // Handle mono recording.
        //
        if(g_usBitsPerSample == 16)
        {
            //
            // Modify the DMA transfer size at it is units not bytes.
            //
            g_sInBuffers[g_ulRecording].ulSize >>= 1;
        }
    }
    else
    {
        //
        // Handle stereo recording.
        //
        if(g_usBitsPerSample == 8)
        {
            //
            // Modify the DMA transfer size at it is units not bytes.
            //
            g_sInBuffers[g_ulRecording].ulSize >>= 1;
        }
        else
        {
            //
            // Modify the DMA transfer size at it is units not bytes.
            //
            g_sInBuffers[g_ulRecording].ulSize >>= 2;
        }
    }

    //
    // Set the addresses and the DMA mode to ping-pong.
    //
    uDMAChannelTransferSet(ulChannel,
                           UDMA_MODE_PINGPONG,
                           g_pvFIFORecord,
                           (unsigned long *)g_sInBuffers[g_ulRecording].pulData,
                           g_sInBuffers[g_ulRecording].ulSize);

    //
    // Enable the RX DMA channel.  At this point the uDMA controller will
    // start servicing the request from the I2S, and the receive side should
    // start receiving data if any is available.
    //
    uDMAChannelEnable(UDMA_CHANNEL_I2S0RX);

    //
    // Indicate that there is still a pending transfer.
    //
    HWREGBITW(&g_ulDMAFlags, FLAG_RX_PENDING) = 1;

    //
    // Toggle which ping-pong DMA setting is in use.
    //
    g_ulRecording ^= 1;

    //
    // Enable the I2S controller to start receiving data.
    //
    I2SRxEnable(I2S0_BASE);

    //
    // Re-enable I2S interrupts.
    //
    IntEnable(INT_I2S0);

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
    g_ucVolume = ulPercent;
    TLV320AIC23BHeadPhoneVolumeSet(ulPercent);
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
