//*****************************************************************************
//
// inrush.c - In-rush current control routine.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "adc_ctrl.h"
#include "main.h"
#include "pins.h"
#include "ui.h"

//*****************************************************************************
//
//! \page inrush_intro Introduction
//!
//! On initial power-up, an in-rush current limiting resistor is applied in
//! series with the AC power line input.  This slows the flow of current into
//! the DC bus capacitors, preventing damage to the power supply section of the
//! board.
//!
//! Once the DC bus voltage reaches a reasonable level (200 V), the in-rush
//! resistor is bypassed by closing a relay.  At this point, the DC bus voltage
//! quickly rises to its operating level.
//!
//! This current limiting function is a one-time process that occurs when the
//! application first starts.  The in-rush resistor is sized such that it could
//! remain active for extended periods of time (for example, if the flash of
//! the microcontroller is erased and there is no code to turn on the relay).
//! The motor should never be run when the in-rush resistor is active.
//!
//! The code for handling in-rush current limiting is contained in
//! <tt>inrush.c</tt>, with <tt>inrush.h</tt> containing the definition for
//! the functions exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup inrush_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Handles the in-rush current control.
//!
//! This function delays while the in-rush current control resistor slows the
//! buildup of voltage in the DC bus capacitors.  Once the voltage is at an
//! adequate level, the in-rush current control resistor is taken out of the
//! circuit to allow current to freely flow from the AC line into the DC bus
//! capacitors.  This is called on startup to avoid excessive current into the
//! DC bus.
//!
//! \return None.
//
//*****************************************************************************
void
InRushDelay(void)
{
    unsigned long ulCount;

    //
    // Blink the fault LED slowly to indicate that in-rush limiting circuit is
    // active.
    //
    UIFaultLEDBlink(100, 50);

    //
    // Configure the first timer to produce a 80% duty cycle PWM at 20 KHz,
    // resulting in 12 V at the in-rush control relay.
    //
    TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM |
                                 TIMER_CFG_B_PERIODIC));
    ulCount = SYSTEM_CLOCK / 20000;
    TimerLoadSet(TIMER0_BASE, TIMER_A, ulCount);
    TimerMatchSet(TIMER0_BASE, TIMER_A, (ulCount * (15 - 12)) / 15);
    TimerEnable(TIMER0_BASE, TIMER_A);

    //
    // Turn the in-rush control relay off so that it will limit the in-rush
    // current.
    //
    GPIODirModeSet(PIN_CCP0_PORT, PIN_CCP0_PIN, GPIO_DIR_MODE_OUT);
    GPIOPinWrite(PIN_CCP0_PORT, PIN_CCP0_PIN, 0);

    //
    // Wait for ten interrupts before checking the DC bus voltage.  This allows
    // the ADC time to gather valid values from its inputs.
    //
    for(ulCount = 0; ulCount < 10; ulCount++)
    {
        SysCtlSleep();
    }

    //
    // Wait until the DC bus voltage rises above 200 V.
    //
    while(g_usBusVoltage < 200)
    {
    }

    //
    // Turn the in-rush control relay on so that it will no longer limit the
    // in-rush current.
    //
    GPIODirModeSet(PIN_CCP0_PORT, PIN_CCP0_PIN, GPIO_DIR_MODE_HW);

    //
    // Wait for the DC bus voltage to rise 20 V above the minimum bus voltage.
    //
    while(g_usBusVoltage < (g_sParameters.usMinVBus + 20))
    {
    }

    //
    // Turn off the fault LED to indicate that in-rush current limiting is
    // complete.
    //
    UIFaultLEDBlink(0, 0);
}

//*****************************************************************************
//
//! Adjusts the in-rush control relay drive signal for operating from the
//! crystal.
//!
//! This function adjusts the drive signal to the in-rush control relay to
//! achieve the desired drive frequency when operating the microcontroller from
//! the crystal instead of from the PLL.
//!
//! \return None.
//
//*****************************************************************************
void
InRushRelayAdjust(void)
{
    unsigned long ulCount;

    //
    // Reconfigure the timer to produce the same 80% duty cycle PWM at 20 KHz;
    // now it is based on the crystal clocking the system instead of the PLL.
    //
    ulCount = CRYSTAL_CLOCK / 20000;
    TimerDisable(TIMER0_BASE, TIMER_A);
    TimerLoadSet(TIMER0_BASE, TIMER_A, ulCount);
    TimerMatchSet(TIMER0_BASE, TIMER_A, (ulCount * (15 - 12)) / 15);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
