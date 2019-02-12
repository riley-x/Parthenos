#pragma once

#include "stdafx.h"
#include "D2Objects.h"
#include "DataManaging.h"
#include "Graphing.h"



// Main plotting window for stock price charts.
// Includes drawing command bar.
class Chart
{
public:
	void Init(HWND hwndParent, float leftOffset);
	void Load(std::wstring ticker, int range = 1260); // range in days. default to 5 years 
	void Paint(D2Objects const & d2);
	void Resize(RECT pRect);
	bool OnLButtonDown(D2D1_POINT_2F cursor);

private:
	// base
	HWND				m_hwndParent;
	D2D1_RECT_F			m_dipRect; // DIPs in main window client coordinates

	// data
	std::vector<OHLC>	m_OHLC;
	std::vector<double> m_closes; // todo turn this into a vector of lines, with colors?

	// drawing
	enum class MainChartType { none, line, candlestick, envelope };
	enum class Timeframe { none, year1 };
	Axes				m_axes;
	MainChartType		m_currentMChart = MainChartType::none;
	Timeframe			m_currentTimeframe = Timeframe::none;

	void DrawMainChart(MainChartType type, Timeframe timeframe);
	void DrawSavedState();
	int FindStart(Timeframe timeframe, OHLC* & data);
	void Candlestick(Timeframe timeframe);
	void Line(Timeframe timeframe);
	void Envelope(std::vector<OHLC> const & ohlc);
};


