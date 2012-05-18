//*****************************************************************************
//
// math.c - Fixed-point arithmetic functions.
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

#include "math.h"

//*****************************************************************************
//
// This function takes two fixed-point numbers, in 16.16 format, and multiplies
// them together, returning the 16.16 fixed-point result.  It is the
// responsibility of the caller to ensure that the dynamic range of the integer
// portion of the value is not exceeded; if it is exceeded the result will not
// be correct.
//
//*****************************************************************************
#if defined(ewarm) || defined(DOXYGEN)
long
MathMul16x16(long lX, long lY)
{
    //
    // The assembly code to efficiently perform the multiply (using the
    // instruction to multiply two 32-bit values and return the full 64-bit
    // result).
    //
    __asm("    smull   r0, r1, r0, r1\n"
          "    adds    r0, r0, #0x8000\n"
          "    adc     r1, r1, #0\n"
          "    lsrs    r0, r0, #16\n"
          "    orr     r0, r0, r1, lsl #16");

    //
    // "Warning[Pe940]: missing return statement at end of non-void function"
    // is suppressed here to avoid putting a "bx lr" in the inline assembly
    // above and a superfluous return statement here.
    //
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#endif
#if defined(gcc) || defined(sourcerygxx) || defined(codered)
long __attribute__((naked))
MathMul16x16(long lX, long lY)
{
    unsigned long ulRet;

    //
    // The assembly code to efficiently perform the multiply (using the
    // instruction to multiply two 32-bit values and return the full 64-bit
    // result).
    //
    __asm("    smull   r0, r1, r0, r1\n"
          "    adds    r0, r0, #0x8000\n"
          "    adc     r1, r1, #0\n"
          "    lsrs    r0, r0, #16\n"
          "    orr     r0, r0, r1, lsl #16\n"
          "    bx      lr" : "=r" (ulRet));

    //
    // The return is handled in the inline assembly, but the compiler will
    // still complain if there is not an explicit return here (despite the fact
    // that this does not result in any code being produced because of the
    // naked attribute).
    //
    return(ulRet);
}
#endif
#if defined(rvmdk) || defined(__ARMCC_VERSION)
__asm long
MathMul16x16(long lX, long lY)
{
    //
    // The assembly code to efficiently perform the multiply (using the
    // instruction to multiply two 32-bit values and return the full 64-bit
    // result).
    //
    smull   r0, r1, r0, r1;
    adds    r0, r0, #0x8000
    adc     r1, r1, #0
    lsrs    r0, r0, #16;
    orr     r0, r0, r1, lsl #16;
    bx      lr;
}
#endif
//
// For CCS implement this function in pure assembly.  This prevents the TI
// compiler from doing funny things with the optimizer.
//
#if defined(ccs)
long MathMul16x16(long lX, long lY);
    __asm("    .sect \".text:MathMul16x16\"\n"
          "    .clink\n"
          "    .thumbfunc MathMul16x16\n"
          "    .thumb\n"
          "    .global MathMul16x16\n"
          "MathMul16x16:\n"
          "    smull   r0, r1, r0, r1\n"
          "    adds    r0, r0, #0x8000\n"
          "    adc     r1, r1, #0\n"
          "    lsrs    r0, r0, #16\n"
          "    orr     r0, r0, r1, lsl #16\n"
          "    bx      lr\n");
#endif

//*****************************************************************************
//
// This function takes two fixed-point numbers, in 16.16 format, and divides
// them, returning the 16.16 fixed-point result.  It is the responsibility of
// the caller to ensure that the dynamic range of the integer portion of the
// value is not exceeded; if it is exceeded the result will not be correct.
//
//*****************************************************************************
#if defined(ewarm) || defined(DOXYGEN)
long
MathDiv16x16(long lX, long lY)
{
    //
    // Clear the flag that indicates if the result should be made negative.
    //
    __asm("    movs    r12, #0\n"

          //
          // See if the numerator is negative and negate it if so.
          //
          "    cmp     r0, #0\n"
          "    itt     lt\n"
          "    eorlt   r12, r12, #1\n"
          "    neglt   r0, r0\n"

          //
          // See if the denominator is negative and negate it if so.
          //
          "    cmp     r1, #0\n"
          "    itt     lt\n"
          "    eorlt   r12, r12, #1\n"
          "    neglt   r1, r1\n"

          //
          // See if the numerator is larger than the denominator.  If so,
          // perform a first division to get the integer portion of the result.
          // If not, then set the integer portion of the result to zero.
          //
          "    cmp     r0, r1\n"
          "    ittte   ge\n"
          "    udivge  r2, r0, r1\n"
          "    mulge   r3, r1, r2\n"
          "    subge   r0, r0, r3\n"
          "    movlt   r2, #0\n"

          //
          // Shift the numerator and denominator down such that there are only
          // 16 bits of numerator, and multiply the numerator by 65536.
          //
          "    clz     r3, r0\n"
          "    cmp     r3, #16\n"
          "    itett   lt\n"
          "    lsllt   r0, r0, r3\n"
          "    lslge   r0, r0, #16\n"
          "    rsblt   r3, r3, #16\n"
          "    lsrlt   r1, r1, r3\n"

          //
          // Perform a division to get the fractional portion of the result.
          //
          "    udiv    r3, r0, r1\n"

          //
          // Combine the integer and fractional portion of the result and
          // negate it if required (i.e. if only the numerator or the
          // denominator was negative).
          //
          "    orr     r0, r3, r2, lsl #16\n"
          "    cmp     r12, #1\n"
          "    it      eq\n"
          "    negeq   r0, r0");

    //
    // "Warning[Pe940]: missing return statement at end of non-void function"
    // is suppressed here to avoid putting a "bx lr" in the inline assembly
    // above and a superfluous return statement here.
    //
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#endif
#if defined(gcc) || defined(sourcerygxx) || defined(codered)
long __attribute__((naked))
MathDiv16x16(long lX, long lY)
{
    unsigned long ulRet;

    //
    // Clear the flag that indicates if the result should be made negative.
    //
    __asm("    movs    r12, #0\n"

          //
          // See if the numerator is negative and negate it if so.
          //
          "    cmp     r0, #0\n"
          "    itt     lt\n"
          "    eorlt   r12, r12, #1\n"
          "    neglt   r0, r0\n"

          //
          // See if the denominator is negative and negate it if so.
          //
          "    cmp     r1, #0\n"
          "    itt     lt\n"
          "    eorlt   r12, r12, #1\n"
          "    neglt   r1, r1\n"

          //
          // See if the numerator is larger than the denominator.  If so,
          // perform a first division to get the integer portion of the result.
          // If not, then set the integer portion of the result to zero.
          //
          "    cmp     r0, r1\n"
          "    ittte   ge\n"
          "    udivge  r2, r0, r1\n"
          "    mulge   r3, r1, r2\n"
          "    subge   r0, r0, r3\n"
          "    movlt   r2, #0\n"

          //
          // Shift the numerator and denominator down such that there are only
          // 16 bits of numerator, and multiply the numerator by 65536.
          //
          "    clz     r3, r0\n"
          "    cmp     r3, #16\n"
          "    itett   lt\n"
          "    lsllt   r0, r0, r3\n"
          "    lslge   r0, r0, #16\n"
          "    rsblt   r3, r3, #16\n"
          "    lsrlt   r1, r1, r3\n"

          //
          // Perform a division to get the fractional portion of the result.
          //
          "    udiv    r3, r0, r1\n"

          //
          // Combine the integer and fractional portion of the result and
          // negate it if required (i.e. if only the numerator or the
          // denominator was negative).
          //
          "    orr     r0, r3, r2, lsl #16\n"
          "    cmp     r12, #1\n"
          "    it      eq\n"
          "    negeq   r0, r0\n"

          //
          // Return the result.
          //
          "    bx      lr" : "=r" (ulRet));

    //
    // The return is handled in the inline assembly, but the compiler will
    // still complain if there is not an explicit return here (despite the fact
    // that this does not result in any code being produced because of the
    // naked attribute).
    //
    return(ulRet);
}
#endif
#if defined(rvmdk) || defined(__ARMCC_VERSION)
__asm long
MathDiv16x16(long lX, long lY)
{
    //
    // Clear the flag that indicates if the result should be made negative.
    //
    movs    r12, #0;

    //
    // See if the numerator is negative and negate it if so.
    //
    cmp     r0, #0;
    itt     lt;
    eorlt   r12, r12, #1;
    neglt   r0, r0;

    //
    // See if the denominator is negative and negate it if so.
    //
    cmp     r1, #0;
    itt     lt;
    eorlt   r12, r12, #1;
    neglt   r1, r1;

    //
    // See if the numerator is larger than the denominator.  If so, perform a
    // first division to get the integer portion of the result.  If not, then
    // set the integer portion of the result to zero.
    //
    cmp     r0, r1;
    ittte   ge;
    udivge  r2, r0, r1;
    mulge   r3, r1, r2;
    subge   r0, r0, r3;
    movlt   r2, #0;

    //
    // Shift the numerator and denominator down such that there are only 16
    // bits of numerator, and multiply the numerator by 65536.
    //
    clz     r3, r0;
    cmp     r3, #16;
    itett   lt;
    lsllt   r0, r0, r3;
    lslge   r0, r0, #16;
    rsblt   r3, r3, #16;
    lsrlt   r1, r1, r3;

    //
    // Perform a division to get the fractional portion of the result.
    //
    udiv    r3, r0, r1;

    //
    // Combine the integer and fractional portion of the result and negate it
    // if required (i.e. if only the numerator or the denominator was
    // negative).
    //
    orr     r0, r3, r2, lsl #16;
    cmp     r12, #1;
    it      eq;
    negeq   r0, r0;

    //
    // Return the result.
    //
    bx      lr;
}
#endif
//
// For CCS implement this function in pure assembly.  This prevents the TI
// compiler from doing funny things with the optimizer.
//
#if defined(ccs)
long MathDiv16x16(long lX, long lY);
    __asm("    .sect \".text:MathDiv16x16\"\n"
          "    .clink\n"
          "    .thumbfunc MathDiv16x16\n"
          "    .thumb\n"
          "    .global MathDiv16x16\n"
          "MathDiv16x16:\n"

          //
          // Clear the flag that indicates if the result should be made
          // negative.
          //
          "    movs    r12, #0\n"

          //
          // See if the numerator is negative and negate it if so.
          //
          "    cmp     r0, #0\n"
          "    itt     LT\n"
          "    eorlt   r12, r12, #1\n"
          "    neglt   r0, r0\n"

          //
          // See if the denominator is negative and negate it if so.
          //
          "    cmp     r1, #0\n"
          "    itt     LT\n"
          "    eorlt   r12, r12, #1\n"
          "    neglt   r1, r1\n"

          //
          // See if the numerator is larger than the denominator.  If so,
          // perform a first division to get the integer portion of the result.
          // If not, then set the integer portion of the result to zero.
          //
          "    cmp     r0, r1\n"
          "    ittte   GE\n"
          "    udivge  r2, r0, r1\n"
          "    mulge   r3, r1, r2\n"
          "    subge   r0, r0, r3\n"
          "    movlt   r2, #0\n"

          //
          // Shift the numerator and denominator down such that there are only
          // 16 bits of numerator, and multiply the numerator by 65536.
          //
          "    clz     r3, r0\n"
          "    cmp     r3, #16\n"
          "    itett   LT\n"
          "    lsllt   r0, r0, r3\n"
          "    lslge   r0, r0, #16\n"
          "    rsblt   r3, r3, #16\n"
          "    lsrlt   r1, r1, r3\n"

          //
          // Perform a division to get the fractional portion of the result.
          //
          "    udiv    r3, r0, r1\n"

          //
          // Combine the integer and fractional portion of the result and
          // negate it if required (i.e. if only the numerator or the
          // denominator was negative).
          //
          "    orr     r0, r3, r2, lsl #16\n"
          "    cmp     r12, #1\n"
          "    it      EQ\n"
          "    negeq   r0, r0\n"

          //
          // Return the result.
          //
          "    bx      lr\n");
#endif

