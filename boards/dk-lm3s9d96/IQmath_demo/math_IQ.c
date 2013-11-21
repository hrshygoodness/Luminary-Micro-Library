//*****************************************************************************
//
// math_IQ.c - Math for doing 3-space rotations and projections, using IQmath.
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
// This is part of revision 10636 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "IQmath/IQmathLib.h"
#include "math_IQ.h"
#include "model.h"

//*****************************************************************************
//
// An array of the transformed model vertices.
//
//*****************************************************************************
static _iq16 g_ppiqVertices[NUM_VERTICES][3];

//*****************************************************************************
//
// Rotates a point around the X axis, using IQmath.
//
//*****************************************************************************
void
IQRotateX(_iq16 *piqPoint, _iq16 iqX)
{
    _iq16 iqSin, iqCos, iqTemp;

    //
    // Compute the sin and cos of the rotation angle.
    //
    iqSin = _IQ16sinPU(_IQ16div(iqX, _IQ16(360)));
    iqCos = _IQ16cosPU(_IQ16div(iqX, _IQ16(360)));

    //
    // Rotate this point around the X axis.
    //
    iqTemp = _IQ16mpy(iqSin, piqPoint[1]) + _IQ16mpy(iqCos, piqPoint[2]);
    piqPoint[1] = _IQ16mpy(iqCos, piqPoint[1]) - _IQ16mpy(iqSin, piqPoint[2]);
    piqPoint[2] = iqTemp;
}

//*****************************************************************************
//
// Rotates a point around the Y axis, using IQmath.
//
//*****************************************************************************
void
IQRotateY(_iq16 *piqPoint, _iq16 iqY)
{
    _iq16 iqSin, iqCos, iqTemp;

    //
    // Compute the sin and cos of the rotation angle.
    //
    iqSin = _IQ16sinPU(_IQ16div(iqY, _IQ16(360)));
    iqCos = _IQ16cosPU(_IQ16div(iqY, _IQ16(360)));

    //
    // Rotate this point around the Y axis.
    //
    iqTemp = _IQ16mpy(iqSin, piqPoint[0]) + _IQ16mpy(iqCos, piqPoint[2]);
    piqPoint[0] = _IQ16mpy(iqCos, piqPoint[0]) - _IQ16mpy(iqSin, piqPoint[2]);
    piqPoint[2] = iqTemp;
}

//*****************************************************************************
//
// Rotates a point around the Z axis, using IQmath.
//
//*****************************************************************************
void
IQRotateZ(_iq16 *piqPoint, _iq16 iqZ)
{
    _iq16 iqSin, iqCos, iqTemp;

    //
    // Compute the sin and cos of the rotation angle.
    //
    iqSin = _IQ16sinPU(_IQ16div(iqZ, _IQ16(360)));
    iqCos = _IQ16cosPU(_IQ16div(iqZ, _IQ16(360)));

    //
    // Rotate this point around the Y axis.
    //
    iqTemp = _IQ16mpy(iqSin, piqPoint[0]) + _IQ16mpy(iqCos, piqPoint[1]);
    piqPoint[0] = _IQ16mpy(iqCos, piqPoint[0]) - _IQ16mpy(iqSin, piqPoint[1]);
    piqPoint[1] = iqTemp;
}

//*****************************************************************************
//
// Transforms the vertices of the model by the specified rotation and
// translation, using IQmath.
//
//*****************************************************************************
void
IQTransformModel(long plRotate[3], long plTranslate[3])
{
    unsigned long ulIdx;
    _iq16 piqPoint[3];

    //
    // Loop through the vertices in the model.
    //
    for(ulIdx = 0; ulIdx < NUM_VERTICES; ulIdx++)
    {
        //
        // Convert this vertex into IQ16 format.
        //
        piqPoint[0] = _IQ16(g_pplVertices[ulIdx][0]);
        piqPoint[1] = _IQ16(g_pplVertices[ulIdx][1]);
        piqPoint[2] = _IQ16(g_pplVertices[ulIdx][2]);

        //
        // Rotate this vertex around the X axis.
        //
        IQRotateX(piqPoint, _IQ16(plRotate[0]));

        //
        // Rotate this vertex around the Y axis.
        //
        IQRotateY(piqPoint, _IQ16(plRotate[1]));

        //
        // Rotate this vertex around the Z axis.
        //
        IQRotateZ(piqPoint, _IQ16(plRotate[2]));

        //
        // Translate this vertex and save it into the translated vertex array.
        //
        g_ppiqVertices[ulIdx][0] = piqPoint[0] + _IQ16(plTranslate[0]);
        g_ppiqVertices[ulIdx][1] = piqPoint[1] + _IQ16(plTranslate[1]);
        g_ppiqVertices[ulIdx][2] = piqPoint[2] + _IQ16(plTranslate[2]);
    }
}

//*****************************************************************************
//
// Performs a perspective project of the vertices, using IQmath.
//
//*****************************************************************************
void
IQProjectModel(void)
{
    unsigned long ulIdx;

    //
    // Loop through the vertices.
    //
    for(ulIdx = 0; ulIdx < NUM_VERTICES; ulIdx++)
    {
        //
        // Divide the X and Y coordinates by the Z coordinate, creating the
        // perspective projection of the point in 3-space.
        //
        g_ppiqVertices[ulIdx][0] = _IQ16div(g_ppiqVertices[ulIdx][0],
                                            g_ppiqVertices[ulIdx][2]);
        g_ppiqVertices[ulIdx][1] = _IQ16div(g_ppiqVertices[ulIdx][1],
                                            g_ppiqVertices[ulIdx][2]);

        //
        // Scale and shift the point in 2-space so that it resides within the
        // display viewport.
        //
        g_pplPoints[ulIdx][0] =
            _IQ16int(_IQ16mpy(g_ppiqVertices[ulIdx][0], _IQ16(640))) + 160;
        g_pplPoints[ulIdx][1] =
            _IQ16int(_IQ16mpy(g_ppiqVertices[ulIdx][1], _IQ16(640))) + 102;
    }
}

//*****************************************************************************
//
// Determines if a face is visible, using IQmath.
//
//*****************************************************************************
static int
IQIsVisible(long lVertex1, long lVertex2, long lVertex3)
{
    _iq16 iqX1, iqY1, iqX2, iqY2;

    //
    // Compute the vector from the second vertex to the first.  Only the X and
    // Y values are needed when computing the cross product, so do not compute
    // the Z value.
    //
    iqX1 = g_ppiqVertices[lVertex1][0] - g_ppiqVertices[lVertex2][0];
    iqY1 = g_ppiqVertices[lVertex1][1] - g_ppiqVertices[lVertex2][1];

    //
    // Compute the vector from the second vertex to the third.  Only the X and
    // Y values are needed when computing the cross product, so do not compute
    // the Z value.
    //
    iqX2 = g_ppiqVertices[lVertex3][0] - g_ppiqVertices[lVertex2][0];
    iqY2 = g_ppiqVertices[lVertex3][1] - g_ppiqVertices[lVertex2][1];

    //
    // Compute the Z value of the cross product of these two vectors.  If it is
    // negative, then this face is visible (i.e. facing the viewport).
    // Otherwise, it is not visible.
    //
    if((_IQ16mpy(iqX1, iqY2) - _IQ16mpy(iqY1, iqX2)) < 0)
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// Determines which faces of the model are visible, using IQmath.
//
//*****************************************************************************
void
IQFindVisible(void)
{
    unsigned long ulIdx;

    //
    // Loop through the faces in the model.
    //
    for(ulIdx = 0; ulIdx < NUM_FACES; ulIdx++)
    {
        //
        // See if this face is visible.
        //
        if(IQIsVisible(g_pplFaces[ulIdx][0], g_pplFaces[ulIdx][1],
                       g_pplFaces[ulIdx][2]))
        {
            //
            // Indicate that this face is visible.
            //
            g_plIsVisible[ulIdx] = 1;
        }
        else
        {
            //
            // Indicate that this face is not visible.
            //
            g_plIsVisible[ulIdx] = 0;
        }
    }
}
