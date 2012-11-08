//*****************************************************************************
//
// vcomp.c - Displays the "VComp Control Mode" panel.
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
#include "status.h"
#include "vcomp.h"

//*****************************************************************************
//
// Buffers to contain the string representation of the current device ID,
// voltage, ramp rate, and compensation rate.
//
//*****************************************************************************
static char g_pcIDBuffer[4];
static char g_pcVoltageBuffer[12];
static char g_pcRampBuffer[12];
static char g_pcCompBuffer[12];

//*****************************************************************************
//
// The widgets that make up the "VComp Control Mode" panel.
//
//*****************************************************************************
static tCanvasWidget g_psVCompWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 "VComp Control Mode", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 24, 16, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcIDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 54, 27, 48, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcVoltageBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 36, 38, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcRampBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 36, 49, 66, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcCompBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 16, 18, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "ID:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 27, 48, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Voltage:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 38, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Ramp:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 49, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Comp:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 9, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the "VComp Control Mode" panel.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psVCompWidgets) /                   \
                                 sizeof(g_psVCompWidgets[0]))

//*****************************************************************************
//
// The voltage commands sent to the motor by the demo mode.  The first member
// of each array is the voltage and the second member is the amount of time (in
// milliseconds) to output that voltage.
//
//*****************************************************************************
static long g_plVCompDemo[][2] =
{
    { 120, 5000 },
    { 60, 5000 },
    { 90, 5000 },
    { 30, 5000 },
    { 0, 1000 },
    { -120, 5000 },
    { -60, 5000 },
    { -90, 5000 },
    { -30, 5000 },
    { 0, 1000 }
};

//*****************************************************************************
//
// Displays the "VComp Control Mode" panel.  The returned valud is the ID of
// the panel to be displayed instead of the "VComp Control Mode" panel.
//
//*****************************************************************************
unsigned long
DisplayVComp(void)
{
    unsigned long ulRamp, ulComp, ulPos, ulIdx, ulDelay, ulDemo, ulTime;
    unsigned long ulStep;
    long lVoltage;

    //
    // Enable voltage compensation control mode.
    //
    CANVCompModeEnable();

    //
    // Set the default voltage.
    //
    lVoltage = 0;
    CANVCompSet(0, 0);

    //
    // Read the ramp rate.
    //
    if(CANReadParameter(LM_API_VCOMP_IN_RAMP, 0, &ulRamp, 0) == 0)
    {
        ulRamp = 0;
    }
    else
    {
        ulRamp = (((ulRamp & 0xffff) * 100) + 128) / 256;
    }

    //
    // Read the compensation rate.
    //
    if(CANReadParameter(LM_API_VCOMP_COMP_RAMP, 0, &ulComp, 0) == 0)
    {
        ulComp = 0;
    }
    else
    {
        ulComp = (((ulComp & 0xffff) * 100) + 128) / 256;
    }

    //
    // Initially, updates to the voltage occur immediately.
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
    for(ulIdx = 0; ulIdx < 4; ulIdx++)
    {
        CanvasFillOff(g_psVCompWidgets + ulIdx);
    }
    CanvasFillOn(g_psVCompWidgets + 1);

    //
    // Add the "VComp Control Mode" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psVCompWidgets + ulIdx));
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
        // Print out the current voltage.
        //
        if(lVoltage < 0)
        {
            usnprintf(g_pcVoltageBuffer, sizeof(g_pcVoltageBuffer),
                      "-%d.%01d V", (0 - lVoltage) / 10, (0 - lVoltage) % 10);
        }
        else
        {
            usnprintf(g_pcVoltageBuffer, sizeof(g_pcVoltageBuffer),
                      "%d.%01d V", lVoltage / 10, lVoltage % 10);
        }

        //
        // Print out the current ramp rate.
        //
        if(ulRamp == 0)
        {
            usnprintf(g_pcRampBuffer, sizeof(g_pcRampBuffer), "none");
        }
        else
        {
            usnprintf(g_pcRampBuffer, sizeof(g_pcRampBuffer), "%d.%02d V/ms",
                      ulRamp / 100, ulRamp % 100);
        }

        //
        // Print out the current compensation rate.
        //
        if(ulComp == 0)
        {
            usnprintf(g_pcCompBuffer, sizeof(g_pcCompBuffer), "none");
        }
        else
        {
            usnprintf(g_pcCompBuffer, sizeof(g_pcCompBuffer), "%d.%02d V/ms",
                      ulComp / 100, ulComp % 100);
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
            // Remove the "VComp Control Mode" panel widgets.
            //
            for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
            {
                WidgetRemove((tWidget *)(g_psVCompWidgets + ulIdx));
            }
            CanvasTextColorSet(g_psVCompWidgets + 2, ClrWhite);

            //
            // Set the output voltage to zero.
            //
            CANVCompSet(0, 0);

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
                if(ulStep ==
                   (sizeof(g_plVCompDemo) / sizeof(g_plVCompDemo[0])))
                {
                    ulStep = 0;
                }

                //
                // Set the voltage as directed by the next step.
                //
                lVoltage = g_plVCompDemo[ulStep][0];
                CANVCompSet((lVoltage * 256) / 10, 0);

                //
                // Set the time delay for this step.
                //
                ulTime = g_ulTickCount + g_plVCompDemo[ulStep][1];
            }
        }

        //
        // See if the up button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the top of the
            // screen and a delayed voltage update is not in progress.
            //
            if((ulPos != 0) && (ulDelay == 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psVCompWidgets + ulPos);

                //
                // Decrement the cursor row, skipping the voltage row when demo
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
                CanvasFillOn(g_psVCompWidgets + ulPos);
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
            // screen and a delayed voltage update is not in progress.
            //
            if((ulPos != 4) && (ulDelay == 0))
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psVCompWidgets + ulPos);

                //
                // Increment the cursor row, skipping the voltage row when demo
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
                CanvasFillOn(g_psVCompWidgets + ulPos);
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
                    CanvasTextColorSet(g_psVCompWidgets + 2, ClrWhite);

                    //
                    // Set the voltage to 0 for the current device ID.
                    //
                    CANVCompSet(0, 0);

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
                    // Enable voltage compensation mode.
                    //
                    CANVCompModeEnable();

                    //
                    // Set the voltage for the new device.
                    //
                    lVoltage = 0;
                    CANVCompSet(0, 0);

                    //
                    // Read the ramp rate.
                    //
                    if(CANReadParameter(LM_API_VCOMP_IN_RAMP, 0, &ulRamp,
                                        0) == 0)
                    {
                        ulRamp = 0;
                    }
                    else
                    {
                        ulRamp = (((ulRamp & 0xffff) * 100) + 128) / 256;
                    }

                    //
                    // Read the compensation rate.
                    //
                    if(CANReadParameter(LM_API_VCOMP_COMP_RAMP, 0, &ulComp,
                                        0) == 0)
                    {
                        ulComp = 0;
                    }
                    else
                    {
                        ulComp = (((ulComp & 0xffff) * 100) + 128) / 256;
                    }
                }
            }

            //
            // See if the voltage is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the voltage if it is not already full reverse.
                //
                if(lVoltage > -120)
                {
                    //
                    // Decrement the voltage.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        lVoltage -= 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1))
                    {
                        lVoltage -= 111;
                    }
                    else
                    {
                        lVoltage--;
                    }
                    if(lVoltage < -120)
                    {
                        lVoltage = -120;
                    }

                    //
                    // Send the updated voltage to the motor controller if a
                    // delayed update is not in progress.
                    //
                    if(ulDelay == 0)
                    {
                        CANVCompSet((lVoltage * 256) / 10, 0);
                    }
                }
            }

            //
            // See if the voltage ramp rate is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the ramp rate if it is not already zero.
                //
                if(ulRamp > 0)
                {
                    //
                    // Decrement the voltage ramp rate.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        ulRamp -= 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1))
                    {
                        ulRamp -= 111;
                    }
                    else
                    {
                        ulRamp--;
                    }
                    if(ulRamp & 0x80000000)
                    {
                        ulRamp = 0;
                    }

                    //
                    // Send the updated voltage ramp rate.
                    //
                    CANVCompInRampSet((ulRamp * 256) / 100);
                }
            }

            //
            // See if the compensation rate is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Only change the compensation rate if it is not already zero.
                //
                if(ulComp > 0)
                {
                    //
                    // Decrement the compensation rate.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        ulComp -= 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1))
                    {
                        ulComp -= 111;
                    }
                    else
                    {
                        ulComp--;
                    }
                    if(ulComp & 0x80000000)
                    {
                        ulComp = 0;
                    }

                    //
                    // Send the updated compensation rate.
                    //
                    CANVCompCompRampSet((ulComp * 256) / 100);
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
                    CanvasTextColorSet(g_psVCompWidgets + 2, ClrWhite);

                    //
                    // Set the voltage to 0 for the current device ID.
                    //
                    CANVCompSet(0, 0);

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
                    // Enable voltage compensation control mode.
                    //
                    CANVCompModeEnable();

                    //
                    // Set the voltage for the new device.
                    //
                    lVoltage = 0;
                    CANVCompSet(0, 0);

                    //
                    // Read the ramp rate.
                    //
                    if(CANReadParameter(LM_API_VCOMP_IN_RAMP, 0, &ulRamp,
                                        0) == 0)
                    {
                        ulRamp = 0;
                    }
                    else
                    {
                        ulRamp = (((ulRamp & 0xffff) * 100) + 128) / 256;
                    }

                    //
                    // Read the compensation rate.
                    //
                    if(CANReadParameter(LM_API_VCOMP_COMP_RAMP, 0, &ulComp,
                                        0) == 0)
                    {
                        ulComp = 0;
                    }
                    else
                    {
                        ulComp = (((ulComp & 0xffff) * 100) + 128) / 256;
                    }
                }
            }

            //
            // See if the voltage is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the voltage if it is not already full forward.
                //
                if(lVoltage < 120)
                {
                    //
                    // Increment the voltage.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        lVoltage += 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1))
                    {
                        lVoltage += 111;
                    }
                    else
                    {
                        lVoltage++;
                    }
                    if(lVoltage > 120)
                    {
                        lVoltage = 120;
                    }

                    //
                    // Send the updated voltage to the motor controller if a
                    // delayed update is not in progress.
                    //
                    if(ulDelay == 0)
                    {
                        CANVCompSet((lVoltage * 256) / 10, 0);
                    }
                }
            }

            //
            // See if the voltage ramp rate is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the ramp rate if it is not already the maximum.
                //
                if(ulRamp < 1200)
                {
                    //
                    // Increment the voltage ramp rate.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        ulRamp += 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1))
                    {
                        ulRamp += 111;
                    }
                    else
                    {
                        ulRamp++;
                    }
                    if(ulRamp > 1200)
                    {
                        ulRamp = 1200;
                    }

                    //
                    // Send the updated voltage ramp rate.
                    //
                    CANVCompInRampSet((ulRamp * 256) / 100);
                }
            }

            //
            // See if the compensation rate is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Only change the compensation rate if it is not already the
                // maximum.
                //
                if(ulComp < 1200)
                {
                    //
                    // Increment the compensation rate.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        ulComp += 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1))
                    {
                        ulComp += 111;
                    }
                    else
                    {
                        ulComp++;
                    }
                    if(ulComp > 1200)
                    {
                        ulComp = 1200;
                    }

                    //
                    // Send the updated compensation rate.
                    //
                    CANVCompCompRampSet((ulComp * 256) / 100);
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
                ulIdx = DisplayMenu(PANEL_VCOMP);

                //
                // See if another panel was selected.
                //
                if(ulIdx != PANEL_VCOMP)
                {
                    //
                    // Disable the status display.
                    //
                    StatusDisable();

                    //
                    // Remove the "VComp Control Mode" panel widgets.
                    //
                    for(ulPos = 0; ulPos < NUM_WIDGETS; ulPos++)
                    {
                        WidgetRemove((tWidget *)(g_psVCompWidgets + ulPos));
                    }
                    CanvasTextColorSet(g_psVCompWidgets + 2, ClrWhite);

                    //
                    // Set the output voltage to zero.
                    //
                    CANVCompSet(0, 0);

                    //
                    // Return the ID of the newly selected panel.
                    //
                    return(ulIdx);
                }

                //
                // Since the "VComp Control Mode" panel was selected from the
                // menu, move the cursor down one row.
                //
                CanvasFillOff(g_psVCompWidgets);
                ulPos++;
                CanvasFillOn(g_psVCompWidgets + 1);
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
                    // Set the output voltage to zero.
                    //
                    lVoltage = 0;
                    CANVCompSet(0, 0);

                    //
                    // Indicate that demo mode has exited by setting the text
                    // color to white.
                    //
                    CanvasTextColorSet(g_psVCompWidgets + 2, ClrWhite);
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
                    CanvasTextColorSet(g_psVCompWidgets + 2, ClrSelected);

                    //
                    // Start with the first step.
                    //
                    ulStep = 0;

                    //
                    // Set the voltage as directed by the first step.
                    //
                    lVoltage = g_plVCompDemo[0][0];
                    CANVCompSet((lVoltage * 256) / 10, 0);

                    //
                    // Set the time delay for the first step.
                    //
                    ulTime = g_ulTickCount + g_plVCompDemo[0][1];
                }
            }

            //
            // See if the cursor is on the voltage selection.
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
                    // Send the delayed voltage update.
                    //
                    CANVCompSet((lVoltage * 256) / 10, 0);

                    //
                    // Change the text color of the voltage selection to white
                    // to indicate that updates will occur immediately.
                    //
                    CanvasTextColorSet(g_psVCompWidgets + 2, ClrWhite);
                }
                else
                {
                    //
                    // Change the text color of the voltage selection to black
                    // to indicate that updates will be delayed.
                    //
                    CanvasTextColorSet(g_psVCompWidgets + 2, ClrBlack);
                }
            }
        }
    }
}
