#pragma once

#include "stdafx.h"
#include "AppItem.h"

class TextBox : public AppItem
{
public:
	using AppItem::AppItem;

	void Paint(D2D1_RECT_F updateRect);
	//void OnMouseMove(D2D1_POINT_2F cursor);
	//void OnLButtonDown(D2D1_POINT_2F cursor);
	//void OnLButtonUp(D2D1_POINT_2F cursor);
	
private:
	bool m_highlight; // mouse is down
};

