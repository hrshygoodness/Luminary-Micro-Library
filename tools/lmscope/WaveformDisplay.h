//*************************************************************************
//
// Class derived from CStatic that we use to manage the waveform
// display control.
//
//*************************************************************************
#pragma once

//
// Indices for each channel used in several member variables
// declared as two element arrays.
//
#define CHANNEL_1 0
#define CHANNEL_2 1

//
// The number of graticule divisions across each display line.
//
#define NUM_HORIZONTAL_DIVISIONS 12

//
// Colors used in drawing the waveform display
//
#define NUM_WAVEFORM_COLORS 6

#define SCOPE_COLOR_BACKGROUND  RGB(0x00, 0x00, 0x00)
#define SCOPE_COLOR_CHANNEL_1   RGB(0xFF, 0xFF, 0x00)
#define SCOPE_COLOR_CHANNEL_2   RGB(0xEE, 0x82, 0xEE)
#define SCOPE_COLOR_GRATICULE   RGB(0x00, 0x80, 0x00)
#define SCOPE_COLOR_TRIG_POS    RGB(0xFF, 0x00, 0x00)
#define SCOPE_COLOR_TRIG_LEVEL  RGB(0xFF, 0x00, 0x00)

// CWaveformDisplay custom control

class CWaveformDisplay : public CStatic
{
	DECLARE_DYNAMIC(CWaveformDisplay)

public:
	CWaveformDisplay();   // standard constructor
	virtual ~CWaveformDisplay();
    void RenderWaveform(tScopeDataStart *pScopeData = NULL,
                        int iElementOffset = 0);
    void InitBitmap(void);
    void SetTimebase(unsigned long uluSPerDivision, BOOL bUpdate = TRUE);
    void SetTriggerLevel(long lTriggerLevelmV, BOOL bUpdate = TRUE);
    void SetTriggerPos(int iTriggerPos, BOOL bUpdate = TRUE);
    void SetChannelPos(int iChannel, long lVerticalOffsetmV,
                       BOOL bUpdate = TRUE);
    void SetChannelScale(int iChannel, unsigned long ulmVPerDivision,
                         BOOL bUpdate = TRUE);
    BOOL SaveAsBMP(CString *pFilename);
    BOOL SaveAsCSV(CString *pFilename);
    BOOL IsTriggerLevelShown(void);
    BOOL IsTriggerPosShown(void);
    BOOL IsGraticuleShown(void);
    BOOL IsGroundShown(void);
    void ShowTriggerLevel(BOOL bShow, BOOL bUpdate = TRUE);
    void ShowTriggerPos(BOOL bShow, BOOL bUpdate = TRUE);
    void ShowGraticule(BOOL bShow, BOOL bUpdate = TRUE);
    void ShowGround(BOOL bShow, BOOL bUpdate = TRUE);
protected:
    void DrawWaveform(CDC *pDC);
    void DrawGraticule(CDC *pDC);
    long GetCenterOffset(void);
    void InternalRenderWaveform(void);
    long SampleToY(short sSamplemV, int iChannel);
    long SampleIndexToX(unsigned long ulSampleIndex, int iChannel);
    void DrawSingleWaveform(CDC *pDC, int iChannel, COLORREF clrColor);
    short GetSample(unsigned long ulIndex, int iChannel,
                    unsigned long *pulTime = NULL);
    HANDLE CreateDIB(int *piSizeDIB);
protected:
    CBitmap m_bmpWaveform;
    CSize m_sizeWaveform;
    int m_iGraticuleSide;
    int m_iGraticuleOriginY;
    int m_iTriggerPos;
    long m_lTriggerLevelmV;
    unsigned long m_uluSPerDivision;
    long m_lVerticalOffsetmV[2];
    unsigned long m_ulmVPerDivision[2];
    BOOL m_bShowGraticule;
    BOOL m_bShowTrigLevel;
    BOOL m_bShowTrigPos;
    BOOL m_bShowGround;
    tScopeDataStart *m_pScopeData;
    tScopeDualDataElement *m_pDualElement;
    tScopeDataElement *m_pSingleElement;
public:
// Dialog Data
	enum { IDD = IDD_LMSCOPE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
public:

};
