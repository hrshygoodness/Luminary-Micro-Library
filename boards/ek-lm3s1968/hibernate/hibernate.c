//*****************************************************************************
//
// hibernate.c - Hibernation example.
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
// This is part of revision 10636 of the EK-LM3S1968 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Hibernate Example (hibernate)</h1>
//!
//! An example to demonstrate the use of the Hibernation module.  The user
//! can put the microcontroller in hibernation by pressing the select
//! button.  The microcontroller will then wake on its own after 5 seconds,
//! or immediately if the user presses the select button again.  The program
//! keeps a count of the number of times it has entered hibernation.  The
//! value of the counter is stored in the battery backed memory of the
//! Hibernation module so that it can be retrieved when the microcontroller
//! wakes.
//
//*****************************************************************************

//*****************************************************************************
//
// Macros to convert character based display rows and columns into the pixel
// based X and Y coordinates needed for display drawing functions.
//
//*****************************************************************************
#define COL(c) ((c) * 6)
#define ROW(r) ((r) * 8)

//*****************************************************************************
//
// A counter that counts the number of ticks of the SysTick interrupt.
//
//*****************************************************************************
volatile unsigned long g_ulSysTickCount = 0;

//*****************************************************************************
//
// A buffer to hold character strings that will be printed to the display.
//
//*****************************************************************************
static char cBuf[40];

//*****************************************************************************
//
// A buffer that holds one display line of dashes.
//
//*****************************************************************************
static char cDashLine[] = "---------------------";

//*****************************************************************************
//
// Text that will be displayed if there is an error.
//
//*****************************************************************************
static char *cErrorText[] =
{
    "The controller did",
    "not enter hib mode.",
    "This could occur if",
    "the button were held",
    "down when trying to",
    "hibernate.",
    "---------------------",
    "   PRESS BUTTON",
    "    TO RESTART",
    0
};

//*****************************************************************************
//
// A flag to indicate the select button was pressed.
//
//*****************************************************************************
static volatile unsigned char bSelectPressed = 0;

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
// Delay a certain number of SysTick timer ticks.
//
//*****************************************************************************
void
SysTickWait(unsigned long ulTicks)
{
    ulTicks += g_ulSysTickCount;
    while(g_ulSysTickCount <= ulTicks)
    {
    }
}

//*****************************************************************************
//
// The SysTick handler.  Increments a tick counter and debounces the push
// button.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    static unsigned char ucSwitches = 0x80;
    static unsigned char ucSwitchClockA = 0;
    static unsigned char ucSwitchClockB = 0;
    unsigned long ulData, ulDelta;

    //
    // Increment the tick counter.
    //
    g_ulSysTickCount++;

    //
    // Read the state of the select button.
    //
    ulData = (GPIOPinRead(GPIO_PORTG_BASE, GPIO_PIN_7));

    //
    // Determine the switches that are at a different state than the debounced
    // state.
    //
    ulDelta = ulData ^ ucSwitches;

    //
    // Increment the clocks by one.
    //
    ucSwitchClockA ^= ucSwitchClockB;
    ucSwitchClockB = ~ucSwitchClockB;

    //
    // Reset the clocks corresponding to switches that have not changed state.
    //
    ucSwitchClockA &= ulDelta;
    ucSwitchClockB &= ulDelta;

    //
    // Get the new debounced switch state.
    //
    ucSwitches &= ucSwitchClockA | ucSwitchClockB;
    ucSwitches |= (~(ucSwitchClockA | ucSwitchClockB)) & ulData;

    //
    // Determine the switches that just changed debounced state.
    //
    ulDelta ^= (ucSwitchClockA | ucSwitchClockB);

    //
    // See if the select button was just pressed.
    //
    if((ulDelta & 0x80) && !(ucSwitches & 0x80))
    {
        //
        // Set a flag to indicate that the select button was just pressed.
        //
        bSelectPressed = 1;
    }

    //
    // Else, see if the select button was just released.
    //
    if((ulDelta & 0x80) && (ucSwitches & 0x80))
    {
        //
        // Clear the button pressed flag.
        //
        bSelectPressed = 0;
    }
}

//*****************************************************************************
//
// Run the hibernate example.  Use a loop to put the microcontroller into
// hibernate mode, and to wake up based on time. Also allow the user to cause
// it to hibernate and/or wake up based on button presses.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIdx;
    unsigned long ulStatus = 0;
    unsigned long ulHibernateCount = 0;

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Initialize the OLED display, and print title message.
    //
    RIT128x96x4Init(1000000);
    RIT128x96x4StringDraw("Hibernate Example", COL(2), ROW(0), 15);
    RIT128x96x4StringDraw(cDashLine, COL(0), ROW(1), 15);

    //
    // Initialize the GPIO needed for the button.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    GPIOPinTypeGPIOInput(GPIO_PORTG_BASE, GPIO_PIN_7);
    GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    //
    // Set up systick to generate interrupts at 100 Hz.
    //
    SysTickPeriodSet(SysCtlClockGet() / 100);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Enable the Hibernation module.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

    //
    // Print wake cause message on display.
    //
    RIT128x96x4StringDraw("Last wake due to:", COL(1), ROW(3), 15);

    //
    // Per an erratum, sometimes on wake the Hibernation module control
    // register will be cleared when it should not be.  As a workaround a
    // location in the non-volatile data area can be read instead.  This data
    // area is cleared to 0 on reset, so if the first location is non-zero then
    // the Hibernation module is in use.  In this case, re-enable the
    // Hibernation module which will ensure that the control register bits have
    // the proper value.
    //
    HibernateDataGet(&ulHibernateCount, 1);
    if(ulHibernateCount)
    {
        HibernateEnableExpClk(SysCtlClockGet());
        HibernateClockSelect(HIBERNATE_CLOCK_SEL_DIV128);
    }

    //
    // Check to see if Hibernation module is already active, which could mean
    // that the processor is waking from a hibernation.
    //
    if(HibernateIsActive())
    {
        //
        // Read the status bits to see what caused the wake.
        //
        ulStatus = HibernateIntStatus(0);

        //
        // Wake was due to the push button.
        //
        if(ulStatus & HIBERNATE_INT_PIN_WAKE)
        {
            RIT128x96x4StringDraw("BUTTON", COL(7), ROW(4), 15);
        }

        //
        // Wake was due to RTC match
        //
        else if(ulStatus & HIBERNATE_INT_RTC_MATCH_0)
        {
            RIT128x96x4StringDraw("TIMEOUT", COL(6), ROW(4), 15);
        }

        //
        // Wake is due to neither button nor RTC, so it must have been a hard
        // reset.
        //
        else
        {
            RIT128x96x4StringDraw("RESET", COL(7), ROW(4), 15);
        }

        //
        // If the wake is due to button or RTC, then read the first location
        // from the battery backed memory, as the hibernation count.
        //
        if(ulStatus & (HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_RTC_MATCH_0))
        {
            HibernateDataGet(&ulHibernateCount, 1);
        }
    }

    //
    // Enable the Hibernation module.  This should always be called, even if
    // the module was already enabled, because this function also initializes
    // some timing parameters.
    //
    HibernateEnableExpClk(SysCtlClockGet());

    //
    // If the wake was not due to button or RTC match, then it was a reset.
    //
    if(!(ulStatus & (HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_RTC_MATCH_0)))
    {
        //
        // Configure the module clock source.
        //
        HibernateClockSelect(HIBERNATE_CLOCK_SEL_DIV128);

        //
        // Finish the wake cause message.
        //
        RIT128x96x4StringDraw("RESET", COL(7), ROW(4), 15);

        //
        // Wait a couple of seconds in case we need to break in with the
        // debugger.
        //
        SysTickWait(3 * 100);

        //
        // Allow time for the crystal to power up.  This line is separated from
        // the above to make it clear this is still needed, even if the above
        // delay is removed.
        //
        SysTickWait(15);
    }

    //
    // Print the count of times that hibernate has occurred.
    //
    usnprintf(cBuf, sizeof(cBuf), "Hib count=%4u", ulHibernateCount);
    RIT128x96x4StringDraw(cBuf, COL(3), ROW(2), 15);

    //
    // Print messages on the screen about hibernation.
    //
    RIT128x96x4StringDraw("Press Select to", COL(3), ROW(6), 15);
    RIT128x96x4StringDraw("hibernate.", COL(6), ROW(7), 15);
    RIT128x96x4StringDraw("Will wake in 5 secs,", COL(0), ROW(9), 15);
    RIT128x96x4StringDraw("or press Select for", COL(0), ROW(10), 15);
    RIT128x96x4StringDraw("immediate wake.", COL(2), ROW(11), 15);
    RIT128x96x4StringDraw(cDashLine, COL(0), ROW(5), 15);
    RIT128x96x4StringDraw(cDashLine, COL(0), ROW(8), 15);

    //
    // Clear the button pressed flag, in case it was held down at the
    // beginning.
    //
    bSelectPressed = 0;

    //
    // Wait for user to press the button.
    //
    while(!bSelectPressed)
    {
        //
        // Wait a bit before looping again.
        //
        SysTickWait(10);
    }

    //
    // Tell user to release the button.
    //
    RIT128x96x4StringDraw("  Release the   ", COL(3), ROW(6), 15);
    RIT128x96x4StringDraw(" button.      ", COL(6), ROW(7), 15);

    //
    // Wait for user to release the button.
    //
    while(bSelectPressed)
    {
    }

    //
    // Increment the hibernation count, and store it in the battery backed
    // memory.
    //
    ulHibernateCount++;
    HibernateDataSet(&ulHibernateCount, 1);

    //
    // Clear and enable the RTC and set the match registers to 5 seconds in the
    // future. Set both to same, though they could be set differently, the
    // first to match will cause a wake.
    //
    HibernateRTCSet(0);
    HibernateRTCEnable();
    HibernateRTCMatch0Set(5);
    HibernateRTCMatch1Set(5);

    //
    // Set wake condition on pin or RTC match.  Board will wake when 5 seconds
    // elapses, or when the button is pressed.
    //
    HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);

    //
    // Request hibernation.
    //
    HibernateRequest();

    //
    // Give it time to activate, it should never get past this wait.
    //
    SysTickWait(100);

    //
    // Should not have got here, something is wrong.  Print an error message to
    // the user.
    //
    RIT128x96x4Clear();
    ulIdx = 0;
    while(cErrorText[ulIdx])
    {
        RIT128x96x4StringDraw(cErrorText[ulIdx], COL(0), ROW(ulIdx), 15);
        ulIdx++;
    }

    //
    // Wait for the user to press the button, then restart the app.
    //
    bSelectPressed = 0;
    while(!bSelectPressed)
    {
    }

    //
    // Reset the processor.
    //
    SysCtlReset();

    //
    // Finished.
    //
    while(1)
    {
    }
}
