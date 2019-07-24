#include "../stdafx.h"
#include "Watchlist.h"
#include "TitleBar.h"
#include "../Utilities/utilities.h"
#include "../Utilities/DataManaging.h"
#include "../Windows/Parthenos.h"


void Watchlist::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		if (ProcessTableCTP(msg)) continue;

		switch (msg.imsg)
		{
		case CTPMessage::WATCHLISTITEM_NEW:
		{
			WatchlistItem *temp = new WatchlistItem(m_hwnd, m_d2, this);
			temp->Load(L"", m_columns);
			m_items.push_back(temp);
			CalculatePositions();
			break;
		}
		case CTPMessage::WATCHLISTITEM_EMPTY:
		{
			// Move empty watchlist item to end then delete it
			auto it = std::find(m_items.begin(), m_items.end(), msg.sender);
			if (it == m_items.end() || it + 1 == m_items.end()) break; // don't delete already empty item at end
			DeleteItem(it - m_items.begin());
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
	SetColumns({
			{90.0f, WatchlistColumn::Ticker, L""},
			{60.0f, WatchlistColumn::Last, L"%.2lf"},
			{60.0f, WatchlistColumn::ChangeP, L"%.2lf"},
			{60.0f, WatchlistColumn::Change1YP, L"%.2lf"},
			{60.0f, WatchlistColumn::EarningsDate, L""}
	});
}

void Watchlist::SetColumns(std::vector<WatchlistColumn> const & columns)
{
	if (columns.empty()) SetColumns();
	else
	{
		m_columns = columns;
		if (m_columns.front().field != WatchlistColumn::Ticker)
			m_columns.insert(m_columns.begin(), { 60.0f, WatchlistColumn::Ticker, L"" });
	}

	std::vector<std::wstring> names;
	std::vector<float> widths;
	std::vector<ColumnType> types;
	for (WatchlistColumn const & col : m_columns)
	{
		names.push_back(col.Name());
		widths.push_back(col.width);
		types.push_back(col.Type());
	}
	Table::SetColumns(names, widths, types);
}

// Get data via batch request
void Watchlist::Load(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions)
{
	QStats qstats;
	try {
		qstats = GetBatchQuoteStats(tickers);
	}
	catch (const std::invalid_argument& ia) {
		OutputDebugStringA(ia.what()); OutputDebugStringA("\n");
	}
	Load(tickers, positions, qstats);
}

// Clears and creates m_items
void Watchlist::Load(std::vector<std::wstring> const & tickers, std::vector<Position> const & positions,
	std::vector<std::pair<Quote, Stats>> const & data)
{
	if (tickers.size() != data.size()) throw ws_exception(L"Watchlist::Load() tickers and data don't line up");

	m_positions = positions;

	std::vector<std::wstring> _tickers = tickers;
	if (m_editableTickers) _tickers.push_back(L""); // Empty WatchlistItem at end for input

	for (auto x : m_items) if (x) delete x;
	m_items.clear();
	m_items.reserve(_tickers.size());
	for (size_t i = 0; i < _tickers.size(); i++)
	{
		WatchlistItem *temp = new WatchlistItem(m_hwnd, m_d2, this, m_editableTickers);
		if (m_editableTickers && i == _tickers.size() - 1)
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



///////////////////////////////////////////////////////////
// --- WatchlistItem ---
///////////////////////////////////////////////////////////

WatchlistItem::WatchlistItem(HWND hwnd, D2Objects const & d2, Watchlist *parent, bool editable) :
	TableRowItem(hwnd, d2, parent), m_parent(parent), m_ticker(hwnd, d2, this), m_editableTickers(editable)
{
	m_ticker.SetParameters(Watchlist::m_hTextPad, false, 7);
}

void WatchlistItem::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(dipRect, m_dipRect)) return;
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);
	CreateTextLayouts();
}

void WatchlistItem::Paint(D2D1_RECT_F updateRect)
{
	m_ticker.Paint(updateRect);

	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	for (size_t i = 0; i < m_data.size(); i++)
	{
		m_d2.pD2DContext->DrawTextLayout(
			D2D1::Point2F(m_origins[i], m_dipRect.top),
			m_pTextLayouts[i],
			m_d2.pBrush
		);
	}
}

inline bool WatchlistItem::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	if (m_editableTickers) return m_ticker.OnMouseMove(cursor, wParam, handeled);
	return false;
}

bool WatchlistItem::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	if (m_editableTickers) handeled = m_ticker.OnLButtonDown(cursor, handeled) || handeled;
	if (!handeled && inRect(cursor, m_dipRect)) m_LButtonDown = true;

	ProcessCTPMessages();
	return handeled;
}

inline void WatchlistItem::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (m_editableTickers) m_ticker.OnLButtonDblclk(cursor, wParam);
}

void WatchlistItem::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
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

inline bool WatchlistItem::OnChar(wchar_t c, LPARAM lParam)
{
	if (m_editableTickers) return m_ticker.OnChar(c, lParam);
	return false;
}

bool WatchlistItem::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = false;
	if (m_editableTickers) out = m_ticker.OnKeyDown(wParam, lParam);
	ProcessCTPMessages();
	return out;
}

inline void WatchlistItem::OnTimer(WPARAM wParam, LPARAM lParam)
{
	if (m_editableTickers) m_ticker.OnTimer(wParam, lParam);
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
				CreateTextLayouts();
				::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			}
			break;
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

double WatchlistItem::GetData(size_t iColumn) const
{
	if (iColumn == 0) return 0.0; // Ticker
	else if (iColumn - 1 < m_data.size()) return m_data[iColumn - 1].first;
	else return -std::numeric_limits<double>::infinity();
}

std::wstring WatchlistItem::GetString(size_t iColumn) const
{
	if (iColumn == 0) return m_ticker.String();
	else if (iColumn - 1 < m_data.size()) return m_data[iColumn - 1].second;
	else return L"";
}

// Copies ticker, columns to member variables and populates 'm_data' with the data and formatted strings
// Fetches data from IEX if 'data' == nullptr. Set 'reload' to not change columns.
void WatchlistItem::Load(std::wstring const & ticker, std::vector<WatchlistColumn> const & columns,
	std::pair<Quote, Stats> const * data, Position const * position, bool reload)
{
	m_currTicker = ticker;
	if (!reload) {
		if (columns.empty() || columns.front().field != WatchlistColumn::Ticker)
			return OutputMessage(L"Misformed column vector: WatchlistItem Load()\n");
		m_columns = columns;
	}

	if (ticker.empty())
	{
		m_data.clear();
		return;
	}

	m_data.resize(m_columns.size() - 1); // m_columns[0] == Ticker
	try
	{
		std::pair<Quote, Stats> qs;
		if (!data) qs = GetQuoteStats(ticker);
		else qs = *data;
		Quote & quote = qs.first;
		Stats & stats = qs.second;

		wchar_t buffer[200] = {};
		for (size_t i = 1; i < m_columns.size(); i++)
		{
			double data = 0; // handle non-double types separately
			switch (m_columns[i].field)
			{
			case WatchlistColumn::Last:
				data = quote.latestPrice;
				break;
			case WatchlistColumn::ChangeP:
				data = quote.changePercent * 100;
				break;
			case WatchlistColumn::Change1YP:
				data = stats.year1ChangePercent * 100;
				break;
			case WatchlistColumn::DivP:
				data = stats.dividendYield;
				break;
			case WatchlistColumn::ExDiv:
				wcscpy_s(buffer, _countof(buffer), DateToWString(stats.exDividendDate).c_str());
				m_data[i - 1] = { stats.exDividendDate, buffer };
				continue;
			case WatchlistColumn::Shares:
				if (position)
				{
					swprintf_s(buffer, _countof(buffer), m_columns[i].format.c_str(), position->n);
					m_data[i - 1] = { position->n, buffer };
				}
				else
					m_data[i - 1] = { 0.0, L"" };
				continue;
			case WatchlistColumn::AvgCost:
				if (position) data = position->avgCost;
				break;
			case WatchlistColumn::Equity:
				if (position) data = quote.latestPrice * position->n;
				break;
			case WatchlistColumn::Realized:
				if (position) data = position->realized_held + position->realized_unheld;
				break;
			case WatchlistColumn::Unrealized:
				if (position) data = position->unrealized;
				break;
			case WatchlistColumn::ReturnsT:
				if (position)
					data = position->realized_held + position->realized_unheld + position->unrealized;
				break;
			case WatchlistColumn::ReturnsP:
				if (position && (position->n > 0 || position->cash_collateral > 0))
					data = (position->realized_held + position->unrealized) / 
						(position->avgCost * position->n + position->cash_collateral) * 100.0f;
				break;
			case WatchlistColumn::APY:
				if (position) data = position->APY * 100.0f;
				break;
			case WatchlistColumn::EarningsDate:
				wcscpy_s(buffer, _countof(buffer), DateToWString(stats.nextEarningsDate).c_str());
				m_data[i - 1] = { stats.nextEarningsDate, buffer };
				continue;
			case WatchlistColumn::None:
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

void WatchlistItem::CreateTextLayouts()
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

	// Column text
	for (auto item : m_pTextLayouts) SafeRelease(&item);
	m_pTextLayouts.resize(m_data.size());
	m_origins.resize(m_data.size());
	for (size_t i = 0; i < m_data.size(); i++)
	{
		// Origins
		m_origins[i] = left + Watchlist::m_hTextPad;
		left += m_columns[i + 1].width; // Shift index into columns by 1 to skip Ticker

		// Layouts
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_data[i].second.c_str(),				// The string to be laid out and formatted.
			m_data[i].second.size(),				// The length of the string.
			m_d2.pTextFormats[Watchlist::m_format],	// The text format to apply to the string (contains font information, etc).
			m_columns[i+1].width - 2 * Watchlist::m_hTextPad, // The width of the layout box.
			m_dipRect.bottom - m_dipRect.top,		// The height of the layout box.
			&m_pTextLayouts[i]						// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) throw Error(L"CreateTextLayout failed");
	}
}




