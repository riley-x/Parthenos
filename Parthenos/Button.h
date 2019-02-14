#pragma once

#include "stdafx.h"
#include "AppItem.h"

class Button : public AppItem
{
public:
	using AppItem::AppItem;

	void OnLButtonDown(D2D1_POINT_2F cursor) { return; }
	void OnLButtonUp(D2D1_POINT_2F cursor) { return; }
protected:
	bool m_isDown; // pressed
};

class IconButton : public Button
{
public:
	using Button::Button;
	void SetIcon(size_t i); // index into d2.pD2DBitmaps
private:
	int m_iBitmap;
};