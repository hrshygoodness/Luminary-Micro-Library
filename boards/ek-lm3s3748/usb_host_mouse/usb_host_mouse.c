//*****************************************************************************
//
// usb_host_mouse.c - main application code for the host mouse example.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidmouse.h"
#include "utils/uartstdio.h"
#include "drivers/formike128x128x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Mouse Host (usb_host_mouse)</h1>
//!
//! This example application demonstrates how to support a USB mouse
//! attached to the evaluation kit board.  The display will show if a mouse
//! is currently connected and the current state of the buttons on the
//! on the bottom status area of the screen.  The main drawing area will show
//! a mouse cursor that can be moved around in the main area of the screen.
//! If the left mouse button is held while moving the mouse, the cursor will
//! draw on the screen.  A side effect of the application not being able to
//! read the current state of the screen is that the cursor will erase
//! anything it moves over while the left mouse button is not pressed.
//
//*****************************************************************************

//*****************************************************************************
//
// These definitions are used to set the USB Mux values for the LM3S3748 board.
//
//*****************************************************************************
#define USB_MUX_GPIO_PERIPH     SYSCTL_PERIPH_GPIOH
#define USB_MUX_GPIO_BASE       GPIO_PORTH_BASE
#define USB_MUX_GPIO_PIN        GPIO_PIN_2
#define USB_MUX_SEL_DEVICE      USB_MUX_GPIO_PIN
#define USB_MUX_SEL_HOST        0

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
    &g_USBHIDClassDriver
    ,&g_sUSBEventDriver
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
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// The global value used to store the mouse instance value.
//
//*****************************************************************************
static unsigned long g_ulMouseInstance;

//*****************************************************************************
//
// The global values used to store the mouse state.
//
//*****************************************************************************
static unsigned long g_ulButtons;
static tRectangle g_sCursor;

//*****************************************************************************
//
// This enumerated type is used to hold the states of the mouse.
//
//*****************************************************************************
enum
{
    //
    // No device is present.
    //
    STATE_NO_DEVICE,

    //
    // Mouse has been detected and needs to be initialized in the main
    // loop.
    //
    STATE_MOUSE_INIT,

    //
    // Mouse is connected and waiting for events.
    //
    STATE_MOUSE_CONNECTED,

    //
    // An unsupported device has been attached.
    //
    STATE_UNKNOWN_DEVICE,

    //
    // A power fault has occured.
    //
    STATE_POWER_FAULT
}
eUSBState;

//*****************************************************************************
//
// These defines are used to define the screen constraints to the application.
//
//*****************************************************************************
#define DISPLAY_BANNER_HEIGHT   14
#define DISPLAY_BANNER_BG       ClrDarkBlue
#define DISPLAY_BANNER_FG       ClrWhite
#define DISPLAY_MOUSE_BG        ClrBlack
#define DISPLAY_MOUSE_FG        ClrWhite
#define DISPLAY_MOUSE_SIZE      2

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
// This function updates the cursor position based on deltas received from
// the mouse device.
//
// \param iXDelta is the signed movement in the X direction.
// \param iYDelta is the signed movement in the Y direction.
//
// This function is called by the mouse handler code when it detects a change
// in the position of the mouse.  It will take the inputs and force them
// to be constrained to the display area of the screen.  If the left mouse
// button is pressed then the mouse will draw on the screen and if it is not
// it will move around normally.  A side effect of not being able to read the
// current state of the screen is that the cursor will erase anything it moves
// over while the left mouse button is not pressed.
//
// \return None.
//
//*****************************************************************************
void
UpdateCursor(int iXDelta, int iYDelta)
{
    int iTemp;

    //
    // If the left button is not pressed then erase the previous cursor
    // position.
    //
    if((g_ulButtons & 1) == 0)
    {
        //
        // Erase the previous cursor.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_MOUSE_BG);
        GrRectFill(&g_sContext, &g_sCursor);
    }

    //
    // Need to do signed math so use the temporary signed value.
    //
    iTemp = g_sCursor.sXMin;

    //
    // Update the X position without going off the screen.
    //
    if(((int)g_sCursor.sXMin + iXDelta + DISPLAY_MOUSE_SIZE) <
       GrContextDpyWidthGet(&g_sContext))
    {
        //
        // Update the X cursor position.
        //
        iTemp += iXDelta;

        //
        // Don't let the cursor go off the left of the screen either.
        //
        if(iTemp < 0)
        {
            iTemp = 0;
        }
    }

    //
    // Update the X position.
    //
    g_sCursor.sXMin = iTemp;
    g_sCursor.sXMax = iTemp + DISPLAY_MOUSE_SIZE;

    //
    // Need to do signed math so use the temporary signed value.
    //
    iTemp = g_sCursor.sYMin;

    //
    // Update the Y position without going off the screen.
    //
    if(((int)g_sCursor.sYMin + iYDelta) < (GrContextDpyHeightGet(&g_sContext) -
       DISPLAY_BANNER_HEIGHT - DISPLAY_MOUSE_SIZE))
    {
        //
        // Update the Y cursor position.
        //
        iTemp += iYDelta;

        //
        // Don't let the cursor overwrite the status area of the screen.
        //
        if(iTemp < DISPLAY_BANNER_HEIGHT + 1)
        {
            iTemp = DISPLAY_BANNER_HEIGHT + 1;
        }
    }

    //
    // Update the Y position.
    //
    g_sCursor.sYMin = iTemp;
    g_sCursor.sYMax = iTemp + DISPLAY_MOUSE_SIZE;

    //
    // Draw the new cursor.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_MOUSE_FG);
    GrRectFill(&g_sContext, &g_sCursor);
}

//*****************************************************************************
//
// This function will update the small mouse button indicators in the status
// bar area of the screen.  This can be called on its own or it will be called
// whenever UpdateStatus() is called as well.
//
//*****************************************************************************
void
UpdateButtons(void)
{
    tRectangle sRect;
    int iButton;

    //
    // Initialize the button indicator.
    //
    sRect.sXMin = GrContextDpyWidthGet(&g_sContext) - 30;
    sRect.sYMin = GrContextDpyHeightGet(&g_sContext) - 12;
    sRect.sXMax = sRect.sXMin + 6;
    sRect.sYMax = sRect.sYMin + 8;

    //
    // Check all three buttons.
    //
    for(iButton = 0; iButton < 3; iButton++)
    {
        //
        // Draw the button indicator red if pressed and black if not pressed.
        //
        if(g_ulButtons & (1 << iButton))
        {
            GrContextForegroundSet(&g_sContext, ClrRed);
        }
        else
        {
            GrContextForegroundSet(&g_sContext, ClrBlack);
        }

        //
        // Draw the back of the  button indicator.
        //
        GrRectFill(&g_sContext, &sRect);

        //
        // Draw the border on the button indicator.
        //
        GrContextForegroundSet(&g_sContext, ClrWhite);
        GrRectDraw(&g_sContext, &sRect);

        //
        // Move to the next button indicator position.
        //
        sRect.sXMin += 8;
        sRect.sXMax += 8;
    }
}

//*****************************************************************************
//
// This function updates the status area of the screen.  It uses the current
// state of the application to print the status bar.
//
//*****************************************************************************
void
UpdateStatus(void)
{
    tRectangle sRect;

    //
    // Fill the bottom rows of the screen with blue to create the status area.
    //
    sRect.sXMin = 0;
    sRect.sYMin = GrContextDpyHeightGet(&g_sContext) -
                  DISPLAY_BANNER_HEIGHT - 1;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = sRect.sYMin + DISPLAY_BANNER_HEIGHT;

    //
    // Draw the background of the banner.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_FG);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);

    //
    // Update the status on the screen.
    //
    if(eUSBState == STATE_NO_DEVICE)
    {
        //
        // Keyboard is currently disconnected.
        //
        GrStringDraw(&g_sContext, "No Device", -1, 4, sRect.sYMin + 4, 0);
    }
    else if(eUSBState == STATE_MOUSE_CONNECTED)
    {
        //
        // Keyboard is connected.
        //
        GrStringDraw(&g_sContext, "Connected", -1, 4, sRect.sYMin + 4, 0);
    }
    else if(eUSBState == STATE_UNKNOWN_DEVICE)
    {
        //
        // Keyboard is connected.
        //
        GrStringDraw(&g_sContext, "Unknown Device", -1, 4, sRect.sYMin + 4, 0);
    }
    else if(eUSBState == STATE_POWER_FAULT)
    {
        //
        // Keyboard is connected.
        //
        GrStringDraw(&g_sContext, "Power Fault", -1, 4, sRect.sYMin + 4, 0);
    }

    UpdateButtons();
}

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mouse device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
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
        // New mouse detected.
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
                //
                // Indicate that the mouse has been detected.
                //
                UARTprintf("Mouse Connected\n");

                //
                // Proceed to the STATE_MOUSE_INIT state so that the main loop
                // can finish initialized the mouse since USBHMouseInit()
                // cannot be called from within a callback.
                //
                eUSBState = STATE_MOUSE_INIT;
            }

            break;
        }
        //
        // Unsupported device detected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
            UARTprintf("Unsupported Device Connected\n");

            //
            // An unknown device was detected.
            //
            eUSBState = STATE_UNKNOWN_DEVICE;

            UpdateStatus();

            break;
        }
        //
        // Device has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Indicate that the device has been disconnected.
            //
            UARTprintf("Device Disconnected\n");

            //
            // Change the state so that the main loop knows that the device is
            // no longer present.
            //
            eUSBState = STATE_NO_DEVICE;

            //
            // Reset the button state.
            //
            g_ulButtons = 0;

            UpdateStatus();

            break;
        }
        //
        // Power Fault has occurred.
        //
        case USB_EVENT_POWER_FAULT:
        {
            UARTprintf("Power Fault\n");

            //
            // No power means no device is present.
            //
            eUSBState = STATE_POWER_FAULT;

            UpdateStatus();

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
// This is the callback from the USB HID mouse handler.
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
MouseCallback(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgParam,
              void *pvMsgData)
{
    switch(ulEvent)
    {
        //
        // Mouse button press detected.
        //
        case USBH_EVENT_HID_MS_PRESS:
        {
            UARTprintf("Button Pressed %02x\n", ulMsgParam);

            //
            // Save the new button that was pressed.
            //
            g_ulButtons |= ulMsgParam;

            break;
        }

        //
        // Mouse button release detected.
        //
        case USBH_EVENT_HID_MS_REL:
        {
            UARTprintf("Button Released %02x\n", ulMsgParam);

            //
            // Remove the button from the pressed state.
            //
            g_ulButtons &= ~ulMsgParam;

            break;
        }

        //
        // Mouse X movement detected.
        //
        case USBH_EVENT_HID_MS_X:
        {
            UARTprintf("X:%02d.\n", (signed char)ulMsgParam);

            //
            // Update the cursor on the screen.
            //
            UpdateCursor((signed char)ulMsgParam, 0);

            break;
        }

        //
        // Mouse Y movement detected.
        //
        case USBH_EVENT_HID_MS_Y:
        {
            UARTprintf("Y:%02d.\n", (signed char)ulMsgParam);

            //
            // Update the cursor on the screen.
            //
            UpdateCursor(0, (signed char)ulMsgParam);

            break;
        }
    }

    //
    // Update the status area of the screen.
    //
    UpdateStatus();

    return(0);
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
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Configure the relevant pins such that UART0 owns them.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Open UART0 for debug output.
    //
    UARTStdioInit(0);

    //
    // Enable the USB mux GPIO.
    //
    SysCtlPeripheralEnable(USB_MUX_GPIO_PERIPH);

    //
    // The LM3S3748 board uses a USB mux that must be switched to use the
    // host connector and not the device connector.
    //
    GPIOPinTypeGPIOOutput(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN);
    GPIOPinWrite(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN, USB_MUX_SEL_HOST);

    //
    // Configure the power pins for host controller.
    //
    GPIOPinTypeUSBDigital(GPIO_PORTH_BASE, GPIO_PIN_3 | GPIO_PIN_4);

    //
    // Configure the USB mux pins as gpios.
    //
    GPIOPinTypeGPIOOutput(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN);
    GPIOPinWrite(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN, USB_MUX_SEL_HOST);

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
    sRect.sYMax = DISPLAY_BANNER_HEIGHT;

    //
    // Set the banner background.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_FG);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "usb_host_mouse", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ulNumHostClassDrivers);

    //
    // Initialized the cursor.
    //
    g_ulButtons = 0;
    g_sCursor.sXMin = GrContextDpyWidthGet(&g_sContext) / 2;
    g_sCursor.sXMax = g_sCursor.sXMin + DISPLAY_MOUSE_SIZE;
    g_sCursor.sYMin = GrContextDpyHeightGet(&g_sContext) / 2;
    g_sCursor.sYMax = g_sCursor.sYMin + DISPLAY_MOUSE_SIZE;

    //
    // Update the status on the screen.
    //
    UpdateStatus();

    //
    // Update the cursor once to display it.
    //
    UpdateCursor(0, 0);

    //
    // Open and instance of the mouse class driver.
    //
    UARTprintf("Host Mouse Application\n");

    //
    // Open an instance of the mouse driver.  The mouse does not need
    // to be present at this time, this just saves a place for it and allows
    // the applications to be notified when a mouse is present.
    //
    g_ulMouseInstance =
        USBHMouseOpen(MouseCallback, g_pucBuffer, MOUSE_MEMORY_SIZE);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH);

    //
    // Initialize the host controller stack.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);

    //
    // Call the main loop for the Host controller driver.
    //
    USBHCDMain();

    //
    // The main loop for the application.
    //
    while(1)
    {
        switch(eUSBState)
        {
            //
            // This state is entered when the mouse is first detected.
            //
            case STATE_MOUSE_INIT:
            {
                //
                // Initialize the newly connected mouse.
                //
                USBHMouseInit(g_ulMouseInstance);

                //
                // Proceed to the mouse connected state.
                //
                eUSBState = STATE_MOUSE_CONNECTED;

                //
                // Update the status on the screen.
                //
                UpdateStatus();

                break;
            }
            case STATE_MOUSE_CONNECTED:
            {
                //
                // Nothing is currently done in the main loop when the mouse
                // is connected.
                //
                break;
            }
            case STATE_NO_DEVICE:
            {
                //
                // The mouse is not connected so nothing needs to be done here.
                //
                break;
            }
            default:
            {
                break;
            }
        }

        //
        // Periodically call the main loop for the Host controller driver.
        //
        USBHCDMain();
    }
}
