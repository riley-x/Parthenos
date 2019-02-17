#pragma once

#include "stdafx.h"
#include "AppItem.h"
#include "TextBox.h"



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
		case ChangeP: return L"\u0394%"; // \u0394 == \Delta
		case Change1YP: return L"\u0394% 1Y";
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
	WatchlistItem(HWND hwnd, D2Objects const & d2) :
		AppItem(hwnd, d2), m_ticker(hwnd, d2, this), m_data(10) {}
	~WatchlistItem() { for (auto item : m_pTextLayouts) SafeRelease(&item); }

	void Paint(D2D1_RECT_F updateRect);

	//void ReceiveMessage(std::wstring msg, int i);

	void Load(std::wstring const & ticker, std::vector<Column> const & columns); // Queries
	// Load should be called after SetSize

	void Add(Column const & col); // Queries 
	void Move(size_t iColumn, size_t iPos);
	void Delete(size_t iColumn);

private:
	TextBox m_ticker;
	std::vector<std::pair<double, std::wstring>> m_data; // data, display string
	std::vector<IDWriteTextLayout*> m_pTextLayouts;
	std::vector<D2D1_POINT_2F> m_origins;

	// Do the actual HTTP query and format the string
	void LoadData(std::wstring const & ticker, std::vector<Column> const & columns);
};

class Watchlist : public AppItem
{
public:
	using AppItem::AppItem;
	~Watchlist();

	void Init(float width);
	void Paint(D2D1_RECT_F updateRect);
	void Resize(RECT pRect, D2D1_RECT_F pDipRect);

	void Load(std::vector<std::wstring> tickers, std::vector<Column> const & columns);

	// Parameters
	static float const m_hTextPad; // 4.0f
	static D2Objects::Formats const m_format = D2Objects::Segoe12;

private:
	std::vector<WatchlistItem*> m_items;
	std::vector<Column> m_columns;
	std::vector<IDWriteTextLayout*> m_pTextLayouts; // For column headers
	
	// Paramters
	float const m_headerHeight = 18.0f;
	float const m_rowHeight = 14.0f;

	// Drawing
	std::vector<float> m_vLines;
	float m_rightBorder;
	float m_headerBorder;
};
