//*****************************************************************************
//
// hello_widget.c - Simple hello world example using
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
// This is part of revision 9453 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/locator.h"
#include "utils/ustdlib.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Hello using Widgets (hello_widget)</h1>
//!
//! A very simple ``hello world'' example written using the Stellaris Graphics
//! Library widgets.  It displays a button which, when pressed, toggles
//! display of the words ``Hello World!'' on the screen.  This may be used as
//! a starting point for more complex widget-based applications.
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.
//
//*****************************************************************************

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tCanvasWidget g_sIPAddr;
extern tPushButtonWidget g_sPushBtn;

//*****************************************************************************
//
// Forward reference to the button press handler.
//
//*****************************************************************************
void OnButtonPress(tWidget *pWidget);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, &g_sBackground, &g_sIPAddr, &g_sPushBtn,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "hello-widget", 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sHeading,
       &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 23),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The button used to hide or display the "Hello World" message.
//
//*****************************************************************************
RectangularButton(g_sPushBtn, &g_sHeading, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 60, 60, 200, 40,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
                   g_pFontCmss22b, "Show Welcome", 0, 0, 0, 0, OnButtonPress);

//*****************************************************************************
//
// The canvas widget used to display the "Hello!" string.  Note that this
// is NOT hooked into the active widget tree (by making it a child of the
// g_sPushBtn widget above) yet since we do not want the widget to be displayed
// until the button is pressed.
//
//*****************************************************************************
Canvas(g_sHello, &g_sPushBtn, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 150, 320, 40,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontCm40, "Hello World!", 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the MAC address.
//
//*****************************************************************************
#define SIZE_MAC_ADDR_BUFFER 32
char g_pcMACString[SIZE_MAC_ADDR_BUFFER];
Canvas(g_sMACAddr, &g_sBackground, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 230, 160, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontFixed6x8, g_pcMACString, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the IP address.
//
//*****************************************************************************
#define SIZE_IP_ADDR_BUFFER 24
char g_pcIPString[SIZE_IP_ADDR_BUFFER];
Canvas(g_sIPAddr, &g_sBackground, &g_sMACAddr, 0,
       &g_sKitronix320x240x16_SSD2119, 160, 230, 160, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontFixed6x8, g_pcIPString, 0, 0);

//*****************************************************************************
//
// A global we use to keep track of whether or not the "Hello" widget is
// currently visible.
//
//*****************************************************************************
tBoolean g_bHelloVisible = false;

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

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
// This is the handler for this SysTick interrupt.  We use this to provide the
// required timer call to the lwIP stack.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
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
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

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
    // Format this address into a string and display it.
    //
    usnprintf(g_pcMACString, SIZE_MAC_ADDR_BUFFER,
              "MAC: %02X-%02X-%02X-%02X-%02X-%02X",
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
    LocatorAppTitleSet("RDK-IDM-SBC hello_widget");

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
        usprintf(g_pcIPString, "IP: %d.%d.%d.%d",
                 ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                 (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
        WidgetPaint((tWidget *)&g_sIPAddr);
    }

    //
    // Return our current IP address.
    //
    return(ulIPAddr);
}

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
// This function is called by the graphics library widget manager in the
// context of WidgetMessageQueueProcess whenever the user releases the
// "Press Me!" button.  We use this notification to display or hide the
// "Hello!" widget.
//
// This is actually a rather inefficient way to accomplish this but it's
// a good example of how to add and remove widgets dynamically.  In
// normal circumstances, you would likely leave the g_sHello widget
// linked into the tree and merely add or remove the text by changing
// its style then repainting.
//
// If using this dynamic add/remove strategy, another useful optimization
// is to use a black canvas widget that covers the same area of the screen
// as the widgets that you will be adding and removing.  If this is the used
// as the point in the tree where the subtree is added or removed, you can
// repaint just the desired area by repainting the black canvas rather than
// repainting the whole tree.
//
//*****************************************************************************
void
OnButtonPress(tWidget *pWidget)
{
    g_bHelloVisible = !g_bHelloVisible;

    if(g_bHelloVisible)
    {
        //
        // Add the Hello widget to the tree as a child of the push
        // button.  We could add it elsewhere but this seems as good a
        // place as any.  It also means we can repaint from g_sPushBtn and
        // this will paint both the button and the welcome message.
        //
        WidgetAdd((tWidget *)&g_sPushBtn, (tWidget *)&g_sHello);

        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sPushBtn, "Hide Welcome");

        //
        // Repaint the pushbutton and all widgets beneath it (in this case,
        // the welcome message).
        //
        WidgetPaint((tWidget *)&g_sPushBtn);
    }
    else
    {
        //
        // Remove the Hello widget from the tree.
        //
        WidgetRemove((tWidget *)&g_sHello);

        //
        // Change the button text to indicate the new function.
        //
        PushButtonTextSet(&g_sPushBtn, "Show Welcome");

        //
        // Repaint the widget tree to remove the Hello widget from the
        // display.  This is rather inefficient but saves having to use
        // additional widgets to overpaint the area of the Hello text (since
        // disabling a widget does not automatically erase whatever it
        // previously displayed on the screen).
        //
        WidgetPaint(WIDGET_ROOT);
    }
}

//*****************************************************************************
//
// Print "Hello World!" to the display on the Intelligent Display Module.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIPAddr;

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout correctly for the IDM-SBC board.
    //
    PinoutSet();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Turn on the display backlight at full brightness.
    //
    Kitronix320x240x16_SSD2119BacklightOn(255);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Initialize the Ethernet hardware and lwIP TCP/IP stack.
    //
    ulIPAddr = TCPIPStackInit();

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);

    //
    // Paint the widget tree to make sure they all appear on the display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop processing widget messages until we are signalled that a remote
    // firmware update has been requested.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();

        //
        // Check for assignment of an IP address or a change in the address.
        //
        ulIPAddr = IPAddressChangeCheck(ulIPAddr);
    }

    //
    // If we drop out, a remote firmware update request has been received.
    // Let the user know what is going on then transfer control to the boot
    // loader.
    //
    PushButtonTextSet(&g_sPushBtn, "Updating Firmware");
    WidgetPaint((tWidget *)&g_sPushBtn);
    WidgetMessageQueueProcess();

    //
    // Transfer control to the bootloader.
    //
    SoftwareUpdateBegin();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
