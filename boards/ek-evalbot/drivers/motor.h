//*****************************************************************************
//
// motor.h - Public type definitions and prototypes provided by the motor
//           module.
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
#ifndef __MOTOR_H__
#define __MOTOR_H__

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
//! \addtogroup motor_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The enumerated type defining motor drive directions.
//
//*****************************************************************************
typedef enum
{
    //
    //! Run the motor in the forward direction.
    //
    FORWARD = 0,

    //
    //! Run the motor in the reverse direction.
    //
    REVERSE
}
tDirection;

//*****************************************************************************
//
//! The enumerated type defining one of the two EVALBOT motors.
//
//*****************************************************************************
typedef enum
{
    //
    //! Defines the left side motor.
    //
    LEFT_SIDE = 0,

    //
    //! Defines the right side motor.
    //
    RIGHT_SIDE
} tSide;

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
extern void MotorsInit(void);
extern void MotorDir(tSide ucMotor, tDirection eDirection);
extern void MotorRun(tSide ucMotor);
extern void MotorStop(tSide ucMotor);
extern void MotorSpeed(tSide ucMotor, unsigned short usPercent);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __MOTOR_H__
