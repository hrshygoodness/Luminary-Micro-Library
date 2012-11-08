//*****************************************************************************
//
// usbdescriptors.c - USB Device Descriptors for the Quickstart Oscilloscope.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usbdescriptors.h"

//*****************************************************************************
//
// Device Descriptor.
//
//*****************************************************************************
const unsigned char g_pDeviceDescriptor[] =
{
    18,                          // Size of this structure.
    USB_DTYPE_DEVICE,            // Type of this structure.
    USBShort(0x110),             // USB version 1.1 (if we say 2.0, hosts assume
                                 // high-speed - see USB 2.0 spec 9.2.6.6)
    USB_CLASS_VEND_SPECIFIC,     // USB Device Class
    0,                           // USB Device Sub-class
    0,                           // USB Device protocol
    64,                          // Maximum packet size for default pipe.
    USBShort(USB_VID_STELLARIS), // Vendor ID (VID).
    USBShort(USB_PID_SCOPE),     // Product ID (PID).
    USBShort(0x100),             // Device Version BCD.
    1,                           // Manufacturer string identifier.
    2,                           // Product string indentifier.
    3,                           // Product serial number.
    1                            // Number of configurations.
};

//*****************************************************************************
//
// Oscilloscope device configuration descriptor.
//
// Note that it is vital that the configuration descriptor bConfigurationValue
// field (byte 6) is 1 for the first configuration and increments by 1 for
// each additional configuration defined here.  This relationship is assumed
// in the device stack for simplicity even though the USB 2.0 specification
// imposes no such restriction on the bConfigurationValue values.
//
//*****************************************************************************
const unsigned char g_pScopeConfigDescriptor[] =
{
    //
    // Configuration descriptor header.
    //
    9,                          // Size of the configuration descriptor.
    USB_DTYPE_CONFIGURATION,    // Type of this descriptor.
    USBShort(32),               // The total size of this full structure.
    1,                          // The number of interfaces in this
                                // configuration.
    1,                          // The unique value for this configuration.
    5,                          // The string identifier that describes this
                                // configuration.
    USB_CONF_ATTR_SELF_PWR,     // Bus Powered, Self Powered, remote wakeup.
    250,                        // The maximum power in 2mA increments.

    //
    // Vendor-specific Interface Descriptor.
    //
    9,                          // Size of the interface descriptor.
    USB_DTYPE_INTERFACE,        // Type of this descriptor.
    0,                          // The index for this interface.
    0,                          // The alternate setting for this interface.
    2,                          // The number of endpoints used by this
                                // interface.
    USB_CLASS_VEND_SPECIFIC,    // The interface class
    0,                          // The interface sub-class.
    0,                          // The interface protocol for the sub-class
                                // specified above.
    4,                          // The string index for this interface.

    //
    // Endpoint Descriptor
    //
    7,                               // The size of the endpoint descriptor.
    USB_DTYPE_ENDPOINT,              // Descriptor type is an endpoint.
    USB_EP_DESC_IN | USB_EP_TO_INDEX(DATA_IN_ENDPOINT),
    USB_EP_ATTR_BULK,                // Endpoint is a bulk endpoint.
    USBShort(DATA_IN_EP_MAX_SIZE),   // The maximum packet size.
    0,                               // The polling interval for this endpoint.

    //
    // Endpoint Descriptor
    //
    7,                               // The size of the endpoint descriptor.
    USB_DTYPE_ENDPOINT,              // Descriptor type is an endpoint.
    USB_EP_DESC_OUT | USB_EP_TO_INDEX(DATA_OUT_ENDPOINT),
    USB_EP_ATTR_BULK,                // Endpoint is a bulk endpoint.
    USBShort(DATA_OUT_EP_MAX_SIZE),  // The maximum packet size.
    0,                               // The polling interval for this endpoint.
};

//*****************************************************************************
//
// The oscilloscope config descriptor is stored in a single block so we only
// have one section. This structure defines the size of the section and its
// start address.
//
//*****************************************************************************
const tConfigSection g_sScopeConfigSection =
{
   sizeof(g_pScopeConfigDescriptor),
   g_pScopeConfigDescriptor
};

//*****************************************************************************
//
// This is the list of sections comprising the single USB configuration
// offered by the oscilloscope. We only ahve a single section so this is
// rather trivial.
//
//*****************************************************************************
const tConfigSection *g_psScopeConfigSections[] =
{
    &g_sScopeConfigSection
};

#define NUM_SCOPE_SECTIONS (sizeof(g_psScopeConfigSections) /                 \
                            sizeof(tConfigSection *))

//*****************************************************************************
//
// This structure defines the complete config descriptor for the single USB
// configuration offered by the oscilloscope.
//
//*****************************************************************************
const tConfigHeader g_sScopeConfigHeader =
{
   NUM_SCOPE_SECTIONS,
   g_psScopeConfigSections
};

//*****************************************************************************
//
// This array contains pointers to each of the USB configuration descriptors
// offered by the device. In this case, we only have a single configuration
// so the table is rather short.
//
//*****************************************************************************
const tConfigHeader * const g_pScopeConfigDescriptors[] =
{
    &g_sScopeConfigHeader
};

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

    (22 + 1) * 2,
    USB_DTYPE_STRING,
    'T', 0, 'e', 0, 'x', 0, 'a', 0, 's', 0, ' ', 0, 'I', 0, 'n', 0, 's', 0,
    't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0, ' ', 0,
    'I', 0, 'n', 0, 'c', 0, '.', 0
};

//*****************************************************************************
//
// The product string.
//
//*****************************************************************************
const unsigned char g_pProductString[] =
{
    (23 + 1) * 2,
    USB_DTYPE_STRING,
    'Q', 0, 'u', 0, 'i', 0, 'c', 0, 'k', 0, 's', 0, 't', 0, 'a', 0, 'r', 0,
    't', 0, ' ', 0, 'O', 0, 's', 0, 'c', 0, 'i', 0, 'l', 0, 'l', 0, 'o', 0,
    's', 0, 'c', 0, 'o', 0, 'p', 0, 'e', 0
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
    '0', 0, '0', 0, '0', 0, '1', 0, '.', 0, '0', 0, '0', 0, '0', 0
};

//*****************************************************************************
//
// The data interface description string.
//
//*****************************************************************************
const unsigned char g_pDataInterfaceString[] =
{
    (22 + 1) * 2,
    USB_DTYPE_STRING,
    'O', 0, 's', 0, 'c', 0, 'i', 0, 'l', 0, 'l', 0, 'o', 0, 's', 0, 'c', 0,
    'o', 0, 'p', 0, 'e', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0,
    'f', 0, 'a', 0, 'c', 0, 'e', 0
};

//*****************************************************************************
//
// The configuration description string.
//
//*****************************************************************************
const unsigned char g_pConfigString[] =
{
    (26 + 1) * 2,
    USB_DTYPE_STRING,
    'O', 0, 's', 0, 'c', 0, 'i', 0, 'l', 0, 'l', 0, 'o', 0, 's', 0, 'c', 0,
    'o', 0, 'p', 0, 'e', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 'f', 0, 'i', 0,
    'g', 0, 'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0, 'o', 0, 'n', 0
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
    g_pDataInterfaceString,
    g_pConfigString
};

//*****************************************************************************
//
// The device information structure for the USB oscilloscope device.
//
//*****************************************************************************
tDeviceInfo g_sScopeDeviceInfo =
{
    //
    // Device event handler callbacks.
    //
    {
        0,                      // GetDescriptor
        0,                      // RequestHandler
        0,                      // InterfaceChange
        HandleConfigChange,     // ConfigChange
        0,                      // DataReceived
        0,                      // DataSentCallback
        HandleReset,            // ResetHandler
        0,                      // SuspendHandler
        0,                      // ResumeHandler
        HandleDisconnect,       // DisconnectHandler
        HandleEndpoints         // EndpointHandler
    },
    g_pDeviceDescriptor,
    g_pScopeConfigDescriptors,
    g_pStringDescriptors,
    (unsigned long)(sizeof(g_pStringDescriptors) /
                    sizeof (unsigned char const *)),
    &g_sUSBDefaultFIFOConfig
};
