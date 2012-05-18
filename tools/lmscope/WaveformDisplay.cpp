// WaveformDisplay.cpp : implementation file
//

#include "stdafx.h"
#define PACKED
#pragma pack(1)
#include "usb_protocol.h"
#pragma pack()
#include "scope_control.h"
#include "lmscope.h"
#include "WaveformDisplay.h"
#include "lmscopeDlg.h"

//***************************************************************************
//
// Macro converting from a millivolt measurement to a Y pixel coordinate
// given a voltage scaling factor.
//
//***************************************************************************

#define MV_TO_Y(mv, scale) (m_iGraticuleOriginY -                           \
                            (((mv) * m_iGraticuleSide) / (long)(scale)))

const COLORREF g_colWaveformColors[NUM_WAVEFORM_COLORS] =
{
    SCOPE_COLOR_BACKGROUND,
    SCOPE_COLOR_CHANNEL_1,
    SCOPE_COLOR_CHANNEL_2,
    SCOPE_COLOR_GRATICULE,
    SCOPE_COLOR_TRIG_POS,
    SCOPE_COLOR_TRIG_LEVEL
};

//***************************************************************************
//
// CWaveformDisplay class
//
//***************************************************************************
IMPLEMENT_DYNAMIC(CWaveformDisplay, CStatic)

CWaveformDisplay::~CWaveformDisplay()
{
}

void CWaveformDisplay::DoDataExchange(CDataExchange* pDX)
{
    CStatic::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CWaveformDisplay, CStatic)
END_MESSAGE_MAP()


//***************************************************************************
//
// CWaveform member functions.
//
//***************************************************************************
CWaveformDisplay::CWaveformDisplay() : CStatic()
{
    //
    // Set defaults for the waveform elements we will draw.
    //
    m_bShowGraticule = TRUE;
    m_bShowTrigLevel = TRUE;
    m_bShowTrigPos = TRUE;

    //
    // Set defaults for various fields we expect the client to set
    // later.
    //
    m_pScopeData = NULL;
    m_pSingleElement = NULL;
    m_pDualElement = NULL;
    m_iTriggerPos = 0;
    m_lTriggerLevelmV = 0;
    m_uluSPerDivision = 100;
    m_lVerticalOffsetmV[0] = 0;
    m_lVerticalOffsetmV[1] = 0;
    m_ulmVPerDivision[0] = 500;
    m_ulmVPerDivision[1] = 500;
}

//***************************************************************************
//
// Initialize the bitmap we use for offscreen drawing.
//
//***************************************************************************
void CWaveformDisplay::InitBitmap(void)
{
    CBitmap * pOldBitmap;
    CDC *pDC;
    CDC MemDC;
    CRect rectArea;

    //
    // Determine the size of the area we are going to be rendering into.
    //
    GetClientRect(&rectArea);
    m_sizeWaveform = rectArea.Size();

    m_iGraticuleSide = m_sizeWaveform.cx / NUM_HORIZONTAL_DIVISIONS;
    m_iGraticuleOriginY = (((m_sizeWaveform.cy / m_iGraticuleSide) / 2) *
                           m_iGraticuleSide);

    // Need a DC
    pDC = GetDC();

    // And a memory DC, set Map Mode
    MemDC.CreateCompatibleDC(pDC);

    // Create a bitmap big enough to hold the window's image
    m_bmpWaveform.CreateCompatibleBitmap(pDC, m_sizeWaveform.cx, m_sizeWaveform.cy);

    // Select the bitmap into the memory DC
    pOldBitmap = MemDC.SelectObject(&m_bmpWaveform);

    // Draw the initial waveform display (background and graticule only)
    DrawWaveform(&MemDC);

    // Select the original bitmap back in
    MemDC.SelectObject(pOldBitmap);

    // Clean up
    ReleaseDC(pDC);

    //
    // Associate the static picture control with our offscreen bitmap
    //
    SetBitmap((HBITMAP)m_bmpWaveform);
}

//***************************************************************************
//
// This function is called whenever new data is received from the
// oscilloscope. It renders the data into our offscreen bitmap then causes
// the picture control on the dialog box to be refreshed.
//
//***************************************************************************
void CWaveformDisplay::RenderWaveform(tScopeDataStart *pScopeData,
                                      int iElementOffset)
{
    //
    // If we already have a dataset, discard it.
    //
    if(m_pScopeData)
    {
        LocalFree(m_pScopeData);
    }

    //
    // Hold on to the new dataset.
    //
    m_pScopeData = pScopeData;

    //
    // Calculate pointers to the first data element in the dataset.
    // Even though the dataset can only be either single or dual
    // channel, we keep two pointers anyway.
    //
    m_pDualElement = (tScopeDualDataElement *)((char *)pScopeData +
                                               iElementOffset);
    m_pSingleElement = (tScopeDataElement *)((char *)pScopeData +
                                             iElementOffset);

    //
    // Call our internal function to render the new data.
    //
    InternalRenderWaveform();
}

//***************************************************************************
//
// This internal function is called to redraw the existing waveform dataset
// after a display parameter change or when new data has just been sent by
// the client.
//
//***************************************************************************
void CWaveformDisplay::InternalRenderWaveform(void)
{
    CBitmap * pOldBitmap;
    CDC *pDC;
    CDC MemDC;
    CRect rectArea;

    //
    // Get a device context.
    //
    pDC = GetDC();

    //
    // Create a memory DC compatible with the one we just got.
    //
    MemDC.CreateCompatibleDC(pDC);

    //
    // Select the bitmap into the memory DC
    //
    pOldBitmap = MemDC.SelectObject(&m_bmpWaveform);

    //
    // Draw the waveform display
    //
    DrawWaveform(&MemDC);

    //
    // Select the original bitmap back in.
    //
    MemDC.SelectObject(pOldBitmap);

    //
    // Clean up
    //
    ReleaseDC(pDC);

    //
    // Invalidate the rectangle associated with the waveform picture control.
    //
    Invalidate(FALSE);
}

//***************************************************************************
//
// Render the latest waveform data, if any, into our offscreen bitmap.
//
//***************************************************************************
void CWaveformDisplay::DrawWaveform(CDC *pDC)
{
    long lCenterOffset;
    long lY;
    long lX;
    COLORREF dwColor = SCOPE_COLOR_BACKGROUND;
    CPen cPenTrigPos(PS_SOLID, 1, SCOPE_COLOR_TRIG_POS);
    CPen cPenTrigLevel(PS_SOLID, 1, SCOPE_COLOR_TRIG_LEVEL);
    CPen *pOldPen;

    //
    // Fill the background of the bitmap with grey.
    //
    pDC->FillSolidRect(0, 0, m_sizeWaveform.cx, m_sizeWaveform.cy, dwColor);

    //
    // Draw the graticule if required.
    //
    if(m_bShowGraticule)
    {
        DrawGraticule(pDC);
    }

    //
    // Render the actual waveform itself.
    //
    if(m_pScopeData)
    {
        //
        // Determine the horizontal offset to apply to the waveform to ensure
        // that the trigger position is where we expect it to be (centered by
        // default).
        //
        lCenterOffset = GetCenterOffset();

        //
        // Draw a vertical line at the trigger point.
        //
        if(m_bShowTrigPos)
        {
            //
            // Select our pen into the DC.
            //
            pOldPen = pDC->SelectObject(&cPenTrigPos);

            //
            // Determine where on the display the trigger point falls
            //
            lY = m_pScopeData->ulTriggerIndex;

            //
            // Now determine where we need to draw the trigger position line.
            //
            lX = SampleIndexToX(lY, CHANNEL_1);

            //
            // If the trigger position is visible in the waveform area of the
            // screen, draw a vertical line there.
            //
            if((lX >= 0) && (lX < m_sizeWaveform.cx))
            {
                pDC->SetDCPenColor(SCOPE_COLOR_TRIG_POS);
                pDC->MoveTo(lX, 0);
                pDC->LineTo(lX, m_sizeWaveform.cy);
            }

            //
            // Revert to the previous pen.
            //
            pDC->SelectObject(pOldPen);
        }

        //
        // ...and a horizontal line at the trigger level.
        //
        if(m_bShowTrigLevel)
        {
            //
            // Convert the trigger level into a Y pixel offset.
            //
            lY = SampleToY((short)m_lTriggerLevelmV, (m_pScopeData->bCh2SampleFirst ?
                           CHANNEL_2 : CHANNEL_1));

            if((lY >= 0) && (lY <= m_sizeWaveform.cy))
            {
                //
                // Select our pen into the DC.
                //
                pOldPen = pDC->SelectObject(&cPenTrigLevel);

                //
                // Draw the line
                //
                pDC->MoveTo(0, lY);
                pDC->LineTo(m_sizeWaveform.cx, lY);

                //
                // Revert to the previous pen
                //
                pDC->SelectObject(pOldPen);
            }
        }

        //
        // Render the waveform for channel 1
        //
        DrawSingleWaveform(pDC, CHANNEL_1, SCOPE_COLOR_CHANNEL_1);

        //
        // Yes - render the waveform for channel 2
        //
        DrawSingleWaveform(pDC, CHANNEL_2, SCOPE_COLOR_CHANNEL_2);
    }
}


//***************************************************************************
//
// Draw the graticule lines on our offscreen bitmap.
//
//***************************************************************************
void CWaveformDisplay::DrawGraticule(CDC *pDC)
{
    int iX;
    int iY;
    CPen cPen(PS_SOLID, 1, SCOPE_COLOR_GRATICULE);
    CPen *pcPenOld;

    //
    // Set the pen color for the graticule lines.
    //
    pcPenOld = pDC->SelectObject(&cPen);

    //
    // Draw the vertical lines.
    //
    for(iX = m_iGraticuleSide; iX < m_sizeWaveform.cx; iX += m_iGraticuleSide)
    {
        pDC->MoveTo(iX, 0);
        pDC->LineTo(iX, m_sizeWaveform.cy);
    }

    //
    // Draw the horizontal lines.
    //
    for(iY = m_iGraticuleSide; iY < m_sizeWaveform.cy; iY += m_iGraticuleSide)
    {
        pDC->MoveTo(0, iY);
        pDC->LineTo(m_sizeWaveform.cx, iY);
    }

    //
    // Revert to the original pen.
    //
    pDC->SelectObject(pcPenOld);
}

//*****************************************************************************
//
// Calculates the horizontal pixel offset required to center the waveform
// trace on the display.
//
// \param pCapData points to a structure containing information on the
// oscilloscope data which is to be rendered.
//
// This function is called to calculate the correct offset required to
// position the rendered waveform such that the middle sample in the
// waveform will appear at the center of the display area.
//
// \return The required offset to correctly center the waveform.
//
//*****************************************************************************
long CWaveformDisplay::GetCenterOffset(void)
{
    long lX;
    long lCenterIndex;

    //
    // If called before we have anything to render, just return 0.
    //
    if(m_pScopeData == NULL)
    {
        return(0);
    }

    //
    // First determine where the center sample is. If this is dual channel
    // data, the number of sample periods is effectively half what it would
    // be if we were dealing with single channel data.
    //
    lCenterIndex = m_pScopeData->ulTotalElements / 2;

    //
    // Now determine the total number of microseconds from the start of the
    // buffer to the center sample.
    //
    lX = lCenterIndex * m_pScopeData->ulSamplePerioduS;

    //
    // Convert from microseconds to pixels given the selected timebase. At
    // this point, we have a distance in pixels from the earliest sample.
    //
    lX = (lX * m_iGraticuleSide)/m_uluSPerDivision;

    //
    // The correction we apply to the x coordinates is the center offset minus
    // half the width of the display.
    //
    return(lX - (m_sizeWaveform.cx / 2));
}

//***************************************************************************
//
// Update the timebase and, optionally, redraw the waveform.
//
//***************************************************************************
void CWaveformDisplay::SetTimebase(unsigned long uluSPerDivision, BOOL bUpdate)
{
    m_uluSPerDivision = uluSPerDivision;

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}

//***************************************************************************
//
// Update the trigger level and, optionally, redraw the waveform.
//
//***************************************************************************
void CWaveformDisplay::SetTriggerLevel(long lTriggerLevelmV, BOOL bUpdate)
{
    m_lTriggerLevelmV = lTriggerLevelmV;

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}

//***************************************************************************
//
// Update the trigger position and, optionally, redraw the waveform.
//
//***************************************************************************
void CWaveformDisplay::SetTriggerPos(int iTriggerPos, BOOL bUpdate)
{

    //
    // The supplied trigger position is in the range (-60, 60) to match
    // the waveform display size on the device. Rescale this so that
    // the same bounds map to the complete width of the window here.
    //
    m_iTriggerPos = (iTriggerPos * m_sizeWaveform.cx) /
        (TRIGGER_POS_SLIDER_MAX - TRIGGER_POS_SLIDER_MIN);

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}

//***************************************************************************
//
// Update the vertical offset for one channel and, optionally, redraw the
// waveform.
//
//***************************************************************************
void CWaveformDisplay::SetChannelPos(int iChannel, long lVerticalOffsetmV,
                                    BOOL bUpdate)
{
    ASSERT((iChannel == CHANNEL_1) || (iChannel == CHANNEL_2));

    m_lVerticalOffsetmV[iChannel] = lVerticalOffsetmV;

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}

//***************************************************************************
//
// Update the vertical scale for one channel and, optionally, redraw the
// waveform.
//
//***************************************************************************
void CWaveformDisplay::SetChannelScale(int iChannel,
                                      unsigned long ulmVPerDivision,
                                      BOOL bUpdate)
{
    ASSERT((iChannel == CHANNEL_1) || (iChannel == CHANNEL_2));

    m_ulmVPerDivision[iChannel] = ulmVPerDivision;

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}

//*****************************************************************************
//
// Convert a sample index in the current data set into a horizontal (x)
// pixel coordinate on the display.
//
//*****************************************************************************
long CWaveformDisplay::SampleIndexToX(unsigned long ulSampleIndex, int iChannel)
{
    long lX;
    bool bApplyOffset;

    //
    // If we don't have any dataset to play with, just return 0.
    //
    if(m_pScopeData == NULL)
    {
        return(0);
    }

    //
    // Determine whether or not we need to apply the second sample time
    // offset.
    //
    bApplyOffset = (((iChannel == CHANNEL_1) &&
                     (m_pScopeData->bCh2SampleFirst)) ||
                    ((iChannel == CHANNEL_2) &&
                     (!m_pScopeData->bCh2SampleFirst)));

    //
    // First determine the number of microseconds from the start of the capture
    // to this sample. Note that we ensure we add the sample offset if this is
    // a sample for the second channel.
    //
    // ulSampleIndex <= 1024 (typically 512)
    // ulSamplePerioduS <= 10000 (min rate of 10mS per sample or 100Hz)
    // ulSampleOffsetuS <= 8
    //
    // so lX <= 10240008, hence no overflow in this calculation.
    //
    lX = (ulSampleIndex * m_pScopeData->ulSamplePerioduS) +
         (bApplyOffset ? m_pScopeData->ulSampleOffsetuS : 0);

    //
    // Now convert from microseconds to pixels given the selected timebase. At
    // this point, we have a distance in pixels from the earliest sample.
    //
    lX = (lX * m_iGraticuleSide)/m_uluSPerDivision;

    //
    // Adjust the pixel position for the current display window. We center the
    // window on the full trace then apply an offset to allow the user to pan
    // the window across the captured waveform (effectively changing the
    // trigger position from their point of view).
    //
    lX = lX - GetCenterOffset() + m_iTriggerPos;

    //
    // All done. Return the result to the caller.
    //
    return(lX);
}

//*****************************************************************************
//
// Convert an ADC sample to a Y coordinate on the display taking into account
// all display parameters in the passed rendering structure.
//
//*****************************************************************************
long CWaveformDisplay::SampleToY(short sSamplemV, int iChannel)
{
    long lY;
    long lMillivolts;

    //
    // Adjust this with the client-supplied offset value for the channel.
    //
    lMillivolts = sSamplemV + m_lVerticalOffsetmV[iChannel];

    //
    // Calculate the actual y coordinate representing this voltage given the
    // voltage scaling supplied by the client.
    //
    lY = MV_TO_Y(lMillivolts, m_ulmVPerDivision[iChannel]);

    return(lY);
}

//*****************************************************************************
//
// Draw the waveform for a single set of captured samples.
//
//*****************************************************************************
void CWaveformDisplay::DrawSingleWaveform(CDC *pDC, int iChannel, COLORREF clrColor)
{
    unsigned long ulLoop;
    short sSample;
    long lX;
    long lY;
    CPen cPen(PS_SOLID, 1, clrColor);
    CPen cDotPen(PS_DOT, 1, clrColor);
    CPen *pOldPen;

    //
    // Check for cases where we are asked to render a waveform for a channel
    // whose data we don't have.
    //
    if(!m_pScopeData->bDualChannel)
    {
        //
        // This is single channel data. Does it contain the channel we are
        // being asked to draw?
        //
        if(((iChannel == CHANNEL_2) && !m_pScopeData->bCh2SampleFirst) ||
           ((iChannel == CHANNEL_1) && m_pScopeData->bCh2SampleFirst))
        {
            return;
        }
    }

    //
    // Draw the ground level if required.
    //
    if(m_bShowGround)
    {
        //
        // Select the dotted pen.
        //
        pOldPen = pDC->SelectObject(&cDotPen);

        //
        // Determine where the ground line should go.
        //
        lY = SampleToY(0, iChannel);

        //
        // If ground is visible, draw the line.
        //
        if((lY >= 0) && (lY < m_sizeWaveform.cy))
        {
            pDC->MoveTo(0, lY);
            pDC->LineTo(m_sizeWaveform.cx, lY);
        }

        pDC->SelectObject(pOldPen);
    }

    //
    // Set the solid pen for the waveform.
    //
    pOldPen = pDC->SelectObject(&cPen);

    //
    // Loop through each of the captured samples to draw the waveform onto
    // the display.
    //
    for(ulLoop = 0; ulLoop < m_pScopeData->ulTotalElements; ulLoop++)
    {
        //
        // Get the screen X coordinate for this sample number given the
        // renderer timebase and offset.
        //
        lX = SampleIndexToX(ulLoop, iChannel);

        //
        // Get the screen Y coordinate for this sample given the renderer
        // voltage scaling factor and offset.
        //
        sSample = GetSample(ulLoop, iChannel);
        lY = SampleToY(sSample, iChannel);

        //
        // If this is not the first pixel, draw a line segment
        //
        if(ulLoop)
        {
            //
            // Draw one line segment.
            //
            pDC->LineTo(lX, lY);
        }
        else
        {
            //
            // Move the cursor to the start position.
            //
            pDC->MoveTo(lX, lY);
        }
    }

    //
    // Reinstate the old pen.
    //
    pDC->SelectObject(pOldPen);
}

//***************************************************************************
//
// Return the ulIndex-th sample for channel iChannel from the current
// dataset taking into account all the weird and wonderful combinations of
// single vs dual channel capture and channel swapping.
//
//***************************************************************************
short CWaveformDisplay::GetSample(unsigned long ulIndex, int iChannel,
                                  unsigned long *pulTime)
{
    ASSERT(m_pScopeData);

    //
    // Do we have single or dual channel data?
    //
    if(m_pScopeData->bDualChannel)
    {
        //
        // Dual channel data.
        //
        if(((iChannel == CHANNEL_1) && !m_pScopeData->bCh2SampleFirst) ||
            ((iChannel == CHANNEL_2) && m_pScopeData->bCh2SampleFirst))
        {
            if(pulTime)
            {
                *pulTime = ulIndex * m_pScopeData->ulSamplePerioduS;
            }
            return(m_pDualElement[ulIndex].sSample1mVolts);
        }
        else
        {
            if(pulTime)
            {
                *pulTime = (ulIndex * m_pScopeData->ulSamplePerioduS) +
                           m_pScopeData->ulSampleOffsetuS;
            }
            return(m_pDualElement[ulIndex].sSample2mVolts);
        }
    }
    else
    {
        //
        // Single channel data. We assume that the caller has already
        // ensured that the dataset contains samples for the desired
        // channel so we just return the single sample here without
        // checking the iChannel value.
        //
        if(pulTime)
        {
            *pulTime = ulIndex * m_pScopeData->ulSamplePerioduS;
        }
        return(m_pSingleElement[ulIndex].sSamplemVolts);
    }
}

//***************************************************************************
//
// Save the current data set as a Windows bitmap in the file whose name
// is provided.
//
//***************************************************************************
BOOL CWaveformDisplay::SaveAsBMP(CString *pFilename)
{
    HANDLE hDIB;
    CFile File;
    BOOL bRetcode;
    int iColors;
    int iSizeDIB;
    BITMAPFILEHEADER bfh;
    LPBITMAPINFOHEADER lpbi;

    //
    // Convert our internal bitmap to a device independent one that we can
    // then save to the file.
    //
    hDIB = CreateDIB(&iSizeDIB);

    if(hDIB)
    {
        //
        // We got the bitmap successfully so now save it.
        //
        bRetcode = File.Open((LPCTSTR)*pFilename,
                             (CFile::modeCreate | CFile::modeWrite));
        if(bRetcode)
        {
            //
            // Get a pointer to the bitmap header.
            //
            lpbi = (LPBITMAPINFOHEADER)hDIB;

            //
            // How many palette entries are there in the bitmap?
            //
            iColors = (lpbi->biBitCount <= 8) ? (1 << lpbi->biBitCount) : 0;

            //
            // Fill in the fields of the file header
            //
            bfh.bfType = ((WORD) ('M' << 8) | 'B');    // "BM"
            bfh.bfSize = iSizeDIB + sizeof(BITMAPFILEHEADER );
            bfh.bfReserved1 = 0;
            bfh.bfReserved2    = 0;
            bfh.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) +
                            lpbi->biSize + (iColors * sizeof(RGBQUAD)));

            //
            // Write the file header and bitmap to the file.
            //
            File.Write(&bfh, sizeof(BITMAPFILEHEADER));
            File.Write(hDIB, iSizeDIB);
            File.Close();
        }

        //
        // Free the DIB
        //
        LocalFree(hDIB);
    }

    return(bRetcode);
}

//***************************************************************************
//
// Save the current data set as a Comma Separated Value file in the file
// whose name is provided.
//
//***************************************************************************
BOOL CWaveformDisplay::SaveAsCSV(CString *pFilename)
{
    CStdioFile File;
    CString strWork;
    BOOL bRetcode;
    unsigned long ulLoop;
    unsigned long ulTime1;
    unsigned long ulTime2;
    short sSample1;
    short sSample2;

    //
    // We can't save the data if we don't have any data to save.
    //
    if(!m_pScopeData)
    {
        return(FALSE);
    }

    //
    // Open the file for writing in text mode.
    //
    bRetcode = File.Open((LPCTSTR)*pFilename, (CFile::modeCreate | CFile::modeWrite |
                         CFile::typeText));

    //
    // Did the file open correctly?
    //
    if(bRetcode)
    {
        //
        // All is well - write the file data.
        //
        File.WriteString(L"Oscilloscope Data\n");
        if(m_pScopeData->bDualChannel)
        {
            File.WriteString(L"Channel 1,, Channel 2\n");
            File.WriteString(L"Time (uS), Sample (mV), Time (uS), Sample (mV)\n");
        }
        else
        {
            File.WriteString(L"Channel 1\n");
            File.WriteString(L"Time (uS), Sample (mV)\n");
        }

        //
        // Write the sample data.
        //
        for(ulLoop = 0; ulLoop < m_pScopeData->ulTotalElements; ulLoop++)
        {
            if(m_pScopeData->bDualChannel)
            {
                sSample1 = GetSample(ulLoop, CHANNEL_1, &ulTime1);
                sSample2 = GetSample(ulLoop, CHANNEL_2, &ulTime2);
                strWork.Format(L"%6d, %6d, %6d, %6d\n", ulTime1, sSample1,
                               ulTime2, sSample2);
            }
            else
            {
                sSample1 = GetSample(ulLoop, CHANNEL_1, &ulTime1);
                strWork.Format(L"%6d, %6d\n", ulTime1, sSample1);
            }

            File.WriteString((LPCTSTR)strWork);
        }

        File.Close();
    }

    return(bRetcode);
}

//***************************************************************************
//
// Create a device independent bitmap from our offscreen drawing surface
// and return the handle to the caller.
//
//***************************************************************************
HANDLE CWaveformDisplay::CreateDIB(int *piSizeDIB)
{
    BITMAP bm;
    BITMAPINFOHEADER bi;
    LPBITMAPINFOHEADER lpbi;
    DWORD dwLen;
    HANDLE hDIB;
    CDC *pDC;
    CPalette pal;
    CPalette *pOldPal;
    int iLoop;

    //
    // Create a palette containing the colors we know are in the waveform image.
    //
    UINT nPalSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * NUM_WAVEFORM_COLORS);
    LOGPALETTE *pLP = (LOGPALETTE *) new BYTE[nPalSize];
    pLP->palVersion = 0x300;
    pLP->palNumEntries = NUM_WAVEFORM_COLORS;

    for(iLoop = 0; iLoop < NUM_WAVEFORM_COLORS; iLoop++)
    {
        pLP->palPalEntry[iLoop].peRed = GetRValue(g_colWaveformColors[iLoop]);
        pLP->palPalEntry[iLoop].peGreen = GetGValue(g_colWaveformColors[iLoop]);
        pLP->palPalEntry[iLoop].peBlue = GetBValue(g_colWaveformColors[iLoop]);
        pLP->palPalEntry[iLoop].peFlags = 0;
    }

    // Create the palette
    pal.CreatePalette( pLP );

    delete[] pLP;

    // Get bitmap information
    m_bmpWaveform.GetObject(sizeof(bm),(LPSTR)&bm);

    // Initialize the bitmapinfoheader
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 8;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    bi.biSizeImage = ((((bi.biWidth * bi.biBitCount) + 31) & ~31) / 8)
                        * bi.biHeight;

    // Compute the size of the  infoheader and the color table
    dwLen  = bi.biSize + 256 * sizeof(RGBQUAD) + bi.biSizeImage;

    // We need a device context to get the DIB from
    pDC = GetDC();
    pOldPal = pDC->SelectPalette(&pal,FALSE);
    pDC->RealizePalette();

    // Allocate enough memory to hold bitmapinfoheader and color table
    hDIB = LocalAlloc(LMEM_FIXED, dwLen);

    if (!hDIB)
    {
        pDC->SelectPalette(pOldPal,FALSE);
        ReleaseDC(pDC);
        return NULL;
    }

    //
    // Copy the bitmap info header into the new allocation.
    //
    lpbi = (LPBITMAPINFOHEADER)hDIB;
    *lpbi = bi;

    // Get the bitmap bits
    lpbi = (LPBITMAPINFOHEADER)hDIB;
    BOOL bGotBits = GetDIBits(pDC->m_hDC, (HBITMAP)m_bmpWaveform.GetSafeHandle(),
                0L,                // Start scan line
                (DWORD)bi.biHeight,        // # of scan lines
                (LPBYTE)lpbi             // address for bitmap bits
                + (bi.biSize + 256 * sizeof(RGBQUAD)),
                (LPBITMAPINFO)lpbi,        // address of bitmapinfo
                (DWORD)DIB_RGB_COLORS);        // Use RGB for color table

    if( !bGotBits )
    {
        LocalFree(hDIB);
        pDC->SelectPalette(pOldPal,FALSE);
        ReleaseDC(pDC);
        return(NULL);
    }

    pDC->SelectPalette(pOldPal,FALSE);
    ReleaseDC(pDC);

    //
    // Tell the caller the size of the bitmap in memory.
    //
    *piSizeDIB = dwLen;

    return(hDIB);
}

//***************************************************************************
//
// Does the display currently show the trigger level?
//
//***************************************************************************
BOOL CWaveformDisplay::IsTriggerLevelShown(void)
{
    return(m_bShowTrigLevel);
}

//***************************************************************************
//
// Does the display currently show the trigger position line?
//
//***************************************************************************
BOOL CWaveformDisplay::IsTriggerPosShown(void)
{
    return(m_bShowTrigPos);
}

//***************************************************************************
//
// Does the display currently show the graticule lines?
//
//***************************************************************************
BOOL CWaveformDisplay::IsGraticuleShown(void)
{
    return(m_bShowGraticule);
}

//***************************************************************************
//
// Does the display currently show the ground line?
//
//***************************************************************************
BOOL CWaveformDisplay::IsGroundShown(void)
{
    return(m_bShowGround);
}

//***************************************************************************
//
// Show or hide the trigger level marking line on the display.
//
//***************************************************************************
void CWaveformDisplay::ShowTriggerLevel(BOOL bShow, BOOL bUpdate)
{
    m_bShowTrigLevel = bShow;

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}

//***************************************************************************
//
// Show or hide the trigger position marking line on the display.
//
//***************************************************************************
void CWaveformDisplay::ShowTriggerPos(BOOL bShow, BOOL bUpdate)
{
    m_bShowTrigPos = bShow;

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}

//***************************************************************************
//
// Show or hide the graticule lines on the display.
//
//***************************************************************************
void CWaveformDisplay::ShowGraticule(BOOL bShow, BOOL bUpdate)
{
    m_bShowGraticule = bShow;

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}

//***************************************************************************
//
// Show or hide the ground marking line on the display.
//
//***************************************************************************
void CWaveformDisplay::ShowGround(BOOL bShow, BOOL bUpdate)
{
    m_bShowGround = bShow;

    if(bUpdate)
    {
        InternalRenderWaveform();
    }
}
