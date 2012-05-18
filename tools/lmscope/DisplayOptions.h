//***************************************************************************
//
// Class header for the lmscope Display Options dialog box.
//
//***************************************************************************
#pragma once
#include "afxwin.h"


// CDisplayOptions dialog

class CDisplayOptions : public CDialog
{
	DECLARE_DYNAMIC(CDisplayOptions)

public:
	CDisplayOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDisplayOptions();

// Dialog Data
	enum { IDD = IDD_DISPLAY_OPTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
public:
    afx_msg void OnBnClickedShowTrigLevel();
public:
    afx_msg void OnBnClickedShowTrigPos();
public:
    afx_msg void OnBnClickedShowGround();
public:
    void SelectWaveform(CWaveformDisplay *pWaveform);
protected:
    CWaveformDisplay *m_pWaveform;
public:
    CButton m_cGraticule;
public:
    CButton m_cTrigLevel;
public:
    CButton m_cTrigPos;
public:
    CButton m_cGround;
public:
    afx_msg void OnBnClickedShowGraticule();
};
