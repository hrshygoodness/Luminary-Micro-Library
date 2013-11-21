//*****************************************************************************
//
// pid.c - PID feedback control algorithm.
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include "pid.h"

//*****************************************************************************
//
// This function will initialize the internal state of the PID algorithm.  This
// must be done before the PID algorithm can be executed or random results will
// occur.
//
// By using a derivative gain of zero, this reduces to a simple PI controller.
// By using a integral and derivative gain of zero, it reduces to an even
// simpler P controller.  The response requirements of the process being
// controlled determines the terms required to achieve that level of response.
// Controlling motors can typically be done with a simple PI controller.
//
//*****************************************************************************
void
PIDInitialize(tPIDState *psState, long lIntegMax, long lIntegMin, long lPGain,
              long lIGain, long lDGain)
{
    //
    // Set the internal state, and save the integrator limits and gain factors.
    //
    psState->lIntegrator = 0;
    psState->lIntegMax = lIntegMax;
    psState->lIntegMin = lIntegMin;
    psState->lPrevError = 0;
    psState->lPGain = lPGain;
    psState->lIGain = lIGain;
    psState->lDGain = lDGain;
}

//*****************************************************************************
//
// This function will change the P gain factor used by the PID algorithm.
//
//*****************************************************************************
void
PIDGainPSet(tPIDState *psState, long lPGain)
{
    //
    // Save the P gain factor.
    //
    psState->lPGain = lPGain;
}

//*****************************************************************************
//
// This function will change the I gain factor used by the PID algorithm.
//
//*****************************************************************************
void
PIDGainISet(tPIDState *psState, long lIGain, long lIntegMax, long lIntegMin)
{
    //
    // Save the I gain factor.
    //
    psState->lIGain = lIGain;

    //
    // Save the integrator limits.
    //
    psState->lIntegMax = lIntegMax;
    psState->lIntegMin = lIntegMin;

    //
    // Limit the integrator to the new limits if necessary.
    //
    if(psState->lIntegrator > lIntegMax)
    {
        psState->lIntegrator = lIntegMax;
    }
    else if(psState->lIntegrator < lIntegMin)
    {
        psState->lIntegrator = lIntegMin;
    }
}

//*****************************************************************************
//
// This function will change the D gain factor used by the PID algorithm.
//
//*****************************************************************************
void
PIDGainDSet(tPIDState *psState, long lDGain)
{
    //
    // Save the D gain factor.
    //
    psState->lDGain = lDGain;
}

//*****************************************************************************
//
// This function resets the internal state of the PID controller, preparing it
// to start operating on a new stream of input values.
//
//*****************************************************************************
void
PIDReset(tPIDState *psState)
{
    //
    // Reset the integrator and previous error.
    //
    psState->lIntegrator = 0;
    psState->lPrevError = 0;
}

//*****************************************************************************
//
// This function will execute another iteration of the PID algorithm.  In
// order to get reliable results from this, the sampled values passed in must
// be captured at fixed intervals (as close as possible).  Deviations from a
// fixed capture interval will result in errors in the control output.
//
//*****************************************************************************
long
PIDUpdate(tPIDState *psState, long lError)
{
    long long llOutput;
    long lOutput;

    //
    // Update the error integrator.
    //
    if((psState->lIntegrator & 0x80000000) == (lError & 0x80000000))
    {
        //
        // Add the error to the integrator.
        //
        psState->lIntegrator += lError;

        //
        // Since the sign of the integrator and error matched before the above
        // addition, if the signs no longer match it is because the integrator
        // rolled over.  In this case, saturate appropriately.
        //
        if((lError < 0) && (psState->lIntegrator > 0))
        {
            psState->lIntegrator = psState->lIntegMin;
        }
        if((lError > 0) && (psState->lIntegrator < 0))
        {
            psState->lIntegrator = psState->lIntegMax;
        }
    }
    else
    {
        //
        // Add the error to the integrator.
        //
        psState->lIntegrator += lError;
    }

    //
    // Saturate the integrator if necessary.
    //
    if(psState->lIntegrator > psState->lIntegMax)
    {
        psState->lIntegrator = psState->lIntegMax;
    }
    if(psState->lIntegrator < psState->lIntegMin)
    {
        psState->lIntegrator = psState->lIntegMin;
    }

    //
    // Compute the new control value.
    //
    llOutput = (((long long)psState->lPGain * (long long)lError) +
                ((long long)psState->lIGain *
                 (long long)psState->lIntegrator) +
                ((long long)psState->lDGain *
                 (long long)(lError - psState->lPrevError)));

    //
    // Clip the new control value as appropriate.
    //
    if(llOutput > (long long)0x7fffffffffff)
    {
        lOutput = 0x7fffffff;
    }
    else if(llOutput < (long long)0xffff800000000000)
    {
        lOutput = 0x80000000;
    }
    else
    {
        lOutput = (llOutput >> 16) & 0xffffffff;
    }

    //
    // Save the current error for computing the derivative on the next
    // iteration.
    //
    psState->lPrevError = lError;

    //
    // Return the control value.
    //
    return(lOutput);
}
