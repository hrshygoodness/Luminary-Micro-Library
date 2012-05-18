//*****************************************************************************
//
// bdc-comm.h - The main control loop definitions.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifndef __BDCCOMM_H__
#define __BDCCOMM_H__

//*****************************************************************************
//
// external definitions.
//
//*****************************************************************************
extern char g_szCOMName[32];
extern unsigned long g_ulID;
extern unsigned long g_ulHeartbeat;
extern unsigned long g_ulBoardStatus;
extern bool g_bBoardStatusActive;
extern bool g_bConnected;
extern bool g_bSynchronousUpdate;
extern double g_dMaxVout;
extern char *g_argv[4];

extern int CmdVoltage(int argc, char *argv[]);
extern int CmdVComp(int argc, char *argv[]);
extern int CmdCurrent(int argc, char *argv[]);
extern int CmdSpeed(int argc, char *argv[]);
extern int CmdPosition(int argc, char *argv[]);
extern int CmdStatus(int argc, char *argv[]);
extern int CmdConfig(int argc, char *argv[]);
extern int CmdPStatus(int argc, char *argv[]);
extern int CmdSystem(int argc, char *argv[]);
extern int CmdUpdate(int argc, char *argv[]);
extern void FindJaguars(void);

#endif
