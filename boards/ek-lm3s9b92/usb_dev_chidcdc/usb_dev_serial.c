//****************************************************************************
//
// usb_dev_serial.c - Routines for handling the USB CDC serial device.
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
// This is part of revision 6075 of the EK-LM3S9B92 Firmware Package.
//
//****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "utils/cmdline.h"
#include "usb_structs.h"

//****************************************************************************
//
// The USB transmit and receive buffer objects.
//
//****************************************************************************
extern const tUSBBuffer g_sTxBuffer;
extern const tUSBBuffer g_sRxBuffer;

//****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//****************************************************************************
#define CMD_BUF_SIZE            64

//****************************************************************************
//
// The buffer that holds the command line.
//
//****************************************************************************
static char g_pcCmdBuf[CMD_BUF_SIZE];
static unsigned long ulCmdIdx;

//****************************************************************************
//
// Character sequence sent to the serial terminal to implement a character
// erase when backspace is pressed.
//
//****************************************************************************
static const char g_pcBackspace[3] = {0x08, ' ', 0x08};

//****************************************************************************
//
// The current serial format information.
//
//****************************************************************************
tLineCoding g_sLineCoding=
{
    //
    // 115200 baud rate.
    //
    115200,

    //
    // 1 Stop Bit.
    //
    1,

    //
    // No Parity.
    //
    0,

    //
    // 8 Bits of data.
    //
    8
};

//****************************************************************************
//
// The LED control macros.
//
//****************************************************************************
#define LEDOn()  ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0)
#define LEDOff() ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0)
#define LEDToggle()                                                         \
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0,                           \
                     (ROM_GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_0) ^        \
                                      GPIO_PIN_0));

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
        while(USBBufferSpaceAvailable(&g_sTxBuffer) < 2)
        {
        }

        //
        // Print the next character.
        //
        USBBufferWrite(&g_sTxBuffer, (const unsigned char *)&pcStr[iIdx], 1);

        //
        // If this is a line feed then send a carriage return as well.
        //
        if(pcStr[iIdx] == 0xa)
        {
            USBBufferWrite(&g_sTxBuffer, (const unsigned char *)&cCR, 1);
        }

        iIdx++;
    }
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
SerialHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
              void *pvMsgData)
{
    tLineCoding *psLineCoding;

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

            //
            // Flush our buffers.
            //
            USBBufferFlush(&g_sTxBuffer);
            USBBufferFlush(&g_sRxBuffer);

            break;
        }

        //
        // Return the current serial communication parameters.
        //
        case USBD_CDC_EVENT_GET_LINE_CODING:
        {
            //
            // Create a pointer to the line coding information.
            //
            psLineCoding = (tLineCoding *)pvMsgData;

            //
            // Copy the current line coding information into the structure.
            //
            psLineCoding->ulRate = g_sLineCoding.ulRate;
            psLineCoding->ucStop = g_sLineCoding.ucStop;
            psLineCoding->ucParity = g_sLineCoding.ucParity;
            psLineCoding->ucDatabits = g_sLineCoding.ucDatabits;
            break;
        }

        //
        // Set the current serial communication parameters.
        //
        case USBD_CDC_EVENT_SET_LINE_CODING:
        {
            //
            // Create a pointer to the line coding information.
            //
            psLineCoding = (tLineCoding *)pvMsgData;

            //
            // Copy the line coding information into the current line coding
            // structure.
            //
            g_sLineCoding.ulRate = psLineCoding->ulRate;
            g_sLineCoding.ucStop = psLineCoding->ucStop;
            g_sLineCoding.ucParity = psLineCoding->ucParity;
            g_sLineCoding.ucDatabits = psLineCoding->ucDatabits;
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
        // All other events are ignored.
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
// \param pvCBData is the client-supplied callback pointer for this channel.
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
TxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
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
            // Toggle the LED if in activity mode.
            //
            if(HWREGBITW(&g_ulFlags, FLAG_LED_ACTIVITY))
            {
                //
                // Toggle the LED.
                //
                LEDToggle();
            }

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
// \param pvCBData is the client-supplied callback data value for this
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
RxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
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
            // Keep reading and processing characters as long as there are new
            // ones in the buffer.
            //
            while(USBBufferRead(pBufferRx,
                                (unsigned char *)&g_pcCmdBuf[ulCmdIdx], 1))
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
                        USBBufferWrite(pBufferTx, (unsigned char *)g_pcBackspace,
                                       3);
                    }
                }
                else
                {
                    //
                    // Feed some the new character into the UART TX FIFO.
                    //
                    USBBufferWrite(pBufferTx,
                                   (unsigned char *)&g_pcCmdBuf[ulCmdIdx], 1);

                    //
                    // If this was a line feed then put out a carriage return
                    // as well.
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
                        HWREGBITW(&g_ulFlags, FLAG_COMMAND_RECEIVED) = 1;

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
// This command allows starting or stopping the mouse from moving.
//
// The first argument should be one of the following:
// on     - Move the mouse in a pattern.
// off    - Stop moving the mouse.
//
//****************************************************************************
int
Cmd_mouse(int argc, char *argv[])
{
    //
    // These values only check the second character since all parameters are
    // different in that character.
    //
    if(argv[1][1] == 'n')
    {
        //
        // Start moving the mouse.
        //
        HWREGBITW(&g_ulFlags, FLAG_MOVE_MOUSE) = 1;
    }
    else if(argv[1][1] == 'f')
    {
        //
        // Start moving the mouse.
        //
        HWREGBITW(&g_ulFlags, FLAG_MOVE_MOUSE) = 0;
    }
    else
    {
        //
        // The command format was not correct so print out some help.
        //
        CommandPrint("\nmouse <on|off>\n");
        CommandPrint("  on  - Mouse will start moving in a square pattern.\n");
        CommandPrint("  off - Mouse will stop moving.\n");
    }

    return(0);
}

//****************************************************************************
//
// This command allows setting, clearing or toggling the Status LED.
//
// The first argument should be one of the following:
// on     - Turn on the LED.
// off    - Turn off the LEd.
// toggle - Toggle the current LED status.
// activity - Set the LED mode to monitor serial activity.
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

        //
        // Switch off activity mode.
        //
        HWREGBITW(&g_ulFlags, FLAG_LED_ACTIVITY) = 0;
    }
    else if(argv[1][1] == 'f')
    {
        //
        // Turn off the LED.
        //
        LEDOff();

        //
        // Switch off activity mode.
        //
        HWREGBITW(&g_ulFlags, FLAG_LED_ACTIVITY) = 0;
    }
    else if(argv[1][1] == 'o')
    {
        //
        // Toggle the LED.
        //
        LEDToggle();

        //
        // Switch off activity mode.
        //
        HWREGBITW(&g_ulFlags, FLAG_LED_ACTIVITY) = 0;
    }
    else if(argv[1][1] == 'c')
    {
        //
        // If this is the "activity" value then set the activity mode.
        //
        HWREGBITW(&g_ulFlags, FLAG_LED_ACTIVITY) = 1;
    }
    else
    {
        //
        // The command format was not correct so print out some help.
        //
        CommandPrint("\nled <on|off|toggle|activity>\n");
        CommandPrint("  on       - Turn on the LED.\n");
        CommandPrint("  off      - Turn off the LED.\n");
        CommandPrint("  toggle   - Toggle the LED state.\n");
        CommandPrint("  activity - LED state will toggle on UART activity.\n");
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
    { "help",  Cmd_help,     "  : Display list of commands" },
    { "h",     Cmd_help,  "     : alias for help" },
    { "?",     Cmd_help,  "     : alias for help" },
    { "mouse", Cmd_mouse,     " : Turn (on|off) mouse movements" },
    { "led",   Cmd_led,     "   : Set LED mode (on|off|toggle|activity)" },
    { 0, 0, 0 }
};

//****************************************************************************
//
// This is the serial initialization routine.
//
//****************************************************************************
void
SerialInit(void)
{
    //
    // Set GPIO D0 as an output.  This drives an LED on the board that can
    // be set or cleared by the led command.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0);
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0);

    //
    // Initialize the transmit and receive buffers for first serial device.
    //
    USBBufferInit((tUSBBuffer *)&g_sTxBuffer);
    USBBufferInit((tUSBBuffer *)&g_sRxBuffer);
}

//****************************************************************************
//
// This is the main loop serial handling function.
//
//****************************************************************************
void
SerialMain(void)
{
    int iStatus;

    //
    // Main application loop.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_COMMAND_RECEIVED))
    {
        //
        // Clear the flag
        //
        HWREGBITW(&g_ulFlags, FLAG_COMMAND_RECEIVED) = 0;

        //
        // Process the command line.
        //
        iStatus = CmdLineProcess(g_pcCmdBuf);

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
}
