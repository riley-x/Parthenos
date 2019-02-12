#include "stdafx.h"
#include "Graphing.h"

template <typename T>
inline float linScale(T val, T val_min, T val_diff, float new_min, float new_diff)
{
	return new_min + (static_cast<float>(val - val_min) / static_cast<float>(val_diff)) * new_diff;
}


void Axes::Clear()
{
	for (Graph *item : m_graphObjects) {
		if (item) delete item;
	}
	m_graphObjects.clear();
}

void Axes::SetLabelSize(float ylabelWidth, float labelHeight)
{
	m_ylabelWidth = ylabelWidth;
	m_labelHeight = labelHeight;
}

void Axes::SetBoundingRect(float left, float top, float right, float bottom)
{
	Clear();

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
}

void Axes::Paint(D2Objects const & d2)
{
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

	setDataRange(dataRange::xmin, 0);
	setDataRange(dataRange::xmax, n - 1);
	setDataRange(dataRange::ymin, low_min);
	setDataRange(dataRange::ymax, high_max);


	auto graph = new CandlestickGraph(m_dataRect, m_dataRange);
	graph->Make(ohlc, n);
	m_graphObjects.push_back(graph);
}


void Axes::Line(double const * data, int n)
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

	setDataRange(dataRange::xmin, 0);
	setDataRange(dataRange::xmax, n - 1);
	setDataRange(dataRange::ymin, min);
	setDataRange(dataRange::ymax, max);

	auto graph = new LineGraph(m_dataRect, m_dataRange);
	graph->Make(data, n);
	m_graphObjects.push_back(graph);
}


// Caculates the DIP coordinates for in_data (setting x values to [0,n) )
// and adds line segments connecting the points
void LineGraph::Make(void const * in_data, int n)
{
	float x_diff = m_dataRect.right - m_dataRect.left;
	float y_diff = m_dataRect.top - m_dataRect.bottom; // flip so origin is bottom-left
	
	double data_xmin = m_dataRange[static_cast<int>(dataRange::xmin)];
	double data_xdiff = m_dataRange[static_cast<int>(dataRange::xmax)] - data_xmin;
	double data_ymin = m_dataRange[static_cast<int>(dataRange::ymin)];
	double data_ydiff = m_dataRange[static_cast<int>(dataRange::ymax)] - data_ymin;

	double const * data = reinterpret_cast<double const *>(in_data);
	m_lines.reserve(n - 1);

	float x = linScale(static_cast<double>(0), data_xmin, data_xdiff, m_dataRect.left, x_diff);
	float y = linScale(data[0], data_ymin, data_ydiff, m_dataRect.bottom, y_diff);
	D2D1_POINT_2F start;
	D2D1_POINT_2F end = D2D1::Point2F(x, y);
	for (int i = 1; i < n; i++)
	{
		start = end;
		x = linScale(static_cast<double>(i), data_xmin, data_xdiff, m_dataRect.left, x_diff);
		y = linScale(data[i], data_ymin, data_ydiff, m_dataRect.bottom, y_diff);
		end = D2D1::Point2F(x, y);
		m_lines.push_back({ start, end });
	}
}

void LineGraph::Paint(D2Objects const & d2)
{
	d2.pBrush->SetColor(D2D1::ColorF(0.8f, 0.0f, 0.5f, 1.0f));
	for (auto line : m_lines)
	{
		d2.pRenderTarget->DrawLine(line.start, line.end, d2.pBrush, 1.0f);
	}
}


void CandlestickGraph::Make(void const * data, int n)
{
	OHLC const * ohlc = reinterpret_cast<OHLC const *>(data);

	// calculate DIP coordiantes
	m_lines.reserve(n);
	m_up_rects.reserve(n);
	m_down_rects.reserve(n);

	float x_diff = m_dataRect.right - m_dataRect.left;
	float y_diff = m_dataRect.top - m_dataRect.bottom; // flip so origin is bottom-left
	if (isnan(m_boxHalfWidth))
	{
		m_boxHalfWidth = min(15.0f, 0.4f * x_diff / n);
	}

	double data_xmin = m_dataRange[static_cast<int>(dataRange::xmin)];
	double data_xdiff = m_dataRange[static_cast<int>(dataRange::xmax)] - data_xmin;
	double data_ymin = m_dataRange[static_cast<int>(dataRange::ymin)];
	double data_ydiff = m_dataRange[static_cast<int>(dataRange::ymax)] - data_ymin;

	for (int i = 0; i < n; i++)
	{
		float x = linScale(static_cast<double>(i), data_xmin, data_xdiff, m_dataRect.left, x_diff);
		float y1 = linScale(ohlc[i].low, data_ymin, data_ydiff, m_dataRect.bottom, y_diff);
		float y2 = linScale(ohlc[i].high, data_ymin, data_ydiff, m_dataRect.bottom, y_diff);
		D2D1_POINT_2F start = D2D1::Point2F(x, y1);
		D2D1_POINT_2F end = D2D1::Point2F(x, y2);
		m_lines.push_back({ start, end });

		D2D1_RECT_F temp = D2D1::RectF(
			x - m_boxHalfWidth,
			0,
			x + m_boxHalfWidth,
			0
		);
		if (ohlc[i].close > ohlc[i].open)
		{
			temp.top = linScale(ohlc[i].close, data_ymin, data_ydiff, m_dataRect.bottom, y_diff);
			temp.bottom = linScale(ohlc[i].open, data_ymin, data_ydiff, m_dataRect.bottom, y_diff);
			m_up_rects.push_back(temp);
		}
		else if (ohlc[i].close < ohlc[i].open)
		{
			temp.top = linScale(ohlc[i].open, data_ymin, data_ydiff, m_dataRect.bottom, y_diff);
			temp.bottom = linScale(ohlc[i].close, data_ymin, data_ydiff, m_dataRect.bottom, y_diff);
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

	d2.pBrush->SetColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f));
	for (auto line : m_no_change)
		d2.pRenderTarget->DrawLine(line.start, line.end, d2.pBrush);

	d2.pBrush->SetColor(D2D1::ColorF(32.0f / 255, 214.0f / 255, 126.0f / 255, 1.0f));
	for (auto rect : m_up_rects)
		d2.pRenderTarget->FillRectangle(rect, d2.pBrush);

	d2.pBrush->SetColor(D2D1::ColorF(1.0f, 0.2f, 0.2f, 1.0f));
	for (auto rect : m_down_rects)
		d2.pRenderTarget->FillRectangle(rect, d2.pBrush);

}

