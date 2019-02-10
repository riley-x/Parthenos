#pragma once

#include "stdafx.h"
#include "D2Objects.h"
#include "DataManaging.h"

class Axes;

// Main plotting window for stock price charts.
// Includes drawing command bar.
class Chart
{
public:
	void Init(float leftOffset);
	void Paint(D2Objects const & d2);
	void Resize(RECT pRect);
private:
	float				m_leftOffset; // Chart is flush right, with fixed-width offset in DIPs on left
	D2D1_RECT_F			m_dipRect; // DIPs in main window client coordinates
	std::vector<Axes>	m_axes;
};

typedef struct LINE_STRUCT {
	D2D1_POINT_2F start;
	D2D1_POINT_2F end;
} Line_t;

class Axes
{
public:
	Axes(float ylabelWidth, float xlabelHeight);
	void SetBoundingRect(float left, float top, float right, float bottom);
	void Paint(D2Objects const & d2);
	void Candlestick(std::vector<OHLC> const & ohlc);
	void Line(std::vector<OHLC> const & ohlc);
	void Envelope(std::vector<OHLC> const & ohlc);

private:
	float m_ylabelWidth; // width of y tick labels in DIPs.
	float m_labelHeight; // height of tick labels in DIPs.
	std::vector<float> m_xTicks; // locations of x ticks
	std::vector<float> m_yTicks; // locations of y ticks

	D2D1_RECT_F m_dipRect; // bounding rect DIPs in main window client coordinates
	D2D1_RECT_F m_axesRect; // rect for the actual axes

	std::vector<Line_t> m_lines; // lines to draw
	std::vector<D2D1_RECT_F> m_rects; // rects to draw
};