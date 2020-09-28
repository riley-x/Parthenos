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
	static const size_t	m_nStudies = 3;

	// Parameters
	float				m_menuHeight	= 26.0f;
	float				m_commandHPad	= 5.0f;
	const std::wstring  m_markerNames[MARK_NMARKERS] = { L"H" };
	const std::wstring	m_studyNames[m_nStudies] = { L"SMA20", L"SMA100", L"RSI" };
	const float			m_studyWidths[m_nStudies] = { 50.0f, 60.0f, 35.0f }; // Manually picked since these don't change dynamically
	const D2D1_COLOR_F	m_studyColors[m_nStudies] = { D2D1::ColorF(0x2777d9), D2D1::ColorF(0xc28030), D2D1::ColorF(0x6417c2) };
		// Solid blue, dull orange, deep purple
	const float			m_auxAxisHeightFrac = 0.20f;


	// State
	std::wstring		m_ticker;
	MainChartType		m_currentMChart = MainChartType::none;
	Timeframe			m_currentTimeframe = Timeframe::none;
	date_t				m_startDate = 0; // First and last dates plotted. These are set by AXES_SELECTION or DrawMainChart.
	date_t				m_endDate = 0; // Inclusive
	bool				m_markerActive[MARK_NMARKERS] = {};
	bool				m_studyActive[m_nStudies] = {};
	std::vector<bool>	m_activeAxes; // Size == m_auxAxes. Always reuse from lowest index.

	// Data
	std::vector<OHLC>			m_ohlc; // Set exclusively by Load(), and ATM always 1260 days
	std::vector<Transaction>	m_history;

	// Child objects
	Axes				m_axes;
	std::vector<Axes*>	m_auxAxes; // For volume, etc. These are reused, see m_activeAxes.
	TextBox				m_tickerBox;
	DropMenuButton		m_timeframeButton;
	ButtonGroup			m_chartTypeButtons;
	ButtonGroup			m_mouseTypeButtons;
	TextButton*			m_markerButtons[MARK_NMARKERS] = {};
	TextButton*			m_studyButtons[m_nStudies] = {};

	// Painting
	D2D1_RECT_F			m_menuRect = {};
	std::vector<float>	m_divisions;

	// Helper functions
	void ResizeAxes();
	void Load(std::wstring ticker, int range = 1260); // # datapoints in days. default to 5 years 
	void DrawMainChart(MainChartType type, Timeframe timeframe);
	void DrawCurrentState();
	int FindStart(Timeframe timeframe, std::vector<OHLC>::iterator & it);
	void Candlestick(std::vector<OHLC>::iterator start, int n);
	void Line(std::vector<OHLC>::iterator start, int n);
	void Envelope(std::vector<OHLC>::iterator start, int n);
	void DrawMarker(Markers i);
	void DrawHistory();
	void DrawStudy(size_t i);
	void RemoveStudy(size_t i);

	// Utility functions
	size_t GetAxes(std::wstring const & name); // Finds the matching aux axes, or returns the next unused one, or creates a new one
	float GetAuxAxisTop(size_t i); // Returns the dip position of the top of aux axis i.



};


