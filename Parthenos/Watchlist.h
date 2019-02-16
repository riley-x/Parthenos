#pragma once

#include "stdafx.h"
#include "AppItem.h"
#include "TextBox.h"

struct Column
{
	float width;
	std::wstring name;
	std::wstring format;
};

class WatchlistItem : public AppItem
{
public:
	WatchlistItem(HWND hwnd, D2Objects const & d2) :
		AppItem(hwnd, d2), m_ticker(hwnd, d2, this), m_data(10) {}

	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);

	//void ReceiveMessage(std::wstring msg, int i);

	void Load(std::wstring ticker, std::vector<Column> columns); // Queries
	void Add(Column const & col); // Queries 
	void Move(size_t iColumn, size_t iPos);
	void Delete(size_t iColumn);

private:
	TextBox m_ticker;
	std::vector<std::pair<double, std::wstring>> m_data; // data, display string
};

class Watchlist : public AppItem
{
public:
	using AppItem::AppItem;
	~Watchlist();

	void Init(float width);
	void Paint(D2D1_RECT_F updateRect);
	void Resize(RECT pRect, D2D1_RECT_F pDipRect);

	void Load(std::vector<std::wstring> tickers, std::vector<Column> columns);
private:
	std::vector<WatchlistItem*> m_items;
	std::vector<Column> m_columns;
	std::vector<IDWriteTextLayout*> m_pTextLayouts; // For column headers
	
	// Paramters
	D2Objects::Formats const m_format = D2Objects::Segoe12;
	float const m_headerHeight = 18.0f;
	float const m_hTextPad = 4.0f;

	// Drawing
	std::vector<float> m_vLines;
	float m_rightBorder;
	float m_headerBorder;
};
