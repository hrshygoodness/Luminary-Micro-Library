//*****************************************************************************
//
// sensors.h - Public type definitions and prototypes provided by the sensors
//             module.
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
//*****************************************************************************
#ifndef __SENSORS_H__
#define __SENSORS_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! \addtogroup sensors_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The enumerated type defining bumper left and right.
//
//*****************************************************************************
typedef enum
{
    //
    //! Left bump sensor.
    //
    BUMP_LEFT = 0,

    //
    //! Right bump sensor.
    //
    BUMP_RIGHT
}
tBumper;

//*****************************************************************************
//
//! The enumerated type defining wheel sensor left and right.
//
//*****************************************************************************
typedef enum
{
    //
    //! Left wheel sensor.
    //
    WHEEL_LEFT = 0,

    //
    //! Right wheel sensor.
    //
    WHEEL_RIGHT
}
tWheel;

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void BumpSensorsInit (void);
extern tBoolean BumpSensorGetStatus(tBumper eBumper);
extern void BumpSensorDebouncer(void);
extern tBoolean BumpSensorGetDebounced(tBumper eBumper);
extern void WheelSensorsInit(void (*pfnCallback)(tWheel eWheel));
extern void WheelSensorEnable(void);
extern void WheelSensorDisable(void);
extern void WheelSensorIntEnable(tWheel eWheel);
extern void WheelSensorIntDisable(tWheel eWheel);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SENSORS_H__
