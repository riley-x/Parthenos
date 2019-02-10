#include "stdafx.h"
#include "Chart.h"
#include "utilities.h"
#include "TitleBar.h"


void Chart::Init(HWND hwndParent, float leftOffset)
{
	m_hwndParent   = hwndParent;
	m_leftOffset   = leftOffset;
	m_dipRect.left = leftOffset;
	m_dipRect.top  = TitleBar::height;
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
}

// TODO: Add button highlight, etc. here?
void Chart::Candlestick(std::vector<OHLC> const & ohlc) 
{ 
	m_axes.Candlestick(ohlc); 
}
void Chart::Line(OHLC const * ohlc, int n) 
{ 
	m_axes.Line(ohlc, n); 
	InvalidateRect(m_hwndParent, NULL, FALSE);
}
void Chart::Envelope(std::vector<OHLC> const & ohlc) 
{ 
	m_axes.Envelope(ohlc); 
}


void Axes::SetLabelSize(float ylabelWidth, float labelHeight)
{
	m_ylabelWidth = ylabelWidth;
	m_labelHeight = labelHeight;
}

void Axes::SetBoundingRect(float left, float top, float right, float bottom)
{
	m_dipRect.left   = left;
	m_dipRect.top    = top;
	m_dipRect.right  = right;
	m_dipRect.bottom = bottom;

	float pad = 14.0;
	m_axesRect.left   = left;
	m_axesRect.top	  = top;
	m_axesRect.right  = right - m_ylabelWidth - pad;
	m_axesRect.bottom = bottom - m_labelHeight - pad;

	Line_t xline = { D2D1::Point2F(m_axesRect.left,  m_axesRect.bottom), 
					 D2D1::Point2F(m_axesRect.right, m_axesRect.bottom) };
	Line_t yline = { D2D1::Point2F(m_axesRect.right, m_axesRect.bottom), 
					 D2D1::Point2F(m_axesRect.right, m_axesRect.top) };

	m_axes_lines.clear();
	m_axes_lines.push_back(xline);
	m_axes_lines.push_back(yline);
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
		d2.pRenderTarget->DrawLine(line.start, line.end, d2.pBrush, 0.5f);
	}
}

void Axes::Candlestick(std::vector<OHLC> const & ohlc)
{
}

template <typename T>
inline float linScale(T val, T val_min, T val_diff, float new_min, float new_diff)
{
	return new_min + (static_cast<float>(val - val_min) / static_cast<float>(val_diff)) * new_diff;
}

void Axes::Line(OHLC const * ohlc, int n)
{
	double close_min = ohlc[0].close;
	double close_max = -1;

	float m_dataPad = 5.0f; // padding between datapoints and border

	float x_min = m_axesRect.left + m_dataPad;
	float x_diff = m_axesRect.right - m_dataPad - x_min;
	float y_min = m_axesRect.bottom - m_dataPad;
	float y_diff = m_axesRect.top + m_dataPad - y_min;

	std::vector<float> x(n);
	std::vector<float> y(n);

	for (int i = 0; i < n; i++)
	{
		OutputDebugString(ohlc[i].to_wstring().c_str());
		x[i] = linScale(i, 0, n, x_min, x_diff);
		if (ohlc[i].close < close_min) close_min = ohlc[i].close;
		else if (ohlc[i].close > close_max) close_max = ohlc[i].close;
	}

	double close_diff = close_max - close_min;
	for (int i = 0; i < n; i++)
	{
		y[i] = linScale(ohlc[i].close, close_min, close_diff, y_min, y_diff);
	}

	RawLine(x, y);
}

void Axes::Envelope(std::vector<OHLC> const & ohlc)
{
}

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

