//*****************************************************************************
//
// usb_audio_structs.c - Data structures defining the USB audio device.
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
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdaudio.h"
#include "usb_audio_structs.h"

extern unsigned long
AudioMessageHandler(void *pvCBData, unsigned long ulEvent,
                    unsigned long ulMsgParam, void *pvMsgData);

//*****************************************************************************
//
// The languages supported by this device.
//
//*****************************************************************************
const unsigned char g_pLangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

//*****************************************************************************
//
// The manufacturer string.
//
//*****************************************************************************
const unsigned char g_pManufacturerString[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'T', 0, 'e', 0, 'x', 0, 'a', 0, 's', 0, ' ', 0, 'I', 0, 'n', 0, 's', 0,
    't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0,
};

//*****************************************************************************
//
// The product string.
//
//*****************************************************************************
const unsigned char g_pProductString[] =
{
    (13 + 1) * 2,
    USB_DTYPE_STRING,
    'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, ' ', 0, 'E', 0, 'x', 0, 'a', 0,
    'm', 0, 'p', 0, 'l', 0, 'e', 0
};

//*****************************************************************************
//
// The serial number string.
//
//*****************************************************************************
const unsigned char g_pSerialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};

//*****************************************************************************
//
// The interface description string.
//
//*****************************************************************************
const unsigned char g_pInterfaceString[] =
{
    (15 + 1) * 2,
    USB_DTYPE_STRING,
    'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, ' ', 0, 'I', 0, 'n', 0,
    't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0
};

//*****************************************************************************
//
// The configuration description string.
//
//*****************************************************************************
const unsigned char g_pConfigString[] =
{
    (20 + 1) * 2,
    USB_DTYPE_STRING,
    'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, ' ', 0, ' ', 0, 'C', 0,
    'o', 0, 'n', 0, 'f', 0, 'i', 0, 'g', 0, 'u', 0, 'r', 0, 'a', 0,
    't', 0, 'i', 0, 'o', 0, 'n', 0
};

//*****************************************************************************
//
// The descriptor string table.
//
//*****************************************************************************
const unsigned char * const g_pStringDescriptors[] =
{
    g_pLangDescriptor,
    g_pManufacturerString,
    g_pProductString,
    g_pSerialNumberString,
    g_pInterfaceString,
    g_pConfigString
};

#define NUM_STRING_DESCRIPTORS (sizeof(g_pStringDescriptors) /                \
                                sizeof(unsigned char *))

//*****************************************************************************
//
// The audio device private instance data, this should never be modified.
//
//*****************************************************************************
static tAudioInstance g_sAudioInstance;

//*****************************************************************************
//
// The audio device initialization and customization structures.
//
//*****************************************************************************
const tUSBDAudioDevice g_sAudioDevice =
{
    //
    // The Vendor ID assigned by USB-IF.
    //
    USB_VID_STELLARIS,

    //
    // The product ID assigned to the USB audio device for USB_VID_STELLARIS
    //
    USB_PID_AUDIO,

    //
    // Vendor Information.
    //
    "TI      ",

    //
    // Product Identification.
    //
    "Audio Device    ",

    //
    // Revision.
    //
    "1.00",

    //
    // Power consumption is 500 milliamps.
    //
    500,

    //
    // The value to be passed to the host in the USB configuration descriptor's
    // bmAttributes field.
    //
    USB_CONF_ATTR_SELF_PWR,

    //
    // A pointer to the control callback message handler.
    //
    AudioMessageHandler,

    //
    // A pointer to the string table.
    //
    g_pStringDescriptors,

    //
    // The number of entries in the string table.
    //
    NUM_STRING_DESCRIPTORS,

    //
    // Maximum volume setting expressed as and 8.8 signed fixed point number.
    //
    VOLUME_MAX,

    //
    // Minimum volume setting expressed as and 8.8 signed fixed point number.
    //
    VOLUME_MIN,

    //
    // Minimum volume step expressed as and 8.8 signed fixed point number.
    //
    VOLUME_STEP,

    //
    // This is the private instance data for the audio class driver.
    //
    &g_sAudioInstance
};
