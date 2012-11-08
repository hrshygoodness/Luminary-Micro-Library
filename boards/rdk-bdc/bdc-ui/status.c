//*****************************************************************************
//
// status.c - Displays status of the currently selected motor controller.
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
#include "utils/ustdlib.h"
#include "shared/can_proto.h"
#include "bdc-ui.h"
#include "can_comm.h"
#include "rit128x96x4.h"

//*****************************************************************************
//
// Buffers to contain the string representation of the bus voltage, output
// voltage, motor current, ambient temperature, motor speed, and motor
// position.
//
//*****************************************************************************
static char g_pcVBus[8];
static char g_pcVOut[8];
static char g_pcCurrent[8];
static char g_pcTemp[8];
static char g_pcSpeed[8];
static char g_pcPosition[8];

//*****************************************************************************
//
// The widgets that make up the status display.
//
//*****************************************************************************
static tCanvasWidget g_psStatusWidgets[] =
{
    //
    // The separator line.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 61, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0),

    //
    // The first row of status, containing the bus and output voltages.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 64, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "Vbus:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 30, 64, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, g_pcVBus,
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 68, 64, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "Vout:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 98, 64, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, g_pcVOut,
                 0, 0),

    //
    // The second row of status, containing the motor current and ambient
    // temperature.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 72, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, " Cur:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 30, 72, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcCurrent, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 68, 72, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "Temp:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 98, 72, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, g_pcTemp,
                 0, 0),

    //
    // The third row of status, containing the motor position and speed.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 80, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, " Spd:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 30, 80, 36, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcSpeed, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 68, 80, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, " Pos:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 98, 80, 30, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcPosition, 0, 0),

    //
    // The fourth row of status, containing the fault and limit switch
    // indicators.
    //
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 88, 36, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "Fault:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 42, 88, 6, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrNotPresent, g_pFontFixed6x8, "C",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 48, 88, 6, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrNotPresent, g_pFontFixed6x8, "T",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 54, 88, 6, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrNotPresent, g_pFontFixed6x8, "V",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 72, 88, 36, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "Limit:",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 114, 88, 6, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrNotPresent, g_pFontFixed6x8, "F",
                 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 120, 88, 6, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrNotPresent, g_pFontFixed6x8, "R",
                 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the status display.
//
//*****************************************************************************
#define NUM_WIDGETS             (sizeof(g_psStatusWidgets) /                  \
                                 sizeof(g_psStatusWidgets[0]))

//*****************************************************************************
//
// A flag that indicates if the VOut display should be based on an ideal 12V
// input (appropriate for voltage control mode) or based on the actual input
// voltage (appropriate for all other control modes).
//
//*****************************************************************************
static unsigned long g_ulVOutIdeal;

//*****************************************************************************
//
// This function enables the status display.
//
//*****************************************************************************
void
StatusEnable(unsigned long ulVOutIdeal)
{
    unsigned long ulIdx;

    //
    // Add the status display widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psStatusWidgets + ulIdx));
    }

    //
    // Save the value of the VOut flag for later use when displaying the VOut
    // status.
    //
    g_ulVOutIdeal = ulVOutIdeal;

    //
    // Enable the gathering of status information over CAN.
    //
    CANStatusEnable();
}

//*****************************************************************************
//
// This function updates the status display.
//
//*****************************************************************************
void
StatusUpdate(void)
{
    unsigned long ulValue;
    long lValue;
    char *pcSign;

    //
    // See if the Vbus value is valid.
    //
    if(HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VBUS) == 1)
    {
        //
        // Grab the Vbus value.
        //
        ulValue = g_ulStatusVbus;

        //
        // Print out the Vbus value, using two fractional digits if the voltage
        // is less than 10 and one otherwise.
        //
        if(ulValue < (10 * 256))
        {
            usnprintf(g_pcVBus, sizeof(g_pcVBus), "%d.%02d", ulValue / 256,
                      ((ulValue % 256) * 100) / 256);
        }
        else
        {
            usnprintf(g_pcVBus, sizeof(g_pcVBus), "%d.%01d", ulValue / 256,
                      ((ulValue % 256) * 10) / 256);
        }
    }
    else
    {
        //
        // Display dashes to indicate that the Vbus value is invalid.
        //
        usnprintf(g_pcVBus, sizeof(g_pcVBus), "---");
    }

    //
    // See if the Vout value is valid.  If the VOut display is based on the
    // actual input voltage, then the Vbus status must be valid as well.
    //
    if(((HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VOUT) == 1) &&
        (g_ulVOutIdeal == 1)) ||
       ((HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VOUT) == 1) &&
        (HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VBUS) == 1) &&
        (g_ulVOutIdeal == 0)))
    {
        //
        // Grab the Vout value.
        //
        lValue = g_lStatusVout;

        //
        // Determine the sign of the Vout value.
        //
        if(lValue < 0)
        {
            pcSign = "-";
            lValue = 0 - lValue;
        }
        else
        {
            pcSign = "";
        }

        //
        // Convert the Vout value from a percentage of 12V to a voltage.
        //
        if(g_ulVOutIdeal)
        {
            lValue = (lValue + 1) * 12;
        }
        else
        {
            lValue = ((lValue + 1) * g_ulStatusVbus) / 256;
        }

        //
        // Print out the Vout value, using two fractional digits if the voltage
        // is less than 10 and one otherwise.
        //
        if(lValue < (10 * 32767))
        {
            usnprintf(g_pcVOut, sizeof(g_pcVOut), "%s%d.%02d", pcSign,
                      lValue / 32767, ((lValue % 32767) * 100) / 32767);
        }
        else
        {
            usnprintf(g_pcVOut, sizeof(g_pcVOut), "%s%d.%01d", pcSign,
                      lValue / 32767, ((lValue % 32767) * 10) / 32767);
        }
    }
    else
    {
        //
        // Display dashes to indicate that the Vout value is invalid.
        //
        usnprintf(g_pcVOut, sizeof(g_pcVOut), "---");
    }

    //
    // See if the motor current value is valid.
    //
    if(HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_CURRENT) == 1)
    {
        //
        // Grab the motor current value.
        //
        lValue = g_lStatusCurrent;

        //
        // Determine the sign of the motor current value.
        //
        if(lValue < 0)
        {
            pcSign = "-";
            lValue = 0 - lValue;
        }
        else
        {
            pcSign = "";
        }

        //
        // Print out the motor current value, using two fractional digits if
        // the current is less than 10 and one otherwise.
        //
        if(lValue < (10 * 256))
        {
            usnprintf(g_pcCurrent, sizeof(g_pcCurrent), "%s%d.%02d", pcSign,
                      lValue / 256, ((lValue % 256) * 100) / 256);
        }
        else
        {
            usnprintf(g_pcCurrent, sizeof(g_pcCurrent), "%s%d.%01d", pcSign,
                      lValue / 256, ((lValue % 256) * 10) / 256);
        }
    }
    else
    {
        //
        // Display dashes to indicate that the motor current value is invalid.
        //
        usnprintf(g_pcCurrent, sizeof(g_pcCurrent), "---");
    }

    //
    // See if the ambient temperature value is valid.
    //
    if(HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_TEMP) == 1)
    {
        //
        // Grab the ambient temperature value.
        //
        ulValue = g_ulStatusTemperature;

        //
        // Print out the ambient temperature value, using two fractional digits
        // if the temperature is less than 10 and one otherwise.
        //
        if(ulValue < (10 * 256))
        {
            usnprintf(g_pcTemp, sizeof(g_pcTemp), "%d.%02d", ulValue / 256,
                      ((ulValue % 256) * 100) / 256);
        }
        else
        {
            usnprintf(g_pcTemp, sizeof(g_pcTemp), "%d.%01d", ulValue / 256,
                      ((ulValue % 256) * 10) / 256);
        }
    }
    else
    {
        //
        // Display dashes to indicate that the ambient temperature is invalid.
        //
        usnprintf(g_pcTemp, sizeof(g_pcTemp), "---");
    }

    //
    // See if the motor speed value is valid.
    //
    if(HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_SPEED) == 1)
    {
        //
        // Grab the motor speed value.
        //
        lValue = g_lStatusSpeed;

        //
        // Determine the sign of the motor speed value.
        //
        if(lValue < 0)
        {
            pcSign = "-";
            lValue = 0 - lValue;
        }
        else
        {
            pcSign = "";
        }

        //
        // Print out the motor speed value, using two fractional digits if the
        // speed is less than 10, one if it is less than 100, and zero
        // otherwise.
        //
        if(lValue < (10 * 65536))
        {
            usnprintf(g_pcSpeed, sizeof(g_pcSpeed), "%s%d.%02d", pcSign,
                      lValue / 65536, ((lValue % 65536) * 100) / 65536);
        }
        else if(lValue < (100 * 65536))
        {
            usnprintf(g_pcSpeed, sizeof(g_pcSpeed), "%s%d.%01d", pcSign,
                      lValue / 65536, ((lValue % 65536) * 10) / 65536);
        }
        else
        {
            usnprintf(g_pcSpeed, sizeof(g_pcSpeed), "%s%d", pcSign,
                      lValue / 65536);
        }
    }
    else
    {
        usnprintf(g_pcSpeed, sizeof(g_pcSpeed), "---");
    }

    //
    // See if the motor position value is valid.
    //
    if(HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_POS) == 1)
    {
        //
        // Grab the motor position value.
        //
        lValue = g_lStatusPosition;

        //
        // Determine the sign of the motor position value.
        //
        if(lValue < 0)
        {
            pcSign = "-";
            lValue = 0 - lValue;
        }
        else
        {
            pcSign = "";
        }

        //
        // Print out the motor position value, using two fractional digits if
        // the position is less than 10, one if it is less than 100, and zero
        // otherwise.
        //
        if(lValue < (10 * 65536))
        {
            usnprintf(g_pcPosition, sizeof(g_pcPosition), "%s%d.%02d", pcSign,
                      lValue / 65536, ((lValue % 65536) * 100) / 65536);
        }
        else if(lValue < (100 * 65536))
        {
            usnprintf(g_pcPosition, sizeof(g_pcPosition), "%s%d.%01d", pcSign,
                      lValue / 65536, ((lValue % 65536) * 10) / 65536);
        }
        else
        {
            usnprintf(g_pcPosition, sizeof(g_pcPosition), "%s%d", pcSign,
                      lValue / 65536);
        }
    }
    else
    {
        usnprintf(g_pcPosition, sizeof(g_pcPosition), "---");
    }

    //
    // Grab the fault status if it is valid and assert no faults otherwise.
    //
    if(HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_FAULT) == 1)
    {
        ulValue = g_ulStatusFault;
    }
    else
    {
        ulValue = 0;
    }

    //
    // Set the color of the fault status indicators based on the fault status.
    //
    CanvasTextColorSet(g_psStatusWidgets + 14,
                       ((ulValue & LM_STATUS_FAULT_ILIMIT) ? ClrWhite :
                        ClrNotPresent));
    CanvasTextColorSet(g_psStatusWidgets + 15,
                       ((ulValue & LM_STATUS_FAULT_TLIMIT) ? ClrWhite :
                        ClrNotPresent));
    CanvasTextColorSet(g_psStatusWidgets + 16,
                       ((ulValue & LM_STATUS_FAULT_VLIMIT) ? ClrWhite :
                        ClrNotPresent));

    //
    // Grab the limit switch values if they are valid and indicate they are
    // both closed otherwise.
    //
    if(HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_LIMIT) == 1)
    {
        ulValue = g_ulStatusLimit;
    }
    else
    {
        ulValue = LM_STATUS_LIMIT_FWD | LM_STATUS_LIMIT_REV;
    }

    //
    // Set the color of the limit switch indicators based on the limit switch
    // values.
    //
    CanvasTextColorSet(g_psStatusWidgets + 18,
                       ((ulValue & LM_STATUS_LIMIT_FWD) ? ClrNotPresent :
                        ClrWhite));
    CanvasTextColorSet(g_psStatusWidgets + 19,
                       ((ulValue & LM_STATUS_LIMIT_REV) ? ClrNotPresent :
                        ClrWhite));
}

//*****************************************************************************
//
// This function disables the status display.
//
//*****************************************************************************
void
StatusDisable(void)
{
    unsigned long ulIdx;

    //
    // Disable the gathering of status information over CAN.
    //
    CANStatusDisable();

    //
    // Remove the status display widgets from the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_WIDGETS; ulIdx++)
    {
        WidgetRemove((tWidget *)(g_psStatusWidgets + ulIdx));
    }
}
