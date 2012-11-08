//*****************************************************************************
//
// renderer.h - Functions and globals related to rendering display elements in
//              the quickstart oscilloscope application.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#ifndef __RENDERER_H__
#define __RENDERER_H__

//*****************************************************************************
//
// Dimensions of the waveform display area in pixels.
//
//*****************************************************************************
#define WAVEFORM_WIDTH          120
#define WAVEFORM_HEIGHT         100

//*****************************************************************************
//
// Coordinates of each edge of the waveform area.  The area is centered left to
// right and positioned one pixel from the top of the display to allow for a
// thin border to be drawn around it.
//
//*****************************************************************************
#define WAVEFORM_LEFT           ((g_sFormike128x128x16.usWidth - \
                                  WAVEFORM_WIDTH) / 2)
#define WAVEFORM_RIGHT          (WAVEFORM_LEFT + WAVEFORM_WIDTH - 1)
#define WAVEFORM_TOP            1
#define WAVEFORM_BOTTOM         WAVEFORM_HEIGHT

//*****************************************************************************
//
// Colors for each display component.
//
//*****************************************************************************
#define WAVEFORM_NUM_COLORS     6

#define CHANNEL_1_COLOR         ClrYellow
#define CHANNEL_2_COLOR         ClrViolet
#define TRIG_POS_COLOR          ClrRed
#define TRIG_LEVEL_COLOR        ClrRed
#define GRATICULE_COLOR         ClrDarkGreen
#define BACKGROUND_COLOR        ClrBlack

//*****************************************************************************
//
// Dimensions of the alert message area in pixels.
//
//*****************************************************************************
#define ALERT_WIDTH             100
#define ALERT_HEIGHT            30
#define ALERT_LINE_HEIGHT       14

//*****************************************************************************
//
// Colors used to display alert messages.
//
//*****************************************************************************
#define ALERT_TEXT_COLOR        ClrWhite
#define ALERT_BACKGROUND_COLOR  ClrDarkBlue
#define ALERT_BORDER_COLOR      ClrWhite

//*****************************************************************************
//
// Definitions related to the image used to store the offscreen waveform
// buffer.
//
//*****************************************************************************
#define OFFSCREEN_BUF_SIZE      GrOffScreen4BPPSize(WAVEFORM_WIDTH,  \
                                                    WAVEFORM_HEIGHT)
extern unsigned char g_pucOffscreenImage[];
extern unsigned long g_pulPalette[];

//*****************************************************************************
//
// Graticule dimensions
//
//*****************************************************************************
#define GRATICULE_SIDE          10

//*****************************************************************************
//
// The effective vertical origin is set half way up the graticule grid.  This
// is the level which will represent 0 (differential) volts.  The calculation
// is performed as it is to ensure that the origin lies on a graticule line.
// If the vertical size of the waveform grid is not an even multiple of the
// graticule size, we would otherwise end up with an origin that didn't
// correspond to one of the lines (not that this need be a terrible thing, of
// course, since the user can move the waveforms up and down arbitrarily
// anyway).
//
//*****************************************************************************
#define GRATICULE_ORIGIN_Y      (((WAVEFORM_HEIGHT / GRATICULE_SIDE) / 2) * \
                                 GRATICULE_SIDE)

//*****************************************************************************
//
// Channel indices for use in populating the ulmVPerDivision and
// lVerticalOffsetmV fields of tRendererParams and in the pbChannels parameter
// to RendererDrawWaveform().
//
//*****************************************************************************
#define CHANNEL_1               0
#define CHANNEL_2               1

//*****************************************************************************
//
// Data type definitions.
//
//*****************************************************************************
typedef struct
{
    //
    // If \e true, the renderer will draw the graticule behind the waveforms,
    // otherwise the graticule is not displayed.
    //
    tBoolean bDrawGraticule;

    //
    // If \e true, the renderer will draw a horizontal line on the display to
    // represent the trigger level, otherwise this line is omitted.
    //
    tBoolean bDrawTrigLevel;

    //
    // If \e true, the renderer will draw a vertical line on the display to
    // represent the trigger position, otherwise this line is omitted.
    //
    tBoolean bDrawTrigPos;

    //
    // If \e true, captions indicating the current timebase and voltage
    // scaling are drawn above the waveform data, otherwise this information
    // is omitted.
    //
    tBoolean bShowCaptions;

    //
    // If \e true, waveform voltage measurements are drawn above the waveform
    // data, otherwise this information is omitted.
    //
    tBoolean bShowMeasurements;

    //
    // If \e true, a horizontal line representing the ground level is drawn
    // across the display in the same color as the waveform it relates to.
    //
    tBoolean bDrawGround;

    //
    // This field sets the vertical scaling to be used when rendering the
    // waveform.  It is expressed in millivolts per division.  Independent
    // scaling factors are provided for each channel.
    //
    unsigned long ulmVPerDivision[2];

    //
    // This field sets the horizontal scaling to be used when rendering the
    // waveform.  It is expressed in microseconds per division.
    //
    unsigned long uluSPerDivision;

    //
    // This field represents the vertical offsets for the each of the
    // two channels.  The array elements are signed numbers representing the
    // number of millivolts to offset the display.  Higher values move the
    // waveform towards the top of the display.
    //
    long lVerticalOffsetmV[2];

    //
    // This field represents the horizontal offset for the waveform displays.
    // It is a signed number representing the number of pixels to offset
    // the waveform.  Higher values cause later (rightmost) sections of the
    // capture sample data set to be displayed.  Note that there is only one
    // horizontal offset adjustment since it is assumed that time correlation
    // of the two oscilloscope channels is important and therefore, both
    // waveforms are offset by the same amount.
    //
    long lHorizontalOffset;

    //
    // This field contains the trigger level for the waveforms to be rendered
    // expressed in millivolts.
    //
    long lTriggerLevelmV;
}
tRendererParams;

typedef struct
{
    //
    // The maximum voltage detected in the rendered waveform.
    //
    long lMaxmV;

    //
    // The minimum voltage detected in the rendered waveform.
    //
    long lMinmV;

    //
    // The mean voltage detected in the rendered waveform.
    //
    long lMeanmV;
}
tWaveformInfo;

typedef struct
{
    //
    // Array of two waveform information structures, one for each channel.
    //
    tWaveformInfo sInfo[2];
}
tRendererMeasurements;

//*****************************************************************************
//
// Graphics context definition.
//
//*****************************************************************************
extern tContext g_sContext;
extern tRendererParams g_sRender;
extern tRendererMeasurements g_sMeasure;

//*****************************************************************************
//
// Rectangles defining the waveform display area and the display as a whole.
//
//*****************************************************************************
extern tRectangle g_sRectWaveform;
extern tRectangle g_sRectDisplay;

//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************
extern void RendererInit(void);
extern void RendererDrawWaveform(tDataAcqCaptureStatus *pCapData,
                                 tRendererParams *pRender,
                                 tBoolean *pbChannels,
                                 tRendererMeasurements *psMeasure);
extern void RendererDrawGraticule(void);
extern void RendererUpdate(void);
extern void RendererUpdateAlert(void);
extern void RendererFillRect(tRectangle *psFillRect, unsigned long ulLevel);
extern void RendererSetCenterOffset(unsigned long ulSampleIndex,
                                    unsigned long ulSamplePerioduS,
                                    tRendererParams *pRender);
extern int RendererFormatDisplayString(char *pcBuffer, int iBufLen,
                                       const char *pcSuffix,
                                       const char *pcUnit,
                                       const char *pcUnit1000, long lValue);
extern void RendererSetAlert(char *pcString, unsigned long ulTicks);
extern void RendererClearAlert(void);
extern void RendererShowStartupScreen(void);
extern void RendererDrawHelpScreen(tBoolean bShow);

#endif // __RENDERER_H__
