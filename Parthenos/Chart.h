#pragma once

#include "stdafx.h"
#include "AppItem.h"
#include "DataManaging.h"
#include "Graphing.h"
#include "TextBox.h"
#include "Button.h"

// Main plotting window for stock price charts.
// Includes drawing command bar.
class Chart : public AppItem
{
public:
	Chart(HWND hwnd, D2Objects const & d2);
	~Chart();
	void Init(float leftOffset);
	void Paint(D2D1_RECT_F updateRect);
	void Resize(RECT pRect, D2D1_RECT_F pDipRect);
	void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnLButtonDown(D2D1_POINT_2F cursor);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnChar(wchar_t c, LPARAM lParam);
	bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM wParam, LPARAM lParam);

	void Load(std::wstring ticker, int range = 1260); // # datapoints in days. default to 5 years 
	void ReceiveMessage(std::wstring msg, int i);

	static const float	m_commandSize;
	static const float	m_tickerBoxWidth;
	static const float	m_timeframeWidth;
private:

	// extra parameters
	float				m_menuHeight	= 25.0f;
	float				m_commandHPad	= 5.0f;

	// state
	std::wstring		m_ticker;
	enum class MainChartType { none, line, candlestick, envelope };
	enum class Timeframe { none, month1, month3, month6, year1, year2, year5 };
	MainChartType		m_currentMChart = MainChartType::none;
	Timeframe			m_currentTimeframe = Timeframe::none;

	// data
	std::vector<OHLC>	m_OHLC;
	std::vector<date_t> m_dates;
	std::vector<double> m_closes;
	std::vector<double> m_highs;
	std::vector<double> m_lows;

	// child objects
	Axes				m_axes;
	TextBox				m_tickerBox;
	ButtonGroup			m_chartTypeButtons;
	DropMenuButton		m_timeframeButton;

	// drawing
	D2D1_RECT_F			m_menuRect;
	std::vector<float>	m_divisions;

	// helper functions
	void DrawMainChart(MainChartType type, Timeframe timeframe);
	void DrawSavedState();
	int FindStart(Timeframe timeframe, OHLC* & data);
	void Candlestick(OHLC const *data, int n);
	void Line(OHLC const *data, int n);
	void Envelope(OHLC const *data, int n);
};


