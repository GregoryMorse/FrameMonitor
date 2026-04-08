
// FrameMonitor.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CFrameMonitorApp:
// See FrameMonitor.cpp for the implementation of this class
//

class CFrameMonitorApp : public CWinApp
{
public:
	CFrameMonitorApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CFrameMonitorApp theApp;
