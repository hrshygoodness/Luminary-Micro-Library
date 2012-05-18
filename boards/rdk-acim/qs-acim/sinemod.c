//*****************************************************************************
//
// sinemod.c - Sine wave modulation routine.
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

#include "inc/hw_types.h"
#include "utils/sine.h"
#include "sinemod.h"
#include "ui.h"

//*****************************************************************************
//
//! \page sinemod_intro Introduction
//!
//! Sine wave modulation is used for driving single-phase AC induction motors
//! and is a method of driving three-phase AC induction motors.  Two or three
//! sine waves, with the appropriate phase shift (180 degrees for single-phase
//! motors and 120 degrees for three-phase motors) are produced.
//!
//! For single-phase motors, this produces an alternating current in the single
//! motor winding, exactly as would be seen by simply connecting the motor to
//! the mains power.  The amplitude of the voltage applied to the motor is the
//! full DC bus voltage.
//!
//! For three-phase motors, this produces an alternating current between each
//! winding pair.  The difference between sine waves that are 120 degrees out
//! of phase is a sine wave with an amplitude of ~86.6% the amplitude of the
//! original sine waves.  Therefore, the full DC bus is not utilized.
//!
//! In order to obtain full DC bus utilization with three-phase motors,
//! over-modulation is supported by specifying an amplitude greater than one.
//! With over-modulation, the portion of the sine wave that is greater than
//! one is clipped to one and the portion less than negative one is clipped to
//! negative one.  During the portion of the sine wave that has been
//! flat-topped, the phase-to-phase current will exceed 86.6%; once the pair
//! of flat-tops start to line up, full DC bus utilization will be achieved.
//! This downside to over-modulation is an increase in the harmonic distortion
//! of the drive waveforms.
//!
//! The code for producing sine wave modulated waveforms is contained in
//! <tt>sinemod.c</tt>, with <tt>sinemod.h</tt> containing the definition for
//! the function exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup sinemod_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Computes sine wave modulated waveforms.
//!
//! \param ulAngle is the current angle of the waveform expressed as a 0.32
//! fixed point value that is the percentage of the way around a circle.
//! \param ulAmplitude is the amplitude of the waveform, as a 16.16 fixed point
//! value.
//! \param pulDutyCycles is a pointer to an array of three unsigned longs to be
//! filled in with the duty cycles of the waveforms, in 16.16 fixed point
//! values between zero and one.
//!
//! This function finds the duty cycle percentage of the sine waveforms for the
//! given angle.  For three-phase operation, there are three waveforms
//! produced, each 120 degrees apart.  For single-phase operation, there are
//! two waveforms produced, each 180 degrees apart.  If the amplitude of the
//! waveform is larger than one, the waveform will be clipped after scaling
//! (flat-topping).
//!
//! \return None.
//
//*****************************************************************************
void
SineModulate(unsigned long ulAngle, unsigned long ulAmplitude,
             unsigned long *pulDutyCycles)
{
    unsigned long ulIdx;
    long lValue;

    //
    // See if this is a single-phase or three-phase motor.
    //
    if(HWREGBITH(&(g_sParameters.usFlags), FLAG_MOTOR_TYPE_BIT) ==
       FLAG_MOTOR_TYPE_1PHASE)
    {
        //
        // Get the sine of this angle.
        //
        lValue = sine(ulAngle) / 2;

        //
        // Multiple the sine by the amplitude.  Special care is taken to keep
        // from overflowing the 32-bit numbers and obtain the correct 16.16
        // fixed point value.
        //
        lValue = ((lValue * (long)(ulAmplitude >> 16)) +
                  ((lValue * (long)(ulAmplitude & 0xffff)) / (long)65536));

        //
        // If the amplitude of the waveform is greater than one or less than
        // negative one then clip it to one or negative one.
        //
        if(lValue > 32767)
        {
            lValue = 32767;
        }
        if(lValue < -32767)
        {
            lValue = -32767;
        }

        //
        // Ouptut the duty cycle of this waveform, adjusting it to be between
        // zero and one, centered around one half.
        //
        pulDutyCycles[0] = lValue + 32768;
        pulDutyCycles[1] = 32768 - lValue;
        pulDutyCycles[2] = 0;
    }
    else
    {
        //
        // Loop through the three waveforms, each 120 degrees apart.
        //
        for(ulIdx = 0; ulIdx < 3; ulIdx++)
        {
            //
            // Get the sine of this angle.
            //
            lValue = sine(ulAngle) / 2;

            //
            // Multiple the sine by the amplitude.  Special care is taken to
            // keep from overflowing the 32-bit numbers and obtain the correct
            // 16.16 fixed point value.
            //
            lValue = ((lValue * (long)(ulAmplitude >> 16)) +
                      ((lValue * (long)(ulAmplitude & 0xffff)) / (long)65536));

            //
            // If the amplitude of the waveform is greater than one or less
            // than negative one then clip it to one or negative one.
            //
            if(lValue > 32767)
            {
                lValue = 32767;
            }
            if(lValue < -32768)
            {
                lValue = -32768;
            }

            //
            // Ouptut the duty cycle of this waveform, adjusting it to be
            // between zero and one, centered around one half.
            //
            pulDutyCycles[ulIdx] = lValue + 32768;

            //
            // Decrement the angle by 120 degrees.
            //
            ulAngle -= 1431655765;
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
