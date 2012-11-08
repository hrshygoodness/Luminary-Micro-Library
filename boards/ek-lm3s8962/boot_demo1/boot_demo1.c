//*****************************************************************************
//
// boot_demo1.c - First boot loader example.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Boot Loader Demo 1 (boot_demo1)</h1>
//!
//! An example to demonstrate the use of the Serial or Ethernet boot loader.
//! After being started by the boot loader, the application will configure the
//! UART and Ethernet controller and branch back to the boot loader to await
//! the start of an update.  The UART will always be configured at 115,200
//! baud and does not require the use of auto-bauding.  The Ethernet controller
//! will be configured for basic operation and enabled.  Reprogramming using
//! the Ethernet boot loader will require both the MAC and IP Addresses.  The
//! MAC address will be displayed at the bottom of the screen.  Since a TCP/IP
//! stack is not being used in this demo application, an IP address will need
//! to be selected that will be on the same subnet as the PC that is being
//! used to program the flash, but is not in conflict with any IP addresses
//! already present on the network. 
//!
//! Both the boot loader and the application must be placed into flash.  Once
//! the boot loader is in flash, it can be used to program the application into
//! flash as well.  Then, the boot loader can be used to replace the
//! application with another.
//!
//! The boot_demo2 application can be used along with this application to
//! easily demonstrate that the boot loader is actually updating the on-chip
//! flash.
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
// Demonstrate the use of the boot loader.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACArray[8];
    static char pcBuf[40];

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable the UART and GPIO modules.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Enable the Ethernet module.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);

    //
    // Make the UART pins be peripheral controlled.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART0_BASE, 8000000, 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));

    //
    // Intialize the Ethernet Controller and disable all Ethernet Controller
    // interrupt sources.
    //
    EthernetIntDisable(ETH_BASE, (ETH_INT_PHY | ETH_INT_MDIO | ETH_INT_RXER |
                       ETH_INT_RXOF | ETH_INT_TX | ETH_INT_TXER | ETH_INT_RX));
    EthernetIntClear(ETH_BASE, EthernetIntStatus(ETH_BASE, false));
    EthernetInitExpClk(ETH_BASE, SysCtlClockGet());
    EthernetConfigSet(ETH_BASE, (ETH_CFG_TX_DPLXEN | ETH_CFG_TX_CRCEN |
                                 ETH_CFG_TX_PADEN));
    EthernetEnable(ETH_BASE);
    FlashUserGet(&ulUser0, &ulUser1);
    pucMACArray[0] = ((ulUser0 >>  0) & 0xff);
    pucMACArray[1] = ((ulUser0 >>  8) & 0xff);
    pucMACArray[2] = ((ulUser0 >> 16) & 0xff);
    pucMACArray[3] = ((ulUser1 >>  0) & 0xff);
    pucMACArray[4] = ((ulUser1 >>  8) & 0xff);
    pucMACArray[5] = ((ulUser1 >> 16) & 0xff);
    EthernetMACAddrSet(ETH_BASE, pucMACArray);

    //
    // Initialize the OLED display.
    //
    RIT128x96x4Init(1000000);

    //
    // Indicate what is happening.
    //
    RIT128x96x4StringDraw("Boot Loader Demo One", 4, 4, 15);
    RIT128x96x4StringDraw("The boot loader is", 10, 36, 15);
    RIT128x96x4StringDraw("now running.", 30, 44, 15);
    RIT128x96x4StringDraw("Update using ...", 16, 52, 15);
    RIT128x96x4StringDraw("UART0:115,200, 8-N-1", 6, 60, 15);
    usprintf(pcBuf, "ETH:%02X.%02X.%02X.%02X.%02X.%02X",
             pucMACArray[0], pucMACArray[1], pucMACArray[2],
             pucMACArray[3], pucMACArray[4], pucMACArray[5]);
    RIT128x96x4StringDraw(pcBuf, 3, 68, 15);

    //
    // Call the boot loader so that it will listen for an update on the UART.
    //
    (*((void (*)(void))(*(unsigned long *)0x2c)))();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
