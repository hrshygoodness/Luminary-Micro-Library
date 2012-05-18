//*****************************************************************************
//
// aes_iv.c - Example initialization vector setup for AES.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "driverlib/systick.h"

//*****************************************************************************
//
// A counter that is changed each time the IV function is called.
//
//*****************************************************************************
static unsigned g_uWalkCounter; // counter changed on each call

//*****************************************************************************
//
// The last value of the SysTick counter.
//
//*****************************************************************************
static unsigned g_uTime;        // last "time" pulled from SysTick

//*****************************************************************************
//
// A random string which should be unique to the application.
//
//*****************************************************************************
const unsigned char g_ucApplicationUnique[8] =
{
    0x1C,0x70,0xE3,0x45,0x3F,0xF9,0x01,0xDA
};

//*****************************************************************************
//
// Generate an IV (initialization vector) for AES use.
//
// \param ucIV is where the generated initialization vector is stored.
// \param bNewTime determines if the SysTick timer is read or if the previous
// time value is used.
//
// This will generate a new unique IV for AES use. It may be set to
// inject the Systick (timer) time value each time or only once.
//
// \note  There are 4 easy methods to handle the Initialization Vector (IV)
// to be shared by two or more devices:
// 1. You build up one from one side and send to the other side using
//    no encryption or ECB encryption. The other side may validate
//    the IV (e.g. matches a CRC code or something). Then, the new
//    IV is sent in each encrypted message or in certain messages (such
//    as requests).
// 2. You send part of the IV to the other side and pre-agree to the
//    rest as an application unique value. Again, the follow on IVs are
//    normally sent in following messages.
// 3. Using time. After an initial message, a time base is agreed.
//    Then, each following IV represents the time since that base.
//    Either the next IV is sent in messages (and so validated by
//    being within a short time range) or the time is rounded up to
//    units such as seconds, so that the reciever can guess the IV
//    (current seconds count or previous seconds count).
// 4. A message counter is used so that each side knows what the
//    next IV will be (and replay attacks will fail). This only
//    works with reliable communications.
//
// \return None.
//
//*****************************************************************************
void
AESGenerateIV(unsigned char ucIV[16], int bNewTime)
{
    //
    // To make an IV, we need to build up a unique 16 byte value
    // we use 3 components using the method 1 or 2 above:
    // - Current value of SysTick: you need to have it running for this to
    //   work. It is best if this is called after some communications
    //   with something else, so a "random" amount of time has passed
    // - Some application unique string of values.
    // - A counter
    //

    //
    // Determine if the SysTick timer should be read.
    // Note that the SysTick value is 24 bits.
    //
    if(bNewTime)
    {
        g_uTime = SysTickValueGet();
    }

    //
    // Change the value of the counter.  Use a prime number so it does not
    // wrap evenly.
    //
    g_uWalkCounter += 617;

    //
    // Build the initialization vector from the counter, the time, and
    // the unique application ID.
    // Note that if the application ID is known by both sides in the
    // transaction, then only the first half of the initialization vector
    // needs to be transmited from one side to the other.
    //
    ((unsigned*)ucIV)[0] = g_uWalkCounter;
    ((unsigned*)ucIV)[1] = g_uTime;
    ((unsigned*)ucIV)[2] = ((unsigned*)g_ucApplicationUnique)[0];
    ((unsigned*)ucIV)[3] = ((unsigned*)g_ucApplicationUnique)[1];
}

