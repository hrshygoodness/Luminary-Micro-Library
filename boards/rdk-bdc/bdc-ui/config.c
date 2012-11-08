//*****************************************************************************
//
// config.c - Displays the "Configuration" panel.
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
#include "config.h"
#include "menu.h"
#include "rit128x96x4.h"

//*****************************************************************************
//
// Buffers to contain the string representation of the current device ID,
// encoder lines, pot turns, brake/coast, soft limit enable, forward limit
// value, forward limit comparison, reverse limit value, reverse limit
// comparison, and maximum output voltage.
//
//*****************************************************************************
static char g_pcIDBuffer[4];
static char g_pcLinesBuffer[8];
static char g_pcTurnsBuffer[4];
static char g_pcBrakeBuffer[8];
static char g_pcLimitBuffer[8];
static char g_pcForwardValueBuffer[12];
static char g_pcForwardCompBuffer[4];
static char g_pcReverseValueBuffer[12];
static char g_pcReverseCompBuffer[4];
static char g_pcMaxVoutBuffer[8];

//*****************************************************************************
//
// The strings that represent the brake/coast settings.
//
//*****************************************************************************
static char *g_ppcBrakeConfig[] =
{
    "jumper",
    "brake",
    "coast"
};

//*****************************************************************************
//
// The strings that represent the soft limit switch settings.
//
//*****************************************************************************
static char *g_ppcLimitConfig[] =
{
    "disable",
    "enable"
};

//*****************************************************************************
//
// The strings that represent the soft limit switch compare settings.
//
//*****************************************************************************
static char *g_ppcLimitCompare[] =
{
    "gt",
    "lt"
};

//*****************************************************************************
//
// The widgets that make up the "Configuration" panel.
//
//*****************************************************************************
static tCanvasWidget g_psConfigWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 "Configuration", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 24, 16, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcIDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 90, 24, 36, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcLinesBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 66, 32, 24, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcTurnsBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 78, 40, 42, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcBrakeBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 72, 48, 48, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcLimitBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 66, 56, 60, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcForwardValueBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 60, 64, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcForwardCompBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 66, 72, 60, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcReverseValueBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 60, 80, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcReverseCompBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 60, 88, 42, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcMaxVoutBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 16, 18, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "ID:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 24, 84, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Encoder lines:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 32, 60, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Pot turns:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 40, 72, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Brake/coast:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 48, 66, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Soft limit:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 56, 60, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Fwd limit:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 64, 54, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Fwd comp:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 72, 60, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Rev limit:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 80, 54, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Rev comp:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 88, 54, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Max Vout:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 9, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the "Configuration" panel.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psConfigWidgets) /                  \
                                 sizeof(g_psConfigWidgets[0]))

//*****************************************************************************
//
// This structure contains the current configuration of the motor controller.
//
//*****************************************************************************
static struct
{
    unsigned long ulLines;
    unsigned long ulTurns;
    unsigned long ulBrake;
    unsigned long ulLimit;
    long lForward;
    unsigned long ulForwardComp;
    long lReverse;
    unsigned long ulReverseComp;
    unsigned long ulMaxVout;
}
g_sConfiguration;

//*****************************************************************************
//
// Reads the configuration of the current motor controller.
//
//*****************************************************************************
void
ConfigRead(void)
{
    //
    // Read the number of encoder lines.
    //
    if(CANReadParameter(LM_API_CFG_ENC_LINES, 0, &(g_sConfiguration.ulLines),
                        0) == 0)
    {
        g_sConfiguration.ulLines = 0;
    }
    else
    {
        g_sConfiguration.ulLines &= 0xffff;
    }

    //
    // Read the number of potentiometer turns.
    //
    if(CANReadParameter(LM_API_CFG_POT_TURNS, 0, &(g_sConfiguration.ulTurns),
                        0) == 0)
    {
        g_sConfiguration.ulTurns = 0;
    }
    else
    {
        g_sConfiguration.ulTurns &= 0xffff;
    }

    //
    // Read the brake/coast configuration.
    //
    if(CANReadParameter(LM_API_CFG_BRAKE_COAST, 0, &(g_sConfiguration.ulBrake),
                        0) == 0)
    {
        g_sConfiguration.ulBrake = 0;
    }
    else if(g_sConfiguration.ulBrake > 2)
    {
        g_sConfiguration.ulBrake = 0;
    }

    //
    // Read the soft limit switch configuration.
    //
    if(CANReadParameter(LM_API_CFG_LIMIT_MODE, 0, &(g_sConfiguration.ulLimit),
                        0) == 0)
    {
        g_sConfiguration.ulLimit = 0;
    }
    else
    {
        g_sConfiguration.ulLimit &= 1;
    }

    //
    // Read the forward limit switch configuration.
    //
    if(CANReadParameter(LM_API_CFG_LIMIT_FWD, 0,
                        (unsigned long *)&(g_sConfiguration.lForward),
                        &(g_sConfiguration.ulForwardComp)) == 0)
    {
        g_sConfiguration.lForward = 0;
        g_sConfiguration.ulForwardComp = 0;
    }
    else
    {
        g_sConfiguration.lForward = (((g_sConfiguration.lForward / 65536) *
                                      1000) +
                                     ((((g_sConfiguration.lForward % 65536) *
                                        1000) + 32768) / 65536));
        g_sConfiguration.ulForwardComp &= 1;
    }

    //
    // Read the reverse limit switch configuration.
    //
    if(CANReadParameter(LM_API_CFG_LIMIT_REV, 0,
                        (unsigned long *)&(g_sConfiguration.lReverse),
                        &(g_sConfiguration.ulReverseComp)) == 0)
    {
        g_sConfiguration.lReverse = 0;
        g_sConfiguration.ulReverseComp = 0;
    }
    else
    {
        g_sConfiguration.lReverse = (((g_sConfiguration.lReverse / 65536) *
                                      1000) +
                                     ((((g_sConfiguration.lReverse % 65536) *
                                        1000) + 32768) / 65536));
        g_sConfiguration.ulReverseComp &= 1;
    }

    //
    // Read the maximum output voltage.
    //
    if(CANReadParameter(LM_API_CFG_MAX_VOUT, 0, &(g_sConfiguration.ulMaxVout),
                        0) == 0)
    {
        g_sConfiguration.ulMaxVout = 120;
    }
    else
    {
        g_sConfiguration.ulMaxVout = ((((g_sConfiguration.ulMaxVout & 65535) *
                                        120) + (6 * 256)) / (12 * 256));
    }
}

//*****************************************************************************
//
// Changes the highlighting on the limit configuration items.
//
//*****************************************************************************
void
ConfigLimitHighlight(void)
{
    //
    // See if the soft limit switches are enabled.
    //
    if(g_sConfiguration.ulLimit)
    {
        //
        // Set the text color to white since the soft limit switches are
        // enabled.
        //
        CanvasTextColorSet(g_psConfigWidgets + 6, ClrWhite);
        CanvasTextColorSet(g_psConfigWidgets + 7, ClrWhite);
        CanvasTextColorSet(g_psConfigWidgets + 8, ClrWhite);
        CanvasTextColorSet(g_psConfigWidgets + 9, ClrWhite);
        CanvasTextColorSet(g_psConfigWidgets + 16, ClrWhite);
        CanvasTextColorSet(g_psConfigWidgets + 17, ClrWhite);
        CanvasTextColorSet(g_psConfigWidgets + 18, ClrWhite);
        CanvasTextColorSet(g_psConfigWidgets + 19, ClrWhite);
    }
    else
    {
        //
        // Set the text color to not present since the soft limit switches are
        // disabled.
        //
        CanvasTextColorSet(g_psConfigWidgets + 6, ClrNotPresent);
        CanvasTextColorSet(g_psConfigWidgets + 7, ClrNotPresent);
        CanvasTextColorSet(g_psConfigWidgets + 8, ClrNotPresent);
        CanvasTextColorSet(g_psConfigWidgets + 9, ClrNotPresent);
        CanvasTextColorSet(g_psConfigWidgets + 16, ClrNotPresent);
        CanvasTextColorSet(g_psConfigWidgets + 17, ClrNotPresent);
        CanvasTextColorSet(g_psConfigWidgets + 18, ClrNotPresent);
        CanvasTextColorSet(g_psConfigWidgets + 19, ClrNotPresent);
    }
}

//*****************************************************************************
//
// Displays the "Configuration" panel.  The returned valud is the ID of the
// panel to be displayed instead of the "Configuration" panel.
//
//*****************************************************************************
unsigned long
DisplayConfig(void)
{
    unsigned long ulPos, ulIdx;

    //
    // Read the current configuration.
    //
    ConfigRead();

    //
    // Set the highlighting on the soft limit switch configuration values.
    //
    ConfigLimitHighlight();

    //
    // Disable the widget fill for all the widgets except the one for the
    // device ID selection.
    //
    for(ulIdx = 0; ulIdx < 11; ulIdx++)
    {
        CanvasFillOff(g_psConfigWidgets + ulIdx);
    }
    CanvasFillOn(g_psConfigWidgets + 1);

    //
    // Add the "Configuration" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psConfigWidgets + ulIdx));
    }

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
        // Print out the current number of encoder lines.
        //
        usnprintf(g_pcLinesBuffer, sizeof(g_pcLinesBuffer), "%d",
                  g_sConfiguration.ulLines);

        //
        // Print out the current number of pot turns.
        //
        usnprintf(g_pcTurnsBuffer, sizeof(g_pcTurnsBuffer), "%d",
                  g_sConfiguration.ulTurns);

        //
        // Print out the current brake/coast configuration.
        //
        usnprintf(g_pcBrakeBuffer, sizeof(g_pcBrakeBuffer), "%s",
                  g_ppcBrakeConfig[g_sConfiguration.ulBrake]);

        //
        // Print out the current soft limit switch configuration.
        //
        usnprintf(g_pcLimitBuffer, sizeof(g_pcLimitBuffer), "%s",
                  g_ppcLimitConfig[g_sConfiguration.ulLimit]);

        //
        // Print out the current forward limit switch value.
        //
        if(g_sConfiguration.lForward < 0)
        {
            usnprintf(g_pcForwardValueBuffer, sizeof(g_pcForwardValueBuffer),
                      "-%d.%03d", (0 - g_sConfiguration.lForward) / 1000,
                      (0 - g_sConfiguration.lForward) % 1000);
        }
        else
        {
            usnprintf(g_pcForwardValueBuffer, sizeof(g_pcForwardValueBuffer),
                      "%d.%03d", g_sConfiguration.lForward / 1000,
                      g_sConfiguration.lForward % 1000);
        }

        //
        // Print out the current forward limit switch compare value.
        //
        usnprintf(g_pcForwardCompBuffer, sizeof(g_pcForwardCompBuffer), "%s",
                  g_ppcLimitCompare[g_sConfiguration.ulForwardComp]);

        //
        // Print out the current reverse limit switch value.
        //
        if(g_sConfiguration.lReverse < 0)
        {
            usnprintf(g_pcReverseValueBuffer, sizeof(g_pcReverseValueBuffer),
                      "-%d.%03d", (0 - g_sConfiguration.lReverse) / 1000,
                      (0 - g_sConfiguration.lReverse) % 1000);
        }
        else
        {
            usnprintf(g_pcReverseValueBuffer, sizeof(g_pcReverseValueBuffer),
                      "%d.%03d", g_sConfiguration.lReverse / 1000,
                      g_sConfiguration.lReverse % 1000);
        }

        //
        // Print out the current reverse limit switch compare value.
        //
        usnprintf(g_pcReverseCompBuffer, sizeof(g_pcReverseCompBuffer), "%s",
                  g_ppcLimitCompare[g_sConfiguration.ulReverseComp]);

        //
        // Print out the current maximum output voltage.
        //
        usnprintf(g_pcMaxVoutBuffer, sizeof(g_pcMaxVoutBuffer),
                      "%d.%01d V", g_sConfiguration.ulMaxVout / 10,
                  g_sConfiguration.ulMaxVout % 10);

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
            // Remove the "Configuration" panel widgets.
            //
            for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
            {
                WidgetRemove((tWidget *)(g_psConfigWidgets + ulIdx));
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
            // Only move the cursor if it is not already at the top of the
            // screen.
            //
            if(ulPos != 0)
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psConfigWidgets + ulPos);

                //
                // Decrement the cursor row.
                //
                ulPos--;
                if((g_sConfiguration.ulLimit == 0) && (ulPos == 9))
                {
                    ulPos = 5;
                }

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psConfigWidgets + ulPos);
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
            // screen.
            //
            if(ulPos != 10)
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psConfigWidgets + ulPos);

                //
                // Increment the cursor row.
                //
                ulPos++;
                if((g_sConfiguration.ulLimit == 0) && (ulPos == 6))
                {
                    ulPos = 10;
                }

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psConfigWidgets + ulPos);
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
                    // Read the configuration of the new controller.
                    //
                    ConfigRead();

                    //
                    // Set the highlighting on the soft limit switch
                    // configuration values.
                    //
                    ConfigLimitHighlight();
                }
            }

            //
            // See if the encoder lines is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the encoder lines if it is not already zero.
                //
                if(g_sConfiguration.ulLines > 0)
                {
                    //
                    // Decrement the number of encoder lines.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sConfiguration.ulLines -= 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1))
                    {
                        g_sConfiguration.ulLines -= 111;
                    }
                    else
                    {
                        g_sConfiguration.ulLines--;
                    }
                    if(g_sConfiguration.ulLines & 0x80000000)
                    {
                        g_sConfiguration.ulLines = 0;
                    }

                    //
                    // Send the updated encoder lines to the motor controller.
                    //
                    CANConfigEncoderLines(g_sConfiguration.ulLines);
                }
            }

            //
            // See if the pot turns is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the pot turns if it is not already zero.
                //
                if(g_sConfiguration.ulTurns > 0)
                {
                    //
                    // Decrement the number of pot turns.
                    //
                    if((HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1))
                    {
                        g_sConfiguration.ulTurns -= 11;
                    }
                    else
                    {
                        g_sConfiguration.ulTurns--;
                    }
                    if(g_sConfiguration.ulTurns & 0x80000000)
                    {
                        g_sConfiguration.ulTurns = 0;
                    }

                    //
                    // Send the updated pot turns.
                    //
                    CANConfigPotTurns(g_sConfiguration.ulTurns);
                }
            }

            //
            // See if the brake/coast is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Decrement the brake/coast setting.
                //
                g_sConfiguration.ulBrake--;
                if(g_sConfiguration.ulBrake & 0x80000000)
                {
                    g_sConfiguration.ulBrake = 2;
                }

                //
                // Send the updated brake/coast configuration.
                //
                CANConfigBrakeCoast(g_sConfiguration.ulBrake);
            }

            //
            // See if the soft limit is being changed.
            //
            else if(ulPos == 5)
            {
                //
                // Toggle the soft limit switch setting.
                //
                g_sConfiguration.ulLimit ^= 1;

                //
                // Set the highlighting on the soft limit switch configuration
                // values.
                //
                ConfigLimitHighlight();

                //
                // Send the updated soft limit switch configuration.
                //
                CANConfigLimitMode(g_sConfiguration.ulLimit);
            }

            //
            // See if the forward limit value is being changed.
            //
            else if(ulPos == 6)
            {
                //
                // Only decrement the value if it is not too small.
                //
                if(g_sConfiguration.lForward > -9999999)
                {
                    //
                    // Decrement the forward limit value.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sConfiguration.lForward -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sConfiguration.lForward -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sConfiguration.lForward -= 1111;
                    }
                    else
                    {
                        g_sConfiguration.lForward--;
                    }
                    if(g_sConfiguration.lForward < -9999999)
                    {
                        g_sConfiguration.lForward = -9999999;
                    }

                    //
                    // Send the updated forward limit switch configuration.
                    //
                    CANConfigLimitForward((((g_sConfiguration.lForward /
                                             1000) * 65536) +
                                           (((g_sConfiguration.lForward %
                                              1000) * 65536) / 1000)),
                                          g_sConfiguration.ulForwardComp);
                }
            }

            //
            // See if the forward limit compare is being changed.
            //
            else if(ulPos == 7)
            {
                //
                // Toggle the forward limit compare setting.
                //
                g_sConfiguration.ulForwardComp ^= 1;

                //
                // Send the updated forward limit switch configuration.
                //
                CANConfigLimitForward((((g_sConfiguration.lForward / 1000) *
                                        65536) +
                                       (((g_sConfiguration.lForward % 1000) *
                                         65536) / 1000)),
                                      g_sConfiguration.ulForwardComp);
            }

            //
            // See if the reverse limit value is being changed.
            //
            else if(ulPos == 8)
            {
                //
                // Only decrement the value if it is not too small.
                //
                if(g_sConfiguration.lReverse > -9999999)
                {
                    //
                    // Decrement the reverse limit value.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1)
                    {
                        g_sConfiguration.lReverse -= 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1)
                    {
                        g_sConfiguration.lReverse -= 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1)
                    {
                        g_sConfiguration.lReverse -= 1111;
                    }
                    else
                    {
                        g_sConfiguration.lReverse--;
                    }
                    if(g_sConfiguration.lReverse < -9999999)
                    {
                        g_sConfiguration.lReverse = -9999999;
                    }

                    //
                    // Send the updated reverse limit switch configuration.
                    //
                    CANConfigLimitReverse((((g_sConfiguration.lReverse /
                                             1000) * 65536) +
                                           (((g_sConfiguration.lReverse %
                                              1000) * 65536) / 1000)),
                                          g_sConfiguration.ulReverseComp);
                }
            }

            //
            // See if the reverse limit compare is being changed.
            //
            else if(ulPos == 9)
            {
                //
                // Toggle the reverse limit compare setting.
                //
                g_sConfiguration.ulReverseComp ^= 1;

                //
                // Send the updated reverse limit switch configuration.
                //
                CANConfigLimitReverse((((g_sConfiguration.lReverse / 1000) *
                                        65536) +
                                       (((g_sConfiguration.lReverse % 1000) *
                                         65536) / 1000)),
                                      g_sConfiguration.ulReverseComp);
            }

            //
            // See if the maximum output voltage is being changed.
            //
            else if(ulPos == 10)
            {
                //
                // Only change the maximum output voltage if it is not zero.
                //
                if(g_sConfiguration.ulMaxVout > 0)
                {
                    //
                    // Decrement the maximum output voltage.
                    //
                    if((HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL1) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL2) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_LEFT_ACCEL3) == 1))
                    {
                        g_sConfiguration.ulMaxVout -= 3;
                    }
                    else
                    {
                        g_sConfiguration.ulMaxVout--;
                    }
                    if(g_sConfiguration.ulMaxVout & 0x80000000)
                    {
                        g_sConfiguration.ulMaxVout = 0;
                    }

                    //
                    // Send the updated maximum output voltage.
                    //
                    CANConfigMaxVOut(((g_sConfiguration.ulMaxVout * 12 * 256) /
                                      120));
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
                    // Read the configuration of the new controller.
                    //
                    ConfigRead();

                    //
                    // Set the highlighting on the soft limit switch
                    // configuration values.
                    //
                    ConfigLimitHighlight();
                }
            }

            //
            // See if the encoder lines is being changed.
            //
            else if(ulPos == 2)
            {
                //
                // Only change the encoder if it is not already at the maximum.
                //
                if(g_sConfiguration.ulLines < 65535)
                {
                    //
                    // Increment the number of encoder lines.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sConfiguration.ulLines += 11;
                    }
                    else if((HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1) ||
                            (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1))
                    {
                        g_sConfiguration.ulLines += 111;
                    }
                    else
                    {
                        g_sConfiguration.ulLines++;
                    }
                    if(g_sConfiguration.ulLines > 65535)
                    {
                        g_sConfiguration.ulLines = 65535;
                    }

                    //
                    // Send the updated encoder lines to the motor controller.
                    //
                    CANConfigEncoderLines(g_sConfiguration.ulLines);
                }
            }

            //
            // See if the pot turns is being changed.
            //
            else if(ulPos == 3)
            {
                //
                // Only change the pot turns if it is not already the maximum.
                //
                if(g_sConfiguration.ulTurns < 999)
                {
                    //
                    // Increment the number of pot turns.
                    //
                    if((HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1))
                    {
                        g_sConfiguration.ulTurns += 11;
                    }
                    else
                    {
                        g_sConfiguration.ulTurns++;
                    }
                    if(g_sConfiguration.ulTurns > 999)
                    {
                        g_sConfiguration.ulTurns = 999;
                    }

                    //
                    // Send the updated pot turns.
                    //
                    CANConfigPotTurns(g_sConfiguration.ulTurns);
                }
            }

            //
            // See if the brake/coast is being changed.
            //
            else if(ulPos == 4)
            {
                //
                // Increment the brake/coast setting.
                //
                g_sConfiguration.ulBrake++;
                if(g_sConfiguration.ulBrake == 3)
                {
                    g_sConfiguration.ulBrake = 0;
                }

                //
                // Send the updated brake/coast configuration.
                //
                CANConfigBrakeCoast(g_sConfiguration.ulBrake);
            }

            //
            // See if the soft limit is being changed.
            //
            else if(ulPos == 5)
            {
                //
                // Toggle the soft limit switch setting.
                //
                g_sConfiguration.ulLimit ^= 1;

                //
                // Set the highlighting on the soft limit switch configuration
                // values.
                //
                ConfigLimitHighlight();

                //
                // Send the updated soft limit switch configuration.
                //
                CANConfigLimitMode(g_sConfiguration.ulLimit);
            }

            //
            // See if the forward limit value is being changed.
            //
            else if(ulPos == 6)
            {
                //
                // Only increment the value if it is not too large.
                //
                if(g_sConfiguration.lForward < 9999999)
                {
                    //
                    // Increment the forward limit value.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sConfiguration.lForward += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sConfiguration.lForward += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sConfiguration.lForward += 1111;
                    }
                    else
                    {
                        g_sConfiguration.lForward++;
                    }
                    if(g_sConfiguration.lForward > 9999999)
                    {
                        g_sConfiguration.lForward = 9999999;
                    }

                    //
                    // Send the updated forward limit switch configuration.
                    //
                    CANConfigLimitForward((((g_sConfiguration.lForward /
                                             1000) * 65536) +
                                           (((g_sConfiguration.lForward %
                                              1000) * 65536) / 1000)),
                                          g_sConfiguration.ulForwardComp);
                }
            }

            //
            // See if the forward limit compare is being changed.
            //
            else if(ulPos == 7)
            {
                //
                // Toggle the forward limit compare setting.
                //
                g_sConfiguration.ulForwardComp ^= 1;

                //
                // Send the updated forward limit switch configuration.
                //
                CANConfigLimitForward((((g_sConfiguration.lForward / 1000) *
                                        65536) +
                                       (((g_sConfiguration.lForward % 1000) *
                                         65536) / 1000)),
                                      g_sConfiguration.ulForwardComp);
            }

            //
            // See if the reverse limit value is being changed.
            //
            else if(ulPos == 8)
            {
                //
                // Only increment the value if it is not too large.
                //
                if(g_sConfiguration.lReverse < 9999999)
                {
                    //
                    // Increment the reverse limit value.
                    //
                    if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1)
                    {
                        g_sConfiguration.lReverse += 11;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1)
                    {
                        g_sConfiguration.lReverse += 111;
                    }
                    else if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1)
                    {
                        g_sConfiguration.lReverse += 1111;
                    }
                    else
                    {
                        g_sConfiguration.lReverse++;
                    }
                    if(g_sConfiguration.lReverse > 9999999)
                    {
                        g_sConfiguration.lReverse = 9999999;
                    }

                    //
                    // Send the updated reverse limit switch configuration.
                    //
                    CANConfigLimitReverse((((g_sConfiguration.lReverse /
                                             1000) * 65536) +
                                           (((g_sConfiguration.lReverse %
                                              1000) * 65536) / 1000)),
                                          g_sConfiguration.ulReverseComp);
                }
            }

            //
            // See if the reverse limit compare is being changed.
            //
            else if(ulPos == 9)
            {
                //
                // Toggle the reverse limit compare setting.
                //
                g_sConfiguration.ulReverseComp ^= 1;

                //
                // Send the updated reverse limit switch configuration.
                //
                CANConfigLimitReverse((((g_sConfiguration.lReverse / 1000) *
                                        65536) +
                                       (((g_sConfiguration.lReverse % 1000) *
                                         65536) / 1000)),
                                      g_sConfiguration.ulReverseComp);
            }

            //
            // See if the maximum output voltage is being changed.
            //
            else if(ulPos == 10)
            {
                //
                // Only change the maximum output voltage if it is not 12.0.
                //
                if(g_sConfiguration.ulMaxVout < 120)
                {
                    //
                    // Increment the maximum output voltage.
                    //
                    if((HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL1) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL2) == 1) ||
                       (HWREGBITW(&g_ulFlags, FLAG_RIGHT_ACCEL3) == 1))
                    {
                        g_sConfiguration.ulMaxVout += 3;
                    }
                    else
                    {
                        g_sConfiguration.ulMaxVout++;
                    }
                    if(g_sConfiguration.ulMaxVout > 120)
                    {
                        g_sConfiguration.ulMaxVout = 120;
                    }

                    //
                    // Send the updated maximum output voltage.
                    //
                    CANConfigMaxVOut(((g_sConfiguration.ulMaxVout * 12 * 256) /
                                      120));
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
                ulIdx = DisplayMenu(PANEL_CONFIGURATION);

                //
                // See if another panel was selected.
                //
                if(ulIdx != PANEL_CONFIGURATION)
                {
                    //
                    // Remove the "Configuration" panel widgets.
                    //
                    for(ulPos = 0; ulPos < NUM_WIDGETS; ulPos++)
                    {
                        WidgetRemove((tWidget *)(g_psConfigWidgets + ulPos));
                    }

                    //
                    // Return the ID of the newly selected panel.
                    //
                    return(ulIdx);
                }

                //
                // Since the "Configuration" panel was selected from the menu,
                // move the cursor down one row.
                //
                CanvasFillOff(g_psConfigWidgets);
                ulPos++;
                CanvasFillOn(g_psConfigWidgets + 1);
            }
        }
    }
}
