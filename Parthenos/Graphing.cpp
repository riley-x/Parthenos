#include "stdafx.h"
#include "Graphing.h"



void Axes::Clear()
{
	for (Graph *item : m_graphObjects) {
		if (item) delete item;
	}
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
	if (m_rescaled) // remake everything
	{
		m_data_xdiff = static_cast<float>(m_dataRange[static_cast<int>(dataRange::xmax)]
			- m_dataRange[static_cast<int>(dataRange::xmin)]);
		m_data_ydiff = static_cast<float>(m_dataRange[static_cast<int>(dataRange::ymax)]
			- m_dataRange[static_cast<int>(dataRange::ymin)]);

		m_imade = 0;
		m_rescaled = false;
	}

	for (m_imade; m_imade < m_graphObjects.size(); m_imade++)
		m_graphObjects[m_imade]->Make();

	m_ismade = true;
}

void Axes::SetLabelSize(float ylabelWidth, float labelHeight)
{
	m_ylabelWidth = ylabelWidth;
	m_labelHeight = labelHeight;
}

// May want to call clear before this
void Axes::SetBoundingRect(float left, float top, float right, float bottom)
{
	m_ismade = false;
	m_imade = 0;
	m_rescaled = true;

	m_dipRect.left = left;
	m_dipRect.top = top;
	m_dipRect.right = right;
	m_dipRect.bottom = bottom;

	float pad = 14.0;
	m_axesRect.left = left;
	m_axesRect.top = top;
	m_axesRect.right = right - m_ylabelWidth - pad;
	m_axesRect.bottom = bottom - m_labelHeight - pad;

	Line_t xline = { D2D1::Point2F(m_axesRect.left,  m_axesRect.bottom),
					 D2D1::Point2F(m_axesRect.right, m_axesRect.bottom) };
	Line_t yline = { D2D1::Point2F(m_axesRect.right, m_axesRect.bottom),
					 D2D1::Point2F(m_axesRect.right, m_axesRect.top) };
	m_axes_lines.clear();
	m_axes_lines.push_back(xline);
	m_axes_lines.push_back(yline);

	m_dataRect.left = m_axesRect.left + m_dataPad;
	m_dataRect.top = m_axesRect.top + m_dataPad;
	m_dataRect.right = m_axesRect.right - m_dataPad;
	m_dataRect.bottom = m_axesRect.bottom - m_dataPad;

	m_rect_xdiff = m_dataRect.right - m_dataRect.left;
	m_rect_ydiff = m_dataRect.top - m_dataRect.bottom; // flip so origin is bottom-left
}

void Axes::Paint(D2Objects const & d2)
{
	if (!m_ismade) Make();

	d2.pBrush->SetColor(D2D1::ColorF(0.15f, 0.15f, 0.15f, 1.0f));
	d2.pRenderTarget->FillRectangle(m_dipRect, d2.pBrush);

	d2.pBrush->SetColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));
	for (auto line : m_axes_lines)
	{
		d2.pRenderTarget->DrawLine(line.start, line.end, d2.pBrush);
	}

	for (auto graph : m_graphObjects)
		graph->Paint(d2);
}

void Axes::Candlestick(OHLC const * ohlc, int n)
{
	// get min/max of data to scale data appropriately
	// set x values to [0, n-1)
	double low_min = ohlc[0].low;
	double high_max = -1;
	for (int i = 0; i < n; i++)
	{
		if (ohlc[i].low < low_min) low_min = ohlc[i].low;
		else if (ohlc[i].high > high_max) high_max = ohlc[i].high;
	}

	m_rescaled = setDataRange(dataRange::xmin, 0) || m_rescaled;
	m_rescaled = setDataRange(dataRange::xmax, n - 1) || m_rescaled;
	m_rescaled = setDataRange(dataRange::ymin, low_min) || m_rescaled;
	m_rescaled = setDataRange(dataRange::ymax, high_max) || m_rescaled;

	auto graph = new CandlestickGraph(this, ohlc, n);
	m_graphObjects.push_back(graph);
	m_ismade = false;
}

void Axes::Line(double const * data, int n, D2D1_COLOR_F color, float stroke_width, ID2D1StrokeStyle * pStyle)
{
	// get min/max of data to scale data appropriately
	// sets x values to [0, n-1)
	double min = data[0];
	double max = data[0];
	for (int i = 0; i < n; i++)
	{
		if (data[i] < min) min = data[i];
		else if (data[i] > max) max = data[i];
	}

	m_rescaled = setDataRange(dataRange::xmin, 0) || m_rescaled;
	m_rescaled = setDataRange(dataRange::xmax, n - 1) || m_rescaled;
	m_rescaled = setDataRange(dataRange::ymin, min) || m_rescaled;
	m_rescaled = setDataRange(dataRange::ymax, max) || m_rescaled;

	auto graph = new LineGraph(this, data, n);
	graph->SetLineProperties(color, stroke_width, pStyle);
	m_graphObjects.push_back(graph);
	m_ismade = false;
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

	if (isnan(m_boxHalfWidth))
	{
		float x_diff = m_axes->GetDataRectXDiff();
		m_boxHalfWidth = min(15.0f, 0.4f * x_diff / m_n);
	}

	for (int i = 0; i < m_n; i++)
	{
		float x = m_axes->XtoDIP(static_cast<double>(i));
		float y1 = m_axes->YtoDIP(ohlc[i].low);
		float y2 = m_axes->YtoDIP(ohlc[i].high);
		D2D1_POINT_2F start = D2D1::Point2F(x, y1);
		D2D1_POINT_2F end = D2D1::Point2F(x, y2);
		m_lines[i] = { start, end };

		D2D1_RECT_F temp = D2D1::RectF(
			x - m_boxHalfWidth,
			0,
			x + m_boxHalfWidth,
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
			D2D1_POINT_2F start = D2D1::Point2F(x - m_boxHalfWidth, y2);
			D2D1_POINT_2F end = D2D1::Point2F(x + m_boxHalfWidth, y2);
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

