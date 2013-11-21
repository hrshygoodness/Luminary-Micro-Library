//*****************************************************************************
//
// io.h - Public type definitions and prototypes related to pushbuttons and
//        LEDs on EVALBOT.
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
#ifndef __IO_H__
#define __IO_H__

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
//! \addtogroup io_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! This enumerated type defines either one or both EVALBOT LEDs.  It is used
//! by functions which change the state of the LEDs.
//
//*****************************************************************************
typedef enum
{
    //
    //! Both LEDs will be affected.
    //
    BOTH_LEDS = 0,

    //
    //! LED 1 on the right side of the EVALBOT will be affected.
    //
    LED_1,

    //
    //! LED 2 on the left side of the EVALBOT will be affected.
    //
    LED_2
}
tLED;

//*****************************************************************************
//
//! This enumerated type defines the two user switches on EVALBOT.
//
//*****************************************************************************
typedef enum
{
    //
    //! Switch 1 nearest the front on the right side of EVALBOT.
    //
    BUTTON_1 = 0,

    //
    //! Switch 2 nearest the back on the right side of EVALBOT.
    //
    BUTTON_2
}
tButton;

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
extern void LEDsInit(void);
extern void LED_Off(tLED eLED);
extern void LED_On(tLED eLED);
extern void LED_Toggle(tLED eLED);
extern void PushButtonsInit(void);
extern tBoolean PushButtonGetStatus(tButton eButton);
extern void PushButtonDebouncer(void);
extern tBoolean PushButtonGetDebounced(tButton eButton);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __IO_H__
