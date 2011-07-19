//*****************************************************************************
//
// showjpeg.c - Example program to decompress and display a JPEG image.
//
// Copyright (c) 2009-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
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
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "drivers/extram.h"
#include "drivers/jpgwidget.h"
#include "drivers/extflash.h"
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
//! the application and shows it on the 320x240 display.  External RAM is used
//! for image storage and decompression workspace.  The image may be scrolled
//! in the display window by dragging a finger across the touchscreen.
//!
//! JPEG decompression and display are handled using a custom graphics library
//! widget, the source for which can be found in drivers/jpgwidget.c.
//!
//! The JPEG library used by this application is release 6b of the Independent
//! JPEG Group's reference decoder.  For more information, see the README and
//! various text file in the /third_party/jpeg directory or visit
//! <a href="http://www.ijg.org/">http://www.ijg.org/</a>.
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
#define IMAGE_HEIGHT    215
JPEGCanvas(g_sImage, &g_sBackground, 0, 0,
          &g_sKitronix320x240x16_SSD2119, IMAGE_LEFT, IMAGE_TOP, IMAGE_WIDTH,
          IMAGE_HEIGHT, JW_STYLE_OUTLINE, ClrBlack, ClrWhite, 0, 0, 0,
          g_pucJPEGImage, sizeof(g_pucJPEGImage), 1, OnJPEGScroll,
          &g_sJPEGInst);

//*****************************************************************************
//
// The canvas widget acting as the background to the whole display under the
// heading banner.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sImage,
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
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontCm20, "showjpeg", 0, 0);

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
    // If we have an SRAM/Flash daughter board installed, initialize it.
    //
    if(g_eDaughterType == DAUGHTER_SRAM_FLASH)
    {
        //
        // This checks that the daughter board flash is accessible.
        //
        bRetcode = ExtFlashPresent();
    }

    //
    // Set GPIO A0 and A1 as UART.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART as a console for text I/O.
    //
    UARTStdioInit(0);

    //
    // Initialize the SDRAM controller and heap if we have not detected any
    // other daughter board attached to the system.
    //
    if(g_eDaughterType == DAUGHTER_NONE)
    {
        bRetcode = SDRAMInit(1, (EPI_SDRAM_CORE_FREQ_50_100 |
                             EPI_SDRAM_FULL_POWER | EPI_SDRAM_SIZE_64MBIT),
                             1024);
    }
    else
    {
        //
        // If we have another daughter board, try to initialize the external
        // RAM heap with any SRAM that it may have.  This memory is used by
        // the JPEGWidget.
        //
        bRetcode = ExtRAMHeapInit();
    }

    if(!bRetcode)
    {
        UARTprintf("Can't initialize external RAM. Aborting.\n");
        while(1);
    }

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

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
    // Print hello message to user.
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
    while(1)
    {
        WidgetMessageQueueProcess();
    }
}
