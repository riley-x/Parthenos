#include "stdafx.h"
#include "Chart.h"
#include "utilities.h"
#include "TitleBar.h"
#include "Colors.h"
#include "Resource.h"

#include <algorithm>

bool candle = true;

Chart::Chart(HWND hwnd, D2Objects const & d2)
	: AppItem(hwnd, d2), m_axes(hwnd, d2), m_tickerBox(hwnd, d2) 
{
	auto temp = new IconButton(hwnd, d2);
	temp->SetIcon(GetResourceIndex(IDB_CANDLESTICK));
	m_iconButtons.push_back(temp);
	
	temp = new IconButton(hwnd, d2);
	temp->SetIcon(GetResourceIndex(IDB_LINE));
	m_iconButtons.push_back(temp);

	for (int i = 0; i < 6; i++)
	{
		m_iconButtons.push_back(new IconButton(hwnd, d2));
	}
}
Chart::~Chart()
{
	for (auto button : m_iconButtons)
		if (button) delete button;
}

// Chart is flush right, with fixed-width offset in DIPs on left
void Chart::Init(float leftOffset)
{
	m_dipRect.left = leftOffset; 
	m_dipRect.top  = TitleBar::height;
	m_pixRect.left = DPIScale::DipsToPixelsX(m_dipRect.left);
	m_pixRect.top = DPIScale::DipsToPixelsY(m_dipRect.top);
	m_menuRect.left = m_dipRect.left;
	m_menuRect.top = m_dipRect.top;
	m_menuRect.bottom = m_menuRect.top + m_menuHeight;
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

void Chart::Paint(D2D1_RECT_F updateRect)
{
	// when invalidating, converts to pixels then back to DIPs -> updateRect has smaller values
	// than when invalidated.
	if (updateRect.bottom <= m_dipRect.top || updateRect.right <= m_dipRect.left) return;

	m_d2.pBrush->SetColor(D2D1::ColorF(0.8f, 0.0f, 0.0f, 1.0f));
	m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 1.0, NULL);

	m_d2.pBrush->SetColor(Colors::MENU_BACKGROUND);
	m_d2.pRenderTarget->FillRectangle(m_menuRect, m_d2.pBrush);


	m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
	D2D1_RECT_F iconRect;
	if (candle)
	{
		iconRect = m_iconButtons[0]->GetDIPRect();
	}
	else
	{
		iconRect = m_iconButtons[1]->GetDIPRect();
	}
	iconRect.left -= m_commandHPad / 2.0f;
	iconRect.top = m_menuRect.top;
	iconRect.right += m_commandHPad / 2.0f;
	iconRect.bottom = m_menuRect.bottom - 1.0f;
	m_d2.pRenderTarget->FillRectangle(iconRect, m_d2.pBrush);


	m_tickerBox.Paint(updateRect);
	for (auto icon : m_iconButtons)
		icon->Paint(updateRect);

	m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
	m_d2.pRenderTarget->DrawLine(
		D2D1::Point2F(m_menuRect.left, m_menuRect.bottom),
		D2D1::Point2F(m_menuRect.right, m_menuRect.bottom),
		m_d2.pBrush,
		0.5f
	);

	if (updateRect.bottom <= m_axes.GetDIPRect().top) return;
	m_axes.Paint(updateRect);

}

void Chart::Resize(RECT pRect, D2D1_RECT_F pDipRect)
{
	m_dipRect.right  = pDipRect.right;
	m_dipRect.bottom = pDipRect.bottom;
	m_pixRect.right  = pRect.right;
	m_pixRect.bottom = pRect.bottom;
	m_menuRect.right = m_dipRect.right;

	m_axes.SetSize(D2D1::RectF(
		m_dipRect.left, 
		m_dipRect.top + m_menuHeight, 
		m_dipRect.right, 
		m_dipRect.bottom
	));

	float top = m_dipRect.top + (m_menuHeight - m_commandSize) / 2.0f;
	m_tickerBox.SetSize(D2D1::RectF(
		m_dipRect.left + m_commandHPad,
		top,
		m_dipRect.left + m_commandHPad + m_labelBoxWidth,
		top + m_commandSize
	));

	
	for (size_t i = 0; i < m_iconButtons.size(); i++)
	{
		float left = m_dipRect.left + m_labelBoxWidth + m_commandHPad * (i + 2) + m_commandSize * i;
		m_iconButtons[i]->SetSize(D2D1::RectF(
			left,
			top,
			left + m_commandSize,
			top + m_commandSize
		));
	}
}

bool Chart::OnLButtonDown(D2D1_POINT_2F cursor)
{
	if (cursor.x > m_dipRect.left &&
		cursor.x < m_dipRect.right &&
		cursor.y > m_dipRect.top &&
		cursor.y < m_dipRect.bottom)
	{
		//if (m_currentMChart == MainChartType::envelope)
		//	DrawMainChart(MainChartType::candlestick, m_currentTimeframe);
		//else if (m_currentMChart == MainChartType::candlestick)
		//	DrawMainChart(MainChartType::line, m_currentTimeframe);
		//else if (m_currentMChart == MainChartType::line)
		//	DrawMainChart(MainChartType::envelope, m_currentTimeframe);

		if (cursor.y < m_menuRect.bottom)
		{
			if (m_tickerBox.OnLButtonDown(cursor)) return true;
			for (size_t i = 0; i < m_iconButtons.size(); i++)
			{
				if (m_iconButtons[i]->OnLButtonDown(cursor))
				{
					// Create icon group class
					if (i == 0)
					{
						DrawMainChart(MainChartType::candlestick, m_currentTimeframe);
						candle = true;
					}
					else if (i == 1)
					{
						DrawMainChart(MainChartType::line, m_currentTimeframe);
						candle = false;
					}
					return true;
				}
			}
		}
	}
	return false;
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

	InvalidateRect(m_hwnd, &m_pixRect, FALSE);
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
	InvalidateRect(m_hwnd, &m_pixRect, FALSE);
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
	InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}

