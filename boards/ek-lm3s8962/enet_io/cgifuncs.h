//*****************************************************************************
//
// cgifuncs.c - Helper functions related to CGI script parameter parsing.
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
// This is part of revision 9453 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#ifndef __CGIFUNCS_H__
#define __CGIFUNCS_H__

//*****************************************************************************
//
// Prototypes of functions exported by this module.
//
//*****************************************************************************
int FindCGIParameter(const char *pcToFind, char *pcParam[], int iNumParams);
tBoolean IsValidHexDigit(const char cDigit);
unsigned char HexDigit(const char cDigit);
tBoolean DecodeHexEscape(const char *pcEncoded, char *pcDecoded);
unsigned long EncodeFormString(const char *pcDecoded, char *pcEncoded,
                               unsigned long ulLen);
unsigned long DecodeFormString(const  char *pcEncoded, char *pcDecoded,
                               unsigned long ulLen);
tBoolean CheckDecimalParam(const char *pcValue, long *plValue);
long GetCGIParam(const char *pcName, char *pcParams[], char *pcValue[],
                 int iNumParams, tBoolean *pbError);

#endif // __CGIFUNCS_H__
