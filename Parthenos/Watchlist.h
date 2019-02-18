#pragma once

#include "stdafx.h"
#include "AppItem.h"
#include "TextBox.h"


class Parthenos;

struct Column
{
	enum Field { None, Ticker, Last, ChangeP, Change1YP, DivP };

	float width;
	Field field = None;
	std::wstring format;

	inline std::wstring Name() const
	{
		switch (field)
		{
		case Ticker: return L"Ticker";
		case Last: return L"Last";
		case ChangeP: return L"\u0394 %"; // \u0394 == \Delta
		case Change1YP: return L"\u03941Y %";
		case DivP: return L"Div %";
		case None:
		default:
			return L"";
		}
	}
};



class WatchlistItem : public AppItem
{
public:
	WatchlistItem(HWND hwnd, D2Objects const & d2, AppItem *parent) :
		AppItem(hwnd, d2), m_parent(parent), m_ticker(hwnd, d2, this), m_data(10) {}
	~WatchlistItem() { for (auto item : m_pTextLayouts) SafeRelease(&item); }

	// AppItem overrides
	void SetSize(D2D1_RECT_F dipRect); // The height should not change, or else call Load() again
	void Paint(D2D1_RECT_F updateRect);
	inline void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam) { m_ticker.OnMouseMove(cursor, wParam); }
	inline bool OnLButtonDown(D2D1_POINT_2F cursor)
	{
		bool out = m_ticker.OnLButtonDown(cursor);
		if (!out && inRect(cursor, m_dipRect)) m_LButtonDown = true;
		ProcessMessages();
		return out;
	}
	inline void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam) { return m_ticker.OnLButtonDblclk(cursor, wParam); }
	inline void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	inline bool OnChar(wchar_t c, LPARAM lParam) { return m_ticker.OnChar(c, lParam); }
	inline bool OnKeyDown(WPARAM wParam, LPARAM lParam) 
	{
		bool out = m_ticker.OnKeyDown(wParam, lParam); 
		ProcessMessages();
		return out;
	}
	inline void OnTimer(WPARAM wParam, LPARAM lParam) { return m_ticker.OnTimer(wParam, lParam); }

	void ProcessMessages();

	// Interface
	inline std::wstring Ticker() const { return m_currTicker; }
	void Load(std::wstring const & ticker, std::vector<Column> const & columns, bool reload = false); // Queries
	// Load should be called after SetSize
	void Add(Column const & col); // Queries 
	void Move(size_t iColumn, size_t iPos);
	void Delete(size_t iColumn);

private:
	WatchlistItem(const WatchlistItem&) = delete; // non construction-copyable
	WatchlistItem& operator=(const WatchlistItem&) = delete; // non copyable

	AppItem *m_parent;
	TextBox m_ticker;
	std::wstring m_currTicker;
	std::vector<Column> m_columns;
	std::vector<std::pair<double, std::wstring>> m_data; // data, display string
	std::vector<IDWriteTextLayout*> m_pTextLayouts;
	std::vector<float> m_origins; // left DIP for the text layouts

	bool m_LButtonDown = false;

	// Do the actual HTTP query and format the string
	void LoadData(std::wstring const & ticker);

};


class Watchlist : public AppItem
{
public:
	Watchlist(HWND hwnd, D2Objects const & d2, Parthenos * parent)
		: AppItem(hwnd, d2), m_parent(parent) {}
	~Watchlist();

	// AppItem overrides
	void Init(float width);
	void Paint(D2D1_RECT_F updateRect);
	void Resize(RECT pRect, D2D1_RECT_F pDipRect);
	void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnLButtonDown(D2D1_POINT_2F cursor);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnChar(wchar_t c, LPARAM lParam);
	bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM wParam, LPARAM lParam);

	void ProcessMessages();

	// Interface
	void Load(std::vector<std::wstring> tickers, std::vector<Column> const & columns);

	// Parameters
	static float const m_hTextPad; // 4.0f
	static D2Objects::Formats const m_format = D2Objects::Segoe12;

private:
	Watchlist(const Watchlist&) = delete; // non construction-copyable
	Watchlist& operator=(const Watchlist&) = delete; // non copyable

	Parthenos *m_parent;

	// Data
	std::vector<WatchlistItem*> m_items;
	std::vector<WatchlistItem*> m_adds;
	std::vector<WatchlistItem*> m_deletes;
	std::vector<Column> m_columns;
	std::vector<IDWriteTextLayout*> m_pTextLayouts; // For column headers
	
	// Flags
	int m_LButtonDown = -1; // Left button pressed on an item
	int m_hover = -1; // Currently hovering over
	bool m_ignoreSelection = false; // Flag to check if drag + drop on same location

	// Paramters
	float const m_headerHeight = 18.0f;
	float const m_rowHeight = 18.0f;

	// Drawing
	std::vector<float> m_vLines;
	std::vector<float> m_hLines;
	float m_rightBorder;
	float m_headerBorder;

	// Helpers

	// Get top of item i
	inline float GetHeight(int i) { return m_dipRect.top + m_headerHeight + i * m_rowHeight; }
	// Get index given y coord
	inline int GetItem(float y) { return static_cast<int>((y - (m_dipRect.top + m_headerHeight)) / m_rowHeight); }
	// Move item iOld to iNew, shifting everything in between appropriately.
	// If iNew < iOld, moves iOld to above the current item at iNew.
	// If iNew > iOld, moves iOld to below the current item at iNew.
	// No bounds check. Make sure iOld and iNew are valid indices.
	void MoveItem(int iOld, int iNew);
};
