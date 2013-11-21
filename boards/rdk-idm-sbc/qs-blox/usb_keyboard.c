//*****************************************************************************
//
// usb_keyboard.c - Functions to handle USB keyboard control of the qs-blox
//                  game.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidkeyboard.h"
#include "third_party/blox/blox.h"
#include "usb_keyboard.h"

//*****************************************************************************
//
// User input flags used to control the game.
//
//*****************************************************************************
extern volatile unsigned long g_ulCommandFlags;

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
// The size of the keyboard device interface's memory pool in bytes.
//
//*****************************************************************************
#define KEYBOARD_MEMORY_SIZE    128

//*****************************************************************************
//
// The memory pool to provide to the keyboard device.
//
//*****************************************************************************
unsigned char g_pucBuffer[KEYBOARD_MEMORY_SIZE];

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Keyboard class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_USBHIDClassDriver
};

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
// The global value used to store the keyboard instance value.
//
//*****************************************************************************
static unsigned long g_ulKeyboardInstance;

//*****************************************************************************
//
// This enumerated type is used to hold the states of the keyboard.
//
//*****************************************************************************
enum
{
    //
    // No device is present.
    //
    STATE_NO_DEVICE,

    //
    // Keyboard has been detected and needs to be initialized in the main
    // loop.
    //
    STATE_KEYBOARD_INIT,

    //
    // Keyboard is connected and waiting for events.
    //
    STATE_KEYBOARD_CONNECTED,

    //
    // An unsupported device has been attached.
    //
    STATE_UNKNOWN_DEVICE,

    //
    // A power fault has occurred.
    //
    STATE_POWER_FAULT
}
g_eUSBState;

extern const tHIDKeyboardUsageTable g_sUSKeyboardMap;

//*****************************************************************************
//
// This is the callback from the USB HID keyboard handler.
//
// \param pvCBData is ignored by this function.
// \param ulEvent is one of the valid events for a keyboard device.
// \param ulMsgParam is defined by the event that occurs.
// \param pvMsgData is a pointer to data that is defined by the event that
// occurs.
//
// This function will be called to inform the application when a keyboard has
// been plugged in or removed and any time a key is pressed or released.
//
// \return This function will return 0.
//
//*****************************************************************************
static unsigned long
KeyboardCallback(void *pvCBData, unsigned long ulEvent,
                 unsigned long ulMsgParam, void *pvMsgData)
{
    switch(ulEvent)
    {
        //
        // New keyboard detected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // Proceed to the STATE_KEYBOARD_INIT state so that the main loop
            // can finish initialized the mouse since USBHKeyboardInit() cannot
            // be called from within a callback.
            //
            g_eUSBState = STATE_KEYBOARD_INIT;

            break;
        }

        //
        // Keyboard has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Change the state so that the main loop knows that the keyboard
            // is no longer present.
            //
            g_eUSBState = STATE_NO_DEVICE;

            break;
        }

        //
        // New Key press detected.
        //
        case USBH_EVENT_HID_KB_PRESS:
        {
            //
            // Look for key presses that correspond to game controls.
            //
            switch(ulMsgParam)
            {
                case HID_KEYB_USAGE_SPACE:
                    g_ulCommandFlags |= BLOX_CMD_DROP;
                    break;

                case HID_KEYB_USAGE_R:
                    g_ulCommandFlags |= BLOX_CMD_ROTATE;
                    break;

                case HID_KEYB_USAGE_RIGHT_ARROW:
                case HID_KEYB_USAGE_UP_ARROW:
                    g_ulCommandFlags |= BLOX_CMD_UP;
                    break;

                case HID_KEYB_USAGE_LEFT_ARROW:
                case HID_KEYB_USAGE_DOWN_ARROW:
                    g_ulCommandFlags |= BLOX_CMD_DOWN;
                    break;

                case HID_KEYB_USAGE_P:
                    g_ulCommandFlags |= BLOX_CMD_PAUSE;
                    break;
            }
            break;
        }

        default:
        {
            //
            // This application ignores modifier keys and key release messages.
            //
            break;
        }
    }
    return(0);
}

//*****************************************************************************
//
// Initialize the USB library and set things up so that we can handle USB HID
// keyboard attachment.
//
//*****************************************************************************
void
USBKeyboardInit(void)
{
    //
    // Initially wait for device connection.
    //
    g_eUSBState = STATE_NO_DEVICE;

    //
    // Configure the power pins for host controller.
    //
    GPIOPinTypeUSBDigital(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ulNumHostClassDrivers);

    //
    // Open an instance of the keyboard driver.  The keyboard does not need
    // to be present at this time, this just save a place for it and allows
    // the applications to be notified when a keyboard is present.
    //
    g_ulKeyboardInstance = USBHKeyboardOpen(KeyboardCallback, g_pucBuffer,
                                            KEYBOARD_MEMORY_SIZE);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Initialize the USB controller for host mode operation.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);
}

//*****************************************************************************
//
// This function must be called periodically from the main loop of the
// application to handle connection and disconnection of the USB keyboard.
//
//*****************************************************************************
void USBKeyboardProcess(void)
{
    //
    // Periodically call the main loop for the Host controller driver.
    //
    USBHCDMain();

    switch(g_eUSBState)
    {
        //
        // This state is entered when they keyboard is first detected.
        //
        case STATE_KEYBOARD_INIT:
        {
            //
            // Initialized the newly connected keyboard.
            //
            USBHKeyboardInit(g_ulKeyboardInstance);

            //
            // Proceed to the keyboard connected state.
            //
            g_eUSBState = STATE_KEYBOARD_CONNECTED;

            break;
        }

        default:
        {
            break;
        }
    }
}
