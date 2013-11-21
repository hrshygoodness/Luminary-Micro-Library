//*****************************************************************************
//
// stripchartmanager.c - Manages a strip chart widget for the data logger.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/slidemenuwidget.h"
#include "utils/ustdlib.h"
#include "stripchartwidget.h"
#include "stripchartmanager.h"
#include "clocksetwidget.h"
#include "qs-logger.h"
#include "menus.h"

//*****************************************************************************
//
// This module manages a strip chart widget for the data logger application.
// It provides functions to make it easy to configure a strip chart for the
// user-selected data series, and to add new data to the strip chart.  The
// functions in this module maintain buffers that hold the data for each data
// series that is selected for display on the strip chart.
//
//*****************************************************************************

//*****************************************************************************
//
// Define a scaling range for each data series.  Since multiple kinds of data
// will be shown on the strip chart, no one particular set of units can be
// selected.  Instead the strip chart Y axis will just be maintained in units
// of pixels, and the table below maps the Y axis range to min and max values
// for each data series.
//
//*****************************************************************************
typedef struct
{
    short sMin;
    short sMax;
} tDisplayScaling;

tDisplayScaling sScaling[] =
{
    { 0, 20000 },       // analog channel inputs, 0-20V (20000 mV)
    { 0, 20000 },
    { 0, 20000 },
    { 0, 20000 },
    { -200, 200 },      // accelerometer axes, -2 - 2g (units of 1/100g)
    { -200, 200 },
    { -200, 200 },
    { 0, 500 },         // temperature, 0 - 50C (units of 1/10C)
    { 0, 500 },
    { 0, 400 },         // current, 0 - 40mA (units of 100uA)
};

//*****************************************************************************
//
// Defines the maximum number of items that are stored in a data series.  This
// matches the width of the strip chart in pixels.
//
//*****************************************************************************
#define SERIES_LENGTH 96

//*****************************************************************************
//
// Create an array of strip chart data series, one for each channel of data
// that the data logger can acquire.  Fields that are unchanging, such as
// the name of each series, are pre-populated here, while other fields that
// may change are updated by functions.  These are the data series that get
// added to the strip chart for each item that is selected for logging.
//
//*****************************************************************************
static tStripChartSeries g_sSeries[] =
{
    { 0, "CH0", 0x000040, 1, 1, 0, 0 },
    { 0, "CH1", ClrLime, 1, 1, 0, 0 },
    { 0, "CH2", ClrAqua, 1, 1, 0, 0 },
    { 0, "CH3", ClrRed, 1, 1, 0, 0 },
    { 0, "ACCELX", ClrBlue, 1, 1, 0, 0 },
    { 0, "ACCELY", 0x00A000, 1, 1, 0, 0 },
    { 0, "ACCELZ", ClrFuchsia, 1, 1, 0, 0 },
    { 0, "CURRENT", ClrYellow, 1, 1, 0, 0 },
    { 0, "EXT TEMP", 0xC00040, 1, 1, 0, 0 },
    { 0, "INT TEMP", 0x60E080, 1, 1, 0, 0 },
};
#define MAX_NUM_SERIES (sizeof(g_sSeries) / sizeof(tStripChartSeries))

//*****************************************************************************
//
// Defines the X-axis for the strip chart.
//
//*****************************************************************************
static tStripChartAxis sAxisX =
{
    "X-AXIS",   // title of axis
    0,          // label for minimum of axis
    0,          // label for maximum of axis
    0,          // minimum value for the axis
    95,         // maximum value for the axis
    10          // grid interval for the axis
};

//*****************************************************************************
//
// Defines the Y-axis for the strip chart.
//
//*****************************************************************************
static tStripChartAxis sAxisY =
{
    0,          // title of the axis
    0,          // label for minimum of axis
    0,          // label for maximum of axis
    0,          // minimum value for the axis
    63,         // maximum value for the axis
    16           // grid interval for the axis
};

//*****************************************************************************
//
// Defines the strip chart widget.  This structure must be fully initialized
// by calling the function StripChartMgrInit().
//
//*****************************************************************************
StripChart(g_sStripChart, 0, 0, 0, 0, 0, 0, 96, 64,
           0, g_pFontFixed6x8, ClrBlack, ClrWhite, ClrWhite, ClrDarkGreen,
           &sAxisX, &sAxisY, 0);

//*****************************************************************************
//
// Creates a buffer space for the values in the data series.  The buffer
// must be large enough to hold all of the data for the maximum possible
// number of data items that are selected.  If less than the maximum number
// are selected then some of the buffer space will be unused.
//
//*****************************************************************************
static unsigned char g_ucSeriesData[MAX_NUM_SERIES * SERIES_LENGTH];

//*****************************************************************************
//
// The count of data series that are selected for showing on the strip chart.
// This value is set when the client calls the function StripChartConfigure().
//
//*****************************************************************************
static unsigned long g_ulSelectedCount;

//*****************************************************************************
//
// The number of items (per series) that have been added to the strip chart.
//
//*****************************************************************************
static unsigned long g_ulItemCount;

//*****************************************************************************
//
// A bit mask of the specific data items that have been selected for logging.
//
//*****************************************************************************
static unsigned long g_ulSelectedMask;

//*****************************************************************************
//
// Configure the strip chart for a selected set of data series.  The selected
// series is passed in as a bit mask.  Each bit that is set in the bit mask
// represents a selected series.  This function will go through the possible
// set of data series and for each that is selected it will be initialized
// and added to the strip chart.
//
//*****************************************************************************
void
StripChartMgrConfigure(unsigned long ulSelectedMask)
{
    unsigned long ulIdx;
    unsigned long ulItemIdx;

    //
    // Save the channel mask for later use
    //
    g_ulSelectedMask = ulSelectedMask;

    //
    // Determine how many series are to appear in the strip chart.
    //
    g_ulSelectedCount = 0;
    ulIdx = ulSelectedMask;
    while(ulIdx)
    {
        if(ulIdx & 1)
        {
            g_ulSelectedCount++;
        }
        ulIdx >>= 1;
    }

    //
    // Reset the number of items that have been stored in the series buffers.
    //
    g_ulItemCount = 0;

    //
    // Remove any series that were already added to the strip chart.
    //
    g_sStripChart.pSeries = 0;

    //
    // Loop through all series, and configure the selected series and add
    // them to the strip chart.
    //
    ulItemIdx = 0;
    for(ulIdx = 0; ulIdx < MAX_NUM_SERIES; ulIdx++)
    {
        //
        // Check to see if this series is selected
        //
        if((1 << ulIdx) & ulSelectedMask)
        {
            //
            // Get a pointer to this series
            //
            tStripChartSeries *pSeries = &g_sSeries[ulIdx];

            //
            // Set the stride for this series.  It will be the same as the
            // number of enabled data items
            //
            pSeries->ucStride = g_ulSelectedCount;

            //
            // Set the series data pointer to start at the first location
            // in the series buffer where this data item will appear.
            //
            pSeries->pvData = &g_ucSeriesData[ulItemIdx];

            //
            // Add the series to the strip chart
            //
            StripChartSeriesAdd(&g_sStripChart, pSeries);

            //
            // Increment the index of selected series
            //
            ulItemIdx++;
        }
    }
}

//*****************************************************************************
//
// Scales the input data value to a Y pixel range according to the scaling
// table at the top of this file.
//
//*****************************************************************************
static unsigned char
ScaleDataToPixelY(short sData, short sMin, short sMax)
{
    long lY;
    short sRange;

    //
    // Adjust the input value so that the min will be the bottom of display.
    //
    sData -= sMin;

    //
    // Compute the range of the input that will appear on the display
    //
    sRange = sMax - sMin;

    //
    // Scale the input to the Y pixel range of the display
    //
    lY = (long)sData * 63L;

    //
    // Add in half of divisor to get proper rounding
    //
    lY += (long)sRange / 2L;

    //
    // Apply final divisor to get Y pixel value on display
    //
    lY /= (long)sRange;

    //
    // If the Y coordinate is out of the range of the display, force the
    // value to be just off the display, in order to avoid aliasing to a
    // bogus Y pixel value when the return value is converted to a smaller
    // data type.
    //
    if((lY < 0) || (lY > 63))
    {
        lY = 64;
    }

    //
    // Return the Y pixel value
    //
    return((unsigned char)lY);
}

//*****************************************************************************
//
// Add data items to the strip chart and advance the strip chart position.
// This function will add the items pointed to by the parameter to the data
// series buffer and the strip chart will be updated to reflect the newly
// added data items.
// This function will assume that the number of items passed by the pointer
// is the same as was selected by the function StripChartMgrConfigure().  It is
// up to the caller to ensure that the amount of data passed matches the
// number of items that were selected when the strip chart was configured.
//
//*****************************************************************************
void
StripChartMgrAddItems(short *psDataItems)
{
    unsigned long ulIdx;
    unsigned char *pucNewData;
    unsigned long ulSelectedMask = g_ulSelectedMask;

    //
    // Check for valid input data pointer
    //
    if(!psDataItems)
    {
        return;
    }

    //
    // If the number count of items in the strip chart is at the maximum, then
    // the items need to "slide down" and new data added to the end of the
    // buffer.
    //
    if(g_ulItemCount == SERIES_LENGTH)
    {
        memmove(&g_ucSeriesData[0], &g_ucSeriesData[g_ulSelectedCount],
                SERIES_LENGTH * g_ulSelectedCount);

        //
        // Set the pointer for newly added data to be the end of the series
        // buffer.
        //
        pucNewData = &g_ucSeriesData[(SERIES_LENGTH - 1) * g_ulSelectedCount];
    }

    //
    // Otherwise, the series data buffer is less than full so compute the
    // correct location in the buffer for the new data to be added.
    //
    else
    {
        pucNewData = &g_ucSeriesData[g_ulItemCount * g_ulSelectedCount];

        //
        // Increment the number of items that have been added to the strip
        // chart series data buffer.
        //
        g_ulItemCount++;

        //
        // Since the count of data items has changed, it must be updated for
        // each series.
        //
        for(ulIdx = 0; ulIdx < MAX_NUM_SERIES; ulIdx++)
        {
            g_sSeries[ulIdx].usNumItems = g_ulItemCount;
        }
    }

    //
    // Convert each of the input data items being added to the strip chart
    // to a scaled Y pixel value.
    //
    ulIdx = 0;
    while(ulSelectedMask)
    {
        //
        // Is this data item being added to the chart?
        //
        if(ulSelectedMask & 1)
        {
            //
            // Scale the data item and add it to the series buffer
            //
            *pucNewData = ScaleDataToPixelY(*psDataItems, sScaling[ulIdx].sMin,
                                            sScaling[ulIdx].sMax);

            //
            // Increment the from/to pointers
            //
            pucNewData++;
            psDataItems++;
        }
        ulIdx++;
        ulSelectedMask >>= 1;
    }

    //
    // Now that data has been added to the strip chart series buffers, either
    // at the end or in the middle, advance the strip chart position by 1.
    // Add a request for painting the strip chart widget.
    //
    StripChartAdvance(&g_sStripChart, 1);
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// Initializes the strip chart manager.  The strip chart needs an on-screen
// and off-screen display for drawing.  These are passed using the init
// function.
//
//*****************************************************************************
void
StripChartMgrInit(void)
{
    g_sStripChart.sBase.pDisplay = &g_sCFAL96x64x16;
    g_sStripChart.pOffscreenDisplay = &g_sOffscreenDisplayA;
}
