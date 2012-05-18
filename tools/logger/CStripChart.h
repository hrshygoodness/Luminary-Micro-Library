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
#ifndef _CSTRIPCHART_H_
#define _CSTRIPCHART_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.h>

//*****************************************************************************
//
// Flags indicating various display annotations to show on the chart.  Note that
// the underlying Fl_Box settings are used for the label text, font, color and
// visibility so these need to be set appropriately too.  The label will be
// displayed unless the NO_LABEL style is set.  For all other cases, the label
// is positioned in the top left of the widget.
//
// A style flag is also included here to decide whether to use points or lines
// when drawing the chart.  If this is not selected, each channel trace is
// drawn by plotting single pixel points to represent each sample.  With the
// flag set, a straight line is drawn between consecutive data points.
//
//*****************************************************************************
#define ANNOTATION_FLAG_SHOW_X_MARKERS 0x00000001
#define ANNOTATION_FLAG_SHOW_Y_MARKERS 0x00000002
#define ANNOTATION_FLAG_LABEL_Y_AXIS   0x00000004
#define ANNOTATION_FLAG_LINE_CHART     0x80000000

//*****************************************************************************
//
// The class description for a CStripChart object.
//
//*****************************************************************************
class CStripChart : public Fl_Box
{
public:
    //
    // Constructor and destructor.
    //
    CStripChart(int iX, int iY, int iW, int iH, const char *pcLabel = 0);
    ~CStripChart(void);

    //
    // The object draw handler.  This is required since we are subclassing
    // Fl_Widget which declares this as a pure virtual function.
    //
    void draw(void);

    //
    // Window event handler.
    //
    int handle(int iEvent);

    //
    // Set the number of data channels that the strip chart should display.
    // By default, a newly created strip chart displays 1 channel but this
    // function may be called to increase that number up to a maximum of 32.
    //
    void SetNumChannels(int iNumChannels);

    //
    // Return the number of channels that the strip chart is configured to
    // display.
    //
    int GetNumChannels(void);

    //
    // Set the Y range represented by the control.  The X range is defined
    // purely by the width of the control since we move the chart 1 pixel left
    // for each new data value added.
    //
    void SetRange(int iMin, int iMax);

    //
    // Query the current Y range settings for the control.
    //
    void GetRange(int *piMin, int *piMax);

    //
    // Set the scaling for any Y axis annotations printed.  Although the data
    // values provided are integers, this number represents a power of 10 to
    // divide the value by when displaying labels.  Setting a precision of 3,
    // for example, divides the integer value by 1000 and displays it with 3
    // 3 digits after the decimal point on Y axis labels.
    //
    void SetPrecision(int iPrecision);

    //
    // Query the precision setting for the control.
    //
    int GetPrecision(void);

    //
    // Enable or disable various annotations on the chart.
    //
    void SetAnnotation(unsigned int uiAnnotationFlags, int iXMarkerSpacing,
                      int iYMarkerSpacing);

    //
    // Set the colors of the annotations on the chart.
    //
    void SetAnnotationColors(Fl_Color XColor, Fl_Color YColor,
                             Fl_Color YLabelColor = FL_DARK_YELLOW);

    //
    // Set the line style to use for X and Y axis markings.  Valid values are
    // as for fl_line_style().
    //
    void SetAxisStyle(int iXStyle, int iYStyle);

    //
    // Set the font and character size used for Y axis labels if
    // ANNOTATION_FLAG_LABEL_Y_AXIS has been set.
    //
    void SetAxisFont(Fl_Font AxisFont, int iSize = 10);

    //
    // Set the colors used for each of the data channel traces on the chart.
    //
    void SetChannelColors(Fl_Color *pColors, int iNumColors = 1);

    //
    // Add a new set of data samples to the chart.  The mask indicates which
    // channels have valid data and allows an application to update a subset of
    // the channels if necessary.
    //
    void AddData(int *piValues, int iNumValues = 1,
                 unsigned long ulMask = 0xFFFFFFFF);

    //
    // Clear all stored chart data and repaint the widget.
    //
    void Clear(void);

    //
    // Enable or disable channels displayed by the strip chart.  By default all
    // channels are enabled.
    //
    void SetChannelVisibility(unsigned int uiMask);

    //
    // Query which channels are currently visible.  The return value is the
    // mask last passed to SetChannelVisibility().
    //
    unsigned int GetChannelVisibility(void);

private:
    //
    // The number of channels supported by this class.
    //
    int miNumChannels;

    //
    // The number of data samples stored for each channel.
    //
    int miNumSamples;

    //
    // A bitmask indicating which channels are being displayed.  A 1 in a bit
    // position enables display of data on that channel.
    //
    unsigned int miChannelVisibility;

    //
    // A pointer to a table if miNumChannels integer pointers each pointing to
    // the data buffer for one channel.
    //
    int **mppiChannelDataTables;

    //
    // The index of the next available slot in the channel data buffers.  It is
    // assumed that all channels are updated simultaneously so we only need a
    // single write pointer.
    //
    int miWriteIndex;

    //
    // A flag to indicate whether the data buffers are full or not.
    //
    bool mbFull;

    //
    // Value controlling scaling of the display in the Y direction.
    //
    int miMin;
    int miMax;

    //
    // Value controlling the number of digits to display after the decimal
    // point when draing Y axis labels.  This is a power of 10 and indicates
    // how the integer range is divided before labels are drawn.  For example,
    // if the range is from 0 - 100, a precision of 2 will display Y axis
    // labels between 0.00 and 1.00.
    //
    int miPrecision;

    //
    // Line style for the X and Y axis markings.
    //
    int miXStyle;
    int miYStyle;

    //
    // The colors of various display elements in the widget.
    //
    Fl_Color mColorXAnnotation;
    Fl_Color mColorYAnnotation;
    Fl_Color mColorYAxisLabels;

    //
    // The font to be used when drawing the Y axis value labels.
    //
    Fl_Font mYLabelFont;

    //
    // The size, in points, of the font to be used to draw the Y axis value
    // labels.
    //
    int miYFontSize;

    //
    // A pointer to an array of miNumChannels unsigned integers containing the
    // colors to be used when displaying data from each channel.
    //
    Fl_Color *mpColorChannels;

    //
    // Flags indicating which display annotations to enable.
    //
    unsigned int muiAnnotationFlags;

    //
    // Pixel spacing of axis marker in the X direction.  The X marker is a
    // vertical line drawn in the control to aid in seeing movement as the
    // chart scrolls.
    //
    int miXMarkerSpacing;

    //
    // A count of pixels drawn since the last X marker was drawn.
    //
    int miXMarkerCount;

    //
    // Spacing of the Y axis markers. This uses the same scale as the max and
    // min values specified via SetRange();
    //
    int miYMarkerSpacing;

    //
    // Discard the internal channel data and color tables.
    //
    void PrivDiscardDataTables();

    //
    // Scale and translate a sample into the range we've been given.
    //
    int PrivScaleY(int iY);

    //
    // Get the index of the following sample taking into account buffer wrap.
    //
    int PrivPrevious(int iIndex);
};

#endif // _CSTRIPCHART_H_
