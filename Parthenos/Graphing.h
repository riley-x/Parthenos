#pragma once

#include "stdafx.h"
#include "AppItem.h"
#include "DataManaging.h"

class Axes;

///////////////////////////////////////////////////////////
// --- Structs ---

typedef struct LINE_STRUCT {
	D2D1_POINT_2F start;
	D2D1_POINT_2F end;
} Line_t;

typedef struct LINE_PROPERTIES {
	D2D1_COLOR_F color = Colors::ALMOST_WHITE;
	float stroke_width = 1.0f;
	ID2D1StrokeStyle * pStyle = NULL;
} LineProps;

enum class MarkerStyle {circle, square, up, down, x};

typedef struct POINTS_PROPERTIES {
	MarkerStyle style = MarkerStyle::circle;
	float scale = 5.0f; // approximate radius
	double x;
	double y;
	D2D1_COLOR_F color = Colors::ACCENT;

	struct POINTS_PROPERTIES(MarkerStyle _style, double _x, double _y, float _scale = 5.0f, D2D1_COLOR_F _color = Colors::ACCENT)
		: style(_style), x(_x), y(_y), scale(_scale), color(_color) {}
} PointProps;

///////////////////////////////////////////////////////////
// --- Graph class ---

class Graph
{
protected:
	Axes *m_axes;
	size_t m_offset; // x values are set to [m_offset, m_offset + m_n)

public:
	Graph(Axes *axes, size_t offset = 0) : m_axes(axes), m_offset(offset) {};
	std::wstring m_name;

	virtual void Make() = 0; // Calculate the D2 DIP objects to paint from given data
	virtual void Paint(D2Objects const & d2) = 0;
	virtual std::wstring GetYLabel(size_t i) const { return L""; } // i is x value, not index into data
};

class LineGraph : public Graph
{
public:
	using Graph::Graph;
	void Make();
	void Paint(D2Objects const & d2);
	std::wstring GetYLabel(size_t i) const;

	inline void SetData(std::vector<double> const & data) { m_y = data; }
	inline void SetLineProperties(LineProps const & props) { m_props = props; }

private:
	std::vector<double> m_y;
	std::vector<Line_t> m_lines; // line segments
	LineProps m_props;
};

class PointsGraph : public Graph
{
public:
	using Graph::Graph;
	void Make();
	void Paint(D2Objects const & d2);

	inline void SetData(std::vector<PointProps> const & data) { m_points = data; }

private:
	std::vector<PointProps> m_points;
	std::vector<D2D1_POINT_2F> m_locs;
};

class CandlestickGraph : public Graph
{
public:
	using Graph::Graph;
	void Make();
	void Paint(D2Objects const & d2);
	std::wstring GetYLabel(size_t i) const;

	inline void SetData(std::vector<OHLC> const & ohlc) { m_ohlc = ohlc; }

private:
	std::vector<OHLC> m_ohlc;
	std::vector<Line_t> m_lines; // low-high lines
	std::vector<Line_t> m_no_change; // when open==close
	std::vector<D2D1_RECT_F> m_up_rects; // green boxes
	std::vector<D2D1_RECT_F> m_down_rects; // red boxes
};

class BarGraph : public Graph
{
public:
	using Graph::Graph;
	void Make();
	void Paint(D2Objects const & d2);
	std::wstring GetYLabel(size_t i) const;

	inline void SetLabels(std::vector<std::wstring> const & labels) { m_labels = labels; }
	inline void SetData(std::vector<std::pair<double, D2D1_COLOR_F>> const & data) { m_data = data; }

private:
	std::vector<std::pair<double, D2D1_COLOR_F>> m_data; // (y value, color) pairs
	std::vector<D2D1_RECT_F>	m_bars;
	std::vector<std::wstring>	m_labels;
	std::vector<D2D1_RECT_F>	m_labelRects;
};

///////////////////////////////////////////////////////////
// --- Axes class ---

class Axes : public AppItem
{
public:
	// Statics
	enum class HoverStyle { none, snap, crosshairs };

	// [0] primary graphs. These graphs affect automatic axes scaling, and should have consistent
	//	   x values. The first graph is used for mouse hover text. Cached painting.
	// [1] secondary graphs. These have x positions dependent on [0], but will scale the y axis. Cached painting.
	// [2] tertiary graphs. These do not scale the axes, and are not cached for painting.
	// [1-2] should not exist when [0] is empty.
	enum GraphGroup : size_t { GG_PRI, GG_SEC, GG_TER, nGraphGroups};

	// Constructor
	Axes(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent);
	~Axes();
	
	// AppItem
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);

	// Interface
	void Clear();
	void Clear(GraphGroup group);
	void Remove(GraphGroup group, std::wstring name);

	// Data pointers to these functions should remain valid until the next Clear() call
	void Candlestick(std::vector<OHLC> const & ohlc, GraphGroup group = GG_PRI);
	void Line(std::vector<date_t> const & dates, std::vector<double> const & ydata,
		LineProps props = {}, GraphGroup group = GG_PRI
	);
	void Bar(std::vector<std::pair<double, D2D1_COLOR_F>> const & data,
		std::vector<std::wstring> const & labels = {},
		GraphGroup group = GG_PRI, size_t offset = 0 // offset of first point from x_min of primary graph(s)
	);
	void DatePoints(std::vector<PointProps> points, GraphGroup group, std::wstring name = L"");

	// Convert between an x/y value and the dip coordinate (relative to window)
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
	inline double DIPtoX(float val) const
	{
		double data_xmin = m_dataRange[static_cast<int>(dataRange::xmin)];
		return data_xmin + ((val - m_dataRect.left) / m_rect_xdiff) * m_data_xdiff;
	}
	inline double DIPtoY(float val) const
	{
		double data_ymin = m_dataRange[static_cast<int>(dataRange::ymin)];
		return data_ymin + ((val - m_dataRect.bottom) / m_rect_ydiff) * m_data_ydiff;
	}

	inline void SetHoverStyle(HoverStyle sty) { m_hoverStyle = sty; }
	inline void SetLabelSize(float ylabelWidth, float labelHeight) 
	{
		m_ylabelWidth = ylabelWidth;
		m_labelHeight = labelHeight;
	}
	inline void SetXAxisPos(double y) { m_xAxisPos = y; }
	inline void SetYAxisPos(double x) { m_yAxisPos = x; }
	inline void SetXLabels() { m_drawxLabels = false; } // no labels == no draw
	inline void SetXLabels(std::vector<std::wstring> const & labels, bool draw = true) 
	{
		m_userXLabels = labels; m_drawxLabels = draw;
	}

	void SetTitle(std::wstring const & title, D2Objects::Formats format = D2Objects::Formats::Segoe18);
	
	inline float GetDataRectXDiff() const { return m_rect_xdiff; }
	inline D2D1_RECT_F GetAxesRect() const { return m_axesRect; }
	inline float GetDataPad() const { return m_dataPad; }
	inline size_t GetNPoints() const { return m_nPoints; }
	ID2D1PathGeometry* GetMarker(MarkerStyle m);

private:

	// Objects
	// individual graphs to plot. these need to be remade when the size of the window changes.
	std::vector<Graph*>		m_graphObjects[nGraphGroups]; 
	ComPtr<ID2D1Bitmap1>	m_primaryCache = nullptr; // caches graph groups 0 and 1, grids, labels, etc.
	ComPtr<ID2D1PathGeometry> m_upMarker = nullptr;
	ComPtr<ID2D1PathGeometry> m_dnMarker = nullptr;
	ComPtr<ID2D1PathGeometry> m_xxMarker = nullptr;

	// Parameters
	float m_ylabelWidth = 40.0f; // width of y tick labels in DIPs.
	float m_labelHeight = 16.0f; // height of tick labels in DIPs.
	float m_dataPad		= 20.0f; // padding between datapoints and border
	float m_labelPad	= 2.0f;  // padding between axes labels and axes; also for hover text and bounding box
	float m_titlePad	= 0.0f;  // offset between dipRect.top and axesRect.top
	double m_xAxisPos	= -std::numeric_limits<double>::infinity(); // y position. -inf to draw at m_axesRect.bottom
	double m_yAxisPos	= std::numeric_limits<double>::infinity();  // x position. +inf to draw at m_axesRect.right

	// Flags and state variables
	bool	m_ismade				= true;  // check to make sure everything is made
	size_t	m_imade[nGraphGroups]	= {};	 // index into m_graphObjects. Objects < i are already made
	bool	m_rescaled				= false; // data ranges changed, so need to call Rescale()
	bool	m_drawxLabels			= true;  // set before SetSize()
	int		m_hoverOn				= -1;	 // current mouse position in terms of x-position [0, n), or -1 for no hover
	HoverStyle	m_hoverStyle		= HoverStyle::none;

	// Data
	size_t m_nPoints; // x values are always plotted as [0, n-1]
	enum class dataRange { xmin, xmax, ymin, ymax };
	double m_dataRange[4] = { nan(""), nan(""), nan(""), nan("") }; // Numerical range of data: x_min, x_max, y_min, y_max
	double m_data_xdiff; // these are calculated on Rescale(), and are what graph objects use to calculate DIPs
	double m_data_ydiff; // note xmax, ymax are not used for any other purpose than calculating these diffs

	// Rects and location
	std::vector<std::tuple<float, double, std::wstring>> m_xTicks; // DIP, value, label
	std::vector<std::tuple<float, double, std::wstring>> m_yTicks; // DIP, value, label
	std::vector<Line_t> m_grid_lines[2]; // x, y
	D2D1_RECT_F			m_axesRect;		// rect for the actual axes
	D2D1_RECT_F			m_dataRect;		// rect for drawable space for data points
	float				m_rect_xdiff;	// m_dataRect.right - m_dataRect.left
	float				m_rect_ydiff;	// m_dataRect.top - m_dataRect.bottom, flip so origin is bottom-left
	D2D1_RECT_F			m_hoverRect;	// background for hover text
	D2D1_POINT_2F		m_hoverLoc;		// position of the mouse in DIPs (to paint crosshairs)

	// Labels
	std::wstring				m_title;
	D2Objects::Formats			m_titleFormat;
	std::vector<date_t>			m_dates; // actual date values (plotted as x = [0, n-1)) for making x labels
	std::vector<std::wstring>	m_userXLabels; // if not dates on x axis, user supplies labels (length n)
	IDWriteTextLayout			*m_hoverLayout = NULL; // layout for hover tooltip text. Remake each OnMouseMove

	// Functions
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

	void Make();
	void Rescale();
	void CalculateXTicks();
	void CalculateXTicks_Dates();
	void CalculateXTicks_User();
	void CalculateYTicks();
	void CreateCachedImage();
	void CreateTriangleMarker(ComPtr<ID2D1PathGeometry> & geometry, int parity);
	void CreateXMarker();
};

