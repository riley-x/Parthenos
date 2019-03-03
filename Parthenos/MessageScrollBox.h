#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "ScrollBar.h"

class MessageScrollBox : public AppItem
{
public:
	MessageScrollBox(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent)
		: AppItem(hwnd, d2), m_scrollBar(hwnd, d2, this), m_parent(parent) 
	{
		// Set 1 line per minStep, 3 lines per detent
		m_scrollBar.SetMinStep(WHEEL_DELTA / 3);
	}

	// AppItem
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnCopy();
	void ProcessCTPMessages();

	// Interface
	void Print(std::wstring const & msg);
	void Clear();

private:
	// Objects
	CTPMessageReceiver  *m_parent;
	ScrollBar			m_scrollBar;

	// Text Data
	std::wstring		m_text;
	IDWriteTextLayout	*m_pTextLayout = NULL; // must recreate each edit
	std::vector<float>	m_linePos; // distance from top of layout to top of line
	DWRITE_TEXT_METRICS m_metrics;
	std::vector<DWRITE_HIT_TEST_METRICS> m_selectionMetrics;

	// State
	size_t				m_visibleLines;
	size_t				m_currLine = 0; // topmost visible line
	bool				m_selection = false; // is actively selecting via mouse?
	size_t				m_iSelectStart; // index into m_text
	size_t				m_iSelectEnd; // set equal to start to indicate nothing selected
	
	// Parameters
	float const			m_titleHeight = 18.0f;

	// Layout
	D2D1_RECT_F			m_titleRect;
	float				m_titleBorderY;
	D2D1_RECT_F			m_layoutRect;

	// Helpers
	void CreateTextLayout();
	int HitTest(D2D1_POINT_2F cursor); // returns the text index under cursor
	void GetSelectionMetrics();
};