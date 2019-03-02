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
#include "PopupWindow.h"
#include "PieChart.h"


typedef struct Account_struct
{
	std::wstring name;
	std::vector<Position> positions;
	std::vector<date_t> histDate;
	std::vector<double> histEquity;
	std::vector<std::pair<double, D2D1_COLOR_F>> returnsBarData; // in equity order
	std::vector<std::pair<double, D2D1_COLOR_F>> returnsPercBarData; // in equity order
	std::vector<std::wstring> tickers; // in equity order
} Account;

class Parthenos : public BorderlessWindow<Parthenos>, public CTPMessageReceiver
{
public:

	Parthenos(PCWSTR szClassName);
	~Parthenos();

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	// Handle initializations here instead of create
	// ClientRect doesn't have correct size during WM_CREATE
	void PreShow();

private:
	// Data
	int	m_currAccount = 0;
	std::vector<std::wstring>				m_accountNames;
	std::vector<Account>					m_accounts; // same order as m_accountNames; extra entry at end for 'All'
	std::vector<std::wstring>				m_tickers; // tickers in all accounts, sorted by name
	std::vector<std::pair<Quote, Stats>>	m_stats; // in same order as m_tickers

	// 'Child' windows
	AddTransactionWindow	*m_addTWin = nullptr;

	// Logical components
	std::vector<AppItem*>	m_activeItems; // items that need to be painted
	std::vector<AppItem*>	m_allItems;
	TitleBar				*m_titleBar;
	Chart					*m_chart;
	Watchlist				*m_watchlist;
	Watchlist				*m_portfolioList;
	MenuBar					*m_menuBar;
	PieChart				*m_pieChart;
	Axes					*m_eqHistoryAxes;
	Axes					*m_returnsAxes;
	Axes					*m_returnsPercAxes;

	// Resource management
	MouseTrackEvents		m_mouseTrack;
	D2Objects				m_d2;
	Timers::WndTimers		m_timers;

	// Layout
	float const	m_titleBarHeight		= 30.0f;
	float const	m_menuBarHeight			= 20.0f;
	float const	m_watchlistWidth		= 350.0f;
	float const	m_portfolioListWidth	= 600.0f;
	float		m_titleBarBottom; // snapped to pixel boundry
	float		m_menuBarBottom;
	float		m_watchlistRight;
	float		m_portfolioListRight;
	std::vector<Line_t> m_dividingLines;

	// Flags and state
	bool m_sizeChanged = false; // For tab changes
	enum Tabs { TPortfolio, TReturns, TChart };
	Tabs m_tab = TChart;

	// Data management
	int AccountToIndex(std::wstring account);
	NestedHoldings CalculateHoldings() const;
	void CalculatePositions(NestedHoldings const & holdings);
	void CalculateReturns();
	void CalculateHistories();
	void AddTransaction(Transaction t);

	// Child management
	void ProcessCTPMessages();
	D2D1_RECT_F CalculateItemRect(AppItem* item, D2D1_RECT_F const & dipRect);
	void CalculateDividingLines(D2D1_RECT_F dipRect);
	void LoadPieChart();
	void UpdatePortfolioPlotters(char account, bool init = false);

	// Message responses
	LRESULT	OnCreate();
	LRESULT OnNCHitTest(POINT cursor);
	LRESULT OnSize(WPARAM wParam);
	LRESULT OnPaint();
	LRESULT OnMouseMove(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonDown(POINT cursor, WPARAM wParam);
	LRESULT OnLButtonDblclk(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonUp(POINT cursor, WPARAM wParam);
	LRESULT OnChar(wchar_t c, LPARAM lParam);
	bool	OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);

	// Deleted
	Parthenos(const Parthenos&) = delete; // non construction-copyable
	Parthenos& operator=(const Parthenos&) = delete; // non copyable
};