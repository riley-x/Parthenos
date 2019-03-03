#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "ScrollBar.h"

class MessageScrollBox : public AppItem
{
public:
	MessageScrollBox(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent)
		: AppItem(hwnd, d2), m_scrollBar(hwnd, d2, this), m_parent(parent) {};

	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);

	void ProcessCTPMessages();


private:
	CTPMessageReceiver *m_parent;

	ScrollBar m_scrollBar;

	int m_scrollCapturedMouse = -1; 
};