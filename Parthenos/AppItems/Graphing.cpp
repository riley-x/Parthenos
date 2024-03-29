#include "../stdafx.h"
#include "Graphing.h"
#include "../Utilities/utilities.h"

Axes::Axes(HWND hwnd, D2Objects const & d2, CTPMessageReceiver * parent)
	: AppItem(hwnd, d2, parent)
{
	// Create marker geometries
	CreateTriangleMarker(m_upMarker, 1);
	CreateTriangleMarker(m_dnMarker, -1);
	CreateXMarker();
}

Axes::~Axes()
{
	Clear();
}

void Axes::Clear()
{
	for (size_t i = 0; i < nGraphGroups; i++)
	{
		for (Graph *item : m_graphObjects[i]) {
			if (item) delete item;
		}
		m_graphObjects[i].clear();
		m_imade[i] = 0;
	}
	m_nPoints = 0;
	m_primaryCache.Reset();
	m_xTicks.clear();
	m_yTicks.clear();
	m_dates.clear();
	m_userXLabels.clear();
	m_grid_lines[0].clear();
	m_grid_lines[1].clear();
	m_hoverTextX = -1;
	SafeRelease(&m_hoverLayout);

	m_ismade = true;
	for (int i = 0; i < 4; i++)
		m_dataRange[i] = nan("");
	m_rescaled = false;
}

void Axes::Clear(GraphGroup group)
{
	if (group == GG_PRI)
	{
		for (int i = 0; i < 4; i++) m_dataRange[i] = nan("");
		m_dates.clear();
		m_hoverTextX = -1;
		m_nPoints = 0;
	}

	for (Graph *item : m_graphObjects[group]) if (item) delete item;
	m_graphObjects[group].clear();
	m_imade[group] = 0;
	m_ismade = false;
}

void Axes::Remove(GraphGroup group, std::wstring name)
{
	for (int i = (int)m_graphObjects[group].size() - 1; i >= 0; i--)
	{
		if (m_graphObjects[group][i]->m_name == name)
		{
			delete m_graphObjects[group][i];
			m_graphObjects[group].erase(m_graphObjects[group].begin() + i);
			m_imade[group] = 0; // excessive
			if (group != GG_TER) m_ismade = false; // need to remake cached image
		}
	}
}

// Converts all graph objects into DIPs. Creates a cached image for the first two graph groups and labels.
// Call when axes are rescaled or graphs are changed.
void Axes::Make()
{
	if (m_rescaled) Rescale(); // sets m_imade = 0, and also creates the labels and grids

	for (size_t i = 0; i < nGraphGroups; i++)
	{
		for (m_imade[i]; m_imade[i] < m_graphObjects[i].size(); m_imade[i]++)
			m_graphObjects[i][m_imade[i]]->Make();
	}
	m_ismade = true;
	CreateCachedImage();
}


// May want to call clear before this
void Axes::SetSize(D2D1_RECT_F dipRect)
{
	m_ismade = false;
	m_rescaled = true;

	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(dipRect);

	float pad = 14.0;
	m_axesRect.left = dipRect.left;
	m_axesRect.top = dipRect.top + m_titlePad;
	m_axesRect.right = dipRect.right - m_ylabelWidth - pad;
	m_axesRect.bottom = (m_drawXLabels) ? dipRect.bottom - m_labelHeight - pad : dipRect.bottom;

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
	m_d2.pD2DContext->FillRectangle(m_dipRect, m_d2.pBrush);

	// Selection
	if (m_select && m_selectStart >= 0 && m_selectEnd != m_selectStart)
	{
		m_d2.pBrush->SetColor(Colors::MAIN_BACKGROUND);
		m_d2.pD2DContext->FillRectangle(
			D2D1::RectF(
				(m_selectStart < m_selectEnd) ? XtoDIP(m_selectStart) : XtoDIP(m_selectEnd),
				m_axesRect.top,
				(m_selectStart < m_selectEnd) ? XtoDIP(m_selectEnd) : XtoDIP(m_selectStart),
				m_axesRect.bottom
			),
			m_d2.pBrush
		);
	}

	// Cached bitmap
	if (m_primaryCache) m_d2.pD2DContext->DrawBitmap(m_primaryCache.Get());

	// Ancillary graphs
	for (Graph* const & graph : m_graphObjects[nGraphGroups - 1])
		graph->Paint(m_d2);

	// Crosshairs
	m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	if (m_hoverLoc.x >= 0 && (m_hoverStyle == HoverStyle::crosshairs // draw vertical line
						   || m_hoverStyle == HoverStyle::snap))
	{
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(m_hoverLoc.x, m_axesRect.top),
			D2D1::Point2F(m_hoverLoc.x, m_axesRect.bottom),
			m_d2.pBrush, 1.0f, m_d2.pDashedStyle
		);
	}
	if (m_hoverLoc.y >= 0 && m_hoverStyle == HoverStyle::crosshairs) // draw horizontal line
	{
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(m_axesRect.left, m_hoverLoc.y),
			D2D1::Point2F(m_axesRect.right, m_hoverLoc.y),
			m_d2.pBrush, 1.0f, m_d2.pDashedStyle
		);
	}

	// Axes
	m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	if (!std::isnan(m_xAxisPos))
	{
		float xaxis_dip_pos;
		if (m_xAxisPos == -std::numeric_limits<float>::infinity()) xaxis_dip_pos = m_axesRect.bottom;
		else xaxis_dip_pos = DPIScale::SnapToPixelY(YtoDIP(m_xAxisPos)) - DPIScale::hpy();

		Line_t xaxis = { D2D1::Point2F(m_axesRect.left,  xaxis_dip_pos),
						 D2D1::Point2F(m_axesRect.right, xaxis_dip_pos) };
		m_d2.pD2DContext->DrawLine(xaxis.start, xaxis.end, m_d2.pBrush, DPIScale::py());
	}
	if (!std::isnan(m_yAxisPos))
	{
		float yaxis_dip_pos;
		if (m_xAxisPos == std::numeric_limits<float>::infinity()) yaxis_dip_pos = m_axesRect.right;
		else yaxis_dip_pos = DPIScale::SnapToPixelY(XtoDIP(m_yAxisPos)) - DPIScale::hpx();

		Line_t yaxis = { D2D1::Point2F(m_axesRect.right, m_axesRect.bottom),
						 D2D1::Point2F(m_axesRect.right, m_axesRect.top) };
		m_d2.pD2DContext->DrawLine(yaxis.start, yaxis.end, m_d2.pBrush, DPIScale::px());
	}

	// Hover text
	if (m_hoverTextX >= 0)
	{
		m_d2.pBrush->SetColor(Colors::MENU_BACKGROUND);
		m_d2.pD2DContext->FillRectangle(m_hoverRect, m_d2.pBrush);
		m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
		m_d2.pD2DContext->DrawTextLayout(
			D2D1::Point2F(m_hoverRect.left + m_labelPad, m_hoverRect.top + m_labelPad),
			m_hoverLayout,
			m_d2.pBrush
		);
	}
}

bool Axes::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	bool invalidate = false;

	// Get snapped x position
	int x = static_cast<int>(round(DIPtoX(cursor.x))); // all x values are [0, n)
	if (x < 0) x = 0; // occurs from padding so that xmin < 0
	else if (x > (int)m_nPoints - 1) x = (int)m_nPoints - 1;

	// Selection
	if (m_select && m_selectStart >= 0 && m_selectEnd != x)
	{
		m_selectEnd = x;
		invalidate = true;
	}

	// Hover text
	bool process = !handeled && m_nPoints != 0 && m_hoverStyle != HoverStyle::none;
	if (process && inRect(cursor, m_axesRect))
	{
		m_hoverLoc = cursor;
		m_hoverLoc.x = XtoDIP(x);
		CreateHoverText(static_cast<size_t>(x), cursor);
		handeled = true;
		invalidate = true;
	}
	else if (process && InMouseWatch(cursor))
	{
		if (inRectX(cursor.x, m_axesRect)) // necessarily not in y, or else in(m_axesRect) barring overlapping
		{
			m_hoverLoc = { XtoDIP(x), -1 };
		}
		// else if (inRectY(cursor.y, m_axesRect))

		if (m_hoverTextX >= 0)
		{
			m_hoverTextX = -1;
			invalidate = true;
		}
	}
	else 
	{
		m_hoverLoc = { -1, -1 };
		if (m_hoverTextX >= 0)
		{
			m_hoverTextX = -1;
			invalidate = true;
		}
	}

	if (invalidate) ::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	return handeled;
}

bool Axes::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	bool track = false;
	if (!handeled && m_select)
	{
		if (inRect(cursor, m_axesRect))
		{
			m_selectActive = true;
			track = true;
			m_parent->PostClientMessage(this, L"", CTPMessage::MOUSE_CAPTURED, 1); // should forward to parthenos
		}
		else if (InMouseWatch(cursor) && inRectX(cursor.x, m_axesRect)) // Mirror selection, but don't send messages ie
		{
			track = true;
			m_selectActive = false;
		}
	}

	if (track)
	{
		m_selectStart = static_cast<int>(round(DIPtoX(cursor.x))); // all x values are [0, n)
		if (m_selectStart < 0) m_selectStart = 0; // occurs from padding so that xmin < 0
		else if (m_selectStart > (int)m_nPoints - 1) m_selectStart = (int)m_nPoints - 1;
		m_selectEnd = m_selectStart;
	}
	else if (m_select && m_selectStart >= 0)
	{
		ResetSelection();
	}

	return m_selectActive;
}

void Axes::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (m_select && m_selectActive)
	{
		m_parent->SendClientMessage(this, L"", CTPMessage::MOUSE_CAPTURED, -1);
		if (m_selectStart < m_selectEnd) // selectEnd is already set by MouseMove (robust?)
			m_parent->SendClientMessage(this, L"", CTPMessage::AXES_SELECTION, m_selectStart, m_selectEnd);
		else if (m_selectStart > m_selectEnd)
			m_parent->SendClientMessage(this, L"", CTPMessage::AXES_SELECTION, m_selectEnd, m_selectStart);
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	}
	
	if (m_select && m_selectStart >= 0) // Inclusive of above case
	{
		ResetSelection();
	}
}

void Axes::Candlestick(std::vector<OHLC> const & ohlc, GraphGroup group)
{
	size_t n = ohlc.size();
	if (n == 0) return;
	
	size_t offset = 0;
	if (group == GG_PRI) // scale axes accordingly
	{
		// check to see if dates already set by another graph
		bool get_dates = m_dates.empty();
		if (!get_dates)
		{
			if (m_dates.size() != n || m_dates.front() != ohlc[0].date || m_dates.back() != ohlc[n - 1].date)
				return OutputMessage(L"Error: Dates don't line up\n");
		}
		else
		{
			m_dates.reserve(n);
		}

		// set x values to [0, n-1]
		double xmin = -0.5; // extra pad
		double xmax = n - 0.5;
		double low_min = ohlc[0].low;
		double high_max = ohlc[0].high;
		for (size_t i = 0; i < n; i++)
		{
			if (get_dates) m_dates.push_back(ohlc[i].date);
			if (ohlc[i].low < low_min) low_min = ohlc[i].low;
			if (ohlc[i].high > high_max) high_max = ohlc[i].high;
		}

		m_rescaled = setDataRange(xmin, xmax, low_min, high_max) || m_rescaled;
		m_nPoints = n;
	}
	else // align dates to primary graphs
	{
		auto it = std::lower_bound(m_dates.begin(), m_dates.end(), ohlc[0].date);
		if (it == m_dates.end()) return OutputMessage(L"Error: Dates don't line up\n");
		offset = static_cast<date_t>(it - m_dates.begin());

		if (offset >= m_nPoints) return;
		else if (offset + n > m_nPoints) n = m_nPoints - offset; // truncate

		if (group == GG_SEC) // scale y axes
		{
			double low_min = ohlc[0].low;
			double high_max = ohlc[0].high;
			for (size_t i = 0; i < n; i++)
			{
				if (ohlc[i].low < low_min) low_min = ohlc[i].low;
				else if (ohlc[i].high > high_max) high_max = ohlc[i].high;
			}
			m_rescaled = setDataRange(dataRange::ymin, low_min) || m_rescaled;
			m_rescaled = setDataRange(dataRange::ymax, high_max) || m_rescaled;
		}
	}

	auto graph = new CandlestickGraph(this, offset);
	graph->SetData(ohlc);
	m_graphObjects[GG_PRI].push_back(graph);
	m_ismade = false;
}

void Axes::Line(std::vector<date_t> const & dates, std::vector<double> const & ydata, LineProps props, GraphGroup group, std::wstring name)
{
	size_t n = dates.size();
	if (n == 0 || n != ydata.size()) return;

	size_t offset = 0;
	if (group == GG_PRI) // scale axes accordingly
	{
		if (m_dates.empty()) m_dates = dates;
		else if (m_dates.size() != n || m_dates.front() != dates[0] || m_dates.back() != dates[n - 1])
			return OutputMessage(L"Error: Dates don't line up\n");

		// sets x values to [0, n-1]
		double xmin = -0.5; // extra pad
		double xmax = n - 0.5;
		double ymin = ydata[0];
		double ymax = ydata[0];
		for (size_t i = 0; i < n; i++)
		{
			if (ydata[i] < ymin) ymin = ydata[i];
			else if (ydata[i] > ymax) ymax = ydata[i];
		}

		m_rescaled = setDataRange(xmin, xmax, ymin, ymax) || m_rescaled;
		m_nPoints = n;
	}
	else // align dates to primary graphs
	{
		auto it = std::lower_bound(m_dates.begin(), m_dates.end(), dates[0]);
		if (it == m_dates.end()) return OutputMessage(L"Error: Dates don't line up\n");
		offset = static_cast<size_t>(it - m_dates.begin());

		if (offset >= m_nPoints) return;
		else if (offset + n > m_nPoints) n = m_nPoints - offset; // truncate

		if (group == GG_SEC) // scale y-axes
		{
			double ymin = ydata[0];
			double ymax = ydata[0];
			for (size_t i = 0; i < n; i++)
			{
				if (ydata[i] < ymin) ymin = ydata[i];
				else if (ydata[i] > ymax) ymax = ydata[i];
			}
			m_rescaled = setDataRange(dataRange::ymin, ymin) || m_rescaled;
			m_rescaled = setDataRange(dataRange::ymax, ymax) || m_rescaled;
		}

	}

	auto graph = new LineGraph(this, offset);
	graph->m_name = name;
	graph->SetData(ydata);
	graph->SetLineProperties(props);
	m_graphObjects[group].push_back(graph);
	m_ismade = false;
}

void Axes::Bar(std::vector<std::pair<double, D2D1_COLOR_F>> const & data, 
	std::vector<std::wstring> const & labels, GraphGroup group, size_t offset)
{
	size_t n = data.size();
	if (n == 0) return;

	if (group == GG_PRI) // scale axes accordingly
	{
		if (m_nPoints > 0 && m_nPoints != n) 
			return OutputMessage(L"Error: Bar data does not agree with current primary group");

		// sets x values to [0, n-1]
		double xmin = -0.5; // extra padding
		double xmax = n - 0.5;
		double ymin = 0;
		double ymax = 0;
		for (size_t i = 0; i < n; i++)
		{
			if (data[i].first < ymin) ymin = data[i].first;
			else if (data[i].first > ymax) ymax = data[i].first;
		}

		m_rescaled = setDataRange(xmin, xmax, ymin, ymax) || m_rescaled;
		m_nPoints = n;
	}
	else
	{
		if (offset >= m_nPoints) return;
		else if (offset + n > m_nPoints) n = m_nPoints - offset; // truncate

		if (group == GG_SEC) // scale y-axis
		{
			double ymin = 0;
			double ymax = 0;
			for (size_t i = 0; i < n; i++)
			{
				if (data[i].first < ymin) ymin = data[i].first;
				else if (data[i].first > ymax) ymax = data[i].first;
			}
			m_rescaled = setDataRange(dataRange::ymin, ymin) || m_rescaled;
			m_rescaled = setDataRange(dataRange::ymax, ymax) || m_rescaled;
		}
	}

	auto graph = new BarGraph(this, offset);
	graph->SetData(data);
	graph->SetLabels(labels);
	m_graphObjects[group].push_back(graph);
	m_ismade = false;
}

// Points are provided correct except for x values, which are date_t.
void Axes::DatePoints(std::vector<PointProps> points, GraphGroup group, std::wstring name)
{
	if (group == GG_PRI || m_dates.empty() || points.empty()) return;

	double ymin = nan("");
	double ymax = nan("");
	for (PointProps & p : points)
	{
		date_t d = static_cast<date_t>(p.x);
		if (d < m_dates.front() || d > m_dates.back()) p.x = std::nan("");
		else
		{
			auto it = std::lower_bound(m_dates.begin(), m_dates.end(), d);
			if (it == m_dates.end()) p.x = std::nan("");
			else
			{
				p.x = static_cast<double>(it - m_dates.begin());
				if (!(ymin < p.y)) ymin = p.y; // negation so always true against nan
				else if (!(ymax > p.y)) ymax = p.y;
			}
		}
	}

	// scale y-axis
	if (group == GG_SEC)
	{
		m_rescaled = setDataRange(dataRange::ymin, ymin) || m_rescaled;
		m_rescaled = setDataRange(dataRange::ymax, ymax) || m_rescaled;
	}

	auto graph = new PointsGraph(this);
	graph->m_name = name;
	graph->SetData(points);
	m_graphObjects[group].push_back(graph);
	m_ismade = false;
}

void Axes::SetTitle(std::wstring const & title, D2Objects::Formats format)
{
	m_title = title;
	m_titlePad = FontSize(format) * 2.0f; // could check for number of lines here
	m_titleFormat = format;
}

ID2D1PathGeometry* Axes::GetMarker(MarkerStyle m)
{
	switch (m)
	{
	case MarkerStyle::down:
		return m_dnMarker.Get();
	case MarkerStyle::up:
		return m_upMarker.Get();
	case MarkerStyle::x:
	default:
		return m_xxMarker.Get();
	}
}

// Call from Make() on data range changing. Recalculates the ticks and sets all imade = 0
void Axes::Rescale()
{
	m_data_xdiff = m_dataRange[static_cast<int>(dataRange::xmax)]
		- m_dataRange[static_cast<int>(dataRange::xmin)];
	m_data_ydiff = m_dataRange[static_cast<int>(dataRange::ymax)]
		- m_dataRange[static_cast<int>(dataRange::ymin)];

	if (m_drawXLabels || m_drawXGridLines) CalculateXTicks();
	CalculateYTicks();

	for (size_t i = 0; i < nGraphGroups; i++) m_imade[i] = 0; // remake everything
	m_rescaled = false;
}

void Axes::CalculateXTicks()
{
	if (!m_userXLabels.empty()) CalculateXTicks_User();
	else if (!m_dates.empty()) CalculateXTicks_Dates();
}

// Calculates human-friendly formatted dates. Auto sets spacing and labels.
// Populates m_grid_lines[0] and m_xTicks.
void Axes::CalculateXTicks_Dates()
{
	size_t nmax = static_cast<size_t>(m_rect_xdiff / (3.0f * m_labelHeight));
	if (nmax == 0) return;
	size_t step = (m_dates.size() + nmax - 1) / nmax; // round up
	if (step == 0) return;

	float xdip = XtoDIP(0); // assumes dates are plotted as [0, n-1]
	float spacing = XtoDIP((double)step) - xdip;

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


		m_xTicks.push_back({ xdip, static_cast<double>(i), std::wstring(buffer) });
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

// Clears m_grid_lines[0] and populates m_xTicks.
void Axes::CalculateXTicks_User()
{
	m_xTicks.clear();
	m_grid_lines[0].clear();
	for (size_t i = 0; i < m_userXLabels.size(); i++)
	{
		m_xTicks.push_back({ XtoDIP((double)i), (double)i, m_userXLabels[i] });
	}
}

void Axes::CalculateYTicks()
{
	if (!m_userYGridLines.empty()) CalculateYTicks_User();
	else CalculateYTicks_Auto();
}

// Gets human-friendly yticks
void Axes::CalculateYTicks_Auto()
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

void Axes::CalculateYTicks_User()
{
	m_yTicks.clear();
	m_grid_lines[1].clear();

	for (double y : m_userYGridLines)
	{
		float ydip = YtoDIP(y);
		wchar_t buffer[20] = {};
		swprintf_s(buffer, _countof(buffer), L"%.2lf", y);
		m_yTicks.push_back({ ydip, y, std::wstring(buffer) });
		m_grid_lines[1].push_back({ 
			D2D1::Point2F(m_axesRect.left, ydip),
			D2D1::Point2F(m_axesRect.right, ydip) 
		});
	}
}

// Creates the cached image containing graph groups 0 and 1, the title, 
// gridlines, and axes labels.
// Does not contain graph group 2, the actual axes lines (need to be painted on top), 
// or the hover artifacts.
void Axes::CreateCachedImage()
{
	RECT rc;
	GetClientRect(m_hwnd, &rc);

	m_primaryCache.Reset();
	// Create a bitmap.
	HRESULT hr = m_d2.pD2DContext->CreateBitmap(
		D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top),
		nullptr, 0,
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			DPIScale::DPIX(), 
			DPIScale::DPIY()),
		&m_primaryCache
	);
	if (FAILED(hr)) return OutputHRerr(hr, L"Axes::CreateCachedImage failed");


	// This function should always be called from OnPaint(), i.e. inside the current paint loop,
	// so end the current draw.
	hr = m_d2.pD2DContext->EndDraw();
	if (FAILED(hr)) OutputHRerr(hr, L"Axes::CreateCachedImage old paint failed");

	// Preserve the pre-existing target.
	ComPtr<ID2D1Image> oldTarget;
	m_d2.pD2DContext->GetTarget(&oldTarget);

	// Render static content to the sceneBitmap.
	m_d2.pD2DContext->SetTarget(m_primaryCache.Get());
	m_d2.pD2DContext->BeginDraw();
	m_d2.pD2DContext->Clear(); // the bitmap is uninitialized right now, clear to transparent

	// Grid lines
	m_d2.pBrush->SetColor(Colors::DULL_LINE);
	if (m_drawXGridLines) // x lines
	{
		for (auto line : m_grid_lines[0]) 
			m_d2.pD2DContext->DrawLine(line.start, line.end, m_d2.pBrush, 0.8f, m_d2.pDashedStyle);
	}
	for (auto line : m_grid_lines[1]) // y lines
	{
		m_d2.pD2DContext->DrawLine(line.start, line.end, m_d2.pBrush, 0.8f, m_d2.pDashedStyle);
	}

	// Title
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	if (!m_title.empty())
	{
		m_d2.pTextFormats[m_titleFormat]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_d2.pD2DContext->DrawTextW(
			m_title.c_str(),
			(UINT32)m_title.size(),
			m_d2.pTextFormats[m_titleFormat],
			D2D1::RectF(m_dipRect.left, m_dipRect.top, m_dipRect.right, m_axesRect.top),
			m_d2.pBrush
		);
		m_d2.pTextFormats[m_titleFormat]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	}

	// Labels
	if (m_drawXLabels)
	{
		for (auto tick : m_xTicks)
		{
			float loc = std::get<0>(tick);
			std::wstring label = std::get<2>(tick);

			m_d2.pD2DContext->DrawText(
				label.c_str(),
				(UINT32)label.size(),
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

		m_d2.pD2DContext->DrawText(
			label.c_str(),
			(UINT32)label.size(),
			m_d2.pTextFormats[D2Objects::Segoe10],
			D2D1::RectF(m_axesRect.right + m_labelPad,
				loc - m_labelHeight / 2.0f,
				m_dipRect.right,
				loc + m_labelHeight / 2.0f),
			m_d2.pBrush
		);
	}

	// Primary and secondary graphs
	for (size_t group = 0; group < nGraphGroups - 1u; group++)
		for (Graph* const & graph : m_graphObjects[group])
			graph->Paint(m_d2);

	hr = m_d2.pD2DContext->EndDraw();
	if (FAILED(hr)) OutputHRerr(hr, L"Axes::CreateCachedImage failed");

	// Restore old target
	m_d2.pD2DContext->SetTarget(oldTarget.Get());
	m_d2.pD2DContext->BeginDraw();
}

void Axes::CreateTriangleMarker(ComPtr<ID2D1PathGeometry> & geometry, int parity)
{
	float const sqrt_3 = 0.866f;
	ComPtr<ID2D1GeometrySink> pSink;

	HRESULT hr = m_d2.pFactory->CreatePathGeometry(&geometry);
	if (SUCCEEDED(hr))
	{
		hr = geometry->Open(&pSink);
	}
	if (SUCCEEDED(hr))
	{
		pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

		pSink->BeginFigure(
			D2D1::Point2F(-sqrt_3, 0.5f * parity), // bottom-left
			D2D1_FIGURE_BEGIN_FILLED
		);
		D2D1_POINT_2F points[3] = {
			   D2D1::Point2F(0, -1.0f * parity), // top
			   D2D1::Point2F(sqrt_3, 0.5f * parity), // bottom-right
			   D2D1::Point2F(-sqrt_3, 0.5f * parity),
		};
		pSink->AddLines(points, ARRAYSIZE(points));
		pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
		hr = pSink->Close();
	}
}

void Axes::CreateXMarker()
{
	float sqrt1_2 = 0.7071f;
	ComPtr<ID2D1GeometrySink> pSink;

	HRESULT hr = m_d2.pFactory->CreatePathGeometry(&m_xxMarker);
	if (SUCCEEDED(hr))
	{
		hr = m_xxMarker->Open(&pSink);
	}
	if (SUCCEEDED(hr))
	{
		pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

		pSink->BeginFigure(
			D2D1::Point2F(-sqrt1_2, -sqrt1_2), // top-left
			D2D1_FIGURE_BEGIN_HOLLOW
		);
		pSink->AddLine(D2D1::Point2F(sqrt1_2, sqrt1_2)); // bottom-right
		pSink->EndFigure(D2D1_FIGURE_END_OPEN);

		pSink->BeginFigure(
			D2D1::Point2F(sqrt1_2, -sqrt1_2), // top-right
			D2D1_FIGURE_BEGIN_HOLLOW
		);
		pSink->AddLine(D2D1::Point2F(-sqrt1_2, sqrt1_2)); // bottom-left
		pSink->EndFigure(D2D1_FIGURE_END_OPEN);

		hr = pSink->Close();
	}
}

void Axes::CreateHoverText(size_t xind, D2D1_POINT_2F cursor)
{
	if (m_hoverTextX != (int)xind || m_hoverStyle == HoverStyle::crosshairs)
	{
		// Calculate new text and layout
		m_hoverTextX = (int)xind;
		std::wstring xlabel, ylabel;

		// Get x label
		if (xind < m_userXLabels.size()) xlabel = m_userXLabels[xind];
		else if (xind < m_dates.size()) xlabel = DateToWString(m_dates[xind]);

		// Get y label (and adjust crosshairs for HoverStyle::snap) from first graph object
		ylabel = (m_hoverStyle == HoverStyle::snap) ? m_graphObjects[0][0]->GetYLabel(xind)
			: (std::wstringstream() << std::fixed << std::setprecision(2) << DIPtoY(cursor.y)).str();

		// Make layout
		std::wstring label = xlabel + L"\n" + ylabel;
		SafeRelease(&m_hoverLayout);
		m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			label.c_str(),	// The string to be laid out and formatted.
			(UINT32)label.size(),	// The length of the string.
			m_d2.pTextFormats[D2Objects::Formats::Segoe12],	// The text format to apply to the string
			200.0f,			// The width of the layout box.
			200.0f,			// The height of the layout box.
			&m_hoverLayout	// The IDWriteTextLayout interface pointer.
		);
		m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	// Calculate bounding box
	if (m_hoverLayout)
	{
		DWRITE_TEXT_METRICS metrics;
		m_hoverLayout->GetMetrics(&metrics);
		if (cursor.x + metrics.width + 10.0f > m_dipRect.right) // go left of cursor
		{
			m_hoverRect.left = cursor.x - metrics.width - 2.0f * m_labelPad;
			m_hoverRect.right = cursor.x;
		}
		else
		{
			m_hoverRect.left = cursor.x;
			m_hoverRect.right = cursor.x + metrics.width + 2.0f * m_labelPad;
		}
		if (cursor.y + metrics.height + 20.0f > m_dipRect.bottom) // go above cursor
		{
			m_hoverRect.top = cursor.y - (metrics.height + 2.0f * m_labelPad);
			m_hoverRect.bottom = cursor.y;
		}
		else
		{
			m_hoverRect.top = cursor.y + 12.0f; // TODO height of cursor?
			m_hoverRect.bottom = cursor.y + 12.0f + metrics.height + 2.0f * m_labelPad;
		}
	}

}

bool Axes::InMouseWatch(D2D1_POINT_2F cursor)
{
	for (const Axes* a : m_mouseWatch)
		if (inRect(cursor, a->GetAxesRect())) return true;
	return false;
}

///////////////////////////////////////////////////////////
// --- LineGraph ---

// Caculates the DIP coordinates for in_data (setting x values to [0,n) )
// and adds line segments connecting the points
void LineGraph::Make()
{	
	size_t n = m_y.size();
	if (n == 0) return;
	m_lines.resize(n - 1);

	float x = m_axes->XtoDIP(static_cast<double>(m_offset));
	float y = m_axes->YtoDIP(m_y[0]);
	D2D1_POINT_2F start;
	D2D1_POINT_2F end = D2D1::Point2F(x, y);
	for (size_t i = 1; i < n; i++)
	{
		start = end;
		x = m_axes->XtoDIP(static_cast<double>(m_offset + i));
		y = m_axes->YtoDIP(m_y[i]);
		end = D2D1::Point2F(x, y);
		m_lines[i - 1] = { start, end };
	}
}

void LineGraph::Paint(D2Objects const & d2)
{
	d2.pBrush->SetColor(m_props.color);
	for (auto line : m_lines)
	{
		d2.pD2DContext->DrawLine(line.start, line.end, d2.pBrush, m_props.stroke_width, m_props.pStyle);
	}
}

std::wstring LineGraph::GetYLabel(size_t i) const
{
	if (i - m_offset < m_y.size()) 
		return (std::wstringstream() << std::fixed << std::setprecision(2) << m_y[i - m_offset]).str();
	return std::wstring();
}

///////////////////////////////////////////////////////////
// --- CandlestickGraph ---

void CandlestickGraph::Make()
{
	size_t n = m_ohlc.size();

	m_lines.resize(n);
	m_up_rects.clear();
	m_down_rects.clear();
	m_no_change.clear();
	m_up_rects.reserve(n/2);
	m_down_rects.reserve(n/2);

	float x_diff = m_axes->GetDataRectXDiff();
	float boxHalfWidth = min(15.0f, 0.4f * x_diff / m_axes->GetNPoints());

	for (size_t i = 0; i < n; i++)
	{
		float x = m_axes->XtoDIP(static_cast<double>(m_offset + i));
		float y1 = m_axes->YtoDIP(m_ohlc[i].low);
		float y2 = m_axes->YtoDIP(m_ohlc[i].high);
		D2D1_POINT_2F start = D2D1::Point2F(x, y1);
		D2D1_POINT_2F end = D2D1::Point2F(x, y2);
		m_lines[i] = { start, end };

		D2D1_RECT_F temp = D2D1::RectF(
			x - boxHalfWidth,
			0,
			x + boxHalfWidth,
			0
		);
		if (m_ohlc[i].close > m_ohlc[i].open)
		{
			temp.top = m_axes->YtoDIP(m_ohlc[i].close);
			temp.bottom = m_axes->YtoDIP(m_ohlc[i].open);
			m_up_rects.push_back(temp);
		}
		else if (m_ohlc[i].close < m_ohlc[i].open)
		{
			temp.top = m_axes->YtoDIP(m_ohlc[i].open);
			temp.bottom = m_axes->YtoDIP(m_ohlc[i].close);
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
	d2.pBrush->SetColor(Colors::BRIGHT_LINE);
	for (auto line : m_lines)
		d2.pD2DContext->DrawLine(line.start, line.end, d2.pBrush);

	for (auto line : m_no_change)
		d2.pD2DContext->DrawLine(line.start, line.end, d2.pBrush);

	d2.pBrush->SetColor(Colors::CANDLEGREEN);
	for (auto rect : m_up_rects)
		d2.pD2DContext->FillRectangle(rect, d2.pBrush);

	d2.pBrush->SetColor(Colors::RED);
	for (auto rect : m_down_rects)
		d2.pD2DContext->FillRectangle(rect, d2.pBrush);

}

std::wstring CandlestickGraph::GetYLabel(size_t i) const
{
	if (i - m_offset < m_ohlc.size()) return m_ohlc[i - m_offset].to_wstring(true);
	return std::wstring();
}


///////////////////////////////////////////////////////////
// --- BarGraph ---

void BarGraph::Make()
{
	size_t n = m_data.size();
	m_bars.resize(n);
	m_labelRects.resize(min(m_labels.size(), n));

	float x_diff = m_axes->GetDataRectXDiff();
	float boxHalfWidth = 0.4f * x_diff / m_axes->GetNPoints();
	float textHalfWidth = 0.5f * x_diff / m_axes->GetNPoints();
	float textMinWidth = 20.0f;
	if (textHalfWidth < textMinWidth) m_labelRects.clear();
	
	for (size_t i = 0; i < n; i++)
	{
		float x = m_axes->XtoDIP(static_cast<double>(i + m_offset));
		float y1, y2; // y1 = bottom, y2 = top
		if (m_data[i].first < 0)
		{
			y1 = m_axes->YtoDIP(m_data[i].first);
			y2 = m_axes->YtoDIP(0);
		}
		else
		{
			y1 = m_axes->YtoDIP(0);
			y2 = m_axes->YtoDIP(m_data[i].first);
		}

		m_bars[i] = D2D1::RectF(
			x - boxHalfWidth,
			y2,
			x + boxHalfWidth,
			y1
		);

		if (i < m_labels.size() && textHalfWidth >= textMinWidth)
		{
			m_labelRects[i] = D2D1::RectF(
				x - textHalfWidth,
				y2 - m_axes->GetDataPad(), // label always above bar
				x + textHalfWidth,
				y2
			);
		}
	}
}

void BarGraph::Paint(D2Objects const & d2)
{
	for (size_t i = 0; i < m_data.size(); i++)
	{
		d2.pBrush->SetColor(m_data[i].second);
		d2.pD2DContext->FillRectangle(m_bars[i], d2.pBrush);
	}

	d2.pBrush->SetColor(Colors::MAIN_TEXT);
	d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	for (size_t i = 0; i < m_labelRects.size(); i++)
	{
		d2.pD2DContext->DrawText(
			m_labels[i].c_str(),
			(UINT32)m_labels[i].size(),
			d2.pTextFormats[D2Objects::Formats::Segoe12],
			m_labelRects[i],
			d2.pBrush
		);
	}
	d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}

std::wstring BarGraph::GetYLabel(size_t i) const
{
	if (i - m_offset < m_data.size()) 
		return (std::wstringstream() << std::fixed << std::setprecision(2) << m_data[i - m_offset].first).str();
	return std::wstring();
}


///////////////////////////////////////////////////////////
// --- PointsGraph ---

void PointsGraph::Make()
{
	m_locs.resize(m_points.size());
	for (size_t i = 0; i < m_points.size(); i++)
	{
		m_locs[i] = D2D1::Point2F(
			m_axes->XtoDIP(m_points[i].x),
			m_axes->YtoDIP(m_points[i].y)
		);
	}
}

void PointsGraph::Paint(D2Objects const & d2)
{
	for (size_t i = 0; i < m_points.size(); i++)
	{
		d2.pBrush->SetColor(m_points[i].color);
		switch (m_points[i].style)
		{
		case MarkerStyle::circle:
			d2.pD2DContext->FillEllipse(D2D1::Ellipse(m_locs[i], m_points[i].scale, m_points[i].scale), d2.pBrush);
			break;
		case MarkerStyle::square:
			d2.pD2DContext->FillRectangle(D2D1::RectF(
				m_locs[i].x - m_points[i].scale,
				m_locs[i].y - m_points[i].scale,
				m_locs[i].x + m_points[i].scale,
				m_locs[i].y + m_points[i].scale
			), d2.pBrush);
			break;
		case MarkerStyle::up:
		case MarkerStyle::down:
		{
			D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(
				m_points[i].scale,
				m_points[i].scale,
				D2D1::Point2F(0, 0)
			);
			D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(m_locs[i].x, m_locs[i].y);
			d2.pD2DContext->SetTransform(scale * translation);
			d2.pD2DContext->FillGeometry(m_axes->GetMarker(m_points[i].style), d2.pBrush);
			d2.pD2DContext->SetTransform(D2D1::Matrix3x2F::Identity());
			break;
		}
		case MarkerStyle::x:
		{
			D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(
				m_points[i].scale,
				m_points[i].scale,
				D2D1::Point2F(0, 0)
			);
			D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(m_locs[i].x, m_locs[i].y);
			d2.pD2DContext->SetTransform(scale * translation);
			d2.pD2DContext->DrawGeometry(m_axes->GetMarker(m_points[i].style), d2.pBrush, 1.0f, d2.pHairlineStyle);
			d2.pD2DContext->SetTransform(D2D1::Matrix3x2F::Identity());
			break;
		}
		default:
			break;
		}
	}
}
