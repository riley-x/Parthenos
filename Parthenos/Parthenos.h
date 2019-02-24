#pragma once

#include "stdafx.h"
#include "resource.h"
#include "BorderlessWindow.h"
#include "D2Objects.h"
#include "TitleBar.h"
#include "FileIO.h"
#include "Chart.h"
#include "Watchlist.h"
#include "MenuBar.h"


class Parthenos : public BorderlessWindow<Parthenos>
{
	// Logical components
	std::vector<AppItem*>	m_activeItems; // items that need to be painted
	std::vector<AppItem*>	m_allItems;
	TitleBar				*m_titleBar;
	Chart					*m_chart;
	Watchlist				*m_watchlist;
	Watchlist				*m_portfolioList;
	MenuBar					*m_menuBar;

	// Resource management
	MouseTrackEvents			m_mouseTrack;
	D2Objects					m_d2;
	std::deque<ClientMessage>	m_messages;

	// Data
	int	m_currAccount = 0;
	std::vector<std::wstring>			m_accounts;
	std::vector<std::vector<Position>>	m_positions; // same order as m_accounts; extra entry at end for 'All'

	// Layout
	float const	m_titleBarHeight		= 30.0f;
	float const	m_menuBarHeight			= 17.0f;
	float const	m_watchlistWidth		= 350.0f;
	float const	m_portfolioListWidth	= 600.0f;

	// Message responses
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

	// Helpers
	int AccountToIndex(std::wstring account);
	void ProcessAppItemMessages();
	D2D1_RECT_F CalculateItemRect(AppItem* item, D2D1_RECT_F const & dipRect);

	// Deleted
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