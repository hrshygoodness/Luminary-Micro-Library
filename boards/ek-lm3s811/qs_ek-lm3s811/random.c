//*****************************************************************************
//
// random.c - Random number generator utilizing MD4 hash function of
//            environmental noise captured via the the potentiometer.
//
// Copyright (c) 2005-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S811 Firmware Package.
//
//*****************************************************************************

#include "random.h"

//*****************************************************************************
//
// The pool of entropy that has been collected.
//
//*****************************************************************************
static unsigned long g_pulEntropy[16];

//*****************************************************************************
//
// The index of the next byte to be added to the entropy pool.
//
//*****************************************************************************
static unsigned long g_ulIndex = 0;

//*****************************************************************************
//
// Add entropy to the pool.
//
//*****************************************************************************
void
RandomAddEntropy(unsigned long ulEntropy)
{
    //
    // Add this byte to the entropy pool.
    //
    ((unsigned char *)g_pulEntropy)[g_ulIndex] = ulEntropy & 0xff;

    //
    // Increment to the next byte of the entropy pool.
    //
    g_ulIndex = (g_ulIndex + 1) & 63;
}

//*****************************************************************************
//
// Generate a random number by running a MD4 hash on the entropy pool.  Note
// that the entropy pool may change from beneath us, but for the purposes of
// generating random numbers that is not a concern.  Also, the MD4 hash was
// broken long ago, but since it is being used to generate random numbers
// instead of providing security this is not a concern.
//
//*****************************************************************************
unsigned long
RandomNumber(void)
{
    unsigned long ulA, ulB, ulC, ulD, ulTemp, ulIdx;

    //
    // Initialize the digest.
    //
    ulA = 0x67452301;
    ulB = 0xefcdab89;
    ulC = 0x98badcfe;
    ulD = 0x10325476;

    //
    // Perform the first round of operations.
    //
#define F(a, b, c, d, k, s)                                 \
    {                                                       \
        ulTemp = a + (d ^ (b & (c ^ d))) + g_pulEntropy[k]; \
        a = (ulTemp << s) | (ulTemp >> (32 - s));           \
    }
    for(ulIdx = 0; ulIdx < 16; ulIdx += 4)
    {
        F(ulA, ulB, ulC, ulD, ulIdx + 0, 3);
        F(ulD, ulA, ulB, ulC, ulIdx + 1, 7);
        F(ulC, ulD, ulA, ulB, ulIdx + 2, 11);
        F(ulB, ulC, ulD, ulA, ulIdx + 3, 19);
    }

    //
    // Perform the second round of operations.
    //
#define G(a, b, c, d, k, s)                                            \
    {                                                                  \
        ulTemp = a + ((b & c) | (b & d) | (c & d)) + g_pulEntropy[k] + \
                 0x5a827999;                                           \
        a = (ulTemp << s) | (ulTemp >> (32 - s));                      \
    }
    for(ulIdx = 0; ulIdx < 4; ulIdx++)
    {
        G(ulA, ulB, ulC, ulD, ulIdx + 0, 3);
        G(ulD, ulA, ulB, ulC, ulIdx + 4, 5);
        G(ulC, ulD, ulA, ulB, ulIdx + 8, 9);
        G(ulB, ulC, ulD, ulA, ulIdx + 12, 13);
    }

    //
    // Perform the third round of operations.
    //
#define H(a, b, c, d, k, s)                                      \
    {                                                            \
        ulTemp = a + (b ^ c ^ d) + g_pulEntropy[k] + 0x6ed9eba1; \
        a = (ulTemp << s) | (ulTemp >> (32 - s));                \
    }
    for(ulIdx = 0; ulIdx < 4; ulIdx += 2)
    {
        H(ulA, ulB, ulC, ulD, ulIdx + 0, 3);
        H(ulD, ulA, ulB, ulC, ulIdx + 8, 9);
        H(ulC, ulD, ulA, ulB, ulIdx + 4, 11);
        H(ulB, ulC, ulD, ulA, ulIdx + 12, 15);

        if(ulIdx == 2)
        {
            ulIdx -= 3;
        }
    }

    //
    // Return the first word of the resulting digest as the random number.
    //
    return(ulA + 0x67452301);
}
