//*****************************************************************************
//
// IQmath_demo.c - Demonstration of the IQmath library.
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
// This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/set_pinout.h"
#include "math_float.h"
#include "math_IQ.h"
#include "model.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>IQmath Demonstration (IQmath_demo)</h1>
//!
//! This application demonstrates the use of the IQmath library, allowing the
//! performance relative to floating-point to be easily seen.  A dodecahedron
//! is rotated in 3-space and displayed on the screen.  Each face of the
//! dodecahedron has a Stellaris logo on it, bring the total vertex count to
//! 704 (i.e. the number of points that are rotated in 3-space for every frame
//! that is displayed).
//!
//! When first started, the IQmath library will be used to rotate the
//! dodecahedron (as indicated by the status line at the bottom of the
//! display).  The user push button on the lower right corner of the board will
//! switch between using IQmath and floating-point math.
//
//*****************************************************************************

//*****************************************************************************
//
// The extends, in 3-space, that the model is allowed to bounce around within.
//
//*****************************************************************************
#define X_MIN                   -400
#define X_MAX                   400
#define Y_MIN                   -140
#define Y_MAX                   140
#define Z_MIN                   3000
#define Z_MAX                   6000

//*****************************************************************************
//
// The width and height of the viewport on the display.
//
//*****************************************************************************
#define WIDTH                   320
#define HEIGHT                  204

//*****************************************************************************
//
// The number of colors in the color table.
//
//*****************************************************************************
#define NUM_COLORS              6

//*****************************************************************************
//
// A table of colors that will be cycled through as the model is rotated in
// 3-space.
//
//*****************************************************************************
static const unsigned char g_ppucColorTargets[NUM_COLORS][3] =
{
    { 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0x00 },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0xff },
    { 0x00, 0x00, 0xff },
    { 0xff, 0x00, 0xff }
};

//*****************************************************************************
//
// An off-screen buffer that is used to render the 3-D image prior to being
// blitted to the screen.
//
//*****************************************************************************
static unsigned char g_pucBuffer[GrOffScreen1BPPSize(WIDTH, HEIGHT)];

//*****************************************************************************
//
// The current seed for the random number generator.
//
//*****************************************************************************
static unsigned long g_ulRandomSeed;

//*****************************************************************************
//
// The current rotation, in each of the three axes, of the model.
//
//*****************************************************************************
static long g_lRotate[3];

//*****************************************************************************
//
// The current position in 3-space of the center of the model.
//
//*****************************************************************************
static long g_lPosition[3];

//*****************************************************************************
//
// The amount by which the rotation is incremented for each frame.
//
//*****************************************************************************
static long g_lRotateDelta[3];

//*****************************************************************************
//
// The amount by which the position is incremented for each frame.
//
//*****************************************************************************
static long g_lPositionDelta[3];

//*****************************************************************************
//
// The index into the color table of the current color target.
//
//*****************************************************************************
static unsigned long g_ulColorTarget;

//*****************************************************************************
//
// The color to be used to draw the next frame.
//
//*****************************************************************************
static unsigned char g_pucColors[3];

//*****************************************************************************
//
// The current target color.
//
//*****************************************************************************
static unsigned char g_pucColorTarget[3];

//*****************************************************************************
//
// The debounced state of the push button.
//
//*****************************************************************************
unsigned char g_ucSwitches = 0x80;

//*****************************************************************************
//
// The vertical counter used to debounce the push buttons.
//
//*****************************************************************************
static unsigned char g_ucSwitchClockA = 0;
static unsigned char g_ucSwitchClockB = 0;

//*****************************************************************************
//
// The flags that control the operation of the demonstration.
//
//*****************************************************************************
static unsigned long g_ulFlags;
#define FLAG_USE_IQMATH         0
#define FLAG_UPDATE_STATUS      1

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// Draws the rotated model into the off-screen buffer.
//
//*****************************************************************************
void
DrawModel(void)
{
    unsigned long ulIdx, ulLine;
    tDisplay sDisplay;
    tContext sContext;
    tRectangle sRect;

    //
    // Initialize the off-screen buffer.
    //
    GrOffScreen1BPPInit(&sDisplay, g_pucBuffer, WIDTH, HEIGHT);
    GrContextInit(&sContext, &sDisplay);

    //
    // Clear the off-screen buffer.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = WIDTH - 1;
    sRect.sYMax = HEIGHT - 1;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);

    //
    // Set the foreground color that will be used to draw the model.
    //
    GrContextForegroundSet(&sContext, ClrWhite);

    //
    // Loop through the faces of the model.
    //
    for(ulIdx = 0; ulIdx < NUM_FACES; ulIdx++)
    {
        //
        // Skip this face if it is not visible.
        //
        if(g_plIsVisible[ulIdx] == 0)
        {
            continue;
        }

        //
        // Draw the lines that make up this face.
        //
        for(ulLine = 0; ulLine < NUM_FACE_LINES; ulLine++)
        {
            GrLineDraw(&sContext, g_pplPoints[g_pplFaces[ulIdx][ulLine]][0],
                       g_pplPoints[g_pplFaces[ulIdx][ulLine]][1],
                       g_pplPoints[g_pplFaces[ulIdx][ulLine + 1]][0],
                       g_pplPoints[g_pplFaces[ulIdx][ulLine + 1]][1]);
        }

        //
        // Draw the lines that make up the logo on this face.
        //
        for(ulLine = 0; ulLine < NUM_LOGO_LINES; ulLine++)
        {
            GrLineDraw(&sContext, g_pplPoints[g_pplLogos[ulIdx][ulLine]][0],
                       g_pplPoints[g_pplLogos[ulIdx][ulLine]][1],
                       g_pplPoints[g_pplLogos[ulIdx][ulLine + 1]][0],
                       g_pplPoints[g_pplLogos[ulIdx][ulLine + 1]][1]);
        }
    }
}

//*****************************************************************************
//
// Generate a new random number.  The number returned would more accruately be
// described as a pseudo-random number since a linear congruence generator is
// being used.
//
//*****************************************************************************
unsigned long
RandomNumber(void)
{
    //
    // Generate a new pseudo-random number with a linear congruence random
    // number generator.  This new random number becomes the seed for the next
    // random number.
    //
    g_ulRandomSeed = (g_ulRandomSeed * 1664525) + 1013904223;

    //
    // Return the new random number.
    //
    return(g_ulRandomSeed);
}

//*****************************************************************************
//
// Updates the value of one axis of the model's position.
//
//*****************************************************************************
unsigned long
UpdatePosition(long *plPosition, long *plDelta, long lMin, long lMax)
{
    //
    // Update the position by the delta.
    //
    *plPosition += *plDelta;

    //
    // See if the position has exceeded the minimum or maximum.  If it has not,
    // then return without any further processing.
    //
    if(*plPosition < lMin)
    {
        *plPosition = lMin;
    }
    else if(*plPosition > lMax)
    {
        *plPosition = lMax;
    }
    else
    {
        return(0);
    }

    //
    // Choose a new delta based on a random number.
    //
    *plDelta = (long)(RandomNumber() >> 28) - 8;

    //
    // Make sure that the delta is not too small.
    //
    if((*plDelta < 0) && (*plDelta > -4))
    {
        *plDelta = -4;
    }
    if((*plDelta >= 0) && (*plDelta < 4))
    {
        *plDelta = 4;
    }

    //
    // Indicate that an extent was reached.
    //
    return(1);
}

//*****************************************************************************
//
// Updates the color used for drawing the model.
//
//*****************************************************************************
void
UpdateColor(void)
{
    unsigned long ulIdx, ulNew;

    //
    // Loop through the three components of the color.
    //
    ulNew = 1;
    for(ulIdx = 0; ulIdx < 3; ulIdx++)
    {
        //
        // See if this component of the color has reached the target.
        //
        if(g_pucColors[ulIdx] != g_pucColorTarget[ulIdx])
        {
            //
            // Either increment or decrement this component of the color as
            // required to make it approach the target.
            //
            if(g_pucColors[ulIdx] > g_pucColorTarget[ulIdx])
            {
                g_pucColors[ulIdx] -= 3;
            }
            else
            {
                g_pucColors[ulIdx] += 3;
            }

            //
            // Indicate that it is not time for a new color target.
            //
            ulNew = 0;
        }
    }

    //
    // See if it is time for a new color target.
    //
    if(ulNew)
    {
        //
        // Increment the color target array index.
        //
        g_ulColorTarget++;
        if(g_ulColorTarget == NUM_COLORS)
        {
            g_ulColorTarget = 0;
        }

        //
        // Copy the color components of the new target color from the array
        // into the color target.
        //
        for(ulIdx = 0; ulIdx < 3; ulIdx++)
        {
            g_pucColorTarget[ulIdx] =
                g_ppucColorTargets[g_ulColorTarget][ulIdx];
        }
    }
}

//*****************************************************************************
//
// Handles the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulData, ulDelta;

    //
    // Read the state of the GPIO input.
    //
    ulData = ROM_GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_7);

    //
    // Determine the switches that are at a different state than the debounced
    // state.
    //
    ulDelta = ulData ^ g_ucSwitches;

    //
    // Increment the clocks by one.
    //
    g_ucSwitchClockA ^= g_ucSwitchClockB;
    g_ucSwitchClockB = ~g_ucSwitchClockB;

    //
    // Reset the clocks corresponding to switches that have not changed state.
    //
    g_ucSwitchClockA &= ulDelta;
    g_ucSwitchClockB &= ulDelta;

    //
    // Get the new debounced switch state.
    //
    g_ucSwitches &= g_ucSwitchClockA | g_ucSwitchClockB;
    g_ucSwitches |= (~(g_ucSwitchClockA | g_ucSwitchClockB)) & ulData;

    //
    // Determine the switches that just changed debounced state.
    //
    ulDelta ^= (g_ucSwitchClockA | g_ucSwitchClockB);

    //
    // See if the button was just pressed.
    //
    if((ulDelta & 0x80) && !(g_ucSwitches & 0x80))
    {
        //
        // Toggle the flag that selects between IQmath and floating point
        // math.
        //
        HWREGBITW(&g_ulFlags, FLAG_USE_IQMATH) ^= 1;

        //
        // Set the flag that indicates that the status line needs to be
        // updated.
        //
        HWREGBITW(&g_ulFlags, FLAG_UPDATE_STATUS) = 1;
    }
}

//*****************************************************************************
//
// This program spins a model in 3-space, using either IQmath or software
// floating point arithmetic (allowing the performance of the two to be easily
// compared).
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIdx, ulBump;
    tContext sContext;
    tRectangle sRect;

    //
    // Set the clocking to run at 80 MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Initialize the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.sYMax = 23;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, g_pFontCm20);
    GrStringDrawCentered(&sContext, "IQmath-demo", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 10, 0);

    //
    // Fill the bottom 12 rows of the screen with blue to create the status
    // line.
    //
    sRect.sYMin = GrContextDpyHeightGet(&sContext) - 12;
    sRect.sYMax = GrContextDpyHeightGet(&sContext) - 1;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Put a white box around the status line.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);

    //
    // Put the initial status in the middle of the status line.
    //
    GrContextFontSet(&sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&sContext, "Using IQmath", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         GrContextDpyHeightGet(&sContext) - 6, 0);

    //
    // Flush any cached drawing operations.
    //
    GrFlush(&sContext);

    //
    // Configure SysTick to generate an interrupt every 10 ms.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 100);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Configure the GPIO that is attached to the user switch.
    //
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_7);

    //
    // Set the initial value of the random seed.
    //
    g_ulRandomSeed = 0xf61e2e60;

    //
    // Set the initial rotation of the model.
    //
    g_lRotate[0] = -58;
    g_lRotate[1] = 0;
    g_lRotate[2] = 0;

    //
    // Set the initial position of the model.
    //
    g_lPosition[0] = 0;
    g_lPosition[1] = 0;
    g_lPosition[2] = Z_MIN;

    //
    // Set the initial rotation delta.
    //
    g_lRotateDelta[0] = 0;
    g_lRotateDelta[1] = 0;
    g_lRotateDelta[2] = 0;

    //
    // Set the initial position delta.
    //
    g_lPositionDelta[0] = 6;
    g_lPositionDelta[1] = 4;
    g_lPositionDelta[2] = 50;

    //
    // Set the initial color.
    //
    g_pucColors[0] = g_ppucColorTargets[0][0];
    g_pucColors[1] = g_ppucColorTargets[0][1];
    g_pucColors[2] = g_ppucColorTargets[0][2];

    //
    // Set the initial target color.
    //
    g_ulColorTarget = 1;
    g_pucColorTarget[0] = g_ppucColorTargets[1][0];
    g_pucColorTarget[1] = g_ppucColorTargets[1][1];
    g_pucColorTarget[2] = g_ppucColorTargets[1][2];

    //
    // Use IQmath by default.
    //
    HWREGBITW(&g_ulFlags, FLAG_USE_IQMATH) = 1;

    //
    // Loop forever, moving and redrawing the model.
    //
    while(1)
    {
        //
        // See if the status line needs to be updated.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UPDATE_STATUS) == 1)
        {
            //
            // Set the foreground and background colors used to draw on the
            // status line.
            //
            GrContextForegroundSet(&sContext, ClrWhite);
            GrContextBackgroundSet(&sContext, ClrDarkBlue);

            //
            // See if IQmath or floating point is being used.
            //
            if(HWREGBITW(&g_ulFlags, FLAG_USE_IQMATH) == 1)
            {
                //
                // Indicate that IQmath is being used.
                //
                GrStringDrawCentered(&sContext,
                                     "         Using IQmath         ", -1,
                                     GrContextDpyWidthGet(&sContext) / 2,
                                     GrContextDpyHeightGet(&sContext) - 6, 1);
            }
            else
            {
                //
                // Indicate that floating point is being used.
                //
                GrStringDrawCentered(&sContext,
                                     "Using Software Floating Point", -1,
                                     GrContextDpyWidthGet(&sContext) / 2,
                                     GrContextDpyHeightGet(&sContext) - 6, 1);
            }

            //
            // The status line has been updated.
            //
            HWREGBITW(&g_ulFlags, FLAG_UPDATE_STATUS) = 0;
        }

        //
        // See if IQmath or flaoting point is being used.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_USE_IQMATH) == 1)
        {
            //
            // Transform the vertices of the model.
            //
            IQTransformModel(g_lRotate, g_lPosition);

            //
            // Perform a perspective project on the vertices of the model.
            //
            IQProjectModel();

            //
            // Find the visible faces of the model.
            //
            IQFindVisible();
        }
        else
        {
            //
            // Transform the vertices of the model.
            //
            FloatTransformModel(g_lRotate, g_lPosition);

            //
            // Perform a perspective project on the vertices of the model.
            //
            FloatProjectModel();

            //
            // Find the visible faces of the model.
            //
            FloatFindVisible();
        }

        //
        // Draw the model into the off-screen buffer.
        //
        DrawModel();

        //
        // Blit the off-screen buffer onto the screen.
        //
        GrContextForegroundSet(&sContext, ((g_pucColors[0] << 16) |
                                           (g_pucColors[1] << 8) |
                                           g_pucColors[2]));
        GrContextBackgroundSet(&sContext, ClrBlack);
        GrImageDraw(&sContext, g_pucBuffer, 0, 24);

        //
        // Update the rotation, wrapping where necessary.
        //
        for(ulIdx = 0; ulIdx < 3; ulIdx++)
        {
            g_lRotate[ulIdx] += g_lRotateDelta[ulIdx];
            if(g_lRotate[ulIdx] < 0)
            {
                g_lRotate[ulIdx] += 360;
            }
            if(g_lRotate[ulIdx] > 360)
            {
                g_lRotate[ulIdx] -= 360;
            }
        }

        //
        // Clear the flags that indicate that the model has bumped the bounds
        // of the area it is constrained within.
        //
        ulBump = 0;

        //
        // Update the position of the model.
        //
        ulBump |= UpdatePosition(&(g_lPosition[0]), &(g_lPositionDelta[0]),
                                 X_MIN, X_MAX) ? 1 : 0;
        ulBump |= UpdatePosition(&(g_lPosition[1]), &(g_lPositionDelta[1]),
                                 Y_MIN, Y_MAX) ? 2 : 0;
        ulBump |= UpdatePosition(&(g_lPosition[2]), &(g_lPositionDelta[2]),
                                 Z_MIN, Z_MAX) ? 4 : 0;

        //
        // If the model bumped into the Z axis limits, the newly selected
        // position delta needs to be increased by an order of magnitude since
        // the Z extents are an order of magnitude larger than the X and Y
        // extents.
        //
        if(ulBump & 4)
        {
            g_lPositionDelta[2] *= 10;
        }

        //
        // If the model bumped into one of the extents, then select a new
        // rotation speed.
        //
        if(ulBump)
        {
            g_lRotateDelta[0] = (long)(RandomNumber() >> 29) - 4;
            g_lRotateDelta[1] = (long)(RandomNumber() >> 29) - 4;
            g_lRotateDelta[2] = (long)(RandomNumber() >> 29) - 4;
        }

        //
        // Update the color so that the next frame is drawn with the next
        // color.
        //
        UpdateColor();
    }
}
