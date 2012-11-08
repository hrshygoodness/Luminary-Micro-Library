//****************************************************************************
//
// usb_dev_mouse.c - Routines for handling the mouse.
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
// This is part of revision 9453 of the EK-LM3S9D90 Firmware Package.
//
//****************************************************************************

#include "inc/hw_types.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "usb_structs.h"

//****************************************************************************
//
// The mouse device.
//
//****************************************************************************
extern const tUSBDHIDMouseDevice g_sMouseDevice;

//****************************************************************************
//
// The incremental update for the mouse.
//
//****************************************************************************
#define MOUSE_MOVE_INC          1
#define MOUSE_MOVE_DEC          -1

//****************************************************************************
//
// The system tick timer rate.
//
//****************************************************************************
#define SYSTICKS_PER_SECOND     100
#define MS_PER_SYSTICK          (1000 / SYSTICKS_PER_SECOND)

//****************************************************************************
//
// The number of system ticks to wait for each USB packet to be sent before
// we assume the host has disconnected.  The value 50 equates to half a
// second.
//
//****************************************************************************
#define MAX_SEND_DELAY          50

//****************************************************************************
//
// This enumeration holds the various states that the mouse can be in during
// normal operation.
//
//****************************************************************************
volatile enum
{
    //
    // Not configured.
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

//****************************************************************************
//
// This function handles notification messages from the mouse device driver.
//
//****************************************************************************
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
            HWREGBITW(&g_ulFlags, FLAG_CONNECTED) = 1;
            break;
        }

        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            HWREGBITW(&g_ulFlags, FLAG_CONNECTED) = 0;
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

//****************************************************************************
//
// This function provides simulated movements of the mouse.
//
//****************************************************************************
void
MoveHandler(void)
{
    unsigned long ulRetcode;
    char cDeltaX, cDeltaY;

    //
    // Allow the mouse to be still if requested.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_MOVE_MOUSE) == 0)
    {
        return;
    }

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
            HWREGBITW(&g_ulFlags, FLAG_CONNECTED) = 0;
        }
    }
}

//****************************************************************************
//
// The initialization code for the mouse device.
//
//****************************************************************************
void
MouseInit(void)
{
}

//****************************************************************************
//
// This is the main loop that runs the application.
//
//****************************************************************************
void
MouseMain(void)
{
    //
    // Now keep processing the mouse as long as the host is connected.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_CONNECTED) == 1)
    {
        //
        // If it is time to move the mouse then do so.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_MOVE_UPDATE) == 1)
        {
            HWREGBITW(&g_ulFlags, FLAG_MOVE_UPDATE) = 0;
            MoveHandler();
        }
    }
}
