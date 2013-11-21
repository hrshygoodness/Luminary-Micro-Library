//*****************************************************************************
//
// simplicitilib.h - Wrapper file used to pull in all relevant header files
//                   required to use the SimpliciTI low power RF stack.
//
// Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris SimpliciTI Library.
//
//*****************************************************************************

#ifndef __SIMPLICITILIB_H__
#define __SIMPLICITILIB_H__

//
// The simpliciti_config.h header file is application-specific and includes
// stack configuration settings.
//
#include "simpliciti_config.h"

//
// Include the SimpliciTI API, BSP and radio header files.
//
#include "Components/bsp/bsp.h"
#include "Components/bsp/drivers/bsp_leds.h"
#include "Components/bsp/drivers/bsp_buttons.h"
#include "Components/mrfi/mrfi.h"
#include "Components/simpliciti/nwk/nwk_types.h"
#include "Components/simpliciti/nwk/nwk_api.h"
#include "Components/simpliciti/nwk/nwk_app.h"
#include "Components/simpliciti/nwk/nwk.h"

#endif // __SIMPLICITILIB_H__
