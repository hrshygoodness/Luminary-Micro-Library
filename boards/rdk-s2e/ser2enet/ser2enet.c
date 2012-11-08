//*****************************************************************************
//
// ser2enet.c - Serial to Ethernet converter.
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
// This is part of revision 9453 of the RDK-S2E Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/ringbuf.h"
#include "utils/swupdate.h"
#ifdef DEBUG_UART
#include "utils/uartstdio.h"
#endif
#include "httpserver_raw/httpd.h"
#include "config.h"
#include "serial.h"
#include "telnet.h"
#include "upnp.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Serial To Ethernet Module (ser2enet)</h1>
//!
//! The Serial to Ethernet Converter provides a means of accessing the UART on
//! a Stellaris device via a network connection.  The UART can be connected to
//! the UART on a non-networked device, providing the ability to access the
//! device via a network.  This can be useful to overcome the cable length
//! limitations of a UART connection (in fact, the cable can become thousands
//! of miles long) and to provide networking capability to existing devices
//! without modifying the device's operation.
//!
//! The converter can be configured to use a static IP configuration or to use
//! DHCP to obtain its IP configuration.  Since the converter is providing a
//! telnet server, the effective use of DHCP requires a reservation in the DHCP
//! server so that the converter gets the same IP address each time it is
//! connected to the network.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup main_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The number of times per second that the SysTick timer generates a processor
//! interrupt.
//
//*****************************************************************************
#define SYSTICKHZ               100

//*****************************************************************************
//
//! The number of milliseconds between SysTick generated processor interrupts.
//
//*****************************************************************************
#define SYSTICKMS               (1000 / SYSTICKHZ)

//*****************************************************************************
//
//! Keeps track of elapsed time in milliseconds.
//
//*****************************************************************************
unsigned long g_ulSystemTimeMS = 0;

//*****************************************************************************
//
//! A flag indicating that a firmware update request has been received.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
//! A flag indicating the current link status.
//
//*****************************************************************************
volatile tBoolean g_bLinkStatusUp = false;

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
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the bootloader to be entered the next time the user enters a
// command on the console.
//
//*****************************************************************************
static void
SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
//! Handles the Ethernet interrupt hooks for the client software.
//!
//! This function will run any handlers that are required to run in the
//! Ethernet interrupt context.  All the actual TCP/IP processing occurs within
//! this function (since lwIP is not re-entrant).
//!
//! \return None.
//
//*****************************************************************************
void
lwIPHostTimerHandler(void)
{
    tBoolean bLinkStatusUp;

    //
    // Get the current link status and see if it has changed.
    //
    bLinkStatusUp = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3) ? false : true;
    if(bLinkStatusUp != g_bLinkStatusUp)
    {
        //
        // Save the new link status.
        //
        g_bLinkStatusUp = bLinkStatusUp;

        //
        // Notify the Telnet module that the link status has changed.
        //
        TelnetNotifyLinkStatus(g_bLinkStatusUp);
    }

    //
    // Service the telnet module.
    //
    TelnetHandler();

    //
    // Service the Universal Plug and Plan (UPnP) module.
    //
    UPnPHandler(g_ulSystemTimeMS);
}

//*****************************************************************************
//
//! Handles the SysTick interrupt.
//!
//! This function is called when the SysTick timer expires.  It increments the
//! lwIP timers and sets a flag indicating that the timeout functions need to
//! be called if necessary.
//!
//! \return None.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Increment a local system time.
    //
    g_ulSystemTimeMS += SYSTICKMS;

    //
    // Call the lwIP timer handler.
    //
    lwIPTimer(SYSTICKMS);
}

//*****************************************************************************
//
//! Handles the main application loop.
//!
//! This function initializes and configures the device and the software, then
//! performs the main loop.
//!
//! \return Never returns.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACAddr[8];
    unsigned long ulLoop;

    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Set the processor to run at 50 MHz, allowing UART operation at up to
    // 3.125 MHz.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable the peripherals used by the application.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);

    //
    // Enable the peripherals that should continue to run when the processor
    // is sleeping.
    //
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ETH);

    //
    // Enable peripheral clock gating.  Note that this is required in order to
    // measure the the processor usage.
    //
    SysCtlPeripheralClockGating(true);

    //
    // Set the priorities of the interrupts used by the application.
    //
    IntPrioritySet(INT_UART0, 0x00);
    IntPrioritySet(INT_UART1, 0x00);
    IntPrioritySet(INT_ETH, 0x20);
    IntPrioritySet(FAULT_SYSTICK, 0x40);

    //
    // Configure SysTick for a periodic interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / SYSTICKHZ);
    SysTickEnable();
    SysTickIntEnable();

#ifdef DEBUG_UART
    //
    // Initialize the UART stdio module so that we can use UARTprintf for
    // debug purposes.  Note that this will corrupt any telnet datastream also
    // carried on this UART so use this option with care and only during debug.
    //
    UARTStdioInit(DEBUG_UART);
#endif

    //
    // Enable Port F for Ethernet LEDs.
    //  LED0        Bit 3   Output
    //  LED1        Bit 2   Output
    //
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Set the link status based on the LED0 signal (which defaults to link
    // status in the PHY).
    //
    g_bLinkStatusUp = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3) ? false : true;

    //
    // Initialize the configuration parameter module.
    //
    ConfigInit();

    //
    // Get the MAC address from the USER0 and USER1 registers in NV memory.
    //
    FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV memory into a MAC address
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
    lwIPInit(pucMACAddr, g_sParameters.ulStaticIP, g_sParameters.ulSubnetMask,
             g_sParameters.ulGatewayIP, ((g_sParameters.ucFlags &
             CONFIG_FLAG_STATICIP) ? IPADDR_USE_STATIC : IPADDR_USE_DHCP));

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet((char *)g_sParameters.ucModName);

    //
    // Initialize the serial port module.
    //
    SerialInit();

    //
    // Initialize the telnet module.
    //
    TelnetInit();

    //
    // Initialize the UPnP session.
    //
    UPnPInit();

    //
    // Initialize the HTTPD web server.
    //
    httpd_init();

    //
    // Configure SSI and CGI processing for our configuration web forms.
    //
    ConfigWebInit();

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Wait for an IP address to be assigned to the board before we try to
    // initiate any connections.
    //
    while(!lwIPLocalIPAddrGet())
    {
        //
        // Do nothing until we get our IP address assigned.
        //
        SysCtlSleep();
    }

    //
    // Initialize the telnet session(s).
    //
    for(ulLoop = 0; ulLoop < MAX_S2E_PORTS; ulLoop++)
    {
        //
        // Are we to operate as a telnet server?
        //
        if((g_sParameters.sPort[ulLoop].ucFlags & PORT_FLAG_TELNET_MODE) ==
           PORT_TELNET_SERVER)
        {
            //
            // Yes - start listening on the required port.
            //
            TelnetListen(g_sParameters.sPort[ulLoop].usTelnetLocalPort,
                         ulLoop);
        }
        else
        {
            //
            // No - we are a client so initiate a connection to the desired
            // IP address using the configured ports.
            //
            TelnetOpen(g_sParameters.sPort[ulLoop].ulTelnetIPAddr,
                       g_sParameters.sPort[ulLoop].usTelnetRemotePort,
                       g_sParameters.sPort[ulLoop].usTelnetLocalPort, ulLoop);
        }
    }

    //
    // Main Application Loop (for systems with no RTOS).  Run every SYSTICK.
    //
    while(true)
    {
        //
        // Wait for an event to occur.
        //
        SysCtlSleep();

        //
        // Check for an IP update request.
        //
        if(g_cUpdateRequired)
        {
            //
            // Delay 2 seconds or so to allow the response page to get back
            // to the browser before we initiate the IP address change.
            //
            SysCtlDelay((SysCtlClockGet() / 3) * 2);

            //
            // Are we updating only the IP address?
            //
            if(g_cUpdateRequired & UPDATE_IP_ADDR)
            {
                //
                // Actually update the IP address.
                //
                g_cUpdateRequired &= ~UPDATE_IP_ADDR;
                ConfigUpdateIPAddress();
            }

            //
            // Are we updating all parameters (including the IP address?)
            //
            if(g_cUpdateRequired & UPDATE_ALL)
            {
                //
                // Update everything.
                //
                g_cUpdateRequired &= ~UPDATE_ALL;
                ConfigUpdateAllParameters(true);
            }
        }

        //
        // Check for bootloader request.
        //
        if((g_bStartBootloader == true) || (g_bFirmwareUpdate == true))
        {
            //
            // Delay a couple of seconds to let any pending web server
            // transmission to complete.  Each loop within SysCtlDelay is
            // 3 instructions long so this delays approximately 2 seconds.
            //
            SysCtlDelay((SysCtlClockGet() / 3) * 2);

            //
            // Initiate the Software Update Process in the Ethernet
            // Bootloader.
            //
            SoftwareUpdateBegin();

            //
            // Should never get here, but stall just in case.
            //
            while(1)
            {
            }
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
