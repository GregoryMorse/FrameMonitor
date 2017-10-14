
// FrameRepeatDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "TimelineStatic.h"

#define WM_CUSTOM_OPENVIDCOMPLETE WM_USER + 1

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

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnCustomMovevideo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCustomOpenVidComplete(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOpenvideo();
	CWinThread* m_pWorkThread;
	CTimelineStatic m_timelineStatic;
	LPVOID m_vc;
	CString m_curFile;
	CStatic m_VideoImg;
	CStatic m_BkgndImg;
	CStatic m_FgndImg;
	afx_msg void OnDestroy();
};
