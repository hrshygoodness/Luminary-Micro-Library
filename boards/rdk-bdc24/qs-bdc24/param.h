//*****************************************************************************
//
// param.h - Prototypes for the motor controller parameter block.
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

#ifndef __PARAM_H__
#define __PARAM_H__

//*****************************************************************************
//
// This structure contains the parameter block for the motor controller.  The
// size of this structure must be FLASH_PB_SIZE bytes; if it is larger then the
// end of the parameter block will not get saved to flash, and if it is smaller
// then whatever appears in SRAM after the parameter block will get saved to
// flash (which could cause problems if the firmware is updated and the
// parameter block in the updated firmware has more parameters and therefore
// tries to use those values).
//
//*****************************************************************************
typedef struct
{
    //
    // The sequence number of this parameter block.  When in RAM, this value is
    // not used.  When in flash, this value is used to determine the parameter
    // block with the most recent information.
    //
    unsigned char ucSequenceNum;

    //
    // The CRC of the parameter block.  When in RAM, this value is not used.
    // When in flash, this value is used to validate the contents of the
    // parameter block (to avoid using a partially written parameter block).
    //
    unsigned char ucCRC;

    //
    // The version of this parameter block.  This can be used to distinguish
    // saved parameters that correspond to an old version of the parameter
    // block.
    //
    unsigned char ucVersion;

    //
    // The current device number for the module.
    //
    unsigned char ucDeviceNumber;

    //
    // The width of the ``negative'' portion of the servo input (between the
    // minimum and neutral).
    //
    unsigned long ulServoNegativeWidth;

    //
    // The width of the servo input that corresponds to neutral.
    //
    unsigned long ulServoNeutralWidth;

    //
    // The width of the ``positive'' portion of the servo input (between the
    // maximum and neutral).
    //
    unsigned long ulServoPositiveWidth;

    //
    // Padding to ensure the whole structure is 64 bytes long.
    //
    unsigned char ucReserved[48];
}
tParameters;

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
extern const unsigned long g_ulFirmwareVersion;
extern unsigned char g_ucHardwareVersion;
extern tParameters g_sParameters;
extern void ParamInit(void);
extern void ParamLoadDefault(void);
extern void ParamLoad(void);
extern void ParamSave(void);

#endif // __PARAM_H__
