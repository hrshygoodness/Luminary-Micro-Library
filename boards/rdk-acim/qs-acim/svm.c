//*****************************************************************************
//
// svm.c - Space vector modulation routine.
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
// This is part of revision 10636 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#include "utils/sine.h"
#include "svm.h"

//*****************************************************************************
//
//! \page svm_intro Introduction
//!
//! Space vector modulation is a method used for driving three-phase AC
//! induction motors.  For each phase of the motor, the corresponding gates
//! will be in one of two states; either the high-side will be on or the
//! low-side will be on.  Therefore, for the three phases, there are eight
//! possible states for the gates (indicating which gate is on):
//!
//! \verbatim
//!     State   U Gate  V Gate  W Gate
//!     0       low     low     low
//!     1       high    low     low
//!     2       high    high    low
//!     3       low     high    low
//!     4       low     high    high
//!     5       low     low     high
//!     6       high    low     high
//!     7       high    high    high
//! \endverbatim
//!
//! Two of those state vectors (state 0 and 7) result in no current flowing
//! through the motor and are referred to as the zero vectors.  The remaining
//! six state vectors result in current flow, and each is spaced every 60
//! degrees around the circle.  Between these state vectors is a sector of the
//! circle.
//!
//! Every angle will fall into one of these sectors, which is bound by two of
//! the state vectors.  Outputting the two state vectors for the appropriate
//! time, and using the zero vectors for the remaining time in the PWM period,
//! any angle and amplitude can be produced.
//!
//! This process results in full utilization of the DC bus; for any angle, the
//! two active state vectors are scaled such that the combined vector reaches
//! the desired amplitude, and is capable of reaching the full DC bus
//! amplitude.
//!
//! The following waveforms show the appearance of the PWM signals in each
//! sector of the circle, along with the state vectors in use.  In each
//! drawing, Q0 is the low-side gate for the U phase, Q1 is the high-side gate
//! drive for the U phase, Q2 is the low-side gate for the V phase, Q3 is the
//! high-side gate for the V phase, Q4 is the low-side gate for the W phase,
//! and Q5 is the high-side gate for the W phase.
//!
//! \verbatim
//!               Sector 1                     Sector 2   
//!
//!        Q1: __----------__           Q1: ____------____
//!        Q0: --__________--           Q0: ----______----
//!
//!        Q3: ____------____           Q3: __----------__
//!        Q2: ----______----           Q2: --__________--
//!
//!        Q5: ______--______           Q5: ______--______
//!        Q4: ------__------           Q4: ------__------
//!
//!     State: 0 1 2 7 2 1 0         State: 0 3 2 7 2 3 0
//!
//!               Sector 3                     Sector 4
//!
//!        Q1: ______--______           Q1: ______--______
//!        Q0: ------__------           Q0: ------__------
//!
//!        Q3: __----------__           Q3: ____------____
//!        Q2: --__________--           Q2: ----______----
//!
//!        Q5: ____------____           Q5: __----------__
//!        Q4: ----______----           Q4: --__________--
//!
//!     State: 0 3 4 7 4 3 0         State: 0 5 4 7 4 5 0
//!
//!               Sector 5                     Sector 6
//!
//!        Q1: ____------____           Q1: __----------__
//!        Q0: ----______----           Q0: --__________--
//!
//!        Q3: ______--______           Q3: ______--______
//!        Q2: ------__------           Q2: ------__------
//!
//!        Q5: __----------__           Q5: ____------____
//!        Q4: --__________--           Q4: ----______----
//!
//!     State: 0 5 6 7 6 5 0         State: 0 1 6 7 6 1 0
//! \endverbatim
//!
//! Proper balancing of these states results in phase-to-phase sinusoidal
//! waveforms being presented to the motor, just as occurs with sine wave
//! modulation.  The real benefit is full utilization of the DC bus, providing
//! more torque from the motor.
//!
//! The code for producing space vector modulated waveforms is contained in
//! <tt>svm.c</tt>, with <tt>svm.h</tt> containing the definition for the
//! function exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup svm_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Computes space vector modulated waveforms.
//!
//! \param ulAngle is the current angle of the waveform expressed as a 0.32
//! fixed point value that is the percentage of the way around a circle.
//! \param ulAmplitude is the amplitude of the waveform, as a 16.16 fixed point
//! value.
//! \param pulDutyCycles is a pointer to an array of three unsigned longs to be
//! filled in with the duty cycles of the waveforms, in 16.16 fixed point
//! values between zero and one.
//!
//! This function finds the duty cycle percentages of the space vector
//! modulated waveforms for the given angle.  If the input amplitude is greater
//! than one, it will be clipped to one before computing the waveforms.
//!
//! \return None.
//
//*****************************************************************************
void
SpaceVectorModulate(unsigned long ulAngle, unsigned long ulAmplitude,
                    unsigned long *pulDutyCycles)
{
    unsigned long ulSector, ulSine1, ulSine2, ulExtra;

    //
    // Scale the amplitude to make it match what would be achieved with sine
    // modulation.  Clip the amplitude to a maximum of one.
    //
    if(ulAmplitude > 75674)
    {
        ulAmplitude = 65536;
    }
    else
    {
        ulAmplitude = (ulAmplitude * 56756) / 65536;
    }

    //
    // Convert the angle into the sector number and the angle within that
    // sector.
    //
    ulSector = ulAngle / 715827883;
    ulAngle %= 715827883;

    //
    // Get the sine of the angle within the sector, as well as the sine of
    // 60 degrees minus the angle.  Each is multiplied by the amplitude of
    // the waveform to determine the percentage of time corresponding to the
    // given state.
    //
    ulSine1 = (sine(ulAngle) * ulAmplitude) / 65536;
    ulSine2 = (sine(715827883 - ulAngle) * ulAmplitude) / 65536;

    //
    // Determine the percentage of remaining time to be split between the two
    // zero states.
    //
    ulExtra = (65536 - ulSine1 - ulSine2) / 2;

    //
    // Compute the duty cycles based on the sector.
    //
    switch(ulSector)
    {
        //
        // Sector zero resides between 0 and 60 degrees.
        //
        case 0:
        {
            //
            // The vector sequence is 0, 1, 2, 7, 2, 1, 0.
            //
            pulDutyCycles[0] = ulSine1 + ulSine2 + ulExtra;
            pulDutyCycles[1] = ulSine1 + ulExtra;
            pulDutyCycles[2] = ulExtra;

            //
            // Done with this sector.
            //
            break;
        }

        //
        // Sector one resides between 60 and 120 degrees.
        //
        case 1:
        {
            //
            // The vector sequence is 0, 3, 2, 7, 2, 3, 0.
            //
            pulDutyCycles[0] = ulSine2 + ulExtra;
            pulDutyCycles[1] = ulSine1 + ulSine2 + ulExtra;
            pulDutyCycles[2] = ulExtra;

            //
            // Done with this sector.
            //
            break;
        }

        //
        // Sector two resides between 120 and 180 degrees.
        //
        case 2:
        {
            //
            // The vector sequence is 0, 3, 4, 7, 4, 3, 0.
            //
            pulDutyCycles[0] = ulExtra;
            pulDutyCycles[1] = ulSine1 + ulSine2 + ulExtra;
            pulDutyCycles[2] = ulSine1 + ulExtra;

            //
            // Done with this sector.
            //
            break;
        }

        //
        // Sector three resides between 180 and 240 degrees.
        //
        case 3:
        {
            //
            // The vector sequence is 0, 5, 4, 7, 4, 5, 0.
            //
            pulDutyCycles[0] = ulExtra;
            pulDutyCycles[1] = ulSine2 + ulExtra;
            pulDutyCycles[2] = ulSine1 + ulSine2 + ulExtra;

            //
            // Done with this sector.
            //
            break;
        }

        //
        // Sector four resides between 240 and 300 degress.
        //
        case 4:
        {
            //
            // The vector sequence is 0, 5, 6, 7, 6, 5, 0.
            //
            pulDutyCycles[0] = ulSine1 + ulExtra;
            pulDutyCycles[1] = ulExtra;
            pulDutyCycles[2] = ulSine1 + ulSine2 + ulExtra;

            //
            // Done with this sector.
            //
            break;
        }

        //
        // Sector five resides between 300 and 360 degrees.  This is also the
        // default case, for angles larger than 360 degrees (which should not
        // occur, but just in case).
        //
        case 5:
        default:
        {
            //
            // The vector sequence is 0, 1, 6, 7, 6, 1, 0.
            //
            pulDutyCycles[0] = ulSine1 + ulSine2 + ulExtra;
            pulDutyCycles[1] = ulExtra;
            pulDutyCycles[2] = ulSine2 + ulExtra;

            //
            // Done with this sector.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
