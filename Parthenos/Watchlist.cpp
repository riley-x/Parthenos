#include "stdafx.h"
#include "Watchlist.h"
#include "TitleBar.h"
#include "utilities.h"

Watchlist::~Watchlist()
{
	for (auto item : m_items) if (item) delete item;
	for (auto layout : m_pTextLayouts) SafeRelease(&layout);
}

// Flush left with fixed width
void Watchlist::Init(float width)
{
	m_dipRect.left = 0.0f;
	m_dipRect.top = DPIScale::SnapToPixelY(TitleBar::height);
	m_dipRect.right = DPIScale::SnapToPixelX(width);

	m_pixRect.left = 0;
	m_pixRect.top = DPIScale::DipsToPixelsY(m_dipRect.top);
	m_pixRect.right = DPIScale::DipsToPixelsX(width);

	m_rightBorder = m_dipRect.right - DPIScale::hpx();
	m_headerBorder = DPIScale::SnapToPixelY(m_dipRect.top + m_headerHeight) - DPIScale::hpy();
}

void Watchlist::Paint(D2D1_RECT_F updateRect)
{
	// When invalidating, converts to pixels then back to DIPs -> updateRect has smaller values
	// than when invalidated.
	if (updateRect.bottom <= m_dipRect.top || updateRect.left + 1 > m_dipRect.right) return;

	// Background
	m_d2.pBrush->SetColor(Colors::WATCH_BACKGROUND);
	m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);

	// Column headers
	float left = m_dipRect.left;
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	for (size_t i = 0; i < m_columns.size(); i++)
	{
		m_d2.pRenderTarget->DrawTextLayout(
			D2D1::Point2F(left + m_hTextPad, m_dipRect.top),
			m_pTextLayouts[i],
			m_d2.pBrush
		);
		left += m_columns[i].width;
	}

	// Header and right dividing line
	m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	m_d2.pRenderTarget->DrawLine(
		D2D1::Point2F(m_rightBorder, m_dipRect.top),
		D2D1::Point2F(m_rightBorder, m_dipRect.bottom),
		m_d2.pBrush,
		DPIScale::px()
	);
	m_d2.pRenderTarget->DrawLine(
		D2D1::Point2F(m_dipRect.left, m_headerBorder),
		D2D1::Point2F(m_rightBorder, m_headerBorder),
		m_d2.pBrush,
		DPIScale::py()
	);

	// Internal lines
	m_d2.pBrush->SetColor(Colors::DULL_LINE);
	for (float line : m_vLines)
	{
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(line, m_dipRect.top),
			D2D1::Point2F(line, m_dipRect.bottom),
			m_d2.pBrush,
			DPIScale::px()
		);
	}
}

void Watchlist::Resize(RECT pRect, D2D1_RECT_F pDipRect)
{
	m_dipRect.bottom = pDipRect.bottom;
	m_pixRect.bottom = pRect.bottom;
}

// TODO
void Watchlist::Load(std::vector<std::wstring> tickers, std::vector<Column> columns)
{
	if (columns.empty())
	{
		m_columns = { {50.0f, L"Ticker", L""}, {50.0f, L"Last", L"%.2lf"} };
	}
	else
	{
		m_columns = columns;
	}

	// Create column headers, divider lines
	float left = m_dipRect.left;
	m_vLines.reserve(m_columns.size());
	m_pTextLayouts.resize(m_columns.size());
	for (size_t i = 0; i < m_columns.size(); i++)
	{
		if (i != 0) m_vLines.push_back(left);

		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_columns[i].name.c_str(),		// The string to be laid out and formatted.
			m_columns[i].name.size(),		// The length of the string.
			m_d2.pTextFormats[m_format],	// The text format to apply to the string (contains font information, etc).
			m_columns[i].width - 2 * m_hTextPad, // The width of the layout box.
			m_headerHeight,					// The height of the layout box.
			&m_pTextLayouts[i]				// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) Error(L"CreateTextLayout failed");

		left += m_columns[i].width;
	}

	//
	return;

	m_items.push_back(new WatchlistItem(m_hwnd, m_d2));
	m_items.push_back(new WatchlistItem(m_hwnd, m_d2));
	for (size_t i = 0; i < m_items.size(); i++)
	{
		//m_items[i]->SetSize();
	}
}

void WatchlistItem::SetSize(D2D1_RECT_F dipRect)
{
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	//m_ticker.SetSize();
}

void WatchlistItem::Paint(D2D1_RECT_F updateRect)
{
}

