#pragma once

#include "stdafx.h"
#include "utilities.h"
#include "D2Objects.h"

class AppItem;

enum class CTPMessage
{
	NONE,
	TITLEBAR_CLOSE, TITLEBAR_MAXRESTORE, TITLEBAR_MIN, TITLEBAR_TAB,
	BUTTON_DOWN, 
	MOUSE_CAPTURED, // < 0 release, >= 0 capture
	TEXTBOX_ENTER, TEXTBOX_DEACTIVATED,
	DROPMENU_SELECTED,
	SCROLLBAR_SCROLL,
	WATCHLISTITEM_NEW, WATCHLISTITEM_EMPTY, WATCHLIST_SELECTED,
	MENUBAR_CALCHOLDINGS, MENUBAR_ACCOUNT, MENUBAR_ADDTRANSACTION,
	WINDOW_ADDTRANSACTION_P
};

// For WINDOW_*_P messages, use sender as pointer to data to pass, since the window is getting destroyed.
typedef struct ClientMessage_struct
{
	AppItem *sender;
	std::wstring msg;
	CTPMessage imsg;
	int iData;
	double dData;
} ClientMessage;


class CTPMessageReceiver
{
public:
	void SendClientMessage(AppItem *sender, std::wstring msg, CTPMessage imsg, int iData = 0, double dData = 0) 
	{
		m_messages.push_front({ sender, msg, imsg, iData, dData });
	}
	void SendClientMessage(ClientMessage const & msg)
	{
		m_messages.push_front(msg);
	}
	void PostClientMessage(AppItem *sender, std::wstring msg, CTPMessage imsg, int iData = 0, double dData = 0) 
	{
		m_messages.push_back({ sender, msg, imsg, iData, dData });
	}
	void PostClientMessage(ClientMessage const & msg)
	{
		m_messages.push_back(msg);
	}
protected:
	virtual void ProcessCTPMessages() { if (!m_messages.empty()) m_messages.clear(); }
	std::deque<ClientMessage> m_messages;
};

class AppItem : public CTPMessageReceiver
{
public:
	AppItem(HWND hwnd, D2Objects const & d2) : m_hwnd(hwnd), m_d2(d2) {}
	virtual void SetSize(D2D1_RECT_F dipRect) // provide item's rect
	{ 
		m_dipRect = dipRect; 
		m_pixRect = DPIScale::DipsToPixels(m_dipRect);
	} 
	virtual void Paint(D2D1_RECT_F updateRect) 
	{ 
		if (overlapRect(m_dipRect, updateRect))
			m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f); 
	}

	// should return true when keyboard captured or mouse handeled (i.e. true if in dipRect to enforce z-order)
	virtual bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled) { return false; }
	virtual bool OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled) { return false; }
	virtual bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled) { return false; } // use 'handled' to respect z-order
	virtual void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam) { return; }
	virtual void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam) { return; }
	virtual bool OnChar(wchar_t c, LPARAM lParam) { return false; }
	virtual bool OnKeyDown(WPARAM wParam, LPARAM lParam) { return false; }
	virtual void OnTimer(WPARAM wParam, LPARAM lParam) { return; }
	virtual bool OnCopy() { return false; }

	D2D1_RECT_F GetDIPRect() const { return m_dipRect; }

protected:
	HWND const		m_hwnd;
	D2Objects const &m_d2;

	RECT			m_pixRect; // pixels in main window client coordinates
	D2D1_RECT_F		m_dipRect = D2D1::RectF(-1, -1, -1, -1);

};