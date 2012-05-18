//*****************************************************************************
//
// hibernate.c - Hibernation example.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/buttons.h"
#include "inc/hw_hibernate.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"

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
    unsigned char ucData, ucDelta;

    //
    // Increment the tick counter.
    //
    g_ulSysTickCount++;

    //
    // Get buttons status using button debouncer driver
    //
    ucData = ButtonsPoll(&ucDelta, 0);

    //
    // See if the select button was just pressed.
    //
    if(BUTTON_PRESSED(SELECT_BUTTON, ucData, ucDelta))
    {
        //
        // Set a flag to indicate that the select button was just pressed.
        //
        bSelectPressed = 1;
    }

    //
    // Else, see if the select button was just released.
    //
    if(BUTTON_RELEASED(SELECT_BUTTON, ucData, ucDelta))
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
    tContext sContext;
    tRectangle sRect;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);

    //
    // Initialize the OLED display
    //
    CFAL96x64x16Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sCFAL96x64x16);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.sYMax = 9;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Change foreground for white text.
    //
    GrContextForegroundSet(&sContext, ClrWhite);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&sContext, "hibernate", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 4, 0);

    //
    // Initialize the buttons driver
    //
    ButtonsInit();

    //
    // Set up systick to generate interrupts at 100 Hz.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 100);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Enable the Hibernation module.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

    //
    // Print wake cause message on display.
    //
    GrStringDrawCentered(&sContext, "Wake due to:", -1,
                         GrContextDpyWidthGet(&sContext) / 2, ROW(2) + 4,
                         true);

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
        HibernateIntClear(ulStatus);

        //
        // Wake was due to the push button.
        //
        if(ulStatus & HIBERNATE_INT_PIN_WAKE)
        {
            GrStringDrawCentered(&sContext, "BUTTON", -1,
                                 GrContextDpyWidthGet(&sContext) / 2,
                                 ROW(3) + 4, true);
        }

        //
        // Wake was due to RTC match
        //
        else if(ulStatus & HIBERNATE_INT_RTC_MATCH_0)
        {
            GrStringDrawCentered(&sContext, "TIMEOUT", -1,
                                 GrContextDpyWidthGet(&sContext) / 2,
                                 ROW(3) + 4, true);
        }

        //
        // Wake is due to neither button nor RTC, so it must have been a hard
        // reset.
        //
        else
        {
            GrStringDrawCentered(&sContext, "RESET", -1,
                                 GrContextDpyWidthGet(&sContext) / 2,
                                 ROW(3) + 4, true);
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
    HibernateEnableExpClk(ROM_SysCtlClockGet());

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
        GrStringDrawCentered(&sContext, "RESET", -1,
                             GrContextDpyWidthGet(&sContext) / 2,
                             ROW(3) + 4, true);

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
    GrStringDrawCentered(&sContext, cBuf, -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(1) + 4, true);

    //
    // Print messages on the screen about hibernation.
    //
    GrStringDrawCentered(&sContext, "Select to Hib", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(4) + 4, true);
    GrStringDrawCentered(&sContext, "Wake in 5 s,", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(5) + 4, true);
    GrStringDrawCentered(&sContext, "or press Select", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(6) + 4, true);
    GrStringDrawCentered(&sContext, "for immed. wake.", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(7) + 4, true);

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
    GrStringDrawCentered(&sContext, "                ", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(4) + 4, true);
    GrStringDrawCentered(&sContext, "Release the", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(5) + 4, true);
    GrStringDrawCentered(&sContext, "button.", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(6) + 4, true);
    GrStringDrawCentered(&sContext, "                ", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         ROW(7) + 4, true);

    //
    // Wait for user to release the button.
    //
    while(bSelectPressed)
    {
    }

    //
    // If hibernation count is very large, it may be that there was already
    // a value in the hibernate memory, so reset the count.
    //
    ulHibernateCount = (ulHibernateCount > 10000) ? 0 : ulHibernateCount;

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
    sRect.sXMin = 0;
    sRect.sXMax = 95;
    sRect.sYMin = 0;
    sRect.sYMax = 63;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);
    GrContextForegroundSet(&sContext, ClrWhite);
    ulIdx = 0;
    while(cErrorText[ulIdx])
    {
        GrStringDraw(&sContext, cErrorText[ulIdx], -1, COL(0), ROW(ulIdx), true);
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
    ROM_SysCtlReset();

    //
    // Finished.
    //
    while(1)
    {
    }
}
