//*****************************************************************************
//
// analog.h - Prototypes for the analog input driver.
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

#ifndef __ANALOG_H__
#define __ANALOG_H__

//*****************************************************************************
//
// A prototype for the callback function utilized by the analog input driver.
//
//*****************************************************************************
typedef void (tAnalogCallback)(unsigned long ulChannel);

//*****************************************************************************
//
// Prototypes for the APIs provided by the analog input driver.
//
//*****************************************************************************
extern short g_psAnalogValues[4];
extern void AnalogIntHandler(void);
extern void AnalogInit(void);
extern void AnalogLevelSet(unsigned long ulChannel, unsigned short usLevel,
                           char cHysteresis);
extern void AnalogCallbackSetAbove(unsigned long ulChannel,
                                   tAnalogCallback *pfnOnAbove);
extern void AnalogCallbackSetBelow(unsigned long ulChannel,
                                   tAnalogCallback *pfnOnBelow);
extern void AnalogCallbackSetRisingEdge(unsigned long ulChannel,
                                        tAnalogCallback *pfnOnRisingEdge);
extern void AnalogCallbackSetFallingEdge(unsigned long ulChannel,
                                         tAnalogCallback *pfnOnFallingEdge);

#endif // __ANALOG_H__
