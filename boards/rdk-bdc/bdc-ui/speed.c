//*****************************************************************************
//
// speed.c - Displays the "Speed Control Mode" panel.
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
#include "speed.h"
#include "status.h"

//*****************************************************************************
//
// Buffers to contain the string representation of the current device ID,
// speed, P coefficient, I coefficient, and D coefficient.
//
//*****************************************************************************
static char g_pcIDBuffer[4];
static char g_pcSpeedBuffer[12];
static char g_pcSpeedPBuffer[12];
static char g_pcSpeedIBuffer[12];
static char g_pcSpeedDBuffer[12];
static char g_pcReferenceBuffer[16];

//*****************************************************************************
//
// The strings that represent the speed reference settings.
//
//*****************************************************************************
static char *g_ppcSpeedReference[] =
{
    "encoder",
    "inv encoder",
    "quad encoder"
};

//*****************************************************************************
//
// The widgets that make up the "Speed Control Mode" panel.
//
//*****************************************************************************
static tCanvasWidget g_psSpeedWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 "Speed Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 24, 12, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcIDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 42, 20, 60, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcSpeedBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 28, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcSpeedPBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 36, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcSpeedIBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 44, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcSpeedDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 30, 52, 84, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcReferenceBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 12, 18, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "ID:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 20, 36, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "Speed:",
                 0, 0),
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
// The number of widgets in the "Speed Control Mode" panel.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psSpeedWidgets) /                   \
                                 sizeof(g_psSpeedWidgets[0]))

//*****************************************************************************
//
// This structure contains the current configuration of the speed control mode
// of the motor controller.
//
//*****************************************************************************
static struct
{
    long lP;
    long lI;
    long lD;
    unsigned long ulSpeedRef;
}
g_sSpeedConfig;

//*****************************************************************************
//
// The speed commands sent to the motor by the demo mode.  The first member of
// each array is the speed and the second member is the amount of time (in
// milliseconds) to output that speed.
//
//*****************************************************************************
static long g_plSpeedDemo[][2] =
{
    { 240, 5000 },
    { 120, 5000 },
    { 180, 5000 },
    { 60, 5000 },
    { 0, 1000 },
    { -240, 5000 },
    { -120, 5000 },
    { -180, 5000 },
    { -60, 5000 },
    { 0, 1000 }
};

//*****************************************************************************
//
// Reads the configuration of the speed control mode of the current motor
// controller.
//
//*****************************************************************************
void
SpeedConfigRead(void)
{
    //
    // Read the PID controller's P coefficient.
    //
    if(CANReadParameter(LM_API_SPD_PC, 0,
                        (unsigned long *)&(g_sSpeedConfig.lP), 0) == 0)
    {
        g_sSpeedConfig.lP = 0;
    }
    else if(g_sSpeedConfig.lP < 0)
    {
        g_sSpeedConfig.lP = (((g_sSpeedConfig.lP / 65536) * 1000) +
                             ((((g_sSpeedConfig.lP % 65536) * 1000) - 32768) /
                              65536));
    }
    else
    {
        g_sSpeedConfig.lP = (((g_sSpeedConfig.lP / 65536) * 1000) +
                             ((((g_sSpeedConfig.lP % 65536) * 1000) + 32768) /
                              65536));
    }

    //
    // Read the PID controller's I coefficient.
    //
    if(CANReadParameter(LM_API_SPD_IC, 0,
                        (unsigned long *)&(g_sSpeedConfig.lI), 0) == 0)
    {
        g_sSpeedConfig.lI = 0;
    }
    else if(g_sSpeedConfig.lI < 0)
    {
        g_sSpeedConfig.lI = (((g_sSpeedConfig.lI / 65536) * 1000) +
                             ((((g_sSpeedConfig.lI % 65536) * 1000) - 32768) /
                              65536));
    }
    else
    {
        g_sSpeedConfig.lI = (((g_sSpeedConfig.lI / 65536) * 1000) +
                             ((((g_sSpeedConfig.lI % 65536) * 1000) + 32768) /
                              65536));
    }

    //
    // Read the PID controller's D coefficient.
    //
    if(CANReadParameter(LM_API_SPD_DC, 0,
                        (unsigned long *)&(g_sSpeedConfig.lD), 0) == 0)
    {
        g_sSpeedConfig.lD = 0;
    }
    else if(g_sSpeedConfig.lD < 0)
    {
        g_sSpeedConfig.lD = (((g_sSpeedConfig.lD / 65536) * 1000) +
                             ((((g_sSpeedConfig.lD % 65536) * 1000) - 32768) /
                              65536));
    }
    else
    {
        g_sSpeedConfig.lD = (((g_sSpeedConfig.lD / 65536) * 1000) +
                             ((((g_sSpeedConfig.lD % 65536) * 1000) + 32768) /
                              65536));
    }

    //
    // Read the speed reference source.
    //
    if(CANReadParameter(LM_API_SPD_REF, 0, &(g_sSpeedConfig.ulSpeedRef),
                        0) == 0)
    {
        g_sSpeedConfig.ulSpeedRef = 0;
    }
    else
    {
        g_sSpeedConfig.ulSpeedRef &= 3;
        if(g_sSpeedConfig.ulSpeedRef)
        {
            g_sSpeedConfig.ulSpeedRef--;
        }
    }
}

//*****************************************************************************
//
// Displays the "Speed Control Mode" panel.  The returned valud is the ID of
// the panel to be displayed instead of the "Speed Control Mode" panel.
//
//*****************************************************************************
unsigned long
DisplaySpeed(void)
{
    unsigned long ulPos, ulIdx, ulDelay, ulDemo, ulTime, ulStep;
    long lSpeed;

    //
    // Enable speed control mode.
    //
    CANSpeedModeEnable();

    //
    // Set the default speed and PID coefficients.
    //
    lSpeed = 0;
    CANSpeedSet(0, 0);

    //
    // Read the current speed mode configuration.
    //
    SpeedConfigRead();

    //
    // Initially, updates to the speed occur immediately.
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
    for(ulIdx = 0; ulIdx < 6; ulIdx++)
    {
        CanvasFillOff(g_psSpeedWidgets + ulIdx);
    }
    CanvasFillOn(g_psSpeedWidgets + 1);

    //
    // Add the "Speed Control Mode" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psSpeedWidgets + ulIdx));
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
        // Print out the current speed.
        //
        if(lSpeed < 0)
        {
            usnprintf(g_pcSpeedBuffer, sizeof(g_pcSpeedBuffer), "-%d rpm",
                      0 - lSpeed);
        }
        else
        {
            usnprintf(g_pcSpeedBuffer, sizeof(g_pcSpeedBuffer), "%d rpm",
                      lSpeed);
        }

        //
        // Print out the current P coefficient.
        //
        if(g_sSpeedConfig.lP < 0)
        {
            usnprintf(g_pcSpeedPBuffer, sizeof(g_pcSpeedPBuffer), "-%d.%03d",
                      (0 - g_sSpeedConfig.lP) / 1000,
                      (0 - g_sSpeedConfig.lP) % 1000);
        }
        else
        {
            usnprintf(g_pcSpeedPBuffer, sizeof(g_pcSpeedPBuffer), "%d.%03d",
                      g_sSpeedConfig.lP / 1000, g_sSpeedConfig.lP % 1000);
        }

        //
        // Irint out the current I coefficient.
        //
        if(g_sSpeedConfig.lI < 0)
        {
            usnprintf(g_pcSpeedIBuffer, sizeof(g_pcSpeedIBuffer), "-%d.%03d",
                      (0 - g_sSpeedConfig.lI) / 1000,
                      (0 - g_sSpeedConfig.lI) % 1000);
        }
        else
        {
            usnprintf(g_pcSpeedIBuffer, sizeof(g_pcSpeedIBuffer), "%d.%03d",
                      g_sSpeedConfig.lI / 1000, g_sSpeedConfig.lI % 1000);
        }

        //
        // Print out the current D coefficient.
        //
        if(g_sSpeedConfig.lD < 0)
        {
            usnprintf(g_pcSpeedDBuffer, sizeof(g_pcSpeedDBuffer), "-%d.%03d",
                      (0 - g_sSpeedConfig.lD) / 1000,
                      (0 - g_sSpeedConfig.lD) % 1000);
        }
        else
        {
            usnprintf(g_pcSpeedDBuffer, sizeof(g_pcSpeedDBuffer), "%d.%03d",
                      g_sSpeedConfig.lD / 1000, g_sSpeedConfig.lD % 1000);
        }

        //
        // Print out the current speed reference source.
        //
        usnprintf(g_pcReferenceBuffer, sizeof(g_pcReferenceBuffer), "%s",
                  g_ppcSpeedReference[g_sSpeedConfig.ulSpeedRef]);

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
            // Remove the "Speed Control Mode" panel widgets.
            //
            for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
            {
                WidgetRemove((tWidget *)(g_psSpeedWidgets + ulIdx));
            }
            CanvasTextColorSet(g_psSpeedWidgets + 2, ClrWhite);

            //
            // Set the output speed to zero.
            //
            CANSpeedSet(0, 0);

            //
            // Disable speed control mode.
            //
            CANSpeedModeDisable();

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
                if(ulStep == (sizeof(g_plSpeedDemo) /
                              sizeof(g_plSpeedDemo[0])))
                {
                    ulStep = 0;
                }

                //
                // Set the speed as directed by the next step.
                //
                lSpeed = g_plSpeedDemo[ulStep][0];
                CANSpeedSet(lSpeed * 65536, 0);

                //
                // Set the time delay for this step.
                //
                ulTime = g_ulTickCount + g_plSpeedDemo[ulStep][1];
            }
        }

        //
        // See if the up button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the top of the
            // screen and a delayed speed update is not in progress.
            //
            if((ulPos != 0) && (ulDelay == 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psSpeedWidgets + ulPos);

                //
                // Decrement the cursor row, skipping the speed row when demo
                // mode is enabled.
                //
                ulPos--;
                if((ulPos == 2) && (ulDemo != 0))
                {
                    ulPos--;
                }

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psSpeedWidgets + ulPos);
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
            // screen and a delayed speed update is not in progress.
            //
            if((ulPos != 6) && (ulDelay == 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psSpeedWidgets + ulPos);

                //
                // Increment the cursor row, skipping the speed row when demo
                // mode is enabled.
                //
                ulPos++;
                if((ulPos == 2) && (ulDemo != 0))
                {
                    ulPos++;
                }

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psSpeedWidgets + ulPos);
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
                    CanvasTextColorSet(g_psSpeedWidgets + 2, ClrWhite);

                    //
                    // Set the speed to 0 for the current device ID.
                    //
                    CANSpeedSet(0, 0);
                    CANSpeedModeDisable();

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
                    // Enable speed control mode.
                    //
                    CANSpeedModeEnable();

                    //
                    // Set the speed for the new device.
                    //
                    lSpeed = 0;
                    CANSpeedSet(0, 0);

                    //
                    // Read the configuration of the new device.
                    //
                    SpeedConfigRead();
                }
            }

            //
            // See if the speed is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the speed if it is not already full reverse.
                //
                if(lSpeed > -20000)
                {
                    //
                    // Decrement the speed.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        lSpeed -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        lSpeed -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        lSpeed -= 1111;
                    }
                    else
                    {
                        lSpeed--;
                    }
                    if(lSpeed < -20000)
                    {
                        lSpeed = -20000;
                    }

                    //
                    // Send the updated speed to the motor controller if a
                    // delayed update is not in progress.
                    //
                    if(ulDelay == 0)
                    {
                        CANSpeedSet(lSpeed * 65536, 0);
                    }
                }
            }

            //
            // See if the speed P gain is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the P gain if it is not already fully negative.
                //
                if(g_sSpeedConfig.lP > (-32767 * 1000))
                {
                    //
                    // Decrement the P gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sSpeedConfig.lP -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sSpeedConfig.lP -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sSpeedConfig.lP -= 1111;
                    }
                    else
                    {
                        g_sSpeedConfig.lP--;
                    }
                    if(g_sSpeedConfig.lP < (-32767 * 1000))
                    {
                        g_sSpeedConfig.lP = -32767 * 1000;
                    }

                    //
                    // Send the new P gain to the motor controller.
                    //
                    CANSpeedPGainSet(((g_sSpeedConfig.lP / 1000) * 65536) +
                                     (((g_sSpeedConfig.lP % 1000) * 65536) /
                                      1000));
                }
            }

            //
            // See if the speed I gain is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Only change the I gain if it is not already fully negative.
                //
                if(g_sSpeedConfig.lI > (-32767 * 1000))
                {
                    //
                    // Decrement the I gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sSpeedConfig.lI -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sSpeedConfig.lI -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sSpeedConfig.lI -= 1111;
                    }
                    else
                    {
                        g_sSpeedConfig.lI--;
                    }
                    if(g_sSpeedConfig.lI < (-32767 * 1000))
                    {
                        g_sSpeedConfig.lI = -32767 * 1000;
                    }

                    //
                    // Send the new I gain to the motor controller.
                    //
                    CANSpeedIGainSet(((g_sSpeedConfig.lI / 1000) * 65536) +
                                     (((g_sSpeedConfig.lI % 1000) * 65536) /
                                      1000));
                }
            }

            //
            // See if the speed D gain is being changed.
            //
            else if(ulPos == 5)
            {
                //
                // Only change the D gain if it is not already fully negative.
                //
                if(g_sSpeedConfig.lD > (-32767 * 1000))
                {
                    //
                    // Decrement the D gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sSpeedConfig.lD -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sSpeedConfig.lD -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sSpeedConfig.lD -= 1111;
                    }
                    else
                    {
                        g_sSpeedConfig.lD--;
                    }
                    if(g_sSpeedConfig.lD < (-32767 * 1000))
                    {
                        g_sSpeedConfig.lD = -32767 * 1000;
                    }

                    //
                    // Send the new D gain to the motor controller.
                    //
                    CANSpeedDGainSet(((g_sSpeedConfig.lD / 1000) * 65536) +
                                     (((g_sSpeedConfig.lD % 1000) * 65536) /
                                      1000));
                }
            }

            //
            // See if the speed reference source is being changed.
            //
            else if(ulPos == 6)
            {
                //
                // Decrement the speed reference index.
                //
                if(g_sSpeedConfig.ulSpeedRef == 0)
                {
                    g_sSpeedConfig.ulSpeedRef = 2;
                }
                else
                {
                    g_sSpeedConfig.ulSpeedRef--;
                }

                //
                // Send the speed reference source to the motor controller.
                //
                CANSpeedRefSet(g_sSpeedConfig.ulSpeedRef ?
                               (g_sSpeedConfig.ulSpeedRef + 1) : 0);
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
                    CanvasTextColorSet(g_psSpeedWidgets + 2, ClrWhite);

                    //
                    // Set the speed to 0 for the current device ID.
                    //
                    CANSpeedSet(0, 0);
                    CANSpeedModeDisable();

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
                    // Enable speed control mode.
                    //
                    CANSpeedModeEnable();

                    //
                    // Set the speed for the new device.
                    //
                    lSpeed = 0;
                    CANSpeedSet(0, 0);

                    //
                    // Read the configuration of the new device.
                    //
                    SpeedConfigRead();
                }
            }

            //
            // See if the speed is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the speed if it is not already full forward.
                //
                if(lSpeed < 20000)
                {
                    //
                    // Increment the speed.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        lSpeed += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        lSpeed += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        lSpeed += 1111;
                    }
                    else
                    {
                        lSpeed++;
                    }
                    if(lSpeed > 20000)
                    {
                        lSpeed = 20000;
                    }

                    //
                    // Send the updated speed to the motor controller if a
                    // delayed update is not in progress.
                    //
                    if(ulDelay == 0)
                    {
                        CANSpeedSet(lSpeed * 65536, 0);
                    }
                }
            }

            //
            // See if the speed P gain is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the P gain if it is not already fully positive.
                //
                if(g_sSpeedConfig.lP < (32767 * 1000))
                {
                    //
                    // Increment the P gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sSpeedConfig.lP += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sSpeedConfig.lP += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sSpeedConfig.lP += 1111;
                    }
                    else
                    {
                        g_sSpeedConfig.lP++;
                    }
                    if(g_sSpeedConfig.lP > (32767 * 1000))
                    {
                        g_sSpeedConfig.lP = 32767 * 1000;
                    }

                    //
                    // Send the new P gain to the motor controller.
                    //
                    CANSpeedPGainSet(((g_sSpeedConfig.lP / 1000) * 65536) +
                                     (((g_sSpeedConfig.lP % 1000) * 65536) /
                                      1000));
                }
            }

            //
            // See if the speed I gain is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Only change the I gain if it is not already fully positive.
                //
                if(g_sSpeedConfig.lI < (32767 * 1000))
                {
                    //
                    // Increment the I gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sSpeedConfig.lI += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sSpeedConfig.lI += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sSpeedConfig.lI += 1111;
                    }
                    else
                    {
                        g_sSpeedConfig.lI++;
                    }
                    if(g_sSpeedConfig.lI > (32767 * 1000))
                    {
                        g_sSpeedConfig.lI = 32767 * 1000;
                    }

                    //
                    // Send the new I gain to the motor controller.
                    //
                    CANSpeedIGainSet(((g_sSpeedConfig.lI / 1000) * 65536) +
                                     (((g_sSpeedConfig.lI % 1000) * 65536) /
                                      1000));
                }
            }

            //
            // See if the speed D gain is being changed.
            //
            else if(ulPos == 5)
            {
                //
                // Only change the D gain if it is not already fully positive.
                //
                if(g_sSpeedConfig.lD < (32767 * 1000))
                {
                    //
                    // Increment the D gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sSpeedConfig.lD += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sSpeedConfig.lD += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sSpeedConfig.lD += 1111;
                    }
                    else
                    {
                        g_sSpeedConfig.lD++;
                    }
                    if(g_sSpeedConfig.lD > (32767 * 1000))
                    {
                        g_sSpeedConfig.lD = 32767 * 1000;
                    }

                    //
                    // Send the new D gain to the motor controller.
                    //
                    CANSpeedDGainSet(((g_sSpeedConfig.lD / 1000) * 65536) +
                                     (((g_sSpeedConfig.lD % 1000) * 65536) /
                                      1000));
                }
            }

            //
            // See if the speed reference source is being changed.
            //
            else if(ulPos == 6)
            {
                //
                // Decrement the speed reference index.
                //
                if(g_sSpeedConfig.ulSpeedRef == 2)
                {
                    g_sSpeedConfig.ulSpeedRef = 0;
                }
                else
                {
                    g_sSpeedConfig.ulSpeedRef++;
                }

                //
                // Send the speed reference source to the motor controller.
                //
                CANSpeedRefSet(g_sSpeedConfig.ulSpeedRef ?
                               (g_sSpeedConfig.ulSpeedRef + 1) : 0);
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
                ulIdx = DisplayMenu(PANEL_SPEED);

                //
                // See if another panel was selected.
                //
                if(ulIdx != PANEL_SPEED)
                {
                    //
                    // Disable the status display.
                    //
                    StatusDisable();

                    //
                    // Remove the "Speed Control Mode" panel widgets.
                    //
                    for(ulPos = 0; ulPos < NUM_WIDGETS; ulPos++)
                    {
                        WidgetRemove((tWidget *)(g_psSpeedWidgets + ulPos));
                    }
                    CanvasTextColorSet(g_psSpeedWidgets + 2, ClrWhite);

                    //
                    // Set the output speed to zero.
                    //
                    CANSpeedSet(0, 0);

                    //
                    // Disable speed control mode.
                    //
                    CANSpeedModeDisable();

                    //
                    // Return the ID of the newly selected panel.
                    //
                    return(ulIdx);
                }

                //
                // Since the "Speed Control Mode" panel was selected from the
                // menu, move the cursor down one row.
                //
                CanvasFillOff(g_psSpeedWidgets);
                ulPos++;
                CanvasFillOn(g_psSpeedWidgets + 1);
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
                    // Set the output speed to zero.
                    //
                    lSpeed = 0;
                    CANSpeedSet(0, 0);

                    //
                    // Indicate that demo mode has exited by setting the text
                    // color to white.
                    //
                    CanvasTextColorSet(g_psSpeedWidgets + 2, ClrWhite);
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
                    CanvasTextColorSet(g_psSpeedWidgets + 2, ClrSelected);

                    //
                    // Start with the first step.
                    //
                    ulStep = 0;

                    //
                    // Set the speed as directed by the first step.
                    //
                    lSpeed = g_plSpeedDemo[0][0];
                    CANSpeedSet(lSpeed * 65536, 0);

                    //
                    // Set the time delay for the first step.
                    //
                    ulTime = g_ulTickCount + g_plSpeedDemo[0][1];
                }
            }

            //
            // See if the cursor is on the speed selection.
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
                    // Send the delayed speed update.
                    //
                    CANSpeedSet(lSpeed * 65536, 0);

                    //
                    // Change the text color of the speed selection to white to
                    // indicate that updates will occur immediately.
                    //
                    CanvasTextColorSet(g_psSpeedWidgets + 2, ClrWhite);
                }
                else
                {
                    //
                    // Change the text color of the speed selection to black to
                    // indicate that updates will be delayed.
                    //
                    CanvasTextColorSet(g_psSpeedWidgets + 2, ClrBlack);
                }
            }
        }
    }
}
