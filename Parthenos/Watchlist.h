#pragma once

#include "stdafx.h"
#include "AppItem.h"
#include "TextBox.h"
#include "DataManaging.h"
#include "ScrollBar.h"

class Watchlist;

struct Column
{
	enum Field { None, Ticker, Last, ChangeP, Change1YP, DivP, ExDiv, Shares, AvgCost, Equity, Realized, Unrealized, ReturnsT, ReturnsP, APY };

	float width;
	Field field = None;
	std::wstring format;

	inline std::wstring Name() const
	{
		switch (field)
		{
		case Ticker:	return L"Ticker";
		case Last:		return L"Last";
		case ChangeP:	return L"\u0394 %"; // \u0394 == \Delta
		case Change1YP: return L"\u03941Y %";
		case DivP:		return L"Div %";
		case ExDiv:		return L"Ex Div";
		case Shares:	return L"Shares";
		case AvgCost:	return L"Avg Cost";
		case Equity:	return L"Equity";
		case Realized:	return L"Realized";
		case Unrealized: return L"Unrealized";
		case ReturnsT:	return L"Returns";
		case ReturnsP:	return L"Return %";
		case APY:		return L"APY %";
		case None:
		default:
			return L"";
		}
	}
};



class WatchlistItem : public AppItem
{
public:
	WatchlistItem(HWND hwnd, D2Objects const & d2, Watchlist *parent, bool editable = true);
	~WatchlistItem() { for (auto item : m_pTextLayouts) SafeRelease(&item); }

	// AppItem overrides
	void SetSize(D2D1_RECT_F dipRect);	// Load should be called before SetSize
	void Paint(D2D1_RECT_F updateRect);
	inline bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled) { return m_ticker.OnMouseMove(cursor, wParam, handeled); }
	inline bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
	{
		if (m_editableTickers) handeled = m_ticker.OnLButtonDown(cursor, handeled) || handeled;
		if (!handeled && inRect(cursor, m_dipRect)) m_LButtonDown = true;
		
		ProcessCTPMessages();
		return handeled;
	}
	inline void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam) { return m_ticker.OnLButtonDblclk(cursor, wParam); }
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	inline bool OnChar(wchar_t c, LPARAM lParam) { return m_ticker.OnChar(c, lParam); }
	inline bool OnKeyDown(WPARAM wParam, LPARAM lParam) 
	{
		bool out = m_ticker.OnKeyDown(wParam, lParam); 
		ProcessCTPMessages();
		return out;
	}
	inline void OnTimer(WPARAM wParam, LPARAM lParam) { return m_ticker.OnTimer(wParam, lParam); }

	void ProcessCTPMessages();


	// Interface
	void Load(std::wstring const & ticker, std::vector<Column> const & columns, 
		std::pair<Quote, Stats> const * data = nullptr, Position const * position = nullptr, bool reload = false);
	void CreateTextLayouts();

	inline std::wstring Ticker() const { return m_currTicker; }
	inline double GetData(size_t iColumn) const
	{ 
		if (iColumn - 1 < m_data.size())
			return m_data[iColumn - 1].first;
		else // i.e. empty ticker -> no data. return -inf for sorting
			return -std::numeric_limits<double>::infinity();
	}

	//void Add(Column const & col); // Queries 
	//void Move(size_t iColumn, size_t iPos);
	//void Delete(size_t iColumn);
	void TruncateColumns(size_t i);

private:
	WatchlistItem(const WatchlistItem&) = delete; // non construction-copyable
	WatchlistItem& operator=(const WatchlistItem&) = delete; // non copyable

	Watchlist *m_parent;
	TextBox m_ticker;

	std::wstring m_currTicker;
	std::vector<Column> m_columns;
	std::vector<std::pair<double, std::wstring>> m_data; // data, display string
	std::vector<IDWriteTextLayout*> m_pTextLayouts;
	std::vector<float> m_origins; // left DIP for the text layouts
	
	bool m_LButtonDown = false;
	bool m_editableTickers;
};


class Watchlist : public AppItem
{
public:
	Watchlist(HWND hwnd, D2Objects const & d2, CTPMessageReceiver * parent, bool editable)
		: AppItem(hwnd, d2, parent), m_scrollBar(hwnd, d2, this), m_editableTickers(editable)
	{
		// Set 1 line per minStep == detent
		m_scrollBar.SetMinStep(WHEEL_DELTA);
	}
	~Watchlist();

	// AppItem overrides
	void Paint(D2D1_RECT_F updateRect);
	void SetSize(D2D1_RECT_F dipRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnChar(wchar_t c, LPARAM lParam);
	bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM wParam, LPARAM lParam);
	void ProcessCTPMessages();

	// Interface
	void SetColumns(); // default columns
	void SetColumns(std::vector<Column> const & columns);
	void Load(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions);
	void Load(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions,
		std::vector<std::pair<Quote, Stats>> const & data);
	inline void Refresh() { CalculatePositions(); CreateTextLayouts(); } // Load creates items but not with position, so call refresh

	// Parameters
	static float const m_hTextPad; // 4.0f
	static D2Objects::Formats const m_format = D2Objects::Segoe12;

private:
	// Objects
	ScrollBar						m_scrollBar;
	std::vector<WatchlistItem*>		m_items;

	// Data
	std::vector<Column>				m_columns;
	std::vector<IDWriteTextLayout*> m_pTextLayouts; // For column headers
	std::vector<Position>			m_positions; // Store these in case columns change from user input
	
	// Flags
	int			m_LButtonDown		= -1;	 // Left button pressed on an item
	int			m_hover				= -1;	 // Currently hovering over
	bool		m_ignoreSelection	= false; // Flag to check if drag + drop on same location
	int			m_sortColumn		= -1;	 // So that double clicking on the same column sorts in reverse

	// Paramters
	float const m_headerHeight		= 18.0f;
	float const m_rowHeight			= 18.0f;
	bool		m_editableTickers	= true;

	// Scroll State
	size_t		m_visibleLines;
	size_t		m_currLine = 0; // topmost visible line

	// Drawing
	std::vector<float>	m_vLines;
	std::vector<float>	m_hLines;
	float				m_headerBorder;

	// Helpers
	void CalculatePositions();
	void CreateTextLayouts();
	// Get top of item i
	inline float GetHeight(int i) { return m_dipRect.top + m_headerHeight + (i - m_currLine) * m_rowHeight; }
	// Get index given y coord
	inline int GetItem(float y) { return static_cast<int>(floor((y - (m_dipRect.top + m_headerHeight)) / m_rowHeight)) + m_currLine; }
	// Move item iOld to iNew, shifting everything in between appropriately.
	// If iNew < iOld, moves iOld to above the current item at iNew.
	// If iNew > iOld, moves iOld to below the current item at iNew.
	// No bounds check. Make sure iOld and iNew are valid indices.
	void MoveItem(int iOld, int iNew);
	void SortByColumn(size_t iColumn);

	// Deleted
	Watchlist(const Watchlist&) = delete;				// non construction-copyable
	Watchlist& operator=(const Watchlist&) = delete;	// non copyable
};
