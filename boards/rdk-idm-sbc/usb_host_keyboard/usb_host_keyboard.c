//*****************************************************************************
//
// usb_host_keyboard.c - main application code for the host keyboard example.
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
// This is part of revision 8555 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhhid.h"
#include "usblib/host/usbhhidkeyboard.h"
#include "utils/uartstdio.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/locator.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "drivers/set_pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Keyboard Host (usb_host_keyboard)</h1>
//!
//! This example application demonstrates how to support a USB keyboard
//! attached to the development kit board.  The display will show if a keyboard
//! is currently connected and the current state of the Caps Lock key on the
//! keyboard that is connected on the bottom status area of the screen.
//! Pressing any keys on the keyboard will cause them to be printed on the
//! screen and to be sent out the UART at 115200 baud with no parity, 8 bits
//! and 1 stop bit.  Any keyboard that supports the USB HID BIOS protocol
//! should work with this demo application.
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.
//
//*****************************************************************************

//*****************************************************************************
//
// The ASCII code for a backspace character.
//
//*****************************************************************************
#define ASCII_BACKSPACE 0x08

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
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the Keyboard class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_USBHIDClassDriver,
    &g_sUSBEventDriver
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
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Our running system tick counter and a global used to determine the time
// elapsed since last call to GetTickms().
//
//*****************************************************************************
unsigned long g_ulSysTickCount;
unsigned long g_ulLastTick;

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

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
    // Keyboard has received a key press that requires updating the keyboard
    // in the main loop.
    //
    STATE_KEYBOARD_UPDATE,

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
// These defines are used to define the screen constraints to the application.
//
//*****************************************************************************
#define DISPLAY_BANNER_HEIGHT   24
#define DISPLAY_BANNER_BG       ClrDarkBlue
#define DISPLAY_TEXT_BORDER     2
#define DISPLAY_TEXT_FG         ClrWhite
#define DISPLAY_TEXT_BG         ClrBlack

//*****************************************************************************
//
// This variable holds the current status of the modifiers keys.
//
//*****************************************************************************
unsigned long g_ulModifiers = 0;

//*****************************************************************************
//
// This is the number of characters that will fit on a line in the text area.
//
//*****************************************************************************
unsigned long g_ulCharsPerLine;

//*****************************************************************************
//
// This is the number of lines that will fit in the text area.
//
//*****************************************************************************
unsigned long g_ulLinesPerScreen;

//*****************************************************************************
//
// This is the current line for printing in the text area.
//
//*****************************************************************************
unsigned long g_ulLine = 0;

//*****************************************************************************
//
// This is the current column for printing in the text area.
//
//*****************************************************************************
unsigned long g_ulColumn = 0;

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
// The buffer used to render the Ethernet MAC address string.
//
//*****************************************************************************
#define SIZE_MAC_ADDR_BUFFER 32
char g_pcMACAddrString [SIZE_MAC_ADDR_BUFFER];

//*****************************************************************************
//
// The buffer used to render the Ethernet IP address string.
//
//*****************************************************************************
#define SIZE_IP_ADDR_BUFFER 24
char g_pcIPAddrString [SIZE_IP_ADDR_BUFFER];

//*****************************************************************************
//
// A signal used to tell the main loop to transfer control to the boot loader
// so that a firmware update can be performed over Ethernet.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// This function is called by the swupdate module whenever it receives a
// signal indicating that a remote firmware update request is being made.  This
// notification occurs in the context of the Ethernet interrupt handler so it
// is vital that you DO NOT transfer control to the boot loader directly from
// this function (since the boot loader does not like being entered in interrupt
// context).
//
//*****************************************************************************
void
SoftwareUpdateRequestCallback(void)
{
    //
    // Set the flag that tells the main task to transfer control to the boot
    // loader.
    //
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// Initialize the Ethernet hardware and lwIP TCP/IP stack and set up to listen
// for remote firmware update requests.
//
//*****************************************************************************
unsigned long
TCPIPStackInit(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACAddr[6];

    //
    // Configure the Ethernet LEDs on PF2 and PF3.
    //  LED0        Bit 3   Output
    //  LED1        Bit 2   Output
    //
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Get the MAC address from the UART0 and UART1 registers in NV ram.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV ram into a MAC address
    // array.
    //
    pucMACAddr[0] = ulUser0 & 0xff;
    pucMACAddr[1] = (ulUser0 >> 8) & 0xff;
    pucMACAddr[2] = (ulUser0 >> 16) & 0xff;
    pucMACAddr[3] = ulUser1 & 0xff;
    pucMACAddr[4] = (ulUser1 >> 8) & 0xff;
    pucMACAddr[5] = (ulUser1 >> 16) & 0xff;

    //
    // Format this address into a string for later display.
    //
    usnprintf(g_pcMACAddrString, SIZE_MAC_ADDR_BUFFER,
              "%02X-%02X-%02X-%02X-%02X-%02X",
              pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
              pucMACAddr[4], pucMACAddr[5]);

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM-SBC usb-host-keyboard");

    //
    // Start monitoring for the special packet that tells us a software
    // download is being requested.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Return our initial IP address.  This is 0 for now since we have not
    // had one assigned yet.
    //
    return(0);
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update our tick counter.
    //
    g_ulSysTickCount++;

    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
}

//*****************************************************************************
//
// This function prints the character out the UART and into the text area of
// the screen.
//
// \param ucChar is the character to print out.
//
// This function handles all of the detail of printing a character to both the
// UART and to the text area of the screen on the evaluation board.  The text
// area of the screen will be cleared any time the text goes beyond the end
// of the text area.
//
// \return None.
//
//*****************************************************************************
void
PrintChar(const char ucChar)
{
    tRectangle sRect;

    //
    // If both the line and column have gone to zero then clear the screen.
    //
    if((g_ulLine == 0) && (g_ulColumn == 0))
    {
        //
        // Form the rectangle that makes up the text box.
        //
        sRect.sXMin = 0;
        sRect.sYMin = DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER;
        sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - DISPLAY_TEXT_BORDER;
        sRect.sYMax = GrContextDpyHeightGet(&g_sContext) -
                      DISPLAY_BANNER_HEIGHT - DISPLAY_TEXT_BORDER;

        //
        // Change the foreground color to black and draw black rectangle to
        // clear the screen.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_TEXT_BG);
        GrRectFill(&g_sContext, &sRect);

        //
        // Reset the foreground color to the text color.
        //
        GrContextForegroundSet(&g_sContext, DISPLAY_TEXT_FG);
    }

    //
    // Send the character to the UART.
    //
    UARTprintf("%c", ucChar);

    //
    // Allow new lines to cause the column to go back to zero.
    //
    if(ucChar != '\n')
    {
        //
        // Did we get a backspace character?
        //
        if(ucChar != ASCII_BACKSPACE)
        {
            //
            // This is not a backspace so print the character to the screen.
            //
            GrStringDraw(&g_sContext, &ucChar, 1,
                         GrFontMaxWidthGet(g_pFontFixed6x8) * g_ulColumn,
                         DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                         (g_ulLine * GrFontHeightGet(g_pFontFixed6x8)), 0);
        }
        else
        {
            //
            // We got a backspace.  If we are at the top left of the screen,
            // return since we don't need to do anything.
            //
            if(g_ulColumn || g_ulLine)
            {
                //
                // Adjust the cursor position to erase the last character.
                //
                if(g_ulColumn)
                {
                    g_ulColumn--;
                }
                else
                {
                    g_ulColumn = g_ulCharsPerLine;
                    g_ulLine--;
                }

                //
                // Print a space at this position then return without fixing up
                // the cursor again.
                //
                GrStringDraw(&g_sContext, " ", 1,
                             GrFontMaxWidthGet(g_pFontFixed6x8) * g_ulColumn,
                             DISPLAY_BANNER_HEIGHT + DISPLAY_TEXT_BORDER +
                             (g_ulLine * GrFontHeightGet(g_pFontFixed6x8)),
                             true);
            }
            return;
        }
    }
    else
    {
        //
        // This will allow the code below to properly handle the new line.
        //
        g_ulColumn = g_ulCharsPerLine;
    }

    //
    // Update the text row and column that the next character will use.
    //
    if(g_ulColumn < g_ulCharsPerLine)
    {
        //
        // No line wrap yet so move one column over.
        //
        g_ulColumn++;
    }
    else
    {
        //
        // Line wrapped so go back to the first column and update the line.
        //
        g_ulColumn = 0;
        g_ulLine++;

        //
        // The line has gone past the end so go back to the first line.
        //
        if(g_ulLine >= g_ulLinesPerScreen)
        {
            g_ulLine = 0;
        }
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

    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);

    //
    // Update the status on the screen.
    //
    if(g_eUSBState == STATE_NO_DEVICE)
    {
        //
        // Keyboard is currently disconnected.
        //
        GrStringDraw(&g_sContext, "no device", -1, 4, sRect.sYMin + 8, 0);
    }
    else if(g_eUSBState == STATE_UNKNOWN_DEVICE)
    {
        //
        // Unknown device is currently connected.
        //
        GrStringDraw(&g_sContext, "unknown dev.", -1, 4, sRect.sYMin + 8, 0);
    }
    else if(g_eUSBState == STATE_POWER_FAULT)
    {
        //
        // Something caused a power fault.
        //
        GrStringDraw(&g_sContext, "power fault", -1, 4, sRect.sYMin + 8, 0);
    }
    else if((g_eUSBState == STATE_KEYBOARD_CONNECTED) ||
            (g_eUSBState == STATE_KEYBOARD_UPDATE))
    {
        //
        // Keyboard is connected.
        //
        GrStringDraw(&g_sContext, "connected", -1, 4, sRect.sYMin + 8, 0);

        //
        // Update the CAPS Lock status.
        //
        if(g_ulModifiers & HID_KEYB_CAPS_LOCK)
        {
            GrStringDraw(&g_sContext, "CAPS", 4, sRect.sXMax - 28,
                         sRect.sYMin + 8, 0);
        }
    }

    //
    // Display the MAC and IP addresses.
    //
    GrStringDraw(&g_sContext, g_pcMACAddrString, -1,
                 sRect.sXMin + 80, sRect.sYMin + 8, 0);
    GrStringDraw(&g_sContext, g_pcIPAddrString, -1,
                 sRect.sXMin + 190, sRect.sYMin + 8, 0);
}

//*****************************************************************************
//
// Check to see if the IP address has changed and, if so, update the
// display.
//
//*****************************************************************************
unsigned long IPAddressChangeCheck(unsigned long ulCurrentIP)
{
    unsigned long ulIPAddr;

    //
    // What is our current IP address?
    //
    ulIPAddr = lwIPLocalIPAddrGet();

    //
    // Has the IP address changed?
    //
    if(ulIPAddr != ulCurrentIP)
    {
        //
        // Yes - the address changed so update the display.
        //
        usprintf(g_pcIPAddrString, "%d.%d.%d.%d",
                 ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                 (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
        UpdateStatus();
    }

    //
    // Return our current IP address.
    //
    return(ulIPAddr);
}

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// \param pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the keyboard device.  At this
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
        // New keyboard detected.
        //
        case USB_EVENT_CONNECTED:
        {
            //
            // See if this is a HID Keyboard.
            //
            if((USBHCDDevClass(pEventInfo->ulInstance, 0) == USB_CLASS_HID) &&
               (USBHCDDevProtocol(pEventInfo->ulInstance, 0) ==
                USB_HID_PROTOCOL_KEYB))
            {
                //
                // Indicate that the keyboard has been detected.
                //
                UARTprintf("Keyboard Connected\n");

                //
                // Proceed to the STATE_KEYBOARD_INIT state so that the main
                // loop can finish initialized the mouse since
                // USBHKeyboardInit() cannot be called from within a callback.
                //
                g_eUSBState = STATE_KEYBOARD_INIT;
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
            g_eUSBState = STATE_UNKNOWN_DEVICE;

            UpdateStatus();
            break;
        }

        //
        // Keyboard has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            UARTprintf("Device Disconnected\n");

            //
            // Unknown device has been removed.
            //
            g_eUSBState = STATE_NO_DEVICE;

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
            g_eUSBState = STATE_POWER_FAULT;

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
unsigned long
KeyboardCallback(void *pvCBData, unsigned long ulEvent,
                 unsigned long ulMsgParam, void *pvMsgData)
{
    unsigned char ucChar;

    switch(ulEvent)
    {
        //
        // New Key press detected.
        //
        case USBH_EVENT_HID_KB_PRESS:
        {
            //
            // If this was a Caps Lock key then update the Caps Lock state.
            //
            if(ulMsgParam == HID_KEYB_USAGE_CAPSLOCK)
            {
                //
                // The main loop needs to update the keyboard's Caps Lock
                // state.
                //
                g_eUSBState = STATE_KEYBOARD_UPDATE;

                //
                // Toggle the current Caps Lock state.
                //
                g_ulModifiers ^= HID_KEYB_CAPS_LOCK;

                //
                // Update the screen based on the Caps Lock status.
                //
                UpdateStatus();
            }
            else if(ulMsgParam == HID_KEYB_USAGE_SCROLLOCK)
            {
                //
                // The main loop needs to update the keyboard's Scroll Lock
                // state.
                //
                g_eUSBState = STATE_KEYBOARD_UPDATE;

                //
                // Toggle the current Scroll Lock state.
                //
                g_ulModifiers ^= HID_KEYB_SCROLL_LOCK;
            }
            else if(ulMsgParam == HID_KEYB_USAGE_NUMLOCK)
            {
                //
                // The main loop needs to update the keyboard's Scroll Lock
                // state.
                //
                g_eUSBState = STATE_KEYBOARD_UPDATE;

                //
                // Toggle the current Num Lock state.
                //
                g_ulModifiers ^= HID_KEYB_NUM_LOCK;
            }
            else
            {
                //
                // Was this the backspace key?
                //
                if((unsigned char)ulMsgParam == HID_KEYB_USAGE_BACKSPACE)
                {
                    //
                    // Yes - set the ASCII code for a backspace key.  This is
                    // not returned by USBHKeyboardUsageToChar since this only
                    // returns printable characters.
                    //
                    ucChar = ASCII_BACKSPACE;
                }
                else
                {
                    //
                    // This is not backspace so try to map the usage code to a
                    // printable ASCII character.
                    //
                    ucChar = (unsigned char)
                        USBHKeyboardUsageToChar(g_ulKeyboardInstance,
                                                &g_sUSKeyboardMap,
                                                (unsigned char)ulMsgParam);
                }

                //
                // A zero value indicates there was no textual mapping of this
                // usage code.
                //
                if(ucChar != 0)
                {
                    PrintChar(ucChar);
                }
            }
            break;
        }
        case USBH_EVENT_HID_KB_MOD:
        {
            //
            // This application ignores the state of the shift or control
            // and other special keys.
            //
            break;
        }
        case USBH_EVENT_HID_KB_REL:
        {
            //
            // This applications ignores the release of keys as well.
            //
            break;
        }
    }
    return(0);
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

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;
    unsigned long ulIPAddr;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the part pinout appropriately for this device.
    //
    PinoutSet();

    //
    // Initially wait for device connection.
    //
    g_eUSBState = STATE_NO_DEVICE;

    //
    // Enable Clocking to the USB controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Configure the relevant pins such that UART0 owns them.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Open UART0 for debug output.
    //
    UARTStdioInit(0);

    //
    // Configure the power pins for host controller.
    //
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    //
    // Turn on USB Phy clock.
    //
    ROM_SysCtlUSBPLLEnable();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the Ethernet hardware and lwIP TCP/IP stack.
    //
    ulIPAddr = TCPIPStackInit();

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

    //
    // Call the host controller driver processing function to prime the pump.
    //
    USBHCDMain();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Turn on the display backlight.
    //
    Kitronix320x240x16_SSD2119BacklightOn(255);

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
    sRect.sYMax = DISPLAY_BANNER_HEIGHT - 1;
    GrContextForegroundSet(&g_sContext, DISPLAY_BANNER_BG);
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
    GrStringDrawCentered(&g_sContext, "usb-host-keyboard", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 8, 0);

    //
    // Calculate the number of characters that will fit on a line.
    // Make sure to leave a small border for the text box.
    //
    g_ulCharsPerLine = (GrContextDpyWidthGet(&g_sContext) - 4) /
                        GrFontMaxWidthGet(g_pFontFixed6x8);

    //
    // Calculate the number of lines per usable text screen.  This requires
    // taking off space for the top and bottom banners and adding a small bit
    // for a border.
    //
    g_ulLinesPerScreen = (GrContextDpyHeightGet(&g_sContext) -
                          (2*(DISPLAY_BANNER_HEIGHT + 1)))/
                          GrFontHeightGet(g_pFontFixed6x8);

    //
    // Open and instance of the keyboard class driver.
    //
    UARTprintf("Host Keyboard Application\n");

    //
    // Initial update of the screen.
    //
    UpdateStatus();

    //
    // The main loop for the application.  Keep running until we receive a
    // request for a remove firmware update from the LMFlash application.
    //
    while(!g_bFirmwareUpdate)
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

                //
                // Maintain the state of the modifiers when a new keyboard is
                // plugged in.
                //
                USBHKeyboardModifierSet(g_ulKeyboardInstance, g_ulModifiers);

                //
                // Update the screen now that the keyboard has been
                // initialized.
                //
                UpdateStatus();

                break;
            }
            case STATE_KEYBOARD_UPDATE:
            {
                //
                // If the application detected a change that required an
                // update to be sent to the keyboard to change the modifier
                // state then call it and return to the connected state.
                //
                g_eUSBState = STATE_KEYBOARD_CONNECTED;

                USBHKeyboardModifierSet(g_ulKeyboardInstance, g_ulModifiers);

                break;
            }
            case STATE_KEYBOARD_CONNECTED:
            {
                //
                // Nothing is currently done in the main loop when the keyboard
                // is connected.
                //
                break;
            }

            case STATE_UNKNOWN_DEVICE:
            {
                //
                // Nothing to do as the device is unknown.
                //
                break;
            }

            case STATE_NO_DEVICE:
            {
                //
                // Nothing is currently done in the main loop when the keyboard
                // is not connected.
                //
                break;
            }
            default:
            {
                break;
            }
        }

        //
        // See if we have an IP address assigned and update the
        // display if necessary.
        //
        ulIPAddr = IPAddressChangeCheck(ulIPAddr);
    }

    //
    // If we drop out of the loop, we must have received a remote firmware
    // update request.
    //
    GrContextFontSet(&g_sContext, g_pFontCmss22b);
    GrStringDrawCentered(&g_sContext, "Firmware Update...", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2,
                         GrContextDpyHeightGet(&g_sContext) / 2, true);

    //
    // Now that everything is complete, transfer control to the boot loader
    // to allow a firmware update to take place.
    //
    SoftwareUpdateBegin();
}
