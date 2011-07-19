//*****************************************************************************
//
// ext_demo_2.c - Example showing execution from external flash memory.  UART0
//                echoes received characters back to the sender under interrupt
//                control then jumps back to the boot loader when "swupd" is
//                entered.  This example is intended for use with the
//                boot_eth_ext boot loader image.
//
// Copyright (c) 2008-2011 Texas Instruments Incorporated.  All rights reserved.
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

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_nvic.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/flash.h"
#include "driverlib/ethernet.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>UART Echo running in external flash (ext_demo_2)</h1>
//!
//! This example application is equivalent to uart_echo but has been reworked
//! to run out of external flash attached via the Extended Peripheral Interface
//! (EPI).  It utilizes the UART to echo text.  The first UART (connected to
//! the FTDI virtual serial port on the evaluation board) will be configured in
//! 115,200 baud, 8-n-1 mode.  All characters received on the UART are
//! transmitted back to the UART and this continues until "swupd" followed by a
//! carriage return is entered, at which point the application transfers
//! control back to the boot loader to initiate a firmware update.
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
// Variables used to manage a buffer which stores the last EXIT_STRING_LENGTH
// characters received.  This is used to determine when someone enters the
// string used to transfer control to the boot loader.
//
//*****************************************************************************
#define EXIT_STRING_LENGTH 5
char *g_pcExitString = "swupd";
char g_pcLastChars[EXIT_STRING_LENGTH];
unsigned long g_ulLastCharIndex = 0;

//*****************************************************************************
//
// Flag used to indicate to the main loop that it should transfer control
// to the boot loader.
//
//*****************************************************************************
volatile tBoolean g_bExitNow;

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
// Determines whether the last characters entered match the exit string.
//
//*****************************************************************************
tBoolean
CheckForExitString(void)
{
    unsigned long ulLoop, ulIndex;

    //
    // She the buffer index representing the oldest character in the last
    // entered string.
    //
    ulIndex = g_ulLastCharIndex;

    //
    // Compare each character of the last string entered with the exit
    // string to see if they match.
    //
    for(ulLoop = 0; ulLoop < EXIT_STRING_LENGTH; ulLoop++)
    {
        //
        // If the current character in the "last entered" string doesn't
        // match the exit string, return immediately.
        //
        if(g_pcLastChars[ulIndex] != g_pcExitString[ulLoop])
        {
            return(false);
        }
        else
        {
            //
            // Move on to the next character in the last entered string
            // buffer, taking care of the wrap.
            //
            ulIndex++;
            if(ulIndex == EXIT_STRING_LENGTH)
            {
                ulIndex = 0;
            }
        }
    }

    //
    // If we drop out, the strings are the same.
    //
    return(true);
}

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    unsigned long ulStatus;
    long lChar;

    //
    // Get the interrrupt status.
    //
    ulStatus = ROM_UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    ROM_UARTIntClear(UART0_BASE, ulStatus);

    //
    // Loop while there are characters in the receive FIFO.
    //
    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //
        lChar = ROM_UARTCharGetNonBlocking(UART0_BASE);
        ROM_UARTCharPutNonBlocking(UART0_BASE, (unsigned char)lChar);

        //
        // Did we just receive a carriage return?
        //
        if(lChar == (long)'\r')
        {
            //
            // Yes - check to see if it was preceded by the "exit" string.
            //
            if(CheckForExitString())
            {
                g_bExitNow = true;
            }
            ROM_UARTCharPutNonBlocking(UART0_BASE, '\n');
        }
        else
        {
            //
            // Remember the character entered so that we can check for the
            // exit string later.
            //
            g_pcLastChars[g_ulLastCharIndex] = (char)lChar;
            g_ulLastCharIndex++;
            if(g_ulLastCharIndex == EXIT_STRING_LENGTH)
            {
                g_ulLastCharIndex = 0;
            }
        }
    }
}

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
void
UARTSend(const unsigned char *pucBuffer, unsigned long ulCount)
{
    //
    // Loop while there are more characters to send.
    //
    while(ulCount--)
    {
        //
        // Write the next character to the UART.
        //
        ROM_UARTCharPut(UART0_BASE, *pucBuffer++);
    }
}

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
    // Unlike applications running from internal flash, PinoutSet() here since
    // these settings have already been made in the boot loader.  Care must be
    // taken not to modify the pin configuration in any way which would affect
    // the ability of the CPU to read from the EPI address space since this
    // will cause the application code to become unavailable and will lead
    // very quickly to a crash.
    //
    // This example does not make use of the display driver or touchscreen but
    // any application running from external flash which does must set the
    // global variable g_eDaughterType to value DAUGHTER_SRAM_FLASH prior to
    // initializing either of these drivers to ensure that they operate
    // correctly.
    //

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
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    //
    // Enable the UART interrupt.
    //
    ROM_IntEnable(INT_UART0);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    //
    // Prompt for text to be entered.
    //
    UARTSend((unsigned char *)"\r\nUART Echo running from external flash\r\n",
             41);
    UARTSend((unsigned char *)"-------------------------------------\r\n\r\n",
             41);
    UARTSend((unsigned char *)"Type \"swupd<Enter>\" to transfer control to "
             "the boot loader.\r\n\r\n", 63);
    UARTSend((unsigned char *)"Enter text: ", 12);

    //
    // Loop until we are asked to transfer control to the boot loader. During
    // this time, the UART interrupt handler is echoing received data back to
    // the sender.
    //
    while(!g_bExitNow)
    {
        //
        // Hang around waiting for a signal to exit.
        //
    }

    //
    // If we drop out of the loop, the user has entered the magic exit signal
    // string so transfer control to the boot loader after telling them what
    // is happening.
    //
    UARTSend((unsigned char *)"\r\nTransfering to boot loader...\r\n\r\n", 35);
    while(UARTBusy(UART0_BASE))
    {
        //
        // Wait for our last message to be transmitted.
        //
    }

    //
    // Turn off interrupts and enter the boot loader via the SVC vector in
    // flash.  This function does not return.
    //
    JumpToBootLoader();

    //
    // Just to keep some compilers happy...
    //
    while(1)
    {
    }
}
