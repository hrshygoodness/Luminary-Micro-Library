//*****************************************************************************
//
// otg_test_tempest.c - Simple test to investigate Tempest USB OTG signaling.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_usb.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/usb.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/device/usbdevice.h"
#include "usblib/host/usbhost.h"
#ifdef DEBUG
#include "utils/uartstdio.h"
#endif
#include "usbdescriptors.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/set_pinout.h"
#include "otg_detect.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB OTG HID Mouse Example (otg_detect)</h1>
//!
//! This example application demonstrates the use of USB On-The-Go (OTG) to
//! offer both USB host and device operation.  When the EK board is connected
//! to a USB host, it acts as a BIOS-compatible USB mouse.  The user button
//! on the board (nearest the USB OTG connector) acts as mouse button 1 and
//! the mouse pointer may be moved by dragging your finger or a stylus across
//! the touchscreen in the desired direction.
//!
//! If a USB mouse is connected to the USB OTG port, the board operates as a
//! USB host and draws dots on the display to track the mouse movement.  The
//! states of up to three mouse buttons are shown at the bottom right of the
//! display.
//
//*****************************************************************************

//*****************************************************************************
//
// The current state of the USB in the system based on the detected mode.
//
//*****************************************************************************
volatile tUSBMode g_eCurrentUSBMode = USB_MODE_NONE;

//*****************************************************************************
//
// The saved number of clock ticks per millisecond.
//
//*****************************************************************************
unsigned long g_ulClockMS;

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
// This global is used to indicate to the main loop that a mode change has
// occured.
//
//*****************************************************************************
unsigned long g_ulNewState;

//*****************************************************************************
//
// Some defines used to help define the screen.
//
//*****************************************************************************
#define LINE_HEIGHT             8
#define LINE_YPOS               4

//*****************************************************************************
//
// The graphics context for the screen.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// A function which returns the number of milliseconds since it was last
// called.  This can be found in usb_dev_mouse.c.
//
//*****************************************************************************
extern unsigned long GetTickms(void);

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
// Callback function for mode changes.
//
//*****************************************************************************
void
ModeCallback(unsigned long ulIndex, tUSBMode eMode)
{
    //
    // Save the new mode.
    //

    g_eCurrentUSBMode = eMode;

    switch(eMode)
    {
        case USB_MODE_HOST:
        {
            DEBUG_PRINT("\nHost Mode.\n");
            break;
        }
        case USB_MODE_DEVICE:
        {
            DEBUG_PRINT("\nDevice Mode.\n");
            break;
        }
        case USB_MODE_NONE:
        {
            DEBUG_PRINT("\nIdle Mode.\n");
            break;
        }
        default:
        {
            DEBUG_PRINT("ERROR: Bad Mode!\n");
            break;
        }
    }
    g_ulNewState = 1;
}

//*****************************************************************************
//
// Capture one sequence of DEVCTL register values during a session request.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 14;
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
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "OTG Example", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

#ifdef DEBUG
    //
    // Configure the relevant pins such that UART0 owns them.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Open the UART for I/O
    //
    UARTStdioInit(0);
#endif

    //
    // Determine the number of SysCtlDelay loops required to delay 1mS.
    //
    g_ulClockMS = ROM_SysCtlClockGet() / (3 * 1000);

    //
    // Configure the required pins for USB operation.
    //
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

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
    // Set the new state so that the screen updates on the first
    // pass.
    //
    g_ulNewState = 1;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Tell the OTG library code how much time has passed in milliseconds
        // since the last call.
        //
        USBOTGMain(GetTickms());

        //
        // Handle deferred state change.
        //
        if(g_ulNewState)
        {
            g_ulNewState =0;

            //
            // Update the status area of the screen.
            //
            ClearMainWindow();

            //
            // Update the status bar with the new mode.
            //
            switch(g_eCurrentUSBMode)
            {
                case USB_MODE_HOST:
                {
                    UpdateStatus("Host Mode", 0, true);
                    break;
                }
                case USB_MODE_DEVICE:
                {
                    UpdateStatus("Device Mode", 0, true);
                    break;
                }
                case USB_MODE_NONE:
                {
                    UpdateStatus("Idle Mode", 0, true);
                    break;
                }
                default:
                {
                    break;
                }
            }

        }

        if(g_eCurrentUSBMode == USB_MODE_DEVICE)
        {
            DeviceMain();
        }
        else if(g_eCurrentUSBMode == USB_MODE_HOST)
        {
            HostMain();
        }
    }
}
