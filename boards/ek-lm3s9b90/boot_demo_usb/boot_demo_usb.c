//*****************************************************************************
//
// boot_demo_usb.c - Main routines for the USB HID/DFU composite device
//                   example.
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
// This is part of revision 8555 of the EK-LM3S9B90 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/device/usbddfu-rt.h"
#include "usblib/device/usbdcomp.h"
#include "utils/uartstdio.h"
#include "usb_mousedfu_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Boot Loader Example (boot_demo_usb)</h1>
//!
//! This example application is used in conjunction with the USB boot loader
//! (boot_usb) and turns the evaluation board into a composite device
//! supporting a mouse via the Human Interface Device class and also publishing
//! runtime Device Firmware Upgrade (DFU) capability.  Make sure that the USB
//! boot loader is flashed at address 0 and that the binary for this
//! application is placed at 0x1800.  When connected to a host system, the
//! application acts as a mouse and moves the pointer in a square pattern for
//! the duration of the time it is plugged in.
//!
//! Since the device also publishes a DFU interface, host software such as the
//! dfuprog tool can determine that the device is capable of receiving software
//! updates over USB.  The runtime DFU protocol allows such tools to signal the
//! device to switch into DFU mode and prepare to receive a new software image.
//!
//! Runtime DFU functionality requires only that the device listen for a
//! particular request (DETACH) from the host and, when this is received,
//! transfer control to the USB boot loader via the normal means to reenumerate
//! as a pure DFU device capable of uploading and downloading firmware images.
//!
//! Windows device drivers for both the runtime and DFU mode of operation can
//! be found in C:/StellarisWare/windows_drivers assuming you installed
//! StellarisWare in the default directory.
//!
//! To illustrate runtime DFU capability, use the <tt>dfuprog</tt> tool which
//! is part of the Stellaris Windows USB Examples package (SW-USB-win-xxxx.msi)
//! Assuming this package is installed in the default location, the
//! <tt>dfuprog</tt> executable can be found in the
//! <tt>C:/Program Files/Texas Instruments/Stellaris/usb_examples</tt>
//! directory.
//!
//! With the device connected to your PC and the device driver installed, enter
//! the following command to enumerate DFU devices:
//!
//! <tt>dfuprog -e</tt>
//!
//! This will list all DFU-capable devices found and you should see that you
//! have one device available which is in ``Runtime'' mode.  Entering the
//! following command will switch this device into DFU mode and leave it ready
//! to receive a new firmware image:
//!
//! <tt>dfuprog -m</tt>
//!
//! After entering this command, you should notice that the device disconnects
//! from the USB bus and reconnects again. Running ``<tt>dfuprog -e</tt>'' a
//! second time will show that the device is now in DFU mode and ready to
//! receive downloads.  At this point, either LM Flash Programmer or dfuprog
//! may be used to send a new application binary to the device.
//!
//! UART0, connected to the FTDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// The incremental update for the mouse.
//
//*****************************************************************************
#define MOUSE_MOVE_INC          1
#define MOUSE_MOVE_DEC          -1

//*****************************************************************************
//
// The system tick timer rate.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND     100
#define MS_PER_SYSTICK          (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Holds command bits used to signal the main loop to perform various tasks.
//
//*****************************************************************************
volatile unsigned long g_ulCommands;
#define TICK_EVENT              0

//*****************************************************************************
//
// A flag used to indicate whether or not we are currently connected to the USB
// host.
//
//*****************************************************************************
volatile tBoolean g_bConnected;

//*****************************************************************************
//
// Flag used to tell the main loop that it's time to pass control back to the
// boot loader for an update.
//
//*****************************************************************************
volatile tBoolean g_bUpdateSignalled;

//*****************************************************************************
//
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
//
//*****************************************************************************
volatile unsigned long g_ulSysTickCount;

//*****************************************************************************
//
// The number of system ticks to wait for each USB packet to be sent before
// we assume the host has disconnected.  The value 50 equates to half a second.
//
//*****************************************************************************
#define MAX_SEND_DELAY          50

//*****************************************************************************
//
// This enumeration holds the various states that the mouse can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Unconfigured.
    //
    MOUSE_STATE_UNCONFIGURED,

    //
    // No keys to send and not waiting on data.
    //
    MOUSE_STATE_IDLE,

    //
    // Waiting on data to be sent out.
    //
    MOUSE_STATE_SENDING
}
g_eMouseState = MOUSE_STATE_UNCONFIGURED;

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
// This is the callback from the USB DFU runtime interface driver.
//
// \param pvCBData is ignored by this function.
// \param ulEvent is one of the valid events for a DFU device.
// \param ulMsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a change occurs
// during operation as a DFU device.  Currently, the only event passed to this
// callback is USBD_DFU_EVENT_DETACH which tells the recipient that they should
// pass control to the boot loader at the earliest, non-interrupt context
// point.
//
// \return This function will return 0.
//
//*****************************************************************************
unsigned long
DFUDetachCallback(void *pvCBData, unsigned long ulEvent,
                  unsigned long ulMsgData, void *pvMsgData)
{
    if(ulEvent == USBD_DFU_EVENT_DETACH)
    {
        //
        // Set the flag that the main loop uses to determine when it is time
        // to transfer control back to the boot loader.  Note that we
        // absolutely DO NOT call USBDDFUUpdateBegin() here since we are
        // currently in interrupt context and this would cause bad things to
        // happen (and the boot loader to not work).
        //
        g_bUpdateSignalled = true;
    }

    return(0);
}

//*****************************************************************************
//
// This function handles notification messages from the mouse device driver.
//
//*****************************************************************************
unsigned long
MouseHandler(void *pvCBData, unsigned long ulEvent,
             unsigned long ulMsgData, void *pvMsgData)
{
    switch(ulEvent)
    {
        //
        // The USB host has connected to and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_eMouseState = MOUSE_STATE_IDLE;
            g_bConnected = true;
            break;
        }

        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_bConnected = false;
            g_eMouseState = MOUSE_STATE_UNCONFIGURED;
            break;
        }

        //
        // A report was sent to the host.  We are not free to send another.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            g_eMouseState = MOUSE_STATE_IDLE;
            break;
        }

    }

    return(0);
}

//***************************************************************************
//
// Wait for a period of time for the state to become idle.
//
// \param ulTimeoutTick is the number of system ticks to wait before
// declaring a timeout and returning \b false.
//
// This function polls the current keyboard state for ulTimeoutTicks system
// ticks waiting for it to become idle.  If the state becomes idle, the
// function returns true.  If it ulTimeoutTicks occur prior to the state
// becoming idle, false is returned to indicate a timeout.
//
// \return Returns \b true on success or \b false on timeout.
//
//***************************************************************************
tBoolean
WaitForSendIdle(unsigned long ulTimeoutTicks)
{
    unsigned long ulStart;
    unsigned long ulNow;
    unsigned long ulElapsed;

    ulStart = g_ulSysTickCount;
    ulElapsed = 0;

    while(ulElapsed < ulTimeoutTicks)
    {
        //
        // Is the mouse is idle, return immediately.
        //
        if(g_eMouseState == MOUSE_STATE_IDLE)
        {
            return(true);
        }

        //
        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ulSysTickCount.
        //
        ulNow = g_ulSysTickCount;
        ulElapsed = ((ulStart < ulNow) ? (ulNow - ulStart) :
                     (((unsigned long)0xFFFFFFFF - ulStart) + ulNow + 1));
    }

    //
    // If we get here, we timed out so return a bad return code to let the
    // caller know.
    //
    return(false);
}

//*****************************************************************************
//
// This function provides simulated movements of the mouse.
//
//*****************************************************************************
void
MoveHandler(void)
{
    unsigned long ulRetcode;
    char cDeltaX, cDeltaY;

    //
    // Determine the direction to move the mouse.
    //
    ulRetcode = g_ulSysTickCount % (4 * SYSTICKS_PER_SECOND);
    if(ulRetcode < SYSTICKS_PER_SECOND)
    {
        cDeltaX = MOUSE_MOVE_INC;
        cDeltaY = 0;
    }
    else if(ulRetcode < (2 * SYSTICKS_PER_SECOND))
    {
        cDeltaX = 0;
        cDeltaY = MOUSE_MOVE_INC;
    }
    else if(ulRetcode < (3 * SYSTICKS_PER_SECOND))
    {
        cDeltaX = (char)MOUSE_MOVE_DEC;
        cDeltaY = 0;
    }
    else
    {
        cDeltaX = 0;
        cDeltaY = (char)MOUSE_MOVE_DEC;
    }

    //
    // Tell the HID driver to send this new report.
    //
    g_eMouseState = MOUSE_STATE_SENDING;
    ulRetcode = USBDHIDMouseStateChange((void *)&g_sMouseDevice, cDeltaX,
                                        cDeltaY, 0);

    //
    // Did we schedule the report for transmission?
    //
    if(ulRetcode == MOUSE_SUCCESS)
    {
        //
        // Wait for the host to acknowledge the transmission if all went well.
        //
        if(!WaitForSendIdle(MAX_SEND_DELAY))
        {
            //
            // The transmission failed, so assume the host disconnected and go
            // back to waiting for a new connection.
            //
            g_bConnected = false;
        }
    }
}

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is called
// periodically and updates a global tick counter then sets a flag to tell the
// main loop to move the mouse.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    g_ulSysTickCount++;
    HWREGBITW(&g_ulCommands, TICK_EVENT) = 1;
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run from the PLL at 50MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Enable the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
    UARTprintf("\033[2JMouse device application\n");

    //
    // Set the system tick to fire 100 times per second.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Initialize each of the device instances that will form our composite
    // USB device.
    //
    g_sCompDevice.psDevices[0].pvInstance =
        USBDHIDMouseCompositeInit(0, (tUSBDHIDMouseDevice *)&g_sMouseDevice);
    g_sCompDevice.psDevices[1].pvInstance =
        USBDDFUCompositeInit(0, (tUSBDDFUDevice *)&g_sDFUDevice);

    //
    // Pass the USB library our device information, initialize the USB
    // controller and connect the device to the bus.
    //
    USBDCompositeInit(0, &g_sCompDevice, DESCRIPTOR_BUFFER_SIZE,
                      g_pcDescriptorBuffer);

    //
    // Drop into the main loop.
    //
    while(!g_bUpdateSignalled)
    {
        //
        // Tell the user what we are doing.
        //
        UARTprintf("Waiting for host...\n");

        //
        // Wait for USB configuration to complete.
        //
        while(!g_bConnected)
        {
        }

        //
        // Update the status.
        //
        UARTprintf("Host connected...\n");

        //
        // Now keep processing the mouse as long as the host is connected and
        // we've not been told to prepare for a firmware upgrade.
        //
        while(g_bConnected && !g_bUpdateSignalled)
        {
            //
            // If it is time to move the mouse then do so.
            //
            if(HWREGBITW(&g_ulCommands, TICK_EVENT) == 1)
            {
                HWREGBITW(&g_ulCommands, TICK_EVENT) = 0;
                MoveHandler();
            }
        }
    }

    //
    // Tell the user what's going on then wait about a second before jumping
    // back into the boot loader.
    //
    UARTprintf("Switching to DFU mode for firmware upgrade...\n");
    SysCtlDelay(ROM_SysCtlClockGet() / 3);

    //
    // If we get here, a firmware upgrade has been signalled so we need to get
    // back into the boot loader to allow this to happen.  Call the USB DFU
    // device class to do this for us.  Note that this function never returns.
    //
    USBDDFUUpdateBegin();
}
