//****************************************************************************
//
// simpliciti_config.h - Configuration options for the SimpliciTI low power RF
//                       stack.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//****************************************************************************
#ifndef _SIMPLICITI_CONFIG_H_
#define _SIMPLICITI_CONFIG_H_

//****************************************************************************
//
// Radio configuration.  Define exactly one of the following ISM_xx labels to
// choose the frequency used to control the EvalBot:
//
// ISM_US - 915MHz (for USA)
// ISM_EU - 868MHz (for Europe)
// ISM_LF - 433MHz (for Japan)
//
//****************************************************************************
#define MRFI_CC1101
#define CHRONOS_RADIO_CONFIG
#define ISM_US
//#define ISM_LF
//#define ISM_EU

//*****************************************************************************
//
// Network Configuration Options
//
//*****************************************************************************

//
// Maximum hop count
//
#define MAX_HOPS 3

//
// Maximum hops away from and AP. Keeps hop count and therefore replay
// storms down for sending to and from polling End Devices. Also used
// when joining since the end devices can't be more than 1 hop away.
//
#define MAX_HOPS_FROM_AP 1

//
// Maximum size of Network application payload.  Do not change unless
// protocol changes are reflected in different maximum network application
// payload size.
//
#define MAX_NWK_PAYLOAD 9

//
// Maximum size of application payload.
//
#define MAX_APP_PAYLOAD 19

//
// Default Link token.
//
#define DEFAULT_LINK_TOKEN 0x01020304

//
// Default Join token.
//
#define DEFAULT_JOIN_TOKEN 0x05060708

//
// Define to set Frequency Agility as active for this build.
//
//#define FREQUENCY_AGILITY

//
// Define to enable application autoacknowledge support.  This requires
// extended API support too.
//
#define APP_AUTO_ACK

//
// Define to enable Extended API.
//
#define EXTENDED_API

//
// Define to enable security.
//
//#define SMPL_SECURE

//
// Define to enable NV object support.
//
//#define NVOBJECT_SUPPORT

//
// Define to enable software timer.
//
//#define SW_TIMER

//*****************************************************************************
//
// Device Configuration Options
//
//*****************************************************************************

//
// Number of connections supported.  Each connection supports bi-directional
// communication.  Access Points and Range Extenders can set this to 0 if they
// do not host End Device objects.
//
#define NUM_CONNECTIONS 3

//
// Size of low level queues for sent and received frames.  Affects RAM usage.
//
// AP needs larger input frame queue if it is supporting store-and-forward
// clients because the forwarded messages are held here.  Two is probably
// enough for an End Device
//
#define SIZE_INFRAME_Q 6

// The output frame queue can be small since Tx is done synchronously. Actually
// 1 is probably enough. If an Access Point device is also hosting an End
// Device that sends to a sleeping peer the output queue should be larger - the
// waiting frames in this case are held here. In that case the output frame
// queue should be bigger.
//
#define SIZE_OUTFRAME_Q 2

// This device's address. The first byte is used as a filter on the
// CC1100/CC2500 radios so THE FIRST BYTE MUST NOT BE either 0x00 or 0xFF.
// Also, for these radios on End Devices the first byte should be the least
// significant byte so the filtering is maximally effective.  Otherwise the
// frame has to be processed by the MCU before it is recognized as not intended
// for the device.  APs and REs run in promiscuous mode so the filtering is
// not done.  This macro intializes a static const array of unsigned characters
// of length NET_ADDR_SIZE (found in nwk_types.h).
//
// Note that in Stellaris example applications, the device address is set
// using the least significant 4 bytes of the Ethernet MAC address unless we
// also define USE_FIXED_DEVICE_ADDRESS.  This ensures uniqueness and prevents
// the need to modify this configuration file for each individual device.  In
// this particular example we use a fixed address to ensure that the devices
// can link to the access point.
//
#define THIS_DEVICE_ADDRESS {{0x89, 0x56, 0x34, 0x12}}

//
// Tell the application to use the fixed address provided above rather than
// basing the address on the Ethernet MAC address.
//
//#define USE_FIXED_DEVICE_ADDRESS

//
// Device type
//
#define ACCESS_POINT

//
// In the special case in which the AP is a data hub, the AP will automaically
// listen for a link each time a new device joins the network. This is a special
// case scenario in which all End Device peers are the AP and every ED links
// to the AP. In this scenario the ED must automatically try and link after the
// Join reply.
//
#define AP_IS_DATA_HUB

//
// Default the Join context to on.
//
#define STARTUP_JOINCONTEXT_ON

//
// Store and forward support: number of clients.
//
#define  NUM_STORE_AND_FWD_CLIENTS 3

//
// For polling End Devices we need to specify that they do so.  Uncomment the
// macro definition below if this is a polling device. This field is used
// by the Access Point to know whether to reserve store-and-forward support
// for the polling End Device during the Join exchange.
//
//#define RX_POLLS

//
// For Stellaris implementations, the following label defines which of the two
// EM2 adapter connections the radio daughter board is plugged in to.  It must
// be defined as either 0 or 1 to use the MOD1 or MOD2 connection respectively.
//
#define MOD2_CONNECTION 0

#endif // _SIMPLICITI_CONFIG_H_
