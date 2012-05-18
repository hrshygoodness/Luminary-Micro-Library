//*****************************************************************************
//
// stripchartwidget.c - A simple strip chart widget.
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

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "stripchartwidget.h"

//*****************************************************************************
//
//! \addtogroup stripchartwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is a custom widget for drawing a simple strip chart.  The strip
// chart can be configured with an X/Y grid, and data series can be added
// to and displayed on the strip chart.  The strip chart can be "advanced"
// so that the grid lines will move on the display.  Before advancing
// the chart, the application must update the series data in the buffers.
// The strip chart will only display whatever is in the series buffers, the
// application must scroll the data in the series data buffers.  By adjusting
// the data in the series data buffers, advancing the strip chart, and
// repainting, the strip chart can be made to scroll the data across the
// display.
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws the strip chart into a drawing context, off-screen buffer.
//!
//! \param pChartWidget points at the StripChartWidget being processed.
//! \param pContext points to the context where all drawing should be done.
//!
//! This function renders a strip chart into a drawing context.
//! It assumes that the drawing context is an off-screen buffer, and that
//! the entire buffer belongs to this widget.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartDraw(tStripChartWidget *pChartWidget, tContext *pContext)
{
    tStripChartAxis *pAxisY;
    long lY;
    long lYgrid;
    long lX;
    long lGridRange;
    long lDispRange;
    long lGridMin;
    long lDispMax;
    tStripChartSeries *pSeries;

    //
    // Check the parameters
    //
    ASSERT(pChartWidget);
    ASSERT(pContext);
    ASSERT(pChartWidget->pAxisY);

    //
    // Get handy pointer to Y axis
    //
    pAxisY = pChartWidget->pAxisY;

    //
    // Find the range of Y axis in Y axis units
    //
    lGridRange = pAxisY->lMax - pAxisY->lMin;

    //
    // Find the range of the Y axis in display units (pixels)
    //
    lDispRange = pContext->sClipRegion.sYMax - pContext->sClipRegion.sYMin;

    //
    // Find the minimum Y units value to be shown, and the maximum of the
    // clipping region.
    //
    lGridMin = pAxisY->lMin;
    lDispMax = pContext->sClipRegion.sYMax;

    //
    // Set the fg color for the rectangle fill to match what we want as the
    // chart background.
    //
    GrContextForegroundSet(pContext, pChartWidget->ulBackgroundColor);
    GrRectFill(pContext, &pContext->sClipRegion);

    //
    // Draw vertical grid lines
    //
    GrContextForegroundSet(pContext, pChartWidget->ulGridColor);
    for(lX = pChartWidget->lGridX; lX < pContext->sClipRegion.sXMax;
        lX += pChartWidget->pAxisX->lGridInterval)
    {
        GrLineDrawV(pContext, pContext->sClipRegion.sXMax - lX,
                    pContext->sClipRegion.sYMin,
                    pContext->sClipRegion.sYMax);
    }

    //
    // Draw horizontal grid lines
    //
    for(lYgrid = pAxisY->lMin; lYgrid < pAxisY->lMax;
        lYgrid += pAxisY->lGridInterval)
    {
        lY = ((lYgrid - lGridMin) * lDispRange) / lGridRange;
        lY = lDispMax - lY;
        GrLineDrawH(pContext, pContext->sClipRegion.sXMin,
                    pContext->sClipRegion.sXMax, lY);
    }

    //
    // Compute location of Y=0 line, and draw it
    //
    lY = ((-lGridMin) * lDispRange) / lGridRange;
    lY = lDispMax - lY;
    GrLineDrawH(pContext, pContext->sClipRegion.sXMin,
                pContext->sClipRegion.sXMax, lY);

    //
    // Iterate through each series to draw it
    //
    pSeries = pChartWidget->pSeries;
    while(pSeries)
    {
        int idx = 0;

        //
        // Find the starting X position on the display for this series.
        // If the series has less data points than can fit on the display
        // then starting X can be somewhere in the middle of the screen.
        //
        lX = 1 + pContext->sClipRegion.sXMax - pSeries->usNumItems;

        //
        // If the starting X is off the left side of the screen, then the
        // staring index (idx) for reading data needs to be adjusted to the
        // first value in the series that will be visible on the screen
        //
        if(lX < pContext->sClipRegion.sXMin)
        {
            idx = pContext->sClipRegion.sXMin - lX;
            lX = pContext->sClipRegion.sXMin;
        }

        //
        // Set the drawing color for this series
        //
        GrContextForegroundSet(pContext, pSeries->ulColor);

        //
        // Scan through all possible X values, find the Y value, and draw the
        // pixel.
        //
        for(; lX <= pContext->sClipRegion.sXMax; lX++)
        {
            //
            // Find the Y value at each position in the data series.  Take into
            // account the data size and the stride
            //
            if(pSeries->ucDataTypeSize == 1)
            {
                lY = ((signed char *)pSeries->pvData)[idx * pSeries->ucStride];
            }
            else if(pSeries->ucDataTypeSize == 2)
            {
                lY = ((short *)pSeries->pvData)[idx * pSeries->ucStride];
            }
            else if(pSeries->ucDataTypeSize == 4)
            {
                lY = ((long *)pSeries->pvData)[idx * pSeries->ucStride];
            }
            else
            {
                //
                // If there is an invalid data size, then just force Y value
                // to be off the display
                //
                lY = lDispMax + 1;
                break;
            }

            //
            // Advance to the next position in the data series.
            //
            idx++;

            //
            // Now scale the Y value according to the axis scaling
            //
            lY = ((lY - lGridMin) * lDispRange) / lGridRange;
            lY = lDispMax - lY;

            //
            // Draw the pixel on the display
            //
            GrPixelDraw(pContext, lX, lY);
        }

        //
        // Advance to the next series until there are no more.
        //
        pSeries = pSeries->pNextSeries;
    }

    //
    // Draw a frame around the entire chart.
    //
    GrContextForegroundSet(pContext, pChartWidget->ulY0Color);
    GrRectDraw(pContext, &pContext->sClipRegion);

    //
    // Draw titles
    //
    GrContextForegroundSet(pContext, pChartWidget->ulTextColor);
    GrContextFontSet(pContext, pChartWidget->pFont);

    //
    // Draw the chart title, if there is one
    //
    if(pChartWidget->pcTitle)
    {
        GrStringDrawCentered(pContext, pChartWidget->pcTitle, -1,
                             pContext->sClipRegion.sXMax / 2,
                             GrFontHeightGet(pChartWidget->pFont), 0);
    }

    //
    // Draw the Y axis max label, if there is one
    //
    if(pChartWidget->pAxisY->pcMaxLabel)
    {
        GrStringDraw(pContext, pChartWidget->pAxisY->pcMaxLabel, -1,
                     pContext->sClipRegion.sXMin +
                     GrFontMaxWidthGet(pChartWidget->pFont) / 2,
                     GrFontHeightGet(pChartWidget->pFont) / 2, 0);
    }

    //
    // Draw the Y axis min label, if there is one
    //
    if(pChartWidget->pAxisY->pcMinLabel)
    {
        GrStringDraw(pContext, pChartWidget->pAxisY->pcMinLabel, -1,
                     pContext->sClipRegion.sXMin +
                     GrFontMaxWidthGet(pChartWidget->pFont) / 2,
                     pContext->sClipRegion.sYMax -
                     (GrFontHeightGet(pChartWidget->pFont) +
                      (GrFontHeightGet(pChartWidget->pFont) / 2)),
                     0);
    }

    //
    // Draw a label for the name of the Y axis, if there is one
    //
    if(pChartWidget->pAxisY->pcName)
    {
        GrStringDraw(pContext, pChartWidget->pAxisY->pcName, -1,
                     pContext->sClipRegion.sXMin + 1,
                     (pContext->sClipRegion.sYMax / 2) -
                     (GrFontHeightGet(pChartWidget->pFont) / 2),
                     1);
    }
}

//*****************************************************************************
//
//! Paints the strip chart on the display.
//!
//! \param pWidget is a pointer to the strip chart widget to be drawn.
//!
//! This function draws the contents of a strip chart on the display.  This is
//! called in response to a \b WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
StripChartPaint(tWidget *pWidget)
{
    tStripChartWidget *pChartWidget;
    tContext sContext;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);
    ASSERT(pWidget->pDisplay);

    //
    // Convert the generic widget pointer into a strip chart widget pointer.
    //
    pChartWidget = (tStripChartWidget *)pWidget;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    ASSERT(pChartWidget->pOffscreenDisplay);
    GrContextInit(&sContext, pChartWidget->pOffscreenDisplay);

    //
    // Render the strip chart into the off-screen buffer
    //
    StripChartDraw(pChartWidget, &sContext);

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, pWidget->pDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(pWidget->sPosition));

    //
    // Now copy the rendered strip chart into the physical display
    //
    GrImageDraw(&sContext, pChartWidget->pOffscreenDisplay->pvDisplayData,
                pWidget->sPosition.sXMin, pWidget->sPosition.sYMin);
}

//*****************************************************************************
//
//! Advances the strip chart X grid by a certain number of pixels.
//!
//! \param pChartWidget is a pointer to the strip chart widget to be advanced.
//! \param lCount is the number of positions to advance the grid.
//!
//! This function advances the X grid of the strip chart by the specified
//! number of positions.  By using this function to advance the grid in
//! combination with updating the data in the series data buffers, the strip
//! chart can be made to appear to scroll across the display.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartAdvance(tStripChartWidget *pChartWidget, long lCount)
{
    //
    // Adjust the starting point of the X-grid
    //
    pChartWidget->lGridX += lCount;
    pChartWidget->lGridX %= pChartWidget->pAxisX->lGridInterval;
}

//*****************************************************************************
//
//! Adds a data series to the strip chart.
//!
//! \param pWidget is a pointer to the strip chart widget to be modified.
//! \param pNewSeries is a strip chart data series to be added to the strip
//! chart.
//!
//! This function will add a data series to the strip chart.  This function
//! just links the series into the strip chart.  It is up to the application
//! to make sure that the data series is initialized correctly.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartSeriesAdd(tStripChartWidget *pWidget, tStripChartSeries *pNewSeries)
{
    //
    // If there is already at least one series in this chart, then link
    // in to the existing chain.
    //
    if(pWidget->pSeries)
    {
        tStripChartSeries *pSeries = pWidget->pSeries;
        while(pSeries->pNextSeries)
        {
            pSeries = pSeries->pNextSeries;
        }
        pSeries->pNextSeries = pNewSeries;
    }

    //
    // Otherwise, there is not already a series in this chart, so set this
    // new series as the first series for the chart.
    //
    else
    {
        pWidget->pSeries = pNewSeries;
    }
    pNewSeries->pNextSeries = 0;
}

//*****************************************************************************
//
//! Removes a data series from the strip chart.
//!
//! \param pWidget is a pointer to the strip chart widget to be modified.
//! \param pOldSeries is a strip chart data series that is to be removed
//! from the strip chart.
//!
//! This function will remove an existing data series from a strip chart.  It
//! will search the list of data series for the specified series, and if
//! found it will be unlinked from the chain of data series for this strip
//! chart.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartSeriesRemove(tStripChartWidget *pWidget,
                       tStripChartSeries *pOldSeries)
{
    //
    // If the series to be removed is the first one, then find the next
    // series in the chain and set it to be first.
    //
    if(pWidget->pSeries == pOldSeries)
    {
        pWidget->pSeries = pOldSeries->pNextSeries;
    }

    //
    // Otherwise, scan through the chain to find the old series
    //
    else
    {
        tStripChartSeries *pSeries = pWidget->pSeries;
        while(pSeries->pNextSeries)
        {
            //
            // If the old series is found, unlink it from the chain
            //
            if(pSeries->pNextSeries == pOldSeries)
            {
                pSeries->pNextSeries = pOldSeries->pNextSeries;
                break;
            }
            else
            {
                pSeries = pSeries->pNextSeries;
            }
        }
    }

    //
    // Finally, set the "next" pointer of the old series to null so that
    // there will not be any confusing chain fragments if this series is
    // reused.
    //
    pOldSeries->pNextSeries = 0;
}

//*****************************************************************************
//
//! Handles messages for a strip chart widget.
//!
//! \param pWidget is a pointer to the strip chart widget.
//! \param ulMsg is the message.
//! \param ulParam1 is the first parameter to the message.
//! \param ulParam2 is the second parameter to the message.
//!
//! This function receives messages intended for this strip chart widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
long
StripChartMsgProc(tWidget *pWidget, unsigned long ulMsg, unsigned long ulParam1,
                  unsigned long ulParam2)
{
    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Determine which message is being sent.
    //
    switch(ulMsg)
    {
        //
        // The widget paint request has been sent.
        //
        case WIDGET_MSG_PAINT:
        {
            //
            // Handle the widget paint request.
            //
            StripChartPaint(pWidget);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // Deliberately ignore all button press messages.  They may be handled
        // by another widget.
        //
        case WIDGET_MSG_KEY_SELECT:
        case WIDGET_MSG_KEY_UP:
        case WIDGET_MSG_KEY_DOWN:
        case WIDGET_MSG_KEY_LEFT:
        case WIDGET_MSG_KEY_RIGHT:
        {
            return(0);
        }

        //
        // An unknown request has been sent.
        //
        default:
        {
            //
            // Let the default message handler process this message.
            //
            return(WidgetDefaultMsgProc(pWidget, ulMsg, ulParam1, ulParam2));
        }
    }
}

//*****************************************************************************
//
//! Initializes a strip chart widget.
//!
//! \param pWidget is a pointer to the strip chart widget to initialize.
//! \param pDisplay is a pointer to the display on which to draw the chart.
//! \param lX is the X coordinate of the upper left corner of the canvas.
//! \param lY is the Y coordinate of the upper left corner of the canvas.
//! \param lWidth is the width of the canvas.
//! \param lHeight is the height of the canvas.
//! \param pcTitle is the label text for the strip chart
//! \param pFont is the font to use for drawing text on the chart.
//! \param ulBackgroundColor is the colr of the background for the chart.
//! \param ulTextColor is the color used for drawing text.
//! \param ulY0Color is the color used for drawing the Y=0 line and the frame
//! around the chart.
//! \param ulGridColor is the color of the X/Y grid
//! \param pAxisX is a pointer to the X-axis object
//! \param pAxisY is a pointer to the Y-axis object
//! \param pOffscreenDisplay is a pointer to an offscreen display that will
//! be used for rendering the strip chart prior to drawing it on the
//! physical display.
//!
//! This function initializes the caller provided strip chart widget.
//!
//! \return None.
//
//*****************************************************************************
void
StripChartInit(tStripChartWidget *pWidget, const tDisplay *pDisplay,
              long lX, long lY, long lWidth, long lHeight,
              char * pcTitle, tFont *pFont,
              unsigned long ulBackgroundColor,
              unsigned long ulTextColor,
              unsigned long ulY0Color,
              unsigned long ulGridColor,
              tStripChartAxis *pAxisX, tStripChartAxis *pAxisY,
              tDisplay *pOffscreenDisplay)
{
    unsigned long ulIdx;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);
    ASSERT(pDisplay);
    ASSERT(pAxisX);
    ASSERT(pAxisY);
    ASSERT(pOffscreenDisplay);

    //
    // Clear out the widget structure.
    //
    for(ulIdx = 0; ulIdx < sizeof(tStripChartWidget); ulIdx += 4)
    {
        ((unsigned long *)pWidget)[ulIdx / 4] = 0;
    }

    //
    // Set the size of the widget structure.
    //
    pWidget->sBase.lSize = sizeof(tStripChartWidget);

    //
    // Mark this widget as fully disconnected.
    //
    pWidget->sBase.pParent = 0;
    pWidget->sBase.pNext = 0;
    pWidget->sBase.pChild = 0;

    //
    // Save the display pointer.
    //
    pWidget->sBase.pDisplay = pDisplay;

    //
    // Set the extents of the display area.
    //
    pWidget->sBase.sPosition.sXMin = lX;
    pWidget->sBase.sPosition.sYMin = lY;
    pWidget->sBase.sPosition.sXMax = lX + lWidth - 1;
    pWidget->sBase.sPosition.sYMax = lY + lHeight - 1;

    //
    // Initialize the widget fields
    //
    pWidget->pcTitle = pcTitle;
    pWidget->pFont = pFont;
    pWidget->ulBackgroundColor = ulBackgroundColor;
    pWidget->ulTextColor = ulTextColor;
    pWidget->ulY0Color = ulY0Color;
    pWidget->ulGridColor = ulGridColor;
    pWidget->pAxisX = pAxisX;
    pWidget->pAxisY = pAxisY;
    pWidget->pOffscreenDisplay = pOffscreenDisplay;

    //
    // Use the strip chart message handler to process messages to this widget.
    //
    pWidget->sBase.pfnMsgProc = StripChartMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

