//*****************************************************************************
//
// CStripChart.h - A strip chart widget for use with FLTK.
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
// This is part of revision 8555 of the Stellaris Firmware Development Package.
//
//*****************************************************************************
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include "CStripChart.h"

#ifndef INT_MAX
#define INT_MAX 0x7FFFFFFF
#endif

//*****************************************************************************
//
// Constructor
//
//*****************************************************************************
CStripChart::CStripChart(int iX, int iY, int iW, int iH, const char *pcLabel) :
  Fl_Box(iX, iY, iW, iH, pcLabel)
{
    //
    // Initialize our member variables.
    //
    miNumChannels = 1;
    miChannelVisibility = 1;
    miNumSamples = iW;
    mpColorChannels = (Fl_Color *)0;
    mppiChannelDataTables = (int **)0;
    SetNumChannels(miNumChannels);

    //
    // Default scaling.
    //
    miMin = 0;
    miMax = 100;
    miPrecision = 0;

    //
    // Default annotation colors.
    //
    mColorXAnnotation = FL_DARK_YELLOW;
    mColorYAnnotation = FL_DARK_YELLOW;
    mColorYAxisLabels = FL_DARK_YELLOW;

    //
    // Default annotation.
    //
    muiAnnotationFlags = ANNOTATION_FLAG_SHOW_X_MARKERS |
                         ANNOTATION_FLAG_SHOW_Y_MARKERS;
    miXMarkerSpacing = 50;
    miYMarkerSpacing = 25;
    miXMarkerCount = 0;
    miXStyle = FL_DOT;
    miYStyle = FL_DOT;

    //
    // Default font and size for Y axis labels.
    //
    mYLabelFont = FL_HELVETICA;
    miYFontSize = 10;
}

//*****************************************************************************
//
// Destructor
//
//*****************************************************************************
CStripChart::~CStripChart(void)
{
    //
    // Free any data tables we have allocated.
    //
    PrivDiscardDataTables();
}

//*****************************************************************************
//
// Private: Get the index of the next sample in the buffer, taking into account
// the wrap.
//
//*****************************************************************************
int CStripChart::PrivPrevious(int iIndex)
{
    if(!iIndex)
    {
        return(miNumSamples - 1);
    }
    else
    {
        return(iIndex - 1);
    }
}

//*****************************************************************************
//
// Private: Scale and translate a supplied Y value into a window coordinate.
//
//*****************************************************************************
int CStripChart::PrivScaleY(int iY)
{
    int iResult;

    //
    // Determine the Y position in the window equivalent to the value passed
    // given the maximum and minimum range set for the control.
    //
    iResult = y() + (((miMax - iY) * h()) / (miMax - miMin));

    return(iResult);
}

//*****************************************************************************
//
// Private: Free any existing channel data table and color table.
//
//*****************************************************************************
void CStripChart::PrivDiscardDataTables(void)
{
    //
    // Free up our existing resources if they have been allocated.
    //
    if(mpColorChannels)
    {
        delete mpColorChannels;
        mpColorChannels = (Fl_Color *)0;
    }

    if(mppiChannelDataTables)
    {
        int iCount;

        //
        // Free up the buffers for each data channel.
        //
        for(iCount = 0; iCount < miNumChannels; iCount++)
        {
            if(mppiChannelDataTables[iCount])
            {
                delete mppiChannelDataTables[iCount];
            }
        }

        //
        // Free the channel data table pointers.
        //
        delete mppiChannelDataTables;
        mppiChannelDataTables = (int **)0;
    }

    //
    // Clear our write index to mark our data buffers as empty.
    //
    mbFull = 0;
    miWriteIndex = 0;
}

//*****************************************************************************
//
// Set the number of data channels that the strip chart should display.
// By default, a newly created strip chart displays 1 channel but this
// function may be called to increase that number up to a maximum of 32.
//
//*****************************************************************************
void CStripChart::SetNumChannels(int iNumChannels)
{
    int iLoop;

    //
    // If we've been asked to configure for more than 32 channels, ignore
    // the call.
    //
    if(iNumChannels >= 32)
    {
        return;
    }

    //
    // Free any existing channel data and color tables.
    //
    PrivDiscardDataTables();

    //
    // Allocate the channel color table.
    //
    mpColorChannels = new Fl_Color[iNumChannels];

    //
    // Allocate the channel data table.  The number of entries for each channel
    // is set to the width of the window since we only need to keep this
    // number samples to allow us to redraw the display in the worst case.
    //
    mppiChannelDataTables = new int *[iNumChannels];

    //
    // Make sure we allocated both tables.
    //
    if(!mpColorChannels || !mppiChannelDataTables)
    {
        PrivDiscardDataTables();
        return;
    }

    //
    // Now initialize each data channel.
    //
    for(iLoop = 0; iLoop < iNumChannels; iLoop++)
    {
        //
        // Allocate the data table for this channel.
        //
        mppiChannelDataTables[iLoop] = new int[miNumSamples];
        if(!mppiChannelDataTables[iLoop])
        {
            PrivDiscardDataTables();
            return;
        }

        //
        // Set the default color for this channel's data to white.
        //
        mpColorChannels[iLoop] = FL_WHITE;
    }

    //
    // If we get here, all is well so we can reset the number of channels.
    //
    miNumChannels = iNumChannels;
}

//*****************************************************************************
//
// Return the number of channels that the strip chart is configured to
// display.
//
//*****************************************************************************
int CStripChart::GetNumChannels()
{
    return(miNumChannels);
}

//*****************************************************************************
//
// Set the Y range represented by the control.  The X range is defined
// purely by the width of the control since we move the chart 1 pixel left
// for each new data value added.
//
//*****************************************************************************
void CStripChart::SetRange(int iMin, int iMax)
{
    //
    // Trivial case - return immediately without doing anything if the range
    // hasn't changed.
    //
    if((iMin == miMin) && (iMax == miMax))
    {
        return;
    }

    //
    // Remember the new range values.
    //
    miMin = iMin;
    miMax = iMax;

    //
    // Force a full redraw the next time we need to paint the control.
    //
    damage(FL_DAMAGE_ALL);
}

//*****************************************************************************
//
// Set the Y range divider used in displaying labels.  The number passed is a
// power of 10 that will be used to divide Y values before displaying a label.
// For example, providing 2 here will display Y labels generated by dividing
// the integer range by 100 and displaying the result as a floating point
// value with 2 digits following the decimal point.
//
//*****************************************************************************
void CStripChart::SetPrecision(int iPrecision)
{
    //
    // Trivial case - return immediately without doing anything if the precision
    // hasn't changed.
    //
    if(iPrecision == miPrecision)
    {
        return;
    }

    //
    // Remember the new precision value.
    //
    miPrecision = iPrecision;

    //
    // Force a full redraw the next time we need to paint the control.
    //
    damage(FL_DAMAGE_ALL);
}

//*****************************************************************************
//
// Return the current Y precision setting.
//
//*****************************************************************************
int CStripChart::GetPrecision(void)
{
    return(miPrecision);
}

//*****************************************************************************
//
// Enable or disable various annotations on the chart.
//
//*****************************************************************************
void CStripChart::SetAnnotation(unsigned int uiAnnotationFlags,
                                int iXMarkerSpacing,
                                int iYMarkerSpacing)
{
    //
    // Trivial case - return immediately without doing anything if the values
    // haven't changed.
    //
    if((uiAnnotationFlags == muiAnnotationFlags) &&
       (iXMarkerSpacing == miXMarkerSpacing) &&
       (iYMarkerSpacing == miYMarkerSpacing))
    {
        return;
    }

    //
    // Remember the annotation style.
    //
    muiAnnotationFlags = uiAnnotationFlags;
    miXMarkerSpacing = iXMarkerSpacing;
    miYMarkerSpacing = iYMarkerSpacing;
    miXMarkerCount = 0;

    //
    // Force a full redraw the next time we need to paint the control.
    //
    damage(FL_DAMAGE_ALL);
}

//*****************************************************************************
//
// Set the colors of the annotations on the chart.
//
//*****************************************************************************
void CStripChart::SetAnnotationColors(Fl_Color XColor, Fl_Color YColor,
                                      Fl_Color YLabelColor)
{
    //
    // Trivial case - return immediately without doing anything if the values
    // haven't changed.
    //
    if((XColor == mColorXAnnotation) &&
       (YColor == mColorYAnnotation) &&
       (YLabelColor == mColorYAxisLabels))
    {
        return;
    }

    //
    // Remember the annotation colors.
    //
    mColorXAnnotation = XColor;
    mColorYAnnotation = YColor;
    mColorYAxisLabels = YLabelColor;

    //
    // Force a full redraw the next time we need to paint the control.
    //
    damage(FL_DAMAGE_ALL);
}

//*****************************************************************************
//
// Set the font and character size used for Y axis labels if
// ANNOTATION_FLAG_LABEL_Y_AXIS has been set.
//
//*****************************************************************************
void CStripChart::SetAxisFont(Fl_Font AxisFont, int iSize)
{
    //
    // Trivial case - return immediately without doing anything if the values
    // haven't changed.
    //
    if((AxisFont == mYLabelFont) && (iSize == miYFontSize))
    {
        return;
    }

    //
    // Remember the new settings.
    //
    mYLabelFont = AxisFont;
    miYFontSize = iSize;

    //
    // Force a full redraw the next time we need to paint the control.
    //
    damage(FL_DAMAGE_ALL);
}

//*****************************************************************************
//
// Set the line style to use for X and Y axis markings.  Valid values are
// as for fl_line_style().
//
//*****************************************************************************
void CStripChart::SetAxisStyle(int iXStyle, int iYStyle)
{
    //
    // Trivial case - return immediately without doing anything if the values
    // haven't changed.
    //
    if((iXStyle == miXStyle) &&
       (iYStyle == miYStyle))
    {
        return;
    }

    //
    // Remember the annotation colors.
    //
    miXStyle = iXStyle;
    miYStyle = iYStyle;

    //
    // Force a full redraw the next time we need to paint the control.
    //
    damage(FL_DAMAGE_ALL);
}

//*****************************************************************************
//
// Set the colors used for each of the data channel traces on the chart.
//
//*****************************************************************************
void CStripChart::SetChannelColors(Fl_Color *pColors, int iNumColors)
{
    bool bChanged;

    //
    // For now, assume nothing changed.
    //
    bChanged = false;

    //
    // Copy the supplied colors, checking to see if each changed.
    //
    for(int iLoop = 0; ((iLoop < iNumColors) && (iLoop < miNumChannels));
        iLoop++)
    {
        //
        // Has this color changed?  If so, set a flag.
        //
        if(mpColorChannels[iLoop] != pColors[iLoop])
        {
            //
            // The new color is different so remember it after setting the flag
            // that tells us something changed.
            //
            bChanged = true;
            mpColorChannels[iLoop] = pColors[iLoop];
        }
    }

    //
    // If something changed, force a redraw of the widget.
    //
    //
    // Force a full redraw the next time we need to paint the control.
    //
    if(bChanged)
    {
        damage(FL_DAMAGE_ALL);
    }
}

//*****************************************************************************
//
// Add a new set of data samples to the chart.
//
//*****************************************************************************
void CStripChart::AddData(int *piValues, int iNumValues, unsigned long ulMask)
{
    int iLoop;

    //
    // Loop through each of the channels and copy the provided data.
    //
    for(iLoop = 0; (iLoop < iNumValues) && (iLoop < miNumChannels); iLoop++)
    {
        //
        // Were we supplied a valid sample for this channel?
        //
        if(ulMask & (1 << iLoop))
        {
            //
            // Yes - copy it.
            //
            mppiChannelDataTables[iLoop][miWriteIndex] = piValues[iLoop];
        }
        else
        {
            //
            // No - put MAXINT in the slot.  This will be clipped off in the
            // drawing process and effectively ignored.
            //
            mppiChannelDataTables[iLoop][miWriteIndex] = INT_MAX;
        }
    }

    //
    // Move on to the next free slot.
    //
    miWriteIndex++;

    //
    // Have we wrapped?
    //
    if(miWriteIndex == miNumSamples)
    {
        miWriteIndex = 0;
        mbFull = true;
    }

    //
    // Update our X marker counter and wrap if necessary.
    //
    miXMarkerCount ++;
    if(miXMarkerCount >= miXMarkerSpacing)
    {
        miXMarkerCount = 0;
    }
}

//*****************************************************************************
//
// Clear all stored chart data and repaint the widget.
//
//*****************************************************************************
void CStripChart::Clear(void)
{
    //
    // Clear out our data structure.
    //
    mbFull = false;
    miWriteIndex = 0;

    //
    // Force a redraw.
    //
    damage(FL_DAMAGE_ALL);
}

//*****************************************************************************
//
// Enable or disable channels displayed by the strip chart.  By default all
// channels are enabled.
//
//*****************************************************************************
void CStripChart::SetChannelVisibility(unsigned int uiMask)
{
    int iChannelMask;

    //
    // Determine the mask for all the channels.
    //
    iChannelMask = (1 << (miNumChannels + 1)) - 1;

    //
    // Has anything changed?
    //
    if((uiMask & iChannelMask) == (miChannelVisibility & iChannelMask))
    {
        return;
    }

    //
    // Save the new channel mask.
    //
    miChannelVisibility = uiMask;

    //
    // Force a redraw.
    //
    damage(FL_DAMAGE_ALL);
}

//*****************************************************************************
//
// Query which channels are currently visible.  The return value is the
// mask last passed to SetChannelVisibility().
//
//*****************************************************************************
unsigned int CStripChart::GetChannelVisibility(void)
{
    //
    // Return the current channel visibility mask.
    //
    return(miChannelVisibility);
}

//*****************************************************************************
//
// Draw the strip chart widget.
//
//*****************************************************************************
void CStripChart::draw(void)
{
    Fl_Color Color;
    int iSamplesToDraw, iSample;

    //
    // Get the existing drawing color.
    //
    Color = Fl_Color();

    //
    // Set the current clipping region to our widget's extent.
    //
    fl_push_clip(x(), y(), w(), h());

    //
    // Draw the background rectangle.
    //
    fl_draw_box(FL_FLAT_BOX, x(), y(), w(), h(), color());

    //
    // Do we need to display any annotations?
    //
    if(muiAnnotationFlags)
    {
        //
        // Yes - we've been asked to draw at least one annotation.  Deal with
        // each in turn.
        //
        if(muiAnnotationFlags & ANNOTATION_FLAG_SHOW_X_MARKERS)
        {
            //
            // Set the drawing style and color we have been asked to use.  Note
            // that the order here is important.  According to the FLTK
            // documentation, Windows forgets the line style if you set the
            // color after the line style.
            //
            fl_color(mColorXAnnotation);
            fl_line_style(miXStyle, 1);

            //
            // Loop through each X marker position
            //
            for(int iLoop = (w() - miXMarkerCount); iLoop > x();
                iLoop -= miXMarkerSpacing)
            {
                fl_line(iLoop + x(), y(), iLoop + x(), y() + h());
            }
        }

        if(muiAnnotationFlags & ANNOTATION_FLAG_SHOW_Y_MARKERS)
        {
            //
            // Set the drawing style and color we have been asked to use.  Note
            // that the order here is important.  According to the FLTK
            // documentation, Windows forgets the line style if you set the
            // color after the line style.
            //
            fl_color(mColorYAnnotation);
            fl_line_style(miYStyle, 1);

            //
            // Loop through each Y marker position
            //
            for(int iLoop = miMin - (miMin % miYMarkerSpacing);
                iLoop < miMax; iLoop += miYMarkerSpacing)
            {
                int iScaledY = PrivScaleY(iLoop);
                fl_line(x(), iScaledY, x() + w(), iScaledY);
            }
        }
    }

    //
    // How many samples do we have to draw?
    //
    iSamplesToDraw = mbFull ? miNumSamples : miWriteIndex;

    //
    // If we are drawing lines rather than points, subtract 1 from the number
    // of samples we are to draw.
    //
    if(muiAnnotationFlags & ANNOTATION_FLAG_LINE_CHART)
    {
        iSamplesToDraw--;
    }

    //
    // Now draw the data values on the chart.
    //
    for(int iChannel = 0; iChannel < miNumChannels; iChannel++)
    {
        int *piTable = mppiChannelDataTables[iChannel];

        //
        // Have we been told to display data from this channel?
        //
        if(!(miChannelVisibility & (1 << iChannel)))
        {
            //
            // No - skip to the next channel.
            //
            continue;
        }

        //
        // Set the solid line style and the desired channel color.
        //
        fl_line_style(0, 1);
        fl_color(mpColorChannels[iChannel]);

        //
        // Determine which sample we start drawing at.
        //
        iSample = (miWriteIndex == 0) ? (miNumSamples - 1) : (miWriteIndex - 1);

        //
        // Draw the trace for this channel.
        //
        for(int iX = 0; iX < iSamplesToDraw; iX++)
        {
            //
            // Is this a bogus value?  We use MAXINT to indicate that a value
            // is not valid and that we shouldn't draw it.
            //
            if(piTable[iSample] != INT_MAX)
            {
                //
                // This is a valid data point so plot it on the chart.
                //
                if(muiAnnotationFlags & ANNOTATION_FLAG_LINE_CHART)
                {
                    //
                    // We're drawing lines so check to see if the previous
                    // sample is valid.
                    //
                    if(piTable[PrivPrevious(iSample)] != INT_MAX)
                    {
                        //
                        // It is so draw a line from the current sample to the
                        // previous one.
                        //
                        fl_line(x() + w() - iX, PrivScaleY(piTable[iSample]),
                                x() + w() - (iX + 1),
                                PrivScaleY(piTable[PrivPrevious(iSample)]));
                    }
                }
                else
                {
                    //
                    // We're plotting points so just draw a dot to represent
                    // this sample.
                    //
                    fl_point(x() + w() - iX,
                         PrivScaleY(piTable[iSample]));
                }
            }
            iSample = PrivPrevious(iSample);
        }
    }

    //
    // If required, label the Y axis markers.
    //
    if(muiAnnotationFlags & ANNOTATION_FLAG_LABEL_Y_AXIS)
    {
        char pcBuffer[16];

        //
        // Set the font and color we have been asked to use.
        //
        fl_font(mYLabelFont, miYFontSize);
        fl_line_style(0, 1);

        //
        // Loop through each Y marker position.  Note that we deliberately don't
        // label the bottom marker since this will most likely be clipped.
        //
        for(int iLoop = (miMin + miYMarkerSpacing) - (miMin % miYMarkerSpacing);
            iLoop < miMax; iLoop += miYMarkerSpacing)
        {
            //
            // Format the label for this particular Y value.
            //
            int iScaledY = PrivScaleY(iLoop);
            sprintf(pcBuffer, "%.*f",
                    GetPrecision(),
                    ((float)iLoop / (float)pow(10.0, GetPrecision())));

            //
            // Now erase part of the Y axis that will fall under the label.  If
            // we don't do this, the text can be tricky to read.
            //
            fl_color(color());
            fl_line(x(), iScaledY, x() + (int)fl_width(pcBuffer) + 4, iScaledY);

            //
            // Now draw the marker text.
            //
            fl_color(mColorYAxisLabels);
            fl_draw(pcBuffer, x()+ 2, iScaledY - (fl_height() / 2), w() - 2,
                    fl_height(), FL_ALIGN_LEFT);
        }
    }

    //
    // If necessary, draw the label string.
    //
    if(labeltype() != FL_NO_LABEL)
    {
        //
        // We need to show the label so do it.  Align top left but shift it a
        // couple of pixels to the right to stop the label from touching the
        // edge of the widget.
        //
        fl_font(labelfont(), labelsize());
        fl_color(labelcolor());
        fl_draw(label(), x()+ 2, y(), w(), h(), FL_ALIGN_TOP_LEFT);
    }

    //
    // Reset the line style, color and clip region.
    //
    fl_pop_clip();
    fl_color(Color);
    fl_line_style(0, 1);
}

//*****************************************************************************
//
// Pass window events through to the base class.
//
//*****************************************************************************
int CStripChart::handle(int iEvent)
{
    return(Fl_Box::handle(iEvent));
}
