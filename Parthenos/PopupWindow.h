#pragma once
#include "stdafx.h"
#include "BorderlessWindow.h"
#include "utilities.h"
#include "D2Objects.h"
#include "AppItem.h"
#include "TitleBar.h"

class Parthenos;


class AddTransactionWindow : public BorderlessWindow<AddTransactionWindow>, public CTPMessageReceiver
{
public:
	AddTransactionWindow(PCWSTR szClassName) : BorderlessWindow(szClassName) {}
	~AddTransactionWindow() { for (auto item : m_items) if (item) delete item; }

	BOOL Create(HINSTANCE hInstance);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void PreShow();

private:

	// Flags
	bool m_created = false; // only allow create once

	// Objects
	std::vector<AppItem*>	m_items;
	TitleBar				*m_titleBar;

	// Resources
	MouseTrackEvents		m_mouseTrack;
	D2Objects				m_d2;

	// Layout
	float const	m_titleBarHeight = 30.0f;

	// Message responses
	LRESULT	OnCreate();
	LRESULT OnNCHitTest(POINT cursor);
	LRESULT OnPaint();
	LRESULT OnMouseMove(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonDown(POINT cursor, WPARAM wParam);
	LRESULT OnLButtonDblclk(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonUp(POINT cursor, WPARAM wParam);
	LRESULT OnChar(wchar_t c, LPARAM lParam);
	bool	OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
	
	void	ProcessCTPMessages();
	D2D1_RECT_F CalculateItemRect(AppItem* item, D2D1_RECT_F const & dipRect);

};