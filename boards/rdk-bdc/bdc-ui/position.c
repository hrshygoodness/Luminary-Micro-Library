//*****************************************************************************
//
// position.c - Displays the "Position Control Mode" panel.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "utils/ustdlib.h"
#include "shared/can_proto.h"
#include "bdc-ui.h"
#include "can_comm.h"
#include "menu.h"
#include "rit128x96x4.h"
#include "position.h"
#include "status.h"

//*****************************************************************************
//
// Buffers to contain the string representation of the current device ID,
// position, P coefficient, I coefficient, D coefficient, and position
// reference.
//
//*****************************************************************************
static char g_pcIDBuffer[4];
static char g_pcPositionBuffer[12];
static char g_pcPositionPBuffer[12];
static char g_pcPositionIBuffer[12];
static char g_pcPositionDBuffer[12];
static char g_pcReferenceBuffer[16];

//*****************************************************************************
//
// The strings that represent the position reference settings.
//
//*****************************************************************************
static char *g_ppcPosReference[] =
{
    "encoder",
    "potentiometer"
};

//*****************************************************************************
//
// The widgets that make up the "Position Control Mode" panel.
//
//*****************************************************************************
static tCanvasWidget g_psPositionWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 "Position Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 24, 12, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcIDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 60, 20, 54, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcPositionBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 28, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcPositionPBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 36, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcPositionIBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 44, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcPositionDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 30, 52, 84, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcReferenceBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 12, 18, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "ID:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 20, 54, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Position:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 28, 12, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "P:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 36, 12, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "I:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 44, 12, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "D:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 52, 24, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "Ref:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 9, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the "Position Control Mode" panel.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psPositionWidgets) /                \
                                 sizeof(g_psPositionWidgets[0]))

//*****************************************************************************
//
// This structure contains the current configuration of the position control
// mode of the motor controller.
//
//*****************************************************************************
static struct
{
    long lPosition;
    long lP;
    long lI;
    long lD;
    unsigned long ulPosRef;
}
g_sPositionConfig;

//*****************************************************************************
//
// The position commands sent to the motor by the demo mode.  The first member
// of each array is the position and the second member is the amount of time
// (in milliseconds) to output that position.
//
//*****************************************************************************
static long g_plPositionDemo[][2] =
{
    { 100, 1000 },
    { 50, 1000 },
    { 75, 1000 },
    { 25, 1000 },
    { 0, 1000 },
    { -100, 1000 },
    { -50, 1000 },
    { -75, 1000 },
    { -25, 1000 },
    { 0, 1000 }
};

//*****************************************************************************
//
// Reads the configuration of the position control mode of the current motor
// controller.
//
//*****************************************************************************
void
PositionConfigRead(void)
{
    //
    // Read the current position.
    //
    if(CANReadParameter(LM_API_STATUS_POS, 0,
                        (unsigned long *)&(g_sPositionConfig.lPosition),
                        0) == 0)
    {
        g_sPositionConfig.lPosition = 0;
    }
    else if(g_sPositionConfig.lPosition < 0)
    {
        g_sPositionConfig.lPosition =
            (((g_sPositionConfig.lPosition / 65536) * 100) +
             ((((g_sPositionConfig.lPosition % 65536) * 100) - 32768) /
              65536));
    }
    else
    {
        g_sPositionConfig.lPosition =
            (((g_sPositionConfig.lPosition / 65536) * 100) +
             ((((g_sPositionConfig.lPosition % 65536) * 100) + 32768) /
              65536));
    }

    //
    // Read the PID controller's P coefficient.
    //
    if(CANReadParameter(LM_API_POS_PC, 0,
                        (unsigned long *)&(g_sPositionConfig.lP), 0) == 0)
    {
        g_sPositionConfig.lP = 0;
    }
    else if(g_sPositionConfig.lP < 0)
    {
        g_sPositionConfig.lP = (((g_sPositionConfig.lP / 65536) * 1000) +
                                ((((g_sPositionConfig.lP % 65536) * 1000) -
                                  32768) / 65536));
    }
    else
    {
        g_sPositionConfig.lP = (((g_sPositionConfig.lP / 65536) * 1000) +
                                ((((g_sPositionConfig.lP % 65536) * 1000) +
                                  32768) / 65536));
    }

    //
    // Read the PID controller's I coefficient.
    //
    if(CANReadParameter(LM_API_POS_IC, 0,
                        (unsigned long *)&(g_sPositionConfig.lI), 0) == 0)
    {
        g_sPositionConfig.lI = 0;
    }
    else if(g_sPositionConfig.lI < 0)
    {
        g_sPositionConfig.lI = (((g_sPositionConfig.lI / 65536) * 1000) +
                                ((((g_sPositionConfig.lI % 65536) * 1000) -
                                  32768) / 65536));
    }
    else
    {
        g_sPositionConfig.lI = (((g_sPositionConfig.lI / 65536) * 1000) +
                                ((((g_sPositionConfig.lI % 65536) * 1000) +
                                  32768) / 65536));
    }

    //
    // Read the PID controller's D coefficient.
    //
    if(CANReadParameter(LM_API_POS_DC, 0,
                        (unsigned long *)&(g_sPositionConfig.lD), 0) == 0)
    {
        g_sPositionConfig.lD = 0;
    }
    else if(g_sPositionConfig.lD < 0)
    {
        g_sPositionConfig.lD = (((g_sPositionConfig.lD / 65536) * 1000) +
                                ((((g_sPositionConfig.lD % 65536) * 1000) -
                                  32768) / 65536));
    }
    else
    {
        g_sPositionConfig.lD = (((g_sPositionConfig.lD / 65536) * 1000) +
                                ((((g_sPositionConfig.lD % 65536) * 1000) +
                                  32768) / 65536));
    }

    //
    // Read the position reference source.
    //
    if(CANReadParameter(LM_API_POS_REF, 0, &(g_sPositionConfig.ulPosRef),
                        0) == 0)
    {
        g_sPositionConfig.ulPosRef = 0;
    }
    else
    {
        g_sPositionConfig.ulPosRef &= 1;
    }
}

//*****************************************************************************
//
// Displays the "Position Control Mode" panel.  The returned valud is the ID of
// the panel to be displayed instead of the "Position Control Mode" panel.
//
//*****************************************************************************
unsigned long
DisplayPosition(void)
{
    unsigned long ulPos, ulIdx, ulDelay, ulDemo, ulTime, ulStep;

    //
    // Read the current position mode configuration.
    //
    PositionConfigRead();

    //
    // Enable position control mode.
    //
    CANPositionModeEnable(((g_sPositionConfig.lPosition / 100) * 65536) +
                          (((g_sPositionConfig.lPosition % 100) * 65536) /
                           100));

    //
    // Initially, updates to the position occur immediately.
    //
    ulDelay = 0;

    //
    // Initially, demo mode is disabled.
    //
    ulDemo = 0;
    ulTime = 0;
    ulStep = 0;

    //
    // Disable the widget fill for all the widgets except the one for the
    // device ID selection.
    //
    for(ulIdx = 0; ulIdx < 7; ulIdx++)
    {
        CanvasFillOff(g_psPositionWidgets + ulIdx);
    }
    CanvasFillOn(g_psPositionWidgets + 1);

    //
    // Add the "Position Control Mode" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPositionWidgets + ulIdx));
    }

    //
    // Enable the status display.
    //
    StatusEnable(0);

    //
    // Set the default cursor position to the device ID selection.
    //
    ulPos = 1;

    //
    // Loop forever.  This loop will be explicitly exited when the proper
    // condition is detected.
    //
    while(1)
    {
        //
        // Print out the current device ID.
        //
        usnprintf(g_pcIDBuffer, sizeof(g_pcIDBuffer), "%d", g_ulCurrentID);

        //
        // Print out the current position.
        //
        if(g_sPositionConfig.lPosition < 0)
        {
            usnprintf(g_pcPositionBuffer, sizeof(g_pcPositionBuffer),
                      "-%d.%02d", (0 - g_sPositionConfig.lPosition) / 100,
                      (0 - g_sPositionConfig.lPosition) % 100);
        }
        else
        {
            usnprintf(g_pcPositionBuffer, sizeof(g_pcPositionBuffer),
                      "%d.%02d", g_sPositionConfig.lPosition / 100,
                      g_sPositionConfig.lPosition % 100);
        }

        //
        // Print out the current P coefficient.
        //
        if(g_sPositionConfig.lP < 0)
        {
            usnprintf(g_pcPositionPBuffer, sizeof(g_pcPositionPBuffer),
                      "-%d.%03d", (0 - g_sPositionConfig.lP) / 1000,
                      (0 - g_sPositionConfig.lP) % 1000);
        }
        else
        {
            usnprintf(g_pcPositionPBuffer, sizeof(g_pcPositionPBuffer),
                      "%d.%03d", g_sPositionConfig.lP / 1000,
                      g_sPositionConfig.lP % 1000);
        }

        //
        // Irint out the current I coefficient.
        //
        if(g_sPositionConfig.lI < 0)
        {
            usnprintf(g_pcPositionIBuffer, sizeof(g_pcPositionIBuffer),
                      "-%d.%03d", (0 - g_sPositionConfig.lI) / 1000,
                      (0 - g_sPositionConfig.lI) % 1000);
        }
        else
        {
            usnprintf(g_pcPositionIBuffer, sizeof(g_pcPositionIBuffer),
                      "%d.%03d", g_sPositionConfig.lI / 1000,
                      g_sPositionConfig.lI % 1000);
        }

        //
        // Print out the current D coefficient.
        //
        if(g_sPositionConfig.lD < 0)
        {
            usnprintf(g_pcPositionDBuffer, sizeof(g_pcPositionDBuffer),
                      "-%d.%03d", (0 - g_sPositionConfig.lD) / 1000,
                      (0 - g_sPositionConfig.lD) % 1000);
        }
        else
        {
            usnprintf(g_pcPositionDBuffer, sizeof(g_pcPositionDBuffer),
                      "%d.%03d", g_sPositionConfig.lD / 1000,
                      g_sPositionConfig.lD % 1000);
        }

        //
        // Print out the current position reference source.
        //
        usnprintf(g_pcReferenceBuffer, sizeof(g_pcReferenceBuffer), "%s",
                  g_ppcPosReference[g_sPositionConfig.ulPosRef]);

        //
        // Update the status display.
        //
        StatusUpdate();

        //
        // Update the display.
        //
        DisplayFlush();

        //
        // See if a serial download has begun.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 1)
        {
            //
            // Disable the status display.
            //
            StatusDisable();

            //
            // Remove the "Position Control Mode" panel widgets.
            //
            for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
            {
                WidgetRemove((tWidget *)(g_psPositionWidgets + ulIdx));
            }
            CanvasTextColorSet(g_psPositionWidgets + 2, ClrWhite);

            //
            // Disable position control mode.
            //
            CANPositionModeDisable();

            //
            // Return the ID of the update panel.
            //
            return(PANEL_UPDATE);
        }

        //
        // See if demo mode is enabled.
        //
        if(ulDemo != 0)
        {
            //
            // See if the current time delay has expired.
            //
            if(ulTime < g_ulTickCount)
            {
                //
                // Increment to the next step, wrapping back to the beginning
                // of the sequence when the end has been reached.
                //
                ulStep++;
                if(ulStep == (sizeof(g_plPositionDemo) /
                              sizeof(g_plPositionDemo[0])))
                {
                    ulStep = 0;
                }

                //
                // Set the position as directed by the next step.
                //
                g_sPositionConfig.lPosition = g_plPositionDemo[ulStep][0];
                CANPositionSet(((g_sPositionConfig.lPosition / 100) * 65536) +
                               (((g_sPositionConfig.lPosition % 100) * 65536) /
                                100), 0);

                //
                // Set the time delay for this step.
                //
                ulTime = g_ulTickCount + g_plPositionDemo[ulStep][1];
            }
        }

        //
        // See if the up button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the top of the
            // screen and a delayed position update is not in progress.
            //
            if((ulPos != 0) && (ulDelay == 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psPositionWidgets + ulPos);

                //
                // Decrement the cursor row, skipping the position row when
                // demo mode is enabled.
                //
                ulPos--;
                if((ulPos == 2) && (ulDemo != 0))
                {
                    ulPos--;
                }

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psPositionWidgets + ulPos);
            }

            //
            // Clear the press flag for the up button.
            //
            HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) = 0;
        }

        //
        // See if the down button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the bottom of the
            // screen and a delayed position update is not in progress.
            //
            if((ulPos != 6) && (ulDelay == 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psPositionWidgets + ulPos);

                //
                // Increment the cursor row, skipping the position row when
                // demo mode is enabled.
                //
                ulPos++;
                if((ulPos == 2) && (ulDemo != 0))
                {
                    ulPos++;
                }

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psPositionWidgets + ulPos);
            }

            //
            // Clear the press flag for the down button.
            //
            HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
        }

        //
        // See if the left button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) == 1)
        {
            //
            // See if the device ID is being changed.
            //
            if(ulPos == 1)
            {
                //
                // Only change the device ID if it is greater than one.
                //
                if(g_ulCurrentID > 1)
                {
                    //
                    // Exit demo mode.
                    //
                    ulDemo = 0;
                    CanvasTextColorSet(g_psPositionWidgets + 2, ClrWhite);

                    //
                    // Disable position control mode for the current device ID.
                    //
                    CANPositionModeDisable();

                    //
                    // Decrement the device ID.
                    //
                    if((HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1))
                    {
                        if(g_ulCurrentID > 3)
                        {
                            CANSetID(g_ulCurrentID - 3);
                        }
                        else
                        {
                            CANSetID(1);
                        }
                    }
                    else
                    {
                        CANSetID(g_ulCurrentID - 1);
                    }

                    //
                    // Read the configuration of the new device.
                    //
                    PositionConfigRead();

                    //
                    // Enable position control mode.
                    //
                    CANPositionModeEnable(((g_sPositionConfig.lPosition /
                                            100) * 65536) +
                                          (((g_sPositionConfig.lPosition %
                                             100) * 65536) / 100));
                }
            }

            //
            // See if the position is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the position if it is not already fully
                // negative.
                //
                if(g_sPositionConfig.lPosition > -20000)
                {
                    //
                    // Decrement the position.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sPositionConfig.lPosition -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sPositionConfig.lPosition -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sPositionConfig.lPosition -= 1111;
                    }
                    else
                    {
                        g_sPositionConfig.lPosition--;
                    }
                    if(g_sPositionConfig.lPosition < -20000)
                    {
                        g_sPositionConfig.lPosition = -20000;
                    }

                    //
                    // Send the updated position to the motor controller if a
                    // delayed update is not in progress.
                    //
                    if(ulDelay == 0)
                    {
                        CANPositionSet(((g_sPositionConfig.lPosition / 100) *
                                        65536) +
                                       (((g_sPositionConfig.lPosition % 100) *
                                         65536) / 100), 0);
                    }
                }
            }

            //
            // See if the position P gain is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the P gain if it is not already fully negative.
                //
                if(g_sPositionConfig.lP > (-32767 * 1000))
                {
                    //
                    // Decrement the P gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sPositionConfig.lP -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sPositionConfig.lP -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sPositionConfig.lP -= 1111;
                    }
                    else
                    {
                        g_sPositionConfig.lP--;
                    }
                    if(g_sPositionConfig.lP < (-32767 * 1000))
                    {
                        g_sPositionConfig.lP = -32767 * 1000;
                    }

                    //
                    // Send the new P gain to the motor controller.
                    //
                    CANPositionPGainSet(((g_sPositionConfig.lP / 1000) *
                                         65536) +
                                        (((g_sPositionConfig.lP % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the position I gain is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Only change the I gain if it is not already fully negative.
                //
                if(g_sPositionConfig.lI > (-32767 * 1000))
                {
                    //
                    // Decrement the I gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sPositionConfig.lI -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sPositionConfig.lI -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sPositionConfig.lI -= 1111;
                    }
                    else
                    {
                        g_sPositionConfig.lI--;
                    }
                    if(g_sPositionConfig.lI < (-32767 * 1000))
                    {
                        g_sPositionConfig.lI = -32767 * 1000;
                    }

                    //
                    // Send the new I gain to the motor controller.
                    //
                    CANPositionIGainSet(((g_sPositionConfig.lI / 1000) *
                                         65536) +
                                        (((g_sPositionConfig.lI % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the position D gain is being changed.
            //
            else if(ulPos == 5)
            {
                //
                // Only change the D gain if it is not already fully negative.
                //
                if(g_sPositionConfig.lD > (-32767 * 1000))
                {
                    //
                    // Decrement the D gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sPositionConfig.lD -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sPositionConfig.lD -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sPositionConfig.lD -= 1111;
                    }
                    else
                    {
                        g_sPositionConfig.lD--;
                    }
                    if(g_sPositionConfig.lD < (-32767 * 1000))
                    {
                        g_sPositionConfig.lD = -32767 * 1000;
                    }

                    //
                    // Send the new D gain to the motor controller.
                    //
                    CANPositionDGainSet(((g_sPositionConfig.lD / 1000) *
                                         65536) +
                                        (((g_sPositionConfig.lD % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the position reference source is being changed.
            //
            else if(ulPos == 6)
            {
                //
                // Toggle to the other position reference source.
                //
                g_sPositionConfig.ulPosRef ^= 1;

                //
                // Send the position reference source to the motor controller.
                //
                CANPositionRefSet(g_sPositionConfig.ulPosRef);
            }

            //
            // Clear the press flag for the left button.
            //
            HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
            HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) = 0;
            HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) = 0;
            HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) = 0;
        }

        //
        // See if the right button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) == 1)
        {
            //
            // See if the device ID is being changed.
            //
            if(ulPos == 1)
            {
                //
                // Only change the device ID if it is less than 63.
                //
                if(g_ulCurrentID < 63)
                {
                    //
                    // Exit demo mode.
                    //
                    ulDemo = 0;
                    CanvasTextColorSet(g_psPositionWidgets + 2, ClrWhite);

                    //
                    // Disable position control mode for the current device ID.
                    //
                    CANPositionModeDisable();

                    //
                    // Increment the device ID.
                    //
                    if((HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1))
                    {
                        if(g_ulCurrentID < 60)
                        {
                            CANSetID(g_ulCurrentID + 3);
                        }
                        else
                        {
                            CANSetID(63);
                        }
                    }
                    else
                    {
                        CANSetID(g_ulCurrentID + 1);
                    }

                    //
                    // Read the configuration of the new device.
                    //
                    PositionConfigRead();

                    //
                    // Enable position control mode.
                    //
                    CANPositionModeEnable(((g_sPositionConfig.lPosition /
                                            100) * 65536) +
                                          (((g_sPositionConfig.lPosition %
                                             100) * 65536) / 100));
                }
            }

            //
            // See if the position is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the position if it is not already fully
                // positive.
                //
                if(g_sPositionConfig.lPosition < 20000)
                {
                    //
                    // Increment the position.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sPositionConfig.lPosition += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sPositionConfig.lPosition += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sPositionConfig.lPosition += 1111;
                    }
                    else
                    {
                        g_sPositionConfig.lPosition++;
                    }
                    if(g_sPositionConfig.lPosition > 20000)
                    {
                        g_sPositionConfig.lPosition = 20000;
                    }

                    //
                    // Send the updated position to the motor controller if a
                    // delayed update is not in progress.
                    //
                    if(ulDelay == 0)
                    {
                        CANPositionSet(((g_sPositionConfig.lPosition / 100) *
                                        65536) +
                                       (((g_sPositionConfig.lPosition % 100) *
                                         65536) / 100), 0);
                    }
                }
            }

            //
            // See if the position P gain is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the P gain if it is not already fully positive.
                //
                if(g_sPositionConfig.lP < (32767 * 1000))
                {
                    //
                    // Increment the P gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sPositionConfig.lP += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sPositionConfig.lP += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sPositionConfig.lP += 1111;
                    }
                    else
                    {
                        g_sPositionConfig.lP++;
                    }
                    if(g_sPositionConfig.lP > (32767 * 1000))
                    {
                        g_sPositionConfig.lP = 32767 * 1000;
                    }

                    //
                    // Send the new P gain to the motor controller.
                    //
                    CANPositionPGainSet(((g_sPositionConfig.lP / 1000) *
                                         65536) +
                                        (((g_sPositionConfig.lP % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the position I gain is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Only change the I gain if it is not already fully positive.
                //
                if(g_sPositionConfig.lI < (32767 * 1000))
                {
                    //
                    // Increment the I gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sPositionConfig.lI += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sPositionConfig.lI += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sPositionConfig.lI += 1111;
                    }
                    else
                    {
                        g_sPositionConfig.lI++;
                    }
                    if(g_sPositionConfig.lI > (32767 * 1000))
                    {
                        g_sPositionConfig.lI = 32767 * 1000;
                    }

                    //
                    // Send the new I gain to the motor controller.
                    //
                    CANPositionIGainSet(((g_sPositionConfig.lI / 1000) *
                                         65536) +
                                        (((g_sPositionConfig.lI % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the position D gain is being changed.
            //
            else if(ulPos == 5)
            {
                //
                // Only change the D gain if it is not already fully positive.
                //
                if(g_sPositionConfig.lD < (32767 * 1000))
                {
                    //
                    // Increment the D gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sPositionConfig.lD += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sPositionConfig.lD += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sPositionConfig.lD += 1111;
                    }
                    else
                    {
                        g_sPositionConfig.lD++;
                    }
                    if(g_sPositionConfig.lD > (32767 * 1000))
                    {
                        g_sPositionConfig.lD = 32767 * 1000;
                    }

                    //
                    // Send the new D gain to the motor controller.
                    //
                    CANPositionDGainSet(((g_sPositionConfig.lD / 1000) *
                                         65536) +
                                        (((g_sPositionConfig.lD % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the position reference source is being changed.
            //
            else if(ulPos == 6)
            {
                //
                // Toggle to the other position reference source.
                //
                g_sPositionConfig.ulPosRef ^= 1;

                //
                // Send the position reference source to the motor controller.
                //
                CANPositionRefSet(g_sPositionConfig.ulPosRef);
            }

            //
            // Clear the press flag for the right button.
            //
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) = 0;
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) = 0;
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) = 0;
        }

        //
        // See if the select button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) == 1)
        {
            //
            // Clear the press flag for the select button.
            //
            HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) = 0;

            //
            // See if the cursor is on the top row of the screen.
            //
            if(ulPos == 0)
            {
                //
                // Display the menu.
                //
                ulIdx = DisplayMenu(PANEL_POSITION);

                //
                // See if another panel was selected.
                //
                if(ulIdx != PANEL_POSITION)
                {
                    //
                    // Disable the status display.
                    //
                    StatusDisable();

                    //
                    // Remove the "Position Control Mode" panel widgets.
                    //
                    for(ulPos = 0; ulPos < NUM_WIDGETS; ulPos++)
                    {
                        WidgetRemove((tWidget *)(g_psPositionWidgets + ulPos));
                    }
                    CanvasTextColorSet(g_psPositionWidgets + 2, ClrWhite);

                    //
                    // Disable position control mode.
                    //
                    CANPositionModeDisable();

                    //
                    // Return the ID of the newly selected panel.
                    //
                    return(ulIdx);
                }

                //
                // Since the "Position Control Mode" panel was selected from
                // the menu, move the cursor down one row.
                //
                CanvasFillOff(g_psPositionWidgets);
                ulPos++;
                CanvasFillOn(g_psPositionWidgets + 1);
            }

            //
            // See if the cursor is on the ID selection.
            //
            else if(ulPos == 1)
            {
                //
                // Toggle demo mode.
                //
                ulDemo ^= 1;

                //
                // See if the demo has just been disabled.
                //
                if(ulDemo == 0)
                {
                    //
                    // Set the output position to the current position.
                    //
                    if(g_lStatusPosition < 0)
                    {
                        g_sPositionConfig.lPosition =
                            (((g_lStatusPosition / 65536) * 100) +
                             ((((g_lStatusPosition % 65536) * 100) - 32768) /
                              65536));
                    }
                    else
                    {
                        g_sPositionConfig.lPosition =
                            (((g_lStatusPosition / 65536) * 100) +
                             ((((g_lStatusPosition % 65536) * 100) + 32768) /
                              65536));
                    }
                    CANPositionSet(((g_sPositionConfig.lPosition / 100) *
                                    65536) +
                                   (((g_sPositionConfig.lPosition % 100) *
                                     65536) / 100), 0);

                    //
                    // Indicate that demo mode has exited by setting the text
                    // color to white.
                    //
                    CanvasTextColorSet(g_psPositionWidgets + 2, ClrWhite);
                }

                //
                // Otherwise start demo mode.
                //
                else
                {
                    //
                    // Indicate that demo mode is active by setting the text
                    // color to gray.
                    //
                    CanvasTextColorSet(g_psPositionWidgets + 2, ClrSelected);

                    //
                    // Start with the first step.
                    //
                    ulStep = 0;

                    //
                    // Set the position as directed by the first step.
                    //
                    g_sPositionConfig.lPosition = g_plPositionDemo[0][0];
                    CANPositionSet(((g_sPositionConfig.lPosition / 100) *
                                    65536) +
                                   (((g_sPositionConfig.lPosition % 100) *
                                     65536) / 100), 0);

                    //
                    // Set the time delay for the first step.
                    //
                    ulTime = g_ulTickCount + g_plPositionDemo[0][1];
                }
            }

            //
            // See if the cursor is on the position selection.
            //
            else if(ulPos == 2)
            {
                //
                // Toggle the state of the delayed update.
                //
                ulDelay ^= 1;

                //
                // See if a delayed update should be performed.
                //
                if(ulDelay == 0)
                {
                    //
                    // Send the delayed position update.
                    //
                    CANPositionSet(((g_sPositionConfig.lPosition / 100) *
                                    65536) +
                                   (((g_sPositionConfig.lPosition % 100) *
                                     65536) / 100), 0);

                    //
                    // Change the text color of the position selection to white
                    // to indicate that updates will occur immediately.
                    //
                    CanvasTextColorSet(g_psPositionWidgets + 2, ClrWhite);
                }
                else
                {
                    //
                    // Change the text color of the position selection to black
                    // to indicate that updates will be delayed.
                    //
                    CanvasTextColorSet(g_psPositionWidgets + 2, ClrBlack);
                }
            }
        }
    }
}
