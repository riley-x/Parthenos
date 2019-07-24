#pragma once

#include "../stdafx.h"
#include "AppItem.h"
#include "TextBox.h"
#include "Table.h"
#include "../Utilities/DataManaging.h"

class WatchlistItem;

struct WatchlistColumn
{
	enum Field { None, Ticker, Last, ChangeP, Change1YP, DivP, ExDiv, Shares, AvgCost, Equity, 
		Realized, Unrealized, ReturnsT, ReturnsP, APY, EarningsDate };

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
		case EarningsDate: return L"Earnings";
		case None:
		default:
			return L"";
		}
	}

	inline ColumnType Type() const
	{
		switch (field)
		{
		case Ticker:
			return ColumnType::String;
		default:
			return ColumnType::Double;
		}
	}
};

class Watchlist : public Table
{
public:
	Watchlist(HWND hwnd, D2Objects const & d2, CTPMessageReceiver * parent, bool editable)
		: Table(hwnd, d2, parent, editable),  m_editableTickers(editable) {}

	// AppItem overrides
	void ProcessCTPMessages();

	// Interface
	void SetColumns(); // default columns
	void SetColumns(std::vector<WatchlistColumn> const & columns);
	void Load(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions);
	void Load(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions,
		std::vector<std::pair<Quote, Stats>> const & data);

private:
	// Data
	std::vector<WatchlistColumn>	m_columns;
	std::vector<Position>			m_positions; // Store these in case columns change from user input
	
	// Flags
	bool m_editableTickers	= true;

	// Deleted
	Watchlist(const Watchlist&) = delete;				// non construction-copyable
	Watchlist& operator=(const Watchlist&) = delete;	// non copyable
};



class WatchlistItem : public TableRowItem
{
public:
	WatchlistItem(HWND hwnd, D2Objects const & d2, Watchlist *parent, bool editable = true);
	~WatchlistItem() { for (auto item : m_pTextLayouts) SafeRelease(&item); }

	// AppItem overrides
	void SetSize(D2D1_RECT_F dipRect);	// Load should be called before SetSize
	void Paint(D2D1_RECT_F updateRect);
	inline bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	inline void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	inline bool OnChar(wchar_t c, LPARAM lParam);
	inline bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	inline void OnTimer(WPARAM wParam, LPARAM lParam);
	void ProcessCTPMessages();

	// TableRowItem ovverrides
	double GetData(size_t iColumn) const;
	std::wstring GetString(size_t iColumn) const;

	// Interface
	void Load(std::wstring const & ticker, std::vector<WatchlistColumn> const & columns,
		std::pair<Quote, Stats> const * data = nullptr, Position const * position = nullptr, bool reload = false);
	void CreateTextLayouts();

	inline std::wstring Ticker() const { return m_currTicker; }

private:
	// Objects
	Watchlist *m_parent;
	TextBox m_ticker;

	// Data
	std::wstring m_currTicker;
	std::vector<WatchlistColumn> m_columns;
	std::vector<std::pair<double, std::wstring>> m_data; // data, display string
	std::vector<IDWriteTextLayout*> m_pTextLayouts;
	std::vector<float> m_origins; // left DIP for the text layouts

	bool m_LButtonDown = false;
	bool m_editableTickers;

	// Deleted
	WatchlistItem(const WatchlistItem&) = delete; // non construction-copyable
	WatchlistItem& operator=(const WatchlistItem&) = delete; // non copyable
};
