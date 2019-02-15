#pragma once

#include "stdafx.h"
#include "AppItem.h"

// Single-line, editable text box
class TextBox : public AppItem
{
public:
	using AppItem::AppItem;
	~TextBox();
	void Paint(D2D1_RECT_F updateRect);
	//void OnMouseMove(D2D1_POINT_2F cursor);
	bool OnLButtonDown(D2D1_POINT_2F cursor);
	//void OnLButtonUp(D2D1_POINT_2F cursor);

	std::wstring String();
	

private:
	// state
	std::wstring m_text;
	bool m_update = true; // text layout needs to be recreated
	bool m_active = false;
	bool m_highlight = false; // mouse is down
	int m_caretPos = -1;

	// data
	D2D1_SIZE_F m_size;
	static int const m_bufferSize = 64;
	wchar_t m_lbuffer[m_bufferSize]; // left buffer
	wchar_t m_rbuffer[m_bufferSize]; // right buffer, reversed
	IDWriteTextLayout *m_pTextLayout = NULL; // recreate each edit

	// helpers
	void CreateTextLayout();
};

