#include "stdafx.h"
#include "Watchlist.h"
#include "TitleBar.h"
#include "utilities.h"

float const Watchlist::m_hTextPad = 4.0f;

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
		if (left >= m_dipRect.right) break;
	}

	// Item texts
	for (auto item : m_items)
		item->Paint(updateRect);

	// Internal lines
	m_d2.pBrush->SetColor(Colors::DULL_LINE);
	for (float line : m_vLines)
	{
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(line, m_dipRect.top),
			D2D1::Point2F(line, m_dipRect.bottom),
			m_d2.pBrush,
			0.5
		);
	}

	// Header and right dividing lines
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

}

void Watchlist::Resize(RECT pRect, D2D1_RECT_F pDipRect)
{
	m_dipRect.bottom = pDipRect.bottom;
	m_pixRect.bottom = pRect.bottom;
}


void Watchlist::Load(std::vector<std::wstring> tickers, std::vector<Column> const & columns)
{
	if (columns.empty())
	{
		m_columns = {
			{60.0f, Column::Ticker, L""},
			{60.0f, Column::Last, L"%.2lf"},
			{60.0f, Column::ChangeP, L"%.2lf"},
			{60.0f, Column::Change1YP, L"%.2lf%%"},
			{60.0f, Column::DivP, L"%.2lf"},
			{50.0f, Column::None, L""}
		};
	}
	else
	{
		m_columns = columns;
		if (m_columns.front().field != Column::Ticker)
			m_columns.insert(m_columns.begin(), { 60.0f, Column::Ticker, L"" });
	}

	// Create column headers, divider lines
	float left = m_dipRect.left;
	m_vLines.reserve(m_columns.size());
	m_pTextLayouts.resize(m_columns.size());
	for (size_t i = 0; i < m_columns.size(); i++)
	{
		if (left + m_columns[i].width >= m_dipRect.right)
		{
			OutputMessage(L"Warning: Columns exceed watchlist width\n");
			if (i > 0)
			{
				m_pTextLayouts.resize(i - 1);
				m_columns.resize(i - 1);
			}
			else
			{
				m_pTextLayouts.clear();
				m_columns.clear();
			}
			break;
		}

		if (i != 0) m_vLines.push_back(left);

		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_columns[i].Name().c_str(),	// The string to be laid out and formatted.
			m_columns[i].Name().size(),		// The length of the string.
			m_d2.pTextFormats[m_format],	// The text format to apply to the string (contains font information, etc).
			m_columns[i].width - 2 * m_hTextPad, // The width of the layout box.
			m_headerHeight,					// The height of the layout box.
			&m_pTextLayouts[i]				// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) Error(L"CreateTextLayout failed");

		left += m_columns[i].width;
	}

	tickers.push_back(L""); // Empty WatchlistItem at end for input
	m_items.reserve(tickers.size() + 1);
	float top = m_dipRect.top + m_headerHeight;
	for (size_t i = 0; i < tickers.size(); i++)
	{
		if (top + m_rowHeight > m_dipRect.bottom) break;

		WatchlistItem *temp = new WatchlistItem(m_hwnd, m_d2);
		temp->SetSize(D2D1::RectF(
			m_dipRect.left,
			top,
			m_dipRect.right,
			top + m_rowHeight
		));
		temp->Load(tickers[i], m_columns);
		m_items.push_back(temp);

		top += m_rowHeight;
	}
}


void WatchlistItem::Paint(D2D1_RECT_F updateRect)
{
	m_ticker.Paint(updateRect);

	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	for (size_t i = 0; i < m_data.size(); i++)
	{
		m_d2.pRenderTarget->DrawTextLayout(
			m_origins[i],
			m_pTextLayouts[i],
			m_d2.pBrush
		);
	}
}

void WatchlistItem::Load(std::wstring const & ticker, std::vector<Column> const & columns)
{
	if (columns.empty() || columns.front().field != Column::Ticker)
	{
		OutputMessage(L"Misformed column vector: WatchlistItem Load()\n");
		return;
	}

	LoadData(ticker, columns);

	// Initialize text box
	float left = m_dipRect.left;
	m_ticker.SetSize(D2D1::RectF(
		left,
		m_dipRect.top,
		left + columns[0].width,
		m_dipRect.bottom
	));
	m_ticker.SetParameters(Watchlist::m_hTextPad, false);
	m_ticker.SetText(ticker);
	left += columns[0].width;

	m_pTextLayouts.resize(m_data.size());
	m_origins.resize(m_data.size());
	for (size_t i = 0; i < m_data.size(); i++)
	{
		// Shift index into columns by 1 to skip Ticker
		if (left + columns[i+1].width >= m_dipRect.right)
		{
			throw std::invalid_argument("Columns exceed WatchlistItem width"); // exception so no clean up necessary
		}

		m_origins[i] = D2D1::Point2F(left + Watchlist::m_hTextPad, m_dipRect.top);

		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_data[i].second.c_str(),				// The string to be laid out and formatted.
			m_data[i].second.size(),				// The length of the string.
			m_d2.pTextFormats[Watchlist::m_format],	// The text format to apply to the string (contains font information, etc).
			columns[i+1].width - 2 * Watchlist::m_hTextPad, // The width of the layout box.
			m_dipRect.bottom - m_dipRect.top,		// The height of the layout box.
			&m_pTextLayouts[i]						// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) Error(L"CreateTextLayout failed");

		left += columns[i+1].width;
	}
}

//TODO
void WatchlistItem::LoadData(std::wstring const & ticker, std::vector<Column> const & columns)
{
	// columns[0] == Ticker
	m_data.resize(columns.size() - 1);
	for (size_t i = 1; i < columns.size(); i++)
	{
		m_data[i - 1].first = 0.0;
		m_data[i - 1].second = L"test";
	}
}

