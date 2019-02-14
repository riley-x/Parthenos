#pragma once

#include "stdafx.h"
#include "AppItem.h"

class Button : public AppItem
{
public:
	using AppItem::AppItem;

	bool OnLButtonDown(D2D1_POINT_2F cursor);
	void OnLButtonUp(D2D1_POINT_2F cursor) { return; }
protected:
	bool m_isDown = false; // pressed
};

class IconButton : public Button
{
public:
	using Button::Button;
	void Paint(D2D1_RECT_F updateRect);
	void SetIcon(size_t i) { m_iBitmap = i; } // index into d2.pD2DBitmaps
private:
	int m_iBitmap = -1;
};