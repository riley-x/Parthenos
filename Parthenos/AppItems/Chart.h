#pragma once

#include "../stdafx.h"
#include "AppItem.h"
#include "../Utilities/DataManaging.h"
#include "Graphing.h"
#include "TextBox.h"
#include "Button.h"

// Main plotting window for stock price charts.
// Includes drawing command bar.
class Chart : public AppItem
{
public:
	// Enums
	enum class MainChartType { none, line, candlestick, envelope };
	enum class Timeframe { none, month1, month3, month6, year1, year2, year5, custom };
	enum Markers : size_t { MARK_HISTORY, MARK_NMARKERS };

	// AppItem overrides
	Chart(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent);
	~Chart();
	void Paint(D2D1_RECT_F updateRect);
	void SetSize(D2D1_RECT_F dipRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnChar(wchar_t c, LPARAM lParam);
	bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM wParam, LPARAM lParam);
	void ProcessCTPMessages();

	// Interface
	void Draw(std::wstring ticker); // uses the chart's saved state
	inline void LoadHistory(std::vector<Transaction> const & hist) { m_history = hist; }

private:
	// Statics
	static const float	m_commandSize;
	static const float	m_tickerBoxWidth;
	static const float	m_timeframeWidth;

	// Parameters
	float				m_menuHeight	= 26.0f;
	float				m_commandHPad	= 5.0f;
	const std::wstring  m_markerNames[MARK_NMARKERS] = { L"H" };

	// State
	std::wstring		m_ticker;
	MainChartType		m_currentMChart = MainChartType::none;
	Timeframe			m_currentTimeframe = Timeframe::none;
	date_t				m_startDate; // for custom timeframe
	date_t				m_endDate;
	bool				m_markerActive[MARK_NMARKERS] = {};

	// Data
	std::vector<OHLC>			m_ohlc;
	std::vector<Transaction>	m_history;

	// Child objects
	Axes				m_axes;
	TextBox				m_tickerBox;
	DropMenuButton		m_timeframeButton;
	ButtonGroup			m_chartTypeButtons;
	ButtonGroup			m_mouseTypeButtons;
	TextButton*			m_markerButtons[MARK_NMARKERS];

	// Painting
	D2D1_RECT_F			m_menuRect;
	std::vector<float>	m_divisions;

	// Helper functions
	void Load(std::wstring ticker, int range = 1260); // # datapoints in days. default to 5 years 
	void DrawMainChart(MainChartType type, Timeframe timeframe);
	void DrawCurrentState();
	int FindStart(Timeframe timeframe, std::vector<OHLC>::iterator & it);
	void Candlestick(std::vector<OHLC>::iterator start, int n);
	void Line(std::vector<OHLC>::iterator start, int n);
	void Envelope(std::vector<OHLC>::iterator start, int n);
	void DrawMarker(Markers i);
	void DrawHistory();
};


