#pragma once

#include "stdafx.h"
#include "utilities.h"
#include "D2Objects.h"

class AppItem;

enum class CTPMessage
{
	NONE,
	TITLEBAR_CLOSE, TITLEBAR_MAXRESTORE, TITLEBAR_MIN, TITLEBAR_TAB,
	TEXTBOX_ENTER, TEXTBOX_DEACTIVATED,
	DROPMENU_SELECTED,
	WATCHLISTITEM_NEW, WATCHLISTITEM_EMPTY, WATCHLIST_SELECTED,
	MENUBAR_ACCOUNT
};

typedef struct ClientMessage_struct
{
	AppItem *sender;
	std::wstring msg;
	CTPMessage imsg;
} ClientMessage;

class AppItem
{
public:
	AppItem(HWND hwnd, D2Objects const & d2) : m_hwnd(hwnd), m_d2(d2) {}
	virtual void Init() { return; }
	virtual void SetSize(D2D1_RECT_F dipRect) // provide item's rect
	{ 
		m_dipRect = dipRect; 
		m_pixRect = DPIScale::DipsToPixels(m_dipRect);
	} 
	virtual void Paint(D2D1_RECT_F updateRect) 
	{ 
		m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f); 
	}

	// generally, should return true when mouse/keyboard captured
	virtual void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam) { return; }
	virtual bool OnLButtonDown(D2D1_POINT_2F cursor) { return false; }
	virtual void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam) { return; }
	virtual void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam) { return; }
	virtual bool OnChar(wchar_t c, LPARAM lParam) { return false; }
	virtual bool OnKeyDown(WPARAM wParam, LPARAM lParam) { return false; }
	virtual void OnTimer(WPARAM wParam, LPARAM lParam) { return; }

	virtual void PostClientMessage(AppItem *sender, std::wstring msg, CTPMessage imsg) { m_messages.push_back({ sender,msg,imsg }); }
	virtual void ProcessMessages() { if (!m_messages.empty()) m_messages.clear(); }

	D2D1_RECT_F GetDIPRect() const { return m_dipRect; }

protected:
	HWND const		m_hwnd;
	D2Objects const &m_d2;

	RECT			m_pixRect; // pixels in main window client coordinates
	D2D1_RECT_F		m_dipRect = D2D1::RectF(-1, -1, -1, -1);

	std::deque<ClientMessage> m_messages;
};