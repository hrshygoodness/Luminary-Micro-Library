//*****************************************************************************
//
// controller.h - Prototypes for the motor controller.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

//*****************************************************************************
//
// The values that indicate the type of communication link that has been
// detected.
//
//*****************************************************************************
#define LINK_TYPE_NONE          0
#define LINK_TYPE_SERVO         1
#define LINK_TYPE_CAN           2
#define LINK_TYPE_UART          3

//*****************************************************************************
//
// Prototypes for the functions exported by the motor controller.
//
//*****************************************************************************
extern void ControllerFaultTimeSet(unsigned long ulTime);
extern unsigned long ControllerFaultTimeGet(void);
extern void ControllerLinkGood(unsigned long ulType);
extern void ControllerLinkLost(unsigned long ulType);
extern unsigned long ControllerLinkType(void);
extern unsigned long ControllerLinkActive(void);
extern void ControllerFaultSignal(unsigned long ulFault);
extern unsigned long ControllerFaultsActive(void);
extern unsigned long ControllerStickyFaultsActive(unsigned long bClear);
extern unsigned char ControllerCurrentFaultsGet(void);
extern unsigned char ControllerTemperatureFaultsGet(void);
extern unsigned char ControllerVBusFaultsGet(void);
extern unsigned char ControllerGateFaultsGet(void);
extern unsigned char ControllerCommunicationFaultsGet(void);
extern void ControllerFaultCountReset(unsigned long ulCounters);
extern void ControllerForceNeutral(void);
extern long ControllerVoltageGet(void);
extern void ControllerVoltageModeSet(unsigned long bEnable);
extern void ControllerVoltageSet(long lVoltage);
extern long ControllerVoltageTargetGet(void);
extern void ControllerVoltageRateSet(unsigned long ulRate);
extern unsigned long ControllerVoltageRateGet(void);
extern void ControllerSpeedModeSet(unsigned long bEnable);
extern void ControllerSpeedSet(long lSpeed);
extern long ControllerSpeedTargetGet(void);
extern long ControllerSpeedGet(void);
extern void ControllerSpeedSrcSet(unsigned long ulSrc);
extern unsigned long ControllerSpeedSrcGet(void);
extern void ControllerSpeedPGainSet(long lPGain);
extern long ControllerSpeedPGainGet(void);
extern void ControllerSpeedIGainSet(long lIGain);
extern long ControllerSpeedIGainGet(void);
extern void ControllerSpeedDGainSet(long lDGain);
extern long ControllerSpeedDGainGet(void);
extern void ControllerVCompModeSet(unsigned long bEnable);
extern void ControllerVCompSet(long lVoltage);
extern long ControllerVCompTargetGet(void);
extern void ControllerVCompInRateSet(unsigned long ulRate);
extern unsigned long ControllerVCompInRateGet(void);
extern void ControllerVCompCompRateSet(unsigned long ulRate);
extern unsigned long ControllerVCompCompRateGet(void);
extern void ControllerPositionModeSet(unsigned long bEnable,
                                      long lStartingPosition);
extern void ControllerPositionSet(long lPosition);
extern long ControllerPositionTargetGet(void);
extern long ControllerPositionGet(void);
extern void ControllerPositionSrcSet(unsigned long ulSrc);
extern unsigned long ControllerPositionSrcGet(void);
extern void ControllerPositionPGainSet(long lPGain);
extern long ControllerPositionPGainGet(void);
extern void ControllerPositionIGainSet(long lIGain);
extern long ControllerPositionIGainGet(void);
extern void ControllerPositionDGainSet(long lDGain);
extern long ControllerPositionDGainGet(void);
extern void ControllerCurrentModeSet(unsigned long bEnable);
extern void ControllerCurrentSet(long lCurrent);
extern long ControllerCurrentTargetGet(void);
extern void ControllerCurrentPGainSet(long lPGain);
extern long ControllerCurrentPGainGet(void);
extern void ControllerCurrentIGainSet(long lIGain);
extern long ControllerCurrentIGainGet(void);
extern void ControllerCurrentDGainSet(long lDGain);
extern long ControllerCurrentDGainGet(void);
extern unsigned long ControllerPowerStatus(void);
extern void ControllerPowerStatusClear(void);
extern void ControllerHaltSet(void);
extern void ControllerHaltClear(void);
extern unsigned long ControllerHalted(void);
extern void ControllerInit(void);
extern void ControllerIntHandler(void);
extern void ControllerWatchdog(unsigned long ulType);
extern unsigned char ControllerControlModeGet(void);

#endif // __CONTROLLER_H__
