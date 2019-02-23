#pragma once

#include "stdafx.h"
#include "resource.h"
#include "BorderlessWindow.h"
#include "D2Objects.h"
#include "TitleBar.h"
#include "FileIO.h"
#include "Chart.h"
#include "Watchlist.h"


class Parthenos : public BorderlessWindow<Parthenos>
{
	// Logical components
	std::vector<AppItem*>	m_activeItems; // items that need to be painted
	std::vector<AppItem*>	m_allItems;
	TitleBar				*m_titleBar;
	Chart					*m_chart;
	Watchlist				*m_watchlist;
	Watchlist				*m_portfolioList;

	// Resource management
	MouseTrackEvents		m_mouseTrack;
	D2Objects				m_d2;
	std::deque<ClientMessage> m_messages;

	// Layout
	float const				m_titleBarHeight = 30.0f;
	float const				m_watchlistWidth = 350.0f;
	float const				m_portfolioListWidth = 500.0f;

	LRESULT	OnCreate();
	LRESULT OnNCHitTest(POINT cursor);
	LRESULT OnPaint();
	LRESULT OnSize(WPARAM wParam);
	LRESULT OnMouseMove(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonDown(POINT cursor, WPARAM wParam);
	LRESULT OnLButtonDblclk(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonUp(POINT cursor, WPARAM wParam);
	LRESULT OnChar(wchar_t c, LPARAM lParam);
	bool	OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);

	void ProcessMessages();
	D2D1_RECT_F CalculateItemRect(AppItem* item, D2D1_RECT_F const & dipRect);

	Parthenos(const Parthenos&) = delete; // non construction-copyable
	Parthenos& operator=(const Parthenos&) = delete; // non copyable
public:

	Parthenos() : BorderlessWindow() {}
	Parthenos(PCWSTR szClassName) : BorderlessWindow(szClassName) {}
	~Parthenos();

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	// From an AppItem
	void SendClientMessage(AppItem *sender, std::wstring msg, CTPMessage imsg) { m_messages.push_front({ sender,msg,imsg }); }
	void PostClientMessage(AppItem *sender, std::wstring msg, CTPMessage imsg) { m_messages.push_back({ sender,msg,imsg }); }
	// Handle initializations here instead of create
	// ClientRect doesn't have correct size during WM_CREATE
	void PreShow(); 

};