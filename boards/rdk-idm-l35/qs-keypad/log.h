//*****************************************************************************
//
// log.h - Prototypes for the UART logging functions.
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
// This is part of revision 8555 of the RDK-IDM-L35 Firmware Package.
//
//*****************************************************************************

#ifndef __LOG_H__
#define __LOG_H__

//*****************************************************************************
//
// Prototypes for the functions provided by the UART logger.
//
//*****************************************************************************
extern void LogIntHandler(void);
extern void LogWrite(char *pcPtr);
extern void LogInit(void);
extern void LogProcessCommands(void);

#endif // __LOG_H__
