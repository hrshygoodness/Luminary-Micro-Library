//*****************************************************************************
//
// usb_structs.c - Data structures defining this CDC USB device.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcomp.h"
#include "usblib/device/usbdcdc.h"
#include "usb_structs.h"

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
    2 + (17 * 2),
    USB_DTYPE_STRING,
    'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0,
    'C', 0, 'O', 0, 'M', 0, ' ', 0, 'P', 0, 'o', 0, 'r', 0, 't', 0,
    's', 0
};

//*****************************************************************************
//
// The serial number string.
//
//*****************************************************************************
const unsigned char g_pSerialNumberString[] =
{
    2 + (8 * 2),
    USB_DTYPE_STRING,
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
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
    g_pSerialNumberString
};

#define NUM_STRING_DESCRIPTORS (sizeof(g_pStringDescriptors) /                \
                                sizeof(unsigned char *))

//*****************************************************************************
//
// CDC device callback function prototypes.
//
//*****************************************************************************
unsigned long ControlHandler(void *pvCBData, unsigned long ulEvent,
                             unsigned long ulMsgValue, void *pvMsgData);

//*****************************************************************************
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
//*****************************************************************************
tCDCSerInstance g_psCDCInstance[2];

const tUSBDCDCDevice g_psCDCDevice[2] =
{
    {
        USB_VID_STELLARIS,
        USB_PID_SERIAL,
        0,
        USB_CONF_ATTR_SELF_PWR,
        ControlHandler,
        (void *)&g_psCDCDevice[0],
        USBBufferEventCallback,
        (void *)&g_psRxBuffer[0],
        USBBufferEventCallback,
        (void *)&g_psTxBuffer[0],
        0, // null string table
        0, // zero strings
        &g_psCDCInstance[0]
    },
    {
        USB_VID_STELLARIS,
        USB_PID_SERIAL,
        0,
        USB_CONF_ATTR_SELF_PWR,
        ControlHandler,
        (void *)&g_psCDCDevice[1],
        USBBufferEventCallback,
        (void *)&g_psRxBuffer[1],
        USBBufferEventCallback,
        (void *)&g_psTxBuffer[1],
        0, // null string table
        0, // zero strings
        &g_psCDCInstance[1]
    }
};

//*****************************************************************************
//
// Receive buffer (from the USB perspective).
//
//*****************************************************************************
unsigned char g_pcUSBRxBuffer[UART_BUFFER_SIZE * 2];
unsigned char g_pucRxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE * 2];
const tUSBBuffer g_psRxBuffer[2] =
{
    {
        false,                          // This is a receive buffer.
        RxHandlerEcho,                  // pfnCallback
        (void *)&g_psCDCDevice[0],      // Callback data is our device
                                        // pointer.
        USBDCDCPacketRead,              // pfnTransfer
        USBDCDCRxPacketAvailable,       // pfnAvailable
        (void *)&g_psCDCDevice[0],      // pvHandle
        g_pcUSBRxBuffer,                // pcBuffer
        UART_BUFFER_SIZE,               // ulBufferSize
        g_pucRxBufferWorkspace          // pvWorkspace
    },
    {
        false,                          // This is a receive buffer.
        RxHandlerCmd,                   // pfnCallback
        (void *)&g_psCDCDevice[1],      // Callback data is our device
                                        // pointer.
        USBDCDCPacketRead,              // pfnTransfer
        USBDCDCRxPacketAvailable,       // pfnAvailable
        (void *)&g_psCDCDevice[1],      // pvHandle
                                        // pcBuffer
        &g_pcUSBRxBuffer[UART_BUFFER_SIZE],
        UART_BUFFER_SIZE,               // ulBufferSize
                                        // pvWorkspace
        &g_pucRxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE]
    }
};

//*****************************************************************************
//
// Transmit buffer (from the USB perspective).
//
//*****************************************************************************
unsigned char g_pcUSBTxBuffer[UART_BUFFER_SIZE * 2];
unsigned char g_pucTxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE * 2];
const tUSBBuffer g_psTxBuffer[2] =
{
    {
        true,                           // This is a transmit buffer.
        TxHandlerEcho,                  // pfnCallback
        (void *)&g_psCDCDevice[0],      // Callback data is our device
                                        // pointer.
        USBDCDCPacketWrite,             // pfnTransfer
        USBDCDCTxPacketAvailable,       // pfnAvailable
        (void *)&g_psCDCDevice[0],      // pvHandle
        g_pcUSBTxBuffer,                // pcBuffer
        UART_BUFFER_SIZE,               // ulBufferSize
        g_pucTxBufferWorkspace          // pvWorkspace
    },
    {
        true,                           // This is a transmit buffer.
        TxHandlerCmd,                   // pfnCallback
        (void *)&g_psCDCDevice[1],      // Callback data is our device
                                        // pointer.
        USBDCDCPacketWrite,             // pfnTransfer
        USBDCDCTxPacketAvailable,       // pfnAvailable
        (void *)&g_psCDCDevice[1],      // pvHandle
                                        // pcBuffer
        &g_pcUSBTxBuffer[UART_BUFFER_SIZE],
        UART_BUFFER_SIZE,               // ulBufferSize
                                        // pvWorkspace
        &g_pucTxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE]
    }
};

//*****************************************************************************
//
// The number of individual device class instances that comprise the composite
// device.
//
//*****************************************************************************
#define NUM_DEVICES 2

//*****************************************************************************
//
// The array of devices supported by this composite device.
//
//*****************************************************************************
tCompositeEntry g_psCompDevices[NUM_DEVICES]=
{
    //
    // Serial Instance 0.
    //
    {
        &g_sCDCSerDeviceInfo,
        0
    },

    //
    // Serial Instance 1.
    //
    {
        &g_sCDCSerDeviceInfo,
        0
    }
};

//*****************************************************************************
//
// The instance data for this composite device.
//
//*****************************************************************************
tCompositeInstance g_CompInstance;

//*****************************************************************************
//
// Additional workspace required by the composite device.
//
//*****************************************************************************
unsigned long g_pulCompWorkspace[NUM_DEVICES];

//*****************************************************************************
//
// Allocate the Device Data for the top level composite device class.
//
//*****************************************************************************
tUSBDCompositeDevice g_sCompDevice =
{
    //
    // Stellaris VID.
    //
    USB_VID_STELLARIS,

    //
    // Stellaris PID for composite serial device.
    //
    USB_PID_COMP_SERIAL,

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
    0,

    //
    // The string table.
    //
    g_pStringDescriptors,
    NUM_STRING_DESCRIPTORS,

    NUM_DEVICES,
    g_psCompDevices,

    //
    // Workspace required by the composite device.
    //
    g_pulCompWorkspace,
    &g_CompInstance
};
