//*****************************************************************************
//
// udma_timer_ccp.c - uDMA with timer edge capture example.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S9D92 Firmware Package.
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
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/udma.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>uDMA with Timer Edge Capture (udma_timer_ccp)</h1>
//!
//! This example application demonstrates the use of the timer edge capture
//! mode to trigger DMA transfers whenever there is an edge capture event.
//! A timer is configured for edge capture mode using CCP1 on pin PD7.  The
//! uDMA controller is configured to transfer the captured edge value to a
//! buffer until a certain number of edges have been captured.  A PWM output
//! is configured to generate a square wave on GPIO port bit PD0 to use as
//! an input source for the CCP1 pin.  In order to run the example, the pins
//! PD0 and PD7 must be jumpered together.
//!
//! After a certain number of edges have been captured, the DMA transfer will
//! be complete.  The example program will print out the captured values.
//!
//! UART0, connected to the FTDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// The period of the PWM output signal which is used to trigger the CCP0 input.
// This is the number of CPU cycles between rising edges on the pin.
//
//*****************************************************************************
#define TIMEOUT_VAL 5000

//*****************************************************************************
//
// An array to hold captured timer events.
//
//*****************************************************************************
#define MAX_TIMER_EVENTS 20
static unsigned short g_usTimerBuf[MAX_TIMER_EVENTS];

//*****************************************************************************
//
// Counters to count occurrences of the interrupt handler and uDMA errors
//
//*****************************************************************************
static unsigned long g_ulTimer0BIntCount = 0;
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
// The interrupt handler for the Timer0B edge capture timer interrupt.  When
// the uDMA channel is used, interrupts from the capture timer are used as DMA
// requests, and this interrupt handler is invoked only at the end of all of
// the DMA transfers.
//
//*****************************************************************************
void
Timer0BIntHandler(void)
{
    unsigned long ulStatus;

    //
    // Clear the timer interrupt.
    //
    ROM_TimerIntClear(TIMER0_BASE, TIMER_CAPB_EVENT);

    //
    // Read the uDMA channel status to verify it is done
    //
    ulStatus = uDMAChannelModeGet(UDMA_CHANNEL_TMR0B);
    if(ulStatus == UDMA_MODE_STOP)
    {
        //
        // Disable the capture timer and set the done flag
        //
        ROM_TimerDisable(TIMER0_BASE, TIMER_B);
        g_bDoneFlag = 1;
    }

    //
    // Increment a counter to indicate the number of times this handler
    // was invoked
    //
    g_ulTimer0BIntCount++;
}

//*****************************************************************************
//
// Set up the PWM0 output to be used as a signal source for the CCP1 input
// pin.  This example application uses PWM0 as the signal source in order
// to demonstrate the CCP usage and also to provide a predictable timing
// source that will produce known values for the capture timer.  In a real
// application some external signal would be used as the signal source for
// the CCP input.
//
//*****************************************************************************
static void
SetupSignalSource(void)
{
    //
    // Enable the GPIO port used for PWM0 output.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Enable the PWM peripheral and configure the GPIO pin
    // used for PWM0.  GPIO pin D0 is used for PWM0 output.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    GPIOPinConfigure(GPIO_PD0_PWM0);
    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);

    //
    // Configure the PWM0 output to generate a 50% square wave with a
    // period of 5000 processor cycles.
    //
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TIMEOUT_VAL);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, TIMEOUT_VAL / 2);
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, 1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}

//*****************************************************************************
//
// This example application demonstrates the use of an edge capture timer to
// request DMA transfers.
//
// Timer0B is used as the capture timer that requests DMA transfers whenever
// a rising edge occurs on CCP1.  PWM0 is used as a source of edges and must
// be jumpered to CCP1 in order for this application to work.
//
// When the application runs, each edge causes the edge capture timer to
// trigger a DMA transfer.  The uDMA controller will then transfer the captured
// timer value into a buffer.

// After a certain number of edges are captured, the application prints out
// the results and compares the elapsed time between edges to the expected
// value.
//
// Note that the "B" timer is used because on some devices the "A" timer does
// not work correctly with the uDMA controller.  Refer to the chip errata for
// details.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIdx;
    unsigned short usTimerElapsed;
    unsigned short usTimerErr;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the UART and write a status message.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
    UARTprintf("\033[2JuDMA edge capture timer example\n\n");
    UARTprintf("This example requires that PD0 and PD7 be jumpered together"
               "\n\n");

    //
    // Create a signal source that can be used as an input for the CCP1 pin.
    //
    SetupSignalSource();

    //
    // Enable the GPIO port used for the CCP1 input.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Configure the Timer0 CCP1 function to use PD7
    //
    GPIOPinConfigure(GPIO_PD7_CCP1);
    GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_7);

    //
    // Set up Timer0B for edge-timer mode, positive edge
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_CAP_TIME);
    TimerControlEvent(TIMER0_BASE, TIMER_B, TIMER_EVENT_POS_EDGE);
    TimerLoadSet(TIMER0_BASE, TIMER_B, 0xffff);

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
    // Put the attributes in a known state for the uDMA Timer0B channel.  These
    // should already be disabled by default.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_TMR0B,
                                    UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

    //
    // Configure DMA channel for Timer0B to transfer 16-bit values, 1 at a
    // time.  The source is fixed and the destination increments by 16-bits
    // (2 bytes) at a time.
    //
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_TMR0B | UDMA_PRI_SELECT,
                              UDMA_SIZE_16 | UDMA_SRC_INC_NONE |
                              UDMA_DST_INC_16 | UDMA_ARB_1);

    //
    // Set up the transfer parameters for the Timer0B primary control
    // structure.  The mode is set to basic, the transfer source is the
    // Timer0B register, and the destination is a memory buffer.  The
    // transfer size is set to a fixed number of capture events.
    //
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_TMR0B | UDMA_PRI_SELECT,
                               UDMA_MODE_BASIC,
                               (void *)(TIMER0_BASE + TIMER_O_TBR),
                               g_usTimerBuf, MAX_TIMER_EVENTS);

    //
    // Enable the timer capture event interrupt.  Note that this signal is
    // used to trigger the DMA request and not an actual interrupt.
    // Start the capture timer running and enable its interrupt channel.
    // The timer interrupt channel is used by the uDMA controller.
    //
    UARTprintf("Starting timer and uDMA\n");
    TimerIntEnable(TIMER0_BASE, TIMER_CAPB_EVENT);
    TimerEnable(TIMER0_BASE, TIMER_B);
    IntEnable(INT_TIMER0B);

    //
    // Now enable the DMA channel for Timer0B.  It should now start performing
    // transfers whenever there is a rising edge detected on the CCP1 pin.
    //
    ROM_uDMAChannelEnable(UDMA_CHANNEL_TMR0B);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

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
    if(g_ulTimer0BIntCount != 1)
    {
        UARTprintf("\nUnexpected number of interrupts occurrred (%d)!!!\n\n",
                   g_ulTimer0BIntCount);
    }

    //
    // Display the timer values that were captured using the edge capture timer
    // with uDMA.  Compare the difference between stored values to the PWM
    // period and make sure they match.  This verifies that the edge capture
    // DMA transfers were occurring with the correct timing.
    //
    UARTprintf("\n      Captured\n");
    UARTprintf("Event   Value   Difference  Status\n");
    UARTprintf("----- --------  ----------  ------\n");
    for(ulIdx = 1; ulIdx < MAX_TIMER_EVENTS; ulIdx++)
    {
        //
        // Due to timer erratum, when the timer rolls past 0 as it counts
        // down, it will trigger an additional DMA transfer even though there
        // was not an edge capture.  This will appear in the data buffer as
        // a duplicate value - the value will be the same as the prior capture
        // value.  Therefore, in this example we skip past the duplicated
        // value.
        //
        if(g_usTimerBuf[ulIdx] == g_usTimerBuf[ulIdx - 1])
        {
            UARTprintf(" %2u    0x%04X   skipped duplicate\n", ulIdx,
                       g_usTimerBuf[ulIdx]);
            continue;
        }

        //
        // Compute the difference between adjacent captured values, and then
        // compare that to the expected timeout period.
        //
        usTimerElapsed = g_usTimerBuf[ulIdx - 1] - g_usTimerBuf[ulIdx];
        usTimerErr = usTimerElapsed > TIMEOUT_VAL ?
                     usTimerElapsed - TIMEOUT_VAL :
                     TIMEOUT_VAL - usTimerElapsed;

        //
        // Print the captured value and the difference from the previous
        //
        UARTprintf(" %2u    0x%04X   %8u   ", ulIdx, g_usTimerBuf[ulIdx],
                   usTimerElapsed);

        //
        // Print error status based on the deviation from expected difference
        // between samples (calculated above).  Allow for a difference of up
        // to 1 cycle.  Any more than that is considered an error.
        //
        if(usTimerErr >  1)
        {
            UARTprintf(" ERROR\n");
        }
        else
        {
            UARTprintf("   OK\n");
        }
    }

    //
    // End of application
    //
    while(1)
    {
    }
}
