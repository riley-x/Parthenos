#pragma once

#include "stdafx.h"
#include "D2Objects.h"
#include "DataManaging.h"


typedef struct LINE_STRUCT {
	D2D1_POINT_2F start;
	D2D1_POINT_2F end;
} Line_t;

enum class dataRange { xmin, xmax, ymin, ymax };


class Graph
{
protected:
	D2D1_RECT_F m_dataRect;
	const double * const m_dataRange;
public:
	Graph(D2D1_RECT_F const & dataRect, const double * dataRange)
		: m_dataRect(dataRect), m_dataRange(dataRange) {};
	virtual void Make(void const * data, int n) = 0;
	virtual void Paint(D2Objects const & d2) = 0;
};

class LineGraph : public Graph
{
public:
	using Graph::Graph;
	void Make(void const * data, int n);
	void Paint(D2Objects const & d2);
	void SetLineProperties(D2D1_COLOR_F color, float stroke_width, ID2D1StrokeStyle * pStyle);
private:
	std::vector<Line_t> m_lines; // line segments
	D2D1_COLOR_F m_color = D2D1::ColorF(0.8f, 0.0f, 0.5f, 1.0f);
	float m_stroke_width = 1.0f;
	ID2D1StrokeStyle * m_pStyle = NULL;
};


class CandlestickGraph : public Graph
{
public:
	using Graph::Graph;
	void Make(void const * data, int n);
	void Paint(D2Objects const & d2);

	float m_boxHalfWidth = nanf("");
private:
	std::vector<Line_t> m_lines; // low-high lines
	std::vector<Line_t> m_no_change; // when open==close
	std::vector<D2D1_RECT_F> m_up_rects; // green boxes
	std::vector<D2D1_RECT_F> m_down_rects; // red boxes
};

class Axes
{
public:
	~Axes() { Clear(); }
	void Clear();

	void SetLabelSize(float ylabelWidth, float labelHeight);
	void SetBoundingRect(float left, float top, float right, float bottom);
	void Paint(D2Objects const & d2);
	void Candlestick(OHLC const * ohlc, int n);
	void Line(double const * data, int n, 
		D2D1_COLOR_F color = D2D1::ColorF(0.8f, 0.0f, 0.5f, 1.0f), 
		float stroke_width = 1.0f,
		ID2D1StrokeStyle * pStyle = NULL);
	//void Envelope(double const * ohlc, int n);

private:

	float m_ylabelWidth = 40.0f; // width of y tick labels in DIPs.
	float m_labelHeight = 14.0f; // height of tick labels in DIPs.
	float m_dataPad = 20.0f; // padding between datapoints and border

	std::vector<float> m_xTicks; // locations of x ticks
	std::vector<float> m_yTicks; // locations of y ticks

	D2D1_RECT_F m_dipRect;  // bounding rect DIPs in main window client coordinates
	D2D1_RECT_F m_axesRect; // rect for the actual axes
	D2D1_RECT_F m_dataRect; // rect for drawable space for data points

	std::vector<Line_t> m_axes_lines;
	std::vector<Line_t> m_grid_lines;

	std::vector<Graph*> m_graphObjects; // THESE NEED TO BE REMADE WHEN RENDER TARGET CHANGES


	// Numerical values of data ranges
	// x_min, x_max, y_min, y_max
	double m_dataRange[4] = { nan(""), nan(""), nan(""), nan("") };
	inline void setDataRange(dataRange i, double val)
	{
		int ind = static_cast<int>(i);
		if ((i == dataRange::xmin || i == dataRange::ymin) && (isnan(m_dataRange[ind]) || val < m_dataRange[ind]))
			m_dataRange[ind] = val;
		else if ((i == dataRange::xmax || i == dataRange::ymax) && (isnan(m_dataRange[ind]) || val > m_dataRange[ind]))
			m_dataRange[ind] = val;
	}

	void RawLine(std::vector<float> x, std::vector<float> y); // (x,y) points in DIPs
};

