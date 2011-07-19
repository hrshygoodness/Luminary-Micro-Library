//*****************************************************************************
//
// boot_demo_eth.c - Ethernet boot loader example application.
//
// Copyright (c) 2008-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/flash.h"
#include "driverlib/uart.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "utils/ustdlib.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet Boot Loader Demo (boot_demo_eth)</h1>
//!
//! An example to demonstrate the use of remote update signaling with the
//! flash-based Ethernet boot loader.  This application configures the Ethernet
//! controller and acquires an IP address which is displayed on the screen
//! along with the board's MAC address.  It then listens for a ``magic packet''
//! telling it that a firmware upgrade request is being made and, when this
//! packet is received, transfers control into the boot loader to perform the
//! upgrade.
//!
//! Although there are three flavors of flash-based boot loader provided with
//! this software release (boot_serial, boot_eth and boot_usb), this example is
//! specific to the Ethernet boot loader since the magic packet used to trigger
//! entry into the boot loader from the application is only sent via Ethernet.
//!
//! The boot_demo1 and boot_demo2 applications do not make use of the Ethernet
//! magic packet and can be used along with this application to easily
//! demonstrate that the boot loader is actually updating the on-chip flash.
//!
//! Note that the LM3S9B96 and other Tempest-class Stellaris devices also
//! support serial and ethernet boot loaders in ROM in silicon revisions B1 or
//! later.  To make use of this function, link your application to run at
//! address 0x0000 in flash and enter the bootloader using either the
//! ROM_UpdateEthernet or ROM_UpdateSerial functions (defined in rom.h).  This
//! mechanism is used in the utils/swupdate.c module when built specifically
//! targeting a suitable Tempest-class device.
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
// A global flag used to indicate if a remote firmware update request has been
// received.
//
//*****************************************************************************
static volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// Buffers used to hold the Ethernet MAC and IP addresses for the board.
//
//*****************************************************************************
#define SIZE_MAC_ADDR_BUFFER 32
#define SIZE_IP_ADDR_BUFFER 32
char g_pcMACAddr[SIZE_MAC_ADDR_BUFFER];
char g_pcIPAddr[SIZE_IP_ADDR_BUFFER];

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;

//*****************************************************************************
//
// Forward reference to the button press handler.
//
//*****************************************************************************
void OnButtonPress(tWidget *pWidget);

//*****************************************************************************
//
// The canvas widget used to display the board's Ethernet IP address.
//
//*****************************************************************************
Canvas(g_sIPAddr, &g_sBackground, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 220, 320, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, &g_sFontCmss18b, g_pcIPAddr, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the board's Ethernet MAC address.  This
// is required if using the Ethernet boot loader.
//
//*****************************************************************************
Canvas(g_sMACAddr, &g_sBackground, &g_sIPAddr, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 200, 320, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, &g_sFontCmss18b, g_pcMACAddr, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the current status.
//
//*****************************************************************************
Canvas(g_sStatus, &g_sBackground, &g_sMACAddr, 0,
       &g_sKitronix320x240x16_SSD2119,  60, 110, 200, 40,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, &g_sFontCmss22b, "Waiting for signal.", 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sStatus,
       &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 23),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, &g_sFontCm20, "boot-demo-eth", 0, 0);

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
// This is the handler for this SysTick interrupt.  We use this to provide the
// required timer call to the lwIP stack.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
}


//*****************************************************************************
//
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the main loop to exit and transfer control to the bootloader.
//
// IMPORTANT:
// Note that this callback is made in interrupt context and, since it is not
// permitted to transfer control to the boot loader from within an interrupt,
// we can't just call SoftwareUpdateBegin() here.
//
//*****************************************************************************
void SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// Perform the initialization steps required to start up the Ethernet controller
// and lwIP stack.
//
//*****************************************************************************
void
SetupForEthernet(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACAddr[6];

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    SysTickEnable();
    SysTickIntEnable();

    //
    // Configure the pins used to control the Ethernet LEDs.
    //  LED0        PF3   Output
    //  LED1        PF2   Output
    //
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Get the MAC address from the UART0 and UART1 registers in NV ram.
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
    // Format this address into the string used by the relevant widget.
    //
    usnprintf(g_pcMACAddr, SIZE_MAC_ADDR_BUFFER,
              "MAC: %02X-%02X-%02X-%02X-%02X-%02X",
              pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
              pucMACAddr[4], pucMACAddr[5]);

    //
    // Remember that we don't have an IP address yet.
    //
    usnprintf(g_pcIPAddr, SIZE_IP_ADDR_BUFFER, "IP: Not assigned");

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);
}

//*****************************************************************************
//
// Initialize UART0 and set the appropriate communication parameters.
//
//*****************************************************************************
void
SetupForUART(void)
{
    //
    // We need to make sure that UART0 and its associated GPIO port are
    // enabled before we pass control to the boot loader.  The serial boot
    // loader does not enable or configure these peripherals for us if we
    // enter it via its SVC vector.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Set GPIO A0 and A1 as UART.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115200, n, 8, 1
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_WLEN_8));

    //
    // Enable the UART operation.
    //
    UARTEnable(UART0_BASE);
}

//*****************************************************************************
//
// Enable the USB controller
//
//*****************************************************************************
void
SetupForUSB(void)
{
    //
    // We need to make sure that UART0 and its associated GPIO port are
    // enabled before we pass control to the boot loader.  The serial boot
    // loader does not enable or configure these peripherals for us if we
    // enter it via its SVC vector.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
}

//*****************************************************************************
//
// A simple application demonstrating use of the boot loader,
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIPAddr;

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the peripherals that each of the boot loader flavors
    // supports.  Although this example is only intended for use with the
    // Ethernet boot loader, we initialize the other two peripherals too just
    // in case it is used with the USB or serial boot loaders.
    //
    SetupForUART();
    SetupForEthernet();
    SetupForUSB();

    //
    // Enable Interrupts
    //
    IntMasterEnable();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Paint the widget tree to make sure they all appear on the display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // We don't have an IP address yet so clear the variable to tell us to
    // check until we are assigned one.
    //
    ulIPAddr = 0;

    //
    // Loop forever, processing widget messages.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // Do we have an IP address yet? If not, check to see if we've been
        // assigned one since the last time we checked.
        //
        if(ulIPAddr == 0)
        {
            //
            // What is our current IP address?
            //
            ulIPAddr = lwIPLocalIPAddrGet();

            //
            // If it's non zero, update the display.
            //
            if(ulIPAddr)
            {
                usprintf(g_pcIPAddr, "IP: %d.%d.%d.%d",
                         ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                         (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
                WidgetPaint((tWidget *)&g_sIPAddr);
            }
        }

        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();
    }

    //
    // If we drop out, the user has pressed the "Update Now" button so we
    // tidy up and transfer control to the boot loader.
    //

    //
    // Tell the user that we got their instruction.
    //
    CanvasTextSet(&g_sStatus, "Updating...");
    WidgetPaint((tWidget *)&g_sStatus);

    //
    // Process all remaining messages on the queue (including the paint message
    // we just posted).
    //
    WidgetMessageQueueProcess();

    //
    // Transfer control to the bootloader.
    //
    SoftwareUpdateBegin();

    //
    // The previous function never returns but we need to stick in a return
    // code here to keep the compiler from generating a warning.
    //
    return(0);
}
