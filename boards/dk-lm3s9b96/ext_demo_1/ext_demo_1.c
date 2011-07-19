//*****************************************************************************
//
// ext_demo_1.c - Simple example showing execution from external flash memory.
//                This example is intended for use with the boot_eth_ext boot
//                loader image.
//
// Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_nvic.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/flash.h"
#include "driverlib/ethernet.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>External flash execution demonstration (ext_demo_1)</h1>
//!
//! This example application illustrates execution out of external flash
//! attached via the lm3s9b96 Extended Peripheral Interface (EPI). It utilizes
//! the UART to display a simple message before immediately transfering control
//! back to the boot loader in preparation for the download of a new
//! application image.  The first UART (connected to the FTDI virtual serial
//! port on the development kit board) will be configured in 115200 baud,
//! 8-n-1 mode.
//!
//! This application is configured specifically for execution from external
//! flash and relies upon the external flash version of the Ethernet boot
//! loader, boot_eth_ext, being present in internal flash.  It will not run
//! with any other boot loader version.  The boot_eth_ext boot loader configures
//! the system clock and EPI to allow access to the external flash address
//! space then relocates the application exception vectors into internal SRAM
//! before branching to the main application in daughter-board flash.
//!
//! Note that execution from external flash should be avoided if at all
//! possible due to significantly lower performance than achievable from
//! internal flash. Using an 8-bit wide interface to flash as found on the
//! Flash/SRAM/LCD daughter board and remembering that an external memory
//! access via EPI takes 8 or 9 system clock cycles, a program running from
//! off-chip memory will typically run at approximately 5% of the speed of the
//! same program in internal flash.
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
// Passes control to the bootloader and initiates a remote software update.
//
// This function passes control to the bootloader and initiates an update of
// the main application firmware image via UART0, Ethernet or USB depending
// upon the specific boot loader binary in use.
//
// \return Never returns.
//
//*****************************************************************************
void
JumpToBootLoader(void)
{
    //
    // Disable all processor interrupts.  Instead of disabling them
    // one at a time, a direct write to NVIC is done to disable all
    // peripheral interrupts.
    //
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;

    //
    // Return control to the boot loader.  This is a call to the SVC
    // handler in the boot loader.
    //
    (*((void (*)(void))(*(unsigned long *)0x2c)))();
}

//*****************************************************************************
//
// Main application entry function.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACArray[6];

    //
    // Unlike applications running from internal flash, do not call PinoutSet()
    // here since these settings have already been made in the boot loader.
    // Care must be taken not to modify the pin configuration in any way which
    // would affect the ability of the CPU to read from the EPI address space
    // since this will cause the application code to become unavailable and
    // will lead very quickly to a crash.
    //
    // This example does not make use of the display driver or touchscreen but
    // any application running from external flash which does must set the
    // global variable g_eDaughterType to value DAUGHTER_SRAM_FLASH prior to
    // initializing either of these drivers to ensure that they operate
    // correctly.

    //
    // Enable the (non-GPIO) peripherals used by this example.  PinoutSet()
    // already enabled GPIO Port A in the boot loader.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Intialize the Ethernet Controller and disable all Ethernet Controller
    // interrupt sources.  This ensures that we are in the correct state to
    // reenter the (Ethernet) boot loader once the main loop exits.  This app
    // doesn't used Ethernet, however.
    //
    ROM_EthernetIntDisable(ETH_BASE, (ETH_INT_PHY | ETH_INT_MDIO |
                           ETH_INT_RXER | ETH_INT_RXOF | ETH_INT_TX |
                           ETH_INT_TXER | ETH_INT_RX));
    ROM_EthernetIntClear(ETH_BASE, EthernetIntStatus(ETH_BASE, false));
    ROM_EthernetInitExpClk(ETH_BASE, SysCtlClockGet());
    ROM_EthernetConfigSet(ETH_BASE, (ETH_CFG_TX_DPLXEN | ETH_CFG_TX_CRCEN |
                          ETH_CFG_TX_PADEN));
    ROM_EthernetEnable(ETH_BASE);
    ROM_FlashUserGet(&ulUser0, &ulUser1);
    pucMACArray[0] = ((ulUser0 >>  0) & 0xff);
    pucMACArray[1] = ((ulUser0 >>  8) & 0xff);
    pucMACArray[2] = ((ulUser0 >> 16) & 0xff);
    pucMACArray[3] = ((ulUser1 >>  0) & 0xff);
    pucMACArray[4] = ((ulUser1 >>  8) & 0xff);
    pucMACArray[5] = ((ulUser1 >> 16) & 0xff);
    ROM_EthernetMACAddrSet(ETH_BASE, pucMACArray);

    //
    // Initialize the UARTStdio module to allow us to output via UART0.
    //
    UARTStdioInit(0);

    //
    // Prompt for text to be entered.
    //
    UARTprintf("\n\nExternal Flash Execution Demonstration\n");
    UARTprintf(  "--------------------------------------\n\n");
    UARTprintf("Congratulations! This application is running from external\n");
    UARTprintf("flash memory.\n\n");
    UARTprintf("Control is now being transferred back to the boot loader\n");
    UARTprintf("in internal flash memory\n\n");

    //
    // Wait for our last transmission to leave the UART.
    //
    while(UARTBusy(UART0_BASE))
    {
        //
        // Wait for our last message to be transmitted.
        //
    }

    //
    // Enter the boot loader via the SVC vector in internal flash.  This
    // function does not return.
    //
    JumpToBootLoader();

    //
    // Just to keep some compilers happy...
    //
    while(1)
    {
    }
}
