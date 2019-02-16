#pragma once

#include "stdafx.h"
#include "AppItem.h"


class PopupMenu : public AppItem
{
public:
	using AppItem::AppItem;
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnLButtonDown(D2D1_POINT_2F cursor);


	void SetItems(std::vector<std::wstring> items);
};