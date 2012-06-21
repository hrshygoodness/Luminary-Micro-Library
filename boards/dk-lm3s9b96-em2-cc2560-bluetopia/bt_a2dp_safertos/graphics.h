//*****************************************************************************
//
// graphics.h - Variables and functions exported from graphics.c in the
//              Bluetooth A2DP example application.
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
// This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2560-BLUETOPIA Firmware Package.
//
//*****************************************************************************

#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include "grlib/grlib.h"
#include "grlib/widget.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Constants that define the buttons that are supported via the Graphics
// module.  These are passed to the callback function that is registered
// when this module is initialized.
//
//*****************************************************************************
#define BUTTON_PRESS_PLAY           1
#define BUTTON_PRESS_PAUSE          2
#define BUTTON_PRESS_NEXT           4
#define BUTTON_PRESS_BACK           8

//*****************************************************************************
//
// Prototypes for various display initialization functions.
//
//*****************************************************************************
extern void InitializeGraphics(void (*sButtonPressCallback)(unsigned int));

//*****************************************************************************
//
// Prototypes for various display update functions.
//
//*****************************************************************************
extern void ProcessGraphics(void);
extern void UpdateStatusBox(const char *pcString);
extern void OnButtonPress(tWidget *pWidget);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __GRAPHICS_H__
