//*****************************************************************************
//
// CTS_structure.c - Capacative Sense structures.
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************
 
//*****************************************************************************
//
// 4 elements configured as a wheel.
// 1 element configured as a button. [Center Button] 
// 1 element configured as a proximity sensor.
// This file contains the structure names and the variable assingments.
//
// Revision 1.00 Baseline
// Developed with IAR 5.10.4 [Kickstart] (5.10.4.30168)
//
// 09/24/10 Rev1.00 Pre-Alpha Version. Added "max_cnts" slider variable in 
//                  CT_Handler object.
//
// 09/29/10 0.02    Update naming convention for elements and sensors.
//
// 10/11/10 0.03    Update
//
//******************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "drivers/CTS_structure.h"

//
// Volume down button on PD3
//
const tCapTouchElement g_sVolumeDownElement =
{
    GPIO_PORTA_AHB_BASE,//ulGPIOPort
    GPIO_PIN_7,//ulGPIOPin
    100,//ulThreshold
    2000//ulMaxResponse
};

//
// Left button on PD2
//
const tCapTouchElement g_sLeftElement =
{
    GPIO_PORTA_AHB_BASE,//ulGPIOPort
    GPIO_PIN_6,//ulGPIOPin
    100,//ulThreshold
    2000//ulMaxResponse
};

//
// Right button on PE3
//
const tCapTouchElement g_sRightElement =
{
    GPIO_PORTA_AHB_BASE,//ulGPIOPort
    GPIO_PIN_2,//ulGPIOPin
    100,//ulThreshold
    2000//ulMaxResponse
};

//
// Volume up button on PE2
//
const tCapTouchElement g_sVolumeUpElement =
{
    GPIO_PORTA_AHB_BASE,//ulGPIOPort
    GPIO_PIN_3,//ulGPIOPin
    100,//ulThreshold
    2000//ulMaxResponse
};

//
// Middle button on PE1
//
const tCapTouchElement g_sMiddleElement =
{
    GPIO_PORTA_AHB_BASE,//ulGPIOPort
    GPIO_PIN_4,//ulGPIOPin
    100,//ulThreshold
    1800//ulMaxResponse
};

const tSensor g_sSensorWheel =
{
    4,//unsigned char ucNumElements;
    100,//unsigned long ulNumSamples;
    64,//unsigned char ucPoints;
    75,//unsigned char ucSensorThreshold;
    0,//unsigned long ulBaseOffset;
    {
        &g_sVolumeUpElement, 
        &g_sRightElement,
        &g_sVolumeDownElement,
        &g_sLeftElement
    }//const tCapTouchElement *Element[MAXIMUM_NUMBER_OF_ELEMENTS_PER_SENSOR];
};

const tSensor g_sMiddleButton = 
{
    1,//unsigned char ucNumElements;
    100,//unsigned long ulNumSamples;
    0,//unsigned char ucPoints;
    0,//unsigned char ucSensorThreshold;
    4,//unsigned long ulBaseOffset;
    {
        &g_sMiddleElement
    }//const tCapTouchElement *Element[MAXIMUM_NUMBER_OF_ELEMENTS_PER_SENSOR];
};

