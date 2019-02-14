#pragma once
#include "stdafx.h"
#include "AppItem.h"

class TitleBar : public AppItem
{
public:
	using AppItem::AppItem;
	void Init();
	void Paint(D2D1_RECT_F updateRect);
	void Resize(RECT pRect, D2D1_RECT_F pDipRect);
	LRESULT HitTest(POINT cursor);
	void Maximize(bool isMax) { m_maximized = isMax; }
	void MouseOn(LRESULT button);

	static float const height; // 30 DIPs

	int bottom() { return m_pixRect.bottom; }

private:
	static int const	nIcons		= 3;
	static float const	iconHPad;
	bool				m_maximized = false;
	LRESULT				m_mouseOn	= HTNOWHERE; // for icon highlighting

	D2D1_RECT_F	m_CommandIconRects[nIcons]; // 32x24 command icons. These are PIXEL coordinates, not DIPs!!!
	D2D1_RECT_F m_TitleIconRect; // 48x48 Parthenos icon. In DIP coordinates.


};