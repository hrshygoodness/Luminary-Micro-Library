//*****************************************************************************
//
// encoder.c - Controls the operation of the quadrature encoder.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/qei.h"
#include "driverlib/rom.h"
#include "driverlib/systick.h"
#include "constants.h"
#include "encoder.h"
#include "math.h"
#include "pins.h"

//*****************************************************************************
//
// The number of lines in the attached quadrature encoder.
//
//*****************************************************************************
static unsigned long g_ulEncoderLines = 0;

//*****************************************************************************
//
// The time at which the previous encoder edge occurred.
//
//*****************************************************************************
static unsigned long g_ulEncoderPrevious;

//*****************************************************************************
//
// The number of system clocks between edges from the encoder.
//
//*****************************************************************************
static unsigned long g_ulEncoderClocks;

//*****************************************************************************
//
// The number of ticks until the encoder (and therefore motor) is assumed to
// have stopped.
//
//*****************************************************************************
static unsigned short g_usEncoderCount;

//*****************************************************************************
//
// A set of flags that track the state of the encoder stop tracking.
//
//*****************************************************************************
#define ENCODER_FLAG_VALID      0
#define ENCODER_FLAG_EDGE       1
#define ENCODER_FLAG_PREVIOUS   2
static unsigned short g_usEncoderFlags;

//*****************************************************************************
//
// This function prepares the quadrature encoder module for capturing the
// position and speed of the motor.
//
//*****************************************************************************
void
EncoderInit(void)
{
    //
    // Configure the QEI pins.
    //
    ROM_GPIOPinTypeQEI(QEI_PHA_PORT, QEI_PHA_PIN);
    ROM_GPIOPinTypeQEI(QEI_PHB_PORT, QEI_PHB_PIN);
    ROM_GPIOPinTypeQEI(QEI_INDEX_PORT, QEI_INDEX_PIN);

    //
    // Configure the QEI module.
    //
    ROM_QEIConfigure(QEI0_BASE,
                     (QEI_CONFIG_RESET_IDX | QEI_CONFIG_CAPTURE_A |
                      QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), 0xffffffff);

    //
    // Initialize the QEI position to zero.
    //
    ROM_QEIPositionSet(QEI0_BASE, 0);

    //
    // Enable the QEI module.
    //
    ROM_QEIEnable(QEI0_BASE);

    //
    // Configure the encoder input to generate an interrupt on every rising
    // edge.
    //
    ROM_GPIOIntTypeSet(QEI_PHA_PORT, QEI_PHA_PIN, GPIO_RISING_EDGE);
    ROM_GPIOPinIntEnable(QEI_PHA_PORT, QEI_PHA_PIN);
    ROM_IntEnable(QEI_PHA_INT);
}

//*****************************************************************************
//
// This function is called periodically to determine when the encoder has
// stopped rotating (based on too much time passing between edges).
//
//*****************************************************************************
void
EncoderTick(void)
{
    //
    // See if an edge has been seen since the last call.
    //
    if(HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_EDGE) == 1)
    {
        //
        // Clear the edge flag.
        //
        HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_EDGE) = 0;

        //
        // Reset the delay counter.
        //
        g_usEncoderCount = ENCODER_WAIT_TIME;
    }

    //
    // Otherwise, see if the delay counter is still active.
    //
    else if(g_usEncoderCount != 0)
    {
        //
        // Decrement the delay counter.
        //
        g_usEncoderCount--;

        //
        // If the delay counter has reached zero, then indicate that there are
        // no valid speed values from the encoder.
        //
        if(g_usEncoderCount == 0)
        {
            HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_PREVIOUS) = 0;
            HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_VALID) = 0;
        }
    }
}

//*****************************************************************************
//
// This function is called to handle the GPIO edge interrupt from the
// quadrature encoder.
//
//*****************************************************************************
void
EncoderIntHandler(void)
{
    unsigned long ulNow;

    //
    // Save the time.
    //
    ulNow = ROM_SysTickValueGet();

    //
    // Clear the encoder interrupt.
    //
    ROM_GPIOPinIntClear(QEI_PHA_PORT, QEI_PHA_PIN);

    //
    // Determine the number of system clocks between the previous edge and this
    // edge.
    //
    if(g_ulEncoderPrevious > ulNow)
    {
        g_ulEncoderClocks = g_ulEncoderPrevious - ulNow;
    }
    else
    {
        g_ulEncoderClocks = (16777216 - ulNow) + g_ulEncoderPrevious;
    }

    //
    // Save the time of the current edge as the time of the previous edge.
    //
    g_ulEncoderPrevious = ulNow;

    //
    // Indicate that an edge has been seen.
    //
    HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_EDGE) = 1;

    //
    // If the previous edge time was valid, then indicate that the time between
    // edges is also now valid.
    //
    if(HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_PREVIOUS) == 1)
    {
        HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_VALID) = 1;
    }

    //
    // Indicate that the previous edge time is valid.
    //
    HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_PREVIOUS) = 1;
}

//*****************************************************************************
//
// This function sets the number of lines in the attached encoder.
//
//*****************************************************************************
void
EncoderLinesSet(unsigned long ulLines)
{
    //
    // Save the number of lines in the encoder.
    //
    g_ulEncoderLines = ulLines;
}

//*****************************************************************************
//
// This function gets the number of lines in the attached encoder.
//
//*****************************************************************************
unsigned long
EncoderLinesGet(void)
{
    //
    // Return the number of lines in the encoder.
    //
    return(g_ulEncoderLines);
}

//*****************************************************************************
//
// This function ``sets'' the position of the encoder.  This is the position
// against which all further movements of the encoder are measured in a
// relative sense.
//
//*****************************************************************************
void
EncoderPositionSet(long lPosition)
{
    //
    // Convert the position into the number of encoder lines.
    //
    lPosition = MathMul16x16(lPosition, g_ulEncoderLines * 2);

    //
    // Set the encoder position in the quadrature encoder module.
    //
    ROM_QEIPositionSet(QEI0_BASE, lPosition);
}

//*****************************************************************************
//
// Gets the current position of the encoder, specified as a signed 16.16 fixed-
// point value that represents a number of full revolutions.
//
//*****************************************************************************
long
EncoderPositionGet(void)
{
    //
    // Convert the encoder position to a number of revolutions and return it.
    //
    return(MathDiv16x16(ROM_QEIPositionGet(QEI0_BASE), g_ulEncoderLines * 2));
}

//*****************************************************************************
//
// Gets the current speed of the encoder, specified as an unsigned 16.16 fixed-
// point value that represents the speed in revolutions per minute (RPM).
//
//*****************************************************************************
long
EncoderVelocityGet(long lSigned)
{
    long lDir;

    //
    // If the time between edges is not valid, then the speed is zero.
    //
    if(HWREGBITH(&g_usEncoderFlags, ENCODER_FLAG_VALID) == 0)
    {
        return(0);
    }

    //
    // See if a signed velocity should be returned.
    //
    if(lSigned)
    {
        //
        // Get the current direction from the QEI module.
        //
        lDir = ROM_QEIDirectionGet(QEI0_BASE);
    }
    else
    {
        //
        // An unsigned velocity is requested, so assume the direction is
        // positive.
        //
        lDir = 1;
    }

    //
    // Convert the time between edges into a speed in RPM and return it.
    //
    return(MathDiv16x16(lDir * SYSCLK * 60,
                        g_ulEncoderClocks * g_ulEncoderLines));
}
