//*****************************************************************************
//
// telnet.h - Definitions for the telnet command interface.
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
// This is part of revision 10636 of the RDK-S2E Firmware Package.
//
//*****************************************************************************

#ifndef __TELNET_H__
#define __TELNET_H__

//*****************************************************************************
//
// Telnet commands, as defined by RFC854.
//
//*****************************************************************************
#define TELNET_IAC              255
#define TELNET_WILL             251
#define TELNET_WONT             252
#define TELNET_DO               253
#define TELNET_DONT             254
#define TELNET_SE               240
#define TELNET_NOP              241
#define TELNET_DATA_MARK        242
#define TELNET_BREAK            243
#define TELNET_IP               244
#define TELNET_AO               245
#define TELNET_AYT              246
#define TELNET_EC               247
#define TELNET_EL               248
#define TELNET_GA               249
#define TELNET_SB               250

//*****************************************************************************
//
// Telnet options, as defined by RFC856-RFC861.
//
//*****************************************************************************
#define TELNET_OPT_BINARY       0
#define TELNET_OPT_ECHO         1
#define TELNET_OPT_SUPPRESS_GA  3
#define TELNET_OPT_STATUS       5
#define TELNET_OPT_TIMING_MARK  6
#define TELNET_OPT_EXOPL        255

#if CONFIG_RFC2217_ENABLED
//*****************************************************************************
//
// Telnet Com Port Control options, as defined by RFC2217.
//
//*****************************************************************************
#define TELNET_OPT_RFC2217              44

//
// Client to Server Option Defintions
//
#define TELNET_C2S_SIGNATURE            0
#define TELNET_C2S_SET_BAUDRATE         1
#define TELNET_C2S_SET_DATASIZE         2
#define TELNET_C2S_SET_PARITY           3
#define TELNET_C2S_SET_STOPSIZE         4
#define TELNET_C2S_SET_CONTROL          5
#define TELNET_C2S_NOTIFY_LINESTATE     6
#define TELNET_C2S_NOTIFY_MODEMSTATE    7
#define TELNET_C2S_FLOWCONTROL_SUSPEND  8
#define TELNET_C2S_FLOWCONTROL_RESUME   9
#define TELNET_C2S_SET_LINESTATE_MASK   10
#define TELNET_C2S_SET_MODEMSTATE_MASK  11
#define TELNET_C2S_PURGE_DATA           12

//
// Server to Client Option Definitions
//
#define TELNET_S2C_SIGNATURE            (0 + 100)
#define TELNET_S2C_SET_BAUDRATE         (1 + 100)
#define TELNET_S2C_SET_DATASIZE         (2 + 100)
#define TELNET_S2C_SET_PARITY           (3 + 100)
#define TELNET_S2C_SET_STOPSIZE         (4 + 100)
#define TELNET_S2C_SET_CONTROL          (5 + 100)
#define TELNET_S2C_NOTIFY_LINESTATE     (6 + 100)
#define TELNET_S2C_NOTIFY_MODEMSTATE    (7 + 100)
#define TELNET_S2C_FLOWCONTROL_SUSPEND  (8 + 100)
#define TELNET_S2C_FLOWCONTROL_RESUME   (9 + 100)
#define TELNET_S2C_SET_LINESTATE_MASK   (10 + 100)
#define TELNET_S2C_SET_MODEMSTATE_MASK  (11 + 100)
#define TELNET_S2C_PURGE_DATA           (12 + 100)
#endif // CONFIG_RFC2217_ENABLED

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void TelnetInit(void);
extern void TelnetListen(unsigned short usTelnetPort,
                         unsigned long ulSerialPort);
extern void TelnetOpen(unsigned long ulIPAddr,
                       unsigned short usTelnetRemotePort,
                       unsigned short usTelnetLocalPort,
                       unsigned long ulSerialPort);
extern void TelnetClose(unsigned long ulSerialPort);
extern void TelnetHandler(void);
extern unsigned short TelnetGetLocalPort(unsigned long ulSerialPort);
extern unsigned short TelnetGetRemotePort(unsigned long ulSerialPort);
#if CONFIG_RFC2217_ENABLED
extern void TelnetNotifyModemState(unsigned long ulPort,
                                   unsigned char ucModemState);
#endif
extern void TelnetNotifyLinkStatus(tBoolean bLinkStatusUp);
#ifdef ENABLE_WEB_DIAGNOSTICS
extern void TelnetWriteDiagInfo(char *pcBuffer, int iLen,
                                unsigned char ucPort);
#endif
#endif // __TELNET_H__
