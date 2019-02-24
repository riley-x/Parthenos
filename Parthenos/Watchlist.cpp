#include "stdafx.h"
#include "Watchlist.h"
#include "TitleBar.h"
#include "utilities.h"
#include "DataManaging.h"
#include "Parthenos.h"

float const Watchlist::m_hTextPad = 4.0f;

Watchlist::~Watchlist()
{
	for (auto item : m_items) if (item) delete item;
	for (auto layout : m_pTextLayouts) SafeRelease(&layout);
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
	for (float line : m_hLines)
	{
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(m_dipRect.left, line),
			D2D1::Point2F(m_dipRect.right, line),
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

	// Drag-drop insertion line
	if (m_LButtonDown >= 0 && m_hover >= 0)
	{
		float y = DPIScale::SnapToPixelY(GetHeight(m_hover)) - DPIScale::hpy();
		m_d2.pBrush->SetColor(Colors::ACCENT);
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(m_dipRect.left, y),
			D2D1::Point2F(m_dipRect.right, y),
			m_d2.pBrush,
			DPIScale::py()
		);
	}

}

// TODO don't need to re-calculate layouts each set size, i.e. making the table larger?
void Watchlist::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(m_dipRect, dipRect)) return;
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	m_rightBorder = m_dipRect.right - DPIScale::hpx();
	m_headerBorder = DPIScale::SnapToPixelY(m_dipRect.top + m_headerHeight) - DPIScale::hpy();

	CalculateLayouts();
}

void Watchlist::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (m_editableTickers)
	{
		for (auto item : m_items) item->OnMouseMove(cursor, wParam);
	}

	int i = GetItem(cursor.y);
	if (m_LButtonDown >= 0 && (wParam & MK_LBUTTON) && i != m_hover)
	{
		m_ignoreSelection = true; // is dragging, so don't update even if dropped in same location
		if (i >= 0 && i < static_cast<int>(m_items.size()))
		{
			m_hover = i;
			::InvalidateRect(m_hwnd, &m_pixRect, false);
		}
		else
		{
			m_hover = -1;
			::InvalidateRect(m_hwnd, &m_pixRect, false);
		}
	}
	//ProcessCTPMessages();
}

bool Watchlist::OnLButtonDown(D2D1_POINT_2F cursor)
{
	bool isTextBox = false;
	for (auto item : m_items)
	{
		if (item->OnLButtonDown(cursor)) isTextBox = true;
	}
	if (!isTextBox && inRect(cursor, m_dipRect))
	{
		int i = GetItem(cursor.y);
		if (i >= 0 && i < static_cast<int>(m_items.size()))
		{
			if (i == m_items.size() - 1 && m_items.back()->Ticker().empty()) return false; // can't select empty item at end
			m_LButtonDown = i;
			// Reset flags for dragging -- not dragging yet
			m_hover = -1;
			m_ignoreSelection = false; 
		}
	}
	ProcessCTPMessages();
	return false; // unused
}

void Watchlist::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (!inRect(cursor, m_dipRect)) return;
	
	int i = GetItem(cursor.y);
	if (i >= 0 && i < static_cast<int>(m_items.size()) && m_editableTickers)
	{
		m_items[i]->OnLButtonDblclk(cursor, wParam);
	}
	else if (i < 0) // double click on header
	{
		auto it = lower_bound(m_vLines.begin(), m_vLines.end(), cursor.x);
		if (it == m_vLines.end()) return;
		size_t column = it - m_vLines.begin();
		SortByColumn(column);
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	}
	//ProcessCTPMessages();
}

// If dragging an element, does the insertion above the drop location
void Watchlist::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	for (auto item : m_items) item->OnLButtonUp(cursor, wParam);
	if (m_LButtonDown >= 0)
	{
		int iNew = GetItem(cursor.y);
		bool draw = false;

		// check for drag + drop
		if (iNew >= 0 && iNew < static_cast<int>(m_items.size()))
		{
			if (iNew < m_LButtonDown)
				MoveItem(m_LButtonDown, iNew);
			else if (iNew > m_LButtonDown + 1)
				MoveItem(m_LButtonDown, iNew - 1); // want insertion above drop location, not below
			if (m_hover >= 0) ::InvalidateRect(m_hwnd, &m_pixRect, false);
		}
		m_hover = -1;
		m_LButtonDown = -1;
	}
	ProcessCTPMessages();
}

bool Watchlist::OnChar(wchar_t c, LPARAM lParam)
{
	if (m_editableTickers)
	{
		c = towupper(c);
		for (auto item : m_items)
		{
			if (item->OnChar(c, lParam)) return true;
		}
	}
	//ProcessCTPMessages();
	return false;
}

bool Watchlist::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = false;
	if (m_editableTickers)
	{
		for (auto item : m_items)
		{
			if (item->OnKeyDown(wParam, lParam)) out = true;
		}
		ProcessCTPMessages();
	}
	return out;
}

void Watchlist::OnTimer(WPARAM wParam, LPARAM lParam)
{
	if (m_editableTickers)
	{
		for (auto item : m_items) item->OnTimer(wParam, lParam);
	}
	//ProcessCTPMessages();
}

void Watchlist::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::WATCHLISTITEM_NEW:
		{
			float top = GetHeight(m_items.size());
			if (top + m_rowHeight > m_dipRect.bottom) break;

			WatchlistItem *temp = new WatchlistItem(m_hwnd, m_d2, this);
			temp->SetSize(D2D1::RectF(
				m_dipRect.left,
				top,
				m_dipRect.right,
				top + m_rowHeight
			));
			temp->Load(L"", m_columns);
			m_items.push_back(temp);
			m_hLines.push_back(top + m_rowHeight);
			break;
		}
		case CTPMessage::WATCHLISTITEM_EMPTY:
		{
			// Move empty watchlist item to end then delete
			bool bDelete = m_items.back()->Ticker().empty(); // empty item at end already
			auto it = std::find(m_items.begin(), m_items.end(), msg.sender);
			if (it == m_items.end() || it + 1 == m_items.end()) break;
			MoveItem(it - m_items.begin(), m_items.size() - 1);
			if (bDelete)
			{
				delete m_items.back();
				m_items.pop_back();
				m_hLines.pop_back();
			}
			break;
		}
		case CTPMessage::WATCHLIST_SELECTED:
		{
			if (m_ignoreSelection) break; // LButtonDown will reset to false
			m_parent->PostClientMessage(this, msg.msg, msg.imsg);
			break;
		}
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

void Watchlist::Load(std::vector<std::wstring> const & tickers, std::vector<Column> const & columns, 
	std::vector<Position> const & positions)
{
	m_tickers = tickers;
	m_tickers.push_back(L""); // Empty WatchlistItem at end for input
	if (columns.empty())
	{
		m_columns = {
			{90.0f, Column::Ticker, L""},
			{60.0f, Column::Last, L"%.2lf"},
			{60.0f, Column::ChangeP, L"%.2lf"},
			{60.0f, Column::Change1YP, L"%.2lf"},
			{60.0f, Column::DivP, L"%.2lf"}
		};
	}
	else
	{
		m_columns = columns;
		if (m_columns.front().field != Column::Ticker)
			m_columns.insert(m_columns.begin(), { 60.0f, Column::Ticker, L"" });
	}
	m_positions = positions;
}


void Watchlist::Reload(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions)
{
	m_tickers = tickers;
	m_tickers.push_back(L""); // Empty WatchlistItem at end for input
	m_positions = positions;

	CreateItems();
}

void Watchlist::CalculateLayouts()
{
	// Create column headers, vertical divider lines
	float left = m_dipRect.left;
	m_vLines.resize(m_columns.size());
	for (auto x : m_pTextLayouts) SafeRelease(&x);
	m_pTextLayouts.resize(m_columns.size());

	for (size_t i = 0; i < m_columns.size(); i++)
	{
		if (left + m_columns[i].width >= m_dipRect.right)
		{
			OutputMessage(L"Warning: Columns exceed watchlist width\n");
			if (i > 0)
			{
				m_vLines.resize(i - 1);
				m_pTextLayouts.resize(i - 1);
				m_columns.resize(i - 1);
			}
			else
			{
				m_vLines.clear();
				m_pTextLayouts.clear();
				m_columns.clear();
			}
			break;
		}

		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_columns[i].Name().c_str(),	// The string to be laid out and formatted.
			m_columns[i].Name().size(),		// The length of the string.
			m_d2.pTextFormats[m_format],	// The text format to apply to the string (contains font information, etc).
			m_columns[i].width - 2 * m_hTextPad, // The width of the layout box.
			m_headerHeight,					// The height of the layout box.
			&m_pTextLayouts[i]				// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) throw Error(L"CreateTextLayout failed");

		left += m_columns[i].width;
		m_vLines[i] = left; // i.e. right sisde of column i
	}

	CreateItems();
}

// Clears and populates m_items based on tickers in m_tickers, setting their position appropriately,
// using data from an iex batch fetch and data in m_positions. Also creates the horizontal divider lines.
void Watchlist::CreateItems()
{
	// Get data via batch request
	std::vector<std::wstring> tickers(m_tickers.begin(), m_tickers.end() - 1); // empty string at end
	std::vector<std::pair<Quote, Stats>> data = GetBatchQuoteStats(tickers);

	// Create WatchlistItems, horizontal divider lines
	m_hLines.resize(m_tickers.size());
	for (auto x : m_items) if (x) delete x;
	m_items.clear();
	m_items.reserve(m_tickers.size());

	float top = m_dipRect.top + m_headerHeight;
	for (size_t i = 0; i < m_tickers.size(); i++)
	{
		if (top + m_rowHeight > m_dipRect.bottom) break;

		// Get position if available
		Position const * pPosition = nullptr;
		auto it = std::lower_bound(m_positions.begin(), m_positions.end(), m_tickers[i], Positions_Compare);
		if (it != m_positions.end()) pPosition = &(*it);

		WatchlistItem *temp = new WatchlistItem(m_hwnd, m_d2, this, m_editableTickers);
		temp->SetSize(D2D1::RectF(
			m_dipRect.left,
			top,
			m_dipRect.right,
			top + m_rowHeight
		));
		if (i < data.size()) temp->Load(m_tickers[i], m_columns, false, &data[i], pPosition);
		else temp->Load(m_tickers[i], m_columns, false, nullptr, pPosition);
		m_items.push_back(temp);

		top += m_rowHeight;
		m_hLines[i] = top; // i.e. bottom of item i
	}
}

// Move item iOld to iNew, shifting everything in between appropriately.
// If iNew < iOld, moves iOld to above the current item at iNew.
// If iNew > iOld, moves iOld to below the current item at iNew.
// No bounds check. Make sure iOld and iNew are valid indices.
void Watchlist::MoveItem(int iOld, int iNew)
{
	if (iOld == iNew) return;
	WatchlistItem *temp = m_items[iOld];

	// need to shift stuff down
	if (iNew < iOld)
	{
		for (int j = iOld; j > iNew; j--) // j is new position of j-1
		{
			m_items[j] = m_items[j - 1];
			m_items[j]->SetSize(D2D1::RectF(
				m_dipRect.left,
				GetHeight(j),
				m_dipRect.right,
				GetHeight(j + 1)
			));
		}
	}
	// need to shift stuff up
	else
	{
		for (int j = iOld; j < iNew; j++) // j is new position of j+1
		{
			m_items[j] = m_items[j + 1];
			m_items[j]->SetSize(D2D1::RectF(
				m_dipRect.left,
				GetHeight(j),
				m_dipRect.right,
				GetHeight(j + 1)
			));
		}
	}

	m_items[iNew] = temp;
	m_items[iNew]->SetSize(D2D1::RectF(
		m_dipRect.left,
		GetHeight(iNew),
		m_dipRect.right,
		GetHeight(iNew + 1)
	));
}

void Watchlist::SortByColumn(size_t iColumn)
{
	if (iColumn >= m_columns.size()) throw std::invalid_argument("SortByColumn invalid column");
	
	auto end = m_items.end();
	if (m_items.back()->Ticker().empty()) end--; // don't sort empty item

	if (iColumn == m_sortColumn) // sort descending
	{
		std::sort(m_items.begin(), end, [iColumn](WatchlistItem const * i1, WatchlistItem const * i2)
		{
			if (iColumn == 0)
				return i1->Ticker() > i2->Ticker();
			else
				return i1->GetData(iColumn) > i2->GetData(iColumn);
		});
		m_sortColumn = -1;
	}
	else // sort ascending
	{
		std::sort(m_items.begin(), end, [iColumn](WatchlistItem const * i1, WatchlistItem const * i2)
		{ 
			if (iColumn == 0)
				return i1->Ticker() < i2->Ticker();
			else
				return i1->GetData(iColumn) < i2->GetData(iColumn); 
		});
		m_sortColumn = iColumn;
	}

	// Reset position of everything
	float top = m_dipRect.top + m_headerHeight;
	for (size_t i = 0; i < m_items.size(); i++)
	{
		m_items[i]->SetSize(D2D1::RectF(
			m_dipRect.left,
			top,
			m_dipRect.right,
			top + m_rowHeight
		));
		top += m_rowHeight;
	}
}

///////////////////////////////////////////////////////////
// --- WatchlistItem ---
///////////////////////////////////////////////////////////


void WatchlistItem::SetSize(D2D1_RECT_F dipRect)
{
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	D2D1_RECT_F temp = m_ticker.GetDIPRect();
	temp.top = m_dipRect.top;
	temp.bottom = m_dipRect.bottom;
	m_ticker.SetSize(temp);
}

void WatchlistItem::Paint(D2D1_RECT_F updateRect)
{
	m_ticker.Paint(updateRect);

	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	for (size_t i = 0; i < m_data.size(); i++)
	{
		m_d2.pRenderTarget->DrawTextLayout(
			D2D1::Point2F(m_origins[i], m_dipRect.top),
			m_pTextLayouts[i],
			m_d2.pBrush
		);
	}
}

inline void WatchlistItem::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (m_editableTickers) m_ticker.OnLButtonUp(cursor, wParam);
	if (m_LButtonDown)
	{
		m_LButtonDown = false;
		if (inRect(cursor, m_dipRect) && cursor.x > m_columns.front().width + m_dipRect.left
			&& !m_ticker.String().empty())
		{
			m_parent->PostClientMessage(this, m_ticker.String(), CTPMessage::WATCHLIST_SELECTED);
		}
	}
	//ProcessCTPMessages(); // not needed
}

void WatchlistItem::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::TEXTBOX_DEACTIVATED:
		case CTPMessage::TEXTBOX_ENTER:
			if (msg.msg != m_currTicker)
			{
				if (m_currTicker.empty()) m_parent->PostClientMessage(this, L"", CTPMessage::WATCHLISTITEM_NEW);
				else if (msg.msg.empty()) m_parent->PostClientMessage(this, L"", CTPMessage::WATCHLISTITEM_EMPTY);
				Load(msg.msg, m_columns, true);
				::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			}
			break;
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}


// If reload, don't initialize text box again
void WatchlistItem::Load(std::wstring const & ticker, std::vector<Column> const & columns, 
	bool reload, std::pair<Quote, Stats>* data, Position const * position)
{
	if (columns.empty() || columns.front().field != Column::Ticker)
	{
		OutputMessage(L"Misformed column vector: WatchlistItem Load()\n");
		return;
	}

	if (!reload)
		m_columns = columns;
	m_currTicker = ticker;
	LoadData(ticker, data, position);

	float left = m_dipRect.left;
	if (!reload)
	{
		// Initialize text box
		m_ticker.SetSize(D2D1::RectF(
			left,
			m_dipRect.top,
			left + columns[0].width,
			m_dipRect.bottom
		));
		m_ticker.SetParameters(Watchlist::m_hTextPad, false, 7);
	}
	m_ticker.SetText(ticker);
	left += columns[0].width;

	for (auto item : m_pTextLayouts) SafeRelease(&item);
	m_pTextLayouts.resize(m_data.size());
	m_origins.resize(m_data.size());

	for (size_t i = 0; i < m_data.size(); i++)
	{
		// Shift index into columns by 1 to skip Ticker
		if (left + columns[i+1].width >= m_dipRect.right)
		{
			throw std::invalid_argument("Columns exceed WatchlistItem width"); // exception so no clean up necessary
		}

		m_origins[i] = left + Watchlist::m_hTextPad;

		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_data[i].second.c_str(),				// The string to be laid out and formatted.
			m_data[i].second.size(),				// The length of the string.
			m_d2.pTextFormats[Watchlist::m_format],	// The text format to apply to the string (contains font information, etc).
			columns[i+1].width - 2 * Watchlist::m_hTextPad, // The width of the layout box.
			m_dipRect.bottom - m_dipRect.top,		// The height of the layout box.
			&m_pTextLayouts[i]						// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) throw Error(L"CreateTextLayout failed");

		left += columns[i+1].width;
	}
}

void WatchlistItem::LoadData(std::wstring const & ticker, std::pair<Quote, Stats> const * data, Position const * position)
{
	if (ticker.empty())
	{
		m_data.clear();
		return;
	}

	// m_columns[0] == Ticker
	m_data.resize(m_columns.size() - 1);
	try
	{
		std::pair<Quote, Stats> qs;
		if (!data) qs = GetQuoteStats(ticker);
		else qs = *data;
		Quote & quote = qs.first;
		Stats & stats = qs.second;

		wchar_t buffer[50] = {};
		for (size_t i = 1; i < m_columns.size(); i++)
		{
			double data = 0; // handle non-double types separately
			switch (m_columns[i].field)
			{
			case Column::Last:
				data = quote.latestPrice;
				break;
			case Column::ChangeP:
				data = quote.changePercent * 100;
				break;
			case Column::Change1YP:
				data = stats.year1ChangePercent * 100;
				break;
			case Column::DivP:
				data = stats.dividendYield;
				break;
			case Column::ExDiv:
				wcscpy_s(buffer, _countof(buffer), DateToWString(stats.exDividendDate).c_str());
				m_data[i - 1] = { stats.exDividendDate, buffer };
				continue;
			case Column::Shares:
				if (position)
				{
					swprintf_s(buffer, _countof(buffer), m_columns[i].format.c_str(), position->n);
					m_data[i - 1] = { position->n, buffer };
				}
				else
					m_data[i - 1] = { 0.0, L"" };
				continue;
			case Column::AvgCost:
				if (position) data = position->avgCost;
				break;
			case Column::Realized:
				if (position) data = position->realized_held + position->realized_unheld;
				break;
			case Column::Unrealized:
				if (position) data = position->unrealized;
				break;
			case Column::ReturnsT:
				if (position)
					data = position->realized_held + position->realized_unheld + position->unrealized;
				break;
			case Column::ReturnsP:
				if (position && position->n != 0) 
					data = (position->realized_held + position->unrealized) / (position->avgCost * position->n) * 100.0f;
				break;
			case Column::APY:
				if (position) data = position->APY * 100.0f;
				break;
			case Column::None:
			default:
				m_data[i - 1] = { 0.0, L"" };
				continue;
			}
			swprintf_s(buffer, _countof(buffer), m_columns[i].format.c_str(), data);
			m_data[i - 1] = { data, buffer };
		}
	}
	catch (const std::invalid_argument& ia) {
		OutputDebugString((ticker + L": ").c_str());
		OutputDebugStringA(ia.what()); OutputDebugStringA("\n");
		m_data.clear();
	}
}


