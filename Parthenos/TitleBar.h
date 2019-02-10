#pragma once
#include "stdafx.h"
#include "D2Objects.h"

class TitleBar
{
public:
	void Init();
	void Paint(D2Objects const & d2);
	void Resize(RECT pRect);
	LRESULT HitTest(POINT cursor);
	void Maximize(bool isMax) { m_maximized = isMax; }
	void MouseOn(LRESULT button, HWND p_hwnd);

	static float const height; // 30 DIPs

	int bottom() { return m_pixRect.bottom; }

private:
	static int const	nIcons		= 3;
	static float const	iconHPad;
	bool				m_maximized = false;
	LRESULT				m_mouseOn	= HTNOWHERE; // for icon highlighting

	RECT		m_pixRect; // pixels in main window client coordinates
	D2D1_RECT_F m_dipRect; // DIPs in main window client coordinates
	D2D1_RECT_F	m_CommandIconRects[nIcons]; // 32x24 command icons. These are PIXEL coordinates, not DIPs!!!
	D2D1_RECT_F m_TitleIconRect; // 48x48 Parthenos icon. In DIP coordinates.


};