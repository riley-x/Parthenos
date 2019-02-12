#include "stdafx.h"
#include "Chart.h"
#include "utilities.h"
#include "TitleBar.h"

#include <algorithm>


// Chart is flush right, with fixed-width offset in DIPs on left
void Chart::Init(HWND hwndParent, float leftOffset)
{
	m_hwndParent   = hwndParent;
	m_dipRect.left = leftOffset; 
	m_dipRect.top  = TitleBar::height;
}

void Chart::Load(std::wstring ticker, int range)
{
	m_OHLC = GetOHLC(ticker, apiSource::alpha, range);
	Candlestick(Timeframe::year1); // TODO change to generic draw function calling m_Current...
	// draw on load, default candlestick 1-year

	//for (int i = 0; i < 10; i++)
	//{
	//	OutputDebugString(test[i].to_wstring().c_str());
	//}
	//OutputMessage(L"Size: %u\n", test.size());
	//for (size_t i = test.size() - 10; i < test.size(); i++)
	//{
	//	OutputDebugString(test[i].to_wstring().c_str());
	//}
}

void Chart::Paint(D2Objects const & d2)
{
	d2.pBrush->SetColor(D2D1::ColorF(0.8f, 0.0f, 0.0f, 1.0f));
	d2.pRenderTarget->DrawRectangle(m_dipRect, d2.pBrush, 1.0, NULL);

	m_axes.Paint(d2);
}

void Chart::Resize(RECT pRect)
{
	m_dipRect.right  = DPIScale::PixelsToDipsX(pRect.right);
	m_dipRect.bottom = DPIScale::PixelsToDipsY(pRect.bottom);

	float m_menuHeight = 30;
	m_axes.SetBoundingRect(
		m_dipRect.left, 
		m_dipRect.top + m_menuHeight, 
		m_dipRect.right, 
		m_dipRect.bottom
	);

	if (m_currentMChart != MainChartType::none)
		Candlestick(m_currentTimeframe); // todo change to what is currently drawn
}

// TODO: Add button highlight, etc. here?
void Chart::Candlestick(Timeframe timeframe)
{
	if (m_OHLC.empty())
	{
		OutputMessage(L"No data!\n");
		return;
	}

	OHLC *data = nullptr;
	int n = 0;

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
			return;
		}
		data = &(*it);
		n = m_OHLC.end() - it;
	}
	break;
	default:
	{
		OutputMessage(L"Timeframe %s not implemented\n", timeframe);
		return;
	}
	}

	m_currentMChart = MainChartType::candlestick;
	m_currentTimeframe = timeframe;

	m_axes.Candlestick(data, n); 
	InvalidateRect(m_hwndParent, NULL, FALSE);
}



void Chart::Line(OHLC const * ohlc, int n)
{ 
	m_axes.Line(ohlc, n); 
	InvalidateRect(m_hwndParent, NULL, FALSE);
}

