//*****************************************************************************
//
// usb_if.c - USB serial port interface for the game.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM3S9D92 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/usbcdc.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "zip/ztypes.h"
#include "common.h"

//*****************************************************************************
//
// The languages supported by this device.
//
//*****************************************************************************
static const unsigned char g_pucLangDescriptor[] =
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
static const unsigned char g_pucManufacturerString[] =
{

    2 + (19 * 2),
    USB_DTYPE_STRING,
    'L', 0, 'u', 0, 'm', 0, 'i', 0, 'n', 0, 'a', 0, 'r', 0, 'y', 0,
    ' ', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, ' ', 0, 'I', 0,
    'n', 0, 'c', 0, '.', 0
};

//*****************************************************************************
//
// The product string.
//
//*****************************************************************************
static const unsigned char g_pucProductString[] =
{
    2 + (16 * 2),
    USB_DTYPE_STRING,
    'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0,
    'C', 0, 'O', 0, 'M', 0, ' ', 0, 'P', 0, 'o', 0, 'r', 0, 't', 0
};

//*****************************************************************************
//
// The serial number string.
//
//*****************************************************************************
static const unsigned char g_pucSerialNumberString[] =
{
    2 + (8 * 2),
    USB_DTYPE_STRING,
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};

//*****************************************************************************
//
// The control interface description string.
//
//*****************************************************************************
static const unsigned char g_pucControlInterfaceString[] =
{
    2 + (21 * 2),
    USB_DTYPE_STRING,
    'A', 0, 'C', 0, 'M', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 't', 0,
    'r', 0, 'o', 0, 'l', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0,
    'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0
};

//*****************************************************************************
//
// The configuration description string.
//
//*****************************************************************************
static const unsigned char g_pucConfigString[] =
{
    2 + (26 * 2),
    USB_DTYPE_STRING,
    'S', 0, 'e', 0, 'l', 0, 'f', 0, ' ', 0, 'P', 0, 'o', 0, 'w', 0,
    'e', 0, 'r', 0, 'e', 0, 'd', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0,
    'f', 0, 'i', 0, 'g', 0, 'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0,
    'o', 0, 'n', 0
};

//*****************************************************************************
//
// The descriptor string table.
//
//*****************************************************************************
static const unsigned char * const g_ppucStringDescriptors[] =
{
    g_pucLangDescriptor,
    g_pucManufacturerString,
    g_pucProductString,
    g_pucSerialNumberString,
    g_pucControlInterfaceString,
    g_pucConfigString
};

//*****************************************************************************
//
// The number of string descriptors.
//
//*****************************************************************************
#define NUM_STRING_DESCRIPTORS  (sizeof(g_ppucStringDescriptors) /            \
                                 sizeof(unsigned char *))

//*****************************************************************************
//
// The size of the transmit and receive buffers used for the redirected UART.
// This number should be a power of 2 for best performance and should be at
// least twice the size of a maxmum-sized USB packet.
//
//*****************************************************************************
#define UART_BUFFER_SIZE        256

//*****************************************************************************
//
// CDC device callback function prototypes.
//
//*****************************************************************************
static unsigned long RxHandler(void *pvCBData, unsigned long ulEvent,
                               unsigned long ulMsgValue, void *pvMsgData);
static unsigned long TxHandler(void *pvCBData, unsigned long ulEvent,
                               unsigned long ulMsgValue, void *pvMsgData);
static unsigned long ControlHandler(void *pvCBData, unsigned long ulEvent,
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
static tCDCSerInstance g_sCDCInstance;

static const tUSBBuffer g_sTxBuffer;
static const tUSBBuffer g_sRxBuffer;

static const tUSBDCDCDevice g_sCDCDevice =
{
    USB_VID_STELLARIS,
    USB_PID_SERIAL,
    0,
    USB_CONF_ATTR_SELF_PWR,
    ControlHandler,
    (void *)&g_sCDCDevice,
    USBBufferEventCallback,
    (void *)&g_sRxBuffer,
    USBBufferEventCallback,
    (void *)&g_sTxBuffer,
    g_ppucStringDescriptors,
    NUM_STRING_DESCRIPTORS,
    &g_sCDCInstance
};

//*****************************************************************************
//
// Receive buffer (from the USB perspective).
//
//*****************************************************************************
static unsigned char g_pucUSBRxBuffer[UART_BUFFER_SIZE];
static unsigned char g_pucRxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
static const tUSBBuffer g_sRxBuffer =
{
    false,                          // This is a receive buffer.
    RxHandler,                      // pfnCallback
    (void *)&g_sCDCDevice,          // Callback data is our device pointer.
    USBDCDCPacketRead,              // pfnTransfer
    USBDCDCRxPacketAvailable,       // pfnAvailable
    (void *)&g_sCDCDevice,          // pvHandle
    g_pucUSBRxBuffer,               // pcBuffer
    UART_BUFFER_SIZE,               // ulBufferSize
    g_pucRxBufferWorkspace          // pvWorkspace
};

//*****************************************************************************
//
// Transmit buffer (from the USB perspective).
//
//*****************************************************************************
static unsigned char g_pucUSBTxBuffer[UART_BUFFER_SIZE];
static unsigned char g_pucTxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
static const tUSBBuffer g_sTxBuffer =
{
    true,                           // This is a transmit buffer.
    TxHandler,                      // pfnCallback
    (void *)&g_sCDCDevice,          // Callback data is our device pointer.
    USBDCDCPacketWrite,             // pfnTransfer
    USBDCDCTxPacketAvailable,       // pfnAvailable
    (void *)&g_sCDCDevice,          // pvHandle
    g_pucUSBTxBuffer,               // pcBuffer
    UART_BUFFER_SIZE,               // ulBufferSize
    g_pucTxBufferWorkspace          // pvWorkspace
};

//*****************************************************************************
//
// A boolean that is true if there is a USB connection.
//
//*****************************************************************************
static tBoolean g_bUSBConnected = false;

//*****************************************************************************
//
// Get the communication parameters in use on the UART.
//
//*****************************************************************************
static void
GetLineCoding(tLineCoding *psLineCoding)
{
    //
    // Always report 115,200, 8-N-1.
    //
    psLineCoding->ulRate = 115200;
    psLineCoding->ucDatabits = 8;
    psLineCoding->ucParity = USB_CDC_PARITY_NONE;
    psLineCoding->ucStop = USB_CDC_STOP_BITS_1;

    //
    // Send the information back to the host.
    //
    USBDCDSendDataEP0(0, (unsigned char *)psLineCoding, sizeof(tLineCoding));
}

//*****************************************************************************
//
// Handles CDC driver notifications related to control and setup of the device.
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the notification event.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to perform control-related
// operations on behalf of the USB host.  These functions include setting
// and querying the serial communication parameters, setting handshake line
// states and sending break conditions.
//
// \return The return value is event-specific.
//
//*****************************************************************************
static unsigned long
ControlHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
               void *pvMsgData)
{
    //
    // Which event was sent?
    //
    switch(ulEvent)
    {
        //
        // The host has connected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // Indicate that the USB is connected.
            //
            g_bUSBConnected = true;

            //
            // Flush the buffers.
            //
            USBBufferFlush(&g_sTxBuffer);
            USBBufferFlush(&g_sRxBuffer);

            break;
        }

        //
        // The host has disconnected.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // If USB is currently being used to play the game, then halt the
            // Z-machine interpreter.
            //
            if(g_ulGameIF == GAME_IF_USB)
            {
                halt = 1;
            }

            //
            // Indicate that the USB is no longer connected.
            //
            g_bUSBConnected = false;

            break;
        }

        //
        // Return the current serial communication parameters.
        //
        case USBD_CDC_EVENT_GET_LINE_CODING:
        {
            GetLineCoding(pvMsgData);
            break;
        }

        //
        // Set the current serial communication parameters.
        //
        case USBD_CDC_EVENT_SET_LINE_CODING:
        {
            break;
        }

        //
        // Set the current serial communication parameters.
        //
        case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
        {
            break;
        }

        //
        // Send a break condition on the serial line.
        //
        case USBD_CDC_EVENT_SEND_BREAK:
        {
            break;
        }

        //
        // Clear the break condition on the serial line.
        //
        case USBD_CDC_EVENT_CLEAR_BREAK:
        {
            break;
        }

        //
        // Ignore SUSPEND and RESUME for now.
        //
        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
        {
            break;
        }

        //
        // Other events can be safely ignored.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param ulCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the notification event.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
static unsigned long
TxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
          void *pvMsgData)
{
    //
    // Which event was sent?
    //
    switch(ulEvent)
    {
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // There is nothing to do here since it is handled by the
            // USBBuffer.
            //
            break;
        }

        //
        // Other events can be safely ignored.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param ulCBData is the client-supplied callback data value for this channel.
// \param ulEvent identifies the notification event.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
static unsigned long
RxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
          void *pvMsgData)
{
    //
    // Which event was sent?
    //
    switch(ulEvent)
    {
        //
        // A new packet has been received.
        //
        case USB_EVENT_RX_AVAILABLE:
        {
            //
            // Since data has been received, see if the game is already being
            // played.
            //
            if(g_ulGameIF == GAME_IF_NONE)
            {
                //
                // The game is not being played, so select the USB interface.
                //
                g_ulGameIF = GAME_IF_USB;

                //
                // Read and discard the character used to activate the game.
                //
                USBBufferRead(&g_sRxBuffer, (unsigned char *)&ulEvent, 1);
            }

            //
            // Otherwise, see if the game is being played on an interface other
            // than USB.
            //
            else if(g_ulGameIF != GAME_IF_USB)
            {
                //
                // Write the error message to the USB interface.
                //
                USBBufferWrite(&g_sTxBuffer, g_pucErrorMessage,
                               sizeof(g_pucErrorMessage));

                //
                // Read and discard all data that was received over USB.
                //
                while(USBBufferDataAvailable(&g_sRxBuffer) != 0)
                {
                    USBBufferRead(&g_sRxBuffer, (unsigned char *)&ulEvent, 1);
                }
            }

            break;
        }

        //
        // This is a request for how much unprocessed data is still waiting to
        // be processed.  Return 0 if the UART is currently idle or 1 if it is
        // in the process of transmitting something.  The actual number of
        // bytes in the UART FIFO is not important here, merely whether or
        // not everything previously sent to us has been transmitted.
        //
        case USB_EVENT_DATA_REMAINING:
        {
            //
            // Get the number of bytes in the buffer and add 1 if some data
            // still has to clear the transmitter.
            //
            return(0);
        }

        //
        // This is a request for a buffer into which the next packet can be
        // read.  This mode of receiving data is not supported so let the
        // driver know by returning 0.  The CDC driver should not be sending
        // this message but this is included just for illustration and
        // completeness.
        //
        case USB_EVENT_REQUEST_BUFFER:
        {
            return(0);
        }

        //
        // Other events can be safely ignored.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// Initializes the USB interface.
//
//*****************************************************************************
void
USBIFInit(void)
{
    //
    // Initialize the transmit and receive buffers.
    //
    USBBufferInit(&g_sTxBuffer);
    USBBufferInit(&g_sRxBuffer);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDCDCInit(0, &g_sCDCDevice);
}

//*****************************************************************************
//
// Reads a character from the USB interface.
//
//*****************************************************************************
unsigned char
USBIFRead(void)
{
    unsigned char ucChar;

    //
    // Return a NUL if the USB is not connected or there is no data in the
    // receive buffer.
    //
    if(!g_bUSBConnected || (USBBufferDataAvailable(&g_sRxBuffer) == 0))
    {
        return(0);
    }

    //
    // Read the next byte from the receive buffer.
    //
    USBBufferRead(&g_sRxBuffer, &ucChar, 1);

    //
    // Return the byte that was read.
    //
    return(ucChar);
}

//*****************************************************************************
//
// Writes a character to the USB interface.
//
//*****************************************************************************
void
USBIFWrite(unsigned char ucChar)
{
    //
    // Delay until there is some space in the output buffer.
    //
    while(g_bUSBConnected)
    {
        if(USBBufferSpaceAvailable(&g_sTxBuffer) != 0)
        {
            break;
        }
    }

    //
    // If there is still a USB connection, write this character into the output
    // buffer.
    //
    if(g_bUSBConnected)
    {
        USBBufferWrite(&g_sTxBuffer, &ucChar, 1);
    }
}
