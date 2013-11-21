//*****************************************************************************
//
// enet.h - Ethernet Definitions/Prototypes.
//
// Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#ifndef __ENET_H__
#define __ENET_H__

//*****************************************************************************
//
// Default TCP/IP Address Configuration (Link Local Address).
//
//*****************************************************************************
//
// The Default IP address to be used.
//
#ifndef DEFAULT_IPADDR0
#define DEFAULT_IPADDR0 169
#endif

#ifndef DEFAULT_IPADDR1
#define DEFAULT_IPADDR1 254
#endif

#ifndef DEFAULT_IPADDR2
#define DEFAULT_IPADDR2 19
#endif

#ifndef DEFAULT_IPADDR3
#define DEFAULT_IPADDR3 63
#endif

//
// The Default Gateway address to be used.
//
#ifndef DEFAULT_GATEWAY_ADDR0
#define DEFAULT_GATEWAY_ADDR0 169
#endif

#ifndef DEFAULT_GATEWAY_ADDR1
#define DEFAULT_GATEWAY_ADDR1 254
#endif

#ifndef DEFAULT_GATEWAY_ADDR2
#define DEFAULT_GATEWAY_ADDR2 0
#endif

#ifndef DEFAULT_GATEWAY_ADDR3
#define DEFAULT_GATEWAY_ADDR3 1
#endif

//
// The Default Network mask to be used.
//
#ifndef DEFAULT_NET_MASK0
#define DEFAULT_NET_MASK0 255
#endif

#ifndef DEFAULT_NET_MASK1
#define DEFAULT_NET_MASK1 255
#endif

#ifndef DEFAULT_NET_MASK2
#define DEFAULT_NET_MASK2 0
#endif

#ifndef DEFAULT_NET_MASK3
#define DEFAULT_NET_MASK3 0
#endif

//*****************************************************************************
//
// External References.
//
//*****************************************************************************
extern void EnetInit(void);
extern void EnetTick(unsigned long ulTickMS);
extern unsigned long EnetGetIPAddr(void);

#endif // __ENET_H__
