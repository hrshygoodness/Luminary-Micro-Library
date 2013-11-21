//*****************************************************************************
//
// trapmod.c - Trapezoid modulation routine.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "main.h"
#include "pins.h"
#include "pwm_ctrl.h"
#include "trapmod.h"
#include "ui.h"

//*****************************************************************************
//
// \page trapmod_intro Introduction
//
// The code for producing trapezoid modulated waveforms is contained in
// <tt>trapmod.c</tt>, with <tt>trapmod.h</tt> containing the definition for
// the function exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
// \defgroup trapmod_api Definitions
// @{
//
//*****************************************************************************

//*****************************************************************************
//
// PWM Phase A (High + Low).
//
//*****************************************************************************
#define PHASE_A         (PWM_PHASEA_HIGH | PWM_PHASEA_LOW)

//*****************************************************************************
//
// PWM Phase B (High + Low).
//
//*****************************************************************************
#define PHASE_B         (PWM_PHASEB_HIGH | PWM_PHASEB_LOW)

//*****************************************************************************
//
// PWM Phase C (High + Low).
//
//*****************************************************************************
#define PHASE_C         (PWM_PHASEC_HIGH | PWM_PHASEC_LOW)

//*****************************************************************************
//
// Mapping from Hall States to Phase Drive states (120 degree spacing).
//
// This array maps the hall state value to the set of PWM
// signals that should be driving at that time.
//
// \verbatim
//     ---------+---+---+---+---+---+---+
//     Phase A  | - | - | Z | + | + | Z |
//     Phase B  | + | Z | - | - | Z | + |
//     Phase C  | Z | + | + | Z | - | - |
//     ---------+---+---+---+---+---+---+
//     Hall A   | 1 | 1 | 1 | 0 | 0 | 0 |
//     Hall B   | 0 | 0 | 1 | 1 | 1 | 0 |
//     Hall C   | 1 | 0 | 0 | 0 | 1 | 1 |
//     ---------+---+---+---+---+---+---+
// \endverbatim
//
//*****************************************************************************
static const unsigned long g_ulHallToPhase120[8] =
{
    0,
    PWM_PHASEC_HIGH | PWM_PHASEA_LOW,
    PWM_PHASEA_HIGH | PWM_PHASEB_LOW,
    PWM_PHASEC_HIGH | PWM_PHASEB_LOW,
    PWM_PHASEB_HIGH | PWM_PHASEC_LOW,
    PWM_PHASEB_HIGH | PWM_PHASEA_LOW,
    PWM_PHASEA_HIGH | PWM_PHASEC_LOW,
    0
};

//*****************************************************************************
//
// Mapping from Hall States to Phase Drive states (60 degree spacing).
//
// This array maps the hall state value to the set of PWM
// signals that should be driving at that time.
//
// \verbatim
//     ---------+---+---+---+---+---+---+
//     Phase A  | + | Z | - | - | Z | + |
//     Phase B  | - | - | Z | + | + | Z |
//     Phase C  | Z | + | + | Z | - | - |
//     ---------+---+---+---+---+---+---+
//     Hall A   | 0 | 1 | 1 | 1 | 0 | 0 |
//     Hall B   | 0 | 0 | 1 | 1 | 1 | 0 |
//     Hall C   | 0 | 0 | 0 | 1 | 1 | 1 |
//     ---------+---+---+---+---+---+---+
// \endverbatim
//
//*****************************************************************************
static const unsigned long g_ulHallToPhase60[8] =
{
    PWM_PHASEA_HIGH | PWM_PHASEB_LOW,
    PWM_PHASEC_HIGH | PWM_PHASEB_LOW,
    0,
    PWM_PHASEC_HIGH | PWM_PHASEA_LOW,
    PWM_PHASEA_HIGH | PWM_PHASEC_LOW,
    0,
    PWM_PHASEB_HIGH | PWM_PHASEC_LOW,
    PWM_PHASEB_HIGH | PWM_PHASEA_LOW,
};

//*****************************************************************************
//
// Controls trapezoid modulated waveforms.
//
// \param ulHall is the current Hall state value for the motor.  This
// value may be read directly from the Hall sensors, if installed, or
// derived from the Back EMF or Linear Hall sensor readings.
//
// This function will control the PWM generator channels based on the changes
// in the Hall Effect sensor value.
//
// \return None.
//
//*****************************************************************************
void
TrapModulate(unsigned long ulHall)
{
    unsigned long ulEnable;

    //
    // If the motor is not running, there is nothing to do.
    //
    if(!MainIsRunning())
    {
        return;
    }

    //
    // Convert the Hall value into a bit-mapped phase enable value.
    //
    if(UI_PARAM_MODULATION == MODULATION_SENSORLESS)
    {
        ulEnable = g_ulHallToPhase120[ulHall];
    }
    else if(UI_PARAM_SENSOR_TYPE == SENSOR_TYPE_GPIO_60)
    {
        ulEnable = g_ulHallToPhase60[ulHall];
    }
    else
    {
        ulEnable = g_ulHallToPhase120[ulHall];
    }

    //
    // If running in reverse, invert the PWM phases.
    //
    if(MainIsReverse())
    {
        if(ulEnable & PHASE_A)
        {
            ulEnable ^= PHASE_A;
        }
        if(ulEnable & PHASE_B)
        {
            ulEnable ^= PHASE_B;
        }
        if(ulEnable & PHASE_C)
        {
            ulEnable ^= PHASE_C;
        }
    }

    //
    // Switch the PWM outputs.
    //
    PWMOutputTrapezoid(ulEnable);
}

//*****************************************************************************
//
// Close the Doxygen group.
// @}
//
//*****************************************************************************
