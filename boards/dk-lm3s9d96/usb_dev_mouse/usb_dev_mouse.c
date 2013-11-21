//*****************************************************************************
//
// usb_dev_mouse.c - Main routines for the enumeration example.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/usb.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "utils/uartstdio.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "usb_mouse_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Mouse Device (usb_dev_mouse)</h1>
//!
//! This example application turns the evaluation board into a USB mouse
//! supporting the Human Interface Device class.  Dragging a finger or stylus
//! over the touchscreen translates into mouse movement and presses on marked
//! areas at the bottom of the screen indicate mouse button press. This input
//! is used to generate messages in HID reports sent to the USB host allowing
//! the evaluation board to control the mouse pointer on the host system.
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
// This is the callback from the USB device HID mouse class driver.
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
    GrContextFontSet(&g_sContext, g_pFontCmss18);

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
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
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
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

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
    GrContextFontSet(&g_sContext, g_pFontCm20);
    GrStringDrawCentered(&g_sContext, "usb-dev-mouse", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 10, 0);

    //
    // Draw the buttons in their initial (unpressed)state.
    //
    UpdateDisplay(g_ucButtons, true);

    //
    // Set the USB stack mode to Device mode with VBUS monitoring.
    //
    USBStackModeSet(0, USB_MODE_DEVICE, 0);

    //
    // Pass the USB library our device information, initialize the USB
    // controller and connect the device to the bus.
    //
    USBDHIDMouseInit(0, (tUSBDHIDMouseDevice *)&g_sMouseDevice);

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
    while(1)
    {
        //
        // Tell the user what we are doing.
        //
        GrContextFontSet(&g_sContext, g_pFontCmss22b);
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
        // Now keep processing the mouse as long as the host is connected.
        //
        while(g_bConnected)
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
        // If we drop out of the previous loop, the host has disconnected so
        // go back and wait for a new connection.
        //
    }
}
