//*****************************************************************************
// 
// language.h - header file for a compressed string table.
//
// Copyright (c) 2008-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
// This is an auto-generated file.  Do not edit by hand.
//
//*****************************************************************************

#define SCOMP_MAX_STRLEN         186   // The maximum size of any string.

extern const unsigned char g_pucTablelanguage[];

//*****************************************************************************
//
// SCOMP_STR_INDEX is an enumeration list that is used to select a string
// from the string table using the GrStringGet() function.
//
//*****************************************************************************
enum SCOMP_STR_INDEX
{
    STR_CONFIG,                   
    STR_INTRO,                    
    STR_UPDATE,                   
    STR_LANGUAGE,                 
    STR_INTRO_1,                  
    STR_INTRO_2,                  
    STR_INTRO_3,                  
    STR_UPDATE_TEXT,              
    STR_UPDATING,                 
};
