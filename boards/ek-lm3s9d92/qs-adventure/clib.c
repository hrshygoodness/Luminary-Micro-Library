//*****************************************************************************
//
// clib.c - C library replacements.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S9D92 Firmware Package.
//
//*****************************************************************************

#ifdef ewarm
#define _NO_DEFINITIONS_IN_HEADER_FILES
#endif
#include "zip/ztypes.h"
#include "common.h"

//*****************************************************************************
//
// The most recently returned random number, used to generate a new random
// number.
//
//*****************************************************************************
static int g_iSeed;

//*****************************************************************************
//
// Sets the seed for the random number generator.
//
//*****************************************************************************
void
srand(unsigned int iSeed)
{
    //
    // Save the seed.
    //
    g_iSeed = iSeed;
}

//*****************************************************************************
//
// Generates a new random number using a pseudo-random number generator.
//
//*****************************************************************************
int
rand(void)
{
    //
    // Generate a new random number and save it as the seed for the next random
    // number.
    //
    g_iSeed = (g_iSeed * 1664525) + 1013904223;

    //
    // Return the new random number.
    //
    return(g_iSeed);
}

//*****************************************************************************
//
// Gets the current time, specified as a number of elapsed seconds.
//
//*****************************************************************************
time_t
time(time_t *ptTimer)
{
    time_t ptRet;

    //
    // Get the current time.
    //
    ptRet = g_ulTime;

    //
    // Save the current time in specified location.
    //
    if(ptTimer)
    {
        *ptTimer = ptRet;
    }

    //
    // Return the current time.
    //
    return(ptRet);
}
