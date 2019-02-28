#pragma once

#include "stdafx.h"
#include "AppItem.h"
#include "DataManaging.h"


typedef struct LINE_STRUCT {
	D2D1_POINT_2F start;
	D2D1_POINT_2F end;
} Line_t;


//template <typename T>
//inline float linScale(T val, T val_min, T val_diff, float new_min, float new_diff)
//{
//	return new_min + (static_cast<float>(val - val_min) / static_cast<float>(val_diff)) * new_diff;
//}

class Axes;

class Graph
{
protected:
	Axes *m_axes;
	void const *m_data; // NOT owned by Axes nor Graph. Recast for each derived class
	int m_n; // length of data array
public:
	Graph(Axes *axes, void const *data, int n) : m_axes(axes), m_data(data), m_n(n) {};
	virtual void Make() = 0; // Calculate the D2 DIP objects to paint from given data
	virtual void Paint(D2Objects const & d2) = 0;
};

// m_data = double* to y values (assumes linear spacing)
class LineGraph : public Graph
{
public:
	using Graph::Graph;
	void Make();
	void Paint(D2Objects const & d2);
	void SetLineProperties(D2D1_COLOR_F color, float stroke_width, ID2D1StrokeStyle * pStyle);
private:
	std::vector<Line_t> m_lines; // line segments
	D2D1_COLOR_F m_color = D2D1::ColorF(0.8f, 0.0f, 0.5f, 1.0f);
	float m_stroke_width = 1.0f;
	ID2D1StrokeStyle * m_pStyle = NULL;
};

// m_data = OHLC*
class CandlestickGraph : public Graph
{
public:
	using Graph::Graph;
	void Make();
	void Paint(D2Objects const & d2);
private:
	std::vector<Line_t> m_lines; // low-high lines
	std::vector<Line_t> m_no_change; // when open==close
	std::vector<D2D1_RECT_F> m_up_rects; // green boxes
	std::vector<D2D1_RECT_F> m_down_rects; // red boxes
};

// m_data = std::pair<double, D2D1_COLOR_F>* to (y value, color) pairs
class BarGraph : public Graph
{
public:
	using Graph::Graph;
	void Make();
	void Paint(D2Objects const & d2);
private:
	std::vector<D2D1_RECT_F> m_bars;
};

class Axes : public AppItem
{
public:
	using AppItem::AppItem;
	~Axes() { Clear(); }
	void Clear();
	void Make();
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);

	// Data pointers to these functions should remain valid until the next Clear() call
	void Candlestick(OHLC const * ohlc, int n);
	void Line(date_t const * dates, double const * ydata, int n, 
		D2D1_COLOR_F color = D2D1::ColorF(0.8f, 0.0f, 0.5f, 1.0f), 
		float stroke_width = 1.0f,
		ID2D1StrokeStyle * pStyle = NULL);
	void Bar(std::pair<double, D2D1_COLOR_F> const * data, int n);

	inline float XtoDIP(double val) const
	{
		double data_xmin = m_dataRange[static_cast<int>(dataRange::xmin)];
		return m_dataRect.left + static_cast<float>((val - data_xmin) / m_data_xdiff) * m_rect_xdiff;
	}
	inline float YtoDIP(double val) const
	{
		double data_ymin = m_dataRange[static_cast<int>(dataRange::ymin)];
		return m_dataRect.bottom + static_cast<float>((val - data_ymin) / m_data_ydiff) * m_rect_ydiff;
	}
	inline double DIPtoY(float val) const
	{
		double data_ymin = m_dataRange[static_cast<int>(dataRange::ymin)];
		return data_ymin + ((val - m_dataRect.bottom) / m_rect_ydiff) * m_data_ydiff;
	}

	inline void SetXAxisPos(float y) { m_xAxisPos = y; }
	inline void SetLabelSize(float ylabelWidth, float labelHeight) {
		m_ylabelWidth = ylabelWidth;
		m_labelHeight = labelHeight;
	}
	inline void SetXLabels() { m_drawxLabels = false; } // no labels == no draw
	inline float GetDataRectXDiff() const { return m_rect_xdiff; }
	inline D2D1_RECT_F GetAxesRect() const { return m_axesRect; }

private:
	// Parameters
	float m_ylabelWidth = 40.0f; // width of y tick labels in DIPs.
	float m_labelHeight = 16.0f; // height of tick labels in DIPs.
	float m_dataPad		= 20.0f; // padding between datapoints and border
	float m_labelPad	= 2.0f;
	float m_xAxisPos	= std::nanf(""); // y position. defaults to draw at m_axesRect.bottom

	// Flags and state variables
	bool m_ismade		= true;  // check to make sure everything is made
	bool m_rescaled		= false; // data ranges changed, so need to call Rescale()
	bool m_drawxLabels	= true;  // set before SetSize()
	size_t m_imade		= 0;	 // stores an index into m_graphObjects. Objects < i are already made
	double m_dataRange[4] = { nan(""), nan(""), nan(""), nan("") }; // Numerical range of data: x_min, x_max, y_min, y_max
	double m_data_xdiff;
	double m_data_ydiff;

	// Rects and location
	std::vector<std::tuple<float, date_t, std::wstring>> m_xTicks; // DIP, date, label
	std::vector<std::tuple<float, double, std::wstring>> m_yTicks; // DIP, value, label
	std::vector<date_t> m_dates; // actual date values (plotted as x = [0, n-1))
	D2D1_RECT_F m_axesRect; // rect for the actual axes
	D2D1_RECT_F m_dataRect; // rect for drawable space for data points
	float m_rect_xdiff;		// m_dataRect.right - m_dataRect.left
	float m_rect_ydiff;		// m_dataRect.top - m_dataRect.bottom, flip so origin is bottom-left

	// Graphing objects
	std::vector<Line_t> m_grid_lines[2]; // x, y
	std::vector<Graph*> m_graphObjects; // THESE NEED TO BE REMADE WHEN RENDER TARGET CHANGES

	// Functions
	enum class dataRange { xmin, xmax, ymin, ymax };
	inline bool setDataRange(dataRange i, double val)
	{
		int ind = static_cast<int>(i);
		if ((i == dataRange::xmin || i == dataRange::ymin) && (isnan(m_dataRange[ind]) || val < m_dataRange[ind]))
		{
			m_dataRange[ind] = val;
			return true;
		}
		else if ((i == dataRange::xmax || i == dataRange::ymax) && (isnan(m_dataRange[ind]) || val > m_dataRange[ind]))
		{
			m_dataRange[ind] = val;
			return true;
		}
		return false;
	}
	inline bool setDataRange(double xmin, double xmax, double ymin, double ymax)
	{
		bool out = false;
		double vals[4] = { xmin, xmax, ymin, ymax };
		for (int i = 0; i < 4; i++)
			out = setDataRange(static_cast<dataRange>(i), vals[i]) || out;
		return out;
	}

	void Rescale();
	void CalculateXTicks();
	void CalculateYTicks();

};

