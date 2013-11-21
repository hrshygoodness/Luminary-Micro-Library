//*****************************************************************************
//
// main.h - Prototypes for the A/C induction motor drive main application.
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
// This is part of revision 10636 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#ifndef __MAIN_H__
#define __MAIN_H__

//*****************************************************************************
//
//! \addtogroup main_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The frequency of the crystal attached to the microcontroller.  This must
//! match the crystal value passed to SysCtlClockSet() in main().
//
//*****************************************************************************
#define CRYSTAL_CLOCK           6000000

//*****************************************************************************
//
//! The frequency of the processor clock, which is also the clock rate of all
//! the peripherals.  This must match the value configured by SysCtlClockSet()
//! in main().
//
//*****************************************************************************
#define SYSTEM_CLOCK            50000000

//*****************************************************************************
//
//! The address of the first block of flash to be used for storing parameters.
//
//*****************************************************************************
#define FLASH_PB_START          0x0000f000

//*****************************************************************************
//
//! The address of the last block of flash to be used for storing parameters.
//! Since the end of flash is used for parameters, this is actually the first
//! address past the end of flash.
//
//*****************************************************************************
#define FLASH_PB_END            0x00010000

//*****************************************************************************
//
// Close the Doxyen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Prototypes for the exported variables and functions.
//
//*****************************************************************************
extern unsigned long g_ulFaultFlags;
extern unsigned char g_ucMotorStatus;
extern unsigned long g_ulAngle;
extern void MainSetPWMFrequency(void);
extern void MainSetFrequency(void);
extern void MainSetDirection(tBoolean bForward);
extern void MainSetLoopMode(tBoolean bClosed);
extern void MainUpdateFAdjI(long lNewFAdjI);
extern void MainWaveformTick(void);
extern void MainMillisecondTick(void);
extern void MainRun(void);
extern void MainStop(void);
extern void MainEmergencyStop(void);
extern unsigned long MainIsRunning(void);
extern void MainSetFault(unsigned long ulFaultFlag);
extern void MainClearFaults(void);
extern unsigned long MainIsFaulted(void);
extern void NmiSR(void);
extern void FaultISR(void);
extern void IntDefaultHandler(void);

#endif // __MAIN_H__
