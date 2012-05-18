//*****************************************************************************
//
// qs-adventure.c - Quickstart game that plays the Colossal Cave Adventure game
//                  by William Crowther.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM3S9D90 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/uartstdio.h"
#include "bget/bget.h"
#include "zip/ztypes.h"
#include "common.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Quick Start Game (qs-adventure)</h1>
//!
//! This game consists of a Z-machine interpreter running a Z-code version of
//! the classic Colossal Cave Adventure game originally created by William
//! Crowther.  The Ethernet interface provides a telnet server and the USB
//! interface provides a CDC serial port.  Either interface can be used to play
//! the game, though not at the same time.
//!
//! The LED on the evaluation board will be turned on when the game is being
//! played; further connections will be refused since only one instance of the
//! game can be played at a time.  The push button on the evaluation board will
//! restart the game from the beginning; this is equivalent to typing
//! ``restart'' followed by ``yes'' in the game itself.
//!
//! The virtual COM port provided by the ICDI board (which is connected to
//! UART0 on the evaluation board) provides a simple status display.  The most
//! important piece of information provided is the IP address of the Ethernet
//! interface, which is selected using AutoIP (which uses DHCP if it is present
//! and a random link-local address otherwise).
//!
//! The game is played by typing simple English sentences in order to direct
//! the actions of the protagonist, with abbreviations being allowed.  For
//! example, ``go west'', ``west'', and ``w'' all perform the same action.
//!
//! Three display modes are available; ``verbose'' (which displays the full
//! description every time a location is visited), ``brief'' (which displays
//! the full description the first time a location is visited and only the name
//! every other time), and ``superbrief'' (which only displays the name).  The
//! default display mode is ``brief'', and ``look'' can be used to get the full
//! description at any time (regardless of the display mode).
//!
//! For a history of the Colossal Cave Adventure game, its creation of the
//! ``interactive fiction'' gaming genre, and game hints, an Internet search
//! will turn up numerous web sites.  A good starting place is
//! http://en.wikipedia.org/wiki/Colossal_Cave_Adventure .
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)

//*****************************************************************************
//
// The error message provided when an attempt is made to play the game when it
// is already being played over a different interface.
//
//*****************************************************************************
const unsigned char g_pucErrorMessage[53] =
    "The game is already being played...try again later!\r\n";

//*****************************************************************************
//
// The number of seconds that have passed.  The starting value corresponds to
// April 18, 2009 at 1pm EDT.
//
//*****************************************************************************
unsigned long g_ulTime = 1240074000;

//*****************************************************************************
//
// The number of SysTick interrupts that have occurred in the last second.
//
//*****************************************************************************
static unsigned long g_ulTicks = 0;

//*****************************************************************************
//
// The memory pool that is managed by bget.
//
//*****************************************************************************
static unsigned char g_pucPool[32768];

//*****************************************************************************
//
// The interface being used to play the game.
//
//*****************************************************************************
volatile unsigned long g_ulGameIF = GAME_IF_NONE;

//*****************************************************************************
//
// This value is set if the game is being restarted by pressing the button on
// the evaluation board.
//
//*****************************************************************************
unsigned long g_ulRestart = 0;

//*****************************************************************************
//
// The state of the push button when the previous SysTick interrupt occurred.
//
//*****************************************************************************
static unsigned long g_ulPreviousButton = 1;

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
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulButton;

    //
    // Read the current state of the push button.
    //
    ulButton = ROM_GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_4);

    //
    // See if the button is pressed and it was pressed during the previous
    // SysTick interrupt.
    //
    if((ulButton == 0) && (g_ulPreviousButton != 0))
    {
        //
        // Halt the Z-machine interpreter.
        //
        halt = 1;

        //
        // Indicate that the Z-machine should be restarted.
        //
        g_ulRestart = 1;
    }

    //
    // Save the current button state for the next SysTick interrupt.
    //
    g_ulPreviousButton = ulButton;

    //
    // Perform any periodic processing required by the Ethernet interface.
    //
    EnetIFTick(SYSTICKMS);

    //
    // Increment the count of SysTicks.
    //
    g_ulTicks++;

    //
    // See if a second has passed.
    //
    if(g_ulTicks == SYSTICKHZ)
    {
        //
        // Reset the count of SysTicks.
        //
        g_ulTicks = 0;

        //
        // Increment the count of seconds.
        //
        g_ulTime++;
    }
}

//*****************************************************************************
//
// Play the Colossal Cave Adventure game using either an Ethernet telnet
// connection or a USB CDC serial connection.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run from the PLL at 80 MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Enable the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);

    //
    // Print out a greeting.
    //
    UARTprintf("\033[2JColossal Cave Adventure\n");
    UARTprintf("-----------------------\n");
    UARTprintf("Connect to either the USB virtual COM port or\n");
    UARTprintf("telnet into the Ethernet port in order to play.\n\n");

    //
    // Enable the GPIO that is used for the on-board push button.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_4);
    ROM_GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);

    //
    // Enable the GPIO that is used for the on-board LED.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0);
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0);

    //
    // Configure SysTick for a periodic interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKHZ);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Initialize the Ethernet and USB interfaces.
    //
    EnetIFInit();
    USBIFInit();

    //
    // Provide a working area to the memory allocator.
    //
    bpool(g_pucPool, sizeof(g_pucPool));

    //
    // Configure the Z-machine interpreter.
    //
    configure(V1, V5);

    //
    // Initialize the Z-machine screen interface.
    //
    initialize_screen();

    //
    // Pre-fill the Z-machine interpreter's cache.
    //
    load_cache();

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Wait until a connection has been made via either the Ethernet or USB
        // interfaces.
        //
        while(g_ulGameIF == GAME_IF_NONE)
        {
        }

        //
        // Turn on the LED to indicate that the game is in progress.
        //
        ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0);

        //
        // Loop until the game has been exited.  Repeat this loop if the game
        // exited because the restart button was pressed.
        //
        do
        {
            //
            // Take the Z-machine interpreter out of the halt state.
            //
            halt = 0;

            //
            // Set the restart flag to zero.
            //
            g_ulRestart = 0;

            //
            // Restart the Z-machine interpreter.
            //
            restart();

            //
            // Run the Z-machine interpreter until it halts.
            //
            interpret();
        }
        while(g_ulRestart);

        //
        // Turn off the LED to indicate that the game has finished.
        //
        ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0);

        //
        // Close down the Ethernet connection if it was being used to play the
        // game.
        //
        if(g_ulGameIF == GAME_IF_ENET)
        {
            EnetIFClose();
        }

        //
        // Forget the interface used to play the game so that the selection
        // process will be repeated.
        //
        g_ulGameIF = GAME_IF_NONE;
    }
}
