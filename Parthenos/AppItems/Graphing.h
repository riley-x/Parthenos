#pragma once

#include "../stdafx.h"
#include "AppItem.h"
#include "../Utilities/DataManaging.h"

class Axes;

///////////////////////////////////////////////////////////
// --- Structs ---

typedef struct LINE_STRUCT {
	D2D1_POINT_2F start;
	D2D1_POINT_2F end;
} Line_t;

typedef struct LINE_PROPERTIES {
	D2D1_COLOR_F color = Colors::BRIGHT_LINE;
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
	// TODO add Rescale() -- keeps data but changes what is visible

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
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);

	///////////////////////////////////////////////////////////////////////////
	// Interface
	///////////////////////////////////////////////////////////////////////////
	
	// Clear and remove
	void Clear();
	void Clear(GraphGroup group);
	void Remove(GraphGroup group, std::wstring name);

	// Plot functions (add a Graph)
	void Candlestick(std::vector<OHLC> const & ohlc, GraphGroup group = GG_PRI);
	void Line(std::vector<date_t> const & dates, std::vector<double> const & ydata,
		LineProps props = {}, GraphGroup group = GG_PRI, std::wstring name = L""
	);
	void Bar(std::vector<std::pair<double, D2D1_COLOR_F>> const & data,
		std::vector<std::wstring> const & labels = {},
		GraphGroup group = GG_PRI, size_t offset = 0 // offset of first point from x_min of primary graph(s)
	);
	void DatePoints(std::vector<PointProps> points, GraphGroup group, std::wstring name = L"");

	///////////////////////////////////////////////////////////////////////////
	// Behavior and Appearance
	///////////////////////////////////////////////////////////////////////////

	inline void SetXAxisPos(double y) { m_xAxisPos = y; }
	inline void SetYAxisPos(double x) { m_yAxisPos = x; }
	inline void SetYGridLines(std::vector<double> ticks) { m_userYGridLines = ticks; } // set empty vector for auto

	void SetTitle(std::wstring const& title, D2Objects::Formats format = D2Objects::Formats::Segoe18);

	inline void SetLabelSize(float ylabelWidth, float labelHeight)
	{
		m_ylabelWidth = ylabelWidth;
		m_labelHeight = labelHeight;
	}
	inline void DrawXLabels(bool draw) { m_drawXLabels = draw; }
	inline void SetXLabels(std::vector<std::wstring> const& labels, bool draw = true)
	{
		m_userXLabels = labels; m_drawXLabels = draw;
	}

	void SetMouseWatch(std::vector<const Axes*> const& partners) { m_mouseWatch = partners; }
			// Will watch for mouse activity in partners->getAxesRect(), presumably for same-x axes partners.
			// Will update drawing for things like hover and selection along the same x positions.
			// If in(other->rect) && !in(m_dipRect), will not i.e. send messages or return handled=true on handlers.
			// 'this' can be in partners. Make sure to update partners appropriately if they are deleted.
	void ResetMouseWatch() { m_mouseWatch.clear(); }

	inline HoverStyle GetHoverStyle() const { return m_hoverStyle; }
	inline void SetHoverStyle(HoverStyle sty) { m_hoverStyle = sty; }

	///////////////////////////////////////////////////////////////////////////
	// Utility
	///////////////////////////////////////////////////////////////////////////

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

	// Simple getters and setters
	inline float GetDataRectXDiff() const { return m_rect_xdiff; }
	inline D2D1_RECT_F GetAxesRect() const { return m_axesRect; }
	inline float GetDataPad() const { return m_dataPad; }
	inline size_t GetNPoints() const { return m_nPoints; }
	ID2D1PathGeometry* GetMarker(MarkerStyle m);

	void SetName(std::wstring const& name) { m_name = name; }
	std::wstring const& GetName() const { return m_name; }

private:

	std::wstring m_name; // identifier

	// Objects
	std::vector<Graph*>			m_graphObjects[nGraphGroups]; // individual graphs to plot. these need to be remade when the size of the window changes.
	ComPtr<ID2D1Bitmap1>		m_primaryCache = nullptr; // caches graph groups 0 and 1, grids, labels, etc.
	ComPtr<ID2D1PathGeometry>	m_upMarker = nullptr;
	ComPtr<ID2D1PathGeometry>	m_dnMarker = nullptr;
	ComPtr<ID2D1PathGeometry>	m_xxMarker = nullptr;

	// Parameters
	float m_ylabelWidth = 40.0f; // width of y tick labels in DIPs.
	float m_labelHeight = 16.0f; // height of tick labels in DIPs.
	float m_dataPad		= 20.0f; // padding between datapoints and border
	float m_labelPad	= 2.0f;  // padding between axes labels and axes; also for hover text and bounding box
	float m_titlePad	= 0.0f;  // offset between dipRect.top and axesRect.top
	double m_xAxisPos	= -std::numeric_limits<double>::infinity(); // y position. -inf to draw at m_axesRect.bottom
	double m_yAxisPos	= std::numeric_limits<double>::infinity();  // x position. +inf to draw at m_axesRect.right
	bool	m_drawXLabels			= true;  // set before SetSize()
	bool	m_drawXGridLines		= true;  // set before SetSize()
	HoverStyle	m_hoverStyle = HoverStyle::none;

	// Flags and state variables
	bool	m_ismade				= true;  // check to make sure everything is made
	size_t	m_imade[nGraphGroups]	= {};	 // index into m_graphObjects. Objects < i are already made
	bool	m_rescaled				= false; // data ranges changed, so need to call Rescale()

	bool	m_select				= true;  // allow mouse to select region
	bool	m_selectActive			= false; // If currently in the proccess of selecting. Set by LButtonDown, unset by LButton{Down,Up}.
											 // This can be false but still tracking a selection in m_mouseWatch, indicated by m_selectStart >= 0.
	int		m_selectStart			= -1;	 // Selection start point in terms of x-position [0, n)
	int		m_selectEnd				= -1;

	D2D1_POINT_2F m_hoverLoc;	// position of the crosshairs in DIPs, or -1 for no x/y line
	int		m_hoverTextX = -1;	// data point for hover text to display, [0, n), or -1 for no hover text

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
	std::vector<const Axes*> m_mouseWatch; // See SetMouseWatch(). Axes pointers not owned.

	// Labels
	std::wstring				m_title;
	D2Objects::Formats			m_titleFormat;
	std::vector<date_t>			m_dates; // actual date values (plotted as x = [0, n-1)) for making x labels
	std::vector<double>			m_userYGridLines; // manual placement of grid lines
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
	void CalculateYTicks_Auto();
	void CalculateYTicks_User();
	void CreateCachedImage();
	void CreateTriangleMarker(ComPtr<ID2D1PathGeometry> & geometry, int parity);
	void CreateXMarker();
	void CreateHoverText(size_t xind, D2D1_POINT_2F cursor);

	void ResetSelection()
	{
		m_selectActive = false;
		m_selectStart = -1;
		m_selectEnd = -1;
	}

	bool InMouseWatch(D2D1_POINT_2F cursor); // If cursor in any of the mouse watch axes rects.
};

