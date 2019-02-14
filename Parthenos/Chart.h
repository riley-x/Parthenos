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
	Chart(HWND hwnd, D2Objects & d2, float leftOffset);
	void Load(std::wstring ticker, int range = 1260); // range in days. default to 5 years 
	void Paint();
	void Resize(RECT pRect);
	void OnLButtonDown(D2D1_POINT_2F cursor);

private:
	// base
	HWND				m_hwnd;
	D2D1_RECT_F			m_dipRect; // DIPs in main window client coordinates
	D2Objects			&m_d2;

	// parameters
	float				m_menuHeight = 25.0f;

	// data
	std::vector<OHLC>	m_OHLC;
	std::vector<date_t> m_dates;
	std::vector<double> m_closes;
	std::vector<double> m_highs;
	std::vector<double> m_lows;

	// drawing
	enum class MainChartType { none, line, candlestick, envelope };
	enum class Timeframe { none, year1 };
	Axes				m_axes;
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


