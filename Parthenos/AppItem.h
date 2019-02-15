#pragma once

#include "stdafx.h"
#include "utilities.h"
#include "D2Objects.h"

class AppItem
{
public:
	AppItem(HWND hwnd, D2Objects const & d2) : m_hwnd(hwnd), m_d2(d2) {}
	virtual void Init() { return; }
	virtual void Resize(RECT pRect, D2D1_RECT_F pDipRect) { return; } // provide parent rect
	virtual void SetSize(D2D1_RECT_F dipRect) // provide item's rect
	{ 
		m_dipRect = dipRect; 
		m_pixRect = DPIScale::DipsToPixels(m_dipRect);
	} 
	virtual void Paint(D2D1_RECT_F updateRect) 
	{ 
		m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f); 
	}

	virtual void OnMouseMove(D2D1_POINT_2F cursor) { return; }
	virtual bool OnLButtonDown(D2D1_POINT_2F cursor) { return false; }
	virtual void OnLButtonUp(D2D1_POINT_2F cursor) { return; }
	virtual bool OnChar(wchar_t c, LPARAM lParam) { return false; }
	virtual bool OnKeyDown(WPARAM wParam, LPARAM lParam) { return false; }

	virtual void ReceiveMessage(std::wstring msg, int i) { return; } 

	D2D1_RECT_F GetDIPRect() const { return m_dipRect; }

protected:
	HWND const m_hwnd;
	D2Objects const &m_d2;

	RECT		m_pixRect; // pixels in main window client coordinates
	D2D1_RECT_F m_dipRect;
};