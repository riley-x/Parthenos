#include "stdafx.h"
#include "Chart.h"
#include "utilities.h"
#include "TitleBar.h"

#include <algorithm>


// Chart is flush right, with fixed-width offset in DIPs on left
void Chart::Init(float leftOffset)
{
	m_dipRect.left = leftOffset; 
	m_dipRect.top  = TitleBar::height;
}

void Chart::Load(std::wstring ticker, int range)
{
	m_OHLC = GetOHLC(ticker, apiSource::alpha, range);
	DrawSavedState(); // draw on load, default candlestick 1-year

	//for (int i = 0; i < 10; i++)
	//{
	//	OutputDebugString(m_OHLC[i].to_wstring().c_str());
	//}
	//OutputMessage(L"Size: %u\n", m_OHLC.size());
	//for (size_t i = m_OHLC.size() - 10; i < m_OHLC.size(); i++)
	//{
	//	OutputDebugString(m_OHLC[i].to_wstring().c_str());
	//}
}

void Chart::Paint()
{
	m_d2.pBrush->SetColor(D2D1::ColorF(0.8f, 0.0f, 0.0f, 1.0f));
	m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 1.0, NULL);

	m_axes.Paint();
}

void Chart::Resize(RECT pRect, D2D1_RECT_F pDipRect)
{
	m_dipRect.right  = pDipRect.right;
	m_dipRect.bottom = pDipRect.bottom;

	m_axes.SetSize(D2D1::Rect(
		m_dipRect.left, 
		m_dipRect.top + m_menuHeight, 
		m_dipRect.right, 
		m_dipRect.bottom
	));
}

void Chart::OnLButtonDown(D2D1_POINT_2F cursor)
{
	if (cursor.x > m_dipRect.left &&
		cursor.x < m_dipRect.right &&
		cursor.y > m_dipRect.top &&
		cursor.y < m_dipRect.bottom)
	{
		if (m_currentMChart == MainChartType::envelope)
			DrawMainChart(MainChartType::candlestick, m_currentTimeframe);
		else if (m_currentMChart == MainChartType::candlestick)
			DrawMainChart(MainChartType::line, m_currentTimeframe);
		else if (m_currentMChart == MainChartType::line)
			DrawMainChart(MainChartType::envelope, m_currentTimeframe);
	}
}

void Chart::DrawMainChart(MainChartType type, Timeframe timeframe)
{
	if (timeframe == Timeframe::none) timeframe = Timeframe::year1;

	switch (type)
	{
	case MainChartType::line:
		Line(timeframe);
		break;
	case MainChartType::envelope:
		Envelope(timeframe);
		break;
	case MainChartType::candlestick:
	case MainChartType::none:
	default:
		Candlestick(timeframe);
		break;
	}
}

// Call this when the chart needs to be completely redrawn but nothing's changed,
// i.e. on resize
void Chart::DrawSavedState()
{
	DrawMainChart(m_currentMChart, m_currentTimeframe);
}

// Finds the starting date in OHLC data given 'timeframe', returning the pointer in 'data'.
// Returns the number of days / data points.
int Chart::FindStart(Timeframe timeframe, OHLC* & data)
{
	if (m_OHLC.empty())
	{
		OutputMessage(L"No data!\n");
		return 0;
	}

	data = nullptr;
	int n;

	switch (timeframe)
	{
	case Timeframe::year1:
	{
		date_t end = m_OHLC.back().date;
		OHLC temp; temp.date = end - DATE_T_1YR;
		auto it = std::lower_bound(m_OHLC.begin(), m_OHLC.end(), temp, OHLC_Compare);
		if (it == m_OHLC.end())
		{
			OutputMessage(L"Didn't find data for candlestick\n");
			return -1;
		}
		data = &(*it);
		n = m_OHLC.end() - it;
	}
	break;
	default:
	{
		OutputMessage(L"Timeframe %s not implemented\n", timeframe);
		return -1;
	}
	}

	return n;
}

// TODO: Add button highlight, etc. here?
// Sets the current state members.
void Chart::Candlestick(Timeframe timeframe)
{
	OHLC *data;
	int n = FindStart(timeframe, data);

	m_currentMChart = MainChartType::candlestick;
	m_currentTimeframe = timeframe;

	m_axes.Clear(); // todo FIXME
	m_axes.Candlestick(data, n); 
	InvalidateRect(m_hwnd, NULL, FALSE);
}


void Chart::Line(Timeframe timeframe)
{ 
	OHLC *data;
	int n = FindStart(timeframe, data);
	
	// data may already exist! TODO: zooming needs to clear
	if (n != m_closes.size() || n != m_dates.size()) 
	{
		m_dates.resize(n);
		m_closes.resize(n);
		int size = m_OHLC.size();
		for (int i = 0; i < n; i++)
		{
			m_dates[i] = m_OHLC[size - n + i].date;
			m_closes[i] = m_OHLC[size - n + i].close;
		}
	}

	m_currentMChart = MainChartType::line;
	m_currentTimeframe = timeframe;

	m_axes.Clear(); // todo FIXME
	m_axes.Line(m_dates.data(), m_closes.data(), n); 
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void Chart::Envelope(Timeframe timeframe)
{
	OHLC *data;
	int n = FindStart(timeframe, data);

	// data may already exist! TODO: zooming needs to clear
	if (n != m_closes.size() || n != m_highs.size()
		|| n != m_lows.size() || n != m_dates.size()) 
	{
		m_closes.resize(n);
		m_highs.resize(n);
		m_lows.resize(n);
		int size = m_OHLC.size();
		for (int i = 0; i < n; i++)
		{
			m_dates[i] = m_OHLC[size - n + i].date;
			m_closes[i] = m_OHLC[size - n + i].close;
			m_highs[i] = m_OHLC[size - n + i].high;
			m_lows[i] = m_OHLC[size - n + i].low;
		}
	}

	m_currentMChart = MainChartType::envelope;
	m_currentTimeframe = timeframe;

	m_axes.Clear(); // todo FIXME
	m_axes.Line(m_dates.data(), m_closes.data(), n);
	m_axes.Line(m_dates.data(), m_highs.data(), n, D2D1::ColorF(0.8f, 0.0f, 0.5f), 0.6f, m_d2.pDashedStyle);
	m_axes.Line(m_dates.data(), m_lows.data(), n, D2D1::ColorF(0.8f, 0.0f, 0.5f), 0.6f, m_d2.pDashedStyle);
	InvalidateRect(m_hwnd, NULL, FALSE);
}

