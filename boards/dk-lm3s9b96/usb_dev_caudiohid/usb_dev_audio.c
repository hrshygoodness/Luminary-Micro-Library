//****************************************************************************
//
// usb_dev_audio.c - Routines to handle the audio device.
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
//****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_udma.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdaudio.h"
#include "usblib/device/usbdcomp.h"
#include "utils/ustdlib.h"
#include "drivers/sound.h"
#include "usb_structs.h"
#include "usb_dev_caudiohid.h"

//****************************************************************************
//
// Buffer management and flags.
//
//****************************************************************************
#define AUDIO_PACKET_SIZE       ((48000*4)/1000)
#define AUDIO_BUFFER_SIZE       (AUDIO_PACKET_SIZE*20)

#define SBUFFER_FLAGS_PLAYING   0x00000001
#define SBUFFER_FLAGS_FILLING   0x00000002
struct
{
    //
    // The main buffer used by both the USB audio class and the sound driver.
    //
    volatile unsigned char pucBuffer[AUDIO_BUFFER_SIZE];

    //
    // The current location of the play pointer.
    //
    volatile unsigned char *pucPlay;

    //
    // The current location of the USB fill pointer.
    //
    volatile unsigned char *pucFill;

    //
    // The amount of sample rate adjustment currently in affect.  This value
    // can only be +1, 0, or -1.
    //
    volatile int iAdjust;

    //
    // Saves the play state for e
    volatile unsigned long ulFlags;
} g_sBuffer;

//****************************************************************************
//
// This macro is used to convert the 16 bit signed 8.8 fixed point number to
// a number that ranges from 0 - 100.
//
//****************************************************************************
#define CONVERT_TO_PERCENT(dbVolume)                                         \
    ((dbVolume - (short)VOLUME_MAX + (short)VOLUME_MIN) * 100) /             \
    ((short)VOLUME_MAX - (short)VOLUME_MIN) + 100;

//****************************************************************************
//
// The current volume setting.
//
//****************************************************************************
short g_sVolume;

//*****************************************************************************
//
// The instance data for the composite device.
//
//*****************************************************************************
extern tUSBDCompositeDevice g_sCompDevice;

//*****************************************************************************
//
// Graphics context used to show text on the color STN display.
//
//*****************************************************************************
extern tContext g_sContext;

//****************************************************************************
//
// This function is used to modify the MCLK used by the I2S interface by a
// given amount.
//
// \param iMClkAdjust is the amount to shift the MCLK divisor expressed as a
// signed 8.4 fixed point number.
//
// This function can be used to make adjustments to the current playback rate
// for the I2S interface without stopping playback.  The adjustment is
// specified in the \e iMClkAdjust parameter and is an 8.4 signed fixed point
// number.  Some care should be used as only small changes should be made to
// prevent noise that may occur due to a rapid change in rate.  This is not
// meant to be a sample rate conversion, it is used to correct for small
// errors in sample rate.
//
// \return None.
//
//****************************************************************************
void
SysCtlI2SMClkAdjust(int iMClkAdjust)
{
    unsigned long ulCurrentSetting;
    int iCurrentDivisor;

    //
    // Get the current setting for mclk divisors.
    //
    ulCurrentSetting = HWREG(SYSCTL_I2SMCLKCFG);

    //
    // Adjust the clock setting.
    //
    iCurrentDivisor = (SYSCTL_I2SMCLKCFG_TXI_M | SYSCTL_I2SMCLKCFG_TXF_M) &
                       ulCurrentSetting;
    iCurrentDivisor += iMClkAdjust;

    //
    // Clear out the previous settings for the transmit divisor.
    //
    ulCurrentSetting &= ~(SYSCTL_I2SMCLKCFG_TXI_M | SYSCTL_I2SMCLKCFG_TXF_M |
                          SYSCTL_I2SMCLKCFG_RXI_M | SYSCTL_I2SMCLKCFG_RXF_M);

    //
    // Add in the new transmit divisor and save it to the register.
    //
    HWREG(SYSCTL_I2SMCLKCFG) = ulCurrentSetting |
                               (unsigned long)iCurrentDivisor |
                               ((unsigned long)iCurrentDivisor <<
                                SYSCTL_I2SMCLKCFG_RXF_S);
}

//****************************************************************************
//
// This function is called back for events in the USB Audio Class.
//
//****************************************************************************
unsigned long
AudioMessageHandler(void *pvCBData, unsigned long ulEvent,
                    unsigned long ulMsgParam, void *pvMsgData)
{
    switch(ulEvent)
    {
        //
        // Either the idle or active state indicates that the USB device
        // has been connected.
        //
        case USBD_AUDIO_EVENT_IDLE:
        case USBD_AUDIO_EVENT_ACTIVE:
        {
            //
            // Now connected.
            //
            HWREGBITW(&g_ulFlags, FLAG_CONNECTED) = 1;
            break;
        }

        //
        // Mute update.
        //
        case USBD_AUDIO_EVENT_MUTE:
        {
            //
            // Check if this was a mute or unmute.
            //
            if(ulMsgParam == 1)
            {
                //
                // Flag the update as a mute.
                //
                HWREGBITW(&g_ulFlags, FLAG_MUTED) = 1;
                HWREGBITW(&g_ulFlags, FLAG_MUTE_UPDATE) = 1;
            }
            else
            {
                //
                // Flag the update as an unmute.
                //
                HWREGBITW(&g_ulFlags, FLAG_MUTED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_MUTE_UPDATE) = 1;
            }
            break;
        }

        //
        // Volume update.
        //
        case USBD_AUDIO_EVENT_VOLUME:
        {
            HWREGBITW(&g_ulFlags, FLAG_VOLUME_UPDATE) = 1;

            //
            // Check for the special case of maximum attenuation.
            //
            if(ulMsgParam == 0x8000)
            {
                //
                // Set the volume to 0.
                //
                g_sVolume = 0;
            }
            else
            {
                //
                // Bias the setting to all positive values.
                //
                g_sVolume = (short)ulMsgParam - (short)VOLUME_MIN;
            }
            break;
        }

        //
        // Handle the disconnect state.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // No longer connected.
            //
            HWREGBITW(&g_ulFlags, FLAG_CONNECTED) = 0;
        }
        default:
        {
            break;
        }
    }
    return(0);
}

//****************************************************************************
//
// Handler for buffers being released by the sound driver.
//
//****************************************************************************
void
SoundBufferCallback(void *pvBuffer, unsigned long ulEvent)
{
    if(ulEvent & BUFFER_EVENT_FREE)
    {
        //
        // Increment the play buffer.
        //
        g_sBuffer.pucPlay += AUDIO_PACKET_SIZE;

        //
        // Wrap the play buffer back to the beginning.
        //
        if((g_sBuffer.pucPlay - g_sBuffer.pucBuffer) == AUDIO_BUFFER_SIZE)
        {
            g_sBuffer.pucPlay = g_sBuffer.pucBuffer;
        }

        //
        // If the buffers ever match then its time to stop playing and reset
        // the state.
        //
        if(g_sBuffer.pucPlay == g_sBuffer.pucFill)
        {
            g_sBuffer.ulFlags &= ~SBUFFER_FLAGS_PLAYING;
            g_sBuffer.pucPlay = g_sBuffer.pucBuffer;
            g_sBuffer.pucFill = g_sBuffer.pucBuffer;
            g_sBuffer.iAdjust = 0;

        }
        else
        {
            //
            // Start playing the next buffer.
            //
            SoundBufferPlay((void *)g_sBuffer.pucPlay, AUDIO_PACKET_SIZE,
                            SoundBufferCallback);
        }
    }
}

//****************************************************************************
//
// Handler for buffers coming from the USB audio device class.
//
//****************************************************************************
void
USBBufferCallback(void *pvBuffer, unsigned long ulParam, unsigned long ulEvent)
{
    //
    // Increment the fill pointer.
    //
    g_sBuffer.pucFill += AUDIO_PACKET_SIZE;

    //
    // At the mid point of the fill buffer check for sample rate drift.
    //
    if((g_sBuffer.pucFill - g_sBuffer.pucBuffer) == (AUDIO_BUFFER_SIZE >> 1))
    {
        //
        // See if we are running slow or fast.
        //
        if(g_sBuffer.pucPlay > g_sBuffer.pucFill)
        {
            //
            // See if the play buffer has fallen behind enough to trigger
            // adjusting the sample rate.
            //
            if((g_sBuffer.pucBuffer + AUDIO_BUFFER_SIZE -
                (AUDIO_PACKET_SIZE * 2)) < g_sBuffer.pucPlay)
            {
                //
                // Only allow an adjustment of at most one fractional bit.
                //
                if(g_sBuffer.iAdjust >= 0)
                {
                    //
                    // Adjust the sample rate down slightly
                    //
                    SysCtlI2SMClkAdjust(-1);
                    g_sBuffer.iAdjust--;
                }
            }
        }
        else
        {
            //
            // See if the play buffer has started leading by enough to trigger
            // adjusting the sample rate.
            //
            if((g_sBuffer.pucBuffer + (AUDIO_PACKET_SIZE * 2)) <
               g_sBuffer.pucPlay)
            {
                //
                // Only allow an adjustment of at most one fractional bit.
                //
                if(g_sBuffer.iAdjust <= 0)
                {
                    //
                    // Adjust the sample rate up slightly
                    //
                    SysCtlI2SMClkAdjust(1);
                    g_sBuffer.iAdjust++;
                }
            }
        }

        //
        // See if the device is currently playing.
        //
        if((g_sBuffer.ulFlags & SBUFFER_FLAGS_PLAYING) == 0)
        {
            //
            // Start playing at the current play pointer.
            //
            g_sBuffer.ulFlags |= SBUFFER_FLAGS_PLAYING;

            SoundBufferPlay((unsigned char *)g_sBuffer.pucPlay,
                            AUDIO_PACKET_SIZE, SoundBufferCallback);
        }
    }

    //
    // Wrap the buffer back to the beginning.
    //
    if((g_sBuffer.pucFill - g_sBuffer.pucBuffer) == AUDIO_BUFFER_SIZE)
    {
        g_sBuffer.pucFill = g_sBuffer.pucBuffer;
    }

    //
    // Allow the USB audio class to fill the next buffer.
    //
    USBAudioBufferOut(g_sCompDevice.psDevices[0].pvInstance,
                      (unsigned char *)g_sBuffer.pucFill, AUDIO_PACKET_SIZE,
                      USBBufferCallback);
}

//****************************************************************************
//
// This function updates the mute area of the status bar.
//
//****************************************************************************
void
UpdateMute(void)
{
    tRectangle sRect;
    short sVolume;

    //
    // Set the bounds of the mute rectangle.
    //
    sRect.sXMin = GrContextDpyWidthGet(&g_sContext) -
                  DISPLAY_STATUS_MUTE_TEXT - DISPLAY_STATUS_MUTE_INSET;

    sRect.sYMin = GrContextDpyHeightGet(&g_sContext) -
                  DISPLAY_BANNER_HEIGHT - 1 + DISPLAY_STATUS_MUTE_INSET;

    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) -
                  DISPLAY_STATUS_MUTE_INSET;

    sRect.sYMax = sRect.sYMin + DISPLAY_BANNER_HEIGHT -
                  (2 * DISPLAY_STATUS_MUTE_INSET);

    //
    // See if the current state is muted or not.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_MUTED))
    {
        //
        // Set the volume to 0.
        //
        SoundVolumeSet(0);

        //
        // Draw the mute background rectangle.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_MUTE_BG);
        GrRectFill(&g_sContext, &sRect);

        //
        // Reset the text color and draw the muted text.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_TEXT_FG);

        GrStringDraw(&g_sContext, "Muted", -1,
                     GrContextDpyWidthGet(&g_sContext) -
                     DISPLAY_STATUS_MUTE_TEXT,
                     DISPLAY_STATUS_TEXT_POSY, 0);
    }
    else
    {
        //
        // Reset the volume to previous setting.
        //
        sVolume = CONVERT_TO_PERCENT(g_sVolume);
        SoundVolumeSet(sVolume);

        //
        // Draw over the mute status area with the banner background.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
        GrRectFill(&g_sContext, &sRect);

        //
        // Reset the text color.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_TEXT_FG);
    }
}

//****************************************************************************
//
// This function updates the volume as well as the volume status bar.
//
//****************************************************************************
void
UpdateVolume(void)
{
    char cString[12];
    short sVolume;

    //
    // Set the colors correctly for the volume string.
    //
    GrContextBackgroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrContextForegroundSet(&g_sContext, DISPLAY_TEXT_FG);

    //
    // Get the current volume in percentage.
    //
    sVolume = CONVERT_TO_PERCENT(g_sVolume);

    //
    // Create the volume string.
    //
    usprintf(cString, "Volume:%3d%%", sVolume);

    //
    // Update the volume string.
    //
    GrStringDraw(&g_sContext, cString, -1, 120,
                 DISPLAY_STATUS_TEXT_POSY, 1);

    //
    // Don't update the actual volume if muted.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_MUTED) == 0)
    {
        //
        // Set the volume to the current setting.
        //
        SoundVolumeSet(sVolume);
    }
}

//****************************************************************************
//
// Initialize the audio interface so that it is ready to start running
// the AudioMain() function.
//
//****************************************************************************
void
AudioInit(void)
{
    //
    // Configure the I2S peripheral.
    //
    SoundInit(0);

    //
    // Set the format of the play back in the sound driver.
    //
    SoundSetFormat(48000, 16, 2);

    SoundVolumeSet(0);

    //
    // Initialize the buffer.
    //
    g_sBuffer.pucFill = g_sBuffer.pucBuffer;
    g_sBuffer.pucPlay = g_sBuffer.pucBuffer;
    g_sBuffer.ulFlags = 0;

    //
    // Get the first USB buffer ready to go and wait for a connect.
    //
    if(USBAudioBufferOut(g_sCompDevice.psDevices[0].pvInstance,
                         (unsigned char *)g_sBuffer.pucFill,
                         AUDIO_PACKET_SIZE, USBBufferCallback) == 0)
    {
        //
        // Now filling data.
        //
        g_sBuffer.ulFlags |= SBUFFER_FLAGS_FILLING;
    }

    //
    // Must disable I2S interrupts during this time to prevent state
    // problems.
    //
    ROM_IntEnable(INT_I2S0);
}

//****************************************************************************
//
// This is the main routine portion of the audio handler.
//
//****************************************************************************
void
AudioMain(void)
{
    //
    // Just return if the device is not connected.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_CONNECTED) == 0)
    {
        return;
    }

    //
    // Check if there was a volume update.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_VOLUME_UPDATE))
    {
        //
        // Clear the volume update flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_VOLUME_UPDATE) = 0;

        //
        // Update the current volume.
        //
        UpdateVolume();
    }

    //
    // Check if there was a mute update.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_MUTE_UPDATE))
    {
        //
        // Update the current mute setting.
        //
        UpdateMute();

        //
        // Clear the mute flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_MUTE_UPDATE) = 0;
    }
}

