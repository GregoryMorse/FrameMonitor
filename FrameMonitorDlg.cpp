
// FrameMonitorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FrameMonitor.h"
#include "FrameMonitorDlg.h"
#include "afxdialogex.h"
//#include "Video.h"
#include <functional>
#include <locale>
#include <codecvt>

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


// CFrameMonitorDlg dialog



extern void VideoCleanup(void*);

CFrameMonitorDlg::CFrameMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_FRAMEMONITOR_DIALOG, pParent)
	, m_vc(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFrameMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	/*DDX_Control(pDX, IDC_TIMELINE, m_timelineStatic);
	DDX_Control(pDX, IDC_VIDEOIMG, m_VideoImg);
	DDX_Control(pDX, IDC_BKGNDIMG, m_BkgndImg);
	DDX_Control(pDX, IDC_FGNDIMG, m_FgndImg);*/
	DDX_Control(pDX, IDC_MAINTAB, m_MainTab);
}

BEGIN_MESSAGE_MAP(CFrameMonitorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPENVIDEO, &CFrameMonitorDlg::OnBnClickedOpenvideo)
	ON_MESSAGE(WM_CUSTOM_MOVEVIDEO, &CFrameMonitorDlg::OnCustomMovevideo)
	ON_MESSAGE(WM_CUSTOM_OPENVIDCOMPLETE, &CFrameMonitorDlg::OnCustomOpenVidComplete)
	ON_MESSAGE(WM_CUSTOM_OPENPROGRESS, &CFrameMonitorDlg::OnCustomOpenProgress)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_MAINTAB, &CFrameMonitorDlg::OnTcnSelchangeMaintab)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CFrameMonitorDlg message handlers

BOOL CFrameMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_MainTab.SetExtendedStyle(m_MainTab.GetExtendedStyle() | WS_EX_CONTROLPARENT);
	RECT rect;
	GetClientRect(&rect);
	m_Progress.Create(WS_VISIBLE | WS_CHILD | PBS_SMOOTH, RECT{ rect.left + 10, rect.top + 10, rect.right - 10, rect.top + 10 + 50 }, this, IDC_PROGRESS);
	m_ProgText.Create(_T("Waiting for video..."), WS_VISIBLE | WS_CHILD | SS_CENTER, RECT{ rect.left + 10, rect.top + 10 + 60, rect.right - 10, rect.bottom - 80 }, this, IDC_PROGTEXT);
	m_timelineStatic.Create(_T(""), WS_VISIBLE | WS_CHILD | SS_NOTIFY, RECT{ rect.left + 10, rect.bottom - 70, rect.right - 140, rect.bottom - 10 }, this, IDC_TIMELINE);
	m_OpenBtn.Create(_T("&Open Video"), WS_VISIBLE | WS_CHILD, RECT{ rect.right - 130, rect.bottom - 70, rect.right - 10, rect.bottom - 10 }, this, IDC_OPENVIDEO);
	m_timelineStatic.SetFont(GetFont());
	m_OpenBtn.SetFont(GetFont());

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

void CFrameMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CFrameMonitorDlg::OnPaint()
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
HCURSOR CFrameMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static TCHAR BASED_CODE szFilter[] = _T("Videos (*.mp4;*.avi;*.mts)|*.mp4; *.avi; *.mts||");

static UINT VideoProcessor(LPVOID pParam)
{
	CFrameMonitorDlg* pfrd = (CFrameMonitorDlg*)pParam;
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;
	std::string str = converter.to_bytes(pfrd->m_curFile);
	/*ProcessVideo(str, pfrd->m_vc, pfrd->m_timelineStatic.m_motionDetected, pfrd->m_timelineStatic.m_oscillation,
		pfrd->m_timelineStatic.m_dMaxContourSize, pfrd->m_timelineStatic.m_dMaxOscillation,
		std::move(std::function<ProgressFunc>([pfrd](int Frame, int TotalFrames) -> void { pfrd->PostMessage(WM_CUSTOM_OPENPROGRESS, Frame, TotalFrames); })), &pfrd->bExit);*/
	pfrd->PostMessageW(WM_CUSTOM_OPENVIDCOMPLETE, 0, 0);
	pfrd->PostMessageW(WM_CUSTOM_MOVEVIDEO, 0, 0);
	return 0;
}

void CFrameMonitorDlg::OnBnClickedOpenvideo()
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
		//VideoCleanup(m_vc);
		VideoCleanup(m_vc);
		m_vc = NULL;
		m_timelineStatic.m_iFrameNum = 0;
		m_timelineStatic.m_motionDetected.clear();
		m_timelineStatic.m_dMaxContourSize = 0;
		m_curFile = dlgFile.GetPathName();
		if (m_BkgndImg.GetSafeHwnd() != NULL) m_BkgndImg.DestroyWindow();
		if (m_FgndImg.GetSafeHwnd() != NULL) m_FgndImg.DestroyWindow();
		if (m_VideoImg.GetSafeHwnd() != NULL) m_VideoImg.DestroyWindow();
		RECT rect;
		GetClientRect(&rect);
		m_OpenTime = (int)time(NULL);
		if (m_Progress.GetSafeHwnd() == NULL) m_Progress.Create(WS_VISIBLE | WS_CHILD | PBS_SMOOTH, RECT{ rect.left + 10, rect.top + 10, rect.right - 10, rect.top + 10 + 50 }, this, IDC_PROGRESS);
		if (m_ProgText.GetSafeHwnd() == NULL) m_ProgText.Create(_T("Waiting for video..."), WS_VISIBLE | WS_CHILD | SS_CENTER, RECT{ rect.left + 10, rect.top + 10 + 60, rect.right - 10, rect.bottom - 80 }, this, IDC_PROGTEXT);
		m_pWorkThread = AfxBeginThread(VideoProcessor, this, 0, CREATE_SUSPENDED, NULL);
		m_pWorkThread->m_bAutoDelete = FALSE; //cannot determine if it terminated otherwise...
		m_pWorkThread->ResumeThread();
	}
}

afx_msg LRESULT CFrameMonitorDlg::OnCustomOpenProgress(WPARAM wParam, LPARAM lParam)
{
	if (m_Progress.GetSafeHwnd() != NULL) {
		m_Progress.SetPos((int)wParam);
		m_Progress.SetRange32(0, (int)lParam);
	}
	if (m_ProgText.GetSafeHwnd() != NULL) {
		TCHAR buf[1024];
		_stprintf(buf, _T("%lu out of %lu (%lu seconds elapsed, %.02f FPS)\nApproximately %.00f seconds remaining"), (unsigned long)wParam, (unsigned long)lParam, (unsigned long)(time(NULL) - m_OpenTime), (time(NULL) == m_OpenTime) ? (double)wParam : ((double)wParam / (time(NULL) - m_OpenTime)), (time(NULL) == m_OpenTime) ? (double)0 : ((lParam - wParam) * (time(NULL) - m_OpenTime) / (double)wParam));
		m_ProgText.SetWindowTextW(buf);
	}
	return 0;
}

afx_msg LRESULT CFrameMonitorDlg::OnCustomOpenVidComplete(WPARAM wParam, LPARAM lParam)
{
	WaitForSingleObject(m_pWorkThread, INFINITE);
	delete m_pWorkThread;
	m_pWorkThread = NULL;
	m_Progress.DestroyWindow();
	m_ProgText.DestroyWindow();
	RECT rect;
	GetClientRect(&rect);
	m_BkgndImg.Create(_T(""), WS_VISIBLE | WS_CHILD | SS_BITMAP, RECT{ rect.right - 130, rect.top + 10, rect.right - 10, rect.top + 10 + 130 }, this, IDC_BKGNDIMG);
	m_FgndImg.Create(_T(""), WS_VISIBLE | WS_CHILD | SS_BITMAP, RECT{ rect.right - 130, rect.top + 150, rect.right - 10, rect.bottom - 80 }, this, IDC_FGNDIMG);
	m_VideoImg.Create(_T(""), WS_VISIBLE | WS_CHILD | SS_BITMAP, RECT{ rect.left + 10, rect.top + 10, rect.right - 140, rect.bottom - 80 }, this, IDC_VIDEOIMG);
	OnCustomMovevideo(0, 0);
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

typedef void (ProgressFunc)(int Frame, int TotalFrames);

//portable adapter layer
HBITMAP GetVideoFrameAdapt(LPVOID p, int iFrameNum, HBITMAP & hBackground, HBITMAP & hForeground, std::function<ProgressFunc> pf, int* pCancel)
{
	/*if (p == NULL) return (HBITMAP)INVALID_HANDLE_VALUE;
	cv::Mat fgnd, bkgnd, bit = GetVideoFrame(p, iFrameNum, bkgnd, fgnd, pf, pCancel);
	hBackground = CreateBitmap(bkgnd.cols, bkgnd.rows, 1, 32, bkgnd.data);
	//hForeground = CreateBitmap(fgnd.cols, fgnd.rows, 1, 1, fgnd.data); //scanline width must be DWORD aligned but it should be here as 640, 480 are multiples of 32!...
	hForeground = CreateBitmap(fgnd.cols, fgnd.rows, 1, 32, fgnd.data);
	return CreateBitmap(bit.cols, bit.rows, 1, 32, bit.data);*/
	return NULL;
}

afx_msg LRESULT CFrameMonitorDlg::OnCustomMovevideo(WPARAM wParam, LPARAM lParam)
{
	if (m_vc != NULL) {
		/*HBITMAP hBackground, hForeground, hBitmap = GetVideoFrameAdapt(m_vc, lParam, hBackground, hForeground, NULL, &bExit);

		hBitmap = m_VideoImg.SetBitmap(ScaleBitmap(hBitmap, &m_VideoImg));
		if (hBitmap != NULL) DeleteObject(hBitmap);

		hForeground = m_FgndImg.SetBitmap(ScaleBitmap(hForeground, &m_FgndImg));
		if (hForeground != NULL) DeleteObject(hForeground);

		hBackground = m_BkgndImg.SetBitmap(ScaleBitmap(hBackground, &m_BkgndImg));
		if (hBackground != NULL) DeleteObject(hBackground);*/
	}
	return 0;
}


void CFrameMonitorDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	if (m_pWorkThread != NULL) {
		WaitForSingleObject(m_pWorkThread, INFINITE);
		delete m_pWorkThread;
		m_pWorkThread = NULL;
	}
	VideoCleanup(m_vc);
	m_vc = NULL;
}


void CFrameMonitorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	RECT rect;
	GetClientRect(&rect);
	if (m_MainTab.GetSafeHwnd() != NULL) {
		RECT r = { rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2 };
		m_MainTab.MoveWindow(&r);
	}
	if (m_BkgndImg.GetSafeHwnd() != NULL) {
		RECT r = { rect.right - 130, rect.top + 10, rect.right - 10, (rect.bottom - 80 - 10) / 2 + 10 - 5 };
		m_BkgndImg.MoveWindow(&r);
	}
	if (m_FgndImg.GetSafeHwnd() != NULL) {
		RECT r = { rect.right - 130, (rect.bottom - 80 - 10) / 2 + 10 + 5, rect.right - 10, rect.bottom - 80 };
		m_FgndImg.MoveWindow(&r);
	}
	if (m_VideoImg.GetSafeHwnd() != NULL) {
		RECT r = { rect.left + 10, rect.top + 10, rect.right - 140, rect.bottom - 80 };
		m_VideoImg.MoveWindow(&r);
	}
	if (m_timelineStatic.GetSafeHwnd() != NULL) {
		RECT r = { rect.left + 10, rect.bottom - 70, rect.right - 140, rect.bottom - 10 };
		m_timelineStatic.MoveWindow(&r);
	}
	if (m_OpenBtn.GetSafeHwnd() != NULL) {
		RECT r = { rect.right - 130, rect.bottom - 70, rect.right - 10, rect.bottom - 10 };
		m_OpenBtn.MoveWindow(&r);
	}
}


void CFrameMonitorDlg::OnTcnSelchangeMaintab(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


void CFrameMonitorDlg::OnClose()
{
	bExit = 1;
	if (m_pWorkThread != NULL) {
		WaitForSingleObject(m_pWorkThread, INFINITE);
		delete m_pWorkThread;
		m_pWorkThread = NULL;
	}
	VideoCleanup(m_vc);
	m_vc = NULL;
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnClose();
}
