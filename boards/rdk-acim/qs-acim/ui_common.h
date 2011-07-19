//*****************************************************************************
//
// ui_common.h - Common definitions for the motor control API.
//
// Copyright (c) 2007-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6852 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#ifndef __UI_COMMON_H__
#define __UI_COMMON_H__

//*****************************************************************************
//
//! \addtogroup ui_common_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! This structure contains a set of variables that describes the properties
//! of a parameter.
//
//*****************************************************************************
typedef struct
{
    //
    //! The ID of this parameter.
    //
    unsigned char ucID;

    //
    //! The size of this parameter in bytes.
    //
    unsigned char ucSize;

    //
    //! The minimum value for this parameter.  If the size of the parameter is
    //! greater than four bytes, then this minimum does not apply.
    //
    unsigned long ulMin;

    //
    //! The maximum value for this parameter.  If the size of the parameter is
    //! greater than four bytes, then this maximum does not apply.
    //
    unsigned long ulMax;

    //
    //! The increment between valid values for this parameter.  If the size of
    //! the parameter is greater than four bytes, then this increment does not
    //! apply.
    //
    unsigned long ulStep;

    //
    //! A pointer to the value of this parameter.
    //
    unsigned char *pucValue;

    //
    //! A pointer to a function that is called when the parameter value is
    //! updated.  If no function needs to be called, then this can be NULL.
    //
    void (*pfnUpdate)(void);
}
tUIParameter;

//*****************************************************************************
//
//! This structure contains a set of variables that describes the properties
//! of a real-time data item.
//
//*****************************************************************************
typedef struct
{
    //
    //! The ID of this real-time data item.
    //
    unsigned char ucID;

    //
    //! The size of this real-time data item in bytes.
    //
    unsigned char ucSize;

    //
    //! A pointer to the value of this real-time data item.
    //
    unsigned char *pucValue;
}
tUIRealTimeData;

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// The type of this target.  This value must be supplied by the application.
//
//*****************************************************************************
extern const unsigned long g_ulUITargetType;

//*****************************************************************************
//
// An array of the parameters supported by this target.  This array must be
// supplied by the application.
//
//*****************************************************************************
extern const tUIParameter g_sUIParameters[];

//*****************************************************************************
//
// The number of parameters supported by this target.  This value must be
// supplied by the application.
//
//*****************************************************************************
extern const unsigned long g_ulUINumParameters;

//*****************************************************************************
//
// An array of the real-time data items supported by this target.  This array
// must be supplied by the application.
//
//*****************************************************************************
extern const tUIRealTimeData g_sUIRealTimeData[];

//*****************************************************************************
//
// The number of real-time data items supported by this target.  This value
// must be supplied by the application.
//
//*****************************************************************************
extern const unsigned long g_ulUINumRealTimeData;

//*****************************************************************************
//
// Starts the motor drive.
//
// This function is called when the motor drive should be started.  This
// function must be supplied by the application.
//
// \return None.
//
//*****************************************************************************
extern void UIRun(void);

//*****************************************************************************
//
// Stops the motor drive.
//
// This function is called when the motor drive should be stopped.  This
// function must be supplied by the application.
//
// \return None.
//
//*****************************************************************************
extern void UIStop(void);

//*****************************************************************************
//
// Emergency stops the motor drive.
//
// This function is called when the motor drive should be emergency stopped.
// This function must be supplied by the application.
//
// \return None.
//
//*****************************************************************************
extern void UIEmergencyStop(void);

//*****************************************************************************
//
// Loads parameters from flash.
//
// This function is called when the parameter set should be loaded from flash.
// This function must be supplied by the application.
//
// \return None.
//
//*****************************************************************************
extern void UIParamLoad(void);

//*****************************************************************************
//
// Saves parameters to flash.
//
// This function is called when the parameter set should be saved to flash.
// This function must be supplied by the application.
//
// \return None.
//
//*****************************************************************************
extern void UIParamSave(void);

//*****************************************************************************
//
// Starts a firmware upgrade.
//
// This function is called when the firmware should be upgraded.  This function
// must be supplied by the application and can not return.
//
// \return None.
//
//*****************************************************************************
extern void UIUpgrade(void);

#endif // __UI_COMMON_H__
