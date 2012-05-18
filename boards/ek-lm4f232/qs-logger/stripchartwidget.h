//*****************************************************************************
//
// stripchartwidget.h - Prototypes for a strip chart widget.
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
// This is part of revision 8555 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#ifndef __STRIPCHARTWIDGET_H__
#define __STRIPCHARTWIDGET_H__

//*****************************************************************************
//
//! \addtogroup stripchartwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! A structure that represents a data series to be shown on the strip chart.
//
//*****************************************************************************
typedef struct _StripChartSeries
{
    //
    //! A pointer to the next series in the chart.
    //
    struct _StripChartSeries *pNextSeries;

    //
    //! A pointer to the brief name of the data set
    //
    char *pcName;

    //
    //! The color of the data series.
    //
    unsigned long ulColor;

    //
    //! The number of bytes of the data type (1, 2, or 4)
    //
    unsigned char ucDataTypeSize;

    //
    //! The stride of the data.  This can be used when this data set is
    //! part of a larger set of samples that appear in a large array
    //! interleaved at a regular interval.  Use a value of 1 if the data set
    //! is not interleaved.
    //
    unsigned char ucStride;

    //
    //! The number of items in the data set
    //
    unsigned short usNumItems;

    //
    //! A pointer to the first data item.
    //
    void *pvData;
}
tStripChartSeries;

//*****************************************************************************
//
//! A structure that represents an axis of the strip chart.
//
//*****************************************************************************
typedef struct _StripChartAxis
{
    //
    //! A brief name for the axis.  Leave null for no name to be shown.
    //
    char *pcName;

    //
    //! Label for the minimum extent of the axis.  Leave null for no label.
    //
    char *pcMinLabel;

    //
    //! Label for the max extent of the axis. Leave null for no label.
    //
    char *pcMaxLabel;

    //
    //! The minimum units value for the axis.
    //
    long lMin;

    //
    //! The maximum units value for the axis
    //
    long lMax;

    //
    //! The grid interval for the axis.  Use 0 for no grid.
    //
    long lGridInterval;
} tStripChartAxis;

//*****************************************************************************
//
//! A structure that represents a strip chart widget.
//
//*****************************************************************************
typedef struct _StripChartWidget
{
    //
    //! The generic widget information.
    //
    tWidget sBase;

    //
    //! The title for the strip chart.  Leave null for no title.
    //
    char *pcTitle;

    //
    //! The font to use for drawing text on the chart.
    //
    const tFont *pFont;

    //
    //! The background color of the chart.
    //
    unsigned long ulBackgroundColor;

    //
    //! The color for text that is drawn on the chart (titles, etc).
    //
    unsigned long ulTextColor;

    //
    //! The color of the Y-axis 0-crossing line.
    //
    unsigned long ulY0Color;

    //
    //! The color of the grid lines.
    //
    unsigned long ulGridColor;

    //
    //! The X axis
    //
    tStripChartAxis *pAxisX;

    //
    //! The Y axis
    //
    tStripChartAxis *pAxisY;

    //
    //! A pointer to the first data series for the strip chart.
    //
    tStripChartSeries *pSeries;

    //
    //! A pointer to an off-screen display to be used for rendering the chart.
    //
    const tDisplay *pOffscreenDisplay;

    //
    //! The current X-grid alignment.  This value changes in order to give the
    //! appearance of the grid moving as the strip chart advances.
    //
    long lGridX;
} tStripChartWidget;

//*****************************************************************************
//
//! Declares an initialized strip chart widget data structure.
//!
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the off-screen display on which to draw.
//! \param lX is the X coordinate of the upper left corner of the canvas.
//! \param lY is the Y coordinate of the upper left corner of the canvas.
//! \param lWidth is the width of the canvas.
//! \param lHeight is the height of the canvas.
//! \param pcTitle is a string for the chart title, NULL for no title.
//! \param pFont is the font used for rendering text on the chart.
//! \param ulBackgroundColor is the background color for the chart.
//! \param ulTextColor is the color of text (titles, labels, etc.)
//! \param ulY0Color is the color of the Y-axis gridline at Y=0
//! \param ulGridColor is the color of grid lines.
//! \param pAxisX is a pointer to the axis structure for the X-axis.
//! \param pAxisY is a pointer to the axis structure for the Y-axis.
//! \param pOffscreenDisplay is a buffer for rendering the chart before
//! showing on the physical display.  The dimensions of the off-screen display
//! should match the drawing area of pDisplay.
//!
//! This macro provides an initialized strip chart widget data structure, which
//! can be used to construct the widget tree at compile time in global variables
//! (as opposed to run-time via function calls).  This must be assigned to a
//! variable, such as:
//!
//! \verbatim
//!     tStripChartWidget g_sStripChart = StripChartStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tStripChartWidget g_psStripChart[] =
//!     {
//!         StripChartStruct(...),
//!         StripChartStruct(...)
//!     };
//! \endverbatim
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define StripChartStruct(pParent, pNext, pChild, pDisplay,                    \
                        lX, lY, lWidth, lHeight,                              \
                        pcTitle, pFont, ulBackgroundColor, ulTextColor,       \
                        ulY0Color, ulGridColor, pAxisX, pAxisY,               \
                        pOffscreenDisplay)                                    \
        {                                                                     \
            {                                                                 \
                sizeof(tStripChartWidget),                                     \
                (tWidget *)(pParent),                                         \
                (tWidget *)(pNext),                                           \
                (tWidget *)(pChild),                                          \
                pDisplay,                                                     \
                {                                                             \
                    lX,                                                       \
                    lY,                                                       \
                    (lX) + (lWidth) - 1,                                      \
                    (lY) + (lHeight) - 1                                      \
                },                                                            \
                StripChartMsgProc                                             \
            },                                                                \
            pcTitle, pFont, ulBackgroundColor, ulTextColor, ulY0Color,        \
            ulGridColor, pAxisX, pAxisY, 0, pOffscreenDisplay, 0              \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a strip chart widget data
//! structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the off-screen display on which to draw.
//! \param lX is the X coordinate of the upper left corner of the canvas.
//! \param lY is the Y coordinate of the upper left corner of the canvas.
//! \param lWidth is the width of the canvas.
//! \param lHeight is the height of the canvas.
//! \param pcTitle is a string for the chart title, NULL for no title.
//! \param pFont is the font used for rendering text on the chart.
//! \param ulBackgroundColor is the background color for the chart.
//! \param ulTextColor is the color of text (titles, labels, etc.)
//! \param ulY0Color is the color of the Y-axis gridline at Y=0
//! \param ulGridColor is the color of grid lines.
//! \param pAxisX is a pointer to the axis structure for the X-axis.
//! \param pAxisY is a pointer to the axis structure for the Y-axis.
//! \param pOffscreenDisplay is a buffer for rendering the chart before
//! showing on the physical display.  The dimensions of the off-screen display
//! should match the drawing area of pDisplay.
//!
//! This macro declares a variable containing an initialized strip chart widget
//! data structure, which can be used to construct the widget tree at compile
//! time in global variables (as opposed to run-time via function calls).
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define StripChart(sName, pParent, pNext, pChild, pDisplay,                   \
                lX, lY, lWidth, lHeight,                                      \
                pcTitle, pFont, ulBackgroundColor, ulTextColor,               \
                ulY0Color, ulGridColor, pAxisX, pAxisY,                       \
                pOffscreenDisplay)                                            \
        tStripChartWidget sName =                                             \
            StripChartStruct(pParent, pNext, pChild, pDisplay,                \
                            lX, lY, lWidth,  lHeight,                         \
                            pcTitle, pFont, ulBackgroundColor, ulTextColor,   \
                            ulY0Color, ulGridColor, pAxisX, pAxisY,           \
                            pOffscreenDisplay)

//*****************************************************************************
//
//! Sets the X-axis of the strip chart.
//!
//! \param pStripChartWidget is a pointer to the strip chart widget to modify.
//! \param pAxis is the new X-axis structure for the strip chart.
//!
//! This function sets the X-axis for the widget.
//!
//! \return None.
//
//*****************************************************************************
#define StripChartXAxisSet(pStripChartWidget, pAxis)                          \
    do                                                                        \
    {                                                                         \
        (pStripChartWidget)->pAxisX = pAxis;                                  \
    } while(0)

//*****************************************************************************
//
//! Sets the Y-axis of the strip chart.
//!
//! \param pStripChartWidget is a pointer to the strip chart widget to modify.
//! \param pAxis is the new Y-axis structure for the strip chart.
//!
//! This function sets the Y-axis for the widget.
//!
//! \return None.
//
//*****************************************************************************
#define StripChartYAxisSet(pStripChartWidget, pAxis)                          \
    do                                                                        \
    {                                                                         \
        (pStripChartWidget)->pAxisY = pAxis;                                  \
    } while(0)

//*****************************************************************************
//
// Prototypes for the strip chart widget APIs.
//
//*****************************************************************************
extern long StripChartMsgProc(tWidget *pWidget, unsigned long ulMsg,
                              unsigned long ulParam1, unsigned long ulParam2);
extern void StripChartInit(tStripChartWidget *pWidget, const tDisplay *pDisplay,
                          long lX, long lY, long lWidth, long lHeight,
                          char * pcTitle, tFont *pFont,
                          unsigned long ulBackgroundColor,
                          unsigned long ulTextColor,
                          unsigned long ulY0Color,
                          unsigned long ulGridColor,
                          tStripChartAxis *pAxisX, tStripChartAxis *pAxisY,
                          tDisplay *pOffscreenDisplay);
extern void StripChartSeriesAdd(tStripChartWidget *pWidget,
                                tStripChartSeries *pSeries);
extern void StripChartSeriesRemove(tStripChartWidget *pWidget,
                                   tStripChartSeries *pSeries);
extern void StripChartAdvance(tStripChartWidget *pChartWidget, long lCount);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __STRIPCHARTWIDGET_H__
