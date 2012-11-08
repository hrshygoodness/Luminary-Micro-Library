//*****************************************************************************
//
// enet_lwip.c - Sample WebServer Application using lwIP.
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
// This is part of revision 9453 of the EK-LM3S9D92 Firmware Package.
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
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/ptpdlib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "random.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet with PTP (enet_ptpd)</h1>
//!
//! This example application demonstrates the operation of the Stellaris
//! Ethernet controller using the lwIP TCP/IP Stack.  DHCP is used to obtain
//! an Ethernet address.  If DHCP times out without obtaining an address,
//! AutoIP will be used to obtain a link-local address.  The address that is
//! selected will be output to the UART.
//! 
//! A default set of pages will be served up by an internal file system and
//! the httpd server.
//!
//! The IEEE 1588 (PTP) software has been enabled in this code to synchronize
//! the internal clock to a network master clock source.
//!
//! UART0, connected to the FTDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
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
#define SYSDIV                  SYSCTL_SYSDIV_2
#define CLKUSE                  SYSCTL_USE_OSC
#define TICKNS                  125

#elif defined(SYSTEM_CLOCK_20MHZ)
#define SYSDIV                  SYSCTL_SYSDIV_10
#define CLKUSE                  SYSCTL_USE_PLL
#define TICKNS                  50

#elif defined(SYSTEM_CLOCK_25MHZ)
#define SYSDIV                  SYSCTL_SYSDIV_8
#define CLKUSE                  SYSCTL_USE_PLL
#define TICKNS                  40

#elif defined(SYSTEM_CLOCK_50MHZ)
#define SYSDIV                  SYSCTL_SYSDIV_4
#define CLKUSE                  SYSCTL_USE_PLL
#define TICKNS                  20

#else
#error "System clock speed is not defined properly!"

#endif

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
// The following defines set the priorities of each of the interrupt used in
// this example.  SysTick must be high priority and capable of preempting other
// interrupts to minimize the effect of system loading on the timestamping
// mechanism.
//
// The application uses the default Priority Group setting of 0 which means
// that there are 8 possible preemptable interrupt levels available using the 3
// bits of priority available on the Stellaris microcontrollers with values
// from 0xE0 (lowest priority) to 0x00 (highest priority).
//
//*****************************************************************************
#define SYSTICK_INT_PRIORITY    0x00
#define ETHERNET_INT_PRIORITY   0x40

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
#define FLAG_PPSOUT             0           // PPS Output is on.
#define FLAG_PPSOFF             1           // PPS Output should be turned off.
#define FLAG_PTPDINIT           2           // PTPd has been initialized.

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
// These debug variables track the number of times the getTime function detects
// a SysTick wrap occurring during the time when it was reading the time.  The
// second value of the timestamp is saved when the wrap was detected to assist
// in trying to correlate this with any external measurements.
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
// Local function prototypes.
//
//*****************************************************************************
static void ptpd_init(void);
static void ptpd_tick(void);

//*****************************************************************************
//
// A twirling line used to indicate that DHCP/AutoIP address acquisition is in
// progress.
//
//*****************************************************************************
static char g_pcTwirl[4] = { '\\', '|', '/', '-' };

//*****************************************************************************
//
// The index into the twirling line array of the next line orientation to be
// printed.
//
//*****************************************************************************
static unsigned long g_ulTwirlPos = 0;

//*****************************************************************************
//
// The most recently assigned IP address.  This is used to detect when the IP
// address has changed (due to DHCP/AutoIP) so that the new address can be
// printed.
//
//*****************************************************************************
static unsigned long g_ulLastIPAddr = 0;

//*****************************************************************************
//
// A mapping from day of the week numbers to the name of the day.
//
//*****************************************************************************
const char *g_ppcDay[7] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

//*****************************************************************************
//
// A mapping from month numbers to the name of the month.
//
//*****************************************************************************
const char *g_ppcMonth[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

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
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************
void
lwIPHostTimerHandler(void)
{
#ifdef ewarm
    volatile unsigned long ulIPAddress;
    unsigned long ulTemp;
#else
    unsigned long ulIPAddress, ulTemp;
#endif

    //
    // Get the local IP address.
    //
    ulIPAddress = lwIPLocalIPAddrGet();

    //
    // If IP Address has not yet been assigned, update the display accordingly
    //
    if(ulIPAddress == 0)
    {
        //
        // Draw a spinning line to indicate that the IP address is being
        // discoverd.
        //
        UARTprintf("\b%c", g_pcTwirl[g_ulTwirlPos]);

        //
        // Update the index into the twirl.
        //
        g_ulTwirlPos = (g_ulTwirlPos + 1) & 3;
    }

    //
    // Check if IP address has changed, and display if it has.
    // 
    else if(g_ulLastIPAddr != ulIPAddress)
    {
        //
        // Display the new IP address.
        //
        ulTemp = ulIPAddress;
        UARTprintf("\rIP: %d.%d.%d.%d       \n", ulTemp & 0xff,
                   (ulTemp >> 8) & 0xff, (ulTemp >> 16) & 0xff,
                   (ulTemp >> 24) & 0xff);

        //
        // Save the new IP address.
        //
        g_ulLastIPAddr = ulIPAddress;

        //
        // Display the new network mask.
        //
        ulTemp = lwIPLocalNetMaskGet();
        UARTprintf("Netmask: %d.%d.%d.%d\n", ulTemp & 0xff,
                   (ulTemp >> 8) & 0xff, (ulTemp >> 16) & 0xff,
                   (ulTemp >> 24) & 0xff);

        //
        // Display the new gateway address.
        //
        ulTemp = lwIPLocalGWAddrGet();
        UARTprintf("Gateway: %d.%d.%d.%d\n", ulTemp & 0xff,
                   (ulTemp >> 8) & 0xff, (ulTemp >> 16) & 0xff,
                   (ulTemp >> 24) & 0xff);
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
    unsigned long ulTemp;
    tTime sLocalTime;

    //
    // Update internal time and set PPS output, if needed.
    //
    g_ulSystemTimeNanoSeconds += SYSTICKNS;
    if(g_ulSystemTimeNanoSeconds >= 1000000000)
    {
        ROM_GPIOPinWrite(PPS_GPIO_BASE, PPS_GPIO_PIN, PPS_GPIO_PIN);
        g_ulSystemTimeNanoSeconds -= 1000000000;
        g_ulSystemTimeSeconds += 1;
        HWREGBITW(&g_ulFlags, FLAG_PPSOUT) = 1;
    }

    //
    // Set a new System Tick Reload Value.
    //
    ulTemp = g_ulSystemTickReload;
    if(ulTemp != g_ulNewSystemTickReload)
    {
        g_ulSystemTickReload = g_ulNewSystemTickReload;

        g_ulSystemTimeNanoSeconds = ((g_ulSystemTimeNanoSeconds / SYSTICKNS) *
                                     SYSTICKNS);
    }

    //
    // For each tick, set the next reload value for fine tuning the clock.
    //
    ulTemp = g_ulSystemTimeTicks;
    if((ulTemp % TICKNS) < g_ulSystemTickHigh)
    {
        ROM_SysTickPeriodSet(g_ulSystemTickReload + 1);
    }
    else
    {
        ROM_SysTickPeriodSet(g_ulSystemTickReload);
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
        ROM_GPIOPinWrite(PPS_GPIO_BASE, PPS_GPIO_PIN, 0);

        //
        // Indicate that the PPS output has been negated.
        //
        HWREGBITW(&g_ulFlags, FLAG_PPSOFF) = 0;

        //
        // Only print the date and time if PTPd has been started (in other
        // words, if an IP address has been obtained).
        //
        if(HWREGBITW(&g_ulFlags, FLAG_PTPDINIT))
        {
            //
            // Convert the elapsed seconds (ulSeconds) into time structure.
            //
            ulocaltime(g_ulSystemTimeSeconds, &sLocalTime);

            //
            // Print out the date and time.
            //
            UARTprintf("\r%3s %3s %2d, %4d %02d:%02d:%02d (GMT)",
                       g_ppcDay[sLocalTime.ucWday],
                       g_ppcMonth[sLocalTime.ucMon], sLocalTime.ucMday,
                       sLocalTime.usYear, sLocalTime.ucHour, sLocalTime.ucMin,
                       sLocalTime.ucSec);
        }
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
    g_sRtOpts.clockVariance = (UInteger32)DEFAULT_CLOCK_VARIANCE;
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
    ROM_EthernetMACAddrGet(ETH_BASE,
                           (unsigned char *)g_sPTPClock.port_uuid_field);

    //
    // Enable Ethernet Multicast Reception (required for PTPd operation).
    // Note:  This must follow lwIP/Ethernet initialization.
    //
    ulTemp = ROM_EthernetConfigGet(ETH_BASE);
    ulTemp |= ETH_CFG_RX_AMULEN;
    ROM_EthernetConfigSet(ETH_BASE, ulTemp);

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
// This example demonstrates the use of the Ethernet Controller.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACArray[8];

    //
    // Set the system clocking as defined above in SYSDIV and CLKUSE.
    //
    ROM_SysCtlClockSet(SYSDIV | CLKUSE | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    //
    // Initialize the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
    UARTprintf("\033[2JEthernet with PTPd\n");

    //
    // Enable and Reset the Ethernet Controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);
    ROM_IntPrioritySet(INT_ETH, ETHERNET_INT_PRIORITY);

    //
    // Enable Port F for Ethernet LEDs.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinConfigure(GPIO_PF2_LED1);
    GPIOPinConfigure(GPIO_PF3_LED0);
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Configure the defined PPS GPIO for output.
    //
    ROM_SysCtlPeripheralEnable(PPS_GPIO_PERIPHERAL);
    ROM_GPIOPinTypeGPIOOutput(PPS_GPIO_BASE, PPS_GPIO_PIN);
    ROM_GPIOPinWrite(PPS_GPIO_BASE, PPS_GPIO_PIN, 0);

    //
    // Configure SysTick for a periodic interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKHZ);
    g_ulSystemTickReload = ROM_SysTickPeriodGet();
    g_ulNewSystemTickReload = g_ulSystemTickReload;
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Read the MAC address from the user registers.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);
    if((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
        //
        // We should never get here.  This is an error if the MAC address has
        // not been programmed into the device.  Exit the program.
        //
        UARTprintf("MAC Address Not Programmed!\n");
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
    LocatorAppTitleSet("EK-LM3S9D92 enet_ptpd");

    //
    // Initialize the Random Number Generator.
    //
    RandomSeed();

    //
    // Indicate that DHCP has started.
    //
    UARTprintf("Waiting for IP... ");

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
// tick rolls over during this function.  If we don't do this, there is a race
// condition that will cause the reported time to be off by a second or so
// once in a blue moon.  This, in turn, causes large perturbations in the
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
    // the seconds, nanoseconds and period values.  If the second SysTick read
    // gives us a higher number than the first read, we know that it wrapped
    // somewhere between the two reads so our seconds and nanoseconds
    // snapshots are suspect.  If this occurs, we go round again.  Note that
    // it is not sufficient merely to read the values with interrupts disabled
    // since the SysTick counter keeps counting regardless of whether or not
    // the wrap interrupt has been serviced.
    //
    do
    {
        ulTime1 = ROM_SysTickValueGet();
        ulSeconds = g_ulSystemTimeSeconds;
        ulNanoseconds = g_ulSystemTimeNanoSeconds;
        ulPeriod = ROM_SysTickPeriodGet();
        ulTime2 = ROM_SysTickValueGet();

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
    // (fine-tuning is handled in the System Tick handler).  We need to update
    // these variables with interrupts disabled since the update must be
    // atomic.
    //
    sProt = sys_arch_protect();
    g_ulSystemTimeSeconds = time->seconds;
    g_ulSystemTimeNanoSeconds = time->nanoseconds;
    sys_arch_unprotect(sProt);
}

//*****************************************************************************
//
// Get the RX Timestamp.  This is called from the lwIP low_level_input function
// when configured to include PTPd support.
//
//*****************************************************************************
void
lwIPHostGetTime(u32_t *time_s, u32_t *time_ns)
{
    TimeInternal psRxTime;
    
    //
    // Get the current IEEE1588 time.
    //
    getTime(&psRxTime);

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
    ulTemp = (ROM_SysCtlClockGet() / SYSTICKHZ) * TICKNS;

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
