#include "stdafx.h"
#include "Graphing.h"
#include "utilities.h"

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
	m_primaryCache.Reset();
	m_xTicks.clear();
	m_yTicks.clear();
	m_dates.clear();
	m_userXLabels.clear();
	m_grid_lines[0].clear();
	m_grid_lines[1].clear();
	SafeRelease(&m_hoverLayout);

	m_ismade = true;
	for (int i = 0; i < 4; i++)
		m_dataRange[i] = nan("");
	m_rescaled = false;
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
	for (size_t i = 0; i < nGraphGroups; i++) m_imade[i] = 0;
	m_rescaled = true;

	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(dipRect);

	float pad = 14.0;
	m_axesRect.left = dipRect.left;
	m_axesRect.top = dipRect.top + m_titlePad;
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

	// Cached bitmap
	if (m_primaryCache) m_d2.pD2DContext->DrawBitmap(m_primaryCache.Get());

	// Ancillary graphs
	for (Graph* const & graph : m_graphObjects[nGraphGroups - 1])
		graph->Paint(m_d2);

	// Crosshairs
	if (m_hoverOn >= 0)
	{
		m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
		if (m_hoverStyle == HoverStyle::crosshairs) // draw horizontal line
		{
			m_d2.pD2DContext->DrawLine(
				D2D1::Point2F(m_axesRect.left, m_hoverLoc.y),
				D2D1::Point2F(m_axesRect.right, m_hoverLoc.y),
				m_d2.pBrush, 1.0f, m_d2.pDashedStyle
			);
		}
		// draw vertical line
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(m_hoverLoc.x, m_axesRect.top),
			D2D1::Point2F(m_hoverLoc.x, m_axesRect.bottom),
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
	if (m_hoverOn >= 0)
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
	if (handeled || m_hoverStyle == HoverStyle::none || m_graphObjects[0].empty() ||
		!inRect(cursor, m_axesRect))
	{
		if (m_hoverOn >= 0)
		{
			m_hoverOn = -1;
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
		return false;
	}

	// Get x position
	double x = round(DIPtoX(cursor.x));
	if (x < 0) x = 0; // occurs from padding so that xmin < 0
	double n = max(m_dates.size(), m_userXLabels.size());
	if (x > n - 1) x = n - 1;
	size_t xind = static_cast<size_t>(x); // all x values are [0, n)
	m_hoverLoc = cursor;
	m_hoverLoc.x = XtoDIP(x);

	if (m_hoverOn != (int)xind || m_hoverStyle == HoverStyle::crosshairs) 
	{
		// Calculate new text and layout
		m_hoverOn = (int)xind;
		std::wstring xlabel, ylabel;

		// Get x label
		if (xind < m_dates.size()) xlabel = DateToWString(m_dates[xind]);
		else if (xind < m_userXLabels.size()) xlabel = m_userXLabels[xind];

		// Get y label (and adjust crosshairs for HoverStyle::snap) from first graph object
		ylabel = (m_hoverStyle == HoverStyle::snap) ? m_graphObjects[0][0]->GetYLabel(xind) : std::to_wstring(DIPtoY(cursor.y));

		// Make layout
		std::wstring label = xlabel + L"\n" + ylabel;
		SafeRelease(&m_hoverLayout);
		m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			label.c_str(),	// The string to be laid out and formatted.
			label.size(),	// The length of the string.
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

	::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	return true;
}

void Axes::Candlestick(OHLC const * ohlc, int n, size_t group)
{
	if (group >= nGraphGroups) group = nGraphGroups - 1;
	
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

	m_rescaled = setDataRange(xmin, xmax, low_min, high_max) || m_rescaled;

	auto graph = new CandlestickGraph(this, ohlc, n);
	m_graphObjects[group].push_back(graph);
	m_ismade = false;
}

void Axes::Line(date_t const * dates, double const * ydata, int n, 
	D2D1_COLOR_F color, float stroke_width, ID2D1StrokeStyle * pStyle, size_t group)
{
	if (group >= nGraphGroups) group = nGraphGroups - 1;
	
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

	m_rescaled = setDataRange(xmin, xmax, ymin, ymax) || m_rescaled;

	auto graph = new LineGraph(this, ydata, n);
	graph->SetLineProperties(color, stroke_width, pStyle);
	m_graphObjects[group].push_back(graph);
	m_ismade = false;
}

void Axes::Bar(std::pair<double, D2D1_COLOR_F> const * data, int n, 
	std::vector<std::wstring> const & labels, size_t group)
{
	if (group >= nGraphGroups) group = nGraphGroups - 1;

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
	graph->SetLabels(labels);
	m_graphObjects[group].push_back(graph);
	m_ismade = false;
}

void Axes::SetTitle(std::wstring const & title, D2Objects::Formats format)
{
	m_title = title;
	m_titlePad = FontSize(format) * 2.0f; // could check for number of lines here
	m_titleFormat = format;
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

	for (size_t i = 0; i < nGraphGroups; i++) m_imade[i] = 0; // remake everything
	m_rescaled = false;
}

void Axes::CalculateXTicks()
{
	if (!m_dates.empty()) CalculateXTicks_Dates();
	else if (!m_userXLabels.empty()) CalculateXTicks_User();
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

// Creates the cached image containing the background, graph groups 0 and 1, the title, 
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
	m_d2.pD2DContext->EndDraw();

	// Preserve the pre-existing target.
	ComPtr<ID2D1Image> oldTarget;
	m_d2.pD2DContext->GetTarget(&oldTarget);

	// Render static content to the sceneBitmap.
	m_d2.pD2DContext->SetTarget(m_primaryCache.Get());
	m_d2.pD2DContext->BeginDraw();

	// Background
	m_d2.pBrush->SetColor(Colors::AXES_BACKGROUND);
	m_d2.pD2DContext->FillRectangle(m_dipRect, m_d2.pBrush);

	// Grid lines
	m_d2.pBrush->SetColor(Colors::DULL_LINE);
	for (auto line : m_grid_lines[0]) // x lines
	{
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
			m_title.size(),
			m_d2.pTextFormats[m_titleFormat],
			D2D1::RectF(m_dipRect.left, m_dipRect.top, m_dipRect.right, m_axesRect.top),
			m_d2.pBrush
		);
		m_d2.pTextFormats[m_titleFormat]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	}

	// Labels
	if (m_drawxLabels)
	{
		for (auto tick : m_xTicks)
		{
			float loc = std::get<0>(tick);
			std::wstring label = std::get<2>(tick);

			m_d2.pD2DContext->DrawText(
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

		m_d2.pD2DContext->DrawText(
			label.c_str(),
			label.size(),
			m_d2.pTextFormats[D2Objects::Segoe10],
			D2D1::RectF(m_axesRect.right + m_labelPad,
				loc - m_labelHeight / 2.0f,
				m_dipRect.right,
				loc + m_labelHeight / 2.0f),
			m_d2.pBrush
		);
	}

	// Primary and secondary graphs
	for (size_t group = 0; group < nGraphGroups - 2; group++)
		for (Graph* const & graph : m_graphObjects[group])
			graph->Paint(m_d2);

	m_d2.pD2DContext->EndDraw();

	// Restore old target
	m_d2.pD2DContext->SetTarget(oldTarget.Get());
	m_d2.pD2DContext->BeginDraw();
}

///////////////////////////////////////////////////////////
// --- LineGraph ---

// Caculates the DIP coordinates for in_data (setting x values to [0,n) )
// and adds line segments connecting the points
void LineGraph::Make()
{	
	double const * data = reinterpret_cast<double const *>(m_data);
	if (m_n == 0) return;
	m_lines.resize(m_n - 1);

	float x = m_axes->XtoDIP(static_cast<double>(0));
	float y = m_axes->YtoDIP(data[0]);
	D2D1_POINT_2F start;
	D2D1_POINT_2F end = D2D1::Point2F(x, y);
	for (size_t i = 1; i < m_n; i++)
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
		d2.pD2DContext->DrawLine(line.start, line.end, d2.pBrush, m_stroke_width, m_pStyle);
	}
}

std::wstring LineGraph::GetYLabel(size_t i) const
{
	double const * data = reinterpret_cast<double const *>(m_data);
	if (i < m_n) return std::to_wstring(data[i]);
	return std::wstring();
}

void LineGraph::SetLineProperties(D2D1_COLOR_F color, float stroke_width, ID2D1StrokeStyle * pStyle)
{
	m_color = color;
	m_stroke_width = stroke_width;
	m_pStyle = pStyle;
}

///////////////////////////////////////////////////////////
// --- CandlestickGraph ---

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

	for (size_t i = 0; i < m_n; i++)
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
		d2.pD2DContext->DrawLine(line.start, line.end, d2.pBrush);

	for (auto line : m_no_change)
		d2.pD2DContext->DrawLine(line.start, line.end, d2.pBrush);

	d2.pBrush->SetColor(D2D1::ColorF(32.0f / 255, 214.0f / 255, 126.0f / 255, 1.0f));
	for (auto rect : m_up_rects)
		d2.pD2DContext->FillRectangle(rect, d2.pBrush);

	d2.pBrush->SetColor(D2D1::ColorF(1.0f, 0.2f, 0.2f, 1.0f));
	for (auto rect : m_down_rects)
		d2.pD2DContext->FillRectangle(rect, d2.pBrush);

}

std::wstring CandlestickGraph::GetYLabel(size_t i) const
{
	OHLC const * ohlc = reinterpret_cast<OHLC const *>(m_data);
	if (i < m_n) return ohlc[i].to_wstring(true);
	return std::wstring();
}


///////////////////////////////////////////////////////////
// --- BarGraph ---

void BarGraph::Make()
{
	std::pair<double, D2D1_COLOR_F> const * data = reinterpret_cast<std::pair<double, D2D1_COLOR_F> const *>(m_data);
	
	m_bars.resize(m_n);
	m_labelLayouts.resize(min(m_labels.size(), m_n));

	float x_diff = m_axes->GetDataRectXDiff();
	float boxHalfWidth = 0.4f * x_diff / m_n;
	float textHalfWidth = 0.5f * x_diff / m_n;
	float textMinWidth = 20.0f;
	if (textHalfWidth < textMinWidth) m_labelLayouts.clear();
	for (size_t i = 0; i < m_n; i++)
	{
		float x = m_axes->XtoDIP(static_cast<double>(i));
		float y1, y2; // y1 = bottom, y2 = top
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

		if (i < m_labels.size() && textHalfWidth >= textMinWidth)
		{
			m_labelLayouts[i] = D2D1::RectF(
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
	std::pair<double, D2D1_COLOR_F> const * data = reinterpret_cast<std::pair<double, D2D1_COLOR_F> const *>(m_data);
	for (size_t i = 0; i < m_n; i++)
	{
		d2.pBrush->SetColor(data[i].second);
		d2.pD2DContext->FillRectangle(m_bars[i], d2.pBrush);
	}

	d2.pBrush->SetColor(Colors::MAIN_TEXT);
	d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	for (size_t i = 0; i < m_labelLayouts.size(); i++)
	{
		d2.pD2DContext->DrawText(
			m_labels[i].c_str(),
			m_labels[i].size(),
			d2.pTextFormats[D2Objects::Formats::Segoe12],
			m_labelLayouts[i],
			d2.pBrush
		);
	}
	d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}

std::wstring BarGraph::GetYLabel(size_t i) const
{
	std::pair<double, D2D1_COLOR_F> const * data = reinterpret_cast<std::pair<double, D2D1_COLOR_F> const *>(m_data);
	if (i < m_n) return std::to_wstring(data[i].first);
	return std::wstring();
}
