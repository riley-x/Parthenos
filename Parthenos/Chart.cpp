#include "stdafx.h"
#include "Chart.h"
#include "utilities.h"
#include "TitleBar.h"
#include "Resource.h"

#include <algorithm>

float const Chart::m_commandSize = 20.0f;
float const Chart::m_tickerBoxWidth = 100.0f;
float const Chart::m_timeframeWidth = 60.0f;

Chart::Chart(HWND hwnd, D2Objects const & d2)
	: AppItem(hwnd, d2), m_axes(hwnd, d2), m_tickerBox(hwnd, d2, this), 
	m_chartTypeButtons(hwnd, d2), m_timeframeButton(hwnd, d2, this)
{
	auto temp = new IconButton(hwnd, d2);
	temp->m_name = L"Candlestick";
	temp->SetIcon(GetResourceIndex(IDB_CANDLESTICK));
	m_chartTypeButtons.Add(temp);
	
	temp = new IconButton(hwnd, d2);
	temp->m_name = L"Line";
	temp->SetIcon(GetResourceIndex(IDB_LINE));
	m_chartTypeButtons.Add(temp);

	temp = new IconButton(hwnd, d2);
	temp->m_name = L"Envelope";
	temp->SetIcon(GetResourceIndex(IDB_ENVELOPE));
	m_chartTypeButtons.Add(temp);

	m_chartTypeButtons.SetActive(0);
}

Chart::~Chart()
{
}

// Chart is flush right, with fixed-width offset in DIPs on left,
// and fixed menu height
void Chart::Init(float leftOffset)
{
	m_dipRect.left = DPIScale::SnapToPixelX(leftOffset); 
	m_dipRect.top  = DPIScale::SnapToPixelY(TitleBar::height);

	m_pixRect.left = DPIScale::DipsToPixelsX(m_dipRect.left);
	m_pixRect.top = DPIScale::DipsToPixelsY(m_dipRect.top);

	m_menuRect.left = m_dipRect.left;
	m_menuRect.top = m_dipRect.top;
	m_menuRect.bottom = DPIScale::SnapToPixelY(m_menuRect.top + m_menuHeight);

	// Ticker box
	float top = m_dipRect.top + (m_menuHeight - m_commandSize) / 2.0f;
	float left = m_dipRect.left + m_commandHPad;
	m_tickerBox.SetSize(D2D1::RectF(
		left,
		top,
		left + m_tickerBoxWidth,
		top + m_commandSize
	));
	left += m_tickerBoxWidth + m_commandHPad;

	// Timeframe drop menu
	m_timeframeButton.SetItems({ L"1M", L"3M", L"6M", L"1Y", L"2Y", L"5Y" });
	m_timeframeButton.SetActive(3); // default 1 year
	m_timeframeButton.SetSize(D2D1::RectF(
		left,
		top,
		left + m_timeframeWidth,
		top + m_commandSize
	));
	left += m_timeframeWidth + m_commandHPad;

	// Division
	m_divisions.push_back(DPIScale::SnapToPixelX(left + m_commandHPad) + DPIScale::hpx());
	left += 3 * m_commandHPad;

	// Main chart type buttons
	for (size_t i = 0; i < m_chartTypeButtons.Size(); i++)
	{
		m_chartTypeButtons.SetSize(i, D2D1::RectF(
			left,
			top,
			left + m_commandSize,
			top + m_commandSize
		));
		m_chartTypeButtons.SetClickRect(i, D2D1::RectF(
			left - m_commandHPad / 2.0f,
			m_menuRect.top,
			left + m_commandSize + m_commandHPad / 2.0f,
			m_menuRect.bottom
		));
		left += m_commandSize + m_commandHPad;
	}

	// Divison
	m_divisions.push_back(DPIScale::SnapToPixelX(left + m_commandHPad) + DPIScale::hpx());
	left += 3 * m_commandHPad;
}

void Chart::Paint(D2D1_RECT_F updateRect)
{
	// When invalidating, converts to pixels then back to DIPs -> updateRect has smaller values
	// than when invalidated.
	if (updateRect.bottom <= m_dipRect.top || updateRect.right <= m_dipRect.left) return;

	if (updateRect.top <= m_menuRect.bottom) {
		// Background of menu
		m_d2.pBrush->SetColor(Colors::MAIN_BACKGROUND);
		m_d2.pRenderTarget->FillRectangle(m_menuRect, m_d2.pBrush);

		// Ticker box
		m_tickerBox.Paint(updateRect);

		// Timeframe menu
		m_timeframeButton.Paint(updateRect);

		// Chart type button highlight
		m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
		D2D1_RECT_F iconRect;
		if (m_chartTypeButtons.GetActiveClickRect(iconRect))
		{
			m_d2.pRenderTarget->FillRectangle(iconRect, m_d2.pBrush);
		}

		// Chart type buttons
		m_chartTypeButtons.Paint(updateRect);

		// Menu division lines
		m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
		for (float x : m_divisions)
		{
			m_d2.pRenderTarget->DrawLine(
				D2D1::Point2F(x, m_menuRect.top),
				D2D1::Point2F(x, m_menuRect.bottom),
				m_d2.pBrush,
				DPIScale::PixelsToDipsX(1)
			);
		}
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(m_menuRect.left, m_menuRect.bottom - DPIScale::hpy()),
			D2D1::Point2F(m_menuRect.right, m_menuRect.bottom - DPIScale::hpy()),
			m_d2.pBrush,
			DPIScale::PixelsToDipsY(1)
		);
	}

	// Axes
	if (updateRect.bottom > m_menuRect.bottom)
		m_axes.Paint(updateRect);

	// Popups -- need to be last
	m_timeframeButton.GetMenu().Paint(updateRect);

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
		m_menuRect.bottom, 
		m_dipRect.right, 
		m_dipRect.bottom
	));
}

void Chart::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam)
{
	m_tickerBox.OnMouseMove(cursor, wParam);
	m_timeframeButton.OnMouseMove(cursor, wParam);
	//ProcessMessages(); // not needed
}

bool Chart::OnLButtonDown(D2D1_POINT_2F cursor)
{
	m_tickerBox.OnLButtonDown(cursor);
	m_timeframeButton.OnLButtonDown(cursor);

	if (inRect(cursor, m_menuRect))
	{
		std::wstring name;
		if (m_chartTypeButtons.OnLButtonDown(cursor, name))
		{
			if (name == L"Candlestick")
				DrawMainChart(m_ticker, MainChartType::candlestick, m_currentTimeframe);
			else if (name == L"Line")
				DrawMainChart(m_ticker, MainChartType::line, m_currentTimeframe);
			else if (name == L"Envelope")
				DrawMainChart(m_ticker, MainChartType::envelope, m_currentTimeframe);
		}
	}

	ProcessMessages();
	return false; // unused
}

void Chart::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	m_tickerBox.OnLButtonDblclk(cursor, wParam);
	// ProcessMessages(); // not needed
}

void Chart::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	m_tickerBox.OnLButtonUp(cursor, wParam);
	// ProcessMessages(); // not needed
}

bool Chart::OnChar(wchar_t c, LPARAM lParam)
{
	c = towupper(c);
	bool out = m_tickerBox.OnChar(c, lParam);
	// ProcessMessages(); // not needed
	return out;
}

bool Chart::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = m_tickerBox.OnKeyDown(wParam, lParam);
	ProcessMessages();
	return out;
}

void Chart::OnTimer(WPARAM wParam, LPARAM lParam)
{
	m_tickerBox.OnTimer(wParam, lParam);
	//ProcessMessages(); // not needed
}

void Chart::Load(std::wstring ticker, int range)
{
	if (ticker == m_ticker && range < static_cast<int>(m_OHLC.size())) return;
	m_dates.clear();
	m_closes.clear();
	m_highs.clear();
	m_lows.clear();
	m_OHLC = GetOHLC(ticker, apiSource::alpha, range);
	DrawMainChart(ticker, m_currentMChart, m_currentTimeframe);

	for (int i = 0; i < 10; i++)
	{
		OutputDebugString(m_OHLC[i].to_wstring().c_str());
	}
	OutputMessage(L"Size: %u\n", m_OHLC.size());
	for (size_t i = m_OHLC.size() - 10; i < m_OHLC.size(); i++)
	{
		OutputDebugString(m_OHLC[i].to_wstring().c_str());
	}
}

void Chart::ProcessMessages()
{
	for (ClientMessage msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::TEXTBOX_ENTER:
			Load(msg.msg);
			break;
		case CTPMessage::TEXTBOX_DEACTIVATED:
			break; // Do nothing
		case CTPMessage::DROPMENU_SELECTED:
		{
			Timeframe tf = Timeframe::none;;
			if (msg.msg == L"1M") tf = Timeframe::month1;
			else if (msg.msg == L"3M") tf = Timeframe::month3;
			else if (msg.msg == L"6M") tf = Timeframe::month6;
			else if (msg.msg == L"1Y") tf = Timeframe::year1;
			else if (msg.msg == L"2Y") tf = Timeframe::year2;
			else if (msg.msg == L"5Y") tf = Timeframe::year5;
			if (tf == m_currentTimeframe) return;
			DrawMainChart(m_ticker, m_currentMChart, tf);
			break;
		}
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

///////////////////////////////////////////////////////////
// --- Helpers ---

// Sets the current state members.
void Chart::DrawMainChart(std::wstring ticker, MainChartType type, Timeframe timeframe)
{

	if (timeframe == Timeframe::none) timeframe = Timeframe::year1;
	if (type == MainChartType::none) type = MainChartType::candlestick;
	if (ticker == m_ticker && m_currentMChart == type && m_currentTimeframe == timeframe) return;
	
	m_ticker = ticker;
	m_currentTimeframe = timeframe;
	m_currentMChart = type;
	m_tickerBox.SetText(m_ticker);

	OHLC *data;
	int n = FindStart(timeframe, data);
	if (n <= 0)
	{
		InvalidateRect(m_hwnd, &m_pixRect, FALSE); // so button clicks still appear
		return;
	}

	m_axes.Clear(); // todo FIXME

	switch (type)
	{
	case MainChartType::line:
		Line(data, n);
		break;
	case MainChartType::envelope:
		Envelope(data, n);
		break;
	case MainChartType::candlestick:
		Candlestick(data, n);
		break;
	}

	InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}

// Call this when the chart needs to be completely redrawn but nothing's changed,
// i.e. on resize
void Chart::DrawSavedState()
{
	DrawMainChart(m_ticker, m_currentMChart, m_currentTimeframe);
}

// Finds the starting date in OHLC data given 'timeframe', returning the pointer in 'data'.
// Returns the number of days / data points, or -1 on error.
int Chart::FindStart(Timeframe timeframe, OHLC* & data)
{
	if (m_OHLC.empty())
	{
		OutputMessage(L"No data!\n");
		return -1;
	}

	data = nullptr;
	int n;
	date_t end = m_OHLC.back().date;
	std::vector<OHLC>::iterator it;
	OHLC temp;

	switch (timeframe)
	{
	case Timeframe::month1:
	{
		if (GetMonth(end) == 1)
		{
			temp.date = end - DATE_T_1YR;
			SetMonth(temp.date, 12);
		}
		else
		{
			temp.date = end - DATE_T_1M;
		}
		break;
	}
	case Timeframe::month3:
	{
		if (GetMonth(end) <= 3)
		{
			temp.date = end - DATE_T_1YR;
			SetMonth(temp.date, 9 + GetMonth(end));
		}
		else
		{
			temp.date = end - 3 * DATE_T_1M;
		}
		break;
	}
	case Timeframe::month6:
	{
		if (GetMonth(end) <= 6)
		{
			temp.date = end - DATE_T_1YR;
			SetMonth(temp.date, 6 + GetMonth(end));
		}
		else
		{
			temp.date = end - 6 * DATE_T_1M;
		}
		break;
	}
	case Timeframe::year1:
	{
		temp.date = end - DATE_T_1YR;
		break;
	}
	case Timeframe::year2:
	{
		temp.date = end - 2 * DATE_T_1YR;
		break;
	}
	case Timeframe::year5:
	{
		temp.date = end - 5 * DATE_T_1YR;
		break;
	}
	default:
	{
		OutputMessage(L"Timeframe %s not implemented\n", timeframe);
		return -1;
	}
	}

	it = std::lower_bound(m_OHLC.begin(), m_OHLC.end(), temp, OHLC_Compare);
	if (it == m_OHLC.end())
	{
		OutputMessage(L"Didn't find data for candlestick\n");
		return -1;
	}

	data = &(*it);
	n = m_OHLC.end() - it;
	return n;
}


void Chart::Candlestick(OHLC const *data, int n)
{
	m_axes.Candlestick(data, n); 
}


void Chart::Line(OHLC const *data, int n)
{ 
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

	m_axes.Line(m_dates.data(), m_closes.data(), n);
}

void Chart::Envelope(OHLC const *data, int n)
{
	// data may already exist! TODO: zooming needs to clear
	if (n != m_closes.size() || n != m_highs.size()
		|| n != m_lows.size() || n != m_dates.size()) 
	{
		m_dates.resize(n);
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

	m_axes.Line(m_dates.data(), m_closes.data(), n);
	m_axes.Line(m_dates.data(), m_highs.data(), n, D2D1::ColorF(0.8f, 0.0f, 0.5f), 0.6f, m_d2.pDashedStyle);
	m_axes.Line(m_dates.data(), m_lows.data(), n, D2D1::ColorF(0.8f, 0.0f, 0.5f), 0.6f, m_d2.pDashedStyle);
}

