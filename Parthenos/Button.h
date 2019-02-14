#pragma once

#include "stdafx.h"
#include "D2Objects.h"

class Button
{
public:
	void Init(HWND hwnd, D2Objects d2);
	void Resize(D2D1_RECT_F dipRect);

	void OnMouseMove(D2D1_POINT_2F cursor);
	void OnLButtonDown(D2D1_POINT_2F cursor);
	void OnLButtonUp(D2D1_POINT_2F cursor);
	
private:
	HWND m_hwnd;
	D2Objects m_d2;
	D2D1_RECT_F m_dipRect;
	bool m_isDown; // pressed
};

class IconButton
{
public:
	void SetIcon(size_t i); // index into d2.pD2DBitmaps
private:
	int m_iBitmap;
};