//****************************************************************************
//
// led_task.c - Control of LEDs on EVALBOT
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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

#include "inc/hw_types.h"
#include "drivers/io.h"
#include "utils/scheduler.h"

//****************************************************************************
//
// This function is the LED "task".  It is called periodically from the
// scheduler in the main application loop.  It toggles the state of both
// LEDs.
//
//****************************************************************************
void
LEDTask(void *pvParam)
{
    //
    // Change the state of both LEDs.
    //
    LED_Toggle(BOTH_LEDS);
}

//****************************************************************************
//
// This function initializes the LED task. It should be called from the
// application initialization code.
//
//****************************************************************************
void
LEDTaskInit(void *pvParam)
{
    //
    // Initialize the board LED driver and turn on one LED.  This will have
    // the effect of the LEDs blinking back and forth, with one on at a time.
    //
    LEDsInit();
    LED_On(LED_1);
}
