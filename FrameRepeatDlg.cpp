
// FrameRepeatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FrameRepeat.h"
#include "FrameRepeatDlg.h"
#include "afxdialogex.h"
#include "Video.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CFrameRepeatDlg dialog



CFrameRepeatDlg::CFrameRepeatDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_FRAMEREPEAT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFrameRepeatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TIMELINE, m_timelineStatic);
	DDX_Control(pDX, IDC_VIDEOIMG, m_VideoImg);
	DDX_Control(pDX, IDC_BKGNDIMG, m_BkgndImg);
	DDX_Control(pDX, IDC_FGNDIMG, m_FgndImg);
}

BEGIN_MESSAGE_MAP(CFrameRepeatDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPENVIDEO, &CFrameRepeatDlg::OnBnClickedOpenvideo)
	ON_MESSAGE(WM_CUSTOM_MOVEVIDEO, &CFrameRepeatDlg::OnCustomMovevideo)
	ON_MESSAGE(WM_CUSTOM_OPENVIDCOMPLETE, &CFrameRepeatDlg::OnCustomOpenVidComplete)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CFrameRepeatDlg message handlers

BOOL CFrameRepeatDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_timelineStatic.ModifyStyle(0, SS_OWNERDRAW);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFrameRepeatDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFrameRepeatDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFrameRepeatDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static TCHAR BASED_CODE szFilter[] = _T("Videos (*.mp4;*.avi)|*.mp4; *.avi||");

static UINT VideoProcessor(LPVOID pParam)
{
	CFrameRepeatDlg* pfrd = (CFrameRepeatDlg*)pParam;
	ProcessVideo(pfrd->m_curFile, pfrd->m_vc, pfrd->m_timelineStatic.m_motionDetected, pfrd->m_timelineStatic.m_dMaxContourSize);
	pfrd->PostMessageW(WM_CUSTOM_OPENVIDCOMPLETE, 0, 0);
	pfrd->PostMessageW(WM_CUSTOM_MOVEVIDEO, 0, 0);
	return 0;
}

void CFrameRepeatDlg::OnBnClickedOpenvideo()
{
	DWORD dwRes;
	if (m_pWorkThread != NULL && WaitForSingleObject(m_pWorkThread, 0) == WAIT_OBJECT_0 &&
		GetExitCodeThread(m_pWorkThread, &dwRes) && dwRes != STILL_ACTIVE) {
		delete m_pWorkThread;
		m_pWorkThread = NULL;
		m_timelineStatic.Invalidate();
	}
	if (m_pWorkThread != NULL) return;
	CFileDialog dlgFile(TRUE, _T("*.mp4"), NULL, OFN_FILEMUSTEXIST, szFilter, this);
	if (dlgFile.DoModal() != IDCANCEL) {
		VideoCleanup(m_vc);
		m_vc = NULL;
		m_timelineStatic.m_iFrameNum = 0;
		m_timelineStatic.m_motionDetected.clear();
		m_timelineStatic.m_dMaxContourSize = 0;
		m_curFile = dlgFile.GetPathName();
		m_pWorkThread = AfxBeginThread(VideoProcessor, this, 0, CREATE_SUSPENDED, NULL);
		m_pWorkThread->m_bAutoDelete = FALSE; //cannot determine if it terminated otherwise...
		m_pWorkThread->ResumeThread();
	}
	// TODO: Add your control notification handler code here
}

afx_msg LRESULT CFrameRepeatDlg::OnCustomOpenVidComplete(WPARAM wParam, LPARAM lParam)
{
	WaitForSingleObject(m_pWorkThread, INFINITE);
	delete m_pWorkThread;
	m_pWorkThread = NULL;
	m_timelineStatic.Invalidate();
	return 0;
}

HBITMAP ScaleBitmap(HBITMAP hBitmap, CWnd* wnd)
{
	RECT rect;
	wnd->GetWindowRect(&rect);
	CDC* pdc = wnd->GetDC();
	HDC hSrcDC = CreateCompatibleDC(pdc->m_hDC);
	HDC hdc = CreateCompatibleDC(pdc->m_hDC);
	HBITMAP hNewBitmap = CreateCompatibleBitmap(pdc->m_hDC, rect.right - rect.left, rect.bottom - rect.top);
	wnd->ReleaseDC(pdc);
	BITMAP bm;
	GetObject(hBitmap, sizeof(BITMAP), &bm);
	HGDIOBJ hOldBitmap = SelectObject(hSrcDC, hBitmap);
	HGDIOBJ hDestOldBitmap = SelectObject(hdc, hNewBitmap);
	SetStretchBltMode(hdc, COLORONCOLOR);
	StretchBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hSrcDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
	SelectObject(hSrcDC, hOldBitmap);
	DeleteDC(hSrcDC);
	SelectObject(hdc, hDestOldBitmap);
	DeleteDC(hdc);
	DeleteObject(hBitmap);
	return hNewBitmap;
}

afx_msg LRESULT CFrameRepeatDlg::OnCustomMovevideo(WPARAM wParam, LPARAM lParam)
{
	HBITMAP hBackground, hForeground, hBitmap = GetVideoFrameAdapt(m_vc, lParam, hBackground, hForeground);

	hBitmap = m_VideoImg.SetBitmap(ScaleBitmap(hBitmap, &m_VideoImg));
	if (hBitmap != NULL) DeleteObject(hBitmap);

	hForeground = m_FgndImg.SetBitmap(ScaleBitmap(hForeground, &m_FgndImg));
	if (hForeground != NULL) DeleteObject(hForeground);

	hBackground = m_BkgndImg.SetBitmap(ScaleBitmap(hBackground, &m_BkgndImg));
	if (hBackground != NULL) DeleteObject(hBackground);
	return 0;
}


void CFrameRepeatDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	WaitForSingleObject(m_pWorkThread, INFINITE);
	delete m_pWorkThread;
	m_pWorkThread = NULL;
	VideoCleanup(m_vc);
	m_vc = NULL;
}
