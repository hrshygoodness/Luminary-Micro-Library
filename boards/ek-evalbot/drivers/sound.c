//*****************************************************************************
//
// sound.c - Functions supporting sound playback on EVALBOT.
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

//*****************************************************************************
//
//! \addtogroup sound_api
//! @{
//
//*****************************************************************************

#include "inc/hw_i2s.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/i2s.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "dac.h"
#include "sound.h"

//*****************************************************************************
//
// This table must be defined in any application wishing to use the sound
// driver since I2S makes use of DMA.
//
//*****************************************************************************
extern tDMAControlTable sDMAControlTable[];

//*****************************************************************************
//
// Definitions of I2S pins used for the sound driver.
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

#define I2S0_MCLKTX_PERIPH      (SYSCTL_PERIPH_GPIOF)
#define I2S0_MCLKTX_PORT        (GPIO_PORTF_BASE)
#define I2S0_MCLKTX_PIN         (GPIO_PIN_1)

//*****************************************************************************
//
// The number of buffers to use for buffering audio playback.
//
//*****************************************************************************
#define NUM_BUFFERS             2

//*****************************************************************************
//
// Flag values used to track the uDMA state.
//
//*****************************************************************************
#define FLAG_RX_PENDING         0
#define FLAG_TX_PENDING         1
static volatile unsigned long g_ulDMAFlags;

//*****************************************************************************
//
// Buffer management structures and definition.
//
//*****************************************************************************
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
    void (* pfnBufferCallback)(void *pvBuffer, unsigned long ulEvent);
}
g_sOutBuffers[NUM_BUFFERS];

//*****************************************************************************
//
// The current volume of the music/sound effects.
//
//*****************************************************************************
static unsigned char g_ucVolume = 100;

//*****************************************************************************
//
// The buffer index that is currently playing.
//
//*****************************************************************************
static unsigned long g_ulPlaying;

//*****************************************************************************
//
// Information on the sound currently being played.
//
//*****************************************************************************
static unsigned long g_ulSampleRate;
static unsigned short g_usChannels;
static unsigned long g_usBitsPerSample;

//*****************************************************************************
//
//! Initialize the sound driver.
//!
//! This function initializes the audio hardware components of the EVALBOT,
//! in preparation for playing sounds using the sound driver.
//!
//! \return None.
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
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Set up the pin mux.
    //
    GPIOPinConfigure(GPIO_PB6_I2S0TXSCK);
    GPIOPinConfigure(GPIO_PE4_I2S0TXWS);
    GPIOPinConfigure(GPIO_PE5_I2S0TXSD);
    GPIOPinConfigure(GPIO_PF1_I2S0TXMCLK);

    //
    // Select alternate functions for all of the I2S pins.
    //
    ROM_SysCtlPeripheralEnable(I2S0_SCLKTX_PERIPH);
    GPIOPinTypeI2S(I2S0_SCLKTX_PORT, I2S0_SCLKTX_PIN);

    ROM_SysCtlPeripheralEnable(I2S0_MCLKTX_PERIPH);
    GPIOPinTypeI2S(I2S0_MCLKTX_PORT, I2S0_MCLKTX_PIN);

    ROM_SysCtlPeripheralEnable(I2S0_LRCTX_PERIPH);
    GPIOPinTypeI2S(I2S0_LRCTX_PORT, I2S0_LRCTX_PIN);

    ROM_SysCtlPeripheralEnable(I2S0_SDATX_PERIPH);
    GPIOPinTypeI2S(I2S0_SDATX_PORT, I2S0_SDATX_PIN);

    //
    // Set up the DMA.
    //
    ROM_uDMAControlBaseSet(&sDMAControlTable[0]);
    ROM_uDMAEnable();

    //
    // Initialize the DAC.
    //
    DACInit();

    //
    // Set the FIFO trigger limit
    //
    I2STxFIFOLimitSet(I2S0_BASE, 4);

    //
    // Clear out all pending interrupts.
    //
    I2SIntClear(I2S0_BASE, I2S_INT_TXERR | I2S_INT_TXREQ );

    //
    // Disable all uDMA attributes.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_I2S0TX, UDMA_ATTR_ALL);

    //
    // Enable the I2S Tx controller.
    //
    I2STxEnable(I2S0_BASE);
}

//*****************************************************************************
//
//! Interrupt handler for the I2S sound driver.
//!
//! This interrupt function is called by the processor due to an interrupt from
//! the I2S peripheral, or due to the completion of an I2S/uDMA transfer.  uDMA
//! is used in ping-pong mode to keep sound buffer data flowing to the I2S
//! audio output.  As each buffer transfer is complete, the client callback
//! function that was specified in the call to SoundBufferPlay() will be
//! called.  The client can then take action to start the next buffer playing.
//!
//! Applications using the sound driver must hook this function to the interrupt
//! vectors for the I2S0 peripheral.
//!
//! \return None.
//!
//! \note This function is called by the interrupt system and should not be
//! called directly from application code.
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
        if(ROM_uDMAChannelModeGet(UDMA_CHANNEL_I2S0TX | UDMA_ALT_SELECT) ==
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
//! Configures the I2S peripheral to play audio in a given format.
//!
//! \param ulSampleRate is the sample rate of the audio to be played in
//!  samples per second.
//! \param usBitsPerSample is the number of bits in each audio sample.
//! \param usChannels is the number of audio channels, 1 for mono, 2 for stereo.
//!
//! This function configures the I2S peripheral in preparation for playing
//! or recording audio data in a particular format.
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

    //
    // Configure the I2S master clock for internal.
    //
    I2SMasterClockSelect(I2S0_BASE, I2S_TX_MCLK_INT);

    //
    // Configure the I2S to be a master.
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
            // limited for lower samples rates (see erratum).
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

        //
        // 16-bit format
        //
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

        //
        // 24-bit format
        //
        else if(g_usBitsPerSample == 24)
        {
            ulFormat |= I2S_CONFIG_WIRE_SIZE_24 | I2S_CONFIG_MODE_MONO |
                        I2S_CONFIG_SAMPLE_SIZE_24;
        }
        else
        {
            ulFormat |= I2S_CONFIG_WIRE_SIZE_32 | I2S_CONFIG_MODE_MONO |
                        I2S_CONFIG_SAMPLE_SIZE_32;
        }
    }

    //
    // Stereo formats
    //
    else
    {
        //
        // 8-bit format
        //
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

        //
        // 16-bit format
        //
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

        //
        // 24-bit format
        //
        else if(g_usBitsPerSample == 24)
        {
            ulFormat |= I2S_CONFIG_WIRE_SIZE_24 | I2S_CONFIG_MODE_DUAL |
                        I2S_CONFIG_SAMPLE_SIZE_24;
        }
        else
        {
            ulFormat |= I2S_CONFIG_WIRE_SIZE_32 | I2S_CONFIG_MODE_DUAL |
                        I2S_CONFIG_SAMPLE_SIZE_32;
        }
    }

    //
    // Configure the I2S TX format.
    //
    I2STxConfigSet(I2S0_BASE, ulFormat);

    //
    // Set the MCLK rate and save it for conversion back to sample rate.
    // The multiply by 8 is due to a 4X oversample rate plus a factor of two
    // since the data is always stereo on the I2S interface.
    //
    g_ulSampleRate = SysCtlI2SMClkSet(0, (ulSampleRate *
                                      usBitsPerSample * 8));

    //
    // Convert the MCLK rate to sample rate.
    //
    g_ulSampleRate = g_ulSampleRate / (usBitsPerSample * 8);

    //
    // Configure the I2S TX DMA channel to use high priority burst transfer.
    //
    ROM_uDMAChannelAttributeEnable(UDMA_CHANNEL_I2S0TX,
                                   (UDMA_ATTR_USEBURST |
                                    UDMA_ATTR_HIGH_PRIORITY));

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
            ulDMASetting = UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                           UDMA_DST_INC_NONE | UDMA_ARB_2;
        }
        else
        {
            //
            // The transfer size is 16 bits from the TX buffer to the TX FIFO.
            //
            ulDMASetting = UDMA_SIZE_16 | UDMA_SRC_INC_16 |
                           UDMA_DST_INC_NONE | UDMA_ARB_2;
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
            ulDMASetting = UDMA_SIZE_16 | UDMA_SRC_INC_16 |
                           UDMA_DST_INC_NONE | UDMA_ARB_2;
        }
        else
        {
            //
            // The transfer size is 32 bits(stereo 16 bits) from the TX buffer
            // to the TX FIFO.
            //
            ulDMASetting = UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                           UDMA_DST_INC_NONE | UDMA_ARB_2;
        }
    }

    //
    // Configure the DMA settings for this channel.
    //
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_I2S0TX | UDMA_PRI_SELECT,
                              ulDMASetting);
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_I2S0TX | UDMA_ALT_SELECT,
                              ulDMASetting);
}


//*****************************************************************************
//
//! Returns the current audio sample rate setting.
//!
//! This function returns the sample rate that is currently set.  The value
//! returned reflects the actual rate set which may be slightly different from
//! the value passed on the last call to SoundSetFormat() if the requested
//! rate could not be matched exactly.
//!
//! \return Returns the sample rate in samples per second.
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
//! \param pfnCallback is a function to call when this buffer has been played.
//!
//! This function starts the playback of a block of PCM audio samples.  If
//! playback of another buffer is currently ongoing, its playback is canceled
//! and the buffer starts playing immediately.
//!
//! \return None.
//
//*****************************************************************************
void
SoundBufferPlay(const void *pvData, unsigned long ulLength,
                void (*pfnCallback)(void *pvBuffer, unsigned long ulEvent))
{
    unsigned long ulChannel;

    //
    // Must disable I2S interrupts during this time to prevent state problems.
    //
    ROM_IntDisable(INT_I2S0);

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
    // Set up the uDMA transfer addresses, using ping-pong mode.
    //
    ROM_uDMAChannelTransferSet(ulChannel,
                           UDMA_MODE_PINGPONG,
                           (unsigned long *)g_sOutBuffers[g_ulPlaying].pulData,
                           (void *)(I2S0_BASE + I2S_O_TXFIFO),
                           g_sOutBuffers[g_ulPlaying].ulSize);

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
    ROM_IntEnable(INT_I2S0);
}

//*****************************************************************************
//
//! Sets the audio volume to a given level.
//!
//! \param ulPercent is the volume level to set.  Valid values are between
//! 0 and 100 inclusive.
//!
//! This function sets the audio output volume.  The supplied level is
//! expressed as a percentage between 0% (silence) and 100% (full volume)
//! inclusive.
//!
//! \return None.
//
//*****************************************************************************
void
SoundVolumeSet(unsigned long ulPercent)
{
    //
    // Make sure we were passed a valid volume setting.
    //
    ASSERT(ulPercent <= 100);

    //
    // Set the volume to the desired level.
    //
    DACVolumeSet(ulPercent);
}

//*****************************************************************************
//
//! Returns the current sound volume.
//!
//! This function returns the current volume, specified as a percentage between
//! 0% (silence) and 100% (full volume), inclusive.
//!
//! \return Returns the current volume setting expressed as a percentage.
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
//! Adjusts the audio volume downwards by a given amount.
//!
//! \param ulPercent is the amount to decrease the volume, specified as a
//! percentage relative to the full volume.
//!
//! This function adjusts the audio output downwards by the specified
//! percentage.  The adjusted volume will not go below 0% (silence).
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
//! Adjusts the audio volume upwards by a given amount.
//!
//! \param ulPercent is the amount to increase the volume, specified as a
//! percentage relative to the full volume.
//!
//! This function adjusts the audio output upwards by the specified percentage.
//! The adjusted volume will not go above 100% (full volume).
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
//! Enables the class D amplifier in the DAC.
//!
//! This function enables the class D amplifier in the DAC.  It is merely a
//! wrapper around the DAC driver's DACClassDEn() function.
//!
//! \return None.
//
//*****************************************************************************
void
SoundClassDEn(void)
{
    DACClassDEn();
}

//*****************************************************************************
//
//! Disables the class D amplifier in the DAC.
//!
//! This function disables the class D amplifier in the DAC.  It is merely a
//! wrapper around the DAC driver's DACClassDDis() function.
//!
//! \return None.
//
//*****************************************************************************
void
SoundClassDDis(void)
{
    DACClassDDis();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
