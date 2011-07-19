//*****************************************************************************
//
// usb_host_audioin.c - Main routine for the USB host audio input example.
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
#include "inc/hw_sysctl.h"
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
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/usb_sound.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "usblib/usblib.h"

//******************************************************************************
//
//! \addtogroup example_list
//! <h1>USB host audio example application using a USB audio device for input
//! and I2S for output. (usb_host_audioin)</h1>
//!
//! This example application demonstrates streaming audio from a USB audio
//! device that supports recording an audio source at 48000 16 bit stereo.
//! The application will start recording audio from the USB audio device when
//! the "Play" button is pressed and stream it to the I2S output on the board.
//! Because some audio devices require more power, you may need to use an
//! external 5 volt supply to provide enough power to the USB audio device.
//!
//
//******************************************************************************

//*****************************************************************************
//
// Our running system tick counter and a global used to determine the time
// elapsed since last call to GetTickms().
//
//*****************************************************************************
unsigned long g_ulSysTickCount;
unsigned long g_ulLastTick;

//*****************************************************************************
//
// This value is used to keep track of the small sample rate adjustments that
// are made to keep the I2S audio output in sync with the USB device.
//
//*****************************************************************************
int g_iAdjust;

//******************************************************************************
//
// Widget definitions
//
//******************************************************************************
#define INITIAL_VOLUME_PERCENT  100

//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define USB_INT_PRIORITY        0x00
#define SYSTICK_INT_PRIORITY    0x40
#define ADC3_INT_PRIORITY       0x80

//******************************************************************************
//
// The number of SysTick ticks per second.
//
//******************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//******************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//******************************************************************************
static void OnBtnPlay(tWidget *pWidget);

//******************************************************************************
//
// Audio buffering definitions, these are optimized to deal with USB audio.
//
//******************************************************************************
#define USB_TRANSFER_SIZE       (192)
#define USB_BUFFERS             (16)
#define AUDIO_BUFFER_SIZE       (USB_TRANSFER_SIZE * USB_BUFFERS)
#define AUDIO_MIN_DIFF          (USB_TRANSFER_SIZE * ((USB_BUFFERS >> 1) - 1))
#define AUDIO_NOMINAL_DIFF      (USB_TRANSFER_SIZE * (USB_BUFFERS >> 1))
#define AUDIO_MAX_DIFF          (USB_TRANSFER_SIZE * ((USB_BUFFERS >> 1) + 1))
unsigned long g_ulTransferSize;
unsigned long g_ulBufferSize;

//******************************************************************************
//
// The main audio buffer and it's pointers.
//
//******************************************************************************
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
#define FLAGS_STREAMING         1

//
// The last transfer has competed so a new one can be started.
//
#define FLAGS_TX_COMPLETE       2

//
// The last input transfer has competed so a new one can be started.
//
#define FLAGS_RX_COMPLETE       3

//
// New audio device present.
//
#define FLAGS_DEVICE_CONNECT    4

//
// New audio device present.
//
#define FLAGS_DEVICE_READY      5

//
// Currently playing audio using the I2S interface.
//
#define FLAGS_PLAYING           6

//******************************************************************************
//
// Widget definitions
//
//******************************************************************************

//******************************************************************************
//
// The list box used to display directory contents.
//
//******************************************************************************
extern tCanvasWidget g_sPlayBackground;

//******************************************************************************
//
// The button used to play/stop a selected file.
//
//******************************************************************************
char g_psPlayText[5] = "Play";
RectangularButton(g_sPlayBtn, &g_sPlayBackground, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 115, 180, 90, 30,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlack, ClrBlue, ClrWhite, ClrWhite,
                   &g_sFontCm20, g_psPlayText, 0, 0, 0, 0, OnBtnPlay);

//******************************************************************************
//
// The canvas widget acting as the background to the play/stop button.
//
//******************************************************************************
Canvas(g_sPlayBackground, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 115, 180, 90, 30,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//******************************************************************************
//
// The status line.
//
//******************************************************************************
#define STATUS_TEXT_SIZE        40
char g_psStatusText[STATUS_TEXT_SIZE] = "";
Canvas(g_sStatus, WIDGET_ROOT, &g_sPlayBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 240-24, 320, 24,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT |
        CANVAS_STYLE_TEXT_LEFT),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontCm20, g_psStatusText, 0, 0);

//******************************************************************************
//
// The heading containing the application title.
//
//******************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sStatus, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontCm20, "usb host audio in", 0, 0);

//******************************************************************************
//
// Handler for playing the buffer that has been filled from the audio in path.
//
//******************************************************************************
static void
PlayBufferCallback(void *pvBuffer, unsigned long ulEvent)
{
    //
    // Just indicate that the last transfer was complete.
    //
    HWREGBITW(&g_ulFlags, FLAGS_TX_COMPLETE) = 1;
}

//******************************************************************************
//
// Schedules new USB Isochronous input from the USB audio device when a
// previous transfer has completed.
//
//******************************************************************************
static void
USBAudioInCallback(void *pvBuffer, unsigned long ulEvent)
{
    //
    // If a buffer has been played then schedule a new one to play.
    //
    if((ulEvent == USB_EVENT_RX_AVAILABLE) &&
       (HWREGBITW(&g_ulFlags, FLAGS_STREAMING)))
    {
        //
        // Increment the read pointer.
        //
        g_pucWrite += g_ulTransferSize;

        //
        // Wrap the read pointer if necessary.
        //
        if(g_pucWrite >= (g_pucAudioBuffer + g_ulBufferSize))
        {
            g_pucWrite = g_pucAudioBuffer;
        }

        //
        // Schedule a new USB audio buffer to be transmitted to the USB
        // audio device.
        //
        USBSoundBufferIn(g_pucWrite, g_ulTransferSize, USBAudioInCallback);

        //
        // Indicate that the last USB transfer has completed.
        //
        HWREGBITW(&g_ulFlags, FLAGS_RX_COMPLETE) = 1;
    }
}

//*****************************************************************************
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
// meant to be a sample rate conversion, it is used to correct for small errors
// in sample rate.
//
// \return None.
//
//*****************************************************************************
void
I2SMClkAdjust(int iMClkAdjust)
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

//******************************************************************************
//
// This function starts up the audio streaming from the USB audio device.  The
// I2S audio is started later when enough audio has been received to start
// transferring buffers to the I2S audio interface.
//
//******************************************************************************
void
StartStreaming(void)
{
    //
    // Request and audio buffer from the USB device.
    //
    USBSoundBufferIn(g_pucRead, g_ulTransferSize, USBAudioInCallback);

    //
    // Change the text on the button to Stop.
    //
    strcpy(g_psPlayText, "Stop");
    WidgetPaint((tWidget *)&g_sPlayBtn);
}

//******************************************************************************
//
// Stops audio streaming for the application.
//
//******************************************************************************
void
StopAudio(void)
{
    int iIdx;
    unsigned long *pulBuffer;

    //
    // Stop playing audio.
    //
    HWREGBITW(&g_ulFlags, FLAGS_STREAMING) = 0;
    HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 0;

    //
    // Reset any sample rate adjustment.
    //
    if(g_iAdjust != 0)
    {
        I2SMClkAdjust(g_iAdjust);
        g_iAdjust = 0;
    }

    //
    // Do unsigned long accesses to clear out the buffer.
    //
    pulBuffer = (unsigned long *)g_pucAudioBuffer;

    //
    // Zero out the buffer.
    //
    for(iIdx = 0; iIdx < (g_ulBufferSize >> 2); iIdx++)
    {
        pulBuffer[iIdx] = 0;
    }

    //
    // Initialize the read and write pointers.
    //
    g_pucRead = g_pucAudioBuffer;
    g_pucWrite = g_pucAudioBuffer;

    //
    // Change the text on the button to Stop.
    //
    strcpy(g_psPlayText, "Play");
    WidgetPaint((tWidget *)&g_sPlayBtn);
}

//******************************************************************************
//
// The "Play/Stop" button widget call back function.
//
// This function is called whenever someone presses the "Play/Stop" button.
//
//******************************************************************************
static void
OnBtnPlay(tWidget *pWidget)
{
    //
    // Determine if this was a Play or Stop command.
    //
    if(HWREGBITW(&g_ulFlags, FLAGS_STREAMING))
    {
        //
        // If already playing then this was a press to stop play back.
        //
        StopAudio();
    }
    else
    {
        //
        // Indicate that audio streaming should start.
        //
        HWREGBITW(&g_ulFlags, FLAGS_STREAMING) = 1;
        StartStreaming();
    }
}

//******************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a
// timer tick every 10ms for internal timing purposes.
//
//******************************************************************************
void
SysTickHandler(void)
{
    //
    // Increment the system tick count.
    //
    g_ulSysTickCount++;
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

//******************************************************************************
//
// This function handled global level events for the USB host audio.  This
// function was passed into the USBSoundInit() function.
//
//******************************************************************************
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

            //
            // Stop streaming audio.
            //
            StopAudio();

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
    ROM_IntPrioritySet(INT_ADC3, ADC3_INT_PRIORITY);

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
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPlayBtn);

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
    // Not playing anything right now.
    //
    g_ulFlags = 0;

    g_ulSysTickCount = 0;
    g_ulLastTick = 0;

    //
    // Configure the USB host audio.
    //
    USBSoundInit(0, AudioEvent);

    //
    // Initialize audio streaming to stopped state.
    //
    StopAudio();

    //
    // Configure the I2S peripheral.
    //
    SoundInit(0);

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

    g_iAdjust = 0;

    //
    //
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
            // Attempt to set the audio format to 48000 16 bit stereo by default
            // otherwise try 44100 16 bit stereo.
            //
            if(USBSoundInputFormatSet(48000, 16, 2) == 0)
            {
                ulTemp = 48000;
            }
            else
            {
                ulTemp = 0;
            }

            //
            // Try to set the output format to match the input and fail if it
            // cannot be set.
            //
            if((ulTemp != 0) && (USBSoundOutputFormatSet(ulTemp, 16, 2) != 0))
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
                g_ulBufferSize = USB_BUFFERS * g_ulTransferSize;

                //
                // Print the time string in the format mm.ss/mm.ss
                //
                usprintf(g_psStatusText, "Ready  %dHz 16 bit Stereo", ulTemp);

                //
                // USB device is ready for operation.
                //
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
        else if(HWREGBITW(&g_ulFlags, FLAGS_TX_COMPLETE))
        {
            //
            // Acknowledge that the transmit has been handled.
            //
            HWREGBITW(&g_ulFlags, FLAGS_TX_COMPLETE) = 0;

            //
            // Make sure that the application is still playing audio.
            //
            if(HWREGBITW(&g_ulFlags, FLAGS_PLAYING))
            {
                //
                // Determine the gap between the read and write pointers.
                //
                if(g_pucRead < g_pucWrite)
                {
                    ulTemp = g_pucWrite - g_pucRead;
                }
                else
                {
                    ulTemp = g_ulBufferSize + g_pucWrite - g_pucRead;
                }

                if(ulTemp < AUDIO_MIN_DIFF)
                {
                    //
                    // Check if the difference in the read and write pointers
                    // has gone beyond the minimum range and there is no current
                    // adjustment in effect.
                    //
                    if(g_iAdjust == 0)
                    {
                        //
                        // Speed up the sample rate by one fractional bit until
                        // the rate moved back into the nominal range.
                        //
                        g_iAdjust = 1;
                        I2SMClkAdjust(g_iAdjust);
                    }
                }
                else if(ulTemp > AUDIO_MAX_DIFF)
                {
                    //
                    // Check if the difference in the read and write pointers
                    // has gone beyond the maximum range and there is no current
                    // adjustment in effect.
                    //
                    if(g_iAdjust == 0)
                    {
                        //
                        // Slow down the sample rate by one fractional bit until
                        // the rate moved back into the nominal range.
                        //
                        g_iAdjust = -1;
                        I2SMClkAdjust(g_iAdjust);
                    }
                }
                else if(ulTemp == AUDIO_NOMINAL_DIFF)
                {
                    //
                    // If the value has returned to the a mid range then set the
                    // adjustment back to normal.
                    //
                    if(g_iAdjust != 0)
                    {
                        //
                        // Move the adjustment back to nominal.
                        //
                        I2SMClkAdjust(-g_iAdjust);

                        //
                        // The adjustment has been set back to nominal so zero
                        // out the value.
                        //
                        g_iAdjust = 0;
                    }
                }

                //
                // Play the current buffer.
                //
                SoundBufferPlay(g_pucRead, g_ulBufferSize >> 1,
                                PlayBufferCallback);
                //
                // Increment the read pointer.
                //
                g_pucRead += g_ulBufferSize >> 1;

                //
                // Wrap the read pointer if necessary.
                //
                if(g_pucRead >= (g_pucAudioBuffer + g_ulBufferSize))
                {
                    g_pucRead = g_pucAudioBuffer;
                }
            }
        }
        else if(HWREGBITW(&g_ulFlags, FLAGS_RX_COMPLETE))
        {
            //
            // Acknowledge the USB receive has been handled.
            //
            HWREGBITW(&g_ulFlags, FLAGS_RX_COMPLETE) = 0;

            //
            // If not already playing out the I2S then start.
            //
            if((HWREGBITW(&g_ulFlags, FLAGS_STREAMING) != 0) &&
               (HWREGBITW(&g_ulFlags, FLAGS_PLAYING) == 0))
            {
                //
                // Determine the gap between the read and write pointers.
                //
                if(g_pucRead < g_pucWrite)
                {
                    ulTemp = g_pucWrite - g_pucRead;
                }
                else
                {
                    ulTemp = g_ulBufferSize + g_pucWrite - g_pucRead;
                }

                //
                // Wait until we have a few buffers before starting playback.
                //
                if(ulTemp >= AUDIO_NOMINAL_DIFF)
                {
                    //
                    // Save playing state and start the first buffer.
                    //
                    HWREGBITW(&g_ulFlags, FLAGS_PLAYING) = 1;
                    SoundBufferPlay(g_pucRead, g_ulBufferSize >> 1,
                                    PlayBufferCallback);
                }
            }
        }

        //
        // Allow the USB non-interrupt code to run.
        //
        USBMain(GetTickms());

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}
