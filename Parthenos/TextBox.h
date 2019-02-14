#pragma once

#include "stdafx.h"
#include "AppItem.h"

class TextBox : public AppItem
{
public:
	using AppItem::AppItem;

	void OnMouseMove(D2D1_POINT_2F cursor) { return; }
	void OnLButtonDown(D2D1_POINT_2F cursor) { return; }
	void OnLButtonUp(D2D1_POINT_2F cursor) { return; }
private:
	
	bool m_highlight; // mouse is down
};

