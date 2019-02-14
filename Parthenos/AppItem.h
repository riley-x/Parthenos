#pragma once

#include "stdafx.h"
#include "D2Objects.h"

class AppItem
{
public:
	AppItem(HWND hwnd, D2Objects const & d2) : m_hwnd(hwnd), m_d2(d2) {}
	virtual void Init() { return; }
	virtual void Resize(RECT pRect, D2D1_RECT_F pDipRect) { return; }
	virtual void Paint() { return; }

	virtual void OnMouseMove(D2D1_POINT_2F cursor) { return; }
	virtual void OnLButtonDown(D2D1_POINT_2F cursor) { return; }
	virtual void OnLButtonUp(D2D1_POINT_2F cursor) { return; }

protected:
	HWND const m_hwnd;
	D2Objects const &m_d2;
	D2D1_RECT_F m_dipRect;
};