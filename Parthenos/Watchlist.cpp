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

// TODO don't need to re-calculate layouts each set size, i.e. making the table larger?
void Watchlist::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(m_dipRect, dipRect)) return;
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	m_headerBorder = DPIScale::SnapToPixelY(m_dipRect.top + m_headerHeight) - DPIScale::hpy();

	CreateTextLayouts();
}

void Watchlist::Paint(D2D1_RECT_F updateRect)
{
	// When invalidating, converts to pixels then back to DIPs -> updateRect has smaller values
	// than when invalidated.
	if (!overlapRect(m_dipRect, updateRect)) return;

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
		item->Paint(m_dipRect); // pass watchlist rect as updateRect to force repaint

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

	// Header dividing line
	m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	m_d2.pRenderTarget->DrawLine(
		D2D1::Point2F(m_dipRect.left, m_headerBorder),
		D2D1::Point2F(m_dipRect.right, m_headerBorder),
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

bool Watchlist::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	int i = GetItem(cursor.y);
	if (m_LButtonDown >= 0 && (wParam & MK_LBUTTON)) // is dragging
	{
		if (m_hover == -1) // First time
		{
			m_ignoreSelection = true; // is dragging, so don't update even if dropped in same location
			m_parent->PostClientMessage(this, L"", CTPMessage::MOUSE_CAPTURED, 1);
		}
		if (i != m_hover)
		{
			if (i >= 0 && i < static_cast<int>(m_items.size()))
				m_hover = i;
			else
				m_hover = -2; // don't set to -1 to avoid confusion with init
			::InvalidateRect(m_hwnd, &m_pixRect, false);
		}
		handeled = true;
	}

	if (m_editableTickers)
	{
		for (auto item : m_items) handeled = item->OnMouseMove(cursor, wParam, handeled) || handeled;
	}

	return handeled;
	//ProcessCTPMessages();
}

bool Watchlist::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	for (auto item : m_items)
	{
		handeled = item->OnLButtonDown(cursor, handeled) || handeled;
	}
	if (!handeled && inRect(cursor, m_dipRect))
	{
		int i = GetItem(cursor.y);
		if (i >= 0 && i < static_cast<int>(m_items.size()))
		{
			if (i == m_items.size() - 1 && m_items.back()->Ticker().empty()) return false; // can't select empty item at end
			m_LButtonDown = i;
			// Reset flags for dragging -- not dragging yet
			m_hover = -1;
			m_ignoreSelection = false; 
			return true;
		}
	}
	ProcessCTPMessages();
	return handeled; // unused
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
		m_parent->PostClientMessage(this, L"", CTPMessage::MOUSE_CAPTURED, -1);
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

void Watchlist::SetColumns()
{
	m_columns = {
			{90.0f, Column::Ticker, L""},
			{60.0f, Column::Last, L"%.2lf"},
			{60.0f, Column::ChangeP, L"%.2lf"},
			{60.0f, Column::Change1YP, L"%.2lf"},
			{60.0f, Column::DivP, L"%.2lf"}
	};
}

void Watchlist::SetColumns(std::vector<Column> const & columns)
{
	if (columns.empty()) SetColumns();
	else
	{
		m_columns = columns;
		if (m_columns.front().field != Column::Ticker)
			m_columns.insert(m_columns.begin(), { 60.0f, Column::Ticker, L"" });
	}
}

// Get data via batch request
void Watchlist::Load(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions)
{
	Load(tickers, positions, GetBatchQuoteStats(tickers));
}

// Clears and creates m_items
void Watchlist::Load(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions,
	std::vector<std::pair<Quote, Stats>> const & data)
{
	if (tickers.size() != data.size())
		return OutputMessage(L"Watchlist tickers and data don't line up\n");

	m_positions = positions;

	std::vector<std::wstring> _tickers = tickers;
	_tickers.push_back(L""); // Empty WatchlistItem at end for input

	for (auto x : m_items) if (x) delete x;
	m_items.clear();
	m_items.reserve(_tickers.size());
	for (size_t i = 0; i < _tickers.size(); i++)
	{
		WatchlistItem *temp = new WatchlistItem(m_hwnd, m_d2, this, m_editableTickers);
		if (i == _tickers.size() - 1)
		{
			temp->Load(_tickers[i], m_columns, nullptr, nullptr);
		}
		else
		{
			// Get position if available
			Position const * pPosition = nullptr;
			auto it = std::lower_bound(m_positions.begin(), m_positions.end(), _tickers[i], Positions_Compare);
			if (it != m_positions.end()) pPosition = &(*it);
			temp->Load(_tickers[i], m_columns, &data[i], pPosition);
		}
		m_items.push_back(temp);
	}
}

void Watchlist::CreateTextLayouts()
{
	// Create column headers, vertical divider lines
	float left = m_dipRect.left;
	m_vLines.resize(m_columns.size());
	for (auto x : m_pTextLayouts) SafeRelease(&x);
	m_pTextLayouts.resize(m_columns.size());

	for (size_t i = 0; i < m_columns.size(); i++)
	{
		if (left + m_columns[i].width > m_dipRect.right)
		{
			OutputMessage(L"Warning: Columns exceed watchlist width\n");
			if (i > 0)
			{
				m_vLines.resize(i - 1);
				m_pTextLayouts.resize(i - 1);
				m_columns.resize(i - 1);
				for (auto item : m_items) item->TruncateColumns(i - 1);
			}
			else
			{
				m_vLines.clear();
				m_pTextLayouts.clear();
				m_columns.clear();
				for (auto item : m_items) item->TruncateColumns(0);
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

	// Set size of WatchlistItems, horizontal divider lines
	m_hLines.resize(m_items.size());
	float top = m_dipRect.top + m_headerHeight;
	for (size_t i = 0; i < m_items.size(); i++)
	{
		if (top + m_rowHeight > m_dipRect.bottom)
		{
			OutputMessage(L"Warning: Watchlist too many items");
			if (i > 0)
			{
				m_hLines.resize(i - 1);
				for (size_t j = i; j < m_items.size(); j++) if (m_items[j]) delete m_items[j];
				m_items.resize(i - 1);
			}
			else
			{
				m_vLines.clear();
				for (auto x : m_items) if (x) delete x;
				m_items.clear();
			}
			break;
		}

		m_items[i]->SetSize(D2D1::RectF(
			m_dipRect.left,
			top,
			m_dipRect.right,
			top + m_rowHeight
		));

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

WatchlistItem::WatchlistItem(HWND hwnd, D2Objects const & d2, AppItem *parent, bool editable) :
	AppItem(hwnd, d2), m_parent(parent), m_ticker(hwnd, d2, this), m_editableTickers(editable)
{
	m_ticker.SetParameters(Watchlist::m_hTextPad, false, 7);
}


void WatchlistItem::SetSize(D2D1_RECT_F dipRect)
{
	bool same_height = false;
	if (m_dipRect.bottom - m_dipRect.top == dipRect.bottom - dipRect.top) same_height = true;

	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	CreateTextLayouts(same_height);
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
				Load(msg.msg, m_columns, nullptr, nullptr, true);
				CreateTextLayouts(false); // force recreate
				::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			}
			break;
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

// Copies ticker, columns to member variables and populates 'm_data' with the data and formatted strings
// Fetches data from IEX if 'data' == nullptr. Set 'reload' to not change columns.
void WatchlistItem::Load(std::wstring const & ticker, std::vector<Column> const & columns,
	std::pair<Quote, Stats> const * data, Position const * position, bool reload)
{
	m_currTicker = ticker;
	if (!reload) {
		if (columns.empty() || columns.front().field != Column::Ticker)
			return OutputMessage(L"Misformed column vector: WatchlistItem Load()\n");
		m_columns = columns;
	}

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
			case Column::Equity:
				if (position) data = quote.latestPrice * position->n;
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

void WatchlistItem::TruncateColumns(size_t i)
{
	if (m_columns.size() > i) m_columns.resize(i);
	if (m_data.size() > i) m_data.resize(i);
}


// If same_height, don't recreate the layouts, just the textbox
void WatchlistItem::CreateTextLayouts(bool same_height)
{
	float left = m_dipRect.left;
	
	// Text box
	m_ticker.SetSize(D2D1::RectF(
		left,
		m_dipRect.top,
		left + m_columns[0].width,
		m_dipRect.bottom
	));
	m_ticker.SetText(m_currTicker);
	left += m_columns[0].width;

	if (same_height) return;

	for (auto item : m_pTextLayouts) SafeRelease(&item);
	m_pTextLayouts.resize(m_data.size());
	m_origins.resize(m_data.size());

	for (size_t i = 0; i < m_data.size(); i++)
	{
		m_origins[i] = left + Watchlist::m_hTextPad;

		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_data[i].second.c_str(),				// The string to be laid out and formatted.
			m_data[i].second.size(),				// The length of the string.
			m_d2.pTextFormats[Watchlist::m_format],	// The text format to apply to the string (contains font information, etc).
			m_columns[i+1].width - 2 * Watchlist::m_hTextPad, // The width of the layout box.
			m_dipRect.bottom - m_dipRect.top,		// The height of the layout box.
			&m_pTextLayouts[i]						// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) throw Error(L"CreateTextLayout failed");

		// Shift index into columns by 1 to skip Ticker
		left += m_columns[i+1].width;
	}
}




