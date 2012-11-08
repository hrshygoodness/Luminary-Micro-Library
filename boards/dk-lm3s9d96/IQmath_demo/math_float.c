//*****************************************************************************
//
// math_float.c - Math for doing 3-space rotations and projections, using
//                floating point.
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

#include <math.h>
#include "math_float.h"
#include "model.h"

//*****************************************************************************
//
// Provide a definition for M_PI, if it was not provided by math.h.
//
//*****************************************************************************
#ifndef M_PI
#define M_PI                    3.14159265358979323846
#endif

//*****************************************************************************
//
// An array of the transformed model vertices.
//
//*****************************************************************************
static float g_ppfVertices[NUM_VERTICES][3];

//*****************************************************************************
//
// Rotates a point around the X axis, using floating point.
//
//*****************************************************************************
void
FloatRotateX(float *pfPoint, float fX)
{
    float fSin, fCos, fTemp;

    //
    // Compute the sin and cos of the rotation angle.
    //
    fSin = sin((fX * M_PI) / 180.0);
    fCos = cos((fX * M_PI) / 180.0);

    //
    // Rotate this point around the X axis.
    //
    fTemp = (fSin * pfPoint[1]) + (fCos * pfPoint[2]);
    pfPoint[1] = (fCos * pfPoint[1]) - (fSin * pfPoint[2]);
    pfPoint[2] = fTemp;
}

//*****************************************************************************
//
// Rotates a point around the Y axis, using floating point.
//
//*****************************************************************************
void
FloatRotateY(float *pfPoint, float fY)
{
    float fSin, fCos, fTemp;

    //
    // Compute the sin and cos of the rotation angle.
    //
    fSin = sin((fY * M_PI) / 180.0);
    fCos = cos((fY * M_PI) / 180.0);

    //
    // Rotate this point around the Y axis.
    //
    fTemp = (fSin * pfPoint[0]) + (fCos * pfPoint[2]);
    pfPoint[0] = (fCos * pfPoint[0]) - (fSin * pfPoint[2]);
    pfPoint[2] = fTemp;
}

//*****************************************************************************
//
// Rotates a point around the Z axis, using floating point.
//
//*****************************************************************************
void
FloatRotateZ(float *pfPoint, float fZ)
{
    float fSin, fCos, fTemp;

    //
    // Compute the sin and cos of the rotation angle.
    //
    fSin = sin((fZ * M_PI) / 180.0);
    fCos = cos((fZ * M_PI) / 180.0);

    //
    // Rotate this point around the Z axis.
    //
    fTemp = (fSin * pfPoint[0]) + (fCos * pfPoint[1]);
    pfPoint[0] = (fCos * pfPoint[0]) - (fSin * pfPoint[1]);
    pfPoint[1] = fTemp;
}

//*****************************************************************************
//
// Transforms the vertices of the model by the specified rotation and
// translation, using floating point.
//
//*****************************************************************************
void
FloatTransformModel(long plRotate[3], long plTranslate[3])
{
    unsigned long ulIdx;
    float pfPoint[3];

    //
    // Loop through the vertices in the model.
    //
    for(ulIdx = 0; ulIdx < NUM_VERTICES; ulIdx++)
    {
        //
        // Convert this vertex into floating point.
        //
        pfPoint[0] = (float)g_pplVertices[ulIdx][0];
        pfPoint[1] = (float)g_pplVertices[ulIdx][1];
        pfPoint[2] = (float)g_pplVertices[ulIdx][2];

        //
        // Rotate this vertex around the X axis.
        //
        FloatRotateX(pfPoint, plRotate[0]);

        //
        // Rotate this vertex around the Y axis.
        //
        FloatRotateY(pfPoint, plRotate[1]);

        //
        // Rotate this vertex around the Z axis.
        //
        FloatRotateZ(pfPoint, plRotate[2]);

        //
        // Translate this vertex and save it into the translated vertex array.
        //
        g_ppfVertices[ulIdx][0] = pfPoint[0] + (float)plTranslate[0];
        g_ppfVertices[ulIdx][1] = pfPoint[1] + (float)plTranslate[1];
        g_ppfVertices[ulIdx][2] = pfPoint[2] + (float)plTranslate[2];
    }
}

//*****************************************************************************
//
// Performs a perspective project of the vertices, using floating point.
//
//*****************************************************************************
void
FloatProjectModel(void)
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
        g_ppfVertices[ulIdx][0] /= g_ppfVertices[ulIdx][2];
        g_ppfVertices[ulIdx][1] /= g_ppfVertices[ulIdx][2];

        //
        // Scale and shift the point in 2-space so that it resides within the
        // display viewport.
        //
        g_pplPoints[ulIdx][0] = (long)((g_ppfVertices[ulIdx][0] * 640) + 160);
        g_pplPoints[ulIdx][1] = (long)((g_ppfVertices[ulIdx][1] * 640) + 102);
    }
}

//*****************************************************************************
//
// Determines if a face is visible, using floating point.
//
//*****************************************************************************
static int
FloatIsVisible(long lVertex1, long lVertex2, long lVertex3)
{
    float fX1, fY1, fX2, fY2;

    //
    // Compute the vector from the second vertex to the first.  Only the X and
    // Y values are needed when computing the cross product, so do not compute
    // the Z value.
    //
    fX1 = g_ppfVertices[lVertex1][0] - g_ppfVertices[lVertex2][0];
    fY1 = g_ppfVertices[lVertex1][1] - g_ppfVertices[lVertex2][1];

    //
    // Compute the vector from the second vertex to the third.  Only the X and
    // Y values are needed when computing the cross product, so do not compute
    // the Z value.
    //
    fX2 = g_ppfVertices[lVertex3][0] - g_ppfVertices[lVertex2][0];
    fY2 = g_ppfVertices[lVertex3][1] - g_ppfVertices[lVertex2][1];

    //
    // Compute the Z value of the cross product of these two vectors.  If it is
    // negative, then this face is visible (i.e. facing the viewport).
    // Otherwise, it is not visible.
    //
    if(((fX1 * fY2) - (fY1 * fX2)) < 0)
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
// Determines which faces of the model are visible, using floating point.
//
//*****************************************************************************
void
FloatFindVisible(void)
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
        if(FloatIsVisible(g_pplFaces[ulIdx][0], g_pplFaces[ulIdx][1],
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
