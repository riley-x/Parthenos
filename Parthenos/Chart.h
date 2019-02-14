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
	void Load(std::wstring ticker, int range = 1260); // range in days. default to 5 years 
	void Paint(D2D1_RECT_F updateRect);
	void Resize(RECT pRect, D2D1_RECT_F pDipRect);
	void OnLButtonDown(D2D1_POINT_2F cursor);

private:

	// parameters
	float				m_menuHeight = 25.0f;

	// data
	std::vector<OHLC>	m_OHLC;
	std::vector<date_t> m_dates;
	std::vector<double> m_closes;
	std::vector<double> m_highs;
	std::vector<double> m_lows;

	// child objects
	Axes				m_axes;
	TextBox				m_tickerBox;
	std::vector<IconButton*> m_iconButtons;

	// drawing
	D2D1_RECT_F			m_menuRect;
	enum class MainChartType { none, line, candlestick, envelope };
	enum class Timeframe { none, year1 };
	MainChartType		m_currentMChart = MainChartType::none;
	Timeframe			m_currentTimeframe = Timeframe::none;

	// helper functions
	void DrawMainChart(MainChartType type, Timeframe timeframe);
	void DrawSavedState();
	int FindStart(Timeframe timeframe, OHLC* & data);
	void Candlestick(Timeframe timeframe);
	void Line(Timeframe timeframe);
	void Envelope(Timeframe timeframe);
};


