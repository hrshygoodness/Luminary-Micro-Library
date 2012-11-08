//*****************************************************************************
//
// usb_funcs.c - Support functions for supporting USB mouse both as a device
//               and as a host and Mass Storage Class host.
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
// This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhmsc.h"
#include "usblib/host/usbhhidmouse.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include "qs-checkout.h"
#include "usb_funcs.h"
#include "usb_mouse_structs.h"
#include "file.h"
#define GUI_WIDGETS_FUNCS_ONLY
#include "gui_widgets.h"

//*****************************************************************************
//
// The GPIO pin which is connected to the user button.
//
//*****************************************************************************
#define USER_BTN_PORT  GPIO_PORTJ_BASE
#define USER_BTN_PIN   GPIO_PIN_7

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
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Mouse class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_USBHIDClassDriver,
    &g_USBHostMSCClassDriver,
    &g_sUSBEventDriver
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
    // We are connected to neither a USB host nor a HID mouse device.
    //
    STATE_NO_CONNECTION,

    //
    // A HID mouse device has been connected to us and is awaiting
    // initialization prior to use.
    //
    STATE_HOST_CONNECTING,

    //
    // A HID mouse device is attached and we are operating as USB host.
    //
    STATE_MOUSE_HOST,

    //
    // We are attached to a USB host and operating as a HID mouse device.
    //
    STATE_MOUSE_DEVICE,

    //
    // We are operating as a USB mouse device and waiting for our last report
    // to be sent to the host.
    //
    STATE_MOUSE_BUSY,

    //
    // Mass storage device is being enumerated.
    //
    STATE_MSC_DEVICE_ENUM,

    //
    // Mass storage device is ready.
    //
    STATE_MSC_DEVICE_READY,

    //
    // An unsupported device has been attached and we are operating as USB host.
    //
    STATE_UNKNOWN_DEVICE,

    //
    // A power fault condition has been reported.
    //
    STATE_POWER_FAULT
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
// This is the callback from the MSC driver.
//
// \param ulInstance is the driver instance which is needed when communicating
// with the driver.
// \param ulEvent is one of the events defined by the driver.
// \param pvData is a pointer to data passed into the initial call to register
// the callback.
//
// This function handles callback events from the MSC driver.  The only events
// currently handled are the MSC_EVENT_OPEN and MSC_EVENT_CLOSE.  This allows
// the main routine to know when an MSC device has been detected and
// enumerated and when an MSC device has been removed from the system.
//
// \return None
//
//*****************************************************************************
void
USBHostMSCCallback(unsigned long ulInstance, unsigned long ulEvent, void *pvData)
{
    //
    // Determine the event.
    //
    switch(ulEvent)
    {
        //
        // Called when the device driver has successfully enumerated an MSC
        // device.
        //
        case MSC_EVENT_OPEN:
        {
            //
            // Proceed to the enumeration state.
            //
            g_eUSBState = STATE_MSC_DEVICE_ENUM;

            break;
        }

        //
        // Called when the device driver has been unloaded due to error or
        // the device is no longer present.
        //
        case MSC_EVENT_CLOSE:
        {
            //
            // Go back to the "no device" state and wait for a new connection.
            //
            g_eUSBState = STATE_NO_CONNECTION;


            //
            // Set as flag to tell the main loop that the connection status
            // changed.
            //
            g_ulChangeFlags |=  MSC_FLAG_CONNECTION;

            //
            // Re-initialize the file system.
            //
            FileInit();

            break;
        }

        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// \param pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mass storage device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
// \return None.
//
//*****************************************************************************
void
USBHCDEvents(void *pvData)
{
    tEventInfo *pEventInfo;

    //
    // Cast this pointer to its actual type.
    //
    pEventInfo = (tEventInfo *)pvData;

    switch(pEventInfo->ulEvent)
    {
        //
        // The USB host has connected to and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // See if this is a HID Mouse.
            //
            if((USBHCDDevClass(pEventInfo->ulInstance, 0) == USB_CLASS_HID) &&
               (USBHCDDevProtocol(pEventInfo->ulInstance, 0) ==
                USB_HID_PROTOCOL_MOUSE))
            {
                g_eUSBState = STATE_MOUSE_DEVICE;

                //
                // Remember to tell the client that the connection state
                // changed.
                //
                g_ulChangeFlags |= MOUSE_FLAG_CONNECTION;
            }
            break;
        }
        //
        // An unsupported device has been connected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
            //
            // An unknown device was detected.
            //
            g_eUSBState = STATE_UNKNOWN_DEVICE;

            break;
        }
        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_eUSBState = STATE_NO_CONNECTION;

            //
            // See if this is a HID Mouse.
            //
            if((USBHCDDevClass(pEventInfo->ulInstance, 0) == USB_CLASS_HID) &&
               (USBHCDDevProtocol(pEventInfo->ulInstance, 0) ==
                USB_HID_PROTOCOL_MOUSE))
            {
                //
                // Remember to tell the client that the connection state
                // changed.
                //
                g_ulChangeFlags |= MOUSE_FLAG_CONNECTION;
            }

            break;
        }
        //
        // A bus power fault was detected.
        //
        case USB_EVENT_POWER_FAULT:
        {
            //
            // No power means no device is present.
            //
            g_eUSBState = STATE_POWER_FAULT;
            PrintfStatus("Power fault");

            break;
        }

        default:
        {
            break;
        }
    }
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
USBDeviceMouseCallback(void *pvCBData, unsigned long ulEvent,
                       unsigned long ulMsgData, void *pvMsgData)
{
    switch(ulEvent)
    {
        //
        // The USB host has connected to and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_eUSBState = STATE_MOUSE_DEVICE;

            //
            // Remember to tell the client that the connection state changed.
            //
            g_ulChangeFlags |= MOUSE_FLAG_CONNECTION;
            break;
        }

        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_eUSBState = STATE_NO_CONNECTION;

            //
            // Remember to tell the client that the connection state changed.
            //
            g_ulChangeFlags |= MOUSE_FLAG_CONNECTION;
            break;
        }

        //
        // A report was sent to the host. We are now free to send another.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            g_eUSBState = STATE_MOUSE_DEVICE;
            break;
        }
    }
    return(0);
}

//*****************************************************************************
//
// This function handles updates due to the touch screen and buttons when in
// USB device mode.
//
// This function is called from the USBMouseProcess each time the touch screen
// state needs to be checked.  If it detects an update it will schedule an
// transfer to the host.
//
//*****************************************************************************
static void
DeviceTouchEventHandler(void)
{
    long lDeltaX, lDeltaY;
    tBoolean bBtnPressed;

    //
    // Get the current state of the user button.
    //
    bBtnPressed = (ROM_GPIOPinRead(USER_BTN_PORT, USER_BTN_PIN) &
                   USER_BTN_PIN) ? false : true;

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
            g_eUSBState = STATE_MOUSE_BUSY;
            USBDHIDMouseStateChange((void *)&g_sMouseDevice,
                                    (char)lDeltaX, (char)lDeltaY,
                                    (bBtnPressed ? MOUSE_REPORT_BUTTON_1 : 0));
        }
    }
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
USBFuncsProcess(void)
{
    unsigned long ulFlags, ulNow, ulElapsed;
    static unsigned long ulLast;

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
        g_eUSBState = STATE_MOUSE_HOST;

        //
        // Remember to tell the client that the connection state changed.
        //
        g_ulChangeFlags |= MOUSE_FLAG_CONNECTION;
    }

    //
    // How much time has elapsed since we last called the USB OTG library?
    //
    ulNow = g_ulSysTickCount * MS_PER_TICK;
    ulElapsed = ulNow - ulLast;

    //
    // If it has been at least 50mS, call the library again and, if we are in
    // device mode, process any mouse moves or button presses.
    //
    if(ulElapsed >= OTG_POLL_INTERVAL_MS)
    {
        //
        // Tell the OTG library code how much time has passed in milliseconds
        // since the last call.  Note that we do this with the Ethernet
        // interrupt disabled since the web server accesses the USB stack and
        // runs in the context of that interrupt.
        //
        ROM_IntDisable(INT_ETH);
        USBOTGMain(ulElapsed);
        ROM_IntEnable(INT_ETH);
        ulLast = ulNow;

        if(g_eUSBState == STATE_MOUSE_DEVICE)
        {
            //
            // We are a device so check to see if there has been any touch-
            // screen activity that would require us to send a new report to
            // the host.
            //
            DeviceTouchEventHandler();
        }
        else if(g_eUSBState == STATE_MSC_DEVICE_ENUM)
        {
            //
            // Check if the Mass storage device is ready.
            //
            if(USBHMSCDriveReady(g_ulMSCInstance) == 0)
            {
                g_eUSBState = STATE_MSC_DEVICE_READY;

                //
                // Set as flag to tell the main loop that the connection status
                // changed.
                //
                g_ulChangeFlags |= MSC_FLAG_CONNECTION;
            }
            else
            {
                //
                // Wait about 500ms before attempting to check if the
                // device is ready again.
                //
                SysCtlDelay(SysCtlClockGet()/(3));
            }
        }
    }

    //
    // Take a copy of the change flags with the USB interrupt disabled then
    // clear the flags.  We also need to ensure that the Ethernet interrupt is
    // disabled since USB calls are made from within that handler and, if we
    // leave it enabled, we can get deadlocked if a web server request comes in
    // during the time the USB interrupt is off.
    //
    ROM_IntDisable(INT_ETH);
    ROM_IntDisable(INT_USB0);
    ulFlags = g_ulChangeFlags;
    g_ulChangeFlags = 0;
    ROM_IntEnable(INT_USB0);
    ROM_IntEnable(INT_ETH);

    //
    // Tell the caller what changed since the last time this function was
    // called.
    //
    return(ulFlags);
}

//*****************************************************************************
//
// Callback function for USB mode changes.
//
//*****************************************************************************
static void
ModeCallback(unsigned long ulIndex, tUSBMode eMode)
{
    //
    // Has the mode changed?  We need this since, if nothing is attached, we
    // will get called every few seconds with USB_MODE_NONE.
    //
    if(eMode != g_eCurrentUSBMode)
    {
        //
        // Save the new mode.
        //
        g_eCurrentUSBMode = eMode;
    }
}

//*****************************************************************************
//
// This function is called by the touch screen driver whenever there is a
// change in press state or position.
//
//*****************************************************************************
void
USBMouseTouchHandler(unsigned long ulMessage, long lX, long lY)
{
    switch(ulMessage)
    {
        //
        // The touch screen has been pressed.  Remember where we are so that
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
        // The touch screen is no longer being pressed.
        //
        case WIDGET_MSG_PTR_UP:
            g_bScreenPressed = false;
            break;

        //
        // The user is dragging his/her finger/stylus over the touch screen.
        //
        case WIDGET_MSG_PTR_MOVE:
            g_lScreenX = lX;
            g_lScreenY = lY;
            break;
    }
}

//*****************************************************************************
//
// This function performs device-specific initialization for the HID mouse.
//
//*****************************************************************************
static void
DeviceInit(void)
{
    //
    // Configure the pin the user button is attached to as an input with a
    // pull-up.
    //
    ROM_GPIODirModeSet(USER_BTN_PORT, USER_BTN_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(USER_BTN_PORT, USER_BTN_PIN, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);

    //
    // Pass the USB library our device information, initialize the USB
    // controller and connect the device to the bus.
    //
    USBDHIDMouseInit(0, (tUSBDHIDMouseDevice *)&g_sMouseDevice);
}

//*****************************************************************************
//
// This function performs host-specific initialization for the HID mouse.
//
//*****************************************************************************
void
HostInit(void)
{
    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ulNumHostClassDrivers);

    //
    // Initialize the button states.
    //
    g_ulButtons = 0;

    //
    // Open an instance of the mouse driver.  The mouse does not need
    // to be present at this time, this just saves a place for it and allows
    // the applications to be notified when a mouse is present.
    //
    g_ulMouseInstance =
        USBHMouseOpen(USBHostMouseCallback, g_pucBuffer, MOUSE_MEMORY_SIZE);

    //
    // Open an instance of the mass storage class driver to support USB
    // flash sticks which may also be used with this application.
    //
    g_ulMSCInstance = USBHMSCDriveOpen(0, USBHostMSCCallback);

    //
    // Configure the power pins for host mode.
    //
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTA_BASE, GPIO_PIN_6);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Call the main loop for the Host controller driver.
    //
    g_eUSBState = STATE_NO_CONNECTION;
}

//*****************************************************************************
//
// Initializes the various USB functions of the system.
//
// ulScreenWidth is the width of the display in pixels.
// ulScreenHeight is the height of the display in pixels.
//
// This function configures the USB library to allow operation as a host for
// a USB mouse or MSC.  The parameters passed inform the module of the screen
// dimensions and these are used to clip the current mouse location such that
// it does not extend off the screen.
//
// Returns true on success or false on failure.
//
//*****************************************************************************
tBoolean
USBFuncsInit(unsigned long ulScreenWidth, unsigned long ulScreenHeight)
{
    //
    // Initialize the USB stack mode and pass in a mode callback.
    //
    USBStackModeSet(0, USB_MODE_OTG, ModeCallback);

    //
    // Initialize the host stack.
    //
    HostInit();

    //
    // Initialize the device stack.
    //
    DeviceInit();

    //
    // Initialize the USB controller for dual mode operation with a 2ms polling
    // rate.
    //
    USBOTGModeInit(0, 2000, g_pHCDPool, HCD_MEMORY_SIZE);

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
    // Initialize the button states.
    //
    g_ulButtons = 0;

    //
    // All is well.
    //
    return(true);
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

//*****************************************************************************
//
// Determine whether or not a USB mouse is currently connected to the board.
//
//*****************************************************************************
tBoolean
USBMouseIsConnected(tBoolean *pbDeviceMode)
{
    //
    // Make sure that a USB flash disk is not currently connected.
    //
    if(USBMSCIsConnected())
    {
        return(false);
    }

    //
    // Tell the caller whether we are operating as a device or a host.  If we
    // are not currently connected to anything, the caller will ignore the
    // value written to *pbDeviceMode.
    //
    *pbDeviceMode = (g_eCurrentUSBMode == USB_MODE_DEVICE) ? true : false;

    //
    // Tell the caller whether we are connected to anything at all.
    //
    return(((g_eCurrentUSBMode == USB_MODE_HOST) ||
            (g_eCurrentUSBMode == USB_MODE_DEVICE)) ? true : false);
}

//*****************************************************************************
//
// Determine whether or not a USB flash drive is currently connected.
//
//*****************************************************************************
tBoolean
USBMSCIsConnected(void)
{
    //
    // Is the USB stick connected?
    //
    if((g_eCurrentUSBMode == USB_MODE_HOST) &&
       (g_eUSBState == STATE_MSC_DEVICE_READY))
    {
        return(true);
    }
    else
    {
        return(false);
    }
}
