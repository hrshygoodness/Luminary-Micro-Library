//*************************************************************************
//
// lmscopeDlg.h : header file
//
//*************************************************************************
#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "waveformdisplay.h"

//*************************************************************************
//
// Ranges for the various sliders used in the dialog box.
//
//*************************************************************************
#define POS_SLIDER_MIN (-16500)
#define POS_SLIDER_MAX 16500
#define TRIGGER_LEVEL_SLIDER_MIN (-16500)
#define TRIGGER_LEVEL_SLIDER_MAX 16500
#define TRIGGER_POS_SLIDER_MIN (-60)
#define TRIGGER_POS_SLIDER_MAX 60

//*************************************************************************
//
// The dialog box background color.
//
//*************************************************************************
#define SCOPE_COLOR_DLG_BACKGROUND RGB(236, 233, 216)

//*************************************************************************
//
// A minor variation on a status bar that lets us set the text from a
// resource ID rather than a string pointer.
//
//*************************************************************************
class CStatusBarExt : public CStatusBar
{
public:
    CStatusBarExt();
    BOOL SetPaneTextByResource(int nIndex, int iStringID, BOOL bUpdate = TRUE);
};

//*************************************************************************
//
// A minor variation on the basic combo box that lets us set the selection
// based on the item value rather than string.
//
//*************************************************************************
class CComboBoxExt : public CComboBox
{
public:
    CComboBoxExt();
    int SetCurSelByValue(DWORD dwValue);
};

//*************************************************************************
//
// ClmscopeDlg dialog
//
//*************************************************************************
class ClmscopeDlg : public CDialog
{
// Construction
public:
    ClmscopeDlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
    enum { IDD = IDD_LMSCOPE_DIALOG };

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
    DECLARE_MESSAGE_MAP()
protected:
    void UpdateControlEnables();
    void ScaleAndFormatString(CString *pcString, LPCTSTR pcSuffix,
                              LPCTSTR pcUnit, LPCTSTR pcUnit1000,
                              long lValue);
    void SetControlsOnConnection(tScopeSettings *pSettings);
    void InitComboBoxContents();
    void UpdateVoltageMeasurements();
protected:
    CBrush m_cBlackBrush;
    CBrush m_cGreyBrush;
    CBrush m_cYellowBrush;
    CBrush m_cVioletBrush;
    CStatusBarExt m_csbStatusBar;
    BOOL m_bConnected;
    BOOL m_bStarted;
    BOOL m_bSaveItemsEnabled;
    BOOL m_bPingResponseReceived;
    BOOL m_bReconnecting;
    int m_iPingCount;
    int m_iPingResponse;
    tScopeDataStart *m_psScopeData;
    int m_iSampleOffset;
public:
    afx_msg void OnBnClickedStartStop();
public:
    afx_msg void OnBnClickedCapture();
public:
    afx_msg void OnBnClickedChannel2Find();
public:
    afx_msg void OnBnClickedChannel1Find();
public:
    afx_msg void OnBnClickedEnableCh2();
public:
    afx_msg void OnCbnSelchangeChannel1Scale();
public:
    afx_msg void OnCbnSelchangeChannel2Scale();
public:
    afx_msg void OnCbnSelchangeTriggerType();
public:
    afx_msg void OnCbnSelchangeTimebase();
public:
    afx_msg void OnBnClickedTriggerCh1();
public:
    afx_msg void OnBnClickedTriggerCh2();
public:
    afx_msg LRESULT OnScopeNoDriver(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeDeviceAvailable(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeDeviceConnected(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeDeviceDisconnected(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeData(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopePingResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeStarted(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeStopped(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeTimebaseChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeTriggerLevelChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeTriggerTypeChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeTriggerPosChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeChannel2(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopePositionChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScopeScaleChanged(WPARAM wParam, LPARAM lParam);
public:
    CSliderCtrl m_cChannel1PosSlider;
public:
    CSliderCtrl m_cChannel2PosSlider;
public:
    CSliderCtrl m_cTriggerLevelSlider;
public:
    CSliderCtrl m_cTriggerPosSlider;
public:
    afx_msg void OnBnClickedOk();
public:
    // Control variable for the Channel1 scale combo box
    CComboBoxExt m_cChannel1Scale;
public:
    // Control variable for the Channel2Scale combo box
    CComboBoxExt m_cChannel2Scale;
public:
    // Control variable for the Channel 2 enable checkbox.
    CButton m_cChannel2Enable;
public:
    // Control variable for the Find Channel 1 button
    CButton m_cFindChannel1;
public:
    // Control variable for the Find Channel 2 button
    CButton m_cFindChannel2;
public:
    // Control variable for the Timebase combo box.
    CComboBoxExt m_cTimebase;
public:
    // Control variable for the One Shot Trigger button
    CButton m_cOneShot;
public:
    // Control Variable for the Channel 1 position text
    CStatic m_cChannel1Pos;
public:
    // Control Variable for the Channel 2 position text
    CStatic m_cChannel2Pos;
public:
    // Control Variable for the Stop/Start button
    CButton m_cStopStart;
public:
    // Control Variable for the trigger type combo box
    CComboBoxExt m_cTriggerType;
public:
    // Control variable for the Channel1 minimum voltage text field
    CStatic m_cCh1Min;
public:
    // Control variable for the Channel1 maximum voltage text field
    CStatic m_cCh1Max;
public:
    // Control variable for the Channel1 mean voltage text field
    CStatic m_cCh1Mean;
public:
    // Control variable for the Channel2 minimum voltage text field
    CStatic m_cCh2Min;
public:
    // Control variable for the Channel2 maximum voltage text field
    CStatic m_cCh2Max;
public:
    // Control variable for the Channel2 mean voltage text field
    CStatic m_cCh2Mean;
public:
    // Control variable for Channel 1 trigger radio button
    CButton m_cTriggerCh1;
public:
    // Control variable for Channel 2 trigger radio button
    CButton m_cTriggerCh2;
public:
    // Control variable for the trigger level text field
    CStatic m_cTriggerLevel;
public:
    // Value variable for the trigger level text field.
    CString m_TriggerLevel;
public:
    // Value variable for the channel 2 position text field.
    CString m_Channel2Pos;
public:
    // Value variable for the channel 1 position text field.
    CString m_Channel1Pos;
public:
    // Value variable for the Channel 1 minimum voltage text field.
    CString m_Ch1Min;
public:
    // Value variable for the Channel 1 maximum voltage text field.
    CString m_Ch1Max;
public:
    // Value variable for the Channel 1 mean voltage text field.
    CString m_Ch1Mean;
public:
    // Value variable for the Channel 2 minimum voltage text field.
    CString m_Ch2Min;
public:
    // Value variable for the Channel 2 maximum voltage text field.
    CString m_Ch2Max;
public:
    // Value variable for the Channel 2 mean voltage text field.
    CString m_Ch2Mean;
public:
    // Control variable for the waveform display area
    CWaveformDisplay m_cWaveform;
public:
    afx_msg void OnTimer(UINT_PTR nIDEvent);
public:
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
public:
    afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
public:
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};
