#pragma once

#include "stdafx.h"
#include "AppItem.h"


class Chart;

// Single-line, editable text box
// Don't use buffer because need to convert to wstring anyways to display
class TextBox : public AppItem
{
public:
	TextBox(HWND hwnd, D2Objects const & d2, AppItem *chart) :
		AppItem(hwnd, d2), m_parent(chart) {};
	~TextBox();
	
	void Paint(D2D1_RECT_F updateRect);
	void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnLButtonDown(D2D1_POINT_2F cursor);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnChar(wchar_t c, LPARAM lParam);
	bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM wParam, LPARAM lParam);

	std::wstring String() const;
	void SetText(std::wstring text);
	void Activate();
	void Deactivate();

private:
	// state
	std::wstring		m_text;

	bool				m_active = false; // enter edit mode with caret
	int					m_ipos = -1; // caret pos in text
	float				m_fpos = -1.0f; // caret pos in parent client coordinates
	bool				m_flash = true; // should caret be shown

	bool				m_selection = false; // highlight a selection
	bool				m_mouseSelection = false; // is selection via mouse?
	int					m_istart; // for selection
	float				m_fstart; // for selection

	bool				m_mouseOver = false;

	// paramters
	float				m_leftOffset = 2.0f;
	size_t				m_maxChars = 9;

	// data
	AppItem				*m_parent;
	IDWriteTextLayout	*m_pTextLayout = NULL; // must recreate each edit

	// helpers
	void CreateTextLayout();
	void MoveCaret(int i); // move right by i
	void DeleteSelection(bool invalidate = true);
};

