//*****************************************************************************
//
// eflash.h - Prototypes for the Ethernet boot loader utility..
//
// Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifndef __EFLASH_H__
#define __EFLASH_H__

//*****************************************************************************
//
// Additional include files that are needed by things defined in this
// header file.
//
//*****************************************************************************
#include <stdio.h>

//*****************************************************************************
//
// Global variables and macros used for "printf" output.
//
//*****************************************************************************
extern int g_iOptVerbose;

//
// "printf" macro for normal output, but that should be ommitted with Quiet
// option.
// 
#define QPRINTF(a)                  \
    do                              \
    {                               \
        if(g_iOptVerbose > 0)       \
        {                           \
            printf a;               \
        }                           \
    }                               \
    while(0)

//
// "printf" macro for verbose output.
//
#define VPRINTF(a)                  \
    do                              \
    {                               \
        if(g_iOptVerbose > 1)       \
        {                           \
            printf a;               \
        }                           \
    }                               \
    while(0)

//
// "printf" macro with additional file debug information.
//
#define EPRINTF(a)                                      \
    do                                                  \
    {                                                   \
        printf("ERROR:%s:%d\n", __FILE__, __LINE__);    \
        printf a;                                       \
    }                                                   \
    while(0)

#endif // #ifndef __EFLASH_H__
