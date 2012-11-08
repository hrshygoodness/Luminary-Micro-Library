//*****************************************************************************
//
// usb_dev_mouse.c - Main routines for the mouse device .
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
// This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
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
#include "utils/uartstdio.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "usb_mouse_structs.h"
#include "otg_detect.h"

//*****************************************************************************
//
// The GPIO pin which is connected to the user button.
//
//*****************************************************************************
#define USER_BTN_PORT  GPIO_PORTJ_BASE
#define USER_BTN_PIN   GPIO_PIN_7

//*****************************************************************************
//
// The defines used with the g_ulCommands variable.
//
//*****************************************************************************
#define UPDATE_TICK_EVENT       0x80000000

//*****************************************************************************
//
// The incremental update for the mouse.
//
//*****************************************************************************
#define MOUSE_MOVE_INC          ((char)4)
#define MOUSE_MOVE_DEC          ((char)-4)

//*****************************************************************************
//
// The system tick timer rate.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Holds command bits used to signal the main loop to perform various tasks.
//
//*****************************************************************************
volatile unsigned long g_ulCommands;

//*****************************************************************************
//
// Holds the current state of the touchscreen - pressed or not.
//
//*****************************************************************************
volatile tBoolean g_bScreenPressed;

//*****************************************************************************
//
// Holds the current state of the user button - pressed or not.
//
//*****************************************************************************
volatile tBoolean g_bButtonPressed;

//*****************************************************************************
//
// Holds the previous press position for the touchscreen.
//
//*****************************************************************************
volatile long g_lScreenStartX;
volatile long g_lScreenStartY;

//*****************************************************************************
//
// Holds the current press position for the touchscreen.
//
//*****************************************************************************
volatile long g_lScreenX;
volatile long g_lScreenY;

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
// Global system tick counter hold the g_ulSysTickCount the last time
// GetTickms() was called.
//
//*****************************************************************************
static unsigned long g_ulLastTick;

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
// Graphics context used to show text on the color STN display.
//
//*****************************************************************************
extern tContext g_sContext;

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
        // A report was sent to the host. We are now free to send another.
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

//***************************************************************************
//
// Wait for a period of time for the state to become idle.
//
// \param ulTimeoutTick is the number of system ticks to wait before
// declaring a timeout and returning \b false.
//
// This function polls the current mouse state for ulTimeoutTicks system
// ticks waiting for it to become idle.  If the state becomes idle, the
// function returns true.  If it ulTimeoutTicks occur prior to the state
// becoming idle, false is returned to indicate a timeout.
//
// \return Returns \b true on success or \b false on timeout.
//
//***************************************************************************
tBoolean WaitForSendIdle(unsigned long ulTimeoutTicks)
{
    unsigned long ulStart;
    unsigned long ulNow;
    unsigned long ulElapsed;

    ulStart = g_ulSysTickCount;
    ulElapsed = 0;

    while (ulElapsed < ulTimeoutTicks)
    {
        //
        // Is the mouse is idle, return immediately.
        //
        if (g_eMouseState == MOUSE_STATE_IDLE)
        {
            return (true);
        }

        //
        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ulSysTickCount.  I suspect you
        // won't likely leave the app running for the 497.1 days it will take
        // for this to occur but you never know...
        //
        ulNow = g_ulSysTickCount;
        ulElapsed = (ulStart < ulNow) ? (ulNow - ulStart)
                        : (((unsigned long)0xFFFFFFFF - ulStart) + ulNow + 1);
    }

    //
    // If we get here, we timed out so return a bad return code to let the
    // caller know.
    //
    return (false);
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
UpdateDisplay(unsigned char ucButtons)
{
    //
    // TODO: Write this.
    //
}

//*****************************************************************************
//
// This function handles updates due to the touchscreen and buttons.
//
// This function is called from the main loop each time the touchscreen state
// needs to be checked.  If it detects an update it will schedule an transfer
// to the host.
//
// Returns Returns \b true on success or \b false if an error is detected.
//
//*****************************************************************************
tBoolean
TouchEventHandler(void)
{
    unsigned long ulRetcode;
    long lDeltaX, lDeltaY;
    tBoolean bSuccess;
    tBoolean bBtnPressed;

    //
    // Assume all is well until we determine otherwise.
    //
    bSuccess = true;

    //
    // Get the current state of the user button.
    //
    bBtnPressed = (GPIOPinRead(USER_BTN_PORT, USER_BTN_PIN) & USER_BTN_PIN) ?
                   false : true;

    //
    // Is someone pressing the screen or has the button changed state?  If so,
    // we determine how far they have dragged their finger/stylus and use this
    // to calculate mouse position changes to send to the host.
    //
    if(g_bScreenPressed || (bBtnPressed != g_bButtonPressed))
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
        if(lDeltaX || lDeltaY || (bBtnPressed != g_bButtonPressed))
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
            g_bButtonPressed = bBtnPressed;

            //
            // Send the report back to the host.
            //
            g_eMouseState = MOUSE_STATE_SENDING;
            ulRetcode = USBDHIDMouseStateChange((void *)&g_sMouseDevice,
                                                (char)lDeltaX, (char)lDeltaY,
                                                (bBtnPressed ?
                                                 MOUSE_REPORT_BUTTON_1 : 0));

            //
            // Did we schedule the report for transmission?
            //
            if(ulRetcode == MOUSE_SUCCESS)
            {
                //
                // Wait for the host to acknowledge the transmission if all went
                // well.
                //
                bSuccess = WaitForSendIdle(MAX_SEND_DELAY);

                //
                // Did we time out waiting for the packet to be sent?
                //
                if (!bSuccess)
                {
                    //
                    // Yes - assume the host disconnected and go back to
                    // waiting for a new connection.
                    //
                    DEBUG_PRINT("Send timed out!\n");
                    g_bConnected = 0;
                }
            }
            else
            {
                //
                // An error was reported when trying to send the report. This
                // may be due to host disconnection but could also be due to a
                // clash between our attempt to send a report and the driver
                // sending the last report in response to an idle timer timeout
                // so we don't jump to the conclusion that we were disconnected
                // in this case.
                //
                DEBUG_PRINT("Can't send report.\n");
                bSuccess = false;
            }
        }
    }

    return(bSuccess);
}

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is called
// periodically and updates a global tick counter then sets a flag to tell the
// main loop to check to see if a new HID report should be sent to the host.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    g_ulSysTickCount++;
    g_ulCommands |= UPDATE_TICK_EVENT;
}


//*****************************************************************************
//
// This function is called by the touchscreen driver whenever there is a
// change in press state or position.
//
//*****************************************************************************
long
DeviceMouseTouchCallback(unsigned long ulMessage, long lX, long lY)
{
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
            break;

        //
        // The touchscreen is no longer being pressed.
        //
        case WIDGET_MSG_PTR_UP:
            g_bScreenPressed = false;
            break;

        //
        // The user is dragging his/her finger/stylus over the touchscreen.
        //
        case WIDGET_MSG_PTR_MOVE:
            g_lScreenX = lX;
            g_lScreenY = lY;
            break;
    }

    //
    // Tell the mouse driver we handled the message.
    //
    return(1);
}

//*****************************************************************************
//
// This function initializes the mouse in device mode.
//
//*****************************************************************************
void
DeviceInit(void)
{
    //
    // Initialize the touchscreen driver and install our event handler.
    //
    TouchScreenInit();
    TouchScreenCallbackSet(DeviceMouseTouchCallback);

    //
    // Configure the pin the user button is attached to as an input with a
    // pull-up.
    //
    GPIODirModeSet(USER_BTN_PORT, USER_BTN_PIN, GPIO_DIR_MODE_IN);
    GPIOPadConfigSet(USER_BTN_PORT, USER_BTN_PIN, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    //
    // Set the system tick to fire 100 times per second.
    //
    SysTickPeriodSet(SysCtlClockGet() / SYSTICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Pass the USB library our device information, initialize the USB
    // controller and connect the device to the bus.
    //
    USBDHIDMouseInit(0, (tUSBDHIDMouseDevice *)&g_sMouseDevice);
}

//*****************************************************************************
//
// This is the main loop that runs the mouse device application.
//
//*****************************************************************************
void
DeviceMain(void)
{
    //
    // If it is time to check the touchscreen state then do so.
    //
    if(g_ulCommands & UPDATE_TICK_EVENT)
    {
        g_ulCommands &= ~UPDATE_TICK_EVENT;
        TouchEventHandler();
    }
}

//*****************************************************************************
//
// This function returns the number of ticks since the last time this function
// was called.
//
//*****************************************************************************
unsigned long
GetTickms(void)
{
    unsigned long ulRetVal;
    unsigned long ulSaved;

    ulRetVal = g_ulSysTickCount;
    ulSaved = ulRetVal;

    if(ulSaved > g_ulLastTick)
    {
        ulRetVal = ulSaved - g_ulLastTick;
    }
    else
    {
        ulRetVal = g_ulLastTick - ulSaved;
    }

    //
    // This could miss a few milliseconds but the timings here are on a
    // much larger scale.
    //
    g_ulLastTick = ulSaved;

    //
    // Return the number of milliseconds since the last time this was called.
    //
    return(ulRetVal * MS_PER_SYSTICK);
}
