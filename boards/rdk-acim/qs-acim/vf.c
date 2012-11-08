//*****************************************************************************
//
// vf.c - V/f control routine.
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
// This is part of revision 9453 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "ui.h"
#include "vf.h"

//*****************************************************************************
//
//! \page vf_intro Introduction
//!
//! In order to maintain a fixed torque over the operating frequency of the
//! motor, the voltage applied to the motor must be varied in proportion to the
//! drive frequency.  This module provides an adjustable V/f curve so that the
//! torque can be held approximately constant across the operating frequency of
//! any given motor.
//!
//! The V/f curve consists of 21 points that provide the amplitude (effectively
//! voltage) based on the drive frequency.  The points are evenly spaced
//! between 0 Hz and either 100 Hz or 400 Hz (based on a configuration value);
//! this provides a point every 5 Hz or 20 Hz.  For frequencies between those
//! in the curve, linear interpolation is used to compute the amplitude.
//!
//! The code for handling the V/f curve is contained in <tt>vf.c</tt>, with
//! <tt>vf.h</tt> containing the definition for the function exported to the
//! remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup vf_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Gets the amplitude based on the frequency.
//!
//! \param ulFrequency is the current motor frequency as a 16.16 fixed point
//! value.
//!
//! This function performs the V/f computation to convert a motor frequency
//! into the amplitude of the waveform.  A V/f table is used to define the
//! mapping of frequency to amplitude; linear interpolation is utilized for
//! frequencies that are not directly defined in the V/f table.
//!
//! \return The amplitude as a 16.16 fixed point value.
//
//*****************************************************************************
unsigned long
VFGetAmplitude(unsigned long ulFrequency)
{
    unsigned long ulRange, ulMin, ulMax;

    //
    // Get the range of the V/f table.
    //
    if(HWREGBITH(&(g_sParameters.usFlags), FLAG_VF_RANGE_BIT) ==
       FLAG_VF_RANGE_100)
    {
        ulRange = 100;
    }
    else
    {
        ulRange = 400;
    }

    //
    // If the input frequency corresponds to a frequency that is beyond the end
    // of the V/f table, simply return the final value of the V/f table.
    //
    if(ulFrequency >= (ulRange * 65536))
    {
        return(g_sParameters.usVFTable[20] * 2);
    }

    //
    // Convert the frequency to the offset into the V/f table of the entry less
    // than or equal to the given frequency.
    //
    ulFrequency = (ulFrequency * 20) / ulRange;

    //
    // Get the two entries from the V/f that surround the distance computed
    // above.
    //
    ulMin = g_sParameters.usVFTable[ulFrequency / 65536] * 2;
    ulMax = g_sParameters.usVFTable[(ulFrequency / 65536) + 1] * 2;

    //
    // Perform linear interpolation between the two V/f entries to get the
    // amplitude for the given frequency.
    //
    return(ulMin + (((ulMax - ulMin) * (ulFrequency & 0xffff)) / 65536));
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
