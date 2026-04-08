
// FrameMonitor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <fstream>
#include <string>
#include "FrameMonitor.h"
#include "FrameMonitorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {

std::string GetExeDir()
{
	char p[MAX_PATH] = {};
	GetModuleFileNameA(NULL, p, MAX_PATH);
	std::string s(p);
	auto pos = s.find_last_of("\\/");
	return (pos != std::string::npos) ? s.substr(0, pos + 1) : std::string();
}

struct LaunchConfig
{
	enum Mode { None, Video, Camera } mode = None;
	std::string videoPath;
	int cameraIndex = 0;
	std::string outputDir;  // empty = current working directory
};

// Reads framemonitor.cfg, searching the working directory then the exe directory.
// Parses the first active 'video=', 'camera=', and 'output_dir=' entries.
LaunchConfig ReadVideoConfig()
{
	LaunchConfig result;
	const std::string candidates[] = {
		"framemonitor.cfg",
		GetExeDir() + "framemonitor.cfg"
	};
	for (const auto& cfgPath : candidates)
	{
		std::ifstream f(cfgPath);
		if (!f.is_open()) continue;
		std::string line;
		while (std::getline(f, line))
		{
			while (!line.empty() && (line.back() == '\r' || line.back() == '\n' ||
				   line.back() == ' '  || line.back() == '\t'))
				line.pop_back();
			auto ns = line.find_first_not_of(" \t");
			if (ns == std::string::npos) continue;
			if (ns > 0) line = line.substr(ns);
			if (line[0] == ';' || line[0] == '#') continue;

			if (result.outputDir.empty() &&
				line.rfind("output_dir=", 0) == 0 && line.size() > 11)
			{
				result.outputDir = line.substr(11);
				continue;
			}
			if (result.mode == LaunchConfig::None)
			{
				if (line.rfind("video=", 0) == 0 && line.size() > 6)
				{
					result.mode = LaunchConfig::Video;
					result.videoPath = line.substr(6);
				}
				else if (line.rfind("camera=", 0) == 0)
				{
					result.mode = LaunchConfig::Camera;
					try { result.cameraIndex = std::stoi(line.substr(7)); } catch (...) {}
				}
			}
		}
		// Stop at the first config file that has any active entry
		if (result.mode != LaunchConfig::None || !result.outputDir.empty()) return result;
	}
	return result;
}

// Opens a file-open dialog and returns the chosen path, or empty if cancelled.
std::string PickVideoFile()
{
	CFileDialog dlg(TRUE, _T("mts"), nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Video Files (*.mts;*.mp4;*.avi;*.mkv;*.mov)")
		_T("|*.mts;*.mp4;*.avi;*.mkv;*.mov|All Files (*.*)|*.*||"));
	if (dlg.DoModal() != IDOK) return {};
	return std::string(CStringA(dlg.GetPathName()));
}

} // namespace

extern std::string g_outputDir; // defined in Video.cpp

// CFrameMonitorApp

BEGIN_MESSAGE_MAP(CFrameMonitorApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CFrameMonitorApp construction

CFrameMonitorApp::CFrameMonitorApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CFrameMonitorApp object

CFrameMonitorApp theApp;

extern int main(int, char**);

// CFrameMonitorApp initialization

BOOL CFrameMonitorApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LaunchConfig cfg = ReadVideoConfig();
	if (cfg.mode == LaunchConfig::None)
	{
		std::string picked = PickVideoFile();
		if (!picked.empty())
		{
			cfg.mode      = LaunchConfig::Video;
			cfg.videoPath = picked;
		}
	}

	INT_PTR nResponse = IDCANCEL;
	if (!cfg.outputDir.empty())
		g_outputDir = cfg.outputDir;
	if (cfg.mode == LaunchConfig::Video)
	{
		const char* szFname[] = { cfg.videoPath.c_str() };
		nResponse = main(1, (char**)szFname);
	}
	else if (cfg.mode == LaunchConfig::Camera)
	{
		nResponse = main(0, (char**)"");
	}
	//CFrameMonitorDlg dlg;
	//m_pMainWnd = &dlg;
	//INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	#if !defined(_AFXDLL)
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

