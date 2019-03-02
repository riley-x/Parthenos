#include "stdafx.h"
#include "Graphing.h"


void Axes::Clear()
{
	for (Graph *item : m_graphObjects) {
		if (item) delete item;
	}
	m_xTicks.clear();
	m_yTicks.clear();
	m_dates.clear();
	m_grid_lines[0].clear();
	m_grid_lines[1].clear();
	m_graphObjects.clear();
	m_ismade = true;
	m_imade = 0;
	for (int i = 0; i < 4; i++)
		m_dataRange[i] = nan("");
	m_rescaled = false;
}

// Converts all graph objects into DIPs. Call when axes are rescaled.
void Axes::Make()
{
	if (m_rescaled) Rescale(); // sets m_imade = 0

	for (m_imade; m_imade < m_graphObjects.size(); m_imade++)
		m_graphObjects[m_imade]->Make();

	m_ismade = true;
}

// May want to call clear before this
void Axes::SetSize(D2D1_RECT_F dipRect)
{
	m_ismade = false;
	m_imade = 0;
	m_rescaled = true;

	m_dipRect = dipRect;

	float pad = 14.0;
	m_axesRect.left = dipRect.left;
	m_axesRect.top = dipRect.top;
	m_axesRect.right = dipRect.right - m_ylabelWidth - pad;
	m_axesRect.bottom = (m_drawxLabels) ? dipRect.bottom - m_labelHeight - pad : dipRect.bottom;

	m_dataRect.left = m_axesRect.left + m_dataPad;
	m_dataRect.top = m_axesRect.top + m_dataPad;
	m_dataRect.right = m_axesRect.right - m_dataPad;
	m_dataRect.bottom = m_axesRect.bottom - m_dataPad;

	m_rect_xdiff = m_dataRect.right - m_dataRect.left;
	m_rect_ydiff = m_dataRect.top - m_dataRect.bottom; // flip so origin is bottom-left
}

void Axes::Paint(D2D1_RECT_F updateRect)
{
	if (!m_ismade) Make();

	// Background
	m_d2.pBrush->SetColor(Colors::AXES_BACKGROUND);
	m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);

	// Grid lines
	m_d2.pBrush->SetColor(Colors::DULL_LINE);
	for (auto line : m_grid_lines[0]) // x lines
	{
		m_d2.pRenderTarget->DrawLine(line.start, line.end, m_d2.pBrush, 0.8f, m_d2.pDashedStyle);
	}
	for (auto line : m_grid_lines[1]) // y lines
	{
		m_d2.pRenderTarget->DrawLine(line.start, line.end, m_d2.pBrush, 0.8f, m_d2.pDashedStyle);
	}

	// Labels
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	if (m_drawxLabels)
	{
		for (auto tick : m_xTicks)
		{
			float loc = std::get<0>(tick);
			std::wstring label = std::get<2>(tick);

			m_d2.pRenderTarget->DrawText(
				label.c_str(),
				label.size(),
				m_d2.pTextFormats[D2Objects::Segoe10],
				D2D1::RectF(loc,
					m_axesRect.bottom + m_labelPad,
					loc + 4.0f * m_labelHeight,
					m_axesRect.bottom + m_labelPad + 2.0f * m_labelHeight),
				m_d2.pBrush
			);
		}
	}
	for (auto tick : m_yTicks)
	{
		float loc = std::get<0>(tick);
		std::wstring label = std::get<2>(tick);

		m_d2.pRenderTarget->DrawText(
			label.c_str(),
			label.size(),
			m_d2.pTextFormats[D2Objects::Segoe10],
			D2D1::RectF(m_axesRect.right + m_labelPad,
				loc - m_labelHeight/2.0f,
				m_dipRect.right,
				loc + m_labelHeight/2.0f),
			m_d2.pBrush
		);
	}

	// Graphs
	for (auto graph : m_graphObjects)
		graph->Paint(m_d2);

	// Axes
	m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	float xaxis_dip_pos;
	if (std::isnan(m_xAxisPos)) xaxis_dip_pos = m_axesRect.bottom;
	else xaxis_dip_pos = DPIScale::SnapToPixelY(YtoDIP(m_xAxisPos)) - DPIScale::hpy();
	Line_t xaxis = { D2D1::Point2F(m_axesRect.left,  xaxis_dip_pos),
					 D2D1::Point2F(m_axesRect.right, xaxis_dip_pos) };
	Line_t yaxis = { D2D1::Point2F(m_axesRect.right, m_axesRect.bottom),
					 D2D1::Point2F(m_axesRect.right, m_axesRect.top) };
	m_d2.pRenderTarget->DrawLine(xaxis.start, xaxis.end, m_d2.pBrush, DPIScale::py());
	m_d2.pRenderTarget->DrawLine(yaxis.start, yaxis.end, m_d2.pBrush, DPIScale::px());
}

void Axes::Candlestick(OHLC const * ohlc, int n)
{
	bool get_dates = m_dates.empty();
	if (!get_dates)
	{
		if (m_dates.size() != n || m_dates.front() != ohlc[0].date || m_dates.back() != ohlc[n - 1].date)
			OutputMessage(L"Error: Dates don't line up\n");
	}
	else
	{
		m_dates.reserve(n);
	}

	// get min/max of data to scale data appropriately
	// set x values to [0, n-1)
	double xmin = -0.5; // extra pad
	double xmax = n - 0.5;
	double low_min = ohlc[0].low;
	double high_max = ohlc[0].high;
	for (int i = 0; i < n; i++)
	{
		if (get_dates) m_dates.push_back(ohlc[i].date);
		if (ohlc[i].low < low_min) low_min = ohlc[i].low;
		else if (ohlc[i].high > high_max) high_max = ohlc[i].high;
	}

	m_rescaled = setDataRange(dataRange::xmin, xmin) || m_rescaled;
	m_rescaled = setDataRange(dataRange::xmax, xmax) || m_rescaled;
	m_rescaled = setDataRange(dataRange::ymin, low_min) || m_rescaled;
	m_rescaled = setDataRange(dataRange::ymax, high_max) || m_rescaled;

	auto graph = new CandlestickGraph(this, ohlc, n);
	m_graphObjects.push_back(graph);
	m_ismade = false;
}

void Axes::Line(date_t const * dates, double const * ydata, int n, 
	D2D1_COLOR_F color, float stroke_width, ID2D1StrokeStyle * pStyle)
{
	if (m_dates.empty()) m_dates = std::vector<date_t>(dates, dates + n);
	else if (m_dates.size() != n || m_dates.front() != dates[0] || m_dates.back() != dates[n - 1])
		OutputMessage(L"Error: Dates don't line up\n");
	
	// get min/max of data to scale data appropriately
	// sets x values to [0, n-1)
	double xmin = 0;
	double xmax = n - 1;
	double ymin = ydata[0];
	double ymax = ydata[0];
	for (int i = 0; i < n; i++)
	{
		if (ydata[i] < ymin) ymin = ydata[i];
		else if (ydata[i] > ymax) ymax = ydata[i];
	}

	m_rescaled = setDataRange(dataRange::xmin, xmin) || m_rescaled;
	m_rescaled = setDataRange(dataRange::xmax, xmax) || m_rescaled;
	m_rescaled = setDataRange(dataRange::ymin, ymin) || m_rescaled;
	m_rescaled = setDataRange(dataRange::ymax, ymax) || m_rescaled;

	auto graph = new LineGraph(this, ydata, n);
	graph->SetLineProperties(color, stroke_width, pStyle);
	m_graphObjects.push_back(graph);
	m_ismade = false;
}

void Axes::Bar(std::pair<double, D2D1_COLOR_F> const * data, int n)
{
	m_drawxLabels = false; // no x labels

	// get min/max of data to scale data appropriately. x values [0,n)
	double xmin = -0.5; // extra padding
	double xmax = n - 0.5;
	double ymin = 0;
	double ymax = 0;
	for (int i = 0; i < n; i++)
	{
		if (data[i].first < ymin) ymin = data[i].first;
		else if (data[i].first > ymax) ymax = data[i].first;
	}

	m_rescaled = setDataRange(xmin, xmax, ymin, ymax) || m_rescaled;

	auto graph = new BarGraph(this, data, n);
	m_graphObjects.push_back(graph);
	m_ismade = false;
}

// Call on data range changing. Recalculates the ticks.
void Axes::Rescale()
{
	m_data_xdiff = m_dataRange[static_cast<int>(dataRange::xmax)]
		- m_dataRange[static_cast<int>(dataRange::xmin)];
	m_data_ydiff = m_dataRange[static_cast<int>(dataRange::ymax)]
		- m_dataRange[static_cast<int>(dataRange::ymin)];

	if (m_drawxLabels) CalculateXTicks();
	CalculateYTicks();

	m_imade = 0; // remake everything
	m_rescaled = false;
}


void Axes::CalculateXTicks()
{
	size_t nmax = static_cast<size_t>(m_rect_xdiff / (3.0f * m_labelHeight));
	if (nmax == 0 || m_dates.empty()) return;
	size_t step = (m_dates.size() + nmax - 1) / nmax; // round up
	float xdip = XtoDIP(0);
	float spacing = XtoDIP(step) - xdip;
	
	int last_month = -1;
	int last_year = -1;
	bool show_days = GetYear(m_dates.back()) - GetYear(m_dates.front()) < 2; 
	// if granularity is small enough, show days. Otherwise show months always

	m_xTicks.clear();
	m_grid_lines[0].clear();
	for (size_t i = 0; i < m_dates.size(); i += step)
	{
		wchar_t buffer[20] = {};
		date_t date = m_dates[i];
		int month = GetMonth(date);
		int year = GetYear(date);

		if (show_days)
		{
			if (month != last_month)
			{
				if (last_month == 12)
					swprintf_s(buffer, _countof(buffer), L"%d\n%d", year, GetDay(date));
				else
					swprintf_s(buffer, _countof(buffer), L"%s\n%d", toMonthWString_Short(date).c_str(), GetDay(date));
			}
			else
			{
				swprintf_s(buffer, _countof(buffer), L"%d", GetDay(date));
			}
			last_month = month;
		}
		else
		{
			if (year != last_year)
				swprintf_s(buffer, _countof(buffer), L"%d", year);
			else
				swprintf_s(buffer, _countof(buffer), L"%s\n%d", toMonthWString_Short(date).c_str(), GetDay(date));
			last_year = year;
		}


		m_xTicks.push_back({ xdip, m_dates[i], std::wstring(buffer) });
		m_grid_lines[0].push_back(
			{ D2D1::Point2F(xdip, m_axesRect.top),
			D2D1::Point2F(xdip, m_axesRect.bottom) }
		);
		xdip += spacing;
	}
	// Add extra lines to the right if space is available.
	// Don't push to xTicks since no date value available.
	while (xdip < m_axesRect.right)
	{
		m_grid_lines[0].push_back(
			{ D2D1::Point2F(xdip, m_axesRect.top),
			D2D1::Point2F(xdip, m_axesRect.bottom) }
		);
		xdip += spacing;
	}
}

// Gets human-friendly yticks
void Axes::CalculateYTicks()
{
	int nmax = static_cast<int>(-m_rect_ydiff / (2.0f * m_labelHeight));
	if (nmax <= 0) return;
	double step = 1.0;
	double ex_step = m_data_ydiff / static_cast<double>(nmax);

	// TODO implement step < 1.0 check
	while (ex_step > 0.0) // while true
	{
		if (ex_step < 0.9)
		{
			step *= 1.0;
			break;
		}
		else if (ex_step < 1.9)
		{
			step *= 2.0;
			break;
		}
		else if (ex_step < 4.8)
		{
			step *= 5.0;
			break;
		}
		else
		{
			step *= 10.0;
			ex_step /= 10.0;
		}
	}

	double ymin = m_dataRange[static_cast<int>(dataRange::ymin)];
	double ymax = m_dataRange[static_cast<int>(dataRange::ymax)];
	
	double start;
	if (signbit(ymin) != signbit(ymax)) start = 0;
	else start = ceil(ymin / step) * step;
	float start_dip = YtoDIP(start);
	float diff = YtoDIP(start + step) - start_dip; // this is negative: moving upwards

	m_yTicks.clear();
	m_grid_lines[1].clear();

	// Add lines moving up from start
	double y = start;
	float ydip = start_dip;
	while (ydip > m_axesRect.top  + m_labelHeight / 2.0f)
	{
		wchar_t buffer[20] = {};
		swprintf_s(buffer, _countof(buffer), L"%.2lf", y);
		m_yTicks.push_back({ ydip, y, std::wstring(buffer) });
		m_grid_lines[1].push_back(
			{D2D1::Point2F(m_axesRect.left, ydip),
			D2D1::Point2F(m_axesRect.right, ydip)}
		);
		y += step;
		ydip += diff;
	}
	// Add lines moving down from start
	y = start - step;
	ydip = start_dip - diff;
	while (ydip < m_axesRect.bottom)
	{
		wchar_t buffer[20] = {};
		swprintf_s(buffer, _countof(buffer), L"%.2lf", y);
		m_yTicks.push_back({ ydip, y, std::wstring(buffer) });
		m_grid_lines[1].push_back(
			{ D2D1::Point2F(m_axesRect.left, ydip),
			D2D1::Point2F(m_axesRect.right, ydip) }
		);
		y -= step;
		ydip -= diff;
	}
}

///////////////////////////////////////////////////////////////////////////////

// Caculates the DIP coordinates for in_data (setting x values to [0,n) )
// and adds line segments connecting the points
void LineGraph::Make()
{	
	double const * data = reinterpret_cast<double const *>(m_data);
	m_lines.resize(m_n - 1);

	float x = m_axes->XtoDIP(static_cast<double>(0));
	float y = m_axes->YtoDIP(data[0]);
	D2D1_POINT_2F start;
	D2D1_POINT_2F end = D2D1::Point2F(x, y);
	for (int i = 1; i < m_n; i++)
	{
		start = end;
		x = m_axes->XtoDIP(static_cast<double>(i));
		y = m_axes->YtoDIP(data[i]);
		end = D2D1::Point2F(x, y);
		m_lines[i - 1] = { start, end };
	}
}

void LineGraph::Paint(D2Objects const & d2)
{
	d2.pBrush->SetColor(m_color);
	for (auto line : m_lines)
	{
		d2.pRenderTarget->DrawLine(line.start, line.end, d2.pBrush, m_stroke_width, m_pStyle);
	}
}

void LineGraph::SetLineProperties(D2D1_COLOR_F color, float stroke_width, ID2D1StrokeStyle * pStyle)
{
	m_color = color;
	m_stroke_width = stroke_width;
	m_pStyle = pStyle;
}


void CandlestickGraph::Make()
{
	OHLC const * ohlc = reinterpret_cast<OHLC const *>(m_data);

	// calculate DIP coordiantes
	m_lines.resize(m_n);
	m_up_rects.clear();
	m_down_rects.clear();
	m_no_change.clear();
	m_up_rects.reserve(m_n/2);
	m_down_rects.reserve(m_n/2);

	float x_diff = m_axes->GetDataRectXDiff();
	float boxHalfWidth = min(15.0f, 0.4f * x_diff / m_n);

	for (int i = 0; i < m_n; i++)
	{
		float x = m_axes->XtoDIP(static_cast<double>(i));
		float y1 = m_axes->YtoDIP(ohlc[i].low);
		float y2 = m_axes->YtoDIP(ohlc[i].high);
		D2D1_POINT_2F start = D2D1::Point2F(x, y1);
		D2D1_POINT_2F end = D2D1::Point2F(x, y2);
		m_lines[i] = { start, end };

		D2D1_RECT_F temp = D2D1::RectF(
			x - boxHalfWidth,
			0,
			x + boxHalfWidth,
			0
		);
		if (ohlc[i].close > ohlc[i].open)
		{
			temp.top = m_axes->YtoDIP(ohlc[i].close);
			temp.bottom = m_axes->YtoDIP(ohlc[i].open);
			m_up_rects.push_back(temp);
		}
		else if (ohlc[i].close < ohlc[i].open)
		{
			temp.top = m_axes->YtoDIP(ohlc[i].open);
			temp.bottom = m_axes->YtoDIP(ohlc[i].close);
			m_down_rects.push_back(temp);
		}
		else
		{
			D2D1_POINT_2F start = D2D1::Point2F(x - boxHalfWidth, y2);
			D2D1_POINT_2F end = D2D1::Point2F(x + boxHalfWidth, y2);
			m_no_change.push_back({ start,end });
		}
	}
}

void CandlestickGraph::Paint(D2Objects const & d2)
{
	d2.pBrush->SetColor(D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f));
	for (auto line : m_lines)
		d2.pRenderTarget->DrawLine(line.start, line.end, d2.pBrush);

	for (auto line : m_no_change)
		d2.pRenderTarget->DrawLine(line.start, line.end, d2.pBrush);

	d2.pBrush->SetColor(D2D1::ColorF(32.0f / 255, 214.0f / 255, 126.0f / 255, 1.0f));
	for (auto rect : m_up_rects)
		d2.pRenderTarget->FillRectangle(rect, d2.pBrush);

	d2.pBrush->SetColor(D2D1::ColorF(1.0f, 0.2f, 0.2f, 1.0f));
	for (auto rect : m_down_rects)
		d2.pRenderTarget->FillRectangle(rect, d2.pBrush);

}

void BarGraph::Make()
{
	std::pair<double, D2D1_COLOR_F> const * data = reinterpret_cast<std::pair<double, D2D1_COLOR_F> const *>(m_data);
	
	m_bars.resize(m_n);

	float x_diff = m_axes->GetDataRectXDiff();
	float boxHalfWidth = 0.4f * x_diff / m_n;
	for (int i = 0; i < m_n; i++)
	{
		float x = m_axes->XtoDIP(static_cast<double>(i));
		float y1, y2;
		if (data[i].first < 0)
		{
			y1 = m_axes->YtoDIP(data[i].first);
			y2 = m_axes->YtoDIP(0);
		}
		else
		{
			y1 = m_axes->YtoDIP(0);
			y2 = m_axes->YtoDIP(data[i].first);
		}

		m_bars[i] = D2D1::RectF(
			x - boxHalfWidth,
			y2,
			x + boxHalfWidth,
			y1
		);
	}
}

void BarGraph::Paint(D2Objects const & d2)
{
	std::pair<double, D2D1_COLOR_F> const * data = reinterpret_cast<std::pair<double, D2D1_COLOR_F> const *>(m_data);
	for (int i = 0; i < m_n; i++)
	{
		d2.pBrush->SetColor(data[i].second);
		d2.pRenderTarget->FillRectangle(m_bars[i], d2.pBrush);
	}
}
