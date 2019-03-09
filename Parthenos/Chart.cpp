#include "stdafx.h"
#include "Chart.h"
#include "utilities.h"
#include "Resource.h"

#include <algorithm>

float const Chart::m_commandSize = 20.0f;
float const Chart::m_tickerBoxWidth = 100.0f;
float const Chart::m_timeframeWidth = 60.0f;

Chart::Chart(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent)
	: AppItem(hwnd, d2, parent), m_axes(hwnd, d2, this), m_tickerBox(hwnd, d2, this), 
	m_timeframeButton(hwnd, d2, this), m_chartTypeButtons(hwnd, d2, this), m_mouseTypeButtons(hwnd, d2, this)
{
	// Timeframe button
	m_timeframeButton.SetItems({ L"1M", L"3M", L"6M", L"1Y", L"2Y", L"5Y" });
	m_timeframeButton.SetActive(3); // default 1 year

	// Chart type buttons (don't need messages so set parent = nullptr)
	auto temp = new IconButton(hwnd, d2, nullptr);
	temp->SetName(L"Candlestick");
	temp->SetIcon(GetResourceIndex(IDB_CANDLESTICK));
	m_chartTypeButtons.Add(temp);
	
	temp = new IconButton(hwnd, d2, nullptr);
	temp->SetName(L"Line");
	temp->SetIcon(GetResourceIndex(IDB_LINE));
	m_chartTypeButtons.Add(temp);

	temp = new IconButton(hwnd, d2, nullptr);
	temp->SetName(L"Envelope");
	temp->SetIcon(GetResourceIndex(IDB_ENVELOPE));
	m_chartTypeButtons.Add(temp);

	m_chartTypeButtons.SetActive(0);

	// Mouse type buttons
	temp = new IconButton(hwnd, d2, nullptr);
	temp->SetName(L"Arrow");
	temp->SetIcon(GetResourceIndex(IDB_ARROWCURSOR));
	m_mouseTypeButtons.Add(temp);

	temp = new IconButton(hwnd, d2, nullptr);
	temp->SetName(L"Snap");
	temp->SetIcon(GetResourceIndex(IDB_SNAPLINE));
	m_mouseTypeButtons.Add(temp);

	temp = new IconButton(hwnd, d2, nullptr);
	temp->SetName(L"Cross");
	temp->SetIcon(GetResourceIndex(IDB_CROSSHAIRS));
	m_mouseTypeButtons.Add(temp);

	m_mouseTypeButtons.SetActive(0);
}

void Chart::SetSize(D2D1_RECT_F dipRect)
{
	bool same_left = false;
	bool same_top = false;
	if (equalRect(m_dipRect, dipRect)) return;
	if (m_dipRect.left == dipRect.left) same_left = true;
	if (m_dipRect.top == dipRect.top) same_top = true;

	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(dipRect);

	m_menuRect = m_dipRect;
	m_menuRect.bottom = DPIScale::SnapToPixelY(m_menuRect.top + m_menuHeight);

	m_axes.SetSize(D2D1::RectF(
		m_dipRect.left,
		m_menuRect.bottom,
		m_dipRect.right,
		m_dipRect.bottom
	));

	// Menu items
	if (!same_left || !same_top)
	{
		float top = m_dipRect.top + (m_menuHeight - m_commandSize) / 2.0f;
		float left = m_dipRect.left + m_commandHPad;
		m_divisions.clear();

		// Ticker box
		m_tickerBox.SetSize(D2D1::RectF(
			left,
			top,
			left + m_tickerBoxWidth,
			top + m_commandSize
		));
		left += m_tickerBoxWidth + m_commandHPad;

		// Timeframe drop menu
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

		// Mouse type buttons
		for (size_t i = 0; i < m_mouseTypeButtons.Size(); i++)
		{
			m_mouseTypeButtons.SetSize(i, D2D1::RectF(
				left,
				top,
				left + m_commandSize,
				top + m_commandSize
			));
			m_mouseTypeButtons.SetClickRect(i, D2D1::RectF(
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
}

void Chart::Paint(D2D1_RECT_F updateRect)
{
	if (!overlapRect(m_dipRect, updateRect)) return;

	if (updateRect.top <= m_menuRect.bottom) {
		// Background of menu
		m_d2.pBrush->SetColor(Colors::AXES_BACKGROUND);
		m_d2.pD2DContext->FillRectangle(m_menuRect, m_d2.pBrush);

		// Ticker box
		m_tickerBox.Paint(m_menuRect); // pass m_menuRect to force repaint

		// Timeframe menu
		m_timeframeButton.Paint(m_menuRect);

		// Button highlights
		m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
		D2D1_RECT_F iconRect;
		if (m_chartTypeButtons.GetActiveClickRect(iconRect))
			m_d2.pD2DContext->FillRectangle(iconRect, m_d2.pBrush);
		if (m_mouseTypeButtons.GetActiveClickRect(iconRect))
			m_d2.pD2DContext->FillRectangle(iconRect, m_d2.pBrush);

		// Icon buttons
		m_chartTypeButtons.Paint(m_menuRect);
		m_mouseTypeButtons.Paint(m_menuRect);

		// Menu division lines
		m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
		for (float x : m_divisions)
		{
			m_d2.pD2DContext->DrawLine(
				D2D1::Point2F(x, m_menuRect.top),
				D2D1::Point2F(x, m_menuRect.bottom),
				m_d2.pBrush,
				DPIScale::PixelsToDipsX(1)
			);
		}
		m_d2.pD2DContext->DrawLine(
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

bool Chart::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	handeled = m_timeframeButton.OnMouseMove(cursor, wParam, handeled) || handeled;
	handeled = m_tickerBox.OnMouseMove(cursor, wParam, handeled) || handeled;
	handeled = m_axes.OnMouseMove(cursor, wParam, handeled) || handeled;
	//ProcessCTPMessages(); // not needed
	return handeled;
}

bool Chart::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	handeled = m_timeframeButton.OnLButtonDown(cursor, handeled) || handeled;
	handeled = m_tickerBox.OnLButtonDown(cursor, handeled) || handeled;

	if (!handeled && inRect(cursor, m_menuRect))
	{
		std::wstring name;
		if (m_chartTypeButtons.OnLButtonDown(cursor, name, handeled))
		{
			MainChartType newType = MainChartType::none;
			if (name == L"Candlestick")
				newType = MainChartType::candlestick;
			else if (name == L"Line")
				newType = MainChartType::line;
			else if (name == L"Envelope")
				newType = MainChartType::envelope;
			else
				OutputMessage(L"Chart %s button not recognized", name);
			if (m_currentMChart != newType)
			{
				m_currentMChart = newType;
				DrawCurrentState();
			}
			handeled = true;
		}
		else if (m_mouseTypeButtons.OnLButtonDown(cursor, name, handeled))
		{
			if (name == L"Arrow")
				m_axes.SetHoverStyle(Axes::HoverStyle::none);
			else if (name == L"Snap")
				m_axes.SetHoverStyle(Axes::HoverStyle::snap);
			else if (name == L"Cross")
				m_axes.SetHoverStyle(Axes::HoverStyle::crosshairs);
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			handeled = true;
		}
	}

	ProcessCTPMessages();
	return handeled; 
}

void Chart::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	m_tickerBox.OnLButtonDblclk(cursor, wParam);
	// ProcessCTPMessages(); // not needed
}

void Chart::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	m_tickerBox.OnLButtonUp(cursor, wParam);
	// ProcessCTPMessages(); // not needed
}

bool Chart::OnChar(wchar_t c, LPARAM lParam)
{
	c = towupper(c);
	bool out = m_tickerBox.OnChar(c, lParam);
	// ProcessCTPMessages(); // not needed
	return out;
}

bool Chart::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = m_tickerBox.OnKeyDown(wParam, lParam);
	ProcessCTPMessages();
	return out;
}

void Chart::OnTimer(WPARAM wParam, LPARAM lParam)
{
	m_tickerBox.OnTimer(wParam, lParam);
	//ProcessCTPMessages(); // not needed
}

void Chart::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::TEXTBOX_ENTER:
			if (msg.msg != m_ticker)
			{
				Load(msg.msg);
				DrawCurrentState();
			}
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
			m_currentTimeframe = tf;
			DrawCurrentState();
			break;
		}
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

void Chart::Draw(std::wstring ticker)
{
	if (ticker != m_ticker)
	{
		Load(ticker);
		DrawCurrentState();
	}
}

// Force draw
void Chart::Draw(std::wstring ticker, MainChartType type, Timeframe tf)
{
	Load(ticker);
	DrawMainChart(type, tf);
}


///////////////////////////////////////////////////////////
// --- Helpers ---

// Clears stored array data used by axes.
// Gets data the data for 'ticker'. Sets the text box and 'm_ticker'.
void Chart::Load(std::wstring ticker, int range)
{
	if (ticker == m_ticker && range < static_cast<int>(m_OHLC.size())) return;
	
	m_axes.Clear(); // todo FIXME
	
	m_dates.clear();
	m_closes.clear();
	m_highs.clear();
	m_lows.clear();
	m_tickerBox.SetText(ticker);
	m_ticker = ticker;

	try {
		m_OHLC = GetOHLC(ticker, apiSource::alpha, range);
	}
	catch (const std::exception & e) {
		m_OHLC.clear();
		InvalidateRect(m_hwnd, &m_pixRect, FALSE); // so button clicks still appear

		std::wstring out = SPrintException(e);
		m_parent->PostClientMessage(this, out, CTPMessage::PRINT);
	}
}

// Sets the current state members.
void Chart::DrawMainChart(MainChartType type, Timeframe timeframe)
{
	if (timeframe == Timeframe::none) timeframe = Timeframe::year1;
	if (type == MainChartType::none) type = MainChartType::candlestick;
	
	m_currentTimeframe = timeframe;
	m_currentMChart = type;

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
		DrawHistory();
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

// Call this when the main chart needs to be completely redrawn but nothing else has changed
// i.e. changing chart type, ticker, or timeframe should preserve technicals and drawings
// TODO delete drawings when ticker changes
void Chart::DrawCurrentState()
{
	DrawMainChart(m_currentMChart, m_currentTimeframe);

	// DrawTechnicals(m_currentTechnicals), etc...
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
		if (GetMonth(end) == 1)
		{
			temp.date = end - DATE_T_1YR;
			SetMonth(temp.date, 12);
		}
		else
			temp.date = end - DATE_T_1M;
		break;
	case Timeframe::month3:
		if (GetMonth(end) <= 3)
		{
			temp.date = end - DATE_T_1YR;
			SetMonth(temp.date, 9 + GetMonth(end));
		}
		else
			temp.date = end - 3 * DATE_T_1M;
		break;
	case Timeframe::month6:
		if (GetMonth(end) <= 6)
		{
			temp.date = end - DATE_T_1YR;
			SetMonth(temp.date, 6 + GetMonth(end));
		}
		else
			temp.date = end - 6 * DATE_T_1M;
		break;
	case Timeframe::year1:
		temp.date = end - DATE_T_1YR;
		break;
	case Timeframe::year2:
		temp.date = end - 2 * DATE_T_1YR;
		break;
	case Timeframe::year5:
		temp.date = end - 5 * DATE_T_1YR;
		break;
	default:
		OutputMessage(L"Timeframe %s not implemented\n", timeframe);
		return -1;
	}

	it = std::lower_bound(m_OHLC.begin(), m_OHLC.end(), temp, OHLC_Compare);
	if (it == m_OHLC.end())
	{
		OutputMessage(L"Didn't find data for main chart\n");
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

	LineProps envelope_props = { Colors::BRIGHT_LINE, 0.6f, m_d2.pDashedStyle };
	m_axes.Line(m_dates.data(), m_closes.data(), n);
	m_axes.Line(m_dates.data(), m_highs.data(), n, envelope_props);
	m_axes.Line(m_dates.data(), m_lows.data(), n, envelope_props);
}

void Chart::DrawHistory()
{
	std::vector<PointProps> points;
	for (Transaction const & t : m_history)
	{
		if (std::wstring(t.ticker) == m_ticker)
		{
			if (isOption(t.type));
			else if (t.n > 0) points.push_back(PointProps(MarkerStyle::up, t.date, t.price));
			else if (t.n < 0) points.push_back(PointProps(MarkerStyle::down, t.date, t.price));
		}
	}

	m_axes.DatePoints(points, Axes::GG_SEC);
}

