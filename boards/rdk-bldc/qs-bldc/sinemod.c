//*****************************************************************************
//
// sinemod.c - Sine wave modulation routine.
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
// This is part of revision 10636 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "utils/sine.h"
#include "sinemod.h"

//*****************************************************************************
//
//! \page sinemod_intro Introduction
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

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
