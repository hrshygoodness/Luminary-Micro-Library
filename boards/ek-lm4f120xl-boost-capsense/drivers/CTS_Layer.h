//*****************************************************************************
//
// CTS_Layer.h - Capacative Sense API Layer Header file definitions.
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
 
#ifndef __CTS_LAYER_H__
#define __CTS_LAYER_H__

#define EVNT            0x01 
#define DOI_MASK        0x02 
#define DOI_INC         0x02  
#define DOI_DEC         0x00  
#define PAST_EVNT       0x04  
#define TRIDOI_VSLOW    0x00   
#define TRIDOI_SLOW     0x10   
#define TRIDOI_MED      0x20  
#define TRIDOI_FAST     0x30  
#define TRADOI_FAST     0x00   
#define TRADOI_MED      0x40   
#define TRADOI_SLOW     0x80  
#define TRADOI_VSLOW    0xC0  



// API Calls
void TI_CAPT_Init_Baseline(const tSensor*);
void TI_CAPT_Update_Baseline(const tSensor*, unsigned char);

void TI_CAPT_Reset_Tracking(void);
void TI_CAPT_Update_Tracking_DOI(unsigned char);
void TI_CAPT_Update_Tracking_Rate(unsigned char);

void TI_CAPT_Raw(const tSensor*, unsigned long*);

void TI_CAPT_Custom(const tSensor *, unsigned long*);

unsigned char TI_CAPT_Button(const tSensor *);
const tCapTouchElement * TI_CAPT_Buttons(const tSensor *);
unsigned long TI_CAPT_Slider(const tSensor*);
unsigned long TI_CAPT_Wheel(const tSensor*);

// Internal Calls
unsigned char Dominant_Element (const tSensor*, unsigned long*);

#endif // __CTS_LAYER_H__
