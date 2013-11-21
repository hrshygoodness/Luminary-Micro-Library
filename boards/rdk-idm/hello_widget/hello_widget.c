//*****************************************************************************
//
// hello_widget.c - Simple hello world example using
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
// This is part of revision 10636 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/ustdlib.h"
#include "drivers/formike240x320x16_ili9320.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Hello using Widgets (hello_widget)</h1>
//!
//! A very simple ``hello world'' example written using the Stellaris Graphics
//! Library widgets.  It displays a button which, when pressed, shows ``Hello
//! World!'' on the screen.  This may be used as a starting point for more
//! complicated widget-based applications.
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.  This
//! feature is available in versions of LM Flash Programmer with build numbers
//! greater than 560.
//
//*****************************************************************************

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tPushButtonWidget g_sPushBtn;

//*****************************************************************************
//
// Forward reference to the button press handler.
//
//*****************************************************************************
void OnButtonPress(tWidget *pWidget);

//*****************************************************************************
//
// The canvas widget used to display the device MAC address and the buffer
// used to hold the MAC address string.
//
//*****************************************************************************
char g_pcMACAddr[32];
Canvas(g_sMACAddr, &g_sBackground, 0, 0,
       &g_sFormike240x320x16_ILI9320, 0, 310, 240, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontFixed6x8, g_pcMACAddr, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the device IP address and the buffer
// used to hold the IP address string.
//
//*****************************************************************************
char g_pcIPAddr[32];
Canvas(g_sIPAddr, &g_sBackground, &g_sMACAddr, 0,
       &g_sFormike240x320x16_ILI9320, 0, 300, 240, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontFixed6x8, g_pcIPAddr, 0, 0);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, &g_sBackground, &g_sIPAddr, &g_sPushBtn,
       &g_sFormike240x320x16_ILI9320, 0, 0, 240, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "hello-widget", 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sHeading,
       &g_sFormike240x320x16_ILI9320, 0, 23, 240, 320 - 23,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The button used to hide or display the "Hello World" message.
//
//*****************************************************************************
RectangularButton(g_sPushBtn, &g_sHeading, 0, 0,
                  &g_sFormike240x320x16_ILI9320, 20, 60, 200, 40,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite,
                   g_pFontCm30i, "Press Me!", 0, 0, 0, 0, OnButtonPress);

//*****************************************************************************
//
// The canvas widget used to display the "Hello!" string.  Note that this
// is NOT hooked into the active widget tree (by making it a child of the
// g_sPushBtn widget above) yet since we do not want the widget to be displayed
// until the button is pressed.
//
//*****************************************************************************
Canvas(g_sHello, &g_sPushBtn, 0, 0,
       &g_sFormike240x320x16_ILI9320, 0, 240, 240, 40,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontCm40, "Hello", 0, 0);

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

//*****************************************************************************
//
// A flag used to indicate that an ethernet remote firmware update request
// has been recieved.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// A global we use to keep track of whether or not the "Hello" widget is
// currently visible.
//
//*****************************************************************************
tBoolean g_bHelloVisible = false;

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.
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
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the bootloader to be entered the next time the user enters a
// command on the console.
//
//*****************************************************************************
void
SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
}


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
// as the point in the tree where the subtree is added to and removed
// from, you can repaint just the desired area by repainting the black
// canvas rather than repainting the whole tree.
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
        // place as any.
        //
        WidgetAdd((tWidget *)&g_sPushBtn, (tWidget *)&g_sHello);
        WidgetPaint((tWidget *)&g_sHello);
    }
    else
    {
        //
        // Remove the Hello widget from the tree.
        //
        WidgetRemove((tWidget *)&g_sHello);

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
    unsigned long ulUser0, ulUser1, ulLastIPAddr, ulIPAddr;
    unsigned char pucMACAddr[6];

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);


    //
    // Configure SysTick for a 100Hz interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    SysTickEnable();
    SysTickIntEnable();

    //
    // Enable Interrupts
    //
    IntMasterEnable();

    //
    // Get the MAC address from the user registers in NV ram.
    //
    FlashUserGet(&ulUser0, &ulUser1);

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
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM hello_widget");

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Initialize the display driver.
    //
    Formike240x320x16_ILI9320Init();

    //
    // Turn on the backlight.
    //
    Formike240x320x16_ILI9320BacklightOn();

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Display the MAC address (so that the user can perform a firmware update
    // if required).
    //
    usprintf(g_pcMACAddr,
             "MAC: %02x-%02x-%02x-%02x-%02x-%02x",
             pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
             pucMACAddr[4], pucMACAddr[5]);

    //
    // Initialize the text shown by the IP address canvas widget.
    //
    usprintf(g_pcIPAddr, "IP: Not assigned");

    //
    // Assume we don't have an IP address yet.
    //
    ulLastIPAddr = 0;

    //
    // Paint the widget tree to make sure they all appear on the display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop until someone requests a remote firmware update.  Inside the loop,
    // we check the IP address and update the display.  This information is
    // needed to allow someone to configure the LMFlash application to update
    // the board but the IP address is likely not available by the time we
    // get here initially.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // What is our current IP address?
        //
        ulIPAddr = lwIPLocalIPAddrGet();

        //
        // If it changed, update the display.
        //
        if(ulIPAddr != ulLastIPAddr)
        {
            ulLastIPAddr = ulIPAddr;

            usprintf(g_pcIPAddr, "IP: %d.%d.%d.%d",
                     ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                     (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
            WidgetPaint((tWidget *)&g_sIPAddr);
        }

        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();
    }

    //
    // If we drop out, a remote firmware update request has been received.
    //

    //
    // Tell the user what's up
    //
    usprintf(g_pcIPAddr, "Updating firmware...");
    usprintf(g_pcMACAddr, "");
    WidgetPaint((tWidget *)&g_sIPAddr);
    WidgetPaint((tWidget *)&g_sMACAddr);

    //
    // Process all remaining widget messages (to ensure that the last
    // two WidgetPaint() calls actually take effect).
    //
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
