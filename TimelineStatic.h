#pragma once

#include <vector>
#define WM_CUSTOM_MOVEVIDEO WM_USER + 2

// CTimelineStatic

class CTimelineStatic : public CStatic
{
	DECLARE_DYNAMIC(CTimelineStatic)

public:
	CTimelineStatic();
	virtual ~CTimelineStatic();

protected:
	DECLARE_MESSAGE_MAP()

public:
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	std::vector<double> m_motionDetected;
	double m_dMaxContourSize;
	int m_iFrameNum;
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};


