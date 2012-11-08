//*****************************************************************************
//
// model.h - Prototypes for the wireframe model that is rotated in 3-space.
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#ifndef __MODEL_H__
#define __MODEL_H__

//*****************************************************************************
//
// The number of vertices in the model.
//
//*****************************************************************************
#define NUM_VERTICES            704

//*****************************************************************************
//
// The number of faces in the model.
//
//*****************************************************************************
#define NUM_FACES               12

//*****************************************************************************
//
// The number of lines in each face of the model.
//
//*****************************************************************************
#define NUM_FACE_LINES          5

//*****************************************************************************
//
// The number of lines in the logo on each face of the model.
//
//*****************************************************************************
#define NUM_LOGO_LINES          57

//*****************************************************************************
//
// The arrays that contain the vertices and faces for the model.
//
//*****************************************************************************
extern const long g_pplVertices[NUM_VERTICES][3];
extern const long g_pplFaces[NUM_FACES][NUM_FACE_LINES + 1];
extern const long g_pplLogos[NUM_FACES][NUM_LOGO_LINES + 1];

//*****************************************************************************
//
// The arrays that contain the transformed and projected vertices and faces for
// the model.
//
//*****************************************************************************
extern long g_plIsVisible[NUM_FACES];
extern long g_pplPoints[NUM_VERTICES][2];

#endif // __MODEL_H__
