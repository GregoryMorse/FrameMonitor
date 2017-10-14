// TimelineStatic.cpp : implementation file
//

#include "stdafx.h"
#include "FrameRepeat.h"
#include "TimelineStatic.h"


// CTimelineStatic

IMPLEMENT_DYNAMIC(CTimelineStatic, CStatic)

CTimelineStatic::CTimelineStatic()
{

}

CTimelineStatic::~CTimelineStatic()
{
}


BEGIN_MESSAGE_MAP(CTimelineStatic, CStatic)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()



// CTimelineStatic message handlers


void CTimelineStatic::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	HBITMAP hbit = CreateCompatibleBitmap(lpDrawItemStruct->hDC, m_motionDetected.size(), 1);
	HDC hdc = CreateCompatibleDC(lpDrawItemStruct->hDC);
	SelectObject(hdc, hbit);
	for (int i = 0; i < m_motionDetected.size(); i++) {
		//red - 255, 0, 0, yellow - 255, 255, 0, white - 255, 255, 255
		double pct = m_motionDetected[i] / m_dMaxContourSize;
		SetPixel(hdc, i, 0, RGB(255, 255 * (pct > 0.5 ? (pct * 2 - 1) : 1), pct > 0.5 ? 0 : 255 * (1 - pct * 2)));
	}
	StretchBlt(lpDrawItemStruct->hDC, 0, 3,
		lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left,
		lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - 6,
		hdc, 0, 0, m_motionDetected.size(), 1, SRCCOPY);
	POINT pt;
	RECT r = { 0, 0, (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left), 3 };
	FillRect(lpDrawItemStruct->hDC, &r, (HBRUSH)HOLLOW_BRUSH);
	r.top = lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - 3;
	r.bottom = lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top;
	FillRect(lpDrawItemStruct->hDC, &r, (HBRUSH)HOLLOW_BRUSH);
	if (m_motionDetected.size() != 0) {
		MoveToEx(lpDrawItemStruct->hDC, (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left) * m_iFrameNum / m_motionDetected.size(), 0, &pt);
		LineTo(lpDrawItemStruct->hDC, (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left) * m_iFrameNum / m_motionDetected.size(), 3);
		MoveToEx(lpDrawItemStruct->hDC, (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left) * m_iFrameNum / m_motionDetected.size(), lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - 1, &pt);
		LineTo(lpDrawItemStruct->hDC, (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left) * m_iFrameNum / m_motionDetected.size(), lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - 4);
		RECT rect = { (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left) * m_iFrameNum / m_motionDetected.size(), 3,
			(lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left) * m_iFrameNum / m_motionDetected.size(), lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - 3 };
		if (rect.left != 0) rect.left--;
		if (rect.right != (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left)) rect.right++;
		InvertRect(lpDrawItemStruct->hDC, &rect);
	}
	DeleteDC(hdc);
}


void CTimelineStatic::OnLButtonDown(UINT nFlags, CPoint point)
{
	RECT rect;
	GetWindowRect(&rect);
	ScreenToClient(&rect);
	GetParent()->PostMessage(WM_CUSTOM_MOVEVIDEO, 0, m_iFrameNum = m_motionDetected.size() * point.x / (rect.right - rect.left + 1));
	Invalidate();
	CStatic::OnLButtonDown(nFlags, point);
}
