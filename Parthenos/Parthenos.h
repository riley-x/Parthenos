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

	// Resource management
	MouseTrackEvents		m_mouseTrack;
	D2Objects				m_d2;

	// Data memebers
	float m_leftPanelWidth = 350.0f; // in DIPs

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

	Parthenos(const Parthenos&) = delete; // non construction-copyable
	Parthenos& operator=(const Parthenos&) = delete; // non copyable
public:

	Parthenos() : BorderlessWindow() {}
	Parthenos(PCWSTR szClassName) : BorderlessWindow(szClassName) {}
	~Parthenos();

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	// Handle initializations here instead of create
	// ClientRect doesn't have correct size during WM_CREATE
	void PreShow(); 

};