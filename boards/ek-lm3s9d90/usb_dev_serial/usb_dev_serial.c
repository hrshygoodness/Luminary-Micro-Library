//*****************************************************************************
//
// usb_dev_serial.c - Main routines for the USB CDC serial example.
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
// This is part of revision 9453 of the EK-LM3S9D90 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "usb_serial_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Serial Device (usb_dev_serial)</h1>
//!
//! This example application turns the evaluation kit into a virtual serial
//! port when connected to the USB host system.  The application supports the
//! USB Communication Device Class, Abstract Control Model to redirect UART0
//! traffic to and from the USB host system.
//!
//! Assuming you installed StellarisWare in the default directory, a
//! driver information (INF) file for use with Windows XP, Windows Vista and
//! Windows7 can be found in C:/StellarisWare/windows_drivers. For Windows
//! 2000, the required INF file is in C:/StellarisWare/windows_drivers/win2K.
//
//*****************************************************************************

//*****************************************************************************
//
// Flag indicating whether or not a Break condition is currently being sent.
//
//*****************************************************************************
static tBoolean g_bSendingBreak = false;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// This function is called whenever serial data is received from the UART.
// It is passed the accumulated error flags from each character received in
// this interrupt and determines from them whether or not an interrupt
// notification to the host is required.
//
// If a notification is required and the control interrupt endpoint is idle,
// send the notification immediately.  If the endpoint is not idle, accumulate
// the errors in a global variable which will be checked on completion of the
// previous notification and used to send a second one if necessary.
//
//*****************************************************************************
static void
CheckForSerialStateChange(const tUSBDCDCDevice *psDevice, long lErrors)
{
    unsigned short usSerialState;

    //
    // Clear the USB serial state.  Since handshakes are being faked, always
    // set the TXCARRIER (DSR) and RXCARRIER (DCD) bits.
    //
    usSerialState = USB_CDC_SERIAL_STATE_TXCARRIER |
                    USB_CDC_SERIAL_STATE_RXCARRIER;

    //
    // Are any error bits set?
    //
    if(lErrors)
    {
        //
        // At least one error is being notified so translate from the hardware
        // error bits into the correct state markers for the USB notification.
        //
        if(lErrors & UART_DR_OE)
        {
            usSerialState |= USB_CDC_SERIAL_STATE_OVERRUN;
        }

        if(lErrors & UART_DR_PE)
        {
            usSerialState |= USB_CDC_SERIAL_STATE_PARITY;
        }

        if(lErrors & UART_DR_FE)
        {
            usSerialState |= USB_CDC_SERIAL_STATE_FRAMING;
        }

        if(lErrors & UART_DR_BE)
        {
            usSerialState |= USB_CDC_SERIAL_STATE_BREAK;
        }

        //
        // Call the CDC driver to notify the state change.
        //
        USBDCDCSerialStateChange((void *)psDevice, usSerialState);
    }
}

//*****************************************************************************
//
// Read as many characters from the UART FIFO as possible and move them into
// the CDC transmit buffer.
//
// \return Returns UART error flags read during data reception.
//
//*****************************************************************************
static long
ReadUARTData(void)
{
    long lChar, lErrors;
    unsigned char ucChar;
    unsigned long ulSpace;

    //
    // Clear the error indicator.
    //
    lErrors = 0;

    //
    // How much space is available in the buffer?
    //
    ulSpace = USBBufferSpaceAvailable(&g_sTxBuffer);

    //
    // Read data from the UART FIFO until there is none left or there is no
    // more space in the receive buffer.
    //
    while(ulSpace && ROM_UARTCharsAvail(UART0_BASE))
    {
        //
        // Read a character from the UART FIFO into the ring buffer if no
        // errors are reported.
        //
        lChar = ROM_UARTCharGetNonBlocking(UART0_BASE);

        //
        // If the character did not contain any error notifications,
        // copy it to the output buffer.
        //
        if(!(lChar & ~0xFF))
        {
            ucChar = (unsigned char)(lChar & 0xFF);
            USBBufferWrite(&g_sTxBuffer, &ucChar, 1);

            //
            // Decrement the number of bytes the buffer can accept.
            //
            ulSpace--;
        }
        else
        {
            //
            // Update the error accumulator.
            //
            lErrors |= lChar;
        }
    }

    //
    // Pass back the accumulated error indicators.
    //
    return(lErrors);
}

//*****************************************************************************
//
// Take as many bytes from the transmit buffer as there is space for and move
// them into the USB UART's transmit FIFO.
//
//*****************************************************************************
static void
USBUARTPrimeTransmit(void)
{
    unsigned long ulRead;
    unsigned char ucChar;

    //
    // If a break condition is currently being sent, don't receive any more
    // data.  Transmission will resume once the break is turned off.
    //
    if(g_bSendingBreak)
    {
        return;
    }

    //
    // If there is space in the UART FIFO, try to read some characters
    // from the receive buffer to fill it again.
    //
    while(ROM_UARTSpaceAvail(UART0_BASE))
    {
        //
        // Get a character from the buffer.
        //
        ulRead = USBBufferRead(&g_sRxBuffer, &ucChar, 1);

        //
        // Was a character read?
        //
        if(ulRead)
        {
            //
            // Place the character in the UART transmit FIFO.
            //
            ROM_UARTCharPutNonBlocking(UART0_BASE, ucChar);
        }
        else
        {
            //
            // There are no more characters so exit the function.
            //
            return;
        }
    }
}

//*****************************************************************************
//
// Interrupt handler for the UART which is being redirected via USB.
//
//*****************************************************************************
void
USBUARTIntHandler(void)
{
    unsigned long ulInts;
    long lErrors;

    //
    // Get and clear the current interrupt source(s)
    //
    ulInts = ROM_UARTIntStatus(UART0_BASE, true);
    ROM_UARTIntClear(UART0_BASE, ulInts);

    //
    // Handle transmit interrupts.
    //
    if(ulInts & UART_INT_TX)
    {
        //
        // Move as many bytes as possible into the transmit FIFO.
        //
        USBUARTPrimeTransmit();

        //
        // If the output buffer is empty, turn off the transmit interrupt.
        //
        if(!USBBufferDataAvailable(&g_sRxBuffer))
        {
            ROM_UARTIntDisable(UART0_BASE, UART_INT_TX);
        }
    }

    //
    // Handle receive interrupts.
    //
    if(ulInts & (UART_INT_RX | UART_INT_RT))
    {
        //
        // Read the UART's characters into the buffer.
        //
        lErrors = ReadUARTData();

        //
        // Check to see if the host needs to be notified of any errors just
        // detected.
        //
        CheckForSerialStateChange(&g_sCDCDevice, lErrors);
    }
}

//*****************************************************************************
//
// Set the communication parameters to use on the UART.
//
//*****************************************************************************
static tBoolean
SetLineCoding(tLineCoding *psLineCoding)
{
    unsigned long ulConfig;
    tBoolean bRetcode;

    //
    // Assume everything is OK until a problem is detected.
    //
    bRetcode = true;

    //
    // Word length.  For invalid values, the default is to set 8 bits per
    // character and return an error.
    //
    switch(psLineCoding->ucDatabits)
    {
        case 5:
        {
            ulConfig = UART_CONFIG_WLEN_5;
            break;
        }

        case 6:
        {
            ulConfig = UART_CONFIG_WLEN_6;
            break;
        }

        case 7:
        {
            ulConfig = UART_CONFIG_WLEN_7;
            break;
        }

        case 8:
        {
            ulConfig = UART_CONFIG_WLEN_8;
            break;
        }

        default:
        {
            ulConfig = UART_CONFIG_WLEN_8;
            bRetcode = false;
            break;
        }
    }

    //
    // Parity.  For any invalid values, set no parity and return an error.
    //
    switch(psLineCoding->ucParity)
    {
        case USB_CDC_PARITY_NONE:
        {
            ulConfig |= UART_CONFIG_PAR_NONE;
            break;
        }

        case USB_CDC_PARITY_ODD:
        {
            ulConfig |= UART_CONFIG_PAR_ODD;
            break;
        }

        case USB_CDC_PARITY_EVEN:
        {
            ulConfig |= UART_CONFIG_PAR_EVEN;
            break;
        }

        case USB_CDC_PARITY_MARK:
        {
            ulConfig |= UART_CONFIG_PAR_ONE;
            break;
        }

        case USB_CDC_PARITY_SPACE:
        {
            ulConfig |= UART_CONFIG_PAR_ZERO;
            break;
        }

        default:
        {
            ulConfig |= UART_CONFIG_PAR_NONE;
            bRetcode = false;
            break;
        }
    }

    //
    // Stop bits.  The hardware only supports 1 or 2 stop bits whereas CDC
    // allows the host to select 1.5 stop bits.  If passed 1.5 (or any other
    // invalid or unsupported value of ucStop, set up for 1 stop bit but return
    // an error in case the caller needs to Stall or otherwise report this back
    // to the host.
    //
    switch(psLineCoding->ucStop)
    {
        //
        // One stop bit requested.
        //
        case USB_CDC_STOP_BITS_1:
        {
            ulConfig |= UART_CONFIG_STOP_ONE;
            break;
        }

        //
        // Two stop bits requested.
        //
        case USB_CDC_STOP_BITS_2:
        {
            ulConfig |= UART_CONFIG_STOP_TWO;
            break;
        }

        //
        // Other cases are either invalid values of ucStop or values that are
        // not supported, so set 1 stop bit but return an error.
        //
        default:
        {
            ulConfig = UART_CONFIG_STOP_ONE;
            bRetcode = false;
            break;
        }
    }

    //
    // Set the UART mode appropriately.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(),
                            psLineCoding->ulRate, ulConfig);

    //
    // Let the caller know if a problem was encountered.
    //
    return(bRetcode);
}

//*****************************************************************************
//
// Get the communication parameters in use on the UART.
//
//*****************************************************************************
static void
GetLineCoding(tLineCoding *psLineCoding)
{
    unsigned long ulConfig;
    unsigned long ulRate;

    //
    // Get the current line coding set in the UART.
    //
    ROM_UARTConfigGetExpClk(UART0_BASE, ROM_SysCtlClockGet(), &ulRate,
                            &ulConfig);
    psLineCoding->ulRate = ulRate;

    //
    // Translate the configuration word length field into the format expected
    // by the host.
    //
    switch(ulConfig & UART_CONFIG_WLEN_MASK)
    {
        case UART_CONFIG_WLEN_8:
        {
            psLineCoding->ucDatabits = 8;
            break;
        }

        case UART_CONFIG_WLEN_7:
        {
            psLineCoding->ucDatabits = 7;
            break;
        }

        case UART_CONFIG_WLEN_6:
        {
            psLineCoding->ucDatabits = 6;
            break;
        }

        case UART_CONFIG_WLEN_5:
        {
            psLineCoding->ucDatabits = 5;
            break;
        }
    }

    //
    // Translate the configuration parity field into the format expected
    // by the host.
    //
    switch(ulConfig & UART_CONFIG_PAR_MASK)
    {
        case UART_CONFIG_PAR_NONE:
        {
            psLineCoding->ucParity = USB_CDC_PARITY_NONE;
            break;
        }

        case UART_CONFIG_PAR_ODD:
        {
            psLineCoding->ucParity = USB_CDC_PARITY_ODD;
            break;
        }

        case UART_CONFIG_PAR_EVEN:
        {
            psLineCoding->ucParity = USB_CDC_PARITY_EVEN;
            break;
        }

        case UART_CONFIG_PAR_ONE:
        {
            psLineCoding->ucParity = USB_CDC_PARITY_MARK;
            break;
        }

        case UART_CONFIG_PAR_ZERO:
        {
            psLineCoding->ucParity = USB_CDC_PARITY_SPACE;
            break;
        }
    }

    //
    // Translate the configuration stop bits field into the format expected
    // by the host.
    //
    switch(ulConfig & UART_CONFIG_STOP_MASK)
    {
        case UART_CONFIG_STOP_ONE:
        {
            psLineCoding->ucStop = USB_CDC_STOP_BITS_1;
            break;
        }

        case UART_CONFIG_STOP_TWO:
        {
            psLineCoding->ucStop = USB_CDC_STOP_BITS_2;
            break;
        }
    }
}

//*****************************************************************************
//
// This function sets or clears a break condition on the redirected UART RX
// line.  A break is started when the function is called with \e bSend set to
// \b true and persists until the function is called again with \e bSend set
// to \b false.
//
//*****************************************************************************
static void
SendBreak(tBoolean bSend)
{
    //
    // Is this the start or stop of a break condition?
    //
    if(!bSend)
    {
        //
        // Remove the break condition on the line.
        //
        ROM_UARTBreakCtl(UART0_BASE, false);
        g_bSendingBreak = false;
    }
    else
    {
        //
        // Start sending a break condition on the line.
        //
        ROM_UARTBreakCtl(UART0_BASE, true);
        g_bSendingBreak = true;
    }
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
unsigned long
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
            // Turn on the user LED.
            //
            GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0);

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
            // Turn off the user LED.
            //
            GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0);

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
            SetLineCoding(pvMsgData);
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
            SendBreak(true);
            break;
        }

        //
        // Clear the break condition on the serial line.
        //
        case USBD_CDC_EVENT_CLEAR_BREAK:
        {
            SendBreak(false);
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
unsigned long
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
unsigned long
RxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
          void *pvMsgData)
{
    unsigned long ulCount;

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
            // Feed some characters into the UART TX FIFO and enable the
            // interrupt.
            //
            USBUARTPrimeTransmit();
            ROM_UARTIntEnable(UART0_BASE, UART_INT_TX);
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
            ulCount = UARTBusy(UART0_BASE) ? 1 : 0;
            return(ulCount);
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
// This is the main application entry function.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run from the PLL at 50MHz
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Enable the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Enable and configure the UART RX and TX pins
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Set the default UART configuration.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE |
                            UART_CONFIG_STOP_ONE);
    ROM_UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    //
    // Configure and enable UART interrupts.
    //
    ROM_UARTIntClear(UART0_BASE, ROM_UARTIntStatus(UART0_BASE, false));
    ROM_UARTIntEnable(UART0_BASE, (UART_INT_OE | UART_INT_BE | UART_INT_PE |
                                   UART_INT_FE | UART_INT_RT | UART_INT_RX));

    //
    // Enable and configure the user LED pin.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0);

    //
    // Turn off the user LED.
    //
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0);

    //
    // Initialize the transmit and receive buffers.
    //
    USBBufferInit(&g_sTxBuffer);
    USBBufferInit(&g_sRxBuffer);

    //
    // Set the USB stack mode to Device mode with VBUS monitoring.
    //
    USBStackModeSet(0, USB_MODE_DEVICE, 0);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDCDCInit(0, &g_sCDCDevice);

    //
    // Enable interrupts now that the application is ready to start.
    //
    ROM_IntEnable(INT_UART0);

    //
    // Main application loop.
    //
    while(1)
    {
    }
}
