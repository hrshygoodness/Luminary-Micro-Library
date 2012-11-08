//*****************************************************************************
//
// boot_demo_eth.c - Ethernet boot loader example application.
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
// This is part of revision 9453 of the EK-LM3S6965 Firmware Package.
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
#include "drivers/rit128x96x4.h"

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
//! The boot_demo1 and boot_demo2 applications do not make use of the Ethernet
//! magic packet and can be used along with this application to easily
//! demonstrate that the boot loader is actually updating the on-chip flash.
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
// A simple application demonstrating use of the boot loader,
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIPAddr;
    unsigned long ulUser0, ulUser1;
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
    // Initialize the OLED display.
    //
    RIT128x96x4Init(1000000);
    RIT128x96x4StringDraw("boot_demo_eth", 25, 0, 15);

    //
    // Enable and Reset the Ethernet Controller.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

    //
    // Enable Port F for Ethernet LEDs.
    //  LED0        Bit 3   Output
    //  LED1        Bit 2   Output
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
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
              "MAC %02X-%02X-%02X-%02X-%02X-%02X",
              pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
              pucMACAddr[4], pucMACAddr[5]);

    //
    // Remember that we don't have an IP address yet.
    //
    ulIPAddr = 0;
    usnprintf(g_pcIPAddr, SIZE_IP_ADDR_BUFFER, "IP  Not assigned");

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Enable Interrupts
    //
    IntMasterEnable();

    //
    // Indicate what is happening.
    //
    RIT128x96x4StringDraw("Boot Loader Ethernet", 4, 20, 15);
    RIT128x96x4StringDraw("Trigger Demo", 28, 30, 15);
    RIT128x96x4StringDraw(g_pcMACAddr, 0, 50, 15);
    RIT128x96x4StringDraw(g_pcIPAddr, 0, 60, 15);
    RIT128x96x4StringDraw("Waiting... ", 31, 80, 15);

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
                usprintf(g_pcIPAddr, "IP  %d.%d.%d.%d",
                         ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                         (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
                RIT128x96x4StringDraw(g_pcIPAddr, 0, 60, 15);
            }
        }
    }

    //
    // If we drop out, the user has pressed the "Update Now" button so we
    // tidy up and transfer control to the boot loader.
    //

    //
    // Tell the user that we got their instruction.
    //
    RIT128x96x4StringDraw("Updating...", 31, 80, 15);

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
