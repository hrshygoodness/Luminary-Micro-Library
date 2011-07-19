//*****************************************************************************
//
// aes_config_opts.h - Application specific configuration options for AES.
//
// Copyright (c) 2009-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __AES_CONFIG_OPTS_H__
#define __AES_CONFIG_OPTS_H__

//*****************************************************************************
//
// Define any exceptions to the default configuration below.
// Refer to the file third_party/aes/aes_config.h for a list of
// configuration options.
//
//*****************************************************************************

//
// Build the application to use the AES tables from ROM.  This makes the
// overall program size smaller.
//
#define TABLE_IN_ROM 1

#endif
