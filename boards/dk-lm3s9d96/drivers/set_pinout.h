//*****************************************************************************
//
// set_pinout.h - Functions related to pinout configuration.
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
// This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#ifndef __SET_PINOUT_H__
#define __SET_PINOUT_H__

//*****************************************************************************
//
// An enum defining the various daughter boards that can be attached to the
// development board.
//
//*****************************************************************************
typedef enum
{
    DAUGHTER_NONE = 0,
    DAUGHTER_SRAM_FLASH = 1,
    DAUGHTER_FPGA = 2,
    DAUGHTER_EM2 = 3,
    DAUGHTER_UNKNOWN = 0xFFFF
}
tDaughterBoard;

//*****************************************************************************
//
// Macro allowing us to pack the fields of a structure.
//
//*****************************************************************************
#if defined(ccs) ||             \
    defined(codered) ||         \
    defined(gcc) ||             \
    defined(rvmdk) ||           \
    defined(__ARMCC_VERSION) || \
    defined(sourcerygxx)
#define PACKEDSTRUCT __attribute__ ((packed))
#elif defined(ewarm)
#define PACKEDSTRUCT
#else
#error Unrecognized COMPILER!
#endif

//*****************************************************************************
//
// This structure represents the data written to the I2S EEPROM on each of the
// daughter boards to identify the installed hardware.
//
//*****************************************************************************
#ifdef ewarm
#pragma pack(1)
#endif

typedef struct
{
    //
    // A simple marker containing "ID".
    //
    unsigned char pucMarker[2];

    //
    // The total length of the ID structure including the marker bytes, this
    // length field and any optional ASCII string.
    //
    unsigned char ucLength;

    //
    // The structure version number.  This will be incremented if the structure
    // content changes.  For now, it will be set to 0.
    //
    unsigned char ucVersion;

    //
    // The ID of the daughter board.  This value matches the appropriate entry
    // in the tDaughterBoard enum.
    //
    unsigned short usBoardID;

    //
    // The revision number of the board.
    //
    unsigned char ucBoardRev;

    //
    // The EPI mode to set for this board.  Valid values are as for EPIModeSet.
    //
    unsigned char ucEPIMode;

    //
    // Bit masks indicating which EPI signals are used by this daughter board.
    // A 1 in bit position n implies that signal EPI0Sn is required and should
    // be configured for EPI use.
    //
    unsigned long ulEPIPins;

    //
    // The desired maximum EPI clock period (governed by COUNT0) in nanoseconds.
    // This must be set such that neither the read (ucReadAccTime) nor write
    // (ucWriteAccTime) access time is greater than 6 times this value.
    //
    unsigned short usRate0nS;

    //
    // The desired maximum EPI clock period (governed by COUNT1) in nanoseconds.
    //
    unsigned short usRate1nS;

    //
    // The device read access time in HB8 or HB16 modes expressed in
    // nanoseconds.  This is used to calculate the number of read wait states
    // used in the EPI configuration.
    //
    unsigned short ucReadAccTime;

    //
    // The device write access time in HB8 or HB16 modes expressed in
    // nanoseconds.  This is used to calculate the number of write wait states
    // used in the EPI configuration.
    //
    unsigned short ucWriteAccTime;

    //
    // The device read cycle time in HB8 or HB16 modes expressed in
    // nanoseconds.
    //
    unsigned short usReadCycleTime;

    //
    // The device write cycle time in HB8 or HB16 modes expressed in
    // nanoseconds.
    //
    unsigned short usWriteCycleTime;

    //
    // The EPI address mapping to use.  Valid values are as for
    // EPIAddressMapSet.
    //
    unsigned char ucAddrMap;

    //
    // The maximum number of EPI clock cycles to wait while an external FIFO
    // ready signal is holding off a transaction or 0 to indicate that the
    // transaction should be held off forever.  This field is ignored if
    // ucEPIMode is set to EPI_MODE_SDRAM.
    //
    unsigned char ucMaxWait;

    //
    // Number of columns for an SDRAM configuration.  Ignored in other modes.
    //
    unsigned short usNumColumns;

    //
    // Number of rows for an SDRAM configuration.  Ignored in other modes.
    //
    unsigned short usNumRows;

    //
    // The device refresh interval in milliseconds for an SDRAM configuration.
    // Ignored in other modes.
    //
    unsigned char ucRefreshInterval;

    //
    // The frame size in EPI clocks.  This field is used only when the EPI mode
    // for the device is EPI_MODE_GENERAL.
    //
    unsigned char ucFrameCount;

    //
    // Non timing-related, mode-dependent EPI configuration parameters as
    // passed to either EPIConfigSDRAMSet(), EPIConfigHB8Set() or
    // EPIConfigNoModeSet().  In mode HB8, the EPI_HB8_WRWAIT_x and
    // EPI_HB8_RDWAIT_x values must not be included in this value since these
    // are calculated based on the access times specified in ucReadAccTime and
    // ucWriteAccTime.
    //
    unsigned long ulConfigFlags;

    //
    // Optional, NULL-terminated ASCII string describing the board.  The actual
    // length of the string is determined by ucLength - offsetof(pucName).
    //
    char pucName[1];
}
PACKEDSTRUCT tDaughterIDInfo;

#ifdef ewarm
#pragma pack()
#endif

//*****************************************************************************
//
// A global variable indicating which of the possible daughter boards is
// currently connected to the development board.
//
//*****************************************************************************
extern tDaughterBoard g_eDaughterType;

//*****************************************************************************
//
// Public function prototypes.
//
//*****************************************************************************
extern void PinoutSet(void);

#endif // __SET_PINOUT_H__
