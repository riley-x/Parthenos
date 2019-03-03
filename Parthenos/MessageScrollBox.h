#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "ScrollBar.h"

class MessageScrollBox : public AppItem
{
public:
	MessageScrollBox(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent)
		: AppItem(hwnd, d2), m_scrollBar(hwnd, d2, this), m_parent(parent) {}

	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);

	void ProcessCTPMessages();


private:
	// Objects
	CTPMessageReceiver  *m_parent;
	ScrollBar			m_scrollBar;

	// Text
	std::wstring		m_text;
	IDWriteTextLayout	*m_pTextLayout = NULL; // must recreate each edit
	DWRITE_TEXT_METRICS m_metrics;
	std::vector<float>	m_linePos; // distance from top of layout to top of line

	// State
	size_t				m_visibleLines;
	size_t				m_lineStart = 0;
	size_t				m_lineEnd = 0; // exclusive

	// Selection
	//bool				m_selection = false; // is selection via mouse?
	//int					m_istart; // for selection
	//float				m_fstart; // for selection
	int					m_scrollCapturedMouse = -1; // < 0 if not captured, > 0 if captured
	
	// Drawing
	D2D1_RECT_F			m_layoutRect;

	// Helpers
	void CreateTextLayout();
};