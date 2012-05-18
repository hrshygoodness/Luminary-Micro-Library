//*****************************************************************************
//
// param.c - Handles the parameter block for the motor controller.
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

#include "inc/hw_types.h"
#include "utils/flash_pb.h"
#include "constants.h"
#include "param.h"

//*****************************************************************************
//
// The firmware version.
//
//*****************************************************************************
const unsigned long g_ulFirmwareVersion = 8555;

//*****************************************************************************
//
// The hardware version.
//
//*****************************************************************************
unsigned char g_ucHardwareVersion = 0;

//*****************************************************************************
//
// The default parameters for the motor controller.  These will be used if
// there is not a parameter block stored in flash, or if the button is pressed
// when the motor controller is powered on.
//
//*****************************************************************************
static const tParameters g_sParametersDefault =
{
    //
    // The sequence number (ucSequenceNum); this value is not important for
    // the copy in SRAM.
    //
    0,

    //
    // The CRC (ucCRC); this value is not important for the copy in SRAM.
    //
    0,

    //
    // The parameter block version number (ucVersion).
    //
    1,

    //
    // Default device number is 1.
    //
    1,

    //
    // The default servo negative width.
    //
    SERVO_DEFAULT_NEU_WIDTH - SERVO_DEFAULT_MIN_WIDTH,

    //
    // The default servo neutral width.
    //
    SERVO_DEFAULT_NEU_WIDTH,

    //
    // The default servo positive width.
    //
    SERVO_DEFAULT_MAX_WIDTH - SERVO_DEFAULT_NEU_WIDTH
};

//*****************************************************************************
//
// The current parameters for the motor controller.
//
//*****************************************************************************
tParameters g_sParameters;

//*****************************************************************************
//
// This function will restore the default values to the motor controller
// parameters.
//
//*****************************************************************************
void
ParamLoadDefault(void)
{
    //
    // Copy the default parameter block to the active parameter block.
    //
    g_sParameters = g_sParametersDefault;
}

//*****************************************************************************
//
// This function reads the saved motor controller parameters from flash, if
// available.
//
//*****************************************************************************
void
ParamLoad(void)
{
    unsigned char *pucBuffer;

    //
    // Get a pointer to the latest parameter block in flash.
    //
    pucBuffer = FlashPBGet();

    //
    // See if a parameter block was found in flash.
    //
    if(pucBuffer)
    {
        //
        // A parameter block was found so copy the contents to the current
        // parameter block.
        //
        g_sParameters = *(tParameters *)pucBuffer;
    }
}

//*****************************************************************************
//
// This function saves the motor controller parameters to flash, preserving
// them across any subsequent power cycles of the controller.
//
//*****************************************************************************
void
ParamSave(void)
{
    //
    // Save the current parameter block to flash.
    //
    FlashPBSave((unsigned char *)&g_sParameters);
}

//*****************************************************************************
//
// This function initializes the parameter block.  If there is a parameter
// block stored in flash, then those values will be used.  Otherwise, the
// default parameter values will be used.
//
//*****************************************************************************
void
ParamInit(void)
{
    //
    // Initialize the flash parameter block driver.
    //
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, FLASH_PB_SIZE);

    //
    // First, load the parameter block with the default values.
    //
    ParamLoadDefault();

    //
    // Then, if available, load the latest non-volatile set of values.
    //
    ParamLoad();
}
