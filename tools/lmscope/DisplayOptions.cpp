//***************************************************************************
//
// DisplayOptions.cpp : implementation file
//
//***************************************************************************
#include "stdafx.h"
#define PACKED
#pragma pack(1)
#include "usb_protocol.h"
#pragma pack()
#include "scope_control.h"
#include "lmscope.h"
#include "WaveformDisplay.h"
#include "DisplayOptions.h"

// CDisplayOptions dialog

IMPLEMENT_DYNAMIC(CDisplayOptions, CDialog)

CDisplayOptions::CDisplayOptions(CWnd* pParent /*=NULL*/)
    : CDialog(CDisplayOptions::IDD, pParent)
{

}

CDisplayOptions::~CDisplayOptions()
{
}

void CDisplayOptions::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SHOW_GRATICULE, m_cGraticule);
    DDX_Control(pDX, IDC_SHOW_TRIG_LEVEL, m_cTrigLevel);
    DDX_Control(pDX, IDC_SHOW_TRIG_POS, m_cTrigPos);
    DDX_Control(pDX, IDC_SHOW_GROUND, m_cGround);
}


BEGIN_MESSAGE_MAP(CDisplayOptions, CDialog)
    ON_BN_CLICKED(IDOK, &CDisplayOptions::OnBnClickedOk)
    ON_BN_CLICKED(IDC_SHOW_TRIG_LEVEL, &CDisplayOptions::OnBnClickedShowTrigLevel)
    ON_BN_CLICKED(IDC_SHOW_TRIG_POS, &CDisplayOptions::OnBnClickedShowTrigPos)
    ON_BN_CLICKED(IDC_SHOW_GROUND, &CDisplayOptions::OnBnClickedShowGround)
    ON_BN_CLICKED(IDC_SHOW_GRATICULE, &CDisplayOptions::OnBnClickedShowGraticule)
END_MESSAGE_MAP()

//***************************************************************************
//
// Dialog initialization.
//
//***************************************************************************
BOOL CDisplayOptions::OnInitDialog(void)
{
    CDialog::OnInitDialog();

    if(m_pWaveform)
    {
        m_cGraticule.SetCheck(m_pWaveform->IsGraticuleShown());
        m_cGround.SetCheck(m_pWaveform->IsGroundShown());
        m_cTrigLevel.SetCheck(m_pWaveform->IsTriggerLevelShown());
        m_cTrigPos.SetCheck(m_pWaveform->IsTriggerPosShown());
    }

    return(TRUE);
}

//***************************************************************************
//***************************************************************************
//
// CDisplayOptions message handlers
//
//***************************************************************************
//***************************************************************************

//***************************************************************************
//
// Dismiss the dialog box with the OK button.
//
//***************************************************************************
void CDisplayOptions::OnBnClickedOk()
{
    OnOK();
}

//***************************************************************************
//
// Handle the "Show Trigger Level" check box.
//
//***************************************************************************
void CDisplayOptions::OnBnClickedShowTrigLevel()
{
    if(m_pWaveform)
    {
        m_pWaveform->ShowTriggerLevel(m_cTrigLevel.GetCheck());
    }
}

//***************************************************************************
//
// Handle the "Show Trigger Position" check box.
//
//***************************************************************************
void CDisplayOptions::OnBnClickedShowTrigPos()
{
    if(m_pWaveform)
    {
        m_pWaveform->ShowTriggerPos(m_cTrigPos.GetCheck());
    }
}

//***************************************************************************
//
// Handle the "Show Ground" check box.
//
//***************************************************************************
void CDisplayOptions::OnBnClickedShowGround()
{
    if(m_pWaveform)
    {
        m_pWaveform->ShowGround(m_cGround.GetCheck());
    }
}

//***************************************************************************
//
// Handle the "Show Graticule" check box.
//
//***************************************************************************
void CDisplayOptions::OnBnClickedShowGraticule()
{
    if(m_pWaveform)
    {
        m_pWaveform->ShowGraticule(m_cGraticule.GetCheck());
    }
}

//***************************************************************************
//
// Receive the CWaveformDisplay pointer from the client to allow display
// control.
//
//***************************************************************************
void CDisplayOptions::SelectWaveform(CWaveformDisplay *pWaveform)
{
    m_pWaveform = pWaveform;
}
