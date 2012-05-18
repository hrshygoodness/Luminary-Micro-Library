//*****************************************************************************
//
// scribble.c - A simple scribble pad to demonstrate the touch screen driver.
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
// This is part of revision 8555 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/ringbuf.h"
#include "utils/swupdate.h"
#include "utils/ustdlib.h"
#include "drivers/formike240x320x16_ili9320.h"
#include "drivers/touch.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Scribble Pad (scribble)</h1>
//!
//! The scribble pad provides a drawing area on the screen.  Touching the
//! screen will draw onto the drawing area using a selection of fundamental
//! colors (in other words, the seven colors produced by the three color
//! channels being either fully on or fully off).  Each time the screen is
//! touched to start a new drawing, the drawing area is erased and the next
//! color is selected.
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
// A structure used to pass touchscreen messages from the interrupt-context
// handler function to the main loop for processing.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulMsg;
    long lX;
    long lY;
}
tScribbleMessage;

//*****************************************************************************
//
// The number of messages we can store in the message queue.
//
//*****************************************************************************
#define MSG_QUEUE_SIZE 16

//*****************************************************************************
//
// The ring buffer memory and control structure we use to implement the
// message queue.
//
//*****************************************************************************
static tScribbleMessage g_psMsgQueueBuffer[MSG_QUEUE_SIZE];
static tRingBufObject g_sMsgQueue;

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
// The colors that are used to draw on the screen.
//
//*****************************************************************************
static const unsigned long g_pulColors[] =
{
    ClrWhite,
    ClrYellow,
    ClrMagenta,
    ClrRed,
    ClrCyan,
    ClrLime,
    ClrBlue
};

//*****************************************************************************
//
// The index to the current color in use.
//
//*****************************************************************************
static unsigned long g_ulColorIdx;

//*****************************************************************************
//
// The previous pen position returned from the touch screen driver.
//
//*****************************************************************************
static long g_lX, g_lY;

//*****************************************************************************
//
// The drawing context used to draw to the screen.
//
//*****************************************************************************
static tContext g_sContext;

//*****************************************************************************
//
// A flag indicating whether or not a remote firmware update request has been
// received.
//
//*****************************************************************************
static volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// Buffers to store the formatted strings containing the board's IP amd MAC
// addresses.
//
//*****************************************************************************
static char g_pcMACString[32];
static char g_pcIPAddrString[32];

//*****************************************************************************
//
// The array of strings we display at the bottom of the screen.  These are
// shown one after the other with a 2 second delay between each.
//
//*****************************************************************************
static char *g_pcInfoStrings[2] =
{
    g_pcMACString,
    g_pcIPAddrString
};

//*****************************************************************************
//
// The index of the string which is to be displayed next.
//
//*****************************************************************************
static unsigned char g_ucInfoIndex = 0;

//*****************************************************************************
//
// A flag used to indicate that it is time to change the info string on the
// display.
//
//*****************************************************************************
static volatile tBoolean g_bInfoStringUpdate = false;

//*****************************************************************************
//
// The number of milliseconds each info string is displayed before it is
// changed.
//
//*****************************************************************************
#define INFO_UPDATE_PERIOD_MS 2000

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_TICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    static unsigned long ulDivide = 0;

    //
    // Update our time divider (used to time info string updates on the
    // display).
    //
    ulDivide += MS_PER_TICK;

    //
    // Is it time to update the info string display?
    //
    if(ulDivide >= INFO_UPDATE_PERIOD_MS)
    {
        //
        // Yes - clear the divider, toggle the string index and signal that
        // a display update is pending.
        //
        ulDivide = 0;
        g_ucInfoIndex = 1 - g_ucInfoIndex;
        g_bInfoStringUpdate = true;
    }

    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
}

//*****************************************************************************
//
// Transfer control to the bootloader to wait for an ethernet-based firmware
// update to occur.
//
//*****************************************************************************
void
UpdateFirmware(void)
{
    //
    // Tell the user what's up
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrStringDrawCentered(&g_sContext, "Updating firmware...", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2,
                         GrContextDpyHeightGet(&g_sContext) / 2,
                         true);

    //
    // Transfer control to the bootloader.
    //
    SoftwareUpdateBegin();
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
// The interrupt-context handler for touch screen events from the touch screen
// driver.  This function merely bundles up the event parameters and posts
// them to a message queue.  In the context of the main loop, they will be
// read from the queue and handled using TSMainHandler().
//
//*****************************************************************************
long TSHandler(unsigned long ulMessage, long lX, long lY)
{
    tScribbleMessage sMsg;

    //
    // Build the message that we will write to the queue.
    //
    sMsg.ulMsg = ulMessage;
    sMsg.lX = lX;
    sMsg.lY = lY;

    //
    // Make sure the queue isn't full. If it is, we just ignore this message.
    //
    if(!RingBufFull(&g_sMsgQueue))
    {
        RingBufWrite(&g_sMsgQueue, (unsigned char *)&sMsg,
                     sizeof(tScribbleMessage));
    }

    //
    // Tell the touch handler that everything is fine.
    //
    return(1);
}

//*****************************************************************************
//
// The main loop handler for touch screen events from the touch screen driver.
//
//*****************************************************************************
long
TSMainHandler(unsigned long ulMessage, long lX, long lY)
{
    tRectangle sRect;

    //
    // See which event is being sent from the touch screen driver.
    //
    switch(ulMessage)
    {
        //
        // The pen has just been placed down.
        //
        case WIDGET_MSG_PTR_DOWN:
        {
            //
            // Erase the drawing area.
            //
            GrContextForegroundSet(&g_sContext, ClrBlack);
            sRect.sXMin = 1;
            sRect.sYMin = 45;
            sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 2;
            sRect.sYMax = GrContextDpyHeightGet(&g_sContext) - 2;
            GrRectFill(&g_sContext, &sRect);

            //
            // Flush any cached drawing operations.
            //
            GrFlush(&g_sContext);

            //
            // Set the drawing color to the current pen color.
            //
            GrContextForegroundSet(&g_sContext, g_pulColors[g_ulColorIdx]);

            //
            // Save the current position.
            //
            g_lX = lX;
            g_lY = lY;

            //
            // This event has been handled.
            //
            break;
        }

        //
        // Then pen has moved.
        //
        case WIDGET_MSG_PTR_MOVE:
        {
            //
            // Draw a line from the previous position to the current position.
            //
            GrLineDraw(&g_sContext, g_lX, g_lY, lX, lY);

            //
            // Flush any cached drawing operations.
            //
            GrFlush(&g_sContext);

            //
            // Save the current position.
            //
            g_lX = lX;
            g_lY = lY;

            //
            // This event has been handled.
            //
            break;
        }

        //
        // The pen has just been picked up.
        //
        case WIDGET_MSG_PTR_UP:
        {
            //
            // Draw a line from the previous position to the current position.
            //
            GrLineDraw(&g_sContext, g_lX, g_lY, lX, lY);

            //
            // Flush any cached drawing operations.
            //
            GrFlush(&g_sContext);

            //
            // Increment to the next drawing color.
            //
            g_ulColorIdx++;
            if(g_ulColorIdx == (sizeof(g_pulColors) / sizeof(g_pulColors[0])))
            {
                g_ulColorIdx = 0;
            }

            //
            // This event has been handled.
            //
            break;
        }
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// This function is called in the context of the main loop to process any
// touch screen messages that have been sent.  Messages are posted to a
// queue from the message handler and pulled off here.  This is required
// since it is not safe to have two different execution contexts performing
// graphics operations using the same graphics context.
//
//*****************************************************************************
void
ProcessTouchMessages(void)
{
    tScribbleMessage sMsg;

    while(!RingBufEmpty(&g_sMsgQueue))
    {
        //
        // Get the next message.
        //
        RingBufRead(&g_sMsgQueue, (unsigned char *)&sMsg,
                    sizeof(tScribbleMessage));

        //
        // Dispatch it to the handler.
        //
        TSMainHandler(sMsg.ulMsg, sMsg.lX, sMsg.lY);
    }
}

//*****************************************************************************
//
// Provides a scribble pad using the display on the Intelligent Display Module.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect, sRectInfo;
    unsigned long ulUser0, ulUser1, ulIPAddr;
    unsigned char pucMACAddr[6];

    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Set the clocking to run from the PLL.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable SysTick to provide a periodic interrupt.  This is used to provide
    // a tick for the TCP/IP stack.
    //
    SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

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
    LocatorAppTitleSet("RDK-IDM scribble");

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
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike240x320x16_ILI9320);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 23;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
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
    GrStringDrawCentered(&g_sContext, "scribble", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 11, 0);

    //
    // Print the instructions across the top of the screen in white with a 20
    // point san-serif font.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrContextFontSet(&g_sContext, g_pFontCmss20);
    GrStringDrawCentered(&g_sContext, "Touch the screen to draw", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 34, 0);

    //
    // Print the board's MAC address (needed to initiate a remote firmware
    // update via the LMFlash utility) at the bottom of the display.
    //
    usprintf(g_pcMACString,
             "MAC: %02x-%02x-%02x-%02x-%02x-%02x",
             pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
             pucMACAddr[4], pucMACAddr[5]);
    GrStringDrawCentered(&g_sContext, g_pcMACString, -1,
                         GrContextDpyWidthGet(&g_sContext) / 2,
                         GrContextDpyHeightGet(&g_sContext) - 10, 0);

    //
    // Initialise the IP address string to show that we don't have one yet.
    //
    usprintf(g_pcIPAddrString, "      IP: Not Assigned      ");
    ulIPAddr = 0;

    //
    // Draw a green box around the scribble area.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 44;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = GrContextDpyHeightGet(&g_sContext) - 21;
    GrContextForegroundSet(&g_sContext, ClrGreen);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&g_sContext);

    //
    // Set the clipping region so that drawing can not occur outside the green
    // box.
    //
    sRect.sXMin++;
    sRect.sYMin++;
    sRect.sXMax--;
    sRect.sYMax--;
    GrContextClipRegionSet(&g_sContext, &sRect);

    //
    // Set up the clipping region we will use when updating the info string
    //
    sRectInfo.sXMin = 0;
    sRectInfo.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRectInfo.sYMax = GrContextDpyHeightGet(&g_sContext) - 1;
    sRectInfo.sYMin = sRectInfo.sYMax - 19;

    //
    // Set the color index to zero.
    //
    g_ulColorIdx = 0;

    //
    // Initialize the message queue we use to pass messages from the touch
    // interrupt handler context to the main loop for processing.
    //
    RingBufInit(&g_sMsgQueue, (unsigned char *)g_psMsgQueueBuffer,
                (MSG_QUEUE_SIZE * sizeof(tScribbleMessage)));

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(TSHandler);

    //
    // Loop forever.  All the drawing is done in the touch screen event
    // handler.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // If we have not had an IP address assigned yet, see if it is now.
        //
        if(ulIPAddr == 0)
        {
            ulIPAddr = lwIPLocalIPAddrGet();

            //
            // If it changed, update the info string containing the address.
            // This will cycle onto the display soon.
            //
            if(ulIPAddr != 0)
            {
                usprintf(g_pcIPAddrString, "      IP: %d.%d.%d.%d      ",
                         ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                         (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
            }
        }

        //
        // Check to see if there are any touchscreen messages to process and
        // handle any that are in the queue.
        //
        ProcessTouchMessages();

        //
        // If we are being asked to redisplay the info string, do it here.
        //
        if(g_bInfoStringUpdate)
        {
            //
            // Clear the flag now that we have seen it.
            //
            g_bInfoStringUpdate = false;

            //
            // Draw the relevant string at the bottom of the display.
            //
            GrContextClipRegionSet(&g_sContext, &sRectInfo);
            GrContextForegroundSet(&g_sContext, ClrWhite);
            GrStringDrawCentered(&g_sContext, g_pcInfoStrings[g_ucInfoIndex],
                                 -1, GrContextDpyWidthGet(&g_sContext) / 2,
                                 GrContextDpyHeightGet(&g_sContext) - 10,
                                 true);
            GrContextClipRegionSet(&g_sContext, &sRect);
            GrContextForegroundSet(&g_sContext, g_pulColors[g_ulColorIdx]);
        }
    }

    //
    // If we drop out, we've been asked to update the firmware image so
    // pass control to the boot loader.
    //
    UpdateFirmware();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
