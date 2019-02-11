#pragma once

#include "stdafx.h"
#include "D2Objects.h"
#include "DataManaging.h"


typedef struct LINE_STRUCT {
	D2D1_POINT_2F start;
	D2D1_POINT_2F end;
} Line_t;


class Graph
{
protected:
	D2D1_RECT_F m_dataRect;
public:
	Graph(D2D1_RECT_F const & dataRect) : m_dataRect(dataRect) {};
	virtual void Make(void const * data, int n) = 0;
	virtual void Paint(D2Objects const & d2) = 0;
};


class CandlestickGraph : public Graph
{
public:
	CandlestickGraph(D2D1_RECT_F const & dataRect) : Graph(dataRect) {};
	void Make(void const * data, int n);
	void Paint(D2Objects const & d2);
private:
	std::vector<Line_t> m_lines; // other lines to draw
	std::vector<D2D1_RECT_F> m_up_rects; // green boxes
	std::vector<D2D1_RECT_F> m_down_rects; // red boxes
};

class Axes
{
public:
	~Axes()
	{
		for (Graph *item : m_graphObjects) {
			if (item) delete item;
		}
	}

	void SetLabelSize(float ylabelWidth, float labelHeight);
	void SetBoundingRect(float left, float top, float right, float bottom);
	void Paint(D2Objects const & d2);
	void Candlestick(OHLC const * ohlc, int n);
	void Line(OHLC const * ohlc, int n);
	void Envelope(std::vector<OHLC> const & ohlc);

private:


	float m_ylabelWidth = 40.0f; // width of y tick labels in DIPs.
	float m_labelHeight = 14.0f; // height of tick labels in DIPs.
	float m_dataPad		= 20.0f; // padding between datapoints and border

	std::vector<float> m_xTicks; // locations of x ticks
	std::vector<float> m_yTicks; // locations of y ticks

	D2D1_RECT_F m_dipRect;  // bounding rect DIPs in main window client coordinates
	D2D1_RECT_F m_axesRect; // rect for the actual axes
	D2D1_RECT_F m_dataRect; // rect for drawable space for data points

	std::vector<Line_t> m_axes_lines;
	std::vector<Line_t> m_grid_lines;
	std::vector<Line_t> m_misc_lines; // other lines to draw
	
	std::vector<Graph*> m_graphObjects;


	void RawLine(std::vector<float> x, std::vector<float> y); // (x,y) points in DIPs
};



// Main plotting window for stock price charts.
// Includes drawing command bar.
class Chart
{
public:
	void Init(HWND hwndParent, float leftOffset);
	void Paint(D2Objects const & d2);
	void Resize(RECT pRect);

	void Candlestick(OHLC const * ohlc, int n);
	void Line(OHLC const * ohlc, int n);
	void Envelope(std::vector<OHLC> const & ohlc);

private:
	HWND				m_hwndParent;
	float				m_leftOffset; // Chart is flush right, with fixed-width offset in DIPs on left
	D2D1_RECT_F			m_dipRect; // DIPs in main window client coordinates
	Axes				m_axes;
};


