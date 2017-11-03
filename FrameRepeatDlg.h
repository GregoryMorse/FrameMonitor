
// FrameRepeatDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "TimelineStatic.h"
#include "afxcmn.h"

#define WM_CUSTOM_OPENVIDCOMPLETE WM_USER + 1
#define WM_CUSTOM_OPENPROGRESS WM_USER + 2

// CFrameRepeatDlg dialog
class CFrameRepeatDlg : public CDialogEx
{
// Construction
public:
	CFrameRepeatDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FRAMEREPEAT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	int m_OpenTime;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnCustomMovevideo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCustomOpenProgress(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCustomOpenVidComplete(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOpenvideo();
	int bExit;
	CWinThread* m_pWorkThread;
	CTimelineStatic m_timelineStatic;
	LPVOID m_vc;
	CString m_curFile;
	CButton m_OpenBtn;
	CStatic m_VideoImg;
	CStatic m_BkgndImg;
	CStatic m_FgndImg;
	CProgressCtrl m_Progress;
	CStatic m_ProgText;
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	CTabCtrl m_MainTab;
	afx_msg void OnTcnSelchangeMaintab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnClose();
};
