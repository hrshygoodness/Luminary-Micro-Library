//*****************************************************************************
//
// boot_demo_usb.c - Main routines for the USB HID/DFU composite device example.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/usb.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/device/usbddfu-rt.h"
#include "usblib/device/usbdcomp.h"
#include "utils/uartstdio.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "usb_hiddfu_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Boot Loader Example (boot_demo_usb)</h1>
//!
//! This example application is used in conjunction with the USB boot loader
//! (boot_usb) and turns the evaluation board into a composite device
//! supporting a mouse via the Human Interface Device class and also publishing
//! runtime Device Firmware Upgrade (DFU) capability.  Dragging a finger or
//! stylus over the touchscreen translates into mouse movement and presses on
//! marked areas at the bottom of the screen indicate mouse button press.  This
//! input is used to generate messages in HID reports sent to the USB host
//! allowing the evaluation board to control the mouse pointer on the host
//! system.
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
//! <tt>C:/Program Files/Texas Instruments/Stellaris/usb_examples</tt> directory.
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
//
//*****************************************************************************

//*****************************************************************************
//
// Debug-related definitions and declarations.
//
// Debug output is available via UART0 if DEBUG is defined during build.
//
//*****************************************************************************
#ifdef DEBUG
//*****************************************************************************
//
// Map all debug print calls to UARTprintf in debug builds.
//
//*****************************************************************************
#define DEBUG_PRINT UARTprintf

#else

//*****************************************************************************
//
// Compile out all debug print calls in release builds.
//
//*****************************************************************************
#define DEBUG_PRINT while(0) ((int (*)(char *, ...))0)
#endif

//*****************************************************************************
//
// The defines used with the g_ulCommands variable.
//
//*****************************************************************************
#define TOUCH_TICK_EVENT       0x80000000

//*****************************************************************************
//
// The system tick timer rate.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 50

//*****************************************************************************
//
// This structure defines the area of the display that is devoted to a
// mouse button.  Touchscreen input in this area is translated into press and
// release messages for the given button.
//
//*****************************************************************************
typedef struct
{
    const char *pszLabel;
    unsigned short usX;
    unsigned short usWidth;
    unsigned char  ucReportFlag;
}
tMouseButtonArea;

//*****************************************************************************
//
// The height of the mouse button bar at the bottom of the display and the
// number of buttons it contains.
//
//*****************************************************************************
#define BUTTON_HEIGHT 30
#define NUM_MOUSE_BUTTONS 3

//*****************************************************************************
//
// Definitions of the positions and labels for each of the three mouse buttons.
//
//*****************************************************************************
static tMouseButtonArea g_sMouseButtons[NUM_MOUSE_BUTTONS] =
{
    { "Button 1", 0,   107, MOUSE_REPORT_BUTTON_1 },
    { "Button 2", 106, 108, MOUSE_REPORT_BUTTON_2 },
    { "Button 3", 213, 107, MOUSE_REPORT_BUTTON_3 }
};

//*****************************************************************************
//
// Holds command bits used to signal the main loop to perform various tasks.
//
//*****************************************************************************
volatile unsigned long g_ulCommands;

//*****************************************************************************
//
// A flag used to indicate whether or not we are currently connected to the USB
// host.
//
//*****************************************************************************
volatile tBoolean g_bConnected;

//*****************************************************************************
//
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
//
//*****************************************************************************
volatile unsigned long g_ulSysTickCount;

//*****************************************************************************
//
// Holds the previous press position for the touchscreen.
//
//*****************************************************************************
static volatile long g_lScreenStartX;
static volatile long g_lScreenStartY;

//*****************************************************************************
//
// Holds the current press position for the touchscreen.
//
//*****************************************************************************
static volatile long g_lScreenX;
static volatile long g_lScreenY;

//*****************************************************************************
//
// Holds the current state of the touchscreen - pressed or not.
//
//*****************************************************************************
static volatile tBoolean g_bScreenPressed;

//*****************************************************************************
//
// Holds the current state of the push button - pressed or not.
//
//*****************************************************************************
static volatile unsigned char g_ucButtons;

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
// Graphics context used to show text on the color STN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Flag used to tell the main loop that it's time to pass control back to the
// boot loader for an update.
//
//*****************************************************************************
volatile tBoolean g_bUpdateSignalled;

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
// This function is called by the touchscreen driver whenever there is a
// change in press state or position.
//
//*****************************************************************************
static long
MouseTouchHandler(unsigned long ulMessage, long lX, long lY)
{
    unsigned long ulLoop;

    switch(ulMessage)
    {
        //
        // The touchscreen has been pressed.  Remember where we are so that
        // we can determine how far the pointer moves later.
        //
        case WIDGET_MSG_PTR_DOWN:
            g_lScreenStartX = lX;
            g_lScreenStartY = lY;
            g_lScreenX = lX;
            g_lScreenY = lY;
            g_bScreenPressed = true;

            //
            // Is the press within the button area?  If so, determine which
            // button has been pressed.
            //
            if(lY >= (GrContextDpyHeightGet(&g_sContext) - BUTTON_HEIGHT))
            {
                //
                // Run through the list of buttons to determine which one was
                // pressed.
                //
                for(ulLoop = 0; ulLoop < NUM_MOUSE_BUTTONS; ulLoop++)
                {
                    if((lX >= g_sMouseButtons[ulLoop].usX) &&
                       (lX < (g_sMouseButtons[ulLoop].usX +
                        g_sMouseButtons[ulLoop].usWidth)))
                    {
                        g_ucButtons |= g_sMouseButtons[ulLoop].ucReportFlag;
                        break;
                    }
                }
            }
            break;

        //
        // The touchscreen is no longer being pressed.
        //
        case WIDGET_MSG_PTR_UP:
            g_bScreenPressed = false;

            //
            // Ensure that all buttons are unpressed.
            //
            g_ucButtons = 0;
            break;

        //
        // The user is dragging his/her finger/stylus over the touchscreen.
        //
        case WIDGET_MSG_PTR_MOVE:
            g_lScreenX = lX;
            g_lScreenY = lY;
            break;
    }

    return(0);
}

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
// This is the callback from the USB composite device class driver.
//
// \param pvCBData is ignored by this function.
// \param ulEvent is one of the valid events for a mouse device.
// \param ulMsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a change occurs
// during operation as a HID class USB mouse device.
//
// \return This function will return 0.
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
            DEBUG_PRINT("Host connected.\n");
            g_eMouseState = MOUSE_STATE_IDLE;
            g_bConnected = true;
            break;
        }

        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            DEBUG_PRINT("Host disconnected.\n");
            g_bConnected = false;
            g_eMouseState = MOUSE_STATE_UNCONFIGURED;
            break;
        }

        //
        // A report was sent to the host. We are not free to send another.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            DEBUG_PRINT("TX complete.\n");
            g_eMouseState = MOUSE_STATE_IDLE;
            break;
        }

    }
    return(0);
}

//*****************************************************************************
//
// This function updates the color STN display to show button state.
//
// This function is called from ButtonHandler to update the display showing
// the state of each of the buttons.
//
// Returns None.
//
//*****************************************************************************
void
UpdateDisplay(unsigned char ucButtons, tBoolean bRedraw)
{
    unsigned long ulLoop;
    tRectangle sRect, sRectOutline;
    static unsigned char ucLastButtons;

    //
    // Initialize the Y coordinates of the button rectangle.
    //
    sRectOutline.sYMin = GrContextDpyHeightGet(&g_sContext) - BUTTON_HEIGHT;
    sRectOutline.sYMax = GrContextDpyHeightGet(&g_sContext) - 1;
    sRect.sYMin = sRectOutline.sYMin + 1;
    sRect.sYMax = sRectOutline.sYMax - 1;

    //
    // Set the font we use for the button text.
    //
    GrContextFontSet(&g_sContext, &g_sFontCmss18);

    //
    // Loop through each of the mouse buttons, drawing each in turn.
    //
    for(ulLoop = 0; ulLoop < NUM_MOUSE_BUTTONS; ulLoop++)
    {
        //
        // Draw the outline if we are redrawing the whole button area.
        //
        if(bRedraw)
        {
            GrContextForegroundSet(&g_sContext, ClrWhite);

            sRectOutline.sXMin = g_sMouseButtons[ulLoop].usX;
            sRectOutline.sXMax = (sRectOutline.sXMin +
                                 g_sMouseButtons[ulLoop].usWidth) - 1;

            GrRectDraw(&g_sContext, &sRectOutline);
        }

        //
        // Has the button state changed since we last drew it or are we
        // drawing the buttons unconditionally?
        //
        if(((g_ucButtons & g_sMouseButtons[ulLoop].ucReportFlag) !=
           (ucLastButtons & g_sMouseButtons[ulLoop].ucReportFlag)) || bRedraw)
        {
            //
            // Set the appropriate button color depending upon whether the
            // button is pressed or not.
            //
            GrContextForegroundSet(&g_sContext, ((g_ucButtons &
                                   g_sMouseButtons[ulLoop].ucReportFlag) ?
                                   ClrRed : ClrGreen));

            sRect.sXMin = g_sMouseButtons[ulLoop].usX + 1;
            sRect.sXMax = (sRect.sXMin + g_sMouseButtons[ulLoop].usWidth) - 3;
            GrRectFill(&g_sContext, &sRect);

            //
            // Draw the button text.
            //
            GrContextForegroundSet(&g_sContext, ClrWhite);
            GrStringDrawCentered(&g_sContext, g_sMouseButtons[ulLoop].pszLabel,
                                 -1, (sRect.sXMin + sRect.sXMax) / 2,
                                 (sRect.sYMin + sRect.sYMax) / 2, 0);
        }
    }

    //
    // Remember the button state we just drew.
    //
    ucLastButtons = ucButtons;
}

//*****************************************************************************
//
// This function handles updates due to touchscreen input.
//
// This function is called periodically from the main loop to check the
// touchscreen state and, if necessary, send a HID report back to the host
// system.
//
// Returns Returns \b true on success or \b false if an error is detected.
//
//*****************************************************************************
static void
TouchHandler(void)
{
    long lDeltaX, lDeltaY;
    static unsigned char ucButtons = 0;

    //
    // Is someone pressing the screen or has the button changed state?  If so,
    // we determine how far they have dragged their finger/stylus and use this
    // to calculate mouse position changes to send to the host.
    //
    if(g_bScreenPressed || (ucButtons != g_ucButtons))
    {
        //
        // Calculate how far we moved since the last time we checked.  This
        // rather odd layout prevents a compiler warning about undefined order
        // of volatile accesses.
        //
        lDeltaX = g_lScreenX;
        lDeltaX -= g_lScreenStartX;
        lDeltaY = g_lScreenY;
        lDeltaY -= g_lScreenStartY;

        //
        // Reset our start position.
        //
        g_lScreenStartX = g_lScreenX;
        g_lScreenStartY = g_lScreenY;

        //
        // Was there any movement or change in button state?
        //
        if(lDeltaX || lDeltaY || (ucButtons != g_ucButtons))
        {
            //
            // Yes - send a report back to the host after clipping the deltas
            // to the maximum we can support.
            //
            lDeltaX = (lDeltaX > 127) ? 127 : lDeltaX;
            lDeltaX = (lDeltaX < -128) ? -128 : lDeltaX;
            lDeltaY = (lDeltaY > 127) ? 127 : lDeltaY;
            lDeltaY = (lDeltaY < -128) ? -128 : lDeltaY;

            //
            // Remember the current button state.
            //
            ucButtons = g_ucButtons;

            //
            // Send the report back to the host.
            //
            USBDHIDMouseStateChange((void *)&g_sMouseDevice,
                                    (char)lDeltaX, (char)lDeltaY,
                                    ucButtons);
        }

        //
        // Update the button portion of the display.
        //
        UpdateDisplay(ucButtons, false);
    }
}

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is called
// periodically and updates a global tick counter then sets a flag to tell the
// main loop to check the button state.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    g_ulSysTickCount++;
    g_ulCommands |= TOUCH_TICK_EVENT;
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

#ifdef DEBUG
    //
    // Open UART0 for debug output.
    //
    UARTStdioInit(0);
#endif

    //
    // Set the system tick to fire 100 times per second.
    //
    SysTickPeriodSet(SysCtlClockGet() / SYSTICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 23;
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
    GrContextFontSet(&g_sContext, &g_sFontCm20);
    GrStringDrawCentered(&g_sContext, "usb-dev-mouse", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 10, 0);

    //
    // Draw the buttons in their initial (unpressed)state.
    //
    UpdateDisplay(g_ucButtons, true);

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
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(MouseTouchHandler);

    //
    // Drop into the main loop.
    //
    while(!g_bUpdateSignalled)
    {
        //
        // Tell the user what we are doing.
        //
        GrContextFontSet(&g_sContext, &g_sFontCmss22b);
        GrContextForegroundSet(&g_sContext, ClrWhite);
        GrStringDrawCentered(&g_sContext, "   Waiting for host...   ", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 40, true);

        //
        // Wait for USB configuration to complete.
        //
        while(!g_bConnected)
        {
        }

        //
        // Update the status.
        //
        GrStringDrawCentered(&g_sContext, "   Host connected...   ", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 40, true);

        //
        // Now keep processing the mouse as long as the host is connected and
        // we've not been told to prepare for a firmware upgrade.
        //
        while(g_bConnected && !g_bUpdateSignalled)
        {
            //
            // If it is time to check the touchscreen state then do so.
            //
            if(g_ulCommands & TOUCH_TICK_EVENT)
            {
                g_ulCommands &= ~TOUCH_TICK_EVENT;
                TouchHandler();
            }
        }

        //
        // If we drop out of the previous loop, either the host has
        // disconnected or a firmware upgrade has been signalled.
        //
    }

    //
    // Tell the user what's going on.
    //
    GrContextFontSet(&g_sContext, &g_sFontCmss22b);
    GrStringDrawCentered(&g_sContext, " Switching to DFU mode ", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 40, true);

    //
    // If we get here, a firmware upgrade has been signalled so we need to get
    // back into the boot loader to allow this to happen.  Call the USB DFU
    // device class to do this for us.  Note that this function never returns.
    //
    USBDDFUUpdateBegin();
}
