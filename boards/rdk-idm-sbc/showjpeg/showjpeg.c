//*****************************************************************************
//
// showjpeg.c - Example program to decompress and display a JPEG image.
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

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "driverlib/epi.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/slider.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "drivers/sdram.h"
#include "drivers/jpgwidget.h"
#include "third_party/jpeg/jinclude.h"
#include "third_party/jpeg/jpeglib.h"
#include "third_party/jpeg/jerror.h"
#include "third_party/jpeg/jramdatasrc.h"

//*****************************************************************************
//
// The character array containing the JPEG image data used by this example.
//
//*****************************************************************************
#include "jpeg_image.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>JPEG Image Decompression (showjpeg)</h1>
//!
//! This example application decompresses a JPEG image which is linked into
//! the application and shows it on the 320x240 display.  SDRAM is used for
//! image storage and decompression workspace.  The image may be scrolled
//! in the display window by dragging a finger across the touchscreen.
//!
//! JPEG decompression and display are handled using a custom graphics library
//! widget, the source for which can be found in drivers/jpgwidget.c.
//!
//! The JPEG library used by this application is release 6b of the Independent
//! JPEG Group's reference decoder.  For more information, see the README and
//! various text file in the /third_party/jpeg directory or visit
//! http://www.ijg.org/.
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.
//
//*****************************************************************************

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

//*****************************************************************************
//
// The number of SysTicks between each redraw of the JPEG image while we are
// scrolling over it.  We throttle the redraws to 5 per second.
//
//*****************************************************************************
#define JPEG_REDRAW_TIMEOUT 20

//*****************************************************************************
//
// The number of SysTick ticks since the system started.
//
//*****************************************************************************
volatile unsigned long g_ulSysTickCount;

//*****************************************************************************
//
// A pointer to the decompressed image pixel data and the width and height of
// the image.
//
//*****************************************************************************
unsigned short *g_pusImageData;
unsigned short g_usImageHeight;
unsigned short g_usImageWidth;
tBoolean g_bImageAvailable;

//*****************************************************************************
//
// The current positions of the horizontal (X) and vertical (Y) sliders.
//
//*****************************************************************************
unsigned short g_usSliderX;
unsigned short g_usSliderY;

//*****************************************************************************
//
// A flag used to indicate that an ethernet remote firmware update request
// has been recieved.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// Prototypes for widget message handler functions.
//
//*****************************************************************************
extern void OnJPEGScroll(tWidget *pWidget, short sX, short sY);

//*****************************************************************************
//
// Widget definitions
//
//*****************************************************************************

//*****************************************************************************
//
// Forward references
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;

//*****************************************************************************
//
// Workspace for the JPEG canvas widget.
//
//*****************************************************************************
tJPEGInst g_sJPEGInst;

//*****************************************************************************
//
// The JPEG canvas widget used to hold the decompressed JPEG image
// display.
//
//*****************************************************************************
#define IMAGE_LEFT      0
#define IMAGE_TOP       25
#define IMAGE_WIDTH     320
#define IMAGE_HEIGHT    200
JPEGCanvas(g_sImage, &g_sBackground, 0, 0,
          &g_sKitronix320x240x16_SSD2119, IMAGE_LEFT, IMAGE_TOP, IMAGE_WIDTH,
          IMAGE_HEIGHT, (JW_STYLE_OUTLINE | JW_STYLE_TEXT), ClrBlack, ClrWhite,
          ClrRed, g_pFontCmss40b, "", g_pucJPEGImage,
          sizeof(g_pucJPEGImage), 1, OnJPEGScroll, &g_sJPEGInst);

//*****************************************************************************
//
// The canvas widget used to display the board MAC address.
//
//*****************************************************************************
char g_pcMACAddr[32];
Canvas(g_sMACAddr, &g_sBackground, &g_sImage, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 230, 160, 10,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_FILL, ClrBlack, 0, ClrWhite,
       g_pFontFixed6x8, g_pcMACAddr, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the board IP address.
//
//*****************************************************************************
char g_pcIPAddr[32];
Canvas(g_sIPAddr, &g_sBackground, &g_sMACAddr, 0,
       &g_sKitronix320x240x16_SSD2119, 160, 230, 160, 10,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_FILL, ClrBlack, 0, ClrWhite,
       g_pFontFixed6x8, g_pcIPAddr, 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the whole display under the
// heading banner.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sIPAddr,
       &g_sKitronix320x240x16_SSD2119, 10, 60, 320, 230,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "showjpeg", 0, 0);

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a
// timer tick every 10 ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
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
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the bootloader to be entered the next time the user enters a
// command on the console.
//
//*****************************************************************************
void SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
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
// This callback is made whenever someone scrolls a JPEG canvas widget.
//
//*****************************************************************************
void
OnJPEGScroll(tWidget *pWidget, short sX, short sY)
{
    static unsigned long ulLastRedraw = 0;

    //
    // We use this callback as a way to pace the repainting of the JPEG
    // image in the widget.  We could set JW_STYLE_SCROLL and have the widget
    // repaint itself every time it receives a pointer move message but these
    // are very frequent so this is likely to result in a waste of CPU.
    // Instead, we monitor callbacks and repaint only if 200mS has passed
    // since the last time we repainted.
    //

    //
    // How long has it been since we last redrew?
    //
    if((g_ulSysTickCount - ulLastRedraw) > JPEG_REDRAW_TIMEOUT)
    {
        WidgetPaint((tWidget *)&g_sImage);
        ulLastRedraw = g_ulSysTickCount;
    }
}

//*****************************************************************************
//
// The program main function.  It performs initialization, then runs
// a command processing loop to read commands from the console.
//
//*****************************************************************************
int
main(void)
{
    tBoolean bRetcode;
    int iRetcode;
    unsigned long ulUser0, ulUser1, ulLastIPAddr, ulIPAddr;
    unsigned char pucMACAddr[6];

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

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
    // Set GPIO A0 and A1 as UART.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the Ethernet LEDs on PF2 and PF3.
    //  LED0        Bit 3   Output
    //  LED1        Bit 2   Output
    //
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Initialize the UART as a console for text I/O.
    //
    UARTStdioInit(0);

    //
    // Get the MAC address from the user registers in NV ram.
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
    // Remember that we don't have an IP address assigned yet.
    //
    ulLastIPAddr = 0;

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM-SBC showjpeg");

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Format the MAC address into a string for the display.
    //
    usprintf(g_pcMACAddr,
             "MAC: %02x-%02x-%02x-%02x-%02x-%02x",
             pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
             pucMACAddr[4], pucMACAddr[5]);

    //
    // Which silicon revision are we running on?
    //
    ulUser0 = HWREG(SYSCTL_DID0) & SYSCTL_DID0_MAJ_M;

    //
    // Initialize the SDRAM to run at 25MHz on A2 silicon or 50MHz on B1.
    //
    bRetcode = SDRAMInit((ulUser0 == SYSCTL_DID0_MAJ_REVA) ? 1 : 0,
              (EPI_SDRAM_CORE_FREQ_50_100 | EPI_SDRAM_FULL_POWER |
              EPI_SDRAM_SIZE_64MBIT), 1024);
    if(!bRetcode)
    {
        UARTprintf("Can't initialize SDRAM. Aborting.\n");
        while(1);
    }

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
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Print hello message to user on the serial terminal.
    //
    UARTprintf("\n\nJPEG Decompression and Display Example Program\n");

    //
    // Decompress the image we linked to the JPEG canvas widget
    //
    iRetcode = JPEGWidgetImageDecompress((tWidget *)&g_sImage);

    //
    // Was the decompression successful?
    //
    if(iRetcode != 0)
    {
        while(1)
        {
            //
            // Something went wrong during the decompression of the JPEG
            // image.  Hang here pending investigation.
            //
        }
    }

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Enter an (almost) infinite loop for reading and processing commands from
    //the user.
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
        // Handle any messages for the displayed widgets.
        //
        WidgetMessageQueueProcess();
    }

    //
    // If we drop out, a remote firmware update request has been received.
    //

    //
    // Tell the user what's up.
    //
    JPEGWidgetTextSet(&g_sImage, "Updating...");
    WidgetPaint((tWidget *)&g_sImage);
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
