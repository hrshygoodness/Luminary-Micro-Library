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

#ifndef __CTS_STRUCTURE_H__
#define __CTS_STRUCTURE_H__


#define MAXIMUM_NUMBER_OF_ELEMENTS_PER_SENSOR  4
#define TOTAL_NUMBER_OF_ELEMENTS 6


#define ILLEGAL_SLIDER_WHEEL_POSITION		0xFFFFFFFF
#define WHEEL

//******************************************************************************
// The following structure definitons are application independent and are not
// intended to be modified.
//
// The CT_handler 'groups' the sensor based upon function and capacitive 
// measurement method.
//******************************************************************************
typedef struct
{
    unsigned long ulGPIOPort;
    unsigned long ulGPIOPin;
    unsigned long ulThreshold;
    unsigned long ulMaxResponse;
}
tCapTouchElement;

typedef struct tSensor
{
    unsigned char ucNumElements;
    unsigned long ulNumSamples;
    unsigned char ucPoints;
    unsigned long ulSensorThreshold;
    unsigned long ulBaseOffset;
    const tCapTouchElement *Element[MAXIMUM_NUMBER_OF_ELEMENTS_PER_SENSOR];
}
tSensor;

extern const tCapTouchElement g_sVolumeDownElement;     // structure containing elements for
extern const tCapTouchElement g_sVolumeUpElement;     // 
extern const tCapTouchElement g_sRightElement;     // 
extern const tCapTouchElement g_sLeftElement;     // 
extern const tCapTouchElement g_sMiddleElement;     // 
extern const tCapTouchElement g_sProximityElement;     // 

extern const tSensor g_sSensorWheel;

extern const tSensor g_sMiddleButton;    // structure of info for a given  

#endif // __CTS_STRUCTURE_H__
