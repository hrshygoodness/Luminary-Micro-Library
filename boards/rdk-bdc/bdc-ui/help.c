//*****************************************************************************
//
// help.c - Displays the "Help" panel.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
#include "bdc-ui.h"
#include "help.h"
#include "menu.h"
#include "rit128x96x4.h"

//*****************************************************************************
//
// The help text.  Strings that start with \001 are headings and are displayed
// centered with a shaded background.
//
//*****************************************************************************
static const char *g_ppcHelpText[] =
{
    //
    // General help text.
    //
    "This application",
    "provides control of",
    "the MDL-BDC board.",
    "Use the UP and DOWN",
    "buttons to scroll",
    "through the help",
    "text, and the LEFT",
    "and RIGHT buttons to",
    "skip between",
    "sections.",
    "",
    "The operation of each",
    "screen is described",
    "in an individual",
    "section below.",
    "",

    //
    // Help on the voltage control mode screen.
    //
    "\001Voltage Control Mode",
    "",
    "This screen provides",
    "control of the motor",
    "in voltage mode.  Use",
    "UP and DOWN to select",
    "the parameter to be",
    "modified, and LEFT",
    "and RIGHT to modify",
    "the parameter.",
    "",
    "The ID parameter",
    "selects the MDL-BDC",
    "to be controlled.",
    "Changing the ID will",
    "stop the motor.",
    "Pressing SELECT will",
    "toggle a demo mode",
    "which will cycle the",
    "motor through a",
    "sequence of voltages.",
    "",
    "The Voltage parameter",
    "will send a voltage",
    "update command to the",
    "motor controller",
    "immediately.  By",
    "pressing SELECT, the",
    "voltage update",
    "command is delayed",
    "until SELECT is",
    "pressed again.",
    "",
    "The Ramp parameter",
    "changes the rate at",
    "which the output",
    "voltage is changed.",
    "This can provide a",
    "smooth start for the",
    "motor, or provide a",
    "means to control the",
    "startup current.",
    "",
    "The status of the",
    "motor controller is",
    "displayed across the",
    "bottom of the screen.",
    "",

    //
    // Help on the voltage compensation control mode screen.
    //
    "\001VComp Control Mode",
    "",
    "This screen provides",
    "control of the motor",
    "in voltage",
    "compensation mode.",
    "Use UP and DOWN to",
    "select the parameter",
    "to be modified, and",
    "LEFT and RIGHT to",
    "modify the parameter.",
    "",
    "The ID parameter",
    "selects the MDL-BDC",
    "to be controlled.",
    "Changing the ID will",
    "stop the motor.",
    "Pressing SELECT will",
    "toggle a demo mode",
    "which will cycle the",
    "motor through a",
    "sequence of voltages.",
    "",
    "The Voltage parameter",
    "will send a voltage",
    "update command to the",
    "motor controller",
    "immediately.  By",
    "pressing SELECT, the",
    "voltage update",
    "command is delayed",
    "until SELECT is",
    "pressed again.",
    "",
    "The Ramp parameter",
    "changes the rate at",
    "which the output",
    "voltage is changed in",
    "response to a change",
    "in the voltage",
    "setting.  This can",
    "provide a smooth",
    "start for the motor,",
    "or provide a means to",
    "control the startup",
    "current.",
    "",
    "The Comp parameter",
    "changes the rate at",
    "which the output",
    "voltage is adjusted",
    "in response to a",
    "change in the MDL-BDC",
    "input voltage.",
    "",

    "The status of the",
    "motor controller is",
    "displayed across the",
    "bottom of the screen.",
    "",

    //
    // Help on the current control mode screen.
    //
    "\001Current Control Mode",
    "",
    "This screen provides",
    "control of the motor",
    "in current mode.  Use",
    "UP and DOWN to select",
    "the parameter to be",
    "modified, and LEFT",
    "and RIGHT to modify",
    "the parameter.",
    "",
    "The ID parameter",
    "selects the MDL-BDC",
    "to be controlled.",
    "Changing the ID will",
    "stop the motor.",
    "Pressing SELECT will",
    "toggle a demo mode",
    "which will cycle the",
    "motor through a",
    "sequence of currents.",
    "",
    "The Current parameter",
    "will send a current",
    "update command to the",
    "motor controller",
    "immediately.  By",
    "pressing SELECT, the",
    "current update",
    "command is delayed",
    "until SELECT is",
    "pressed again.",
    "",
    "The P parameter",
    "specifies the gain",
    "applied to the",
    "instantaneous current",
    "error.",
    "",
    "The I parameter",
    "specifies the gain",
    "applied to the",
    "integral of the",
    "current error.",
    "",
    "The D parameter",
    "specifies the gain",
    "applied to the",
    "derivitive of the",
    "current error.",
    "",

    //
    // Help on the speed control mode screen.
    //
    "\001Speed Control Mode",
    "",
    "This screen provides",
    "control of the motor",
    "in speed mode.  Use",
    "UP and DOWN to select",
    "the parameter to be",
    "modified, and LEFT",
    "and RIGHT to modify",
    "the parameter.",
    "",
    "The ID parameter",
    "selects the MDL-BDC",
    "to be controlled.",
    "Changing the ID will",
    "stop the motor.",
    "Pressing SELECT will",
    "toggle a demo mode",
    "which will cycle the",
    "motor through a",
    "sequence of speeds.",
    "",
    "The Speed parameter",
    "will send a speed",
    "update command to the",
    "motor controller",
    "immediately.  By",
    "pressing SELECT, the",
    "speed update command",
    "is delayed until",
    "SELECT is pressed",
    "again.",
    "",
    "The P parameter",
    "specifies the gain",
    "applied to the",
    "instantaneous speed",
    "error.",
    "",
    "The I parameter",
    "specifies the gain",
    "applied to the",
    "integral of the speed",
    "error.",
    "",
    "The D parameter",
    "specifies the gain",
    "applied to the",
    "derivitive of the",
    "speed error.",
    "",

    //
    // Help on the position control mode screen.
    //
    "\001Position Control Mode",
    "",
    "This screen provides",
    "control of the motor",
    "in position mode.",
    "Use UP and DOWN to",
    "select the parameter",
    "to be modified, and",
    "LEFT and RIGHT to",
    "modify the parameter.",
    "",
    "The ID parameter",
    "selects the MDL-BDC",
    "to be controlled.",
    "Changing the ID will",
    "stop the motor.",
    "Pressing SELECT will",
    "toggle a demo mode",
    "which will cycle the",
    "motor through a",
    "sequence of",
    "positions.",
    "",
    "The Position",
    "parameter will send a",
    "position update",
    "command to the motor",
    "controller",
    "immediately.  By",
    "pressing SELECT, the",
    "position update",
    "command is delayed",
    "until SELECT is",
    "pressed again.",
    "",
    "The P parameter",
    "specifies the gain",
    "applied to the",
    "instantaneous",
    "position error.",
    "",
    "The I parameter",
    "specifies the gain",
    "applied to the",
    "integral of the",
    "position error.",
    "",
    "The D parameter",
    "specifies the gain",
    "applied to the",
    "derivitive of the",
    "position error.",
    "",
    "The Reference",
    "parameter specifies",
    "the method used to",
    "determine the",
    "position of the",
    "motor.",
    "",

    //
    // Help on the configuration screen.
    //
    "\001Configuration",
    "",
    "This screen is used",
    "to configure the",
    "motor controller.",
    "Use UP and DOWN to",
    "select the parameter",
    "to be modified, and",
    "LEFT and RIGHT to",
    "modify the parameter.",
    "",
    "The ID parameter",
    "selects the MDL-BDC",
    "to be configured.",
    "",
    "The Encoder Lines",
    "parameter specifies",
    "the number of lines",
    "in the encoder (if",
    "present).",
    "",
    "The Potentiometer",
    "Turns parameter",
    "specifies the number",
    "of turns in the",
    "potentiometer (if",
    "present).",
    "",
    "The Brake/Coast",
    "parameter specifies",
    "the action to take",
    "when not driving the",
    "motor.  \"Jumper\" will",
    "act based on the",
    "jumper setting,",
    "\"Brake\" will",
    "electrically brake",
    "the motor, and",
    "\"Coast\" will allow",
    "the motor to coast to",
    "a stop.",
    "",
    "The Soft Limit Switch",
    "parameter allows the",
    "soft limit switches",
    "to be enabled or",
    "disabled.",
    "",
    "The Forward Limit",
    "Position parameter",
    "specifies the",
    "position of the",
    "forward soft limit",
    "switch.",
    "",
    "The Forward",
    "Comparison parameter",
    "specifies how the",
    "motor position is",
    "compared to the",
    "forward limit switch",
    "position.",
    "",
    "The Reverse Limit",
    "Position parameter",
    "specifies the",
    "position of the",
    "reverse soft limit",
    "switch.",
    "",
    "The Reverse",
    "Comparison parameter",
    "specifies how the",
    "motor position is",
    "compared to the",
    "reverse limit switch",
    "position.",
    "",
    "The Maximum Output",
    "Voltage parameter",
    "specifies the maximum",
    "voltage that can be",
    "applied to the",
    "attached motor.",
    "",

    //
    // Help on the device list screen.
    //
    "\001Device List",
    "",
    "This screen lists the",
    "motor controller(s)",
    "that are present on",
    "the network.  The IDs",
    "that correspond to",
    "devices that are not",
    "present are dim and",
    "those that are",
    "present are bright.",
    "",
    "By highlighting a",
    "device number and",
    "pressing SELECT, that",
    "device ID will be",
    "assigned.  The motor",
    "controller will wait",
    "for five seconds for",
    "its button to be",
    "pressed, indicating",
    "that it should accept",
    "the ID assignment.",
    "",

    //
    // Help on the firmware update screen.
    //
    "\001Firmware Update",
    "",
    "This screen provides",
    "a means of updating",
    "the firmware on the",
    "motor controller.",
    "",
    "The ID parameter",
    "selects the MDL-BDC",
    "to be updated.",
    "",
    "The version of the",
    "firmware on the",
    "selected MDL-BDC is",
    "displayed beneath",
    "the ID parameter.",
    "",
    "By highlighting and",
    "selecting the \"Start\"",
    "button, the firmware",
    "on the selected motor",
    "controller will be",
    "updated.",
    "",
    "The UART can be used",
    "to update the",
    "firmware that is",
    "downloaded into the",
    "MDL-BDC.  When a UART",
    "update starts, this",
    "screen will become",
    "active immediately",
    "and display the",
    "progress of the",
    "update.  Once the",
    "UART update is",
    "complete, the MDL-BDC",
    "is updated with the",
    "new firmware.",
    "",

    //
    // Help on the LED codes.
    //
    "\001MDL-BDC LED Codes",
    "",
    "* Solid Yellow",
    "",
    "  The motor is in",
    "  neutral.",
    "",
    "* Flashing Green",
    "",
    "  The motor is in",
    "  proportional",
    "  forward.",
    "",
    "* Solid Green",
    "",
    "  The motor is in",
    "  full forward.",
    "",
    "* Flashing Red",
    "",
    "  The motor is in",
    "  proportional",
    "  reverse.",
    "",
    "* Solid Red",
    "",
    "  The motor is in",
    "  full reverse.",
    "",
    "* Flash Yellow/Red",
    "",
    "  The controller has",
    "  detected a current",
    "  fault condition.",
    "",
    "* Slow Flash Red",
    "",
    "  The controller has",
    "  detected a fault",
    "  condition other",
    "  than a current",
    "  fault.",
    "",
    "* Slow Flash Yellow",
    "",
    "  The controller does",
    "  not have a control",
    "  link.",
    "",
    "* Fast Flash Yellow",
    "",
    "  The controller does",
    "  not have an ID",
    "  assigned.",
    "",
    "* Slow Flash Green",
    "",
    "  The controller is",
    "  in ID assignment",
    "  mode."
};

//*****************************************************************************
//
// The number of lines in the help text.
//
//*****************************************************************************
#define NUM_LINES               (sizeof(g_ppcHelpText) /                      \
                                 sizeof(g_ppcHelpText[0]))

//*****************************************************************************
//
// The index into the help text of the first line of text to display on the
// screen.
//
//*****************************************************************************
static unsigned long g_ulDelta;

//*****************************************************************************
//
// The function that is called when the help text canvas is painted to the
// screen.
//
//*****************************************************************************
static void
OnPaint(tWidget *pWidget, tContext *pContext)
{
    unsigned long ulIdx;
    tRectangle sRect;

    //
    // Prepare the drawing context.
    //
    GrContextForegroundSet(pContext, ClrWhite);
    GrContextFontSet(pContext, g_pFontFixed6x8);

    //
    // Loop through the ten lines of the display.
    //
    for(ulIdx = 0; ulIdx < 10; ulIdx++)
    {
        //
        // See if this line of help text is a heading.
        //
        if(g_ppcHelpText[g_ulDelta + ulIdx][0] == '\001')
        {
            //
            // Draw a shaded rectangle across this entire line of the display.
            //
            sRect.sXMin = 0;
            sRect.sYMin = (ulIdx * 8) + 16;
            sRect.sXMax = 127;
            sRect.sYMax = sRect.sYMin + 7;
            GrContextForegroundSet(pContext, ClrSelected);
            GrRectFill(pContext, &sRect);

            //
            // Draw this line of text centered on the display.
            //
            GrContextForegroundSet(pContext, ClrWhite);
            GrStringDrawCentered(pContext,
                                 g_ppcHelpText[g_ulDelta + ulIdx] + 1, -1, 63,
                                 (ulIdx * 8) + 19, 0);
        }
        else
        {
            //
            // Draw this line of text on the display.
            //
            GrStringDraw(pContext, g_ppcHelpText[g_ulDelta + ulIdx], -1, 0,
                         (ulIdx * 8) + 16, 0);
        }
    }
}

//*****************************************************************************
//
// The widgets that make up the "Help" panel.
//
//*****************************************************************************
static tCanvasWidget g_psHelpWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT | CANVAS_STYLE_FILL, ClrSelected, 0,
                 ClrWhite, g_pFontFixed6x8, "Help", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 12, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 16, 128, 80,
                 CANVAS_STYLE_FILL | CANVAS_STYLE_APP_DRAWN, ClrBlack, 0, 0, 0,
                 0, 0, OnPaint)
};

//*****************************************************************************
//
// The number of widgets in the "Help" panel.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psHelpWidgets) /                    \
                                 sizeof(g_psHelpWidgets[0]))

//*****************************************************************************
//
// Displays the "Help" panel.  The returned value is the ID of the panel to be
// displayed instead of the "Help" panel.
//
//*****************************************************************************
unsigned long
DisplayHelp(void)
{
    unsigned long ulIdx, ulPanel;

    //
    // Add the "Help" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psHelpWidgets + ulIdx));
    }

    //
    // Set the help text offset to zero.
    //
    g_ulDelta = 0;

    //
    // Loop forever.  This loop will be explicitly exited when the proper
    // condition is detected.
    //
    while(1)
    {
        //
        // Update the display.
        //
        DisplayFlush();

        //
        // Wait until the up, down, left, right, or select button is pressed,
        // or a serial download begins.
        //
        while((HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 0))
        {
        }

        //
        // See if a serial download has begun.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 1)
        {
            //
            // Remove the "Help" panel widgets.
            //
            for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
            {
                WidgetRemove((tWidget *)(g_psHelpWidgets + ulIdx));
            }

            //
            // Return the ID of the update panel.
            //
            return(PANEL_UPDATE);
        }

        //
        // See if the up button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 1)
        {
            //
            // Decrement the help text offset if the first line of help text is
            // not presently visible.
            //
            if(g_ulDelta > 0)
            {
                g_ulDelta--;
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
            // Increment the help text offset if the last line of help text is
            // not presently visible.
            //
            if(g_ulDelta < (NUM_LINES - 10))
            {
                g_ulDelta++;
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
            // Skip to the previous section only if not already at the
            // beginning of the help text.
            //
            if(g_ulDelta != 0)
            {
                //
                // Decrement by one line.
                //
                g_ulDelta--;

                //
                // Look for the start of the previous section, stopping at the
                // beginning of the help text if one is not found.
                //
                while(g_ulDelta != 0)
                {
                    //
                    // Stop looking if this line starts a section.
                    //
                    if(g_ppcHelpText[g_ulDelta][0] == '\001')
                    {
                        break;
                    }

                    //
                    // Decrement by one line.
                    //
                    g_ulDelta--;
                }
            }

            //
            // Clear the press flag for the left button.
            //
            HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
        }

        //
        // See if the right button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) == 1)
        {
            //
            // Skip to the next section only if not already at the end of the
            // help text.
            //
            if(g_ulDelta != (NUM_LINES - 10))
            {
                //
                // Increment by one line.
                //
                g_ulDelta++;

                //
                // Look for the start of the next section, stopping at the end
                // of the help text if one is not found.
                //
                while(g_ulDelta != (NUM_LINES - 10))
                {
                    //
                    // Stop looking if this line starts a section.
                    //
                    if(g_ppcHelpText[g_ulDelta][0] == '\001')
                    {
                        break;
                    }

                    //
                    // Increment by one line.
                    //
                    g_ulDelta++;
                }
            }

            //
            // Clear the press flag for the right button.
            //
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
        }

        //
        // See if the select button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) == 1)
        {
            //
            // Clear the press flag for the left, right, and select buttons.
            //
            HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
            HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) = 0;

            //
            // Display the menu.
            //
            ulPanel = DisplayMenu(PANEL_HELP);

            //
            // See if another panel was selected.
            //
            if(ulPanel != PANEL_HELP)
            {
                //
                // Remove the "About" panel widgets.
                //
                for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
                {
                    WidgetRemove((tWidget *)(g_psHelpWidgets + ulIdx));
                }

                //
                // Return the ID of the newly selected panel.
                //
                return(ulPanel);
            }
        }
    }
}
