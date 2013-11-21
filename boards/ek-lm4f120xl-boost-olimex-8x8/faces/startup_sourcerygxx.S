//*****************************************************************************
//
// startup_sourcerygxx.S - Startup code for Sourcery CodeBench.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

    .syntax unified
    .thumb

//*****************************************************************************
//
// Place this code into the .cs3.reset section so that it can be executed prior
// to the startup code in Sourcery CodeBench.
//
//*****************************************************************************
    .section .cs3.reset,"x"

//*****************************************************************************
//
// The reset handler, which overrides the one provided by CS3.
//
//*****************************************************************************
    .globl __cs3_reset
    .thumb_func
__cs3_reset:
    //
    // Enable the floating-point unit.  This must be done here to handle the
    // case where main() uses floating-point and the function prologue saves
    // floating-point registers (which will fault if floating-point is not
    // enabled).  Any configuration of the floating-point unit using DriverLib
    // APIs must be done here prior to the floating-point unit being enabled.
    //
    // Note that this does not use DriverLib since it might not be included in
    // this project.
    //
    movw    r0, #0xed88
    movt    r0, #0xe000
    ldr     r1, [r0]
    orr     r1, #0x00f00000
    str     r1, [r0]

    //
    // Branch to the normal CS3 startup code.
    //
    .extern __cs3_reset_ekc_lm4f232
    ldr     r0, =__cs3_reset_ekc_lm4f232
    bx      r0

    //
    // Place the literal pool here.
    //
    .ltorg

    //
    // The end of the file.
    //
    .end
