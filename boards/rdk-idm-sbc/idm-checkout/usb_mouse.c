//*****************************************************************************
//
// usb_mouse.c - Support functions for supporting USB mouse both as a device
//               and as a host.
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
// This is part of revision 9453 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidmouse.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "idm-checkout.h"
#include "usb_mouse.h"

//*****************************************************************************
//
// The polling interval we use between calls to the OTG library.
//
//*****************************************************************************
#define OTG_POLL_INTERVAL_MS 50

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
unsigned char g_pHCDPool[HCD_MEMORY_SIZE];

//*****************************************************************************
//
// The size of the mouse device interface's memory pool in bytes.
//
//*****************************************************************************
#define MOUSE_MEMORY_SIZE       128

//*****************************************************************************
//
// The memory pool to provide to the mouse device.
//
//*****************************************************************************
unsigned char g_pucBuffer[MOUSE_MEMORY_SIZE];

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Mouse class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_USBHIDClassDriver
};

//*****************************************************************************
//
// This enumerated type is used to hold the states of the mouse during USB host
// operation.
//
//*****************************************************************************
typedef enum
{
    //
    // No USB device is connected to us.
    //
    STATE_NO_CONNECTION,

    //
    // A HID mouse device has been connected to us and is awaiting
    // initialization prior to use.
    //
    STATE_HOST_CONNECTING,

    //
    // A HID mouse device is attached.
    //
    STATE_MOUSE_CONNECTED
}
tUSBState;

tUSBState g_eUSBState = STATE_NO_CONNECTION;

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
static const unsigned long g_ulNumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

//*****************************************************************************
//
// The global value used to store the mouse value.
//
//*****************************************************************************
static unsigned long g_ulMouseInstance;

//*****************************************************************************
//
// The current state of the USB in the system based on the detected mode.
//
//*****************************************************************************
volatile tUSBMode g_eCurrentUSBMode = USB_MODE_NONE;

//*****************************************************************************
//
// Flags used to remember which parameters have changed since the last
// call to USBMouseHostProcess().
//
//*****************************************************************************
static unsigned long g_ulChangeFlags;

//*****************************************************************************
//
// The global values used to store the mouse state.
//
//*****************************************************************************
static unsigned long g_ulButtons;
static short g_sCursorX, g_sCursorY;

//*****************************************************************************
//
// Holds the previous press position for the touch screen.
//
//*****************************************************************************
static volatile long g_lScreenStartX;
static volatile long g_lScreenStartY;

//*****************************************************************************
//
// Holds the current press position for the touch screen.
//
//*****************************************************************************
static volatile long g_lScreenX;
static volatile long g_lScreenY;

//*****************************************************************************
//
// Holds the current state of the touch screen - pressed or not.
//
//*****************************************************************************
static volatile tBoolean g_bScreenPressed;

//*****************************************************************************
//
// Holds the current state of the push button - pressed or not.
//
//*****************************************************************************
static volatile tBoolean g_bButtonPressed;

//*****************************************************************************
//
// The dimensions of the display.  These values are used to set the initial
// cursor position to the center of the display when a mouse is first
// connected.
//
//*****************************************************************************
static int g_iScreenWidth, g_iScreenHeight;

//*****************************************************************************
//
// This function is called in the context of USBMouseHostProcess() to update
// the mouse position.  It returns true if either X or Y coordinate changed
// otherwise it returns false.
//
//*****************************************************************************
static tBoolean
UpdateCursor(int iXDelta, int iYDelta)
{
    int iNewX, iNewY;
    tBoolean bRetcode;

    //
    // Determine the new coordinates.
    //
    iNewX = (int)g_sCursorX + iXDelta;
    iNewY = (int)g_sCursorY + iYDelta;

    //
    // Clip them to the screen.
    //
    iNewX = (iNewX >= g_iScreenWidth) ? (g_iScreenWidth - 1) : iNewX;
    iNewY = (iNewY >= g_iScreenHeight) ? (g_iScreenHeight - 1) : iNewY;
    iNewX = (iNewX < 0) ? 0 : iNewX;
    iNewY = (iNewY < 0) ? 0 : iNewY;

    //
    // Did anything change?
    //
    bRetcode = ((g_sCursorX != (short)iNewX) || (g_sCursorY != (short)iNewY));

    //
    // Update the cursor position.
    //
    g_sCursorX = (short)iNewX;
    g_sCursorY = (short)iNewY;

    //
    // Tell the caller whether or not they need to update the display as a
    // result of a cursor position change.
    //
    return(bRetcode);
}

//*****************************************************************************
//
// This is the callback from the USB host HID mouse handler.
//
// \param pvCBData is ignored by this function.
// \param ulEvent is one of the valid events for a mouse device.
// \param ulMsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a mouse has
// been plugged in or removed and any time mouse movement or button pressed
// is detected.
//
// \return This function will return 0.
//
//*****************************************************************************
unsigned long
USBHostMouseCallback(void *pvCBData, unsigned long ulEvent,
                     unsigned long ulMsgParam, void *pvMsgData)
{
    switch(ulEvent)
    {
        //
        // New mouse detected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // Proceed to the STATE_MOUSE_CONNECTING state so that the main
            // loop can finish initializing the mouse since USBHMouseInit()
            // cannot be called from within a callback.
            //
            g_eUSBState = STATE_HOST_CONNECTING;
            break;
        }

        //
        // Mouse has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Change the state so that the main loop knows that the mouse is
            // no longer present.
            //
            g_eUSBState = STATE_NO_CONNECTION;

            //
            // Reset the button state.
            //
            g_ulButtons = 0;

            //
            // Remember to tell the client that the connection state changed.
            //
            g_ulChangeFlags |= MOUSE_FLAG_CONNECTION;

            break;
        }

        //
        // Mouse button press detected.
        //
        case USBH_EVENT_HID_MS_PRESS:
        {
            //
            // Save the new button that was pressed.
            //
            g_ulButtons |= ulMsgParam;

            //
            // Remember to tell the client that the button state changed.
            //
            g_ulChangeFlags |= MOUSE_FLAG_BUTTONS;

            break;
        }

        //
        // Mouse button release detected.
        //
        case USBH_EVENT_HID_MS_REL:
        {
            //
            // Remove the button from the pressed state.
            //
            g_ulButtons &= ~ulMsgParam;

            //
            // Remember to tell the client that the button state changed.
            //
            g_ulChangeFlags |= MOUSE_FLAG_BUTTONS;

            break;
        }

        //
        // Mouse X movement detected.
        //
        case USBH_EVENT_HID_MS_X:
        {
            //
            // Update the cursor position and, if it changed, set a flag for
            // the client.
            //
            if(UpdateCursor((signed char)ulMsgParam, 0))
            {
                g_ulChangeFlags |= MOUSE_FLAG_POSITION;
            }

            break;
        }

        //
        // Mouse Y movement detected.
        //
        case USBH_EVENT_HID_MS_Y:
        {
            //
            // Update the cursor position and, if it changed, set a flag for
            // the client.
            //
            if(UpdateCursor(0, (signed char)ulMsgParam))
            {
                g_ulChangeFlags |= MOUSE_FLAG_POSITION;
            }

            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// This function must be called periodically by the main loop of the
// application to process any USB mouse events.  The return code is a collection
// of bits indicating which mouse parameters changed since the last time the
// function was called.  The caller may make use of this information to update
// the user interface.
//
// Parameter ulElapsedMs indicates the number of milliseconds that have passed
// since the last time this call was made.
//
//*****************************************************************************
unsigned long
USBMouseProcess(void)
{
    unsigned long ulFlags;

    //
    // Has a mouse just been connected?  If so, we need to complete the
    // initialization process.
    //
    if(g_eUSBState == STATE_HOST_CONNECTING)
    {
        //
        // Initialize the newly connected mouse.
        //
        USBHMouseInit(g_ulMouseInstance);

        //
        // Proceed to the mouse connected state.
        //
        g_eUSBState = STATE_MOUSE_CONNECTED;

        //
        // Remember to tell the client that the connection state changed.
        //
        g_ulChangeFlags |= MOUSE_FLAG_CONNECTION;
    }

    //
    // Periodically call the main loop for the Host controller driver.
    //
    USBHCDMain();

    //
    // Take a copy of the change flags with the USB interrupt disabled then
    // clear the flags.
    //
    IntDisable(INT_USB0);
    ulFlags = g_ulChangeFlags;
    g_ulChangeFlags = 0;
    IntEnable(INT_USB0);

    //
    // Tell the caller what changed since the last time this function was
    // called.
    //
    return(ulFlags);
}

//*****************************************************************************
//
// Initializes the system for operation as a host for a USB mouse.
//
// ulScreenWidth is the width of the display in pixels.
// ulScreenHeight is the height of the display in pixels.
//
// This function configures the USB library to allow operation as a host for
// a USB mouse.  The parameters passed inform the module of the screen
// dimensions and these are used to clip the current mouse location such that
// it does not extend off the screen.
//
// Returns true on success or false on failure.
//
//*****************************************************************************
tBoolean
USBMouseInit(unsigned long ulScreenWidth, unsigned long ulScreenHeight)
{
    //
    // Initialize the button states.
    //
    g_ulButtons = 0;

    //
    // Remember that we have not detected any connection yet.
    //
    g_eUSBState = STATE_NO_CONNECTION;

    //
    // Force the caller to update all parameters the first time they call
    // USBMouseHostProcess().
    //
    g_ulChangeFlags = (MOUSE_FLAG_CONNECTION | MOUSE_FLAG_POSITION  |
                       MOUSE_FLAG_BUTTONS);

    //
    // Initialize the cursor position.
    //
    g_iScreenWidth = (int)ulScreenWidth;
    g_iScreenHeight = (int)ulScreenHeight;
    g_sCursorX = (short)(ulScreenWidth / 2);
    g_sCursorY = (short)(ulScreenHeight / 2);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ulNumHostClassDrivers);

    //
    // Open an instance of the mouse driver.  The mouse does not need
    // to be present at this time, this just saves a place for it and allows
    // the applications to be notified when a mouse is present.
    //
    g_ulMouseInstance =
        USBHMouseOpen(USBHostMouseCallback, g_pucBuffer, MOUSE_MEMORY_SIZE);

    //
    // Configure the power pins for host mode.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinTypeUSBDigital(GPIO_PORTA_BASE, GPIO_PIN_6);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Initialize the USB controller for host mode operation.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);

    //
    // Call the USB host handler function once just to prime the pump.
    //
    USBHCDMain();

    //
    // All is well.
    //
    return(true);
}

//*****************************************************************************
//
// Determine whether or not a USB mouse is currently connected to the board.
//
//*****************************************************************************
tBoolean
USBMouseIsConnected(void)
{
    //
    // Tell the caller whether we are connected to anything at all.
    //
    return((g_eUSBState == STATE_MOUSE_CONNECTED) ? true : false);
}

//*****************************************************************************
//
// Returns the current mouse cursor position, clipped to the display.  This call
// is only valid when we are operating as the USB host and a HID mouse is
// connected to the development board.
//
//*****************************************************************************
void
USBMouseHostPositionGet(short *psX, short *psY)
{
    *psX = g_sCursorX;
    *psY = g_sCursorY;
}

//*****************************************************************************
//
// Returns the current states of the mouse buttons.  This call is only valid
// when we are operating as the USB host and a HID mouse is connected to the
// development board.
//
//*****************************************************************************
unsigned long
USBMouseHostButtonsGet(void)
{
    return(g_ulButtons);
}
