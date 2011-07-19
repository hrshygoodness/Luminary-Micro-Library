//****************************************************************************
//
// usb_dev_caudiohid.c - The main routine for the combined audio hid keyboard
// device.
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
//****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_udma.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdaudio.h"
#include "usblib/device/usbdcomp.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidkeyb.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/sound.h"
#include "drivers/set_pinout.h"
#include "usb_structs.h"
#include "usb_dev_caudiohid.h"

//****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Audio Device (usb_dev_caudiohid)</h1>
//!
//! This example application turns the evaluation board into a composite USB
//! device supporting the Human Interface Device keyboard class and a USB audio
//! device that supports playback of a single 16 bit stereo audio stream at
//! 48 kHz sample rate.
//!
//! The audio device supports only plackback and will respond to volume control
//! and mute changes and apply them to the sound driver.  These volume control
//! changes will only affect the headphone output and not the line output
//! because the audio DAC used on this board only allows volume changes to the
//! headphones.  The USB audio device example will work on any operating system
//! that supports USB audio class devices natively.  There should be no
//! addition operating system specific drivers required to use the example. The
//! application's main task is to pass buffers to the USB library's audio
//! device class, receive them back with audio data and pass the buffers on to
//! the sound driver for this board.
//!
//! This keyboard device supports the Human Interface Device class and the
//! color LCD display shows a virtual keyboard and taps on the touchscreen will
//! send appropriate key usage codes back to the USB host. Modifier keys
//! (Shift, Ctrl and Alt) are ``sticky'' and tapping them toggles their state.
//! The board status LED is used to indicate the current Caps Lock state and is
//! updated in response to pressing the ``Caps'' key on the virtual keyboard or
//! any other keyboard attached to the same USB host system.  The keyboard
//! device implemented by this application also supports USB remote wakeup
//! allowing it to request the host to reactivate a suspended bus.  If the bus
//! is suspended (as indicated on the application display), touching the
//! display will request a remote wakeup assuming the host has not
//! specifically disabled such requests.
//
//****************************************************************************

extern const tUSBDAudioDevice g_sAudioDevice;
extern const tUSBDHIDKeyboardDevice g_sKeyboardDevice;
extern tUSBDCompositeDevice g_sCompDevice;

//****************************************************************************
//
// The DMA control structure table.
//
//****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//****************************************************************************
//
// Graphics context used to show text on the color STN display.
//
//****************************************************************************
tContext g_sContext;

//****************************************************************************
//
// The memory allocated to hold the composite descriptor that is created by
// the call to USBDCompositeInit().
//
//****************************************************************************
#define DESCRIPTOR_DATA_SIZE    (COMPOSITE_DHID_SIZE + COMPOSITE_DAUDIO_SIZE)
unsigned char g_pucDescriptorData[DESCRIPTOR_DATA_SIZE];

volatile unsigned long g_ulFlags;

//****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

unsigned long
EventHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgData,
             void *pvMsgData)
{
    unsigned long ulNewEvent;

    //
    // Default to signal a new event.
    //
    ulNewEvent = 1;

    switch(ulEvent)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // Now connected.
            //
            HWREGBITW(&g_ulFlags, FLAG_CONNECTED) = 1;

            //
            // Not suspended by default.
            //
            HWREGBITW(&g_ulFlags, FLAG_SUSPENDED) = 0;

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

            break;
        }

        //
        // This event indicates that the host has suspended the USB bus.
        //
        case USB_EVENT_SUSPEND:
        {
            //
            // Set the Suspended flag.
            //
            HWREGBITW(&g_ulFlags, FLAG_SUSPENDED) = 1;

            break;
        }

        //
        // This event signals that the host has resumed signaling on the bus.
        //
        case USB_EVENT_RESUME:
        {
            //
            // Clear the suspended flag.
            //
            HWREGBITW(&g_ulFlags, FLAG_SUSPENDED) = 0;

            break;
        }

        default:
        {
            //
            // Clear the new event if this was an unknown event.
            //
            ulNewEvent = 0;

            break;
        }
    }

    //
    // See if the status needs to be updated.
    //
    if(ulNewEvent)
    {
        //
        // Set the status update flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_STATUS_UPDATE) = 1;
    }
    return(0);
}

//****************************************************************************
//
// This function updates the status area of the screen.  It uses the current
// state of the application to print the status bar.
//
//****************************************************************************
void
UpdateStatus(void)
{
    tRectangle sRect;

    //
    // Clear the status update flag.
    //
    HWREGBITW(&g_ulFlags, FLAG_STATUS_UPDATE) = 0;

    //
    // Fill the bottom rows of the screen with blue to create the status area.
    //
    sRect.sXMin = 0;
    sRect.sYMin = GrContextDpyHeightGet(&g_sContext) -
                  DISPLAY_BANNER_HEIGHT - 1;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = sRect.sYMin + DISPLAY_BANNER_HEIGHT;

    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_TEXT_FG);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

    GrContextBackgroundSet(&g_sContext, DISPLAY_BANNER_BG);

    //
    // Update the status on the screen.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_CONNECTED))
    {
        //
        // If connected and suspended then suspended.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SUSPENDED))
        {
            //
            // Device is currently connected.
            //
            GrStringDraw(&g_sContext, "Suspended", -1, 4,
                         DISPLAY_STATUS_TEXT_POSY, 0);
        }
        else
        {
            //
            // Device is currently connected.
            //
            GrStringDraw(&g_sContext, "Connected", -1, 4,
                         DISPLAY_STATUS_TEXT_POSY, 0);
        }

        //
        // Update the current mute setting.
        //
        UpdateMute();

        //
        // Update the current volume setting.
        //
        UpdateVolume();
    }
    else
    {
        //
        // Keyboard is currently disconnected.
        //
        GrStringDraw(&g_sContext, "Disconnected", -1, 4, sRect.sYMin + 8, 0);
    }
}

//****************************************************************************
//
// This is the main loop that runs the application.
//
//****************************************************************************
int
main(void)
{
    tRectangle sRect;

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Set the device pin out appropriately for this board.
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
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = DISPLAY_BANNER_HEIGHT - 1;
    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_TEXT_FG);
    GrContextFontSet(&g_sContext, &g_sFontCm20);
    GrStringDrawCentered(&g_sContext, "usb-dev-audio-hid", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 10, 0);

    //
    // Initialize to nothing set.
    //
    g_ulFlags = 0;

    //
    // Pass the USB library our device information, initialize the USB
    // controller and connect the device to the bus.
    //
    g_sCompDevice.psDevices[0].pvInstance =
        USBDAudioCompositeInit(0, &g_sAudioDevice);
    g_sCompDevice.psDevices[1].pvInstance =
        USBDHIDKeyboardCompositeInit(0, &g_sKeyboardDevice);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDCompositeInit(0, &g_sCompDevice, DESCRIPTOR_DATA_SIZE,
                      g_pucDescriptorData);

    //
    // Initialized the audio control.
    //
    AudioInit();

    //
    // Initialized the keyboard control.
    //
    KeyboardInit();

    //
    // Update the status bar.
    //
    UpdateStatus();

    //
    // Drop into the main loop.
    //
    while(1)
    {
        //
        // Detect if the status needs to be updated.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_STATUS_UPDATE))
        {
            UpdateStatus();
        }

        //
        // Call the audio main loop handler.
        //
        AudioMain();

        //
        // Call the keyboard main loop handler.
        //
        KeyboardMain();
    }
}
