//*****************************************************************************
//
// usb_device.c - Functions handling USB device operation for the
//                Quickstart Oscilloscope.
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
// This is part of revision 8555 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/usb.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "utils/uartstdio.h"
#include "data-acq.h"
#include "qs-scope.h"
#include "renderer.h"
#include "usbdescriptors.h"
#include "usbhw.h"
#include "usb_host.h"
#include "usb_protocol.h"

//*****************************************************************************
//
// This enumeration holds the various states that the endpoints can be in
// during normal operation.
//
//*****************************************************************************
typedef enum
{
    //
    // Unconfigured.
    //
    USBDEV_STATE_UNCONFIGURED,

    //
    // No outstanding transaction remains to be completed.
    //
    USBDEV_STATE_IDLE,

    //
    // Waiting on completion of a send or receive transaction.  For the OUT
    // endpoint, this indicates that the device has not processed the last
    // packet and another is waiting to be read and ACKed in the FIFO.  For
    // the IN endpoint, this means we have sent a packet but have not yet
    // received the ACK for it.
    //
    USBDEV_STATE_WAIT_DATA
}
tUSBEndpointState;

volatile tUSBEndpointState g_eDataInState;
volatile tUSBEndpointState g_eDataOutState;

//*****************************************************************************
//
// This enumeration holds the various states that the device can be in during
// normal operation.
//
//*****************************************************************************
typedef enum
{
    //
    // The device is disconnected from the host.
    //
    DEVICE_DISCONNECTED,

    //
    // The device is connected to the host but has not received a HOST_HELLO
    // packet to initiate communication.
    //
    DEVICE_WAITING_FOR_HELLO,

    //
    // The device has successfully received a HOST_HELLO packet and responded
    // with HELLO_RESPONSE.  Normal communication is ongoing.
    //
    DEVICE_COMMUNICATING
}
tUSBDeviceState;

tUSBDeviceState g_eDeviceState;
tBoolean g_bDataEnabled;
volatile tBoolean g_bBlockSend;

//*****************************************************************************
//
// The number of SysTick ticks we will wait when trying to send data before
// we declare a timeout and give up.  This equates to 0.5 seconds.
//
//*****************************************************************************
#define USB_DATA_TIMEOUT_TICKS  (SYSTICKS_PER_SECOND / 2)

//*****************************************************************************
//
// Buffers used to hold data received from the host and a global holding the
// number of bytes of data in the receive (OUT) buffer.
//
//*****************************************************************************
static unsigned char g_pucDataOutBuffer[DATA_IN_EP_MAX_SIZE];
static unsigned long g_ulDataOutSize;

//*****************************************************************************
//
// Macros holding the maximum number of tScopeDataElement and
// tScopeDualDataElement entries that can be transmitted in a
// single SCOPE_PKT_DATA packet.
//
//*****************************************************************************
#define SINGLE_ELEMENTS_PER_PACKET                          \
                                ((DATA_IN_EP_MAX_SIZE -     \
                                  sizeof(tScopePacket)) /   \
                                 sizeof(tScopeDataElement))
#define DUAL_ELEMENTS_PER_PACKET                                \
                                ((DATA_IN_EP_MAX_SIZE -         \
                                  sizeof(tScopePacket)) /       \
                                 sizeof(tScopeDualDataElement))

//*****************************************************************************
//
// Global flag indicating that a USB configuration has been set.
//
//*****************************************************************************
static volatile unsigned long g_ulUSBConfigured;

//*****************************************************************************
//
// Global flags used to indicate to the transmit path that a response to a
// PING or HELLO from the host is required.
//
//*****************************************************************************
static tBoolean g_bSendHelloResponse;
static tBoolean g_bSendPingResponse;
static unsigned char g_ucPingEcho1;
static unsigned long g_ulPingEcho2;

//*****************************************************************************
//
// Internal function prototypes.
//
//*****************************************************************************
static void ProcessDataFromHost(unsigned long ulStatus);
static void ProcessDataToHost(unsigned long ulStatus);
static tBoolean SendUSBPacket(tScopePacket *psHeader, unsigned char *pucData,
                              unsigned long ulLength);
static void CleanUpOnDisconnect(tBoolean bInformUser);
static tBoolean WaitForUSBSendIdle(unsigned long ulTimeout);

//*****************************************************************************
//
// Send the SCOPE_PKT_HELLO_RESPONSE packet and current settings back to the
// USB host.
//
//*****************************************************************************
static void
SendHelloResponse(void)
{
    tScopePacket sPacket;
    tScopeSettings sSettings;
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usLevel;
    tBoolean bTriggerCh1;
    tBoolean bRetcode;

    //
    // Are we free to send the data just now?
    //
    if((g_eDataInState == USBDEV_STATE_IDLE) && !g_bBlockSend)
    {
        sPacket.ucVersion = SCOPE_PROTOCOL_VERSION_1;
        sPacket.ucHdrLength = sizeof(tScopePacket);
        sPacket.ucPacketType = SCOPE_PKT_HELLO_RESPONSE;
        sPacket.ucParam = 0;
        sPacket.ulParam = 0;
        sPacket.ulDataLength = sizeof(tScopeSettings);

        //
        // The payload of the HELLO_RESPONSE packet contains the current
        // application settings that are relevant to allow the host to
        // correctly configure its user interface.
        //
        sSettings.ulTriggerLevelmV = g_sRender.lTriggerLevelmV;
        sSettings.ulTimebaseuS = g_sRender.uluSPerDivision;
        sSettings.lTriggerPos = g_sRender.lHorizontalOffset;
        sSettings.sChannel1OffsetmV = g_sRender.lVerticalOffsetmV[CHANNEL_1];
        sSettings.sChannel2OffsetmV = g_sRender.lVerticalOffsetmV[CHANNEL_2];
        sSettings.usChannel1ScalemVdiv = g_sRender.ulmVPerDivision[CHANNEL_1];
        sSettings.usChannel2ScalemVdiv = g_sRender.ulmVPerDivision[CHANNEL_2];
        sSettings.ucChannel2Enabled = g_pbActiveChannels[CHANNEL_2];
        sSettings.ucStarted = g_bContinuousCapture;

        //
        // Get the trigger type.  We make the (currently valid) assumption that
        // the members of tTriggerType align with the SCOPE_TRIGGER_TYPE_XXX
        // definitions from usb_protocol.h.
        //
        DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usLevel);
        bTriggerCh1 = DataAcquisitionGetTriggerChannel();

        sSettings.ucTriggerType = (unsigned char)eType;
        sSettings.ucTriggerChannel = bTriggerCh1 ? SCOPE_CHANNEL_1 :
                                                   SCOPE_CHANNEL_2;

        //
        // Write the packet data to the USB endpoint FIFO.
        //
        bRetcode = SendUSBPacket(&sPacket, (unsigned char *)&sSettings,
                                 sizeof(tScopeSettings));
        if(bRetcode)
        {
            //
            // We sent the response so turn off the flag telling us to do this.
            //
            g_bSendHelloResponse = false;

            //
            // Update our device state to indicate that normal communication
            // is now possible.
            //
            g_eDeviceState = DEVICE_COMMUNICATING;

            //
            // Inform the main loop that the host is disconnecting.
            //
            COMMAND_FLAG_WRITE(SCOPE_USB_HOST_CONNECT, 0);
        }
    }
    else
    {
        //
        // We're waiting for a previous packet to be sent so can't send the
        // response right now.  Set a flag to ourselves telling us to send
        // this response next time the transmitter is available.
        //
        g_bSendHelloResponse = true;
    }
}

//*****************************************************************************
//
// Send the SCOPE_PKT_PING_RESPONSE packet back to the USB host.
//
//*****************************************************************************
static void
SendPingResponse(unsigned char ucEcho1, unsigned long ulEcho2)
{
    tScopePacket sPacket;

    //
    // If no outstanding transmission is pending, go ahead and send the
    // response immediately.
    //
    if((g_eDataInState == USBDEV_STATE_IDLE) && !g_bBlockSend)
    {
        sPacket.ucVersion = SCOPE_PROTOCOL_VERSION_1;
        sPacket.ucHdrLength = sizeof(tScopePacket);
        sPacket.ucPacketType = SCOPE_PKT_PING_RESPONSE;
        sPacket.ucParam = ucEcho1;
        sPacket.ulParam = ulEcho2;
        sPacket.ulDataLength = 0;

        //
        // Mark the fact that we don't have a ping response pending any
        // more.
        //
        g_bSendPingResponse = false;

        //
        // Write the packet data to the USB endpoint FIFO.
        //
        SendUSBPacket(&sPacket, (unsigned char *)0, 0);
    }
    else
    {
        //
        // We're waiting for a previous packet to be sent so can't send the
        // response right now.  Flag this so that we send the response later.
        //
        g_bSendPingResponse = true;
        g_ucPingEcho1 = ucEcho1;
        g_ulPingEcho2 = ulEcho2;
  }
}

//*****************************************************************************
//
// Handles USB packets received from the host.
//
// \param pcPacket points to the first byte of a received packet.
// \param ulSize is the size in bytes of the packet pointed to by \e pcPacket.
//
// This function is called in interrupt context whenever a new packet has
// been read from the USB host.  Here we parse the packet and pass on any
// orders it contains to the main loop for later action.
//
// In the current version of the protocol, all transactions from the host
// are contained in a single USB packet with no additional data so we
// don't need to worry about variable sized data here or transactions that
// span more than one packet.
//
// \return None.
//
//*****************************************************************************
void
ProcessUSBPacket(unsigned char *pcPacket, unsigned long ulSize)
{
    tScopePacket *psPacket;

    //
    // Get a pointer to our packet header structure.
    //
    psPacket = (tScopePacket *)pcPacket;

    //
    // Check the validity of the packet we just received.
    //
    if(psPacket->ucVersion != SCOPE_PROTOCOL_VERSION_1)
    {
        //
        // Someone is asking for a protocol later than we support or
        // is sending us bogus packets.  Check to see if this is a
        // a HOST_HELLO packet to determine which of these is the case.
        //
        if(!((psPacket->ucPacketType == SCOPE_PKT_HOST_HELLO) &&
           (ulSize == psPacket->ucHdrLength)))
        {
            //
            // This is not a HOST_HELLO packet with the expected length
            // so we assume it's either invalid or something we can't handle
            // and merely ignore it.
            //
            UARTprintf("Unexpected USB packet\n");
            return;
        }
        else
        {
            //
            // This is something other than a HOST_HELLO packet.  We don't
            // support this level of the protocol and the host hasn't
            // sent us the HELLO packet so we ignore this completely.
            UARTprintf("USB packet ignored\n");
            return;
        }
    }
    else
    {
        //
        // Seems to be a packet using the protocol version we support so
        // all seems well so far.  Check that the header length is correct
        // for this protocol version.
        //
        if(psPacket->ucHdrLength != sizeof(tScopePacket))
        {
            //
            // Header length is wrong so ignore the packet.
            //
            UARTprintf("USB header error - wrong length\n");
            return;
        }
    }

    //
    // Parse the packet and send the appropriate command to the main loop.
    //
    switch(psPacket->ucPacketType)
    {
        case SCOPE_PKT_HOST_HELLO:
        {
            //
            // We respond with a HELLO_RESPONSE packet.  Don't update the state
            // until this is actually sent, though.
            //
            SendHelloResponse();
            break;
        }

        case SCOPE_PKT_HOST_GOODBYE:
        {
            //
            // The host application is closing down so go back to the state
            // where we are waiting for a HOST_HELLO packet.
            //
            g_eDeviceState = DEVICE_WAITING_FOR_HELLO;

            //
            // Inform the main loop that the host is disconnecting.
            //
            COMMAND_FLAG_WRITE(SCOPE_USB_HOST_REMOVE, 0);
            break;
        }

        case SCOPE_PKT_PING:
        {
            //
            // We respond with a PING_RESPONSE packet.
            //
            SendPingResponse(psPacket->ucParam, psPacket->ulParam);
            break;
        }

        case SCOPE_PKT_CAPTURE:
        {
            //
            // Send a command to the main loop asking to capture a frame.
            //
            COMMAND_FLAG_WRITE(SCOPE_CAPTURE, 0);
            break;
        }

        case SCOPE_PKT_START:
        {
            //
            // Send a command to the main loop to start automatic capture.
            //
            COMMAND_FLAG_WRITE(SCOPE_START, 0);
            break;
        }

        case SCOPE_PKT_STOP:
        {
            //
            // Send a command to the main loop to start automatic capture.
            //
            COMMAND_FLAG_WRITE(SCOPE_STOP, 0);
            break;
        }

        case SCOPE_PKT_SET_TIMEBASE:
        {
            //
            // Send a command to the main loop to set the capture timebase.
            //
            COMMAND_FLAG_WRITE(SCOPE_CHANGE_TIMEBASE, psPacket->ulParam);
            break;
        }

        case SCOPE_PKT_SET_TRIGGER_TYPE:
        {
            //
            // Send commands to the main loop to set the trigger type and
            // channel.
            //
            COMMAND_FLAG_WRITE(SCOPE_CHANGE_TRIGGER, psPacket->ulParam);
            COMMAND_FLAG_WRITE(SCOPE_SET_TRIGGER_CH,
                               ((psPacket->ucParam == SCOPE_CHANNEL_1) ?
                                CHANNEL_1 : CHANNEL_2));
            break;
        }

        case SCOPE_PKT_SET_TRIGGER_LEVEL:
        {
            //
            // Send a command to the main loop to set the trigger level.
            //
            COMMAND_FLAG_WRITE(SCOPE_TRIGGER_LEVEL, psPacket->ulParam);
            break;
        }

        case SCOPE_PKT_SET_TRIGGER_POS:
        {
            //
            // Send a command to the main loop to set the trigger position.
            //
            COMMAND_FLAG_WRITE(SCOPE_TRIGGER_POS, psPacket->ulParam);
            break;
        }

        case SCOPE_PKT_SET_CHANNEL2:
        {
            //
            // Send a command to the main loop to enable or disable channel
            // 2 as required.
            //
            COMMAND_FLAG_WRITE(SCOPE_CH2_DISPLAY,
                               (unsigned long)psPacket->ucParam);
            break;
        }

        case SCOPE_PKT_RETRANSMIT:
        {
            //
            // Send a command to the main loop to have it retransmit the latest
            // captured waveform data.
            //
            COMMAND_FLAG_WRITE(SCOPE_RETRANSMIT, 0);
            break;
        }

        case SCOPE_PKT_DATA_CONTROL:
        {
            //
            // Set or clear the flag we use to gate data transmission to '
            // the host.
            //
            g_bDataEnabled = (tBoolean)psPacket->ucParam;
            break;
        }

        case SCOPE_PKT_FIND:
        {
            //
            // Send a command to the main loop to have it rescale and
            // reposition the appropriate waveform.
            //
            COMMAND_FLAG_WRITE(SCOPE_FIND,
                               ((psPacket->ucParam == SCOPE_CHANNEL_1) ?
                                  CHANNEL_1 : CHANNEL_2));
            break;
        }

        case SCOPE_PKT_SET_POSITION:
        {
            //
            // Send a command to the main loop to have it reposition the
            // appropriate waveform.
            //
            COMMAND_FLAG_WRITE(((psPacket->ucParam == SCOPE_CHANNEL_1) ?
                                  SCOPE_CH1_POS : SCOPE_CH2_POS),
                               psPacket->ulParam);
            break;
        }

        case SCOPE_PKT_SET_SCALE:
        {
            //
            // Send a command to the main loop to have it rescale the
            // appropriate waveform.
            //
            COMMAND_FLAG_WRITE(((psPacket->ucParam == SCOPE_CHANNEL_1) ?
                                  SCOPE_CH1_SCALE : SCOPE_CH2_SCALE),
                               psPacket->ulParam);
            break;
        }

        //
        // Handle all unrecognized packets.
        //
        default:
        {
            //
            // We received an unrecognized packet.  Ignore it.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// This function is called from HandleEndpoints for all interrupts signaling
// the arrival of data on the bulk OUT endpoint (in other words, whenever
// the host has sent us data).  We read the data into our local buffer assuming
// it has already been emptied and ACK the data.  If the buffer is not yet
// available, we merely return without doing anything.  The USB hardware will
// continue to NACK the data until we later read the new packet from the
// FIFO and ACK it.
//
// NOTE: This function must not block since it is called from interrupt
// context.
//
//*****************************************************************************
static void
ProcessDataFromHost(unsigned long ulStatus)
{
    unsigned long ulCount;
    int iRetcode;

    switch(g_eDataOutState)
    {
        case USBDEV_STATE_UNCONFIGURED:
            //
            // Weird - we got a packet before the configuration was set.
            // Just handle it as normal anyway.
            //
        case USBDEV_STATE_IDLE:
        {
            //
            // Read the packet we got into the local buffer and keep track of
            // its size.
            //
            ulCount = DATA_OUT_EP_MAX_SIZE;
            iRetcode =  USBEndpointDataGet(USB0_BASE, DATA_OUT_ENDPOINT,
                                           g_pucDataOutBuffer,
                                           &ulCount);
            //
            // Was there a packet to read?
            //
            if(iRetcode != -1)
            {
                g_ulDataOutSize = ulCount;

                //
                //
                // ACK the packet.  This tells the host that it is free to send
                // the next one.
                //
                USBDevEndpointDataAck(USB0_BASE, DATA_OUT_ENDPOINT, true);

                //
                // Parse the packet contents and send appropriate commands to
                // the main loop.
                //
                ProcessUSBPacket(g_pucDataOutBuffer, g_ulDataOutSize);
            }

            //
            // We have handled the packet so jump back to idle state and wait
            // for the next one.
            //
            g_eDataOutState = USBDEV_STATE_IDLE;

            break;
        }

        case USBDEV_STATE_WAIT_DATA:
        {
            //
            // We shouldn't get here.  This implies that we received a second
            // packet while we still had one in the FIFO which is not ACKed.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// This function is called from HandleEndpoints for all interrupts originating
// from the bulk IN endpoint (in other words, whenever data has been
// transmitted to the USB host).
//
//*****************************************************************************
static void
ProcessDataToHost(unsigned long ulStatus)
{
    if(ulStatus)
    {
        //
        // Clear the status bits.
        //
        USBDevEndpointStatusClear(USB0_BASE, DATA_IN_ENDPOINT, ulStatus);
    }

    //
    // Our IN endpoint is now idle.
    //
    g_eDataInState = USBDEV_STATE_IDLE;

    //
    // Our last transmission completed.  Do we need to send any further data
    // or are we finished?
    //
    if(g_bSendHelloResponse)
    {
        //
        // We need to send a HELLO response packet so do this now.
        //
        SendHelloResponse();
    }
    else
    {
        //
        // Do we have a pending PING_RESPONSE to send?
        //
        if(g_bSendPingResponse)
        {
            SendPingResponse(g_ucPingEcho1, g_ulPingEcho2);
        }
    }
}

//*****************************************************************************
//
// This function sends a single packet with optional data block to the USB
// host.
//
// \note This rather simplified function will not allow more data to be sent
// than there is space in the USB FIFO for the data IN endpoint.
//
// \return Returns \b true if the packet was scheduled to be sent or \b false
// if an error occurred.
//
//*****************************************************************************
static tBoolean
SendUSBPacket(tScopePacket *psHeader, unsigned char *pucData,
              unsigned long ulLength)
{
    int iRetcode;

    //
    // Put the packet header into the transmit FIFO.
    //
    iRetcode = USBEndpointDataPut(USB0_BASE, DATA_IN_ENDPOINT,
                                  (unsigned char *)psHeader,
                                  sizeof(tScopePacket));
    if(iRetcode == 0)
    {
        //
        // If the packet has optional data, send this too.
        //
        if(ulLength != 0)
        {
            iRetcode = USBEndpointDataPut(USB0_BASE, DATA_IN_ENDPOINT,
                                          pucData, ulLength);
        }

        //
        // Did we manage to put all the data into the USB FIFO?
        //
        if(iRetcode == 0)
        {
            //
            // We successfully put the data in the FIFO so now initiate the
            // transmission.
            //
            g_eDataInState = USBDEV_STATE_WAIT_DATA;
            iRetcode = USBEndpointDataSend(USB0_BASE, DATA_IN_ENDPOINT,
                                           USB_TRANS_IN);
            if(iRetcode != 0)
            {
                //
                // USBEndpointDataSend failed!
                //
                return(false);
            }
        }
        else
        {
            //
            // USBEndpointDataPut for the payload failed!
            //
            return(false);
        }
    }
    else
    {
        //
        // USBEndpointDataPut for the header failed!
        //
        return(false);
    }

    //
    // If we get here, things must have gone well
    //
    return(true);
}

//*****************************************************************************
//
// Initializes the USB stack for the oscilloscope device.
//
//*****************************************************************************
tBoolean
ScopeUSBDeviceInit(void)
{
    //
    // Not configured initially.
    //
    g_ulUSBConfigured = 0;

    //
    // Configure the USB mux on the board to put us in device mode.  We pull
    // the relevant pin high to do this.
    //
    SysCtlPeripheralEnable(USB_MUX_GPIO_PERIPH);
    GPIOPinTypeGPIOOutput(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN);
    GPIOPinWrite(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN, USB_MUX_SEL_DEVICE);

    //
    // Tell the stack that we will be operating as a device rather than a
    // host.  This calls back to ScopeUSBModeCallback where we perform final
    // initialization.
    //
    USBStackModeSet(0, USB_MODE_DEVICE, ScopeUSBModeCallback);

    //
    // Enter the idle state.
    //
    CleanUpOnDisconnect(false);

    //
    // Pass our device information to the USB library.
    //
    USBDCDInit(0, &g_sScopeDeviceInfo);

    return(true);
}

//*****************************************************************************
//
// Removes the oscilloscope USB device from the bus.
//
//*****************************************************************************
void
ScopeUSBDeviceTerm(void)
{
    //
    // Remember that we are no longer configured.
    //
    g_ulUSBConfigured = 0;

    //
    // Tell the USB library that we are no longer using the controller.
    //
    USBDCDTerm(0);
}

//*****************************************************************************
//
// This public function sends a single packet to the host and returns once
// the packet has been acknowledged.
//
// \param ucPacketType is the type of packet to send.
// \param ucParam is the first packet parameter value.
// \param ulParam is the second packet parameter value.
//
// \note This rather simplified function will not allow more data to be sent
// than there is space in the USB FIFO for the data IN endpoint.
//
// \return Returns \b true if the packet was sent successfully or \b false
// if an error occurred or we are not connected to the host.
//
//*****************************************************************************
tBoolean
ScopeUSBDeviceSendPacket(unsigned char ucPacketType, unsigned char ucParam,
                         unsigned long ulParam)
{
    tBoolean bRetcode;
    tScopePacket sPacket;

    if(g_eDeviceState == DEVICE_COMMUNICATING)
    {
        //
        // Set the flag which prevents any automatic packet transmission
        // from interrupt context while we are trying to send a packet
        // from this context.
        //
        g_bBlockSend = true;

        //
        // Wait until it is safe to send the packet.
        //
        bRetcode = WaitForUSBSendIdle(USB_DATA_TIMEOUT_TICKS);

        //
        // If we didn't time out, go ahead and send the packet.
        //
        if(bRetcode)
        {
            //
            // Fill in the packet header structure.
            //
            sPacket.ucVersion = SCOPE_PROTOCOL_VERSION_1;
            sPacket.ucHdrLength = sizeof(tScopePacket);
            sPacket.ucPacketType = ucPacketType;
            sPacket.ucParam = ucParam;
            sPacket.ulParam = ulParam;
            sPacket.ulDataLength = 0;

            //
            // ...and send it back to the host.
            //
            bRetcode = SendUSBPacket(&sPacket, 0, 0);

            //
            // If we scheduled the packet for transmission successfully,
            // wait until it has been sent.
            //
            if(bRetcode)
            {
                bRetcode = WaitForUSBSendIdle(USB_DATA_TIMEOUT_TICKS);
            }
        }
    }
    else
    {
        //
        // We are not connected to the host so discard the packet.
        //
        bRetcode = false;
    }

    //
    // Release the transmission block flag.
    //
    g_bBlockSend = false;

    return(bRetcode);
}

//*****************************************************************************
//
// This function should be called in all error conditions which cause us to
// believe that the host disconnected or which cause us to need to
// synchronize with the host.  We clean up all state and revert to
// the original disconnected state where we wait for a SCOPE_PKT_HOST_HELLO
// and ignore all other incoming packets.  The host should recognize that it is
// no longer being communicated with and initiate communications after this.
//
//*****************************************************************************
static void
CleanUpOnDisconnect(tBoolean bInformUser)
{
    //
    // Get back to the default state.  We are not in communication, both
    // endpoints are idle and we are not automatically sending data back to
    // the host.
    //
    UARTprintf("USB host disconnected\n");
    g_eDataInState = USBDEV_STATE_IDLE;
    g_eDataOutState = USBDEV_STATE_IDLE;
    g_eDeviceState = DEVICE_DISCONNECTED;
    g_bDataEnabled = false;
    g_bBlockSend = false;

    //
    // Inform the main loop that the host is disconnecting.
    //
    if(bInformUser)
    {
        COMMAND_FLAG_WRITE(SCOPE_USB_HOST_REMOVE, 0);
    }
}

//*****************************************************************************
//
// This function will poll until either the USB data IN endpoint is idle or a
// timeout occurs.  The ulTimeout parameter provides the required timeout
// expressed in a number of system ticks.
//
//*****************************************************************************
static tBoolean
WaitForUSBSendIdle(unsigned long ulTimeout)
{
    unsigned long ulTimeEnd;

    //
    // What will the time be when we time out?
    //
    ulTimeEnd = g_ulSysTickCounter + ulTimeout;

    //
    // Wait for the USB endpoint to become free.
    //
    while((g_eDataInState != USBDEV_STATE_IDLE) &&
          (g_ulSysTickCounter != ulTimeEnd))
    {
        //
        // Wait around...
        //
    }

    //
    // Did we drop out of the loop because the endpoint is idle or
    // due to a timeout? Return true if the endpoint is now idle
    // or false if not (since this implies we timed out).
    //
    return(g_eDataInState == USBDEV_STATE_IDLE);
}

//*****************************************************************************
//
// This function is called by the main loop to transmit captured waveform
// data to the USB host (if connected).  The function blocks until the complete
// data set has been transmitted and acknowledged by the host.
//
// If the host is not currently connected, this function returns immediately
// indicating an error.
//
// If automatic data transmission is currently enabled, the call is ignored
// unless the \e bAuto parameter is \b false indicating that the call is the
// result of a specific request for data from the host.
//
// Note: This function must not be called from interrupt context since it
// blocks until all data is sent or a timeout occurs.
//
// \return Returns \b true on success or \b false on error.
//
//*****************************************************************************
tBoolean
ScopeUSBDeviceSendData(tDataAcqCaptureStatus *pCapInfo, tBoolean bAuto)
{
    tScopeDataStart sStartInfo;
    tScopePacket sPacket;
    tBoolean bRetcode;
    unsigned char ucPacketCount;
    unsigned long ulSampleCount;
    unsigned long ulSampleIndex;
    unsigned long ulMaxSample;
    unsigned long ulLoop;
    int iRetcode;

    //
    // We only send data if the device is connected to a USB host
    // and the host has either requested the data or enabled automatic
    // data transmission.
    //
    if((g_eDeviceState == DEVICE_COMMUNICATING) &&
       (!bAuto || (bAuto && g_bDataEnabled)))
    {
        //
        // Set the flag that signals the interrupt context code not to
        // try to send anything immediately.
        //
        g_bBlockSend = true;

        //
        // Wait for the data IN endpoint to complete sending
        // the packet.
        //
        bRetcode = WaitForUSBSendIdle(USB_DATA_TIMEOUT_TICKS);

        //
        // If we didn't time out, go ahead and send the data
        //
        if(bRetcode)
        {
            //
            // The endpoint is idle so send our data.  We first send a
            // SCOPE_PKT_DATA_START packet.
            //
            sStartInfo.ulSampleOffsetuS = pCapInfo->ulSampleOffsetuS;
            sStartInfo.ulSamplePerioduS = pCapInfo->ulSamplePerioduS;
            sStartInfo.bDualChannel = pCapInfo->bDualMode;
            sStartInfo.bCh2SampleFirst = pCapInfo->bBSampleFirst;
            sStartInfo.ulTotalElements = pCapInfo->bDualMode ?
                              (pCapInfo->ulMaxSamples / 2) :
                              pCapInfo->ulMaxSamples;

            //
            // Map the trigger index in the ring buffer to an element
            // index in the data we will be sending back to the USB host.
            //
            sStartInfo.ulTriggerIndex = DISTANCE_FROM_START(
                                              pCapInfo->ulStartIndex,
                                              pCapInfo->ulTriggerIndex,
                                              pCapInfo->ulMaxSamples);
            if(pCapInfo->bDualMode)
            {
                sStartInfo.ulTriggerIndex /= 2;
            }

            sPacket.ucVersion = SCOPE_PROTOCOL_VERSION_1;
            sPacket.ucHdrLength = sizeof(tScopePacket);
            sPacket.ucPacketType = SCOPE_PKT_DATA_START;
            sPacket.ucParam = 0;
            sPacket.ulParam = sStartInfo.ulTotalElements;
            sPacket.ulDataLength = sizeof(tScopeDataStart);
            bRetcode = SendUSBPacket(&sPacket, (unsigned char *)&sStartInfo,
                                     sizeof(tScopeDataStart));

            //
            // If we can't send the packet for some reason,
            // return an error.
            //
            if(!bRetcode)
            {
                CleanUpOnDisconnect(true);
                return(false);
            }

            //
            // Now we send the actual data samples in multiple
            // SCOPE_PKT_DATA packets.
            //
            ulSampleCount = 0;
            ulSampleIndex = pCapInfo->ulStartIndex;
            ucPacketCount = 1;
            ulMaxSample = sPacket.ulParam;
            sPacket.ucPacketType = SCOPE_PKT_DATA;

            while(ulSampleCount < ulMaxSample)
            {
                //
                // Wait for the data IN endpoint to complete sending
                // the previous packet.
                //
                bRetcode = WaitForUSBSendIdle(USB_DATA_TIMEOUT_TICKS);

                //
                // Immediately mark the endpoint as waiting for data.  This is
                // intended to reduce the likelihood of an incoming PING or
                // HELLO packet from causing our interrupt service routine to
                // write to the USB endpoint FIFO while this function is
                // running.  There is, admittedly, still a tiny timing hole
                // here (if a USB interrupt for a PING packet is serviced
                // between WaitForUSBSendIdle and the next line, we have a
                // problem).
                //
                g_eDataInState = USBDEV_STATE_WAIT_DATA;

                if(!bRetcode)
                {
                    CleanUpOnDisconnect(true);
                    return(false);
                }

                //
                // Update the header for the new packet.
                //
                sPacket.ucParam = ucPacketCount++;

                //
                // How many samples will we send in this packet?
                //
                if(pCapInfo->bDualMode)
                {
                    sPacket.ulParam = min(DUAL_ELEMENTS_PER_PACKET,
                                          (ulMaxSample - ulSampleCount));
                    sPacket.ulDataLength = (sPacket.ulParam *
                                            sizeof(tScopeDualDataElement));
                }
                else
                {
                    sPacket.ulParam = min(SINGLE_ELEMENTS_PER_PACKET,
                                          (ulMaxSample - ulSampleCount));
                    sPacket.ulDataLength = (sPacket.ulParam *
                                            sizeof(tScopeDataElement));
                }

                //
                // Put the packet header into the transmit FIFO.
                //
                USBEndpointDataPut(USB0_BASE, DATA_IN_ENDPOINT,
                                   (unsigned char *)&sPacket,
                                    sizeof(tScopePacket));

                //
                // Now write the sample data for this packet.
                //
                for(ulLoop = 0; ulLoop < sPacket.ulParam; ulLoop++)
                {
                    //
                    // Are we dealing with single or dual channel data?
                    //
                    if(pCapInfo->bDualMode)
                    {
                        //
                        // Dual channel - send two samples per element.
                        //
                        tScopeDualDataElement sData;

                        sData.ulTimeuS = ((ulSampleCount + ulLoop) *
                                          pCapInfo->ulSamplePerioduS);
                        sData.sSample1mVolts = ADC_SAMPLE_TO_MV(
                                        pCapInfo->pusBuffer[ulSampleIndex++]);
                        sData.sSample2mVolts = ADC_SAMPLE_TO_MV(
                                        pCapInfo->pusBuffer[ulSampleIndex++]);

                        //
                        // Adjust for the wrap at the end of the ring buffer.
                        // It's fine to do this after writing both samples
                        // since the sample buffer must be a multiple of two
                        // samples long - we can't have a wrap between samples.
                        //
                        if(ulSampleIndex >= pCapInfo->ulMaxSamples)
                        {
                            ulSampleIndex -= pCapInfo->ulMaxSamples;
                        }

                        //
                        // Write the sample to the USB FIFO.
                        //
                        USBEndpointDataPut(USB0_BASE, DATA_IN_ENDPOINT,
                                           (unsigned char *)&sData,
                                            sizeof(tScopeDualDataElement));
                    }
                    else
                    {
                        //
                        // Single channel - send one sample per element.
                        //
                        tScopeDataElement sData;

                        sData.ulTimeuS = ((ulSampleCount + ulLoop) *
                                          pCapInfo->ulSamplePerioduS);
                        sData.sSamplemVolts = ADC_SAMPLE_TO_MV(
                                        pCapInfo->pusBuffer[ulSampleIndex++]);

                        //
                        // Adjust for the wrap at the end of the ring buffer.
                        //
                        if(ulSampleIndex >= pCapInfo->ulMaxSamples)
                        {
                            ulSampleIndex -= pCapInfo->ulMaxSamples;
                        }

                        //
                        // Write the sample to the USB FIFO.
                        //
                        USBEndpointDataPut(USB0_BASE, DATA_IN_ENDPOINT,
                                           (unsigned char *)&sData,
                                            sizeof(tScopeDataElement));
                    }
                }

                //
                // At this point, we've written all the data for a single data
                // packet so go ahead and send it.
                //
                iRetcode = USBEndpointDataSend(USB0_BASE, DATA_IN_ENDPOINT,
                                               USB_TRANS_IN);

                //
                // Was the transmission scheduled successfully?
                //
                if(iRetcode != 0)
                {
                    //
                    // No - something went wrong.  Handle this as if the host
                    // disconnected.
                    //
                    CleanUpOnDisconnect(true);
                    return(false);
                }

                //
                // Update our element counter for the next loop.
                //
                ulSampleCount += sPacket.ulParam;
            }

            //
            // At this point, all the data packets have been sent.  All we need
            // to do now is send a terminating SCOPE_PKT_DATA_END.
            //
            sPacket.ucPacketType = SCOPE_PKT_DATA_END;
            sPacket.ucParam = ucPacketCount;
            sPacket.ulParam = 0;
            sPacket.ulDataLength = 0;

            //
            // Wait for the data IN endpoint to complete sending
            // the previous packet.
            //
            bRetcode = WaitForUSBSendIdle(USB_DATA_TIMEOUT_TICKS);
            if(bRetcode)
            {
                bRetcode = SendUSBPacket(&sPacket, 0, 0);
            }

            if(!bRetcode)
            {
                //
                // We couldn't send the packet.  Treat this as a host
                // disconnection.
                //
                CleanUpOnDisconnect(true);
                return(false);
            }
            else
            {
                //
                // Clear the flag blocking the ISR from transmitting.
                //
                g_bBlockSend = false;
                return(true);
            }
        }
        else
        {
            //
            // We timed out waiting for the data IN endpoint to become free.
            // Something horrid happened so disconnect and wait for the host to
            // initiate communication.
            //
            CleanUpOnDisconnect(true);
        }
    }

    //
    // If we get here data transmission was disabled or we are
    // not connected to the host.
    //
    return(false);
}

//*****************************************************************************
//
// Called by the USB stack for any activity involving one of our endpoints
// other than EP0.  This function is a fan out that merely directs the call to
// the correct handler depending upon the endpoint and transaction direction
// signaled in ulStatus.
//
//*****************************************************************************
void
HandleEndpoints(void *pvInstance, unsigned long ulStatus)
{
    //
    // Handler for the bulk OUT data endpoint.
    //
    if(ulStatus & USB_INTEP_DEV_OUT_2)
    {
        //
        // Data is being sent to us from the host.
        //
        ProcessDataFromHost(ulStatus);
        ulStatus &= ~USB_INTEP_DEV_OUT_2;
    }

    //
    // Handler for the bulk IN data endpoint.
    //
    if(ulStatus & USB_INTEP_DEV_IN_1)
    {
        ProcessDataToHost(ulStatus);
        ulStatus &= ~USB_INTEP_DEV_IN_1;
    }

    //
    // Check to see if any other interrupt causes are being signaled.  If
    // there are any we don't support, this is a problem.
    //
    if(ulStatus)
    {
        UARTprintf("Unhandled EP interrupt 0x%08x!\n", ulStatus);
    }
}

//*****************************************************************************
//
// Called by the USB stack whenever a configuration change occurs.
//
//*****************************************************************************
void
HandleConfigChange(void *pvInstance, unsigned long ulInfo)
{
    UARTprintf("USB configuration change 0x%08x\n", ulInfo);

    //
    // Save this change in state.
    //
    g_ulUSBConfigured = ulInfo;

    //
    //
    // The host setting the configuration indicates to us that we are now
    // connected so update the device state.
    //
    g_eDeviceState = DEVICE_WAITING_FOR_HELLO;
}

//*****************************************************************************
//
// This function is called by the USB device stack whenever a bus reset occurs.
// We use this notification to tidy up and get back into an idle state in
// preparation for reconnection.
//
//*****************************************************************************
void
HandleReset(void *pvInstance)
{
    UARTprintf("USB reset - cleaning up.\n");

    //
    // Reset the states of all our endpoints.
    //
    CleanUpOnDisconnect(false);
}

//*****************************************************************************
//
// This function is called by the USB device stack whenever the host
// disconnects.  We use this notification to tidy up and get back into an
// idle state in preparation for reconnection.
//
//*****************************************************************************
void
HandleDisconnect(void *pvInstance)
{
    UARTprintf("USB host disconnected - cleaning up.\n");

    //
    // Reset the states of all our endpoints.
    //
    CleanUpOnDisconnect(true);
}
