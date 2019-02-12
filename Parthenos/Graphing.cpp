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
	d2.pBrush->SetColor(D2D1::ColorF(0.8f, 0.0f, 0.5f, 1.0f));
	for (auto line : m_misc_lines)
	{
		d2.pRenderTarget->DrawLine(line.start, line.end, d2.pBrush, 1.0f);
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


void Axes::Line(OHLC const * ohlc, int n)
{
	// first pass get min/max of data to scale data appropriately
	double close_min = ohlc[0].close;
	double close_max = -1;

	for (int i = 0; i < n; i++)
	{
		if (ohlc[i].close < close_min) close_min = ohlc[i].close;
		else if (ohlc[i].close > close_max) close_max = ohlc[i].close;
	}

	// second pass calculate DIP coordiantes
	std::vector<float> x(n);
	std::vector<float> y(n);

	float x_diff = m_dataRect.right - m_dataRect.left;
	float y_diff = m_dataRect.top - m_dataRect.bottom; // flip so origin is bottom-left
	double close_diff = close_max - close_min;

	for (int i = 0; i < n; i++)
	{
		x[i] = linScale(i, 0, n - 1, m_dataRect.left, x_diff);
		y[i] = linScale(ohlc[i].close, close_min, close_diff, m_dataRect.bottom, y_diff);
	}

	RawLine(x, y);
}


// Pushes to m_misc_lines a series of line segments made by connecting
// the points (x[i], y[i])
void Axes::RawLine(std::vector<float> x, std::vector<float> y)
{
	m_misc_lines.reserve(m_misc_lines.size() + x.size() - 1);

	D2D1_POINT_2F start;
	D2D1_POINT_2F end = D2D1::Point2F(x[0], y[0]);

	for (size_t i = 0; i < x.size() - 1; i++)
	{
		start = end;
		end = D2D1::Point2F(x[i + 1], y[i + 1]);
		m_misc_lines.push_back({ start, end });
	}
}

void CandlestickGraph::Make(void const * data, int n)
{
	OHLC const * ohlc = reinterpret_cast<OHLC const *>(data);

	// second pass calculate DIP coordiantes
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
	double x_data_diff = m_dataRange[static_cast<int>(dataRange::xmax)] - data_xmin;
	double data_ymin = m_dataRange[static_cast<int>(dataRange::ymin)];
	double y_data_diff = m_dataRange[static_cast<int>(dataRange::ymax)] - data_ymin;

	for (int i = 0; i < n; i++)
	{
		float x = linScale(static_cast<double>(i), data_xmin, x_data_diff, m_dataRect.left, x_diff);
		float y1 = linScale(ohlc[i].low, data_ymin, y_data_diff, m_dataRect.bottom, y_diff);
		float y2 = linScale(ohlc[i].high, data_ymin, y_data_diff, m_dataRect.bottom, y_diff);
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
			temp.top = linScale(ohlc[i].close, data_ymin, y_data_diff, m_dataRect.bottom, y_diff);
			temp.bottom = linScale(ohlc[i].open, data_ymin, y_data_diff, m_dataRect.bottom, y_diff);
			m_up_rects.push_back(temp);
		}
		else if (ohlc[i].close < ohlc[i].open)
		{
			temp.top = linScale(ohlc[i].open, data_ymin, y_data_diff, m_dataRect.bottom, y_diff);
			temp.bottom = linScale(ohlc[i].close, data_ymin, y_data_diff, m_dataRect.bottom, y_diff);
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
