//*****************************************************************************
//
// current.c - Displays the "Current Control Mode" panel.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the RDK-BDC Firmware Package.
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
#include "current.h"
#include "status.h"

//*****************************************************************************
//
// Buffers to contain the string representation of the current device ID,
// current, P coefficient, I coefficient, and D coefficient.
//
//*****************************************************************************
static char g_pcIDBuffer[4];
static char g_pcCurrentBuffer[12];
static char g_pcCurrentPBuffer[12];
static char g_pcCurrentIBuffer[12];
static char g_pcCurrentDBuffer[12];

//*****************************************************************************
//
// The widgets that make up the "Current Control Mode" panel.
//
//*****************************************************************************
static tCanvasWidget g_psCurrentWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 "Current Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 24, 16, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcIDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 54, 24, 54, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcCurrentBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 32, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcCurrentPBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 40, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcCurrentIBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 18, 48, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcCurrentDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 16, 18, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "ID:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 24, 48, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Current:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 32, 12, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "P:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 40, 12, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "I:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 48, 12, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "D:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 9, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the "Current Control Mode" panel.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psCurrentWidgets) /                \
                                 sizeof(g_psCurrentWidgets[0]))

//*****************************************************************************
//
// This structure contains the current configuration of the current control
// mode of the motor controller.
//
//*****************************************************************************
static struct
{
    long lP;
    long lI;
    long lD;
    unsigned long ulPosRef;
}
g_sCurrentConfig;

//*****************************************************************************
//
// The current commands sent to the motor by the demo mode.  The first member
// of each array is the current and the second member is the amount of time
// (in milliseconds) to output that current.
//
//*****************************************************************************
static long g_plCurrentDemo[][2] =
{
    { 340, 5000 },
    { 240, 5000 },
    { 290, 5000 },
    { 190, 5000 },
    { 0, 1000 },
    { -340, 5000 },
    { -240, 5000 },
    { -290, 5000 },
    { -190, 5000 },
    { 0, 1000 }
};

//*****************************************************************************
//
// Reads the configuration of the current control mode of the current motor
// controller.
//
//*****************************************************************************
void
CurrentConfigRead(void)
{
    //
    // Read the PID controller's P coefficient.
    //
    if(CANReadParameter(LM_API_ICTRL_PC, 0,
                        (unsigned long *)&(g_sCurrentConfig.lP), 0) == 0)
    {
        g_sCurrentConfig.lP = 0;
    }
    else if(g_sCurrentConfig.lP < 0)
    {
        g_sCurrentConfig.lP = (((g_sCurrentConfig.lP / 65536) * 1000) +
                             ((((g_sCurrentConfig.lP % 65536) * 1000) -
                               32768) / 65536));
    }
    else
    {
        g_sCurrentConfig.lP = (((g_sCurrentConfig.lP / 65536) * 1000) +
                             ((((g_sCurrentConfig.lP % 65536) * 1000) +
                               32768) / 65536));
    }

    //
    // Read the PID controller's I coefficient.
    //
    if(CANReadParameter(LM_API_ICTRL_IC, 0,
                        (unsigned long *)&(g_sCurrentConfig.lI), 0) == 0)
    {
        g_sCurrentConfig.lI = 0;
    }
    else if(g_sCurrentConfig.lI < 0)
    {
        g_sCurrentConfig.lI = (((g_sCurrentConfig.lI / 65536) * 1000) +
                             ((((g_sCurrentConfig.lI % 65536) * 1000) -
                               32768) / 65536));
    }
    else
    {
        g_sCurrentConfig.lI = (((g_sCurrentConfig.lI / 65536) * 1000) +
                             ((((g_sCurrentConfig.lI % 65536) * 1000) +
                               32768) / 65536));
    }

    //
    // Read the PID controller's D coefficient.
    //
    if(CANReadParameter(LM_API_ICTRL_DC, 0,
                        (unsigned long *)&(g_sCurrentConfig.lD), 0) == 0)
    {
        g_sCurrentConfig.lD = 0;
    }
    else if(g_sCurrentConfig.lD < 0)
    {
        g_sCurrentConfig.lD = (((g_sCurrentConfig.lD / 65536) * 1000) +
                             ((((g_sCurrentConfig.lD % 65536) * 1000) -
                               32768) / 65536));
    }
    else
    {
        g_sCurrentConfig.lD = (((g_sCurrentConfig.lD / 65536) * 1000) +
                             ((((g_sCurrentConfig.lD % 65536) * 1000) +
                               32768) / 65536));
    }
}

//*****************************************************************************
//
// Displays the "Current Control Mode" panel.  The returned valud is the ID of
// the panel to be displayed instead of the "Current Control Mode" panel.
//
//*****************************************************************************
unsigned long
DisplayCurrent(void)
{
    unsigned long ulPos, ulIdx, ulDelay, ulDemo, ulTime, ulStep;
    long lCurrent;

    //
    // Enable current control mode.
    //
    CANCurrentModeEnable();

    //
    // Set the default current and PID coefficients.
    //
    lCurrent = 0;
    CANCurrentSet(0, 0);

    //
    // Read the current current mode configuration.
    //
    CurrentConfigRead();

    //
    // Initially, updates to the current occur immediately.
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
        CanvasFillOff(g_psCurrentWidgets + ulIdx);
    }
    CanvasFillOn(g_psCurrentWidgets + 1);

    //
    // Add the "Current Control Mode" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psCurrentWidgets + ulIdx));
    }

    //
    // Enable the status display.
    //
    StatusEnable(0);

    //
    // Set the default cursor current to the device ID selection.
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
        // Print out the current current.
        //
        if(lCurrent < 0)
        {
            usnprintf(g_pcCurrentBuffer, sizeof(g_pcCurrentBuffer), "-%d.%02d",
                      (0 - lCurrent) / 100, (0 - lCurrent) % 100);
        }
        else
        {
            usnprintf(g_pcCurrentBuffer, sizeof(g_pcCurrentBuffer), "%d.%02d",
                      lCurrent / 100, lCurrent % 100);
        }

        //
        // Print out the current P coefficient.
        //
        if(g_sCurrentConfig.lP < 0)
        {
            usnprintf(g_pcCurrentPBuffer, sizeof(g_pcCurrentPBuffer),
                      "-%d.%03d", (0 - g_sCurrentConfig.lP) / 1000,
                      (0 - g_sCurrentConfig.lP) % 1000);
        }
        else
        {
            usnprintf(g_pcCurrentPBuffer, sizeof(g_pcCurrentPBuffer),
                      "%d.%03d", g_sCurrentConfig.lP / 1000,
                      g_sCurrentConfig.lP % 1000);
        }

        //
        // Irint out the current I coefficient.
        //
        if(g_sCurrentConfig.lI < 0)
        {
            usnprintf(g_pcCurrentIBuffer, sizeof(g_pcCurrentIBuffer),
                      "-%d.%03d", (0 - g_sCurrentConfig.lI) / 1000,
                      (0 - g_sCurrentConfig.lI) % 1000);
        }
        else
        {
            usnprintf(g_pcCurrentIBuffer, sizeof(g_pcCurrentIBuffer),
                      "%d.%03d", g_sCurrentConfig.lI / 1000,
                      g_sCurrentConfig.lI % 1000);
        }

        //
        // Print out the current D coefficient.
        //
        if(g_sCurrentConfig.lD < 0)
        {
            usnprintf(g_pcCurrentDBuffer, sizeof(g_pcCurrentDBuffer),
                      "-%d.%03d", (0 - g_sCurrentConfig.lD) / 1000,
                      (0 - g_sCurrentConfig.lD) % 1000);
        }
        else
        {
            usnprintf(g_pcCurrentDBuffer, sizeof(g_pcCurrentDBuffer),
                      "%d.%03d", g_sCurrentConfig.lD / 1000,
                      g_sCurrentConfig.lD % 1000);
        }

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
            // Remove the "Current Control Mode" panel widgets.
            //
            for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
            {
                WidgetRemove((tWidget *)(g_psCurrentWidgets + ulIdx));
            }
            CanvasTextColorSet(g_psCurrentWidgets + 2, ClrWhite);

            //
            // Set the output current to zero.
            //
            CANCurrentSet(0, 0);

            //
            // Disable current control mode.
            //
            CANCurrentModeDisable();

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
                if(ulStep == (sizeof(g_plCurrentDemo) /
                              sizeof(g_plCurrentDemo[0])))
                {
                    ulStep = 0;
                }

                //
                // Set the current as directed by the next step.
                //
                lCurrent = g_plCurrentDemo[ulStep][0];
                CANCurrentSet((lCurrent * 256) / 100, 0);

                //
                // Set the time delay for this step.
                //
                ulTime = g_ulTickCount + g_plCurrentDemo[ulStep][1];
            }
        }

        //
        // See if the up button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the top of the
            // screen and a delayed current update is not in progress.
            //
            if((ulPos != 0) && (ulDelay == 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psCurrentWidgets + ulPos);

                //
                // Decrement the cursor row, skipping the current row when
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
                CanvasFillOn(g_psCurrentWidgets + ulPos);
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
            // screen and a delayed current update is not in progress.
            //
            if((ulPos != 5) && (ulDelay == 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psCurrentWidgets + ulPos);

                //
                // Increment the cursor row, skipping the current row when
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
                CanvasFillOn(g_psCurrentWidgets + ulPos);
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
                    CanvasTextColorSet(g_psCurrentWidgets + 2, ClrWhite);

                    //
                    // Set the current to 0 for the current device ID.
                    //
                    CANCurrentSet(0, 0);
                    CANCurrentModeDisable();

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
                    // Enable current control mode.
                    //
                    CANCurrentModeEnable();

                    //
                    // Set the current for the new device.
                    //
                    lCurrent = 0;
                    CANCurrentSet(0, 0);

                    //
                    // Read the configuration of the new device.
                    //
                    CurrentConfigRead();
                }
            }

            //
            // See if the current is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the current if it is not already full reverse.
                //
                if(lCurrent > -4000)
                {
                    //
                    // Decrement the current.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        lCurrent -= 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1))
                    {
                        lCurrent -= 111;
                    }
                    else
                    {
                        lCurrent--;
                    }
                    if(lCurrent < -4000)
                    {
                        lCurrent = -4000;
                    }

                    //
                    // Send the updated current to the motor controller if a
                    // delayed update is not in progress.
                    //
                    if(ulDelay == 0)
                    {
                        CANCurrentSet((lCurrent * 256) / 100, 0);
                    }
                }
            }

            //
            // See if the current P gain is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the P gain if it is not already fully negative.
                //
                if(g_sCurrentConfig.lP > (-32767 * 1000))
                {
                    //
                    // Decrement the P gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sCurrentConfig.lP -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sCurrentConfig.lP -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sCurrentConfig.lP -= 1111;
                    }
                    else
                    {
                        g_sCurrentConfig.lP--;
                    }
                    if(g_sCurrentConfig.lP < (-32767 * 1000))
                    {
                        g_sCurrentConfig.lP = -32767 * 1000;
                    }

                    //
                    // Send the new P gain to the motor controller.
                    //
                    CANCurrentPGainSet(((g_sCurrentConfig.lP / 1000) *
                                         65536) +
                                        (((g_sCurrentConfig.lP % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the current I gain is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Only change the I gain if it is not already fully negative.
                //
                if(g_sCurrentConfig.lI > (-32767 * 1000))
                {
                    //
                    // Decrement the I gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sCurrentConfig.lI -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sCurrentConfig.lI -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sCurrentConfig.lI -= 1111;
                    }
                    else
                    {
                        g_sCurrentConfig.lI--;
                    }
                    if(g_sCurrentConfig.lI < (-32767 * 1000))
                    {
                        g_sCurrentConfig.lI = -32767 * 1000;
                    }

                    //
                    // Send the new I gain to the motor controller.
                    //
                    CANCurrentIGainSet(((g_sCurrentConfig.lI / 1000) *
                                         65536) +
                                        (((g_sCurrentConfig.lI % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the current D gain is being changed.
            //
            else if(ulPos == 5)
            {
                //
                // Only change the D gain if it is not already fully negative.
                //
                if(g_sCurrentConfig.lD > (-32767 * 1000))
                {
                    //
                    // Decrement the D gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sCurrentConfig.lD -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sCurrentConfig.lD -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sCurrentConfig.lD -= 1111;
                    }
                    else
                    {
                        g_sCurrentConfig.lD--;
                    }
                    if(g_sCurrentConfig.lD < (-32767 * 1000))
                    {
                        g_sCurrentConfig.lD = -32767 * 1000;
                    }

                    //
                    // Send the new D gain to the motor controller.
                    //
                    CANCurrentDGainSet(((g_sCurrentConfig.lD / 1000) *
                                         65536) +
                                        (((g_sCurrentConfig.lD % 1000) *
                                          65536) / 1000));
                }
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
                    CanvasTextColorSet(g_psCurrentWidgets + 2, ClrWhite);

                    //
                    // Set the current to 0 for the current device ID.
                    //
                    CANCurrentSet(0, 0);
                    CANCurrentModeDisable();

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
                    // Enable current control mode.
                    //
                    CANCurrentModeEnable();

                    //
                    // Set the current for the new device.
                    //
                    lCurrent = 0;
                    CANCurrentSet(0, 0);

                    //
                    // Read the configuration of the new device.
                    //
                    CurrentConfigRead();
                }
            }

            //
            // See if the current is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the current if it is not already full forward.
                //
                if(lCurrent < 4000)
                {
                    //
                    // Increment the current.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        lCurrent += 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1))
                    {
                        lCurrent += 111;
                    }
                    else
                    {
                        lCurrent++;
                    }
                    if(lCurrent > 4000)
                    {
                        lCurrent = 4000;
                    }

                    //
                    // Send the updated current to the motor controller if a
                    // delayed update is not in progress.
                    //
                    if(ulDelay == 0)
                    {
                        CANCurrentSet((lCurrent * 256) / 100, 0);
                    }
                }
            }

            //
            // See if the current P gain is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the P gain if it is not already fully positive.
                //
                if(g_sCurrentConfig.lP < (32767 * 1000))
                {
                    //
                    // Increment the P gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sCurrentConfig.lP += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sCurrentConfig.lP += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sCurrentConfig.lP += 1111;
                    }
                    else
                    {
                        g_sCurrentConfig.lP++;
                    }
                    if(g_sCurrentConfig.lP > (32767 * 1000))
                    {
                        g_sCurrentConfig.lP = 32767 * 1000;
                    }

                    //
                    // Send the new P gain to the motor controller.
                    //
                    CANCurrentPGainSet(((g_sCurrentConfig.lP / 1000) *
                                         65536) +
                                        (((g_sCurrentConfig.lP % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the current I gain is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Only change the I gain if it is not already fully positive.
                //
                if(g_sCurrentConfig.lI < (32767 * 1000))
                {
                    //
                    // Increment the I gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sCurrentConfig.lI += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sCurrentConfig.lI += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sCurrentConfig.lI += 1111;
                    }
                    else
                    {
                        g_sCurrentConfig.lI++;
                    }
                    if(g_sCurrentConfig.lI > (32767 * 1000))
                    {
                        g_sCurrentConfig.lI = 32767 * 1000;
                    }

                    //
                    // Send the new I gain to the motor controller.
                    //
                    CANCurrentIGainSet(((g_sCurrentConfig.lI / 1000) *
                                         65536) +
                                        (((g_sCurrentConfig.lI % 1000) *
                                          65536) / 1000));
                }
            }

            //
            // See if the current D gain is being changed.
            //
            else if(ulPos == 5)
            {
                //
                // Only change the D gain if it is not already fully positive.
                //
                if(g_sCurrentConfig.lD < (32767 * 1000))
                {
                    //
                    // Increment the D gain.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sCurrentConfig.lD += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sCurrentConfig.lD += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sCurrentConfig.lD += 1111;
                    }
                    else
                    {
                        g_sCurrentConfig.lD++;
                    }
                    if(g_sCurrentConfig.lD > (32767 * 1000))
                    {
                        g_sCurrentConfig.lD = 32767 * 1000;
                    }

                    //
                    // Send the new D gain to the motor controller.
                    //
                    CANCurrentDGainSet(((g_sCurrentConfig.lD / 1000) *
                                         65536) +
                                        (((g_sCurrentConfig.lD % 1000) *
                                          65536) / 1000));
                }
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
                ulIdx = DisplayMenu(PANEL_CURRENT);

                //
                // See if another panel was selected.
                //
                if(ulIdx != PANEL_CURRENT)
                {
                    //
                    // Disable the status display.
                    //
                    StatusDisable();

                    //
                    // Remove the "Current Control Mode" panel widgets.
                    //
                    for(ulPos = 0; ulPos < NUM_WIDGETS; ulPos++)
                    {
                        WidgetRemove((tWidget *)(g_psCurrentWidgets + ulPos));
                    }
                    CanvasTextColorSet(g_psCurrentWidgets + 2, ClrWhite);

                    //
                    // Set the output current to zero.
                    //
                    CANCurrentSet(0, 0);

                    //
                    // Disable current control mode.
                    //
                    CANCurrentModeDisable();

                    //
                    // Return the ID of the newly selected panel.
                    //
                    return(ulIdx);
                }

                //
                // Since the "Current Control Mode" panel was selected from
                // the menu, move the cursor down one row.
                //
                CanvasFillOff(g_psCurrentWidgets);
                ulPos++;
                CanvasFillOn(g_psCurrentWidgets + 1);
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
                    // Set the output current to zero.
                    //
                    lCurrent = 0;
                    CANCurrentSet(0, 0);

                    //
                    // Indicate that demo mode has exited by setting the text
                    // color to white.
                    //
                    CanvasTextColorSet(g_psCurrentWidgets + 2, ClrWhite);
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
                    CanvasTextColorSet(g_psCurrentWidgets + 2, ClrSelected);

                    //
                    // Start with the first step.
                    //
                    ulStep = 0;

                    //
                    // Set the current as directed by the first step.
                    //
                    lCurrent = g_plCurrentDemo[0][0];
                    CANCurrentSet((lCurrent * 256) / 100, 0);

                    //
                    // Set the time delay for the first step.
                    //
                    ulTime = g_ulTickCount + g_plCurrentDemo[0][1];
                }
            }

            //
            // See if the cursor is on the current selection.
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
                    // Send the delayed current update.
                    //
                    CANCurrentSet((lCurrent * 256) / 100, 0);

                    //
                    // Change the text color of the current selection to white
                    // to indicate that updates will occur immediately.
                    //
                    CanvasTextColorSet(g_psCurrentWidgets + 2, ClrWhite);
                }
                else
                {
                    //
                    // Change the text color of the current selection to black
                    // to indicate that updates will be delayed.
                    //
                    CanvasTextColorSet(g_psCurrentWidgets + 2, ClrBlack);
                }
            }
        }
    }
}
