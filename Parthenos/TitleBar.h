#pragma once
#include "stdafx.h"
#include "D2Objects.h"

class TitleBar
{
public:
	TitleBar();

	void Paint(D2Objects const & d2);
	void Resize(RECT pRect);
	LRESULT HitTest(D2D1_POINT_2F cursor);

	int left() const { return m_pixRect.left; }
	int top() const { return m_pixRect.top; }
	int right() const { return m_pixRect.right; }
	int bottom() const { return m_pixRect.bottom; }
	int width() const { return m_pixRect.right - m_pixRect.left; }
	int height() const { return m_pixRect.bottom - m_pixRect.top; }

private:
	static int const nIcons = 3;
	static float const iconPad;

	RECT		m_pixRect; // pixels in main window client coordinates
	D2D1_RECT_F m_dipRect; // DIPs in main window client coordinates
	D2D1_RECT_F	m_CommandIconRects[nIcons]; // 24x18 command icons


};