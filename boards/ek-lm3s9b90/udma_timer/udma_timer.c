//*****************************************************************************
//
// udma_timer.c - uDMA with timer example.
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
// This is part of revision 8555 of the EK-LM3S9B90 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/udma.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>uDMA with Timer (udma_timer)</h1>
//!
//! This example application demonstrates the use of the timer to trigger
//! periodic DMA transfers.  A timer is configured for periodic operation.
//! The uDMA controller channel is configured to perform a transfer when
//! requested from the timer.  For the purposes of this demonstration, the
//! data that is transferred is the value of a separate free-running timer.
//! However in a real application the data transferred could be to/from memory
//! or a peripheral.
//!
//! After a small number of transfers are performed, the captured timer values
//! are compared to make sure the expected duration elapsed between transfers.
//! The results are printed out.
//!
//! UART0, connected to the FTDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// The timeout value to use for the periodic timer.  Even though a 32-bit
// timer is used, this value must be 16-bit due to a chip errata that causes
// a DMA request whenever the timer rolls past the 16 bit boundary, resulting
// in incorrect timing of the DMA requests.  As long as the timeout value is
// a 16-bit number, the timing works correctly.
//
//*****************************************************************************
#define TIMEOUT_VAL 60000

//*****************************************************************************
//
// An array to hold captured timer events.
//
//*****************************************************************************
#define MAX_TIMER_EVENTS 20
static unsigned long g_ulTimerBuf[MAX_TIMER_EVENTS];

//*****************************************************************************
//
// Counters to count occurrences of the interrupt handler and uDMA errors
//
//*****************************************************************************
static unsigned long g_ulTimer0AIntCount = 0;
static unsigned long g_uluDMAErrCount = 0;

//*****************************************************************************
//
// A flag to indicate when the DMA transfers are done
//
//*****************************************************************************
volatile unsigned int g_bDoneFlag = 0;

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
unsigned char ucControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(ucControlTable, 1024)
unsigned char ucControlTable[1024];
#else
unsigned char ucControlTable[1024] __attribute__ ((aligned(1024)));
#endif

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
// The interrupt handler for uDMA errors.  This interrupt will occur if the
// uDMA encounters a bus error while trying to perform a transfer.  This
// handler just increments a counter if an error occurs.
//
//*****************************************************************************
void
uDMAErrorHandler(void)
{
    unsigned long ulStatus;

    //
    // Check for uDMA error bit
    //
    ulStatus = ROM_uDMAErrorStatusGet();

    //
    // If there is a uDMA error, then clear the error and increment
    // the error counter.
    //
    if(ulStatus)
    {
        ROM_uDMAErrorStatusClear();
        g_uluDMAErrCount++;
    }
}

//*****************************************************************************
//
// The interrupt handler for the periodic timer interrupt.  When the uDMA
// channel is used, interrupts from the periodic timer are used as DMA
// requests, and this interrupt handler is invoked only at the end of all of
// the DMA transfers.
//
//*****************************************************************************
void
Timer0IntHandler(void)
{
    unsigned long ulStatus;

    //
    // Clear the timer interrupt.
    //
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Read the uDMA channel status to verify it is done
    //
    ulStatus = uDMAChannelModeGet(UDMA_CHANNEL_TMR0A);
    if(ulStatus == UDMA_MODE_STOP)
    {
        //
        // Disable the periodic timer and set the done flag
        //
        ROM_TimerDisable(TIMER0_BASE, TIMER_A);
        g_bDoneFlag = 1;
    }

    //
    // Increment a counter to indicate the number of times this handler
    // was invoked
    //
    g_ulTimer0AIntCount++;
}

//*****************************************************************************
//
// This example application demonstrates the use of a periodic timer to
// request DMA transfers.
//
// Timer0 is used as the periodic timer that requests DMA transfers.
// Timer1 is a free running counter that is used as the source data for
// DMA transfers.  The captured counter values from Timer1 are copied by
// uDMA into a buffer.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIdx;
    unsigned long ulThisTimerVal;
    unsigned long ulPrevTimerVal;
    unsigned long ulTimerElapsed;
    unsigned long ulTimerErr;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the UART and write status.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
    UARTprintf("\033[2JuDMA periodic timer example\n\n");

    //
    // Enable the timers used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    //
    // Enable the uDMA peripheral
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);

    //
    // Enable the uDMA controller error interrupt.  This interrupt will occur
    // if there is a bus error during a transfer.
    //
    ROM_IntEnable(INT_UDMAERR);

    //
    // Enable the uDMA controller.
    //
    ROM_uDMAEnable();

    //
    // Point at the control table to use for channel control structures.
    //
    ROM_uDMAControlBaseSet(ucControlTable);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Configure one of the timers as free running 32-bit counter.  Its
    // value will be used as a time reference.
    //
    ROM_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, ~0);
    ROM_TimerEnable(TIMER1_BASE, TIMER_A);

    //
    // Configure the 32-bit periodic timer.
    //
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, TIMEOUT_VAL - 1);

    //
    // Enable the timer master interrupt.  The timer interrupt will actually
    // be generated by the uDMA controller when the timer channel transfer is
    // complete.  The interrupts on the timer (TimerIntEnable) do not need
    // to be configured.
    //
    ROM_IntEnable(INT_TIMER0A);

    //
    // Put the attributes in a known state for the uDMA Timer0A channel.  These
    // should already be disabled by default.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_TMR0A,
                                    UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

    //
    // Set up the DMA channel for Timer 0A.  Set it up to transfer single
    // 32-bit words at a time.  The source is non-incrementing, the
    // destination is incrementing.
    //
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_TMR0A | UDMA_PRI_SELECT,
                              UDMA_SIZE_32 |
                              UDMA_SRC_INC_NONE | UDMA_DST_INC_32 |
                              UDMA_ARB_1);

    //
    // Set up the transfer for Timer 0A DMA channel.  Basic mode is used,
    // which means that one transfer will occur per timer request (timeout).
    // The amount transferred per timeout is determined by the arbitration
    // size (see function above).  The source will be the value of free running
    // Timer1, and the destination is a memory buffer.  Thus, the value of the
    // free running Timer1 will be stored in a buffer every time the periodic
    // Timer0 times out.
    //
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_TMR0A | UDMA_PRI_SELECT,
                               UDMA_MODE_BASIC,
                               (void *)(TIMER1_BASE + TIMER_O_TAV),
                               g_ulTimerBuf, MAX_TIMER_EVENTS);

    //
    // Enable the timers and the DMA channel.
    //
    UARTprintf("Using timeout value of %u\n", TIMEOUT_VAL);
    UARTprintf("Starting timer and uDMA\n");
    TimerEnable(TIMER0_BASE, TIMER_A);
    uDMAChannelEnable(UDMA_CHANNEL_TMR0A);

    //
    // Wait for the transfer to complete.
    //
    UARTprintf("Waiting for transfers to complete\n");
    while(!g_bDoneFlag)
    {
    }

    //
    // Check for the expected number of occurrences of the interrupt handler,
    // and that there are no DMA errors
    //
    if(g_uluDMAErrCount != 0)
    {
        UARTprintf("\nuDMA errors were detected!!!\n\n");
    }
    if(g_ulTimer0AIntCount != 1)
    {
        UARTprintf("\nUnexpected number of interrupts occurrred (%d)!!!\n\n",
                   g_ulTimer0AIntCount);
    }

    //
    // Display the timer values that were transferred using timer triggered
    // uDMA.  Compare the difference between stored values to the timer
    // period and make sure they match.  This verifies that the periodic
    // DMA transfers were occuring with the correct timing.
    //
    UARTprintf("\n       Captured\n");
    UARTprintf("Event    Value    Difference  Status\n");
    UARTprintf("----- ----------  ----------  ------\n");
    for(ulIdx = 1; ulIdx < MAX_TIMER_EVENTS; ulIdx++)
    {
        //
        // Compute the difference between adjacent captured values, and then
        // compare that to the expected timeout period.
        //
        ulThisTimerVal = g_ulTimerBuf[ulIdx];
        ulPrevTimerVal = g_ulTimerBuf[ulIdx - 1];
        ulTimerElapsed = ulThisTimerVal > ulPrevTimerVal ?
                         ulThisTimerVal - ulPrevTimerVal :
                         ulPrevTimerVal - ulThisTimerVal;
        ulTimerErr = ulTimerElapsed > TIMEOUT_VAL ?
                     ulTimerElapsed - TIMEOUT_VAL :
                     TIMEOUT_VAL - ulTimerElapsed;

        //
        // Print the captured value and the difference from the previous
        //
        UARTprintf(" %2u   0x%08X  %8u   ", ulIdx, ulThisTimerVal,
                   ulTimerElapsed);

        //
        // Print error status based on the deviation from expected difference
        // between samples (calculated above).  Allow for a difference of up
        // to 1 cycle.  Any more than that is considered an error.
        //
        if(ulTimerErr >  1)
        {
            UARTprintf(" ERROR\n");
        }
        else
        {
            UARTprintf(" OK\n");
        }
    }

    //
    // End of application
    //
    while(1)
    {
    }
}
