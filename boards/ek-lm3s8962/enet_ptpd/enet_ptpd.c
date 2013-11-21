//*****************************************************************************
//
// enet_lwip.c - Sample WebServer Application using lwIP.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/ptpdlib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "drivers/rit128x96x4.h"
#include "random.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet with PTP (enet_ptpd)</h1>
//!
//! This example application demonstrates the operation of the Stellaris
//! Ethernet controller using the lwIP TCP/IP Stack.  DHCP is used to obtain
//! an Ethernet address.  If DHCP times out without obtaining an address,
//! AUTOIP will be used to obtain a link-local address.  The address that is
//! selected will be shown on the OLED display.
//! 
//! A default set of pages will be served up by an internal file system and
//! the httpd server.
//!
//! The IEEE 1588 (PTP) software has been enabled in this code to synchronize
//! the internal clock to a network master clock source.
//!
//! Two methods of receive packet timestamping are implemented.  The default
//! mode uses the Stellaris hardware timestamp mechanism to capture Ethernet
//! packet reception time using timer 3B.  On parts which do not support
//! hardware timestamping or if the application is started up with the
//! ``Select'' button pressed, software time stamping is used.
//!
//! For additional details on lwIP, refer to the lwIP web page at:
//! http://savannah.nongnu.org/projects/lwip/
//!
//! For additional details on the PTPd software, refer to the PTPd web page at:
//! http://ptpd.sourceforge.net
//
//*****************************************************************************

//*****************************************************************************
//
// Define the system clock rate here.  One of the following must be defined to
// choose the system clock rate.
//
//*****************************************************************************
//#define SYSTEM_CLOCK_8MHZ
//#define SYSTEM_CLOCK_20MHZ
//#define SYSTEM_CLOCK_25MHZ
#define SYSTEM_CLOCK_50MHZ

//*****************************************************************************
//
// Clock and PWM dividers used depend on which system clock rate is chosen.
//
//*****************************************************************************
#if defined(SYSTEM_CLOCK_8MHZ)
#define SYSDIV      SYSCTL_SYSDIV_1
#define PWMDIV      SYSCTL_PWMDIV_1
#define CLKUSE      SYSCTL_USE_OSC
#define TICKNS      125

#elif defined(SYSTEM_CLOCK_20MHZ)
#define SYSDIV      SYSCTL_SYSDIV_10
#define PWMDIV      SYSCTL_PWMDIV_2
#define CLKUSE      SYSCTL_USE_PLL
#define TICKNS      50

#elif defined(SYSTEM_CLOCK_25MHZ)
#define SYSDIV      SYSCTL_SYSDIV_8
#define PWMDIV      SYSCTL_PWMDIV_2
#define CLKUSE      SYSCTL_USE_PLL
#define TICKNS      40

#elif defined(SYSTEM_CLOCK_50MHZ)
#define SYSDIV      SYSCTL_SYSDIV_4
#define PWMDIV      SYSCTL_PWMDIV_2
#define CLKUSE      SYSCTL_USE_PLL
#define TICKNS      20

#else
#error "System clock speed is not defined properly!"

#endif

//*****************************************************************************
//
// Select button GPIO definitions.  The GPIO defined here is assumed to be
// attached to a button which, when pressed during application initialization,
// signals that Ethernet packet timestamping hardware is not to be used.  If
// the button is not pressed, the hardware timestamp feature will be used if
// it is available on the target IC.
//
//*****************************************************************************
#define SEL_BTN_GPIO_PERIPHERAL SYSCTL_PERIPH_GPIOF
#define SEL_BTN_GPIO_BASE       GPIO_PORTF_BASE
#define SEL_BTN_GPIO_PIN        GPIO_PIN_1

//*****************************************************************************
//
// Pulse Per Second (PPS) Output Definitions
//
//*****************************************************************************
#define PPS_GPIO_PERIPHERAL     SYSCTL_PERIPH_GPIOB
#define PPS_GPIO_BASE           GPIO_PORTB_BASE
#define PPS_GPIO_PIN            GPIO_PIN_0

//*****************************************************************************
//
// The following group of labels define the priorities of each of the interrupt
// we use in this example.  SysTick must be high priority and capable of
// preempting other interrupts to minimize the effect of system loading on the
// timestamping mechanism.
//
// The application uses the default Priority Group setting of 0 which means
// that we have 8 possible preemptable interrupt levels available to us using
// the 3 bits of priority available on the Stellaris microcontrollers with
// values from 0xE0 (lowest priority) to 0x00 (highest priority).
//
//*****************************************************************************
#define SYSTICK_INT_PRIORITY  0x00
#define ETHERNET_INT_PRIORITY 0x80

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)
#define SYSTICKUS               (1000000 / SYSTICKHZ)
#define SYSTICKNS               (1000000000 / SYSTICKHZ)

//*****************************************************************************
//
// A set of flags used to track the state of the application.
//
//*****************************************************************************
static volatile unsigned long g_ulFlags;
#define FLAG_PPSOUT             0   // PPS Output is on.
#define FLAG_PPSOFF             1   // PPS Output should be turned off.
#define FLAG_PTPDINIT           2   // PTPd has been initialized.
#define FLAG_HWTIMESTAMP        3   // Using hardware ethernet timestamping.

//*****************************************************************************
//
// System Time - Internal representaion.
//
//*****************************************************************************
volatile unsigned long g_ulSystemTimeSeconds;
volatile unsigned long g_ulSystemTimeNanoSeconds;

//*****************************************************************************
//
// System Run Time - Ticks
//
//*****************************************************************************
volatile unsigned long g_ulSystemTimeTicks;

//*****************************************************************************
//
// These debug variables track the number of times the getTime function reckons
// it detected a SysTick wrap occurring during the time when it was reading the
// time.  We also record the second value of the timestamp when the wrap was
// detected in case we want to try to correlate this with any external
// measurements.
//
//*****************************************************************************
#ifdef DEBUG
unsigned long g_ulSysTickWrapDetect = 0;
unsigned long g_ulSysTickWrapTime = 0;
unsigned long g_ulGetTimeWrapCount = 0;
#endif

//*****************************************************************************
//
// Local data for clocks and timers.
//
//*****************************************************************************
static volatile unsigned long g_ulNewSystemTickReload = 0;
static volatile unsigned long g_ulSystemTickHigh = 0;
static volatile unsigned long g_ulSystemTickReload = 0;

//*****************************************************************************
//
// Statically allocated runtime options and parameters for PTPd.
//
//*****************************************************************************
static PtpClock g_sPTPClock;
static ForeignMasterRecord g_psForeignMasterRec[DEFUALT_MAX_FOREIGN_RECORDS];
static RunTimeOpts g_sRtOpts;

//*****************************************************************************
//
// External Application references.
//
//*****************************************************************************
extern void fs_init(void);
extern void fs_tick(unsigned long ulTickMS);

//*****************************************************************************
//
// Local function prototypes.
//
//*****************************************************************************
static void adjust_rx_timestamp(TimeInternal *psRxTime, unsigned long ulRxTime,
                                unsigned long ulNow);
static void ptpd_init(void);
static void ptpd_tick(void);

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
// Display Date and Time.
//
//*****************************************************************************
static char g_pucBuf[23];
const char *g_ppcDay[7] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
const char *g_ppcMonth[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void
DisplayDate(unsigned long ulSeconds, unsigned long ulRow)
{
    tTime sLocalTime;

    //
    // Convert the elapsed seconds (ulSeconds) into time structure.
    //
    ulocaltime(ulSeconds, &sLocalTime);

    //
    // Generate an appropriate date string for OLED display.
    //
    usprintf(g_pucBuf, "%3s %3s %2d, %4d", g_ppcDay[sLocalTime.ucWday],
             g_ppcMonth[sLocalTime.ucMon], sLocalTime.ucMday,
             sLocalTime.usYear);
    RIT128x96x4StringDraw(g_pucBuf, 12, ulRow, 15);
}

static void
DisplayTime(unsigned long ulSeconds, unsigned long ulRow)
{
    tTime sLocalTime;

    //
    // Convert the elapsed seconds (ulSeconds) into time structure.
    //
    ulocaltime(ulSeconds, &sLocalTime);

    //
    // Generate an appropriate date string for OLED display.
    //
    usprintf(g_pucBuf, "%02d:%02d:%02d (GMT)", sLocalTime.ucHour,
             sLocalTime.ucMin, sLocalTime.ucSec);
    RIT128x96x4StringDraw(g_pucBuf, 12, ulRow, 15);
}

//*****************************************************************************
//
// Display an lwIP type IP Address.
//
//*****************************************************************************
void
DisplayIPAddress(unsigned long ipaddr, unsigned long ulCol,
                 unsigned long ulRow)
{
    char pucBuf[17];
    unsigned char *pucTemp = (unsigned char *)&ipaddr;

    //
    // Convert the IP Address into a string.
    //
    usprintf(pucBuf, "%d.%d.%d.%d ", pucTemp[0], pucTemp[1], pucTemp[2],
             pucTemp[3]);

    //
    // Display the string.
    //
    RIT128x96x4StringDraw(pucBuf, ulCol, ulRow, 15);
}

//*****************************************************************************
//
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************
void
lwIPHostTimerHandler(void)
{
    static unsigned long ulLastIPAddress = 0;
    unsigned long ulIPAddress;

    ulIPAddress = lwIPLocalIPAddrGet();

    //
    // If IP Address has not yet been assigned, update the display accordingly
    //
    if(ulIPAddress == 0)
    {
        static int iColumn = 6;

        //
        // Update status bar on the display.
        //
        if(iColumn < 12)
        {
            RIT128x96x4StringDraw("< ", 0, 24, 15);
            RIT128x96x4StringDraw("*", iColumn, 24, 7);
        }
        else
        {
            RIT128x96x4StringDraw(" *", iColumn - 6, 24, 7);
        }

        iColumn++;
        if(iColumn > 114)
        {
            iColumn = 6;
            RIT128x96x4StringDraw(" >", 114, 24, 15);
        }
    }

    //
    // Check if IP address has changed, and display if it has.
    // 
    else if(ulLastIPAddress != ulIPAddress)
    {
        ulLastIPAddress = ulIPAddress;
        RIT128x96x4StringDraw("                       ", 0, 16, 15);
        RIT128x96x4StringDraw("                       ", 0, 24, 15);
        RIT128x96x4StringDraw("IP:   ", 0, 16, 15);
        RIT128x96x4StringDraw("MASK: ", 0, 24, 15);
        RIT128x96x4StringDraw("GW:   ", 0, 32, 15);
        DisplayIPAddress(ulIPAddress, 36, 16);
        ulIPAddress = lwIPLocalNetMaskGet();
        DisplayIPAddress(ulIPAddress, 36, 24);
        ulIPAddress = lwIPLocalGWAddrGet();
        DisplayIPAddress(ulIPAddress, 36, 32);
    }

    //
    // If IP address has been assigned, initialize the PTPD software (if
    // not already initialized).
    //
    if(ulIPAddress && !HWREGBITW(&g_ulFlags, FLAG_PTPDINIT))
    {
        ptpd_init();
        HWREGBITW(&g_ulFlags, FLAG_PTPDINIT) = 1;
    }

    //
    // If PTPD software has been initialized, run the ptpd tick.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_PTPDINIT))
    {
        ptpd_tick();
    }
}

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update internal time and set PPS output, if needed.
    //
    g_ulSystemTimeNanoSeconds += SYSTICKNS;
    if(g_ulSystemTimeNanoSeconds >= 1000000000)
    {
        GPIOPinWrite(PPS_GPIO_BASE, PPS_GPIO_PIN, PPS_GPIO_PIN);
        g_ulSystemTimeNanoSeconds -= 1000000000;
        g_ulSystemTimeSeconds += 1;
        HWREGBITW(&g_ulFlags, FLAG_PPSOUT) = 1;
    }

    //
    // Set a new System Tick Reload Value.
    //
    if(g_ulSystemTickReload != g_ulNewSystemTickReload)
    {
        g_ulSystemTickReload = g_ulNewSystemTickReload;

        g_ulSystemTimeNanoSeconds = ((g_ulSystemTimeNanoSeconds / SYSTICKNS) *
                                     SYSTICKNS);
    }

    //
    // For each tick, set the next reload value for fine tuning the clock.
    //
    if((g_ulSystemTimeTicks % TICKNS) < g_ulSystemTickHigh)
    {
        SysTickPeriodSet(g_ulSystemTickReload + 1);
    }
    else
    {
        SysTickPeriodSet(g_ulSystemTickReload);
    }

    //
    // Service the PTPd Timer.
    //
    timerTick(SYSTICKMS);

    //
    // Increment the run-time tick counter.
    //
    g_ulSystemTimeTicks++;

    //
    // Clear PPS output when needed and display time of day.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_PPSOFF))
    {
        //
        // Negate the PPS output.
        //
        GPIOPinWrite(PPS_GPIO_BASE, PPS_GPIO_PIN, 0);

        //
        // Indicate that we have negated the PPS output.
        //
        HWREGBITW(&g_ulFlags, FLAG_PPSOFF) = 0;

        //
        // Display Date and Time.
        //
        DisplayDate(g_ulSystemTimeSeconds, 48);
        DisplayTime(g_ulSystemTimeSeconds, 56);
    }

    //
    // Setup to disable the PPS output on the next pass.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_PPSOUT))
    {
        //
        // Setup to turn off the PPS output.
        //
        HWREGBITW(&g_ulFlags, FLAG_PPSOUT) = 0;
        HWREGBITW(&g_ulFlags, FLAG_PPSOFF) = 1;
    }

    //
    // Call the lwIP timer handler.
    //
    lwIPTimer(SYSTICKMS);
}

//*****************************************************************************
//
// Initialization code for PTPD software.
//
//*****************************************************************************
static void
ptpd_init(void)
{
    unsigned long ulTemp;

    //
    // Clear out all of the run time options and protocol stack options.
    //
    memset(&g_sRtOpts, 0, sizeof(g_sRtOpts));
    memset(&g_sPTPClock, 0, sizeof(g_sPTPClock));

    //
    // Initialize all PTPd run time options to a valid, default value.
    //
    g_sRtOpts.syncInterval = DEFUALT_SYNC_INTERVAL;
    memcpy(g_sRtOpts.subdomainName, DEFAULT_PTP_DOMAIN_NAME,
           PTP_SUBDOMAIN_NAME_LENGTH);
    memcpy(g_sRtOpts.clockIdentifier, IDENTIFIER_DFLT, PTP_CODE_STRING_LENGTH);
    g_sRtOpts.clockVariance = DEFAULT_CLOCK_VARIANCE;
    g_sRtOpts.clockStratum = DEFAULT_CLOCK_STRATUM;
    g_sRtOpts.clockPreferred = FALSE;
    g_sRtOpts.currentUtcOffset = DEFAULT_UTC_OFFSET;
    g_sRtOpts.epochNumber = 0;
    memcpy(g_sRtOpts.ifaceName, "LMI", strlen("LMI"));
    g_sRtOpts.noResetClock = DEFAULT_NO_RESET_CLOCK;
    g_sRtOpts.noAdjust = FALSE;
    g_sRtOpts.displayStats = FALSE;
    g_sRtOpts.csvStats = FALSE;
    g_sRtOpts.unicastAddress[0] = 0;
    g_sRtOpts.ap = DEFAULT_AP;
    g_sRtOpts.ai = DEFAULT_AI;
    g_sRtOpts.s = DEFAULT_DELAY_S;
    g_sRtOpts.inboundLatency.seconds = 0;
    g_sRtOpts.inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
    g_sRtOpts.outboundLatency.seconds = 0;
    g_sRtOpts.outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
    g_sRtOpts.max_foreign_records = DEFUALT_MAX_FOREIGN_RECORDS;
    g_sRtOpts.slaveOnly = TRUE;
    g_sRtOpts.probe = FALSE;
    g_sRtOpts.probe_management_key = 0;
    g_sRtOpts.probe_record_key = 0;
    g_sRtOpts.halfEpoch = FALSE;

    //
    // Initialize the PTP Clock Fields.
    //
    g_sPTPClock.foreign = &g_psForeignMasterRec[0];

    //
    // Configure port "uuid" parameters.
    //
    g_sPTPClock.port_communication_technology = PTP_ETHER;
    EthernetMACAddrGet(ETH_BASE, (unsigned char *)g_sPTPClock.port_uuid_field);

    //
    // Enable Ethernet Multicast Reception (required for PTPd operation).
    // Note:  This must follow lwIP/Ethernet initialization.
    //
    ulTemp = EthernetConfigGet(ETH_BASE);
    ulTemp |= ETH_CFG_RX_AMULEN;
    if(HWREGBITW(&g_ulFlags, FLAG_HWTIMESTAMP))
    {
        ulTemp |= ETH_CFG_TS_TSEN;
    }
    EthernetConfigSet(ETH_BASE, ulTemp);

    //
    // Run the protocol engine for the first time to initialize the state
    // machines.
    //
    protocol_first(&g_sRtOpts, &g_sPTPClock);
}

//*****************************************************************************
//
// Run the protocol engine loop/poll.
//
//*****************************************************************************
static void
ptpd_tick(void)
{
    //
    // Run the protocol engine for each pass through the main process loop.
    //
    protocol_loop(&g_sRtOpts, &g_sPTPClock);
}

//*****************************************************************************
//
// Adjust the supplied timestamp to account for interrupt latency.
//
//*****************************************************************************
static void
adjust_rx_timestamp(TimeInternal *psRxTime, unsigned long ulRxTime,
                    unsigned long ulNow)
{
    unsigned long ulCorrection;

    //
    // Time parameters ulNow and ulRxTime are assumed to have originated from
    // a 16 bit down counter operating over its full range.
    //

    //
    // Determine the number of cycles between the receive timestamp and the
    // point that it was read.
    //
    if(ulNow < ulRxTime)
    {
        //
        // The timer didn't wrap between the timestamp and now.
        //
        ulCorrection = ulRxTime - ulNow;
    }
    else
    {
        //
        // The timer wrapped between the timestamp and now
        //
        ulCorrection = ulRxTime + (0x10000 - ulNow);
    }

    //
    // Convert the correction from cycles to nanoseconds.
    //
    ulCorrection *= TICKNS;

    //
    // Subtract the correction from the supplied timestamp value.
    //
    if(psRxTime->nanoseconds >= ulCorrection)
    {
        //
        // In this case, we need only adjust the nanoseconds value since there
        // is no borrow from the seconds required.
        //
        psRxTime->nanoseconds -= ulCorrection;
    }
    else
    {
        //
        // Here, the adjustment affects both the seconds and nanoseconds
        // fields.  The correction cannot be more than 1 second (16 bit counter
        // maximum offset and minimum cycle time of 125nS gives a maximum
        // correction of 8.192mS) so we don't need to perform any modulo
        // calculations here.
        //
        psRxTime->seconds--;
        psRxTime->nanoseconds += (1000000000 - ulCorrection);
    }
}

//*****************************************************************************
//
// This example demonstrates the use of the Ethernet Controller.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACArray[8];
    volatile unsigned long ulDelay;

    //
    // Set the system clocking as defined above in SYSDIV and CLKUSE.
    //
    SysCtlClockSet(SYSDIV | CLKUSE | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);

    //
    // Set up for debug output to the UART.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);

    //
    // Initialize the OLED display.
    //
    RIT128x96x4Init(1000000);
    RIT128x96x4StringDraw("Ethernet with PTPd", 12, 0, 15);

    //
    // Enable and Reset the Ethernet Controller.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);
    IntPrioritySet(INT_ETH, ETHERNET_INT_PRIORITY);

    //
    // Enable Port F for Ethernet LEDs.
    //  LED0        Bit 3   Output
    //  LED1        Bit 2   Output
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Configure the defined PPS GPIO for output.
    //
    SysCtlPeripheralEnable(PPS_GPIO_PERIPHERAL);
    GPIOPinTypeGPIOOutput(PPS_GPIO_BASE, PPS_GPIO_PIN);
    GPIOPinWrite(PPS_GPIO_BASE, PPS_GPIO_PIN, 0);

    //
    // We test the state of the SELECT button on the board (assuming it is
    // present) and, if pressed, disable hardware timestamping of Ethernet
    // packets. This allows us to test the PTP clock response in both cases
    // without the need to recompile.
    //
    SysCtlPeripheralEnable(SEL_BTN_GPIO_PERIPHERAL);
    GPIOPinTypeGPIOInput(SEL_BTN_GPIO_BASE, SEL_BTN_GPIO_PIN);
    GPIOPadConfigSet(SEL_BTN_GPIO_BASE, SEL_BTN_GPIO_PIN, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    //
    // Wait a while before reading the GPIOs since we just modified the
    // pad configuration.
    //
    for(ulDelay = 0; ulDelay < 10000; ulDelay++)
    {
    }

    //
    // If the SELECT button is pressed, the GPIO is pulled low so we read 0
    // from GPIOPinRead.  If we detect this, disable hardware Ethernet
    // timestamping if available.
    //
    if(GPIOPinRead(SEL_BTN_GPIO_BASE, SEL_BTN_GPIO_PIN))
    {
        //
        // If we have been asked to use hardware timestamps, first make sure
        // that the target IC actually supports the feature.
        //
        if(!SysCtlPeripheralPresent(SYSCTL_PERIPH_IEEE1588))
        {
            //
            // Oops - the target IC does not support hardware timestamping of
            // Ethernet packets.  Display a warning that will remain visible
            // while DHCP is getting us an IP address then fall back to the
            // software timestamp software approach.
            //
            HWREGBITW(&g_ulFlags, FLAG_HWTIMESTAMP) = 0;
            RIT128x96x4StringDraw("No H/W IEEE1588!", 0, 32, 15);
        }

        else
        {
            //
            // Enable timer 3 to capture the timestamps of the incoming
            // packets.
            //
            HWREGBITW(&g_ulFlags, FLAG_HWTIMESTAMP) = 1;
            SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
            SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER3);

            //
            // Configure Timer 3 as 2 16 bit counters.  Timer B is used to
            // capture the time of the last Ethernet RX interrupt and we leave
            // Timer A free running to allow us to determine how much time has
            // passed between the interrupt firing and the ISR actually
            // reading the packet.  Had we been interested in transmit
            // timestamps, time 3A would have been used for this and a second
            // timer block would be needed to provide the free-running
            // reference count.
            //
            TimerConfigure(TIMER3_BASE, (TIMER_CFG_SPLIT_PAIR |
                                         TIMER_CFG_A_PERIODIC |
                                         TIMER_CFG_B_CAP_TIME));
            TimerPrescaleSet(TIMER3_BASE, TIMER_BOTH, 0);
            TimerLoadSet(TIMER3_BASE, TIMER_BOTH, 0xFFFF);
            TimerControlEvent(TIMER3_BASE, TIMER_B, TIMER_EVENT_POS_EDGE);

            //
            // Start the timers running.
            //
            TimerEnable(TIMER3_BASE, TIMER_BOTH);
        }
    }
    else
    {
        //
        // The user was pressing the select button as the application started
        // so we disable hardware timestamping.
        //
        HWREGBITW(&g_ulFlags, FLAG_HWTIMESTAMP) = 0;
        RIT128x96x4StringDraw("H/W timestamps off", 0, 32, 15);
    }

    //
    // Configure SysTick for a periodic interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / SYSTICKHZ);
    g_ulSystemTickReload = SysTickPeriodGet();
    g_ulNewSystemTickReload = g_ulSystemTickReload;
    SysTickEnable();
    SysTickIntEnable();

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Configure the hardware MAC address for Ethernet Controller filtering of
    // incoming packets.  The MAC address will be stored in the non-volatile
    // USER0 and USER1 registers.  These registers can be read using the
    // FlashUserGet function, as illustrated below.
    //
    FlashUserGet(&ulUser0, &ulUser1);
    if((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
        //
        // We should never get here.  This is an error if the MAC address has
        // not been programmed into the device.  Exit the program.
        //
        RIT128x96x4StringDraw("MAC Address", 0, 16, 15);
        RIT128x96x4StringDraw("Not Programmed!", 0, 24, 15);
        while(1)
        {
        }
    }

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    //
    pucMACArray[0] = ((ulUser0 >>  0) & 0xff);
    pucMACArray[1] = ((ulUser0 >>  8) & 0xff);
    pucMACArray[2] = ((ulUser0 >> 16) & 0xff);
    pucMACArray[3] = ((ulUser1 >>  0) & 0xff);
    pucMACArray[4] = ((ulUser1 >>  8) & 0xff);
    pucMACArray[5] = ((ulUser1 >> 16) & 0xff);

    //
    // Initialze the lwIP library, using DHCP.
    //
    lwIPInit(pucMACArray, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACArray);
    LocatorAppTitleSet("EK-LM3S8962 enet_ptpd");

    //
    // Initialize the file system.
    //
    fs_init();

    //
    // Initialize the Random Number Generator.
    //
    RandomSeed();

    //
    // Indicate that DHCP has started.
    //
    RIT128x96x4StringDraw("Waiting for IP", 0, 16, 15);
    RIT128x96x4StringDraw("<                   > ", 0, 24, 15);

    //
    // Initialize a sample httpd server.
    //
    httpd_init();

    //
    // Loop forever.  All the work is done in interrupt handlers.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// The following set of functions are LMI Board/Chip Specific implementations
// of functions required by PTPd software.  Prototypes are defined in ptpd.h,
// or one of its included files.
//
//*****************************************************************************

//*****************************************************************************
//
// Display Statistics.  For now, do nothing, but this could be used to either
// update a web page, send data to the serial port, or to the OLED display.
//
// Refer to the ptpd software "src/dep/sys.c" for example code.
//
//*****************************************************************************
void
displayStats(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
}

//*****************************************************************************
//
// This function returns the local time (in PTPd internal time format).  This
// time is maintained by the SysTick interrupt.
//
// Note: It is very important to ensure that we detect cases where the system
// tick rolls over during this function. If we don't do this, there is a race
// condition that will cause the reported time to be off by a second or so
// once in a blue moon. This, in turn, causes large perturbations in the
// 1588 time controller resulting in large deltas for many seconds as the
// controller tries to compensate.
//
//*****************************************************************************
void
getTime(TimeInternal *time)
{
    unsigned long ulTime1;
    unsigned long ulTime2;
    unsigned long ulSeconds;
    unsigned long ulPeriod;
    unsigned long ulNanoseconds;

    //
    // We read the SysTick value twice, sandwiching taking snapshots of
    // the seconds, nanoseconds and period values. If the second SysTick read
    // gives us a higher number than the first read, we know that it wrapped
    // somewhere between the two reads so our seconds and nanoseconds
    // snapshots are suspect. If this occurs, we go round again. Note that
    // it is not sufficient merely to read the values with interrupts disabled
    // since the SysTick counter keeps counting regardless of whether or not
    // the wrap interrupt has been serviced.
    //
    do
    {
        ulTime1 = SysTickValueGet();
        ulSeconds = g_ulSystemTimeSeconds;
        ulNanoseconds = g_ulSystemTimeNanoSeconds;
        ulPeriod = SysTickPeriodGet();
        ulTime2 = SysTickValueGet();

#ifdef DEBUG
        //
        // In debug builds, keep track of the number of times this function was
        // called just as the SysTick wrapped.
        //
        if(ulTime2 > ulTime1)
        {
            g_ulSysTickWrapDetect++;
            g_ulSysTickWrapTime = ulSeconds;
        }
#endif
    }
    while(ulTime2 > ulTime1);

    //
    // Fill in the seconds field from the snapshot we just took.
    //
    time->seconds = ulSeconds;

    //
    // Fill in the nanoseconds field from the snapshots.
    //
    time->nanoseconds = (ulNanoseconds + (ulPeriod - ulTime2) * TICKNS);

    //
    // Adjust for any case where we accumulate more than 1 second's worth of
    // nanoseconds.
    //
    if(time->nanoseconds >= 1000000000)
    {
#ifdef DEBUG
        g_ulGetTimeWrapCount++;
#endif
        time->seconds++;
        time->nanoseconds -= 1000000000;
    }
}

//*****************************************************************************
//
// This function will set the local time (provided in PTPd internal time
// format).  This time is maintained by the SysTick interrupt.
//
//*****************************************************************************
void
setTime(TimeInternal *time)
{
    sys_prot_t sProt;

    //
    // Update the System Tick Handler time values from the given PTPd time
    // (fine-tuning is handled in the System Tick handler). We need to update
    // these variables with interrupts disabled since the update must be
    // atomic.
    //
#ifdef DEBUG
    UARTprintf("Setting time %d.%09d\n", time->seconds, time->nanoseconds);
#endif
    sProt = sys_arch_protect();
    g_ulSystemTimeSeconds = time->seconds;
    g_ulSystemTimeNanoSeconds = time->nanoseconds;
    sys_arch_unprotect(sProt);
}

//*****************************************************************************
//
// Get the RX Timestamp. This is called from the lwIP low_level_input function
// when configured to include PTPd support.
//
//*****************************************************************************
void
lwIPHostGetTime(u32_t *time_s, u32_t *time_ns)
{
    unsigned long ulNow;
    unsigned long ulTimestamp;
    TimeInternal psRxTime;
    
    //
    // Get the current IEEE1588 time.
    //
    getTime(&psRxTime);

    //
    // If we are using the hardware timestamp mechanism, get the timestamp and
    // use it to adjust the packet timestamp accordingly.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_HWTIMESTAMP))
    {
        //
        // Read the (now frozen) timer value and the still-running timer.
        //
        ulTimestamp = TimerValueGet(TIMER3_BASE, TIMER_B);
        ulNow = TimerValueGet(TIMER3_BASE, TIMER_A);

        //
        // Adjust the current time with the difference between now and the
        // actual timestamp.
        //
        adjust_rx_timestamp(&psRxTime, ulTimestamp, ulNow);
    }

    //
    // Fill in the appropriate return values.
    //
    *time_s = psRxTime.seconds;
    *time_ns = psRxTime.nanoseconds;
}

//*****************************************************************************
//
// This function returns a random number, using the functions in random.c.
//
//*****************************************************************************
UInteger16
getRand(UInteger32 *seed)
{
    unsigned long ulTemp;
    UInteger16 uiTemp;

    //
    // Re-seed the random number generator.
    //
    RandomAddEntropy(*seed);
    RandomSeed();

    //
    // Get a random number and return a 16-bit, truncated version.
    //
    ulTemp = RandomNumber();
    uiTemp = (UInteger16)(ulTemp & 0xFFFF);
    return(uiTemp);
}

//*****************************************************************************
//
// Based on the value (adj) provided by the PTPd Clock Servo routine, this
// function will adjust the SysTick periodic interval to allow fine-tuning of
// the PTP Clock.
//
//*****************************************************************************
Boolean
adjFreq(Integer32 adj)
{
    unsigned long ulTemp;

    //
    // Check for max/min value of adjustment.
    //
    if(adj > ADJ_MAX)
    {
        adj = ADJ_MAX;
    }
    else if(adj < -ADJ_MAX)
    {
        adj = -ADJ_MAX;
    }

    //
    // Convert input to nanoseconds / systick.
    //
    adj = adj / SYSTICKHZ;

    //
    // Get the nominal tick reload value and convert to nano seconds.
    //
    ulTemp = (SysCtlClockGet() / SYSTICKHZ) * TICKNS;

    //
    // Factor in the adjustment.
    //
    ulTemp -= adj;

    //
    // Get a modulo count of nanoseconds for fine tuning.
    //
    g_ulSystemTickHigh = ulTemp % TICKNS;

    //
    // Set the reload value.
    //
    g_ulNewSystemTickReload = ulTemp / TICKNS;

    //
    // Return.
    //
    return(TRUE);
}
