//****************************************************************************
//
// usb_structs.c - Data structures defining the composite HID mouse and CDC
// serial USB device.
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9107 of the EK-LM3S9B92 Firmware Package.
//
//****************************************************************************

#include "inc/hw_types.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/usb-ids.h"
#include "usblib/usbcdc.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcomp.h"
#include "usblib/device/usbdcdc.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usb_structs.h"

//****************************************************************************
//
// External Class specific handlers.
//
//****************************************************************************
extern const tUSBBuffer g_sTxBuffer;
extern const tUSBBuffer g_sRxBuffer;

//****************************************************************************
//
// The languages supported by this device.
//
//****************************************************************************
const unsigned char g_pLangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

//****************************************************************************
//
// The manufacturer string.
//
//****************************************************************************
const unsigned char g_pManufacturerString[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'T', 0, 'e', 0, 'x', 0, 'a', 0, 's', 0, ' ', 0, 'I', 0, 'n', 0, 's', 0,
    't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0,
};

//****************************************************************************
//
// The product string.
//
//****************************************************************************
const unsigned char g_pProductString[] =
{
    (42 + 1) * 2,
    USB_DTYPE_STRING,
    'C', 0, 'o', 0, 'm', 0, 'p', 0, 'o', 0, 's', 0, 'i', 0, 't', 0,
    'e', 0, ' ', 0, 'H', 0, 'I', 0, 'D', 0, ' ', 0, 'M', 0, 'o', 0,
    'u', 0, 's', 0, 'e', 0, ' ', 0, 'a', 0, 'n', 0, 'd', 0, ' ', 0,
    'C', 0, 'D', 0, 'C', 0, ' ', 0, 'S', 0, 'e', 0, 'r', 0, 'i', 0,
    'a', 0, 'l', 0, ' ', 0, 'E', 0, 'x', 0, 'a', 0, 'm', 0, 'p', 0,
    'l', 0, 'e', 0,
};

//****************************************************************************
//
// The serial number string.
//
//****************************************************************************
const unsigned char g_pSerialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};

//****************************************************************************
//
// The descriptor string table.
//
//****************************************************************************
const unsigned char * const g_pStringDescriptors[] =
{
    g_pLangDescriptor,
    g_pManufacturerString,
    g_pProductString,
    g_pSerialNumberString
};

#define NUM_STRING_DESCRIPTORS (sizeof(g_pStringDescriptors) /               \
                                sizeof(unsigned char *))

//****************************************************************************
//
// The HID mouse device initialization and customization structures.
//
//****************************************************************************
tHIDMouseInstance g_sMouseInstance;

const tUSBDHIDMouseDevice g_sMouseDevice =
{
    //
    // Stellaris VID.
    //
    USB_VID_STELLARIS,

    //
    // Stellaris HID Mouse PID.
    //
    USB_PID_MOUSE,

    //
    // This is in 2mA increments so 500mA.
    //
    250,

    //
    // Bus powered device.
    //
    USB_CONF_ATTR_BUS_PWR,

    //
    // The Mouse handler function.
    //
    MouseHandler,

    //
    // Point to the mouse device structure.
    //
    (void *)&g_sMouseDevice,

    //
    // The composite device does not use the strings from the class.
    //
    0,
    0,

    //
    // The instance data for this mouse device.
    //
    &g_sMouseInstance
};

//****************************************************************************
//
// The CDC device initialization and customization structures. In this case,
// we are using USBBuffers between the CDC device class driver and the
// application code. The function pointers and callback data values are set
// to insert a buffer in each of the data channels, transmit and receive.
//
// With the buffer in place, the CDC channel callback is set to the relevant
// channel function and the callback data is set to point to the channel
// instance data. The buffer, in turn, has its callback set to the application
// function and the callback data set to our CDC instance structure.
//
//****************************************************************************
tCDCSerInstance g_sCDCInstance;

const tUSBDCDCDevice g_sCDCDevice =
{
    //
    // Stellaris VID.
    //
    USB_VID_STELLARIS,

    //
    // Stellaris Virtual Serial Port PID.
    //
    USB_PID_SERIAL,

    //
    // This is in 2mA increments so 500mA.
    //
    250,

    //
    // Bus powered device.
    //
    USB_CONF_ATTR_BUS_PWR,

    //
    // Serial device information callback function.
    //
    SerialHandler,

    //
    // The CDC Serial device information.
    //
    (void *)&g_sCDCDevice,

    //
    // Receive buffer.
    //
    USBBufferEventCallback,
    (void *)&g_sRxBuffer,

    //
    // Transmit buffer.
    //
    USBBufferEventCallback,
    (void *)&g_sTxBuffer,

    //
    // The composite device does not use the strings from the class.
    //
    0,
    0,

    //
    // The serial instance data for this device.
    //
    &g_sCDCInstance
};

//****************************************************************************
//
// Receive buffer (from the USB perspective).
//
//****************************************************************************
unsigned char g_pcUSBRxBuffer[UART_BUFFER_SIZE];
unsigned char g_pucRxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
const tUSBBuffer g_sRxBuffer =
{
    false,                          // This is a receive buffer.
    RxHandler,                      // pfnCallback
    (void *)&g_sCDCDevice,          // Callback data is our device pointer.
    USBDCDCPacketRead,              // pfnTransfer
    USBDCDCRxPacketAvailable,       // pfnAvailable
    (void *)&g_sCDCDevice,          // pvHandle
    g_pcUSBRxBuffer,                // pcBuffer
    UART_BUFFER_SIZE,               // ulBufferSize
    g_pucRxBufferWorkspace          // pvWorkspace
};

//****************************************************************************
//
// Transmit buffer (from the USB perspective).
//
//****************************************************************************
unsigned char g_pcUSBTxBuffer[UART_BUFFER_SIZE];
unsigned char g_pucTxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
const tUSBBuffer g_sTxBuffer =
{
    true,                           // This is a transmit buffer.
    TxHandler,                      // pfnCallback
    (void *)&g_sCDCDevice,          // Callback data is our device pointer.
    USBDCDCPacketWrite,             // pfnTransfer
    USBDCDCTxPacketAvailable,       // pfnAvailable
    (void *)&g_sCDCDevice,          // pvHandle
    g_pcUSBTxBuffer,                // pcBuffer
    UART_BUFFER_SIZE,               // ulBufferSize
    g_pucTxBufferWorkspace          // pvWorkspace
};

//****************************************************************************
//
// The number of individual device class instances comprising this composite
// device.
//
//****************************************************************************
#define NUM_DEVICES 2

//****************************************************************************
//
// The array of devices supported by this composite device.
//
//****************************************************************************
tCompositeEntry g_psCompDevices[NUM_DEVICES]=
{
    //
    // HID Mouse Information.
    //
    {
        &g_sHIDDeviceInfo,
        0
    },

    //
    // Serial Device Instance.
    //
    {
        &g_sCDCSerDeviceInfo,
        0
    }
};

//****************************************************************************
//
// Additional workspaced required by the composite device.
//
//****************************************************************************
unsigned long g_pulCompWorkspace[NUM_DEVICES];

//****************************************************************************
//
// The instance data for this composite device.
//
//****************************************************************************
tCompositeInstance g_CompInstance;

//****************************************************************************
//
// Allocate the Device Data for the top level composite device class.
//
//****************************************************************************
tUSBDCompositeDevice g_sCompDevice =
{
    //
    // Stellaris VID.
    //
    USB_VID_STELLARIS,

    //
    // Stellaris PID for composite serial device.
    //
    USB_PID_COMP_HID_SER,

    //
    // This is in 2mA increments so 500mA.
    //
    250,

    //
    // Bus powered device.
    //
    USB_CONF_ATTR_BUS_PWR,

    //
    // There is no need for a default composite event handler.
    //
    EventHandler,

    //
    // The string table.
    //
    g_pStringDescriptors,
    NUM_STRING_DESCRIPTORS,

    //
    // The Composite device array.
    //
    NUM_DEVICES,
    g_psCompDevices,

    //
    // Workspace required by the composite device.
    //
    g_pulCompWorkspace,
    &g_CompInstance
};
