//*****************************************************************************
//
// renderer.c - Functions related to graphics and display handling for the
//              Quickstart Oscilloscope application.
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

#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "drivers/formike128x128x16.h"
#include "data-acq.h"
#include "images.h"
#include "qs-scope.h"
#include "renderer.h"

//*****************************************************************************
//
// Graphics library context information.
//
//*****************************************************************************
tContext g_sContext;
tContext g_sOffscreenContext;
tDisplay g_sOffscreenDisplay;

//*****************************************************************************
//
// Time in SysTicks to wait with startup logos displayed before continuing.
//
//*****************************************************************************
#define LOGO_DISPLAY_DELAY      200

//*****************************************************************************
//
// Graphics library context information.
//
//*****************************************************************************
unsigned char g_pucOffscreenImage[OFFSCREEN_BUF_SIZE];

//*****************************************************************************
//
// The color palette used to render the waveform into the offscreen 4bpp
// buffer.
//
//*****************************************************************************
unsigned long g_pulPalette[WAVEFORM_NUM_COLORS] =
{
    CHANNEL_1_COLOR,
    CHANNEL_2_COLOR,
    TRIG_POS_COLOR,
    TRIG_LEVEL_COLOR,
    GRATICULE_COLOR,
    BACKGROUND_COLOR
};

//*****************************************************************************
//
// Waveform rendering parameters
//
//*****************************************************************************
tRendererParams g_sRender =
{
    true,                                   // bDrawGraticule
    true,                                   // bDrawTrigLevel
    true,                                   // bDrawTrigPos
    true,                                   // bShowCaptions
    true,                                   // bShowMeasurements
    true,                                   // bDrawGround
    {DEFAULT_SCALE_MV, DEFAULT_SCALE_MV},   // ulmVPerDivision
    DEFAULT_TIMEBASE_US,                    // uluSPerDivision
    {0, 0},                                 // lVerticalOffsetmV
    0,                                      // lHorizontalOffset
    DEFAULT_TRIGGER_LEVEL_MV                // lTriggerLevelmV
};

//*****************************************************************************
//
// Max, min and mean voltages measured while rendering the last captured
// waveform.
//
//*****************************************************************************
tRendererMeasurements g_sMeasure;

//*****************************************************************************
//
// The bounding rectangle for the waveform display area, the offscreen image
// used to render the waveform and the display as a whole.
//
//*****************************************************************************
tRectangle g_sRectWaveform;
tRectangle g_sRectWaveformOffscreen;
tRectangle g_sRectAlert;
tRectangle g_sRectDisplay;

//*****************************************************************************
//
// The horizontal offset used to center the waveform on the display.
//
//*****************************************************************************
long g_lCenterOffset;

//*****************************************************************************
//
// Variables related to alert message display.
//
//*****************************************************************************
char *g_pcAlertMessage = (char *)0;
unsigned long g_ulAlertCount = 0;
unsigned long g_ulAlertStart = 0;

//*****************************************************************************
//
// Display coordinates for various elements of the help screen.
//
//*****************************************************************************
#define HELP_TOP                16
#define HELP_LEFT               4
#define HELP_LINE_HEIGHT        10
#define HELP_LINE_Y(num)        (HELP_TOP + ((num) * HELP_LINE_HEIGHT))

#define LINE_LEFT_START         40
#define PIN_START_X             63
#define PIN_WIDTH               9
#define PIN_POS_X(num)          (PIN_START_X +((num) * PIN_WIDTH))

//*****************************************************************************
//
// Text shown on the help screen.  The first character of each string is
// used as a label for the relevant pin.
//
//*****************************************************************************
const char *g_pcHelpText[] =
{
    "1. Speaker",
    "2. Channel1+",
    "3. Channel1-",
    "4. Ground",
    "5. 1KHz Tone",
    "6. Channel2+",
    "7. Channel2-",
    "8. Ground"
};

#define NUM_HELP_LINES          (sizeof(g_pcHelpText) / sizeof(char *))

//*****************************************************************************
//
// Convert between millivolts and a y pixel coordinate given a voltage scaling
// factor.
//
//*****************************************************************************
#define MV_TO_Y(mv, scale)      (GRATICULE_ORIGIN_Y -                       \
                                 (((mv) * GRATICULE_SIDE) / (long)(scale)))

//*****************************************************************************
//
// Buffer used when rendering captions over the oscilloscope waveform.
//
//*****************************************************************************
#define CAPTION_BUFFER_SIZE     32
char g_pcCaptionBuffer[CAPTION_BUFFER_SIZE];

//*****************************************************************************
//
// The number of pixels between adjacent caption lines on the waveform
// display.
//
//*****************************************************************************
#define CAPTION_LINE_SPACING    10

//*****************************************************************************
//
// Convert a sample index in the current capture buffer into a horizontal (x)
// pixel coordinate on the display.  The sample index passed is the sample
// number counting from the start sample and NOT the index into the capture
// buffer itself.  In other words, ulSampleIndex == 0 is the earliest sample.
//
//*****************************************************************************
static long
SampleIndexToX(unsigned long ulSampleIndex, unsigned long ulSamplePerioduS,
               unsigned long ulSampleOffsetuS, tRendererParams *pRender)
{
    long lX;

    //
    // First determine the number of microseconds from the start of the capture
    // to this sample.
    //
    // ulSampleIndex <= 1024 (typically 512)
    // ulSamplePerioduS <= 10000 (min rate of 10mS per sample or 100Hz)
    // ulSampleOffsetuS <= 8
    //
    // so lX <= 10240008, hence no overflow in this calculation.
    //
    lX = (ulSampleIndex * ulSamplePerioduS) + ulSampleOffsetuS;

    //
    // Now convert from microseconds to pixels given the selected timebase.  At
    // this point, we have a distance in pixels from the earliest sample.
    //
    lX = (lX * GRATICULE_SIDE)/pRender->uluSPerDivision;

    //
    // Adjust the pixel position for the current display window.  We center the
    // window on the full trace then apply an offset to allow the user to pan
    // the window across the captured waveform (effectively changing the
    // trigger position from their point of view).
    //
    lX = lX - g_lCenterOffset + pRender->lHorizontalOffset;

    //
    // All done.  Return the result to the caller.
    //
    return(lX);
}

//*****************************************************************************
//
// Convert an ADC sample to a Y coordinate on the display taking into account
// all display parameters in the passed rendering structure.
//
//*****************************************************************************
static long
SampleToY(unsigned short usSample, tRendererParams *pRender, int iChannel)
{
    long lMillivolts;
    long lY;

    //
    // Convert the ADC sample value into a number of millivolts
    //
    lMillivolts = ADC_SAMPLE_TO_MV(usSample);

    //
    // Adjust this with the value supplied in the rendering structure.
    //
    lMillivolts += pRender->lVerticalOffsetmV[iChannel];

    //
    // Calculate the actual y coordinate representing this voltage given the
    // voltage scaling supplied in the rendering structure.
    //
    lY = MV_TO_Y(lMillivolts, pRender->ulmVPerDivision[iChannel]);

    return(lY);
}

//*****************************************************************************
//
// Draw the waveform for a single set of captured samples.
//
//*****************************************************************************
static void
DrawSingleWaveform(tDataAcqCaptureStatus *pCapData, tRendererParams *pRender,
                   int iChannel, unsigned long ulColor, tWaveformInfo *psInfo)
{
    unsigned long ulLoop;
    unsigned long ulSample;
    unsigned long ulIncrement;
    unsigned long ulOffset;
    long lX;
    long lY;
    long lLastX;
    long lLastY;
    long lAccumulator;
    unsigned short usMin;
    unsigned short usMax;

    //
    // Initialize the variables used to measure the min, max and average
    // voltages in the waveform.
    //
    usMin = (1 << ADC_NUM_BITS);
    usMax = 0;
    lAccumulator = 0;

    //
    // Check for cases where we are asked to render a waveform for channel 2
    // but have not been passed dual channel data.
    //
    if((iChannel == CHANNEL_2) && !pCapData->bDualMode)
    {
        //
        // Ignore the request since we can't render what we don't have.
        //
        return;
    }

    //
    // Set the sample increment is depending upon the type of data set we have
    // been passed.
    //
    ulIncrement = (pCapData->bDualMode) ? 2 : 1;

    //
    // Set the drawing color.
    //
    GrContextForegroundSet(&g_sOffscreenContext, ulColor);

    //
    // If required, draw the dotted ground line
    //
    if(pRender->bDrawGround)
    {
        lY = SampleToY(MV_TO_ADC_SAMPLE(0), pRender, iChannel);
        for(lX = 0; lX < WAVEFORM_WIDTH - 1; lX += 2)
        {
            GrPixelDraw(&g_sOffscreenContext, lX, lY);
        }
    }

    //
    // Get the index of the first sample value in the capture buffer for this
    // channel.  Note that we have to be careful to take into account the
    // sample order.  Since we're doing this comparison anyway, also determine
    // whether this channel's samples need to have a time offset applied.
    //
    if(pCapData->bBSampleFirst)
    {
        //
        // The reversed case - the B (channel 2) sample is first in the
        // buffer
        //
        ulSample = pCapData->ulStartIndex + (1 - iChannel);
        ulOffset = (iChannel == CHANNEL_2) ? 0 : pCapData->ulSampleOffsetuS;
    }
    else
    {
        //
        // The normal case - the A (channel 1) sample is first in the
        // buffer.
        //
        ulSample = pCapData->ulStartIndex + iChannel;
        ulOffset = (iChannel == CHANNEL_1) ? 0 : pCapData->ulSampleOffsetuS;
    }

    //
    // We need to initialize lLastX and lLastY purely to keep the compiler
    // happy...
    //
    lLastX = 0;
    lLastY = 0;

    //
    // Loop through each of the captured samples to draw the waveform onto
    // the display.
    //
    for(ulLoop = 0; ulLoop < pCapData->ulSamplesCaptured; ulLoop++)
    {
        //
        // Get the screen X coordinate for this sample number given the
        // renderer timebase and offset.
        //
        lX = SampleIndexToX(ulLoop, pCapData->ulSamplePerioduS, ulOffset,
                            pRender);

        //
        // Get the screen Y coordinate for this sample given the renderer
        // voltage scaling factor and offset.
        //
        lY = SampleToY(pCapData->pusBuffer[ulSample], pRender, iChannel);

        //
        // Update our measurements.  Note that we conver to mV as we
        // accumulate since, if we don't do this, the mean we calculate
        // here differs from the mean we calculate on the PC side (since we
        // send converted mV values rather than ADC samples in the USB
        // packets).
        //
        lAccumulator += ADC_SAMPLE_TO_MV(pCapData->pusBuffer[ulSample]);

        if(pCapData->pusBuffer[ulSample] > usMax)
        {
            usMax = pCapData->pusBuffer[ulSample];
        }

        if(pCapData->pusBuffer[ulSample] < usMin)
        {
            usMin = pCapData->pusBuffer[ulSample];
        }

        //
        // If this is not the first pixel, draw a line segment
        //
        if(ulLoop)
        {
            //
            // Draw one line segment.
            //
            GrLineDraw(&g_sOffscreenContext, lLastX, lLastY, lX, lY);
        }

        //
        // Remember the current point for use next time round the loop.
        //
        lLastX = lX;
        lLastY = lY;

        //
        // Move on to the next sample, handling the buffer wrap if necessary.
        //
        ulSample += ulIncrement;

        if(ulSample >= pCapData->ulMaxSamples)
        {
            ulSample -= pCapData->ulMaxSamples;
        }
    }

    //
    // Update the waveform voltage information that is to be passed back to the
    // client if any samples were actually captured.
    //
    if(pCapData->ulSamplesCaptured)
    {
        psInfo->lMaxmV = ADC_SAMPLE_TO_MV(usMax);
        psInfo->lMinmV = ADC_SAMPLE_TO_MV(usMin);
        psInfo->lMeanmV = lAccumulator / (long)pCapData->ulSamplesCaptured;
    }
}

//*****************************************************************************
//
// Initialize the graphics functions and display hardware.
//
//*****************************************************************************
void
RendererInit(void)
{
    //
    // Initialize the display driver.
    //
    Formike128x128x16Init();

    //
    // Turn on the backlight.
    //
    Formike128x128x16BacklightOn();

    //
    // Initialize the display graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike128x128x16);

    //
    // Set the font we will be using.
    //
    GrContextFontSet(&g_sContext, g_pFontCmss12);

    //
    // Set up the context for the offscreen rendering of the waveform.
    //
    GrOffScreen4BPPInit(&g_sOffscreenDisplay, g_pucOffscreenImage,
                        WAVEFORM_WIDTH, WAVEFORM_HEIGHT);

    //
    // Set the color palette we will use for the offscreen surface.
    //
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplay, g_pulPalette, 0,
                              WAVEFORM_NUM_COLORS);

    //
    // Initialize the offscreen graphics context.
    //
    GrContextInit(&g_sOffscreenContext, &g_sOffscreenDisplay);

    //
    // Set up our waveform area and display bounding rectangles.
    //
    g_sRectWaveform.sXMin = WAVEFORM_LEFT;
    g_sRectWaveform.sYMin = WAVEFORM_TOP;
    g_sRectWaveform.sXMax = WAVEFORM_RIGHT;
    g_sRectWaveform.sYMax = WAVEFORM_BOTTOM;

    g_sRectWaveformOffscreen.sXMin = 0;
    g_sRectWaveformOffscreen.sYMin = 0;
    g_sRectWaveformOffscreen.sXMax = WAVEFORM_WIDTH - 1;
    g_sRectWaveformOffscreen.sYMax = WAVEFORM_HEIGHT - 1;

    g_sRectDisplay.sXMin = 0;
    g_sRectDisplay.sYMin = 0;
    g_sRectDisplay.sXMax = DpyWidthGet(&g_sFormike128x128x16) - 1;
    g_sRectDisplay.sYMax = DpyHeightGet(&g_sFormike128x128x16) - 1;

    //
    // Determine where the alert display is.
    //
    g_sRectAlert.sXMin = g_sRectWaveform.sXMin +
                         (WAVEFORM_WIDTH - ALERT_WIDTH) / 2;
    g_sRectAlert.sYMin = g_sRectWaveform.sYMin +
                         (WAVEFORM_HEIGHT - ALERT_HEIGHT) / 2;
    g_sRectAlert.sXMax = g_sRectAlert.sXMin + ALERT_WIDTH;
    g_sRectAlert.sYMax = g_sRectAlert.sYMin + ALERT_HEIGHT;

    //
    // Set the offscreen context clipping region to the whole area.
    //
    GrContextClipRegionSet(&g_sOffscreenContext, &g_sRectWaveformOffscreen);
}

//*****************************************************************************
//
// Calculates the horizontal pixel offset required to center the waveform
// trace on the display.
//
// \param ulSampleIndex is the index of the center sample in the buffer
// holding the data which will ultimately be rendered to the display.
// \param ulSamplePerioduS is the number of microseconds between adjacent
// samples for the same channel.
// \param pRender points to a structure containing rendering parameters.
//
// This function is called by the client whenever the timebase changes.  It
// calculates the correct offset required to position the rendered waveform
// such that the supplied sample index appears at the horizontal center of
// the display.  Typically, ulSampleIndex will represent half the total number
// of samples in the capture buffer.
//
// \return None.
//
//*****************************************************************************
static long
GetCenterOffset(tDataAcqCaptureStatus *pCapData, tRendererParams *pRender)
{
    long lX;
    long lCenterIndex;

    //
    // First determine where the center sample is.  If this is dual channel
    // data, the number of sample periods is effectively half what it would
    // be if we were dealing with single channel data.
    //
    lCenterIndex = pCapData->ulMaxSamples / (pCapData->bDualMode ? 4 : 2);

    //
    // Now determine the total number of microseconds from the start of the
    // buffer to the center sample.
    //
    lX = lCenterIndex * pCapData->ulSamplePerioduS;

    //
    // Convert from microseconds to pixels given the selected timebase.  At
    // this point, we have a distance in pixels from the earliest sample.
    //
    lX = (lX * GRATICULE_SIDE)/pRender->uluSPerDivision;

    //
    // The correction we apply to the x coordinates is the center offset minus
    // half the width of the display.
    //
    return(lX - (WAVEFORM_WIDTH / 2));
}

//*****************************************************************************
//
// Formats an ASCII string containing a number, its units and a suffix string.
//
// \param pcBuffer points to the buffer into which the string is to be
// formatted.
// \param iBufLen contains the number of bytes of storage pointed to by
// \e pcBuffer.
// \param pcSuffix points to a string that is to be appended to the end of
// the formatted number and units.
// \param pcUnit points to a string describing the unit of measurement of
// the number as passed in \e lValue.
// \param pcUnit1000 points to a string describing the unit of measurement of
// the (\e lValue / 1000)/
// \param lValue is the number which is to be formatted into the buffer.
//
// This function is called to generate strings suitable for display when the
// number to be rendered may take on a wide range of values.  It considers the
// size of \e lValue and, if necessary, divides by 1000 and formats it with
// as few digits after the decimal point as necessary (to remove trailing 0s).
// For example, passing lValue 5300, pcSuffix "/div", pcUnit "mV" and
// pcUnit1000 "V" would return formatted string "5.3V/div".  Reducing lValue
// to 800 would result in "800mV/div".
//
// \return Returns the number of characters rendered into \e pcBuffer.
//
//*****************************************************************************
int
RendererFormatDisplayString(char *pcBuffer, int iBufLen, const char *pcSuffix,
                            const char *pcUnit, const char *pcUnit1000,
                            long lValue)
{
    int iLen;

    if(abs(lValue) >= 1000)
    {
        //
        // The value is greater than or equal to 1000 so we will divide down
        // and show it in decimal format.
        //

        //
        // Check for trailing zeros and format as appropriate.
        //
        if((lValue % 1000) == 0)
        {
            //
            // Multiple of 1000 - no decimal point or fractional digits needed.
            //
            iLen = usnprintf(pcBuffer, iBufLen, "%d%s%s", (lValue / 1000),
                             pcUnit1000, pcSuffix);
        }
        else if((lValue % 100) == 0)
        {
            //
            // Multiple of 100 - 1 decimal place needed.
            //
            iLen = usnprintf(pcBuffer, iBufLen, "%d.%d%s%s", (lValue / 1000),
                             abs((lValue % 1000) / 100), pcUnit1000, pcSuffix);
        }
        else if((lValue % 10) == 0)
        {
            //
            // Multiple of 10 - 2 decimal place needed.
            //
            iLen = usnprintf(pcBuffer, iBufLen, "%d.%02d%s%s", (lValue / 1000),
                             abs((lValue % 1000) / 10), pcUnit1000, pcSuffix);
        }
        else
        {
            //
            // 3 decimal place needed.
            //
            iLen = usnprintf(pcBuffer, iBufLen, "%d.%03d%s%s", (lValue / 1000),
                             abs(lValue % 1000), pcUnit1000, pcSuffix);
        }
    }
    else
    {
        //
        // The value passed is less than 1000 so we just display it as it is.
        //
        iLen = usnprintf(pcBuffer, iBufLen, "%d%s%s", lValue, pcUnit,
                         pcSuffix);
    }

    return(iLen);
}

static void
RendererShowMeasurements(tWaveformInfo *psInfo, tBoolean bChannel1)
{
    long lX;
    long lY;
    int iLen;

    //
    // Determine the starting position based on the channel whose information
    // is being drawn.
    //
    lY = bChannel1 ? 0 : (WAVEFORM_HEIGHT - (3 * CAPTION_LINE_SPACING));

    //
    // Set the appropriate color depending upon the channel
    //
    GrContextForegroundSet(&g_sOffscreenContext,
                           bChannel1 ? CHANNEL_1_COLOR : CHANNEL_2_COLOR);

    //
    // Format a string containing the minimum value.
    //
    iLen = RendererFormatDisplayString(g_pcCaptionBuffer,
                               CAPTION_BUFFER_SIZE, "", "mV min" , "V min",
                               psInfo->lMinmV);

    //
    // How wide is the string (in pixels)?
    //
    lX = GrStringWidthGet(&g_sOffscreenContext, g_pcCaptionBuffer,
                          (unsigned long)iLen);

    //
    // Draw the string on the right side of the screen.
    //
    GrStringDraw(&g_sOffscreenContext, g_pcCaptionBuffer,(unsigned long)iLen,
                 WAVEFORM_WIDTH - lX, lY, false);

    //
    // Format a string containing the minimum value.
    //
    iLen = RendererFormatDisplayString(g_pcCaptionBuffer,
                               CAPTION_BUFFER_SIZE, "", "mV max" , "V max",
                               psInfo->lMaxmV);

    //
    // How wide is the string (in pixels)?
    //
    lX = GrStringWidthGet(&g_sOffscreenContext, g_pcCaptionBuffer,
                          (unsigned long)iLen);

    //
    // Draw the string on the right side of the screen.
    //
    GrStringDraw(&g_sOffscreenContext, g_pcCaptionBuffer,(unsigned long)iLen,
                 WAVEFORM_WIDTH - lX, lY + CAPTION_LINE_SPACING, false);

    //
    // Format a string containing the mean voltage.
    //
    iLen = RendererFormatDisplayString(g_pcCaptionBuffer,
                               CAPTION_BUFFER_SIZE, " avg", "mV" , "V",
                               psInfo->lMeanmV);

    //
    // How wide is the string (in pixels)?
    //
    lX = GrStringWidthGet(&g_sOffscreenContext, g_pcCaptionBuffer,
                          (unsigned long)iLen);

    //
    // Draw the string at the top right of the screen below the previous
    // string.
    //
    GrStringDraw(&g_sOffscreenContext, g_pcCaptionBuffer,(unsigned long)iLen,
                 WAVEFORM_WIDTH - lX,
                 lY + (2 * CAPTION_LINE_SPACING), false);
}

//*****************************************************************************
//
// Renders one or both waveforms from data referenced by the pCapData structure
// passed.
//
// \param pCapData is the capture status information as returned by a call
// to DataAcquisitionGetStatus().  This structure contains a pointer to the
// sample data and information related to number of samples and trigger
// position.
// \param pRender points to a structure containing parameters controlling the
// rendering of the waveform display.
// \param pbChannels points to an array of 2 tBoolean values indicating
// which of the two channels to render.  pbChannels[0] indicates whether or not
// to render the waveform from channel 1 and pbChannels[1] controls rendering
// for channel 2.
// \param psMeasure points to a structure which will be written with
// information on the waveforms rendered.  This information includes maximum
// voltage, minimum voltage and mean voltage.
//
// This function is called to redraw the waveform area of the display.
// Parameters passed provide access to the waveform sample data and display
// scaling and positioning choices.
//
// The sample data will contain data for either one or two channels.  In cases
// where the data contains only one channel samples and this function is
// called requesting redraw of both channels, no error is generated and no
// waveform displayed for channel 2.
//
// \note The caller is responsible for calling RendererUpdate() after this
// call returns.  This allows an application to perform additional display
// rendering prior to flushing the information to a frame-buffer based
// display device.
//
// \return None.
//
//*****************************************************************************
void
RendererDrawWaveform(tDataAcqCaptureStatus *pCapData, tRendererParams *pRender,
                     tBoolean *pbChannels, tRendererMeasurements *psMeasure)
{
    long lX;
    long lY;
    int iLen;
    int iChannel;

    //
    // Set the font we will use for captions.
    //
    GrContextFontSet(&g_sOffscreenContext, g_pFontFixed6x8);

    //
    // Clear the offscreen display.
    //
    RendererFillRect(&g_sRectWaveformOffscreen, BACKGROUND_COLOR);

    //
    // Draw the graticule
    //
    if(pRender->bDrawGraticule)
    {
        RendererDrawGraticule();
    }

    //
    // Determine the horizontal offset to apply to the waveform to ensure
    // that the trigger position is where we expect it to be (centered by
    // default).
    //
    g_lCenterOffset = GetCenterOffset(pCapData, pRender);

    //
    // Draw a vertical line at the trigger point.
    //
    if(pRender->bDrawTrigPos)
    {
        //
        // Determine where on the display the trigger point falls
        //
        lY = DISTANCE_FROM_START(pCapData->ulStartIndex,
                                 pCapData->ulTriggerIndex,
                                 pCapData->ulMaxSamples);
        //
        // Correct for dual channel case where we have 2 samples in the buffer
        // for every sample period.
        //
        if(pCapData->bDualMode)
        {
            lY /= 2;
        }

        //
        // Now determine where we need to draw the trigger position line.
        // Since we always trigger on the first sampel in the buffer, the
        // sample offset must be 0.
        //
        lX = SampleIndexToX(lY, pCapData->ulSamplePerioduS, 0, pRender);

        //
        // If the trigger position is visible in the waveform area of the
        // screen, draw a vertical line there.
        //
        if((lX >= 0) && (lX < WAVEFORM_WIDTH))
        {
            GrContextForegroundSet(&g_sOffscreenContext, TRIG_POS_COLOR);
            GrLineDraw(&g_sOffscreenContext, lX, 0, lX, WAVEFORM_HEIGHT - 1);
        }
    }

    //
    // ...and a horizontal line at the trigger level.
    //
    if(pRender->bDrawTrigLevel)
    {
        //
        // Which channel are we triggering on?
        //
        iChannel = pCapData->bBSampleFirst ? CHANNEL_2 : CHANNEL_1;

        lY = MV_TO_Y((pRender->lTriggerLevelmV +
                      pRender->lVerticalOffsetmV[iChannel]),
                     pRender->ulmVPerDivision[iChannel]);
        if((lY >= WAVEFORM_TOP) && (lY <= WAVEFORM_BOTTOM))
        {
            GrContextForegroundSet(&g_sOffscreenContext, TRIG_LEVEL_COLOR);
            GrLineDraw(&g_sOffscreenContext, 0, lY, WAVEFORM_WIDTH - 1, lY);
        }
    }

    //
    // Have we been asked to render the channel 1 waveform?
    //
    if(pbChannels[CHANNEL_1])
    {
        //
        // Yes - render the waveform for channel 1
        //
        DrawSingleWaveform(pCapData, pRender, CHANNEL_1, CHANNEL_1_COLOR,
                           &(psMeasure->sInfo[CHANNEL_1]));
    }

    //
    // Have we been asked to render the channel 2 waveform?
    //
    if(pbChannels[CHANNEL_2])
    {
        //
        // Yes - render the waveform for channel 2
        //
        DrawSingleWaveform(pCapData, pRender, CHANNEL_2, CHANNEL_2_COLOR,
                           &(psMeasure->sInfo[CHANNEL_2]));
    }

    //
    // Render caption information if requested
    //
    if(pRender->bShowCaptions)
    {
        //
        // Are we showing the channel 1 waveform?
        //
        if(pbChannels[CHANNEL_1])
        {
            //
            // Yes - display channel 1 captions.  First the voltage scale
            //
            GrContextForegroundSet(&g_sOffscreenContext, CHANNEL_1_COLOR);
            iLen = RendererFormatDisplayString(g_pcCaptionBuffer,
                                   CAPTION_BUFFER_SIZE, "/div", "mV" , "V",
                                   (long)g_sRender.ulmVPerDivision[CHANNEL_1]);
            GrStringDraw(&g_sOffscreenContext, g_pcCaptionBuffer,
                         (unsigned long)iLen, 0, 0, false);

            //
            // Next the timebase
            //
            iLen = RendererFormatDisplayString(g_pcCaptionBuffer,
                                    CAPTION_BUFFER_SIZE, "/div", "uS" , "mS",
                                    (long)g_sRender.uluSPerDivision);
            GrStringDraw(&g_sOffscreenContext, g_pcCaptionBuffer,
                        (unsigned long)iLen, 0, CAPTION_LINE_SPACING, false);
        }

        //
        // Are we showing the channel 2 waveform?
        //
        if(pbChannels[CHANNEL_2] & pCapData->bDualMode)
        {
            //
            // Yes - display channel 2 voltage scale
            //
            GrContextForegroundSet(&g_sOffscreenContext, CHANNEL_2_COLOR);
            iLen = RendererFormatDisplayString(g_pcCaptionBuffer,
                                   CAPTION_BUFFER_SIZE, "/div", "mV" , "V",
                                   (long)g_sRender.ulmVPerDivision[CHANNEL_2]);
            GrStringDraw(&g_sOffscreenContext, g_pcCaptionBuffer,
                         (unsigned long)iLen, 0,
                         (WAVEFORM_HEIGHT - CAPTION_LINE_SPACING), false);
        }
    }

    //
    // Render waveform voltages if requested
    //
    if(pRender->bShowMeasurements)
    {
        //
        // Are we showing the channel 1 waveform?
        //
        if(pbChannels[CHANNEL_1])
        {
            //
            // Yes - display channel 1 measurements.
            //
            RendererShowMeasurements(&psMeasure->sInfo[CHANNEL_1], true);
        }

        //
        // Are we showing the channel 2 waveform?
        //
        if(pbChannels[CHANNEL_2] & pCapData->bDualMode)
        {
            //
            // Yes - display channel 2 measurements
            //
            RendererShowMeasurements(&psMeasure->sInfo[CHANNEL_2], false);
        }
    }
}

//*****************************************************************************
//
// Draws the graticule on the offscreen waveform display area.
//
//*****************************************************************************
void
RendererDrawGraticule(void)
{
    unsigned long ulLoop;

    //
    // Set the drawing color
    //
    GrContextForegroundSet(&g_sOffscreenContext, GRATICULE_COLOR);

    //
    // Draw the vertical lines
    //
    for(ulLoop = GRATICULE_SIDE; ulLoop < WAVEFORM_WIDTH;
        ulLoop += GRATICULE_SIDE)
    {
        GrLineDraw(&g_sOffscreenContext, ulLoop, 0, ulLoop,
                   WAVEFORM_HEIGHT - 1);
    }

    //
    // Draw the horizontal lines
    //
    for(ulLoop = GRATICULE_SIDE; ulLoop < WAVEFORM_HEIGHT;
        ulLoop += GRATICULE_SIDE)
    {
        GrLineDraw(&g_sOffscreenContext, 0, ulLoop,
                   WAVEFORM_WIDTH - 1, ulLoop);
    }
}

//*****************************************************************************
//
// Handle timing checks for the alert message and, if the message has just
// been dismissed, refresh the display as required.
//
//*****************************************************************************
void
RendererUpdateAlert(void)
{
    unsigned long ulElapsed;
    unsigned long ulNow;

    //
    // If an alert is currently displayed, determine whether or not it is time
    // to erase it.
    //
    if(g_ulAlertCount)
    {
        //
        // Take a snapshot of the system tick counter.
        //
        ulNow = g_ulSysTickCounter;

        //
        // Determine how many ticks elapsed since the alert was first
        // displayed.  This takes account of wrap in the tick counter
        // (which occurs after about 497 days of operation).
        //
        ulElapsed = ((ulNow >= g_ulAlertStart) ? (ulNow - g_ulAlertStart) :
                     (ulNow + (0xFFFFFFFF - g_ulAlertStart) + 1));

        //
        // If sufficient time has elapsed, turn off the alert.
        //
        if(ulElapsed >= g_ulAlertCount)
        {
            g_ulAlertCount = 0;

            //
            // Redraw the refresh area with either the help screen or
            // the waveform depending upon what was being shown behind
            // the alert.
            //

            //
            // Set the clipping region to the alert box area.
            //
            GrContextClipRegionSet(&g_sContext, &g_sRectAlert);

            if(g_bShowingHelp)
            {
                RendererDrawHelpScreen(true);
            }
            else
            {
                //
                // Update the alert box from the current offscreen
                // waveform image.
                //
                GrImageDraw(&g_sContext, g_pucOffscreenImage, WAVEFORM_LEFT,
                            WAVEFORM_TOP);
            }

            //
            // Remove the clipping region.
            //
            GrContextClipRegionSet(&g_sContext, &g_sRectDisplay);
        }
    }
}

//*****************************************************************************
//
// Updates the display with the contents of the offscreen frame buffer.
//
//*****************************************************************************
void
RendererUpdate(void)
{
    tRectangle sRect;

    //
    // Handle the alert message if any is currently being displayed.
    //
    RendererUpdateAlert();

    //
    // If we are displaying an alert, don't update the waveform in the
    // area of the alert box.
    //
    if(!g_ulAlertCount)
    {
        //
        // Copy the whole offscreen image to the display.
        //
        GrImageDraw(&g_sContext, g_pucOffscreenImage, WAVEFORM_LEFT,
                    WAVEFORM_TOP);
    }
    else
    {
        //
        // Update the sections of the offscreen bitmap outside the area
        // used to display the alert message.  We do this with 4 blits, each
        // performed with a different clipping rectangle set.
        //

        //
        // Top rectangle.
        //
        sRect.sXMin = WAVEFORM_LEFT;
        sRect.sXMax = WAVEFORM_RIGHT;
        sRect.sYMin = 0;
        sRect.sYMax = g_sRectAlert.sYMin - 1;
        GrContextClipRegionSet(&g_sContext, &sRect);

        GrImageDraw(&g_sContext, g_pucOffscreenImage, WAVEFORM_LEFT,
                    WAVEFORM_TOP);

        //
        // Bottom rectangle.
        //
        sRect.sXMin = WAVEFORM_LEFT;
        sRect.sXMax = WAVEFORM_LEFT + WAVEFORM_WIDTH - 1;
        sRect.sYMin = g_sRectAlert.sYMax + 1;
        sRect.sYMax = g_sRectWaveform.sYMax;
        GrContextClipRegionSet(&g_sContext, &sRect);

        GrImageDraw(&g_sContext, g_pucOffscreenImage, WAVEFORM_LEFT,
                    WAVEFORM_TOP);

        //
        // Left rectangle.
        //
        sRect.sXMin = WAVEFORM_LEFT;
        sRect.sXMax = g_sRectAlert.sXMin - 1;
        sRect.sYMin = g_sRectAlert.sYMin;
        sRect.sYMax = g_sRectAlert.sYMax;
        GrContextClipRegionSet(&g_sContext, &sRect);

        GrImageDraw(&g_sContext, g_pucOffscreenImage, WAVEFORM_LEFT,
                    WAVEFORM_TOP);

        //
        // Right rectangle.
        //
        sRect.sXMin = g_sRectAlert.sXMax + 1;;
        sRect.sXMax = g_sRectWaveform.sXMax;
        sRect.sYMin = g_sRectAlert.sYMin;
        sRect.sYMax = g_sRectAlert.sYMax;
        GrContextClipRegionSet(&g_sContext, &sRect);

        GrImageDraw(&g_sContext, g_pucOffscreenImage, WAVEFORM_LEFT,
                    WAVEFORM_TOP);

        //
        // Set the clipping rectangle back to the full screen.
        //
        GrContextClipRegionSet(&g_sContext, &g_sRectDisplay);
    }

    //
    // Flush any pending updates to the glass (a no-op for the Formike display
    // but included since this is the correct way to use the API).
    //
    GrFlush(&g_sContext);
}

//*****************************************************************************
//
// Fills a rectangle of the offscreen frame buffer with pixels of level
// ulLevel.  If psFillRect is NULL, defaults to the full waveform area.
//
//*****************************************************************************
void
RendererFillRect(tRectangle *psFillRect, unsigned long ulColor)
{
    tRectangle *psRect;

    //
    // If passed a NULL rectangle, we default to the full display
    //
    if(psFillRect == (tRectangle *)0)
    {
        psRect = &g_sRectWaveformOffscreen;
    }
    else
    {
        psRect = psFillRect;
    }

    GrContextForegroundSet(&g_sOffscreenContext, ulColor);
    GrRectFill(&g_sOffscreenContext, psRect);
}

//*****************************************************************************
//
// Show the current alert message in a centered box in the middle of the
// waveform display area.
//
//*****************************************************************************
static void
DrawAlert(void)
{
    unsigned long ulLine2Index;
    long lX;
    long lY;

    //
    // Fill the alert box with the appropriate background color and outline it
    // with the border color.
    //
    GrContextForegroundSet(&g_sContext, ALERT_BACKGROUND_COLOR);
    GrRectFill(&g_sContext, &g_sRectAlert);
    GrContextForegroundSet(&g_sContext, ALERT_BORDER_COLOR);
    GrRectDraw(&g_sContext, &g_sRectAlert);
    GrContextForegroundSet(&g_sContext, ALERT_TEXT_COLOR);

    //
    // Get a pointer to the second line of text in the alert string.  If this
    // is NULL, we know there is only 1 line.
    //
    for(ulLine2Index = 0; g_pcAlertMessage[ulLine2Index] != '\0';
        ulLine2Index++)
    {
        if(g_pcAlertMessage[ulLine2Index] == '\n')
        {
            ulLine2Index++;
            break;
        }
    }

    //
    // Where's the center of the rectangle?
    //
    lX = (g_sRectAlert.sXMax + g_sRectAlert.sXMin) / 2;
    lY = (g_sRectAlert.sYMax + g_sRectAlert.sYMin) / 2;

    //
    // If there are 2 strings, adjust the Y coordinate to accommodate both
    // and render both.
    //
    if(g_pcAlertMessage[ulLine2Index] != '\0')
    {
        lY -= ALERT_LINE_HEIGHT / 2;
        GrStringDrawCentered(&g_sContext, g_pcAlertMessage,
                             (ulLine2Index - 1), lX, lY, false);
        lY += ALERT_LINE_HEIGHT;
        GrStringDrawCentered(&g_sContext, &g_pcAlertMessage[ulLine2Index],
                             -1, lX, lY, false);
    }
    else
    {
        //
        // Render only one line of text.
        //
        GrStringDrawCentered(&g_sContext, g_pcAlertMessage, -1, lX, lY,
                             false);
    }
}

//*****************************************************************************
//
// Sets a user alert message and the time for which it will be displayed.
// A user alert message is displayed in the middle of the waveform display
// area and remains there for the number of system ticks defined by the ulTicks
// parameter.  If ulTicks is 0, the message is displayed until the next call
// to RendererUpdate() at which point it is erased.
//
// String pcString can contain either 1 or 2 lines of text with the lines
// delimited by a single '\n' character.
//
//*****************************************************************************
void
RendererSetAlert(char *pcString, unsigned long ulTicks)
{
    //
    // Save the start time, duration and string to be displayed.
    //
    g_pcAlertMessage = pcString;
    g_ulAlertCount = ulTicks ? ulTicks : 1;
    g_ulAlertStart = g_ulSysTickCounter;

    //
    // Cause the alert to be displayed immediately.
    //
    DrawAlert();
}

//*****************************************************************************
//
// If an alert message is currently displayed, this call causes it to
// be removed on the next call to RendererUpdate.
//
//*****************************************************************************
void
RendererClearAlert(void)
{
    g_ulAlertCount = 0;
}

//*****************************************************************************
//
// Extract a color entry from an image palette.  Note that the caller is
// responsible for ensuring that the image and palette index are valid.
//
//*****************************************************************************
static unsigned long
GetImageColor(const unsigned char *pucImg, unsigned long ulIndex)
{
    const unsigned char *pucPalEntry;
    unsigned long ulColor;

    pucPalEntry = pucImg + 6;
    pucPalEntry += ulIndex * 3;

    ulColor = (unsigned long)(*pucPalEntry++) << ClrBlueShift;
    ulColor |= (unsigned long)(*pucPalEntry++) << ClrGreenShift;
    ulColor |= (unsigned long)(*pucPalEntry++) << ClrRedShift;

    return(ulColor);
}

#define SCALE_COMPONENT(val, scale)                          \
                                ((0xFF * (0x100 - (scale)) + \
                                  ((val) * (scale))) >> 8)

//*****************************************************************************
//
// Sets the destination image palette to a version of the source palette
// mixed with white.
//
// This function generates a palette for the destination image which is
// a blend of the source palette and pure white with \e ulScale containing
// the portion of white to include.
//
// For each palette color component,
//
// Dest = (255 * (256 - ulScale)) / 256 + ((Src * ulScale ) / 256)
//
//*****************************************************************************
static void
ScalePalette(const unsigned char *pucSource, tDisplay *psDest,
             unsigned long ulScale)
{
    unsigned long ulIndex;
    unsigned long ulSrc;
    unsigned long ulDest;

    for(ulIndex = 0; ulIndex < GrImageColorsGet(pucSource); ulIndex++)
    {
        ulSrc = GetImageColor(pucSource, ulIndex);

        ulDest = SCALE_COMPONENT(ulSrc & 0xFF, ulScale);
        ulDest |= (SCALE_COMPONENT((ulSrc >> 8) & 0xFF, ulScale) << 8);
        ulDest |= (SCALE_COMPONENT((ulSrc >> 16) & 0xFF, ulScale) <<16);

        GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplay, &ulDest, ulIndex, 1);
    }
}

//*****************************************************************************
//
// Fade an image from white to its final color palette at the given screen
// position.
//
// Parameter ulSpeed indicates the fade rate in terms of SysTick counts
// (1/100ths of seconds).  The fade is performed using 64 steps so the total
// time will be ((64 * ulSpeed) / 100) seconds.
//
//*****************************************************************************
void
FadeInImage(const unsigned char *pucImage, long lX, long lY,
            unsigned long ulSpeed)
{
    unsigned long ulLoop;
    unsigned long ulColor;
    unsigned long ulNumColors;
    tRectangle sRectClip;
    unsigned long ulNextStep;

    //
    // Set the offscreen surface palette to the palette we need to use for
    // the image.
    //
    ulNumColors = GrImageColorsGet(pucImage);
    for(ulLoop = 0; ulLoop < ulNumColors; ulLoop++)
    {
        ulColor = GetImageColor(pucImage, ulLoop);
        GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplay, &ulColor, ulLoop, 1);
    }

    //
    // Now blit the logo from flash into the top left corner of our
    // offscreen buffer.
    //
    GrImageDraw(&g_sOffscreenContext, pucImage, 0, 0);

    //
    // Determine the clipping rectangle on the screen such that we draw only
    // the logo.
    //
    sRectClip.sXMin = (short)lX;
    sRectClip.sYMin = (short)lY;
    sRectClip.sXMax = sRectClip.sXMin + GrImageWidthGet(pucImage) - 1;
    sRectClip.sYMax = sRectClip.sYMin + GrImageHeightGet(pucImage) - 1;
    GrContextClipRegionSet(&g_sContext, &sRectClip);

    //
    // Prime our timer check value.
    //
    ulNextStep = g_ulSysTickCounter + ulSpeed;

    //
    // Now fade the image in.  We do this by scaling the palette entries in the
    // offscreen image and refreshing the display.
    //
    for(ulLoop = 0; ulLoop <= 256; ulLoop += 4)
    {
        ScalePalette(pucImage, &g_sOffscreenDisplay, ulLoop);
        GrImageDraw(&g_sContext, g_pucOffscreenImage, lX, lY);

        while(ulNextStep > g_ulSysTickCounter)
        {
            //
            // Wait for the desired amount of time before performing the next
            // fade step.
            //
        }

        //
        // Determine when the next step should be finished
        //
        ulNextStep = g_ulSysTickCounter + ulSpeed;
    }

    //
    // Perform one last step to ensure that we always end up with the final
    // palette regardless of the speed supplied.
    //
    ScalePalette(pucImage, &g_sOffscreenDisplay, 256);
    GrImageDraw(&g_sContext, g_pucOffscreenImage, lX, lY);

    //
    // Now that the fade is done, reset the main display clipping region.
    //
    GrContextClipRegionSet(&g_sContext, &g_sRectDisplay);
}

//*****************************************************************************
//
// Displays the oscilloscope connection help screen.
//
// This function overwrites the waveform display area with information showing
// which connection is which in the 8 pin connector above the color STN
// display.
//
//*****************************************************************************
void
RendererDrawHelpScreen(tBoolean bShow)
{
    unsigned long ulLoop;
    tRectangle sRectEdge;

    //
    // Are we showing or hiding the help screen?
    //
    if(bShow)
    {
        //
        // We are showing the help screen.
        //

        //
        // Clear the waveform display area with black in preparation for
        // displaying the help text.
        //
        GrContextForegroundSet(&g_sContext, BACKGROUND_COLOR);
        GrRectFill(&g_sContext, &g_sRectWaveform);

        //
        // Set the font we will use for the text.
        //
        GrContextFontSet(&g_sOffscreenContext, g_pFontFixed6x8);

        //
        // Set the color we will for the lines.
        //
        GrContextForegroundSet(&g_sContext, ClrRed);

        //
        // Draw the lines joining the text to the pin it represents.
        //
        for(ulLoop = 0; ulLoop < 8; ulLoop++)
        {
            //
            // Draw two red lines joining the right of the text with the
            // pin position.
            //
            GrLineDrawH(&g_sContext, LINE_LEFT_START, PIN_POS_X(ulLoop),
                        HELP_LINE_Y(ulLoop) + 1 + (HELP_LINE_HEIGHT / 2));
            GrLineDrawV(&g_sContext, PIN_POS_X(ulLoop), 0,
                        HELP_LINE_Y(ulLoop) + 1 + (HELP_LINE_HEIGHT / 2));

            //
            // Use the first character of the string as the pin number
            // and show it somewhere near where the pin is.
            //
            GrStringDrawCentered(&g_sContext, g_pcHelpText[ulLoop], 1,
                        PIN_POS_X(ulLoop), HELP_LINE_HEIGHT / 2, true);
        }

        //
        // Set the color we will use for the text.
        //
        GrContextForegroundSet(&g_sContext, ClrWhite);

        //
        // Write the lines of text to the display.
        //
        for(ulLoop = 0; ulLoop < NUM_HELP_LINES; ulLoop++)
        {
            //
            // Render the text
            //
            GrStringDraw(&g_sContext, g_pcHelpText[ulLoop], -1, HELP_LEFT,
                         HELP_LINE_Y(ulLoop), true);
        }
    }
    else
    {
        //
        // We are hiding the help screen.  We need to tidy up, erasing
        // the edge areas and refreshing the waveform area with the
        // current offscreen image.
        //

        //
        // Set the drawing color to black
        //
        GrContextForegroundSet(&g_sContext, BACKGROUND_COLOR);

        //
        // Fill the left edge sliver with black
        //
        sRectEdge.sXMin = 0;
        sRectEdge.sXMax = WAVEFORM_LEFT - 1;
        sRectEdge.sYMin = 0;
        sRectEdge.sYMax = WAVEFORM_BOTTOM - 1;
        GrRectFill(&g_sContext, &sRectEdge);

        //
        // Fill the right edge sliver with black
        //
        sRectEdge.sXMin += WAVEFORM_WIDTH;
        sRectEdge.sXMax = g_sRectDisplay.sXMax;
        GrRectFill(&g_sContext, &sRectEdge);

        //
        // Now redraw the waveform area from the offscreen image.
        //
        RendererUpdate();
    }
}

//*****************************************************************************
//
// Displays the application startup screen.
//
// This function, called immediately after RendererInit() and before any
// waveforms have been captured or displayed, shows the startup animation for
// the application.  The screen fades from black to white then the Texas
// Instruments logo fades down from white.  Once this fade is completed, the
// third party toolchain vendor logo is shown at the bottom of the screen.  We
// wait a couple of seconds then clear the screen to black and return.
//
//*****************************************************************************
void
RendererShowStartupScreen(void)
{
    unsigned long ulLoop;
    unsigned long ulColor;
    long lX;
    long lY;

    //
    // Fade the screen to white.
    //
    for(ulLoop = 0; ulLoop < 256; ulLoop += 4)
    {
        ulColor = (((ulLoop & 0xFF) << ClrRedShift) |
                   ((ulLoop & 0xFF) << ClrGreenShift) |
                   ((ulLoop & 0xFF) << ClrBlueShift));
        GrContextForegroundSet(&g_sContext, ulColor);
        GrRectFill(&g_sContext, &g_sRectDisplay);
    }

    //
    // Determine where on the screen the TI logo is going to go.  We center
    // it horizontally and set the vertical position to leave sufficient
    // space to place the toolchain logo beneath it.
    //
    lX = ((DpyWidthGet(&g_sFormike128x128x16) -
           GrImageWidthGet(g_pucTILogoImage)) / 2);

    if(GrImageWidthGet(g_pucToolchainLogoImage) == 0)
    {
        //
        // The application has been built without a special toolchain logo
        // so we just center the TI logo on the screen.
        //
        lY = ((DpyHeightGet(&g_sFormike128x128x16) -
               GrImageHeightGet(g_pucTILogoImage)) / 2);
    }
    else
    {
        //
        // We have to fit a toolchain logo at the bottom of the screen after
        // we fade in the TI logo.  Position the first logo such that we have
        // equal amounts of whitespace above, below and between the two logos.
        //
        lY = ((DpyHeightGet(&g_sFormike128x128x16) -
               (GrImageHeightGet(g_pucTILogoImage) +
                GrImageHeightGet(g_pucToolchainLogoImage))) / 3);
    }

    //
    // Now fade in the TI logo.
    //
    FadeInImage(g_pucTILogoImage, lX, lY, 3);

    //
    // Display the toolchain logo if one has been included.
    //
    if(GrImageWidthGet(g_pucToolchainLogoImage) != 0)
    {
        lY = (DpyHeightGet(&g_sFormike128x128x16) -
              (lY + GrImageHeightGet(g_pucToolchainLogoImage)));
        lX = ((DpyWidthGet(&g_sFormike128x128x16) -
               GrImageWidthGet(g_pucToolchainLogoImage)) / 2);

        FadeInImage(g_pucToolchainLogoImage, lX, lY, 2);
    }

    //
    // Wait for a while with the logos displayed.
    //
    ulLoop = g_ulSysTickCounter + LOGO_DISPLAY_DELAY;
    while(ulLoop > g_ulSysTickCounter)
    {
        //
        // Wait around for the desired time to elapse before continuing.
        //
    }

    //
    // Get things back to the state they need to be in before we start
    // rendering waveforms.
    //
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplay, g_pulPalette, 0,
                              WAVEFORM_NUM_COLORS);
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &g_sRectDisplay);
}
