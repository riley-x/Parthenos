#pragma once

#include "../stdafx.h"
#include "AppItem.h"


class Chart;

// Single-line, editable text box
// Don't use buffer because need to convert to wstring anyways to display
class TextBox : public AppItem
{
public:
	TextBox(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent, 
		D2Objects::Formats format = D2Objects::Segoe12, float leftOffset = 2.0f, bool border = true) :
		AppItem(hwnd, d2, parent), m_format(format), m_leftOffset(leftOffset), m_border(border) {};
	~TextBox();
	
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnChar(wchar_t c, LPARAM lParam);
	bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM wParam, LPARAM lParam);

	inline std::wstring String() const { return m_text; }
	void SetText(std::wstring const & text);
	inline void SetParameters(float leftOffset, bool border, size_t maxChars = 9) 
	{
		m_leftOffset = leftOffset; m_border = border; m_maxChars = maxChars;
	}
	void Activate();
	void Deactivate(bool message = false);

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

	// paramters
	bool				m_border;
	float				m_leftOffset;
	size_t				m_maxChars = 9;
	D2Objects::Formats	m_format;

	// data
	IDWriteTextLayout	*m_pTextLayout = NULL; // must recreate each edit

	// helpers
	void CreateTextLayout();
	void MoveCaret(int i); // move right by i
	void DeleteSelection(bool invalidate = true);

	// deleted
	TextBox(const TextBox&) = delete; // non construction-copyable
	TextBox& operator=(const TextBox&) = delete; // non copyable

};

