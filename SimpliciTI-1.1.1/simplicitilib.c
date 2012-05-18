//*****************************************************************************
//
// simplicitilib.c - Wrapper file used to add the SimpliciTI low power RF
//                   stack to an application.
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the Stellaris SimpliciTI Library.
//
//*****************************************************************************

//
// The simpliciti_config.h header file is application-specific and includes
// stack configuration settings.
//
#include "simpliciti_config.h"

//
// Include the SimpliciTI stack source.  Note that many of these files are
// dependent upon configuration options found in simpliciti_config.h which are
// used to control, for example, the board support package (BSP) in use and
// the particular radio family to be supported.
//
#include "Components/simpliciti/nwk/nwk.c"
#include "Components/simpliciti/nwk/nwk_api.c"
#include "Components/simpliciti/nwk/nwk_frame.c"
#include "Components/simpliciti/nwk/nwk_globals.c"
#include "Components/simpliciti/nwk/nwk_QMgmt.c"
#include "Components/simpliciti/nwk_applications/nwk_freq.c"
#include "Components/simpliciti/nwk_applications/nwk_ioctl.c"
#include "Components/simpliciti/nwk_applications/nwk_join.c"
#include "Components/simpliciti/nwk_applications/nwk_link.c"
#include "Components/simpliciti/nwk_applications/nwk_mgmt.c"
#include "Components/simpliciti/nwk_applications/nwk_ping.c"
#include "Components/simpliciti/nwk_applications/nwk_security.c"
#include "Components/bsp/bsp.c"
#include "Components/mrfi/mrfi.c"
