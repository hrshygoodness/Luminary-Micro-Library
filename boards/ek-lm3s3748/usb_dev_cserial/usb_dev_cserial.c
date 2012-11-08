//****************************************************************************
//
// usb_dev_cserial.c - Main routines for the USB CDC composite serial example.
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
//****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/usb.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcomp.h"
#include "usblib/device/usbdcdc.h"
#include "utils/cmdline.h"
#include "utils/ustdlib.h"
#include "drivers/formike128x128x16.h"
#include "usb_structs.h"

//****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Composite Serial Device (usb_dev_cserial)</h1>
//!
//! This example application turns the evaluation kit into a multiple virtual
//! serial ports when connected to the USB host system.  The application
//! supports the USB Communication Device Class, Abstract Control Model to
//! redirect UART0 traffic to and from the USB host system.  The first virtual
//! serial port will echo data to the physical UART0 port on the device which
//! is connected to the virtual serial port on the FTDI device on this board.
//! The physical UART0 will also echo onto the first virtual serial device
//! provided by the Stellaris controller.  The second Stellaris virtual serial
//! port will provide a console that can echo data to both the FTDI virtual
//! serial port and the first Stellaris virtual serial port.  It will also
//! allow turning on, off or toggling the boards led status.  Typing a "?" and
//! pressing return should echo a list of commands to the terminal, since this
//! board can show up as possibly three individual virtual serial devices.
//!
//! Assuming you installed StellarisWare in the default directory, a
//! driver information (INF) file for use with Windows XP, Windows Vista and
//! Windows7 can be found in C:/StellarisWare/windows_drivers. For Windows
//! 2000, the required INF file is in C:/StellarisWare/windows_drivers/win2K.
//
//****************************************************************************

//****************************************************************************
//
// Note:
//
// This example is intended to run on Stellaris evaluation kit hardware
// where the UARTs are wired solely for TX and RX, and do not have GPIOs
// connected to act as handshake signals.  As a result, this example mimics
// the case where communication is always possible.  It reports DSR, DCD
// and CTS as high to ensure that the USB host recognizes that data can be
// sent and merely ignores the host's requested DTR and RTS states.  "TODO"
// comments in the code indicate where code would be required to add support
// for real handshakes.
//
//****************************************************************************

//****************************************************************************
//
// Configuration and tuning parameters.
//
//****************************************************************************

//****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//****************************************************************************
#define SYSTICKS_PER_SECOND 100
#define SYSTICK_PERIOD_MS (1000 / SYSTICKS_PER_SECOND)

//****************************************************************************
//
// USB mux GPIO definitions.
//
//****************************************************************************
#define USB_MUX_GPIO_PERIPH     SYSCTL_PERIPH_GPIOH
#define USB_MUX_GPIO_BASE       GPIO_PORTH_BASE
#define USB_MUX_GPIO_PIN        GPIO_PIN_2
#define USB_MUX_SEL_DEVICE      USB_MUX_GPIO_PIN

//****************************************************************************
//
// Variables tracking transmit and receive counts.
//
//****************************************************************************
volatile unsigned long g_ulUARTTxCount = 0;
volatile unsigned long g_ulUARTRxCount = 0;

//****************************************************************************
//
// Default line coding settings for the redirected UART.
//
//****************************************************************************
#define DEFAULT_BIT_RATE        115200
#define DEFAULT_UART_CONFIG     (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | \
                                 UART_CONFIG_STOP_ONE)

//****************************************************************************
//
// GPIO peripherals and pins muxed with the redirected UART.  These will
// depend upon the IC in use and the UART selected in UART0_BASE.  Be careful
// that these settings all agree with the hardware you are using.
//
//****************************************************************************
#define TX_GPIO_BASE            GPIO_PORTA_BASE
#define TX_GPIO_PERIPH          SYSCTL_PERIPH_GPIOA
#define TX_GPIO_PIN             GPIO_PIN_1

#define RX_GPIO_BASE            GPIO_PORTA_BASE
#define RX_GPIO_PERIPH          SYSCTL_PERIPH_GPIOA
#define RX_GPIO_PIN             GPIO_PIN_0

//****************************************************************************
//
// The LED control macros.
//
//****************************************************************************
#define LEDOn()  ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);

#define LEDOff() ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0)
#define LEDToggle()                                                         \
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0,                           \
                     (ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) ^        \
                                      GPIO_PIN_0));

//****************************************************************************
//
// Character sequence sent to the serial terminal to implement a character
// erase when backspace is pressed.
//
//****************************************************************************
static const char g_pcBackspace[3] = {0x08, ' ', 0x08};

//****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//****************************************************************************
#define CMD_BUF_SIZE            256

//****************************************************************************
//
// The buffer that holds the command line.
//
//****************************************************************************
static char g_pcCmdBuf[CMD_BUF_SIZE];
static unsigned long ulCmdIdx;

//****************************************************************************
//
// Flag indicating whether or not we are currently sending a Break condition.
//
//****************************************************************************
static tBoolean g_bSendingBreak = false;

//****************************************************************************
//
// Global system tick counter
//
//****************************************************************************
volatile unsigned long g_ulSysTickCount = 0;

//****************************************************************************
//
// The memory allocated to hold the composite descriptor that is created by
// the call to USBDCompositeInit().
//
//****************************************************************************
#define DESCRIPTOR_DATA_SIZE    (COMPOSITE_DCDC_SIZE*2)
unsigned char g_pucDescriptorData[DESCRIPTOR_DATA_SIZE];

//****************************************************************************
//
// Graphics context used to show text on the color STN display.
//
//****************************************************************************
tContext g_sContext;
#define TEXT_FONT               g_pFontFixed6x8
#define TEXT_HEIGHT             8
#define BUFFER_METER_HEIGHT     12
#define BUFFER_METER_WIDTH      52

//****************************************************************************
//
// Flags used to pass commands from interrupt context to the main loop.
//
//****************************************************************************
#define COMMAND_PACKET_RECEIVED 0x00000001
#define COMMAND_STATUS_UPDATE   0x00000002
#define COMMAND_RECEIVED        0x00000004

volatile unsigned long g_ulFlags = 0;
char *g_pcStatus;

//****************************************************************************
//
// Global flag indicating that a USB configuration has been set.
//
//****************************************************************************
static volatile tBoolean g_bUSBConfigured = false;

//****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    while(1)
    {
    }
}
#endif

//****************************************************************************
//
// This function will print out to the console UART and not the echo UART.
//
//****************************************************************************
void
CommandPrint(const char *pcStr)
{
    int iIdx;
    const char cCR = 0xd;

    iIdx = 0;

    while(pcStr[iIdx] != 0)
    {
        //
        // Wait for space for two bytes in case there is a need to send out
        // the line feed plus the carriage return.
        //
        while(USBBufferSpaceAvailable(&g_psTxBuffer[1]) < 2)
        {
        }

        //
        // Print the next character.
        //
        USBBufferWrite(&g_psTxBuffer[1], (const unsigned char *)&pcStr[iIdx],
                       1);

        //
        // If this is a line feed then send a carriage return as well.
        //
        if(pcStr[iIdx] == 0xa)
        {
            USBBufferWrite(&g_psTxBuffer[1], (const unsigned char *)&cCR, 1);
        }

        iIdx++;
    }
}

//****************************************************************************
//
// Shows the status string on the color STN display.
//
// \param psContext is a pointer to the graphics context representing the
// display.
// \param pcStatus is a pointer to the string to be shown.
//
//****************************************************************************
void
DisplayStatus(tContext *psContext, char *pcStatus)
{
    tRectangle sRect;

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = GrContextDpyHeightGet(&g_sContext) - 15;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Draw the status string.
    //
    GrContextBackgroundSet(&g_sContext, ClrDarkBlue);
    GrStringDraw(psContext, pcStatus, -1,
                 4, GrContextDpyHeightGet(&g_sContext) - 11,
                 true);

    GrContextBackgroundSet(&g_sContext, ClrBlack);
}

//****************************************************************************
//
// This function is called whenever serial data is received from the UART.
// It is passed the accumulated error flags from each character received in
// this interrupt and determines from them whether or not an interrupt
// notification to the host is required.
//
// If a notification is required and the control interrupt endpoint is idle,
// we send the notification immediately.  If the endpoint is not idle, we
// accumulate the errors in a global variable which will be checked on
// completion of the previous notification and used to send a second one
// if necessary.
//
//****************************************************************************
static void
CheckForSerialStateChange(const tUSBDCDCDevice *psDevice, long lErrors)
{
    unsigned short usSerialState;

    //
    // Clear our USB serial state.  Since we are faking the handshakes, always
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
        // At least one error is being notified so translate from our hardware
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

//****************************************************************************
//
// Read as many characters from the UART FIFO as we can and move them into
// the CDC transmit buffer.
//
// \return Returns UART error flags read during data reception.
//
//****************************************************************************
static long
ReadUARTData(void)
{
    long lChar, lErrors;
    unsigned char ucChar;
    unsigned long ulSpace;

    //
    // Clear our error indicator.
    //
    lErrors = 0;

    //
    // How much space do we have in the buffer?
    //
    ulSpace = USBBufferSpaceAvailable((tUSBBuffer *)&g_psTxBuffer[0]);

    //
    // Read data from the UART FIFO until there is none left or we run
    // out of space in our receive buffer.
    //
    while(ulSpace && UARTCharsAvail(UART0_BASE))
    {
        //
        // Read a character from the UART FIFO into the ring buffer if no
        // errors are reported.
        //
        lChar = UARTCharGetNonBlocking(UART0_BASE);

        //
        // If the character did not contain any error notifications,
        // copy it to the output buffer.
        //
        if(!(lChar & ~0xFF))
        {
            ucChar = (unsigned char)(lChar & 0xFF);

            USBBufferWrite((tUSBBuffer *)&g_psTxBuffer[0],
                           (unsigned char *)&ucChar, 1);

            //
            // Decrement the number of bytes we know the buffer can accept.
            //
            ulSpace--;
        }
        else
        {
            //
            // Update our error accumulator.
            //
            lErrors |= lChar;
        }

        //
        // Update our count of bytes received via the UART.
        //
        g_ulUARTRxCount++;
    }

    //
    // Pass back the accumulated error indicators.
    //
    return(lErrors);
}

//****************************************************************************
//
// Take as many bytes from the transmit buffer as we have space for and move
// them into the USB UART's transmit FIFO.
//
//****************************************************************************
static void
USBUARTPrimeTransmit(unsigned long ulBase)
{
    unsigned long ulRead;
    unsigned char ucChar;

    //
    // If we are currently sending a break condition, don't receive any
    // more data. We will resume transmission once the break is turned off.
    //
    if(g_bSendingBreak)
    {
        return;
    }

    //
    // If there is space in the UART FIFO, try to read some characters
    // from the receive buffer to fill it again.
    //
    while(UARTSpaceAvail(ulBase))
    {
        //
        // Get a character from the buffer.
        //
        ulRead = USBBufferRead((tUSBBuffer *)&g_psRxBuffer[0], &ucChar, 1);

        //
        // Did we get a character?
        //
        if(ulRead)
        {
            //
            // Place the character in the UART transmit FIFO.
            //
            UARTCharPut(ulBase, ucChar);

            //
            // Update our count of bytes transmitted via the UART.
            //
            g_ulUARTTxCount++;
        }
        else
        {
            //
            // We ran out of characters so exit the function.
            //
            return;
        }
    }
}

//****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update our system time.
    //
    g_ulSysTickCount++;
}

//****************************************************************************
//
// Interrupt handler for the UART which we are redirecting via USB.
//
//****************************************************************************
void
USBUARTIntHandler(void)
{
    unsigned long ulInts;
    long lErrors;

    //
    // Get and clear the current interrupt source(s)
    //
    ulInts = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, ulInts);

    //
    // Are we being interrupted because the TX FIFO has space available?
    //
    if(ulInts & UART_INT_TX)
    {
        //
        // Move as many bytes as we can into the transmit FIFO.
        //
        USBUARTPrimeTransmit(UART0_BASE);

        //
        // If the output buffer is empty, turn off the transmit interrupt.
        //
        if(!USBBufferDataAvailable(&g_psRxBuffer[0]))
        {
            UARTIntDisable(UART0_BASE, UART_INT_TX);
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
        // Check to see if we need to notify the host of any errors we just
        // detected.
        //
        CheckForSerialStateChange(&g_psCDCDevice[0], lErrors);
    }
}

//****************************************************************************
//
// Set the state of the RS232 RTS and DTR signals.  Handshaking is not
// supported so this request will be ignored.
//
//****************************************************************************
static void
SetControlLineState(unsigned short usState)
{
}

//****************************************************************************
//
// Set the communication parameters to use on the UART.
//
//****************************************************************************
static tBoolean
SetLineCoding(tLineCoding *psLineCoding)
{
    unsigned long ulConfig;
    tBoolean bRetcode;

    //
    // Assume everything is OK until we detect any problem.
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
    // Parity.  For any invalid values, we set no parity and return an error.
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
    // Stop bits.  Our hardware only supports 1 or 2 stop bits whereas CDC
    // allows the host to select 1.5 stop bits.  If passed 1.5 (or any other
    // invalid or unsupported value of ucStop, we set up for 1 stop bit but
    // return an error in case the caller needs to Stall or otherwise report
    // this back to the host.
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
        // Other cases are either invalid values of ucStop or values that we
        // cannot support so set 1 stop bit but return an error.
        //
        default:
        {
            ulConfig = UART_CONFIG_STOP_ONE;
            bRetcode |= false;
            break;
        }
    }

    //
    // Set the UART mode appropriately.
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), psLineCoding->ulRate,
                        ulConfig);

    //
    // Let the caller know if we had a problem or not.
    //
    return(bRetcode);
}

//****************************************************************************
//
// Get the communication parameters in use on the UART.
//
//****************************************************************************
static void
GetLineCoding(tLineCoding *psLineCoding)
{
    unsigned long ulConfig;
    unsigned long ulRate;

    //
    // Get the current line coding set in the UART.
    //
    UARTConfigGetExpClk(UART0_BASE, SysCtlClockGet(), &ulRate,
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

//****************************************************************************
//
// This function sets or clears a break condition on the redirected UART RX
// line.  A break is started when the function is called with \e bSend set to
// \b true and persists until the function is called again with \e bSend set
// to \b false.
//
//****************************************************************************
static void
SendBreak(tBoolean bSend)
{
    //
    // Are we being asked to start or stop the break condition?
    //
    if(!bSend)
    {
        //
        // Remove the break condition on the line.
        //
        UARTBreakCtl(UART0_BASE, false);
        g_bSendingBreak = false;
    }
    else
    {
        //
        // Start sending a break condition on the line.
        //
        UARTBreakCtl(UART0_BASE, true);
        g_bSendingBreak = true;
    }
}

//****************************************************************************
//
// Draw a horizontal meter at a given position on the display and fill it
// with green.
//
//****************************************************************************
void
DrawBufferMeter(tContext *psContext, long lX, long lY)
{
    tRectangle sRect;
    long lCorrectedY;

    //
    // Correct the Y coordinate so that the meter is centered on the same line
    // as the text caption to its left.
    //
    lCorrectedY = lY - ((BUFFER_METER_HEIGHT - TEXT_HEIGHT) / 2);

    //
    // Determine the bounding rectangle of the meter.
    //
    sRect.sXMin = lX;
    sRect.sXMax = lX + BUFFER_METER_WIDTH - 1;
    sRect.sYMin = lCorrectedY;
    sRect.sYMax = lCorrectedY + BUFFER_METER_HEIGHT - 1;

    //
    // Fill the meter with green to indicate empty
    //
    GrContextForegroundSet(psContext, ClrGreen);
    GrRectFill(psContext, &sRect);

    //
    // Put a white box around the meter.
    //
    GrContextForegroundSet(psContext, ClrWhite);
    GrRectDraw(psContext, &sRect);
}

//****************************************************************************
//
// Draw green and red blocks within a graphical meter on the display to
// indicate percentage fullness of some quantity (transmit and receive buffers
// in this case).
//
//****************************************************************************
void
UpdateBufferMeter(tContext *psContext, unsigned long ulFullPercent, long lX,
                  long lY)
{
    tRectangle sRect;
    long lCorrectedY;
    long lXBreak;

    //
    // Correct the Y coordinate so that the meter is centered on the same line
    // as the text caption to its left and so that we avoid the meter's 1
    // pixel white border.
    //
    lCorrectedY = lY - ((BUFFER_METER_HEIGHT - TEXT_HEIGHT) / 2) + 1;

    //
    // Determine where the break point between full (red) and empty (green)
    // sections occurs.
    //
    lXBreak = (lX + 1) + (ulFullPercent * (BUFFER_METER_WIDTH - 2)) / 100;

    //
    // Determine the bounding rectangle of the full section.
    //
    sRect.sXMin = lX + 1;
    sRect.sXMax = lXBreak;
    sRect.sYMin = lCorrectedY;
    sRect.sYMax = lCorrectedY + BUFFER_METER_HEIGHT - 3;

    //
    // Fill the full section with red (if there is anything to draw)
    //
    if(ulFullPercent)
    {
        GrContextForegroundSet(psContext, ClrRed);
        GrRectFill(psContext, &sRect);
    }

    //
    // Fill the empty section with green.
    //
    sRect.sXMin = lXBreak;
    sRect.sXMax = lX + BUFFER_METER_WIDTH - 2;
    if(sRect.sXMax > sRect.sXMin)
    {
        GrContextForegroundSet(psContext, ClrGreen);
        GrRectFill(psContext, &sRect);
    }

    //
    // Revert to white for text drawing which may occur later.
    //
    GrContextForegroundSet(psContext, ClrWhite);

}

//****************************************************************************
//
// Handles CDC driver notifications related to control and setup of the
// device.
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
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
//****************************************************************************
unsigned long
ControlHandler(void *pvCBData, unsigned long ulEvent,
               unsigned long ulMsgValue, void *pvMsgData)
{
    unsigned long ulIntsOff;

    //
    // Which event are we being asked to process?
    //
    switch(ulEvent)
    {
        //
        // We are connected to a host and communication is now possible.
        //
        case USB_EVENT_CONNECTED:
        {
            g_bUSBConfigured = true;

            //
            // Flush our buffers.
            //
            USBBufferFlush(&g_psTxBuffer[0]);
            USBBufferFlush(&g_psRxBuffer[0]);

            //
            // Tell the main loop to update the display.
            //
            ulIntsOff = IntMasterDisable();
            g_pcStatus = "Host connected.";
            g_ulFlags |= COMMAND_STATUS_UPDATE;
            if(!ulIntsOff)
            {
                IntMasterEnable();
            }
            break;
        }

        //
        // The host has disconnected.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_bUSBConfigured = false;
            ulIntsOff = IntMasterDisable();
            g_pcStatus = "Host disconnected.";
            g_ulFlags |= COMMAND_STATUS_UPDATE;
            if(!ulIntsOff)
            {
                IntMasterEnable();
            }
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
            SetControlLineState((unsigned short)ulMsgValue);
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
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param ulCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//****************************************************************************
unsigned long
TxHandlerEcho(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
              void *pvMsgData)
{
    //
    // Which event have we been sent?
    //
    switch(ulEvent)
    {
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Since we are using the USBBuffer, we don't need to do anything
            // here.
            //
            break;
        }

        //
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
        {
            break;
        }
    }
    return(0);
}

//****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param ulCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//****************************************************************************
unsigned long
TxHandlerCmd(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
             void *pvMsgData)
{
    //
    // Which event have we been sent?
    //
    switch(ulEvent)
    {
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Since we are using the USBBuffer, we don't need to do anything
            // here.
            //
            break;
        }

        //
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
        {
            break;
        }
    }
    return(0);
}

//****************************************************************************
//
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param ulCBData is the client-supplied callback data value for this
// channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//****************************************************************************
unsigned long
RxHandlerEcho(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
              void *pvMsgData)
{
    unsigned long ulCount;

    //
    // Which event are we being sent?
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
            // interrupt so we are told when there is more space.
            //
            USBUARTPrimeTransmit(UART0_BASE);
            UARTIntEnable(UART0_BASE, UART_INT_TX);
            break;
        }

        //
        // We are being asked how much unprocessed data we have still to
        // process. We return 0 if the UART is currently idle or 1 if it is
        // in the process of transmitting something. The actual number of
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
        // We are being asked to provide a buffer into which the next packet
        // can be read. We do not support this mode of receiving data so let
        // the driver know by returning 0. The CDC driver should not be
        // sending this message but this is included just for illustration and
        // completeness.
        //
        case USB_EVENT_REQUEST_BUFFER:
        {
            return(0);
        }

        //
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//****************************************************************************
//
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param ulCBData is the client-supplied callback data value for this
// channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//****************************************************************************
unsigned long
RxHandlerCmd(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
             void *pvMsgData)
{
    unsigned char ucChar;
    const tUSBDCDCDevice *psCDCDevice;
    const tUSBBuffer *pBufferRx;
    const tUSBBuffer *pBufferTx;

    //
    // Which event are we being sent?
    //
    switch(ulEvent)
    {
        //
        // A new packet has been received.
        //
        case USB_EVENT_RX_AVAILABLE:
        {
            //
            // Create a device pointer.
            //
            psCDCDevice = (const tUSBDCDCDevice *)pvCBData;
            pBufferRx = (const tUSBBuffer *)psCDCDevice->pvRxCBData;
            pBufferTx = (const tUSBBuffer *)psCDCDevice->pvTxCBData;

            //
            // Keep reading characters as long as there are more to receive.
            //
            while(USBBufferRead(pBufferRx,
                                       (unsigned char *)&g_pcCmdBuf[ulCmdIdx],
                                       1))
            {
                //
                // If this is a backspace character, erase the last thing typed
                // assuming there's something there to type.
                //
                if(g_pcCmdBuf[ulCmdIdx] == 0x08)
                {
                    //
                    // If our current command buffer has any characters in it,
                    // erase the last one.
                    //
                    if(ulCmdIdx)
                    {
                        //
                        // Delete the last character.
                        //
                        ulCmdIdx--;

                        //
                        // Send a backspace, a space and a further backspace so
                        // that the character is erased from the terminal too.
                        //
                        USBBufferWrite(pBufferTx,
                                       (unsigned char *)g_pcBackspace, 3);
                    }
                }
                //
                // If this was a line feed then put out a carriage return as
                // well.
                //
                else
                {
                    //
                    // Feed the new characters into the UART TX FIFO.
                    //
                    USBBufferWrite(pBufferTx,
                                   (unsigned char *)&g_pcCmdBuf[ulCmdIdx], 1);

                    //
                    // Was this a carriage return?
                    //
                    if(g_pcCmdBuf[ulCmdIdx] == 0xd)
                    {
                        //
                        // Set a line feed.
                        //
                        ucChar = 0xa;
                        USBBufferWrite(pBufferTx, &ucChar, 1);

                        //
                        // Indicate that a command has been received.
                        //
                        g_ulFlags |= COMMAND_RECEIVED;

                        g_pcCmdBuf[ulCmdIdx] = 0;

                        ulCmdIdx = 0;
                    }

                    //
                    // Only increment if the index has not reached the end of
                    // the buffer and continually overwrite the last value if
                    // the buffer does attempt to overflow.
                    //
                    else if(ulCmdIdx < CMD_BUF_SIZE)
                    {
                        ulCmdIdx++;
                    }
                }
            }

            break;
        }

        //
        // We are being asked how much unprocessed data we have still to
        // process. We return 0 if the UART is currently idle or 1 if it is
        // in the process of transmitting something. The actual number of
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
        // We are being asked to provide a buffer into which the next packet
        // can be read. We do not support this mode of receiving data so let
        // the driver know by returning 0. The CDC driver should not be
        // sending this message but this is included just for illustration and
        // completeness.
        //
        case USB_EVENT_REQUEST_BUFFER:
        {
            return(0);
        }

        //
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//****************************************************************************
//
// This command allows setting, clearing or toggling the Status LED.
//
// The first argument should be one of the following:
// on     - Turn on the LED.
// off    - Turn off the LED.
// toggle - Toggle the current LED status.
//
//****************************************************************************
int
Cmd_led(int argc, char *argv[])
{
    //
    // These values only check the second character since all parameters are
    // different in that character.
    //
    if(argv[1][1] == 'n')
    {
        //
        // Turn on the LED.
        //
        LEDOn();
    }
    else if(argv[1][1] == 'f')
    {
        //
        // Turn off the LED.
        //
        LEDOff();
    }
    else if(argv[1][1] == 'o')
    {
        //
        // Toggle the LED.
        //
        LEDToggle();
    }
    else
    {
        //
        // The command format was not correct so print out some help.
        //
        CommandPrint("\nled <on|off|toggle>\n");
        CommandPrint("  on       - Turn on the LED.\n");
        CommandPrint("  off      - Turn off the LED.\n");
        CommandPrint("  toggle   - Toggle the LED state.\n");
    }
    return(0);
}

//****************************************************************************
//
// This is a stub that will not be called.  It is here to echo the help string
// but will be handled before being called by CmdLineProcess().
//
//****************************************************************************
int
Cmd_echo(int argc, char *argv[])
{
    return(0);
}

//****************************************************************************
//
// This function is called when "echo" command is issued so that the
// CmdLineProcess() function does not attempt to split up the string based on
// space delimiters.
//
//****************************************************************************
int
Echo(char *pucStr)
{
    int iIdx;

    //
    // Fail the command if the "echo" command is not terminated with a space.
    //
    if(pucStr[4] != ' ')
    {
        return(-1);
    }

    //
    // Put out a carriage return and line feed to both echo ports.
    //
    USBBufferWrite((tUSBBuffer *)&g_psTxBuffer[0], (unsigned char *)"\r\n", 2);
    UARTCharPut(UART0_BASE, '\r');
    UARTCharPut(UART0_BASE, '\n');

    //
    // Loop through the characters and print them to both echo ports.
    //
    for(iIdx = 5; iIdx < CMD_BUF_SIZE; iIdx++)
    {
        //
        // If a null is found then go to the next argument and replace the
        // null with a space character.
        //
        if(pucStr[iIdx] == 0)
        {
            break;
        }

        //
        // Write out the character to both echo ports.
        //
        USBBufferWrite((tUSBBuffer *)&g_psTxBuffer[0],
                       (unsigned char *)&pucStr[iIdx], 1);
        UARTCharPut(UART0_BASE, pucStr[iIdx]);
    }
    return(0);
}

//****************************************************************************
//
// This function implements the "help" command.  It prints a simple list of
// the available commands with a brief description.
//
//****************************************************************************
int
Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    //
    // Print some header text.
    //
    CommandPrint("\nAvailable commands\n");
    CommandPrint("------------------\n");

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_sCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        CommandPrint(pEntry->pcCmd);
        CommandPrint(pEntry->pcHelp);
        CommandPrint("\n");

        //
        // Advance to the next entry in the table.
        //
        pEntry++;
    }

    //
    // Return success.
    //
    return(0);
}

//****************************************************************************
//
// This is the table that holds the command names, implementing functions, and
// brief description.
//
//****************************************************************************
tCmdLineEntry g_sCmdTable[] =
{
    { "help",  Cmd_help,      " : Display list of commands" },
    { "h",     Cmd_help,   "    : alias for help" },
    { "?",     Cmd_help,   "    : alias for help" },
    { "echo",  Cmd_echo,      " : Text will be displayed on all echo ports" },
    { "led",   Cmd_led,      "  : Turn on/off/toggle the Status LED" },
    { 0, 0, 0 }
};

//****************************************************************************
//
// This is the main application entry function.
//
//****************************************************************************
int
main(void)
{
    unsigned long ulTxCount;
    unsigned long ulRxCount;
    int iStatus;
    tRectangle sRect;
    char pcBuffer[16];
    unsigned long ulFullness;

    //
    // Set the clocking to run from the PLL at 50MHz
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Not configured initially.
    //
    g_bUSBConfigured = false;

    //
    // Initialize the display driver.
    //
    Formike128x128x16Init();

    //
    // Turn on the backlight.
    //
    Formike128x128x16BacklightOn();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike128x128x16);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 14;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "usb_dev_cserial", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Draw the initial status bar.
    //
    DisplayStatus(&g_sContext, "");

    //
    // Show the various static text elements on the color STN display.
    //
    GrContextFontSet(&g_sContext, TEXT_FONT);
    GrStringDraw(&g_sContext, "Tx bytes:", -1, 8, 30, false);
    GrStringDraw(&g_sContext, "Tx buffer:", -1, 8, 45, false);
    GrStringDraw(&g_sContext, "Rx bytes:", -1, 8, 70, false);
    GrStringDraw(&g_sContext, "Rx buffer:", -1, 8, 85, false);
    DrawBufferMeter(&g_sContext, 70, 45);
    DrawBufferMeter(&g_sContext, 70, 85);

    //
    // Enable the UART that we will be redirecting.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Enable and configure the UART RX and TX pins
    //
    SysCtlPeripheralEnable(TX_GPIO_PERIPH);
    SysCtlPeripheralEnable(RX_GPIO_PERIPH);
    GPIOPinTypeUART(TX_GPIO_BASE, TX_GPIO_PIN);
    GPIOPinTypeUART(RX_GPIO_BASE, RX_GPIO_PIN);

    //
    // Set GPIO F0 as an output.  This drives an LED on the board that can
    // be set or cleared by the led command.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);


    //
    // Turn off the LED.
    //
    LEDOff();

    //
    // Set the default UART configuration.
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), DEFAULT_BIT_RATE,
                        DEFAULT_UART_CONFIG);
    UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    //
    // Configure and enable UART interrupts.
    //
    UARTIntClear(UART0_BASE, UARTIntStatus(UART0_BASE, false));
    UARTIntEnable(UART0_BASE, (UART_INT_OE | UART_INT_BE | UART_INT_PE |
                  UART_INT_FE | UART_INT_RT | UART_INT_TX | UART_INT_RX));

    //
    // Configure the USB mux on the board to put us in device mode.  We pull
    // the relevant pin high to do this.
    //
    SysCtlPeripheralEnable(USB_MUX_GPIO_PERIPH);
    GPIOPinTypeGPIOOutput(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN);
    GPIOPinWrite(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN, USB_MUX_SEL_DEVICE);

    //
    // Enable the system tick.
    //
    SysTickPeriodSet(SysCtlClockGet() / SYSTICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Tell the user what we are up to.
    //
    DisplayStatus(&g_sContext, "Configuring USB...");

    //
    // Initialize the transmit and receive buffers for first serial device.
    //
    USBBufferInit((tUSBBuffer *)&g_psTxBuffer[0]);
    USBBufferInit((tUSBBuffer *)&g_psRxBuffer[0]);

    //
    // Initialize the first serial port instances that is part of this
    // composite device.
    //
    g_sCompDevice.psDevices[0].pvInstance =
        USBDCDCCompositeInit(0, (tUSBDCDCDevice *)&g_psCDCDevice[0]);

    //
    // Initialize the transmit and receive buffers for second serial device.
    //
    USBBufferInit((tUSBBuffer *)&g_psTxBuffer[1]);
    USBBufferInit((tUSBBuffer *)&g_psRxBuffer[1]);

    //
    // Initialize the second serial port instances that is part of this
    // composite device.
    //
    g_sCompDevice.psDevices[1].pvInstance =
        USBDCDCCompositeInit(0, (tUSBDCDCDevice *)&g_psCDCDevice[1]);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDCompositeInit(0, &g_sCompDevice, DESCRIPTOR_DATA_SIZE,
                      g_pucDescriptorData);

    //
    // Wait for initial configuration to complete.
    //
    DisplayStatus(&g_sContext, "Waiting for host...");

    //
    // Clear our local byte counters.
    //
    ulRxCount = 0;
    ulTxCount = 0;

    //
    // Set the command index to 0 to start out.
    //
    ulCmdIdx = 0;

    //
    // Enable interrupts now that the application is ready to start.
    //
    IntEnable(INT_UART0);

    //
    // Main application loop.
    //
    while(1)
    {
        if(g_ulFlags & COMMAND_RECEIVED)
        {
            //
            // Clear the flag
            //
            g_ulFlags &= ~COMMAND_RECEIVED;

            //
            // Check if this is the "echo" command, "echo" in hex is 0x6f686365
            // this prevents a more complicated string compare.
            //
            if(0x6f686365 == *(unsigned long *)g_pcCmdBuf)
            {
                //
                // Print out the string.
                //
                iStatus = Echo(g_pcCmdBuf);
            }
            else
            {
                //
                // Process the command line.
                //
                iStatus = CmdLineProcess(g_pcCmdBuf);
            }

            //
            // Handle the case of bad command.
            //
            if(iStatus == CMDLINE_BAD_CMD)
            {
                CommandPrint(g_pcCmdBuf);
                CommandPrint(" is not a valid command!\n");
            }
            CommandPrint("\n> ");
        }

        //
        // Have we been asked to update the status display?
        //
        if(g_ulFlags & COMMAND_STATUS_UPDATE)
        {
            //
            // Clear the command flag
            //
            IntMasterDisable();
            g_ulFlags &= ~COMMAND_STATUS_UPDATE;
            IntMasterEnable();

            DisplayStatus(&g_sContext, g_pcStatus);
        }

        //
        // Has there been any transmit traffic since we last checked?
        //
        if(ulTxCount != g_ulUARTTxCount)
        {
            //
            // Take a snapshot of the latest transmit count.
            //
            ulTxCount = g_ulUARTTxCount;

            //
            // Update the display of bytes transmitted by the UART.
            //
            usnprintf(pcBuffer, 16, "%d", ulTxCount);
            GrStringDraw(&g_sContext, pcBuffer, -1, 70, 30, true);

            //
            // Update the RX buffer fullness. Remember that the buffers are
            // named relative to the USB whereas the status display is from
            // the UART's perspective. The USB's receive buffer is the UART's
            // transmit buffer.
            //
            ulFullness = ((USBBufferDataAvailable(&g_psRxBuffer[0]) * 100) /
                          UART_BUFFER_SIZE);

            UpdateBufferMeter(&g_sContext, ulFullness, 70, 45);
        }

        //
        // Has there been any receive traffic since we last checked?
        //
        if(ulRxCount != g_ulUARTRxCount)
        {
            //
            // Take a snapshot of the latest receive count.
            //
            ulRxCount = g_ulUARTRxCount;

            //
            // Update the display of bytes received by the UART.
            //
            usnprintf(pcBuffer, 16, "%d", ulRxCount);
            GrStringDraw(&g_sContext, pcBuffer, -1, 70, 70, true);

            //
            // Update the TX buffer fullness. Remember that the buffers are
            // named relative to the USB whereas the status display is from
            // the UART's perspective. The USB's transmit buffer is the UART's
            // receive buffer.
            //
            ulFullness = ((USBBufferDataAvailable(&g_psTxBuffer[0]) * 100) /
                          UART_BUFFER_SIZE);

            UpdateBufferMeter(&g_sContext, ulFullness, 70, 85);
        }
    }
}
