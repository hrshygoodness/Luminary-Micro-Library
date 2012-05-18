//***************************************************************************
//
// lmscopeDlg.cpp : implementation file
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
#include "lmscopeDlg.h"
#include "DisplayOptions.h"
#include "hlp\ctrlhlp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//***************************************************************************
//
// Control ID to help topic ID mappings.
//
//***************************************************************************
static DWORD g_pdwHelpIDs[] =
{
    IDC_CHANNEL1_FIND,      HIDC_CHANNEL1_FIND,
    IDC_CHANNEL2_FIND,      HIDC_CHANNEL2_FIND,
    IDC_CHANNEL1_SCALE,     HIDC_CHANNEL1_SCALE,
    IDC_CHANNEL2_SCALE,     HIDC_CHANNEL2_SCALE,
    IDC_CHANNEL1_POS,       HIDC_CHANNEL1_POS,
    IDC_CHANNEL2_POS,       HIDC_CHANNEL2_POS,
    IDC_TRIGGER_TYPE,       HIDC_TRIGGER_TYPE,
    IDC_ENABLE_CH2,         HIDC_ENABLE_CH2,
    IDC_TRIGGER_CH1,        HIDC_TRIGGER_CH1,
    IDC_TRIGGER_CH2,        HIDC_TRIGGER_CH2,
    IDC_START_STOP,         HIDC_START_STOP,
    IDC_CAPTURE,            HIDC_CAPTURE,
    IDC_TRIGGER_LEVEL,      HIDC_TRIGGER_LEVEL,
    IDC_TIMEBASE,           HIDC_TIMEBASE,
    IDC_TRIGGER_POS,        HIDC_TRIGGER_POS,
    IDC_WAVEFORM,           HIDC_WAVEFORM,
    IDOK,                   HIDOK,
    IDC_CH1_MIN,            HIDC_CH1_MIN,
    IDC_CH1_MAX,            HIDC_CH1_MAX,
    IDC_CH1_MEAN,           HIDC_CH1_MEAN,
    IDC_CH2_MIN,            HIDC_CH2_MIN,
    IDC_CH2_MAX,            HIDC_CH2_MAX,
    IDC_CH2_MEAN,           HIDC_CH2_MEAN,
    //
    // Ids for which there is no context-sensitive help.
    //
    IDC_CH1_TEXT1,          -1,
    IDC_CH1_TEXT2,          -1,
    IDC_CH2_TEXT1,          -1,
    IDC_CH2_TEXT2,          -1,
    IDC_CHANNEL1_POS_TEXT,  -1,
    IDC_CHANNEL2_POS_TEXT,  -1,
    IDC_TRIGGER_LEVEL_TEXT, -1,
    IDC_STATIC,             -1,
    0,                      -1,
    0,0
};

//***************************************************************************
//
// A number larger than the largest control ID that we want to show context
// sensitive help for. This helps guard against error message boxes if people
// right click on the application status bar.
//
//***************************************************************************
#define CTRL_ID_MAX 5000

//***************************************************************************
//
// Initialization values for the voltage scale combo boxes.
//
//***************************************************************************
const DWORD g_pdwVoltages[] =
{
   100, 200, 500, 1000, 2000, 5000, 10000
};

//***************************************************************************
//
// Initialization values for the timebase combo box.
//
//***************************************************************************
const DWORD g_pdwTimebases[] =
{
    2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000
};

//***************************************************************************
//
// Ping timer parameters. When connected, we ping the device every 2 seconds
// to ensure that the connection is still active.
//
//***************************************************************************
#define PING_TIMER 1
#define PING_FREQUENCY_MS 2000

//***************************************************************************
//
// Macro to round a number to the nearest multiple of another number.
//
//***************************************************************************
#define ROUND(num, mult) ((((num) + ((mult) / 2)) / (mult)) * (mult))

typedef struct
{
    int iStringID;
    DWORD dwValue;
} tComboEntry;

//***************************************************************************
//
// Initialization values for the trigger type combo box.
//
//***************************************************************************
tComboEntry g_psTriggers[] =
{
    {IDS_RISING, SCOPE_TRIGGER_TYPE_RISING},
    {IDS_FALLING, SCOPE_TRIGGER_TYPE_FALLING},
    {IDS_LEVEL, SCOPE_TRIGGER_TYPE_LEVEL},
    {IDS_ALWAYS, SCOPE_TRIGGER_TYPE_ALWAYS}
};

//
// Macro to reverse the sense of a slider value. Windows insists that vertical
// sliders have their minimum value at the top position and this is not well
// suited to our model.
//
#define REVERSE_SLIDER(pos, max, min) ((max) - ((pos) - (min)))

static UINT BASED_CODE indicators[] =
{
    ID_INDICATOR_STATUS
};

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    enum { IDD = IDD_ABOUTBOX };

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// ClmscopeDlg dialog




ClmscopeDlg::ClmscopeDlg(CWnd* pParent /*=NULL*/)
    : CDialog(ClmscopeDlg::IDD, pParent)
    , m_TriggerLevel(_T(""))
    , m_Channel2Pos(_T(""))
    , m_Channel1Pos(_T(""))
    , m_Ch1Min(_T(""))
    , m_Ch1Max(_T(""))
    , m_Ch1Mean(_T(""))
    , m_Ch2Min(_T(""))
    , m_Ch2Max(_T(""))
    , m_Ch2Mean(_T(""))
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void ClmscopeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CHANNEL1_POS, m_cChannel1PosSlider);
    DDX_Control(pDX, IDC_CHANNEL2_POS, m_cChannel2PosSlider);
    DDX_Control(pDX, IDC_TRIGGER_LEVEL, m_cTriggerLevelSlider);
    DDX_Control(pDX, IDC_TRIGGER_POS, m_cTriggerPosSlider);
    DDX_Control(pDX, IDC_CHANNEL1_SCALE, m_cChannel1Scale);
    DDX_Control(pDX, IDC_CHANNEL2_SCALE, m_cChannel2Scale);
    DDX_Control(pDX, IDC_ENABLE_CH2, m_cChannel2Enable);
    DDX_Control(pDX, IDC_CHANNEL1_FIND, m_cFindChannel1);
    DDX_Control(pDX, IDC_CHANNEL2_FIND, m_cFindChannel2);
    DDX_Control(pDX, IDC_TIMEBASE, m_cTimebase);
    DDX_Control(pDX, IDC_CAPTURE, m_cOneShot);
    DDX_Control(pDX, IDC_CHANNEL1_POS_TEXT, m_cChannel1Pos);
    DDX_Control(pDX, IDC_CHANNEL2_POS_TEXT, m_cChannel2Pos);
    DDX_Control(pDX, IDC_START_STOP, m_cStopStart);
    DDX_Control(pDX, IDC_TRIGGER_TYPE, m_cTriggerType);
    DDX_Control(pDX, IDC_CH1_MIN, m_cCh1Min);
    DDX_Control(pDX, IDC_CH1_MAX, m_cCh1Max);
    DDX_Control(pDX, IDC_CH1_MEAN, m_cCh1Mean);
    DDX_Control(pDX, IDC_CH2_MIN, m_cCh2Min);
    DDX_Control(pDX, IDC_CH2_MAX, m_cCh2Max);
    DDX_Control(pDX, IDC_CH2_MEAN, m_cCh2Mean);
    DDX_Control(pDX, IDC_TRIGGER_CH1, m_cTriggerCh1);
    DDX_Control(pDX, IDC_TRIGGER_CH2, m_cTriggerCh2);
    DDX_Control(pDX, IDC_TRIGGER_LEVEL_TEXT, m_cTriggerLevel);
    DDX_Text(pDX, IDC_TRIGGER_LEVEL_TEXT, m_TriggerLevel);
    DDX_Text(pDX, IDC_CHANNEL2_POS_TEXT, m_Channel2Pos);
    DDX_Text(pDX, IDC_CHANNEL1_POS_TEXT, m_Channel1Pos);
    DDX_Text(pDX, IDC_CH1_MIN, m_Ch1Min);
    DDX_Text(pDX, IDC_CH1_MAX, m_Ch1Max);
    DDX_Text(pDX, IDC_CH1_MEAN, m_Ch1Mean);
    DDX_Text(pDX, IDC_CH2_MIN, m_Ch2Min);
    DDX_Text(pDX, IDC_CH2_MAX, m_Ch2Max);
    DDX_Text(pDX, IDC_CH2_MEAN, m_Ch2Mean);
    DDX_Control(pDX, IDC_WAVEFORM, m_cWaveform);
}

BEGIN_MESSAGE_MAP(ClmscopeDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_START_STOP, &ClmscopeDlg::OnBnClickedStartStop)
    ON_BN_CLICKED(IDC_CAPTURE, &ClmscopeDlg::OnBnClickedCapture)
    ON_BN_CLICKED(IDC_CHANNEL2_FIND, &ClmscopeDlg::OnBnClickedChannel2Find)
    ON_BN_CLICKED(IDC_CHANNEL1_FIND, &ClmscopeDlg::OnBnClickedChannel1Find)
    ON_BN_CLICKED(IDC_ENABLE_CH2, &ClmscopeDlg::OnBnClickedEnableCh2)
    ON_CBN_SELCHANGE(IDC_CHANNEL1_SCALE, &ClmscopeDlg::OnCbnSelchangeChannel1Scale)
    ON_CBN_SELCHANGE(IDC_CHANNEL2_SCALE, &ClmscopeDlg::OnCbnSelchangeChannel2Scale)
    ON_CBN_SELCHANGE(IDC_TRIGGER_TYPE, &ClmscopeDlg::OnCbnSelchangeTriggerType)
    ON_CBN_SELCHANGE(IDC_TIMEBASE, &ClmscopeDlg::OnCbnSelchangeTimebase)
    ON_BN_CLICKED(IDC_TRIGGER_CH1, &ClmscopeDlg::OnBnClickedTriggerCh1)
    ON_BN_CLICKED(IDC_TRIGGER_CH2, &ClmscopeDlg::OnBnClickedTriggerCh2)
    ON_MESSAGE(WM_SCOPE_NO_DRIVER, &ClmscopeDlg::OnScopeNoDriver)
    ON_MESSAGE(WM_SCOPE_DEVICE_AVAILABLE, &ClmscopeDlg::OnScopeDeviceAvailable)
    ON_MESSAGE(WM_SCOPE_DEVICE_CONNECTED, &ClmscopeDlg::OnScopeDeviceConnected)
    ON_MESSAGE(WM_SCOPE_DEVICE_DISCONNECTED, &ClmscopeDlg::OnScopeDeviceDisconnected)
    ON_MESSAGE(WM_SCOPE_DATA, &ClmscopeDlg::OnScopeData)
    ON_MESSAGE(WM_SCOPE_PING_RESPONSE, &ClmscopeDlg::OnScopePingResponse)
    ON_MESSAGE(WM_SCOPE_STARTED, &ClmscopeDlg::OnScopeStarted)
    ON_MESSAGE(WM_SCOPE_STOPPED, &ClmscopeDlg::OnScopeStopped)
    ON_MESSAGE(WM_SCOPE_TIMEBASE_CHANGED, &ClmscopeDlg::OnScopeTimebaseChanged)
    ON_MESSAGE(WM_SCOPE_TRIGGER_LEVEL_CHANGED, &ClmscopeDlg::OnScopeTriggerLevelChanged)
    ON_MESSAGE(WM_SCOPE_TRIGGER_TYPE_CHANGED, &ClmscopeDlg::OnScopeTriggerTypeChanged)
    ON_MESSAGE(WM_SCOPE_TRIGGER_POS_CHANGED, &ClmscopeDlg::OnScopeTriggerPosChanged)
    ON_MESSAGE(WM_SCOPE_CHANNEL2, &ClmscopeDlg::OnScopeChannel2)
    ON_MESSAGE(WM_SCOPE_POS_CHANGED, &ClmscopeDlg::OnScopePositionChanged)
    ON_MESSAGE(WM_SCOPE_SCALE_CHANGED, &ClmscopeDlg::OnScopeScaleChanged)
    ON_BN_CLICKED(IDOK, &ClmscopeDlg::OnBnClickedOk)
    ON_WM_TIMER()
    ON_WM_CTLCOLOR()
    ON_WM_CONTEXTMENU()
    ON_WM_HELPINFO()
END_MESSAGE_MAP()


// ClmscopeDlg message handlers

//***************************************************************************
//
// Called during dialog initialization.
//
//***************************************************************************
BOOL ClmscopeDlg::OnInitDialog()
{
    CRect rect;
    bool bRetcode;

    CDialog::OnInitDialog();

    //
    // Check that all menu IDs are valid for use in the system menu.
    //
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    ASSERT((IDM_SAVEASBMP & 0xFFF0) == IDM_SAVEASBMP);
    ASSERT(IDM_SAVEASBMP < 0xF000);
    ASSERT((IDM_SAVEASCSV & 0xFFF0) == IDM_SAVEASCSV);
    ASSERT(IDM_SAVEASCSV < 0xF000);
    ASSERT((IDM_DISPLAYOPTS & 0xFFF0) == IDM_DISPLAYOPTS);
    ASSERT(IDM_DISPLAYOPTS < 0xF000);
    ASSERT((IDM_SHOWHELP & 0xFFF0) == IDM_SHOWHELP);
    ASSERT(IDM_SHOWHELP < 0xF000);

    //
    // Remember that the "Save As" menu entries are disabled.
    //
    m_bSaveItemsEnabled = FALSE;

    //
    // Clear our keep-alive counters.
    //
    m_iPingResponse = 0;
    m_iPingCount = 0;
    m_bPingResponseReceived = TRUE;
    m_bReconnecting = FALSE;

    //
    // Add our entries to the system menu.
    //
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strEntry;

        //
        // Add a separator to the bottom of the existing menu.
        //
        pSysMenu->AppendMenu(MF_SEPARATOR);

        //
        // Add the "Save as bitmap" menu item.
        //
        strEntry.LoadString(IDS_SAVEASBMP);
        if(!strEntry.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_STRING, IDM_SAVEASBMP, strEntry);
            pSysMenu->EnableMenuItem(IDM_SAVEASBMP, MF_GRAYED);
        }

        //
        // Add the "Save as spreadsheet" menu item.
        //
        strEntry.LoadString(IDS_SAVEASCSV);
        if(!strEntry.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_STRING, IDM_SAVEASCSV, strEntry);
            pSysMenu->EnableMenuItem(IDM_SAVEASCSV, MF_GRAYED);
        }

        //
        // Add the "Display options" menu item.
        //
        strEntry.LoadString(IDS_DISPLAY_OPTS);
        if(!strEntry.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_STRING, IDM_DISPLAYOPTS, strEntry);
        }

        //
        // Add a separator to the menu.
        //
        pSysMenu->AppendMenu(MF_SEPARATOR);
        
        //
        // Add the "LMScope Help..." menu item.
        //
        strEntry.LoadString(IDS_SHOWHELP);
        if (!strEntry.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_STRING, IDM_SHOWHELP, strEntry);
        }

        //
        // Add the "About" menu item.
        //
        strEntry.LoadString(IDS_ABOUTBOX);
        if (!strEntry.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strEntry);
        }
    }

    //
    // Initialize the two brushes we use for control background colors.
    //
    m_cBlackBrush.CreateSolidBrush(SCOPE_COLOR_BACKGROUND);
    m_cGreyBrush.CreateSolidBrush(SCOPE_COLOR_DLG_BACKGROUND);
    m_cYellowBrush.CreateSolidBrush(SCOPE_COLOR_CHANNEL_1);
    m_cVioletBrush.CreateSolidBrush(SCOPE_COLOR_CHANNEL_2);

    //
    // Set the icon for this dialog.  The framework does this automatically
    // when the application's main window is not a dialog.
    //
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    //
    // Initialize status bar
    //
    m_csbStatusBar.Create(this); //We create the status bar

    m_csbStatusBar.SetIndicators(indicators,1); //Set the number of panes

    GetClientRect(&rect);

    //
    //Size the status bar to the width of the window
    //
    m_csbStatusBar.SetPaneInfo(0,ID_INDICATOR_STATUS,
                               SBPS_NORMAL,rect.Width());

    //
    // Draw the status bar on the screen.
    //
    RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,
    ID_INDICATOR_STATUS);

    //
    // Set the background color
    //
    m_csbStatusBar.GetStatusBarCtrl().SetBkColor(SCOPE_COLOR_DLG_BACKGROUND);

    //
    // Set the initial status text.
    //
    m_csbStatusBar.SetPaneTextByResource(0, IDS_STATUS_SEARCHING, TRUE);

    //
    // Remember that we are not connected to the scope yet.
    //
    m_bConnected = FALSE;

    //
    // Set the ranges of the various sliders on the dialog.
    //
    m_cChannel1PosSlider.SetRange(POS_SLIDER_MIN, POS_SLIDER_MAX, FALSE);
    m_cChannel2PosSlider.SetRange(POS_SLIDER_MIN, POS_SLIDER_MAX, FALSE);
    m_cTriggerPosSlider.SetRange(TRIGGER_POS_SLIDER_MIN, TRIGGER_POS_SLIDER_MAX,
                                 FALSE);
    m_cTriggerLevelSlider.SetRange(TRIGGER_LEVEL_SLIDER_MIN,
                                   TRIGGER_LEVEL_SLIDER_MAX, FALSE);

    m_cTriggerPosSlider.SetPos(
             (TRIGGER_POS_SLIDER_MAX - TRIGGER_POS_SLIDER_MIN) / 2);
    m_cChannel1PosSlider.SetPos((POS_SLIDER_MAX - POS_SLIDER_MIN) / 2);
    m_cChannel2PosSlider.SetPos((POS_SLIDER_MAX - POS_SLIDER_MIN) / 2);
    m_cTriggerLevelSlider.SetPos(
             (TRIGGER_LEVEL_SLIDER_MAX - TRIGGER_LEVEL_SLIDER_MIN) / 2);

    //
    // Set the choices available in the various comboboxes
    //
    InitComboBoxContents();

    //
    // Update the enable/disable state of the various controls
    //
    UpdateControlEnables();

    //
    // Initialize the waveform display custom control.
    //
    m_cWaveform.InitBitmap();

    //
    // Initialize the scope control module
    //
    bRetcode = ScopeControlInit(m_hWnd);

    return TRUE;  // return TRUE unless you set the focus to a control
}

//
// Handle the commands we added to the system menu.
//
void ClmscopeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    switch(nID & 0xFFF0)
    {
        case IDM_ABOUTBOX:
        {
            CAboutDlg dlgAbout;
            dlgAbout.DoModal();
        }
        break;

        case IDM_DISPLAYOPTS:
        {
                CDisplayOptions dlgDisplay;
                dlgDisplay.SelectWaveform(&m_cWaveform);
                dlgDisplay.DoModal();
        }
        break;

        case IDM_SAVEASBMP:
        {
            CString strFileName;
            INT_PTR iRet;
            CFileDialog FileDlg(FALSE, NULL, NULL, OFN_OVERWRITEPROMPT,
                                L"Bitmaps (*.bmp)|*.bmp||");

            iRet = FileDlg.DoModal();

            strFileName = FileDlg.GetPathName();

            if(iRet == IDOK)
            {
                m_cWaveform.SaveAsBMP(&strFileName);
            }
        }
        break;

        case IDM_SAVEASCSV:
        {
            CString strFileName;
            INT_PTR iRet;
            CFileDialog FileDlg(FALSE, NULL, NULL, OFN_OVERWRITEPROMPT,
                                L"Comma Separated Values (*.csv)|*.csv||");

            iRet = FileDlg.DoModal();

            strFileName = FileDlg.GetPathName();

            if(iRet == IDOK)
            {
                m_cWaveform.SaveAsCSV(&strFileName);
            }
        }
        break;

        case IDM_SHOWHELP:
        {
            CWinApp *theApp = AfxGetApp();
            CString strHelpFile = theApp->m_pszHelpFilePath;

            ::HtmlHelp(::GetDesktopWindow(), (LPCWSTR)strHelpFile,
                   HH_DISPLAY_TOPIC, NULL);
        }
        break;

        default:
        {
            CDialog::OnSysCommand(nID, lParam);
        }
        break;
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void ClmscopeDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR ClmscopeDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// The system calls this function when any horizontal scroll bar or slider is
// moved.
void ClmscopeDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
    //
    // We only have 1 horizontal slider in this application so this must be
    // the trigger position. Update the trigger position if this notification
    // related to a new position.
    //
    if(nSBCode != SB_ENDSCROLL)
    {
        ScopeControlSetTriggerPos(nPos);
    }
}

// The system calls this function when any vertical scroll bar or slider is
// moved.
void ClmscopeDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
    CSliderCtrl *pSlider = (CSliderCtrl *)pScrollBar;
    long lPos;

    //
    // Ignore ENDSCROLL notifications.
    //
    if(nSBCode == SB_ENDSCROLL)
    {
        return;
    }

    //
    // We have several vertical sliders in the application so we need to
    // determine which one generated this message and handle it
    // appropriately.
    //
    if(pSlider->m_hWnd == m_cTriggerLevelSlider.m_hWnd)
    {
        //
        // Update the trigger level.
        //

        lPos = REVERSE_SLIDER(nPos, TRIGGER_LEVEL_SLIDER_MAX,
                             TRIGGER_LEVEL_SLIDER_MIN);
        lPos = ROUND(lPos, 100);
        ScopeControlSetTriggerLevel(lPos);
    }

    if(pScrollBar->m_hWnd == m_cChannel1PosSlider.m_hWnd)
    {
        //
        // Update the channel 1 position
        //
        lPos = REVERSE_SLIDER(nPos, POS_SLIDER_MAX, POS_SLIDER_MIN);
        lPos = ROUND(lPos, 100);
        ScopeControlSetPosition(SCOPE_CHANNEL_1, lPos);
    }

    if(pScrollBar->m_hWnd == m_cChannel2PosSlider.m_hWnd)
    {
        //
        // Update the channel 2 position
        //
        lPos = REVERSE_SLIDER(nPos, POS_SLIDER_MAX, POS_SLIDER_MIN);
        lPos = ROUND(lPos, 100);
        ScopeControlSetPosition(SCOPE_CHANNEL_2, lPos);
    }
}


//***************************************************************************
//
// Start or stop automatic capture of oscilloscope waveforms.
//
//***************************************************************************
void ClmscopeDlg::OnBnClickedStartStop()
{
    //
    // Ask the scope control module to either start or stop automatic capture.
    //
    ScopeControlStartStop(!m_bStarted);
}

//***************************************************************************
//
// Perform a 1-shot capture
//
//***************************************************************************
void ClmscopeDlg::OnBnClickedCapture()
{
    //
    // Ask the scope control module to capture a single waveform.
    //
    ScopeControlCapture();
}

//***************************************************************************
//
// Find the channel 2 signal, reposition and rescale to make it appear
// visible on the screen.
//
//***************************************************************************
void ClmscopeDlg::OnBnClickedChannel2Find()
{
    // Ask the scope to find the channel 2 waveform for us.
    ScopeControlFind(SCOPE_CHANNEL_2);
}

//***************************************************************************
//
// Find the channel 1 signal, reposition and rescale to make it appear
// visible on the screen.
//
//***************************************************************************
void ClmscopeDlg::OnBnClickedChannel1Find()
{
    // Ask the scope to find the channel 1 waveform for us.
    ScopeControlFind(SCOPE_CHANNEL_1);
}

//***************************************************************************
//
// Enable or disable channel 2 capture.
//
//***************************************************************************
void ClmscopeDlg::OnBnClickedEnableCh2()
{
    //
    // Is the button checked or not?
    //
    m_cChannel2Enable.GetCheck();

    //
    // Ask the scope control module to enable or disable channel 2.
    //
    ScopeControlEnableChannel2(m_cChannel2Enable.GetCheck() ? true : false);
}

//***************************************************************************
//
// The user has changed the channel 1 scale combo box selection.
//
//***************************************************************************
void ClmscopeDlg::OnCbnSelchangeChannel1Scale()
{
    unsigned long ulValue;

    //
    // Get the trigger type control selection and ask the scope to change
    // to this setting.
    //
    ulValue = (unsigned long)m_cChannel1Scale.GetItemData(
                                        m_cChannel1Scale.GetCurSel());

    //
    // Ask the scope to change to the new setting.
    //
    ScopeControlSetScale(SCOPE_CHANNEL_1, ulValue);
}

//***************************************************************************
//
// The user has changed the channel 2 scale combo box selection.
//
//***************************************************************************
void ClmscopeDlg::OnCbnSelchangeChannel2Scale()
{
    unsigned long ulValue;

    //
    // Get the trigger type control selection and ask the scope to change
    // to this setting.
    //
    ulValue = (unsigned long)m_cChannel2Scale.GetItemData(
                                        m_cChannel2Scale.GetCurSel());

    //
    // Ask the scope to change to the new setting.
    //
    ScopeControlSetScale(SCOPE_CHANNEL_2, ulValue);

}

//***************************************************************************
//
// The user has changed the trigger type combo box selection.
//
//***************************************************************************
void ClmscopeDlg::OnCbnSelchangeTriggerType()
{
    unsigned long ulValue;
    unsigned char ucChannel;

    //
    // Get the trigger type control selection and ask the scope to change
    // to this setting.
    //
    ulValue = (unsigned long)m_cTriggerType.GetItemData(
                                        m_cTriggerType.GetCurSel());
    ucChannel = m_cTriggerCh1.GetCheck() ? SCOPE_CHANNEL_1 : SCOPE_CHANNEL_2;
    ScopeControlSetTrigger(ucChannel, ulValue);
}

//***************************************************************************
//
// The user has changed the timebase combo box selection.
//
//***************************************************************************
void ClmscopeDlg::OnCbnSelchangeTimebase()
{
    unsigned long ulValue;

    //
    // Get the timebase control selection and ask the scope to change
    // to this setting.
    //
    ulValue = (unsigned long)m_cTimebase.GetItemData(m_cTimebase.GetCurSel());

    //
    // Pass on the change to the device.
    //
    ScopeControlSetTimebase(ulValue);
}

//***************************************************************************
//
// The user has requested triggering on channel 1.
//
//***************************************************************************
void ClmscopeDlg::OnBnClickedTriggerCh1()
{
    unsigned long ulValue;
    unsigned char ucChannel;

    //
    // Get the trigger type control selection and ask the scope to change
    // to this setting.
    //
    ulValue = (unsigned long)m_cTriggerType.GetItemData(
                                        m_cTriggerType.GetCurSel());
    ucChannel = m_cTriggerCh1.GetCheck() ? SCOPE_CHANNEL_1 : SCOPE_CHANNEL_2;
    ScopeControlSetTrigger(ucChannel, ulValue);
}

//***************************************************************************
//
// The user has requested triggering on channel 2.
//
//***************************************************************************
void ClmscopeDlg::OnBnClickedTriggerCh2()
{
    unsigned long ulValue;
    unsigned char ucChannel;

    //
    // Get the trigger type control selection and ask the scope to change
    // to this setting.
    //
    ulValue = (unsigned long)m_cTriggerType.GetItemData(
                                        m_cTriggerType.GetCurSel());
    ucChannel = m_cTriggerCh1.GetCheck() ? SCOPE_CHANNEL_1 : SCOPE_CHANNEL_2;
    ScopeControlSetTrigger(ucChannel, ulValue);
}

//***************************************************************************
//
// The scope control module is unable to find the device driver for the
// oscilloscope.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeNoDriver(WPARAM wParam, LPARAM lParam)
{
    m_csbStatusBar.SetPaneTextByResource(0, IDS_STATUS_NO_DRIVER, TRUE);
    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope device has been connected and is ready to start
// communication. We call ScopeControlConnect() to perform the HELLO
// handshake.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeDeviceAvailable(WPARAM wParam, LPARAM lParam)
{
    bool bRetcode;

    //
    // Update the status bar text since we now know the device is there.
    //
    m_csbStatusBar.SetPaneTextByResource(0, IDS_STATUS_FOUND, TRUE);

    //
    // Attempt to connect to the device.
    //
    bRetcode = ScopeControlConnect(NULL);

    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope device has responded to our ScopeControlConnect()
// call so we are now ready for business. We are passed a pointer to a
// tScopeSettings structure which we use to initialze the various user
// interface controls to match the current oscilloscope settings.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeDeviceConnected(WPARAM wParam, LPARAM lParam)
{
    tScopeSettings *psSettings = (tScopeSettings *)lParam;

    //
    // Set our flags to indicate that the device is connected.
    //
    m_bConnected = TRUE;
    m_bReconnecting = FALSE;

    //
    // Update the enable/disable state of the various controls
    //
    UpdateControlEnables();

    //
    // Update the status bar text since are now in full communication
    // with the oscilloscope.
    //
    m_csbStatusBar.SetPaneTextByResource(0, IDS_STATUS_CONNECTED, TRUE);

    //
    // Set the states of our various controls to match the remote
    // settings.
    //
    SetControlsOnConnection(psSettings);

    //
    // Free the memory passed to us in lParam
    //
    LocalFree((HLOCAL)psSettings);

    //
    // Ask the scope to send us data automatically.
    //
    ScopeControlAutomaticDataTransmission(true);

    //
    // Start our PING timer.
    //
    m_bPingResponseReceived = TRUE;
    SetTimer(PING_TIMER, PING_FREQUENCY_MS, 0);

    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope device has been disconnected.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeDeviceDisconnected(WPARAM wParam, LPARAM lParam)
{
    //
    // Set our flag to indicate that the device is not connected.
    //
    m_bConnected = FALSE;

    //
    // Update the enable/disable state of the various controls
    //
    UpdateControlEnables();

    //
    // Update the status bar text to indicate that the device has been
    // disconnected.
    //
    m_csbStatusBar.SetPaneTextByResource(0, IDS_STATUS_SEARCHING, TRUE);

    //
    // Stop our ping timer.
    //
    KillTimer(PING_TIMER);

    return((LRESULT)0);
}

//***************************************************************************
//
// This handler is called whenever the oscilloscope device has sent us a
// new waveform dataset.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeData(WPARAM wParam, LPARAM lParam)
{
    //
    // Note that the CWaveformDisplay control is considered to "own"
    // the dataset pointer so we don't free up the previous one
    // here. This is done inside m_cWaveform.RenderWaveform.
    //

    //
    // Keep hold of the new dataset.
    //
    m_iSampleOffset = (int)wParam;
    m_psScopeData = (tScopeDataStart *)lParam;

    //
    // Update the text showing min, max and mean voltages for each
    // channel.
    //
    UpdateVoltageMeasurements();

    //
    // Update waveform display with new data.
    //
    m_cWaveform.RenderWaveform(m_psScopeData, m_iSampleOffset);

    //
    // Now that we have data, we can enable the "Save As" menu options.
    //
    if(!m_bSaveItemsEnabled)
    {
        //
        // Get a handle to the system menu.
        //
        CMenu* pSysMenu = GetSystemMenu(FALSE);

        //
        // Did we get the handle?
        //
        if(pSysMenu)
        {
            //
            // Yes - enable both the "Save As" menu options.
            //
            pSysMenu->EnableMenuItem(IDM_SAVEASBMP, MF_ENABLED);
            pSysMenu->EnableMenuItem(IDM_SAVEASCSV, MF_ENABLED);
        }

        //
        // Remember that we already enabled the options.
        //
        m_bSaveItemsEnabled = TRUE;
    }

    return((LRESULT)0);
}

//***************************************************************************
//
// Handle ping responses from the oscilloscope device.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopePingResponse(WPARAM wParam, LPARAM lParam)
{
    //
    // Set the flag indicating that our last ping was responded to.
    //
    m_iPingResponse++;
    m_bPingResponseReceived = TRUE;
    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope is reporting that automated data capture has stopped.
// Update the UI to match.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeStopped(WPARAM wParam, LPARAM lParam)
{
    CString strStart;

    //
    // Remember which state we are in.
    //
    m_bStarted = FALSE;

    //
    // Get the "Start" string from our resources.
    //
    strStart.LoadString(IDS_START);

    //
    // Set the Stop/Start button text to indicate that the button will
    // now start capture rather than stop it.
    //
    m_cStopStart.SetWindowText(strStart);

    //
    // Enable the "One Shot Capture" button.
    //
    m_cOneShot.EnableWindow(TRUE);
    return((LRESULT)0);

}

//***************************************************************************
//
// The oscilloscope is reporting that automatic waveform capture has started.
// Update the UI to match.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeStarted(WPARAM wParam, LPARAM lParam)
{
    CString strStop;

    //
    // Remember which state we are in.
    //
    m_bStarted = TRUE;

    //
    // Get the "Stop" string from our resources.
    //
    strStop.LoadString(IDS_STOP);

    //
    // Set the Stop/Start button text to indicate that the button will
    // now stop capture rather than stop it.
    //
    m_cStopStart.SetWindowText(strStop);

    //
    // Disable the "One Shot Capture" button.
    //
    m_cOneShot.EnableWindow(FALSE);

    //
    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope is reporting that the timebase has been changed.
// Update the UI to match.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeTimebaseChanged(WPARAM wParam, LPARAM lParam)
{
    //
    // Update our combo box selection to match.
    //
    m_cTimebase.SetCurSelByValue((DWORD)lParam);

    //
    // Update the current waveform display with the new timebase.
    //
    m_cWaveform.SetTimebase((unsigned long)lParam);

    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope is reporting that the vertical scaling for a channel has
// been changed. Update the UI to match.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeScaleChanged(WPARAM wParam, LPARAM lParam)
{
    //
    // Update our combo box selection to match and redraw
    // the waveform.
    //
    if(wParam == SCOPE_CHANNEL_1)
    {
        m_cChannel1Scale.SetCurSelByValue((DWORD)lParam);
        m_cWaveform.SetChannelScale(CHANNEL_1, (int)lParam);
    }
    else
    {
        m_cChannel2Scale.SetCurSelByValue((DWORD)lParam);
        m_cWaveform.SetChannelScale(CHANNEL_2, (int)lParam);
    }

    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope is reporting that the vertical position of a channel has
// been changed. Update the UI to match.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopePositionChanged(WPARAM wParam, LPARAM lParam)
{
    CString strText;
    long lPos;

    //
    // Format the offset as a string
    //
    ScaleAndFormatString(&strText,  (LPCTSTR)L"", (LPCTSTR)L"mV",
                         (LPCTSTR)L"V", (long)lParam);

    //
    // We invert the slider relative to the value received since Windows
    // insists that vertical sliders have the maximum value at the bottom
    // rather than the top.
    //
    lPos = REVERSE_SLIDER((long)lParam, POS_SLIDER_MAX, POS_SLIDER_MIN);

    //
    // Update our slider and text to show the new position.
    //
    if(wParam == SCOPE_CHANNEL_1)
    {

        m_cChannel1PosSlider.SetPos((int)lPos);
        m_cChannel1Pos.SetWindowTextW(strText);

        //
        // Update the current waveform display with the new offset.
        //
        m_cWaveform.SetChannelPos(CHANNEL_1, (int)lParam);
    }
    else
    {
        m_cChannel2PosSlider.SetPos((int)lPos);
        m_cChannel2Pos.SetWindowTextW(strText);

        //
        // Update the current waveform display with the new offset.
        //
        m_cWaveform.SetChannelPos(CHANNEL_2, (int)lParam);
    }

    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope is reporting that the trigger level has been changed.
// Update the UI to match.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeTriggerLevelChanged(WPARAM wParam, LPARAM lParam)
{
    CString strText;
    long lPos;

    //
    // Set the trigger level.
    //
    ScaleAndFormatString(&strText,  (LPCTSTR)L"", (LPCTSTR)L"mV",
                         (LPCTSTR)L"V", (long)lParam);
    m_cTriggerLevel.SetWindowTextW(strText);
    lPos = REVERSE_SLIDER((long)lParam, TRIGGER_LEVEL_SLIDER_MAX,
                          TRIGGER_LEVEL_SLIDER_MIN);
    m_cTriggerLevelSlider.SetPos((int)lPos);

    //
    // Update the current waveform display with the new timebase.
    //
    m_cWaveform.SetTriggerLevel((long)lParam);

    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope is reporting that the trigger position has been changed.
// Update the UI to match.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeTriggerPosChanged(WPARAM wParam, LPARAM lParam)
{
    CString strText;

    //
    // Set the trigger position.
    //
    m_cTriggerPosSlider.SetPos((int)lParam);

    //
    // Update the current waveform display with the new trigger position.
    //
    m_cWaveform.SetTriggerPos((int)lParam);

    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope is reporting that the trigger level has been changed.
// Update the UI to match.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeTriggerTypeChanged(WPARAM wParam, LPARAM lParam)
{
    //
    // Set the trigger type.
    //
    m_cTriggerType.SetCurSelByValue((DWORD)lParam);

    //
    // Set the trigger channel radio button
    //
    if(wParam == SCOPE_CHANNEL_1)
    {
        m_cTriggerCh1.SetCheck(1);
        m_cTriggerCh2.SetCheck(0);
    }
    else
    {
        m_cTriggerCh2.SetCheck(1);
        m_cTriggerCh1.SetCheck(0);
    }

    return((LRESULT)0);
}

//***************************************************************************
//
// The oscilloscope is reporting that channel 2 capture has been enabled or
// disabled.
//
//***************************************************************************
LRESULT ClmscopeDlg::OnScopeChannel2(WPARAM wParam, LPARAM lParam)
{
    BOOL bEnabled;

    //
    // Was the channel enabled or disabled?
    //
    bEnabled = (wParam == SCOPE_CHANNEL2_ENABLE) ? TRUE : FALSE;

    //
    // Channel 2 has been enabled or disabled. Set the state of the
    // checkbox appropriately and also the radio button that allows selection
    // of channel 2 as the trigger source.
    //
    m_cChannel2Enable.SetCheck(bEnabled);
    m_cTriggerCh2.EnableWindow(bEnabled);
    m_cFindChannel2.EnableWindow(bEnabled);
    m_cChannel2Scale.EnableWindow(bEnabled);
    m_cChannel2PosSlider.EnableWindow(bEnabled);
    m_cChannel2Pos.EnableWindow(bEnabled);
    m_cChannel2Scale.EnableWindow(bEnabled);
    m_cCh2Min.EnableWindow(bEnabled);
    m_cCh2Max.EnableWindow(bEnabled);
    m_cCh2Mean.EnableWindow(bEnabled);

    return((LRESULT)0);
}

//***************************************************************************
//
// Enable or disable controls as required by the current connection state.
//
//***************************************************************************
void ClmscopeDlg::UpdateControlEnables(void)
{
    m_cChannel1PosSlider.EnableWindow(m_bConnected);
    m_cChannel2PosSlider.EnableWindow(m_bConnected);
    m_cTriggerLevelSlider.EnableWindow(m_bConnected);
    m_cTriggerPosSlider.EnableWindow(m_bConnected);
    m_cChannel1Scale.EnableWindow(m_bConnected);
    m_cChannel2Scale.EnableWindow(m_bConnected);
    m_cChannel2Enable.EnableWindow(m_bConnected);
    m_cFindChannel1.EnableWindow(m_bConnected);
    m_cFindChannel2.EnableWindow(m_bConnected);
    m_cTimebase.EnableWindow(m_bConnected);
    m_cOneShot.EnableWindow(m_bConnected);
    m_cChannel1Pos.EnableWindow(m_bConnected);
    m_cChannel2Pos.EnableWindow(m_bConnected);
    m_cStopStart.EnableWindow(m_bConnected);
    m_cTriggerType.EnableWindow(m_bConnected);
    m_cCh1Min.EnableWindow(m_bConnected);
    m_cCh1Max.EnableWindow(m_bConnected);
    m_cCh1Mean.EnableWindow(m_bConnected);
    m_cCh2Min.EnableWindow(m_bConnected);
    m_cCh2Max.EnableWindow(m_bConnected);
    m_cCh2Mean.EnableWindow(m_bConnected);
    m_cTriggerCh1.EnableWindow(m_bConnected);
    m_cTriggerCh2.EnableWindow(m_bConnected);
    m_cTriggerLevel.EnableWindow(m_bConnected);
}

//***************************************************************************
//
// Set the values of all the controls after we have connected and
// received a settings structure from the device.
//
//***************************************************************************
void ClmscopeDlg::SetControlsOnConnection(tScopeSettings *pSettings)
{
    CString strText;

    //
    // Set the state of the various channel 2 controls
    //
    OnScopeChannel2((pSettings->ucChannel2Enabled ? SCOPE_CHANNEL2_ENABLE :
                                                    SCOPE_CHANNEL2_DISABLE), 0);

    //
    // Set the trigger type.
    //
    OnScopeTriggerTypeChanged(pSettings->ucTriggerChannel, pSettings->ucTriggerType);

    //
    // Set the trigger level.
    //
    OnScopeTriggerLevelChanged(0, pSettings->ulTriggerLevelmV);

    //
    // Set the timebase.
    //
    OnScopeTimebaseChanged(0, pSettings->ulTimebaseuS);

    //
    // Set the channel 1 and 2 vertical scales.
    //
    OnScopeScaleChanged(SCOPE_CHANNEL_1, pSettings->usChannel1ScalemVdiv);
    OnScopeScaleChanged(SCOPE_CHANNEL_2, pSettings->usChannel2ScalemVdiv);

    //
    // Set the initial channel 1 and 2 vertical offset.
    //
    OnScopePositionChanged(SCOPE_CHANNEL_1, pSettings->sChannel1OffsetmV);
    OnScopePositionChanged(SCOPE_CHANNEL_2, pSettings->sChannel2OffsetmV);

    //
    // Set the trigger position slider.
    //
    OnScopeTriggerPosChanged(0, pSettings->lTriggerPos);

    //
    // Determine whether we need to enable or disable the "One Shot" button.
    //
    if(pSettings->ucStarted)
    {
        OnScopeStarted(0, 0);
    }
    else
    {
        OnScopeStopped(0, 0);
    }
}

//***************************************************************************
//
// The "Quit" button has been pressed. Tidy up and exit.
//
//***************************************************************************
void ClmscopeDlg::OnBnClickedOk()
{
    //
    // Update the status bar text
    //
    m_csbStatusBar.SetPaneTextByResource(0, IDS_STATUS_CLOSING, TRUE);

    //
    // Free resources allocated by the scope control module.
    //
    ScopeControlDisconnect();
    ScopeControlTerm();

    //
    // Call Windows to do the usual shutdown processing.
    //
    OnOK();
}

//*****************************************************************************
//
// Formats a string containing a number, its units and a suffix string.
//
// \param pcString points to the CString into which the string is to be
// formatted.
// \param pcSuffix points to a string that is to be appended to the end of
// the formatted number and units.
// \param pcUnit points to a string describing the unit of measurement of
// the number as passed in lValue.
// \param pcUnit1000 points to a string describing the unit of measurement of
// the (lValue / 1000).
// \param lValue is the number which is to be formatted into the buffer.
//
// This function is called to generate strings suitable for display when the
// number to be rendered may take on a wide range of values. It considers the
// size of lValue and, if necessary, divides by 1000 and formats it with
// as few digits after the decimal point as necessary (to remove trailing 0s).
// For example, passing lValue 5300, pcSuffix "/div", pcUnit "mV" and
// pcUnit1000 "V" would return formatted string "5.3V/div". Reducing lValue
// to 800 would result in "800mV/div".
//
// \return None.
//
//*****************************************************************************
void ClmscopeDlg::ScaleAndFormatString(CString *pcString, LPCTSTR pcSuffix,
                             LPCTSTR pcUnit, LPCTSTR pcUnit1000, long lValue)
{
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
            pcString->Format((LPCTSTR)L"%d%s%s", (lValue / 1000), pcUnit1000,
                              pcSuffix);
        }
        else if((lValue % 100) == 0)
        {
            //
            // Multiple of 100 - 1 decimal place needed.
            //
            pcString->Format((LPCTSTR)L"%d.%d%s%s", (lValue / 1000),
                             abs((lValue % 1000) / 100), pcUnit1000,
                             pcSuffix);
        }
        else if((lValue % 10) == 0)
        {
            //
            // Multiple of 10 - 2 decimal place needed.
            //
            pcString->Format((LPCTSTR)L"%d.%02d%s%s", (lValue / 1000),
                             abs((lValue % 1000) / 10), pcUnit1000, pcSuffix);
        }
        else
        {
            //
            // 3 decimal place needed.
            //
            pcString->Format((LPCTSTR)L"%d.%03d%s%s", (lValue / 1000),
                             abs(lValue % 1000), pcUnit1000, pcSuffix);
        }
    }
    else
    {
        //
        // The value passed is less than 1000 so we just display it as it is.
        //
        pcString->Format((LPCTSTR)L"%d%s%s", lValue, pcUnit,  pcSuffix);
    }
}

//***************************************************************************
//
// Fill the various combo boxes with the appropriate strings and values.
//
//***************************************************************************
void ClmscopeDlg::InitComboBoxContents(void)
{
    int iLoop;

    //
    // Empty each of the combo boxes.
    //
    m_cTimebase.ResetContent();
    m_cChannel1Scale.ResetContent();
    m_cChannel2Scale.ResetContent();
    m_cTriggerType.ResetContent();

    //
    // Fill the scale combo boxes.
    //
    for(iLoop = 0; iLoop < (sizeof(g_pdwVoltages) / sizeof(DWORD)); iLoop++)
    {
        CString strText;

        //
        // Format the string matching the millivolt value in the array.
        //
        ScaleAndFormatString(&strText,  (LPCTSTR)L"\\div", (LPCTSTR)L"mV",
                             (LPCTSTR)L"V", g_pdwVoltages[iLoop]);
        m_cChannel1Scale.InsertString(iLoop, strText);
        m_cChannel1Scale.SetItemData(iLoop, g_pdwVoltages[iLoop]);
        m_cChannel2Scale.InsertString(iLoop, strText);
        m_cChannel2Scale.SetItemData(iLoop, g_pdwVoltages[iLoop]);
    }

    //
    // Fill the timebase combo box.
    //
    for(iLoop = 0; iLoop < (sizeof(g_pdwTimebases) / sizeof(DWORD)); iLoop++)
    {
        CString strText;

        //
        // Format the string matching the millivolt value in the array.
        //
        ScaleAndFormatString(&strText,  (LPCTSTR)L"\\div", (LPCTSTR)L"uS",
                             (LPCTSTR)L"mS", g_pdwTimebases[iLoop]);
        m_cTimebase.InsertString(iLoop, strText);
        m_cTimebase.SetItemData(iLoop, g_pdwTimebases[iLoop]);
    }

    //
    // Fill the trigger type combo box.
    //
    for(iLoop = 0; iLoop < (sizeof(g_psTriggers) / sizeof(tComboEntry)); iLoop++)
    {
        CString strText;

        //
        // Format the string matching the millivolt value in the array.
        //
        strText.LoadString(g_psTriggers[iLoop].iStringID);
        m_cTriggerType.InsertString(iLoop, strText);
        m_cTriggerType.SetItemData(iLoop, g_psTriggers[iLoop].dwValue);
    }
}


//***************************************************************************
//
// Update the voltage measurements displayed when a new data set is
// received.
//
//***************************************************************************
void ClmscopeDlg::UpdateVoltageMeasurements(void)
{
    unsigned long ulLoop;
    int iMax[2];
    int iMin[2];
    int iMean[2];
    CString strMin;
    CString strMax;
    CString strMean;

    //
    // If we don't have any data, just clear the various display strings.
    //
    if(m_psScopeData == NULL)
    {
        strMin = L"";
        m_cCh1Max.SetWindowText(strMin);
        m_cCh2Max.SetWindowText(strMin);
        m_cCh1Min.SetWindowText(strMin);
        m_cCh2Min.SetWindowText(strMin);
        m_cCh1Mean.SetWindowText(strMin);
        m_cCh2Mean.SetWindowText(strMin);
        return;
    }

    //
    // Clear our result variables.
    //
    iMax[0] = -30000;
    iMax[1] = -30000;
    iMin[0] = 30000;
    iMin[1] = 30000;
    iMean[0] = 0;
    iMean[1] = 0;

    //
    // Are we dealing with a single- or dual-channel dataset?
    //
    if(m_psScopeData->bDualChannel)
    {
        tScopeDualDataElement *psElement;

        //
        // Dual channel data.
        //
        psElement = (tScopeDualDataElement *)((char *)m_psScopeData +
                                          m_iSampleOffset);

        //
        // Find the minimum, maximum and mean voltages for the channels.
        //
        for(ulLoop = 0; ulLoop < m_psScopeData->ulTotalElements; ulLoop++)
        {
            //
            // Add to our mean accumulation totals.
            //
            iMean[0] += (int)psElement[ulLoop].sSample1mVolts;
            iMean[1] += (int)psElement[ulLoop].sSample2mVolts;

            //
            // New minimum value for either channel?
            //
            if(psElement[ulLoop].sSample1mVolts < iMin[0])
            {
                iMin[0] = (int)psElement[ulLoop].sSample1mVolts;
            }

            if(psElement[ulLoop].sSample2mVolts < iMin[1])
            {
                iMin[1] = (int)psElement[ulLoop].sSample2mVolts;
            }

            //
            // New maximum value for either channel?
            //
            if(psElement[ulLoop].sSample1mVolts > iMax[0])
            {
                iMax[0] = (int)psElement[ulLoop].sSample1mVolts;
            }

            if(psElement[ulLoop].sSample2mVolts > iMax[1])
            {
                iMax[1] = (int)psElement[ulLoop].sSample2mVolts;
            }
        }

        //
        // Divide the mean accumulators by the number of samples to give
        // the real means.
        //
        iMean[0] = iMean[0] / (int)m_psScopeData->ulTotalElements;
        iMean[1] = iMean[1] / (int)m_psScopeData->ulTotalElements;

    }
    else
    {
        //
        // Single channel data.
        //
        tScopeDataElement *psElement;

        //
        // Get a pointer to the first sample.
        //
        psElement = (tScopeDataElement *)((char *)m_psScopeData +
                                          m_iSampleOffset);

        //
        // Determine the minimum, maximum and mean values for this channel.
        //
        for(ulLoop = 0; ulLoop < m_psScopeData->ulTotalElements; ulLoop++)
        {
            //
            // Add to our mean accumulation total.
            //
            iMean[0] += (int)psElement[ulLoop].sSamplemVolts;

            //
            // New minimum value?
            //
            if(psElement[ulLoop].sSamplemVolts < iMin[0])
            {
                iMin[0] = (int)psElement[ulLoop].sSamplemVolts;
            }

            //
            // New maximum value?
            //
            if(psElement[ulLoop].sSamplemVolts > iMax[0])
            {
                iMax[0] = (int)psElement[ulLoop].sSamplemVolts;
            }
        }

        //
        // Divide the mean accumulator by the number of samples to give
        // the real mean.
        //
        iMean[0] = iMean[0] / (int)m_psScopeData->ulTotalElements;
    }

    //
    // Calculations are complete so now update the display.
    //
    ScaleAndFormatString(&strMin, (LPCTSTR)L"", (LPCTSTR)L"mV",
                         (LPCTSTR)L"V", (long)iMin[0]);
    ScaleAndFormatString(&strMax, (LPCTSTR)L"", (LPCTSTR)L"mV",
                         (LPCTSTR)L"V", (long)iMax[0]);
    ScaleAndFormatString(&strMean, (LPCTSTR)L"", (LPCTSTR)L"mV",
                         (LPCTSTR)L"V", (long)iMean[0]);

    //
    // We always have at least 1 channel of data but which one
    // is first?
    //
    if(m_psScopeData->bCh2SampleFirst)
    {
        m_cCh2Max.SetWindowText(strMax);
        m_cCh2Min.SetWindowText(strMin);
        m_cCh2Mean.SetWindowText(strMean);
    }
    else
    {
        m_cCh1Max.SetWindowText(strMax);
        m_cCh1Min.SetWindowText(strMin);
        m_cCh1Mean.SetWindowText(strMean);
    }

    //
    // Do we have two channel data?
    //
    if(m_psScopeData->bDualChannel)
    {
        ScaleAndFormatString(&strMin, (LPCTSTR)L"", (LPCTSTR)L"mV",
                             (LPCTSTR)L"V", (long)iMin[1]);
        ScaleAndFormatString(&strMax, (LPCTSTR)L"", (LPCTSTR)L"mV",
                             (LPCTSTR)L"V", (long)iMax[1]);
        ScaleAndFormatString(&strMean, (LPCTSTR)L"", (LPCTSTR)L"mV",
                             (LPCTSTR)L"V", (long)iMean[1]);

        //
        // Which channel do these measurements relate to?
        //
        if(m_psScopeData->bCh2SampleFirst)
        {
            m_cCh1Max.SetWindowText(strMax);
            m_cCh1Min.SetWindowText(strMin);
            m_cCh1Mean.SetWindowText(strMean);
        }
        else
        {
            m_cCh2Max.SetWindowText(strMax);
            m_cCh2Min.SetWindowText(strMin);
            m_cCh2Mean.SetWindowText(strMean);
        }
    }
    else
    {
        //
        // Single channel data so we need to clear the text in
        // the other channel's measurement display controls.
        //
        strMin = L"";

        //
        // If the channel 2 sample is first and this is single
        // channel data, channel 1 must be disabled.
        //
        if(m_psScopeData->bCh2SampleFirst)
        {
            m_cCh1Max.SetWindowText(strMin);
            m_cCh1Min.SetWindowText(strMin);
            m_cCh1Mean.SetWindowText(strMin);
        }
        else
        {
            m_cCh2Max.SetWindowText(strMin);
            m_cCh2Min.SetWindowText(strMin);
            m_cCh2Mean.SetWindowText(strMin);
        }
    }
}

//***************************************************************************
//
// Add to the existing CStatusBar class to allow setting strings from a
// resource ID rather than a string pointer.
//
//***************************************************************************
CStatusBarExt::CStatusBarExt() : CStatusBar()
{
    //
    // Just use the existing CStatusBar constructor.
    //
}

//***************************************************************************
//
// Set the pane text from a string in the application string table.
//
//***************************************************************************
BOOL CStatusBarExt::SetPaneTextByResource(int nIndex, int iStringID,
                                          BOOL bUpdate)
{
    CString strText;

    strText.LoadStringW(iStringID);
    return(this->SetPaneText(nIndex, (LPCTSTR)strText, bUpdate));
}

CComboBoxExt::CComboBoxExt() : CComboBox()
{
    //
    // Just use the existing CComboBox constructor.
    //
}

//***************************************************************************
//
// Set the combo box selection based on the value assigned to an item
// rather than its index or string.
//
//***************************************************************************
int CComboBoxExt::SetCurSelByValue(DWORD dwValue)
{
    int iLoop;
    int iCount;

    //
    // How many items does this combo box contain?
    //
    iCount = GetCount();

    //
    // Check each looking for a data match with dwValue.
    //
    for(iLoop = 0; iLoop < iCount; iLoop++)
    {
        //
        // Does this item's data match?
        //
        if(dwValue == (DWORD)GetItemData(iLoop))
        {
            //
            // Yes - set the selection and return.
            //
            SetCurSel(iLoop);
            return(iLoop);
        }
    }

    //
    // No item was found that has the supplied value associated with it.
    //
    return(CB_ERR);
}

//***************************************************************************
//
// Our keep-alive timer expired. Check to ensure that we got a response to
// our last ping. If we did, issue another ping, else send the HOST_HELLO
// message to try to reconnect to the device.
//
//***************************************************************************
void ClmscopeDlg::OnTimer(UINT_PTR nIDEvent)
{
// DEBUG ONLY - remove this!
#ifdef NEVER
    if(nIDEvent == PING_TIMER)
    {
        //
        // Have we received a response to our last ping?
        //
        if(m_bPingResponseReceived)
        {
            //
            // Yes - the connection is still alive. Send another ping.
            //
            m_bPingResponseReceived = FALSE;
            m_iPingCount++;
            ScopeControlPing(0, 0);
        }
        else
        {
            //
            // The connection appears to have been broken. Since we have not received a
            // notification from ScopeControl, the break must not be due to physical
            // removal of the USB device so send a new HOST_HELLO to reconnect at the
            // application level.
            //
            m_bReconnecting = TRUE;
            ScopeControlConnect(NULL);
        }
    }
#endif
    CDialog::OnTimer(nIDEvent);
}


//***************************************************************************
//
// This function is called by Windows to give us an opportunity to set the
// background color of various controls in the dialog.
//
//***************************************************************************
HBRUSH ClmscopeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    int iCtrlID;

    //
    // Are we being asked for the dialog background color?
    //
    if(nCtlColor == CTLCOLOR_DLG)
    {
        //
        // Yes - make sure we pass back the grey color that matches
        // the background of our Luminary Micro logo bitmap.
        //
        return((HBRUSH)m_cGreyBrush);
    }

    //
    // Determine which control is being painted.
    //
    iCtrlID = pWnd->GetDlgCtrlID();

    switch(iCtrlID)
    {
        //
        // We use a black brush to paint the background of the waveform
        // display control.
        //
        case IDC_WAVEFORM:
        {
            hbr = m_cBlackBrush;
        }
        break;

        //
        // All other controls use the default grey brush.
        //
        default:
            pDC->SetBkColor(SCOPE_COLOR_DLG_BACKGROUND);
            hbr = m_cGreyBrush;
            break;
    }

    //
    // Return the appropriate brush handle.
    //
    return(hbr);
}

//***************************************************************************
//
// Map help IDs so that right click provides context-sensitive help.
//
//***************************************************************************
void ClmscopeDlg::OnContextMenu(CWnd *pWnd, CPoint point)
{
    CString strHelpFile;
    int iCtrlID;

    //
    // Determine the path to the help file.
    //
    strHelpFile = AfxGetApp()->m_pszHelpFilePath;
    strHelpFile += "::/ctrlhlp.txt";

    //
    // Which control is asking for help?
    //
    iCtrlID = pWnd->GetDlgCtrlID();

    //
    // Make sure the mouse was clicked on a control and not the dialog
    // background before calling HtmlHelp. If we don't do this, right
    // clicks not above a control result in a nasty error message box.
    //
    if((iCtrlID > 0) && (iCtrlID < CTRL_ID_MAX))
    {
        //
        // Call the help system to show the help information in a
        // popup.
        //
        ::HtmlHelp(pWnd->GetSafeHwnd(), (LPCWSTR)strHelpFile,
                   HH_TP_HELP_CONTEXTMENU, (DWORD)g_pdwHelpIDs);
    }
}

//***************************************************************************
//
// Map help IDs so that F1 provides context-sensitive help.
//
//***************************************************************************
BOOL ClmscopeDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
    int iControlID;

    //
    // Which control is requesting help?
    //
    iControlID = pHelpInfo->iCtrlId;

    //
    // Make sure the control is something we are interested in showing
    // help for.
    //
    if((iControlID > 0)  && (iControlID < CTRL_ID_MAX))
    {
        CWinApp *theApp = AfxGetApp();
        CString strHelpFile = theApp->m_pszHelpFilePath;
        strHelpFile += "::/ctrlhlp.txt";

        //
        // Call the help system to show the help information in a
        // popup.
        //
        ::HtmlHelp((HWND)pHelpInfo->hItemHandle, (LPCWSTR)strHelpFile,
                    HH_TP_HELP_WM_HELP, (DWORD)g_pdwHelpIDs);

    }

    return(TRUE);
}
