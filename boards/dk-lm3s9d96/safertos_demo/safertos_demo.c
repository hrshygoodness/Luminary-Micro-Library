//*****************************************************************************
//
// safertos_demo.c - Simple SafeRTOS example.
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
// This is part of revision 10636 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "SafeRTOS/SafeRTOS_API.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/set_pinout.h"
#include "display_task.h"
#include "idle_task.h"
#include "led_task.h"
#include "lwip_task.h"
#include "spider_task.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>SafeRTOS Example (safertos_demo)</h1>
//!
//! This application utilizes SafeRTOS to perform a variety of tasks in a
//! concurrent fashion.  The following tasks are created:
//!
//! * An lwIP task, which serves up web pages via the Ethernet interface.  This
//!   is actually two tasks, one which runs the lwIP stack and one which
//!   manages the Ethernet interface (sending and receiving raw packets).
//!
//! * An LED task, which simply blinks the on-board LED at a user-controllable
//!   rate (changed via the web interface).
//!
//! * A set of spider tasks, each of which controls a spider that crawls around
//!   the LCD.  The speed at which the spiders move is controllable via the web
//!   interface.  Up to thirty-two spider tasks can be run concurrently (an
//!   application-imposed limit).
//!
//! * A spider control task, which manages presses on the touch screen and
//!   determines if a spider task should be terminated (if the spider is
//!   ``squished'') or if a new spider task should be created (if no spider is
//!   ``squished'').
//!
//! * There is an automatically created idle task, which monitors changes in
//!   the board's IP address and sends those changes to the user via a UART
//!   message.
//!
//! Across the bottom of the LCD, several status items are displayed:  the
//! amount of time the application has been running, the number of tasks that
//! are running, the IP address of the board, the number of Ethernet packets
//! that have been transmitted, and the number of Ethernet packets that have
//! been received.
//!
//! The finder application (in tools/finder) can also be used to discover the
//! IP address of the board.  The finder application will search the network
//! for all boards that respond to its requests and display information about
//! them.
//!
//! The web site served by lwIP includes the ability to adjust the toggle rate
//! of the LED task and the update speed of the spiders (all spiders move at
//! the same speed).
//!
//! For additional details on SafeRTOS, refer to the SafeRTOS web page at:
//! http://www.highintegritysystems.com/safertos/
//!
//! For additional details on lwIP, refer to the lwIP web page at:
//! http://savannah.nongnu.org/projects/lwip/
//
//*****************************************************************************

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
// This hook is called by SafeRTOS when an error is detected.
//
//*****************************************************************************
static void
SafeRTOSErrorHook(xTaskHandle xHandleOfTaskWithError,
                  signed portCHAR *pcNameOfTaskWithError,
                  portBASE_TYPE xErrorCode)
{
    tContext sContext;

    //
    // A fatal SafeRTOS error was detected, so display an error message.
    //
    GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);
    GrContextForegroundSet(&sContext, ClrRed);
    GrContextBackgroundSet(&sContext, ClrBlack);
    GrContextFontSet(&sContext, g_pFontCm20);
    GrStringDrawCentered(&sContext, "Fatal SafeRTOS error!", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                          24), 1);

    //
    // This function can not return, so loop forever.  Interrupts are disabled
    // on entry to this function, so no processor interrupts will interrupt
    // this loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// The parameters used to initialize SafeRTOS.
//
//*****************************************************************************
static xPORT_INIT_PARAMETERS g_sSafeRTOSPortInit =
{
    //
    // System clock rate.
    //
    80000000,

    //
    // Scheduler tick rate.
    //
    1000 / portTICK_RATE_MS,

    //
    // Task delete hook.
    //
    SafeRTOSTaskDeleteHook,

    //
    // Error hook.
    //
    SafeRTOSErrorHook,

    //
    // Idle hook.
    //
    SafeRTOSIdleHook,

    //
    // System stack location.
    //
    0,

    //
    // System stack size.
    //
    0,

    //
    // Vector table base.
    //
    0
};

//*****************************************************************************
//
// Initialize SafeRTOS and start the initial set of tasks.
//
//*****************************************************************************
int
main(void)
{
    tContext sContext;
    tRectangle sRect;

    //
    // Set the clocking to run at 80 MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Initialize the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.sYMax = 23;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, g_pFontCm20);
    GrStringDrawCentered(&sContext, "safertos-demo", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 10, 0);

    //
    // Set the location and size of the system stack.
    //
    g_sSafeRTOSPortInit.pulSystemStackLocation =
        (unsigned portLONG *)(*(unsigned long *)0);
    g_sSafeRTOSPortInit.ulSystemStackSizeBytes = 128 * 4;

    //
    // Set the location of the vector table.
    //
    g_sSafeRTOSPortInit.pulVectorTableBase =
        (unsigned portLONG *)HWREG(NVIC_VTABLE);

    //
    // Initialize the SafeRTOS kernel.
    //
    vTaskInitializeScheduler((signed portCHAR *)g_pulIdleTaskStack,
                             sizeof(g_pulIdleTaskStack), 0,
                             &g_sSafeRTOSPortInit);

    //
    // Create the display task.
    //
    if(DisplayTaskInit() != 0)
    {
        GrContextForegroundSet(&sContext, ClrRed);
        GrStringDrawCentered(&sContext, "Failed to create display task!", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                              24), 0);
        while(1)
        {
        }
    }

    //
    // Create the spider task.
    //
    if(SpiderTaskInit() != 0)
    {
        GrContextForegroundSet(&sContext, ClrRed);
        GrStringDrawCentered(&sContext, "Failed to create spider task!", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                              24), 0);
        while(1)
        {
        }
    }

    //
    // Create the LED task.
    //
    if(LEDTaskInit() != 0)
    {
        GrContextForegroundSet(&sContext, ClrRed);
        GrStringDrawCentered(&sContext, "Failed to create LED task!", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                              24), 0);
        while(1)
        {
        }
    }

    //
    // Create the lwIP tasks.
    //
    if(lwIPTaskInit() != 0)
    {
        GrContextForegroundSet(&sContext, ClrRed);
        GrStringDrawCentered(&sContext, "Failed to create lwIP tasks!", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             (((GrContextDpyHeightGet(&sContext) - 24) / 2) +
                              24), 0);
        while(1)
        {
        }
    }

    //
    // Start the scheduler.  This should not return.
    //
    xTaskStartScheduler(pdTRUE);

    //
    // In case the scheduler returns for some reason, print an error and loop
    // forever.
    //
    GrContextForegroundSet(&sContext, ClrRed);
    GrStringDrawCentered(&sContext, "Failed to start scheduler!", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         (((GrContextDpyHeightGet(&sContext) - 24) / 2) + 24),
                         0);
    while(1)
    {
    }
}
