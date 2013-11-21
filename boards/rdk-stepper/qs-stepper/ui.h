//*****************************************************************************
//
// ui.h - Prototypes for the user interface.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#ifndef __UI_H__
#define __UI_H__

//*****************************************************************************
//
//! \addtogroup ui_api
//! @{
//
//*****************************************************************************


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Prototypes for the globals exported from the user interface.
//
//*****************************************************************************
extern void UIInit(void);
extern void UIRunInterface(void);
extern void SysTickIntHandler(void);
extern void UISetMotion(void);
extern void UISetChopperBlanking(void);
extern void UISetMotorParms(void);
extern void UISetControlMode(void);
extern void UISetDecayMode(void);
extern void UISetStepMode(void);
extern void UISetFixedOnTime(void);
extern void UISetPWMFreq(void);
extern void UIClearFaults(void);
extern void UISetFaultParms(void);
extern void UIOnBoard(void);

#endif // __UI_H__
