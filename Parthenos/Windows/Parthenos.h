#pragma once

#include "../stdafx.h"
#include "../resource.h"
#include "BorderlessWindow.h"
#include "PopupWindow.h"
#include "../Utilities/D2Objects.h"
#include "../AppItems/TitleBar.h"
#include "../AppItems/Chart.h"
#include "../AppItems/Watchlist.h"
#include "../AppItems/MenuBar.h"
#include "../AppItems/PieChart.h"
#include "../AppItems/MessageScrollBox.h"


struct Account
{
	std::wstring name;
	std::vector<Position> positions;
	std::vector<date_t> histDate; // equity history dates
	std::vector<double> histEquity; // equity history values, size == histDate
	std::vector<std::pair<double, D2D1_COLOR_F>> returnsBarData; // in stock equity order
	std::vector<std::pair<double, D2D1_COLOR_F>> returnsPercBarData; // in stock equity order
	std::vector<std::wstring> tickers; // in equity order
	std::vector<std::wstring> percTickers; // for % return graph
};

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
	char									m_currAccount = 0;
	std::vector<std::wstring>				m_accountNames;
	std::vector<Account>					m_accounts; // same order as m_accountNames; extra entry at end for 'All'
	std::vector<std::wstring>				m_tickers; // tickers in all accounts, sorted by name
	std::vector<D2D1_COLOR_F>				m_tickerColors; // colors for each ticker in m_tickers, same order
	std::vector<std::pair<Quote, Stats>>	m_stats; // in same order as m_tickers

	// 'Child' windows
	std::unordered_set<PopupWindow*> m_childWindows;

	// Logical components
	std::vector<AppItem*>	m_activeItems; // items that need to be painted
	std::vector<AppItem*>	m_allItems;
	TitleBar				*m_titleBar			= nullptr;
	Chart					*m_chart			= nullptr;
	Watchlist				*m_watchlist		= nullptr;
	Watchlist				*m_portfolioList	= nullptr;
	MenuBar					*m_menuBar			= nullptr;
	PieChart				*m_pieChart			= nullptr;
	Axes					*m_eqHistoryAxes	= nullptr;
	Axes					*m_returnsAxes		= nullptr;
	Axes					*m_returnsPercAxes	= nullptr;
	MessageScrollBox		*m_msgBox			= nullptr;

	// Resource management
	MouseTrackEvents		m_mouseTrack;
	D2Objects				m_d2;
	Timers::WndTimers		m_timers;

	// Layout
	float const	m_titleBarHeight		= 30.0f;
	float const	m_menuBarHeight			= 20.0f;
	float const	m_watchlistWidth		= 350.0f;
	float const	m_portfolioListWidth	= 720.0f;
	float		m_titleBarBottom		= 0; // snapped to pixel boundry
	float		m_menuBarBottom			= 0;
	float		m_watchlistRight		= 0;
	float		m_portfolioListRight	= 0;
	float		m_halfBelowMenu			= 0;
	float		m_centerX				= 0;
	std::vector<Line_t> m_dividingLines;

	// Flags and state
	bool m_sizeChanged = false; // For tab changes
	enum Tabs { TPortfolio, TReturns, TChart };
	Tabs m_tab = TChart;
	AppItem *m_mouseCaptured = nullptr;

	// Data management
	void InitData();
	void InitRealtimeData();
	int AccountToIndex(std::wstring account);
	std::vector<Holdings> CalculateHoldings() const;
	void CalculatePositions(std::vector<Holdings> const & holdings);
	void CalculateReturns();
	void CalculateHistories();
	void CalculateAllHistory();
	std::vector<TimeSeries> GetHist(size_t i);
	void AddTransaction(Transaction t);
	void PrintDividendSummary() const;
	void PrintOptionSummary() const;
	std::vector<Transaction> GetTrans() const;

	// Child management
	void InitItems();
	void InitItemsWithRealtimeData();
	void ProcessCTPMessages();
	void ProcessMenuMessage(bool & pop_front);
	D2D1_RECT_F CalculateItemRect(AppItem* item, D2D1_RECT_F const & dipRect);
	void CalculateDividingLines(D2D1_RECT_F dipRect);
	void LoadPieChart();
	void UpdatePortfolioPlotters(char account, bool init = false);
	std::wstring LaunchFileDialog(bool save = false);

	// Message responses
	LRESULT	OnCreate();
	LRESULT OnNCHitTest(POINT cursor);
	LRESULT OnSize(WPARAM wParam);
	LRESULT OnPaint();
	LRESULT OnMouseMove(POINT cursor, WPARAM wParam);
	LRESULT OnMouseWheel(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonDown(POINT cursor, WPARAM wParam);
	LRESULT OnLButtonDblclk(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonUp(POINT cursor, WPARAM wParam);
	LRESULT OnChar(wchar_t c, LPARAM lParam);
	bool	OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
	void	OnCopy();

	// Deleted
	Parthenos(const Parthenos&) = delete; // non construction-copyable
	Parthenos& operator=(const Parthenos&) = delete; // non copyable
};