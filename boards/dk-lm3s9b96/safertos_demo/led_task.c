//*****************************************************************************
//
// led_task.c - A simple flashing LED task.
//
// Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "SafeRTOS/SafeRTOS_API.h"
#include "idle_task.h"
#include "led_task.h"
#include "priorities.h"

//*****************************************************************************
//
// The stack for the LED toggle task.
//
//*****************************************************************************
static unsigned long g_pulLEDTaskStack[128];

//*****************************************************************************
//
// The amount of time to delay between toggles of the LED.
//
//*****************************************************************************
unsigned long g_ulLEDDelay = 500;

//*****************************************************************************
//
// This task simply toggles the user LED at a 1 Hz rate.
//
//*****************************************************************************
static void
LEDTask(void *pvParameters)
{
    portTickType ulLastTime;

    //
    // Get the current tick count.
    //
    ulLastTime = xTaskGetTickCount();

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Turn on the user LED.
        //
        ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);

        //
        // Wait for the required amount of time.
        //
        xTaskDelayUntil(&ulLastTime, g_ulLEDDelay);

        //
        // Turn off the user LED.
        //
        ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);

        //
        // Wait for the required amount of time.
        //
        xTaskDelayUntil(&ulLastTime, g_ulLEDDelay);
    }
}

//*****************************************************************************
//
// Initializes the LED task.
//
//*****************************************************************************
unsigned long
LEDTaskInit(void)
{
    //
    // Initialize the GPIO used to drive the user LED.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

    //
    // Create the LED task.
    //
    if(xTaskCreate(LEDTask, (signed portCHAR *)"LED",
                   (signed portCHAR *)g_pulLEDTaskStack,
                   sizeof(g_pulLEDTaskStack), NULL, PRIORITY_LED_TASK,
                   NULL) != pdPASS)
    {
        return(1);
    }
    TaskCreated();

    //
    // Success.
    //
    return(0);
}
