//*****************************************************************************
//
// main.c - Stepper motor drive main application.
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
// This is part of revision 10636 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "blinker.h"
#include "main.h"
#include "stepcfg.h"
#include "ui.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Stepper Motor Drive Application (qs-stepper)</h1>
//!
//! This application is a motor drive for bipolar stepper motors.  The
//! following features are available:
//!
//! - Full and Half Stepping
//! - Slow and Fast Current Decay
//! - Chopping or PWM current control modes
//! - Adjustable Drive and Holding Current
//! - Stepping rates up to 10,000 steps/second for suitable motors
//! - DC bus voltage monitoring and CPU temperature monitoring
//! - Overcurrent fault protection.
//! - Simple on-board user interface (via a potentiometer and push button)
//! - Comprehensive serial user interface
//! - Several configurable drive parameters
//! - Persistent storage of drive parameters in flash
//
//*****************************************************************************

//*****************************************************************************
//
//! \page main_intro Introduction
//!
//! This is the application's main entry point.  It provides default
//! handlers for NMI, Fault, and unhandled interrupts.  It also provides
//! the C main() entry point.
//!
//! This module just provides a bare minimum hardware initialization, and
//! then calls the initialization function for the User Interface.  After
//! that it stays in a loop, putting the processor to sleep when no interrupt
//! is being serviced. 
//!
//! The code for this module is contained in <tt>main.c</tt>,
//! with <tt>main.h</tt> containing the definitions for the variables and
//! functions exported to the rest of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup main_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Handles errors from the driver library.
//!
//! This function is called when an error is encountered by the driver library.
//! Typically, the errors will be invalid parameters passed to the library's
//! APIs.
//!
//! In this application, nothing is done in this function.  It does provide a
//! convenient location for a breakpoint that will catch all driver library
//! errors.
//!
//! \return None.
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
//! This is the code that gets called when the processor receives a NMI.  This
//! simply enters an infinite loop, preserving the system state for examination
//! by a debugger.
//!
//! \return None.
//
//*****************************************************************************
void
NmiSR(void)
{
    //
    // Disable all interrupts.
    //
    IntMasterDisable();

    //
    // Turn off all the PWM outputs.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                              PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT,
                              false);

    //
    // Turn on STATUS and MODE LEDs.  They will remain on.
    //
    BlinkStart(STATUS_LED, 1, 1, 1);
    BlinkStart(MODE_LED, 1, 1, 1);
    BlinkHandler();

    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
//! This is the code that gets called when the processor receives a fault
//! interrupt.  This simply enters an infinite loop, preserving the system
//! state for examination by a debugger.
//!
//! \return None.
//
//*****************************************************************************
void
FaultISR(void)
{
    //
    // Disable all interrupts.
    //
    IntMasterDisable();

    //
    // Turn off all the PWM outputs.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                              PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT,
                              false);

    //
    // Turn on STATUS and MODE LEDs.  They will remain on.
    //
    BlinkStart(STATUS_LED, 1, 1, 1);
    BlinkStart(MODE_LED, 1, 1, 1);
    BlinkHandler();

    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
//! This is the code that gets called when the processor receives an unexpected
//! interrupt.  This simply enters an infinite loop, preserving the system
//! state for examination by a debugger.
//!
//! \return None.
//
//*****************************************************************************
void
IntDefaultHandler(void)
{
    //
    // Disable all interrupts.
    //
    IntMasterDisable();

    //
    // Turn off all the PWM outputs.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                              PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT,
                              false);

    //
    // Turn on STATUS and MODE LEDs.  They will remain on.
    //
    BlinkStart(STATUS_LED, 1, 1, 1);
    BlinkStart(MODE_LED, 1, 1, 1);
    BlinkHandler();

    //
    // Go into an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
//! The application main entry point.
//!
//! This is where the program starts running.  It initializes the system
//! clock.  Then it calls the User Interface module initialization function,
//! UIInit().  This will cause all the other modules to be initialized.
//! Finally, it enters an infinite loop, sleeping while waiting for an
//! interrupt to occur.
//!
//! \return None.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set cpu clock for 50 MHz
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_6MHZ);

    //
    // Enable peripherals to operate when CPU is in sleep
    //
    SysCtlPeripheralClockGating(true);

    //
    // Initialize the user interface,
    // on-board and off-board
    //
    UIInit();

    //
    // Loop forever.  All the real work is done in interrupt handlers.
    //
    while(1)
    {
        //
        // Put the processor to sleep when it is waiting for an
        // interrupt to occur.
        //
        SysCtlSleep();
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
