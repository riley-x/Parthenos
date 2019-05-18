#include "stdafx.h"
#include "Table.h"

float const Table::m_hTextPad = 4.0f;

Table::~Table()
{
	for (auto item : m_items) if (item) delete item;
	for (auto layout : m_columnLayouts) SafeRelease(&layout);
}

void Table::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(m_dipRect, dipRect)) return;

	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	m_headerBorder = DPIScale::SnapToPixelY(m_dipRect.top + m_headerHeight) - DPIScale::hpy();
	m_visibleLines = static_cast<size_t>(floorf((m_dipRect.bottom - m_headerBorder) / m_rowHeight));

	Refresh(); // also sets number of lines for scrollbar

	// scroll bar auto sets left
	m_scrollBar.SetSize(D2D1::RectF(0.0f, m_headerBorder + 1, m_dipRect.right, m_dipRect.bottom));
}

void Table::Paint(D2D1_RECT_F updateRect)
{
	if (!overlapRect(m_dipRect, updateRect)) return;

	// Scrollbar only
	if (updateRect.left + 1 >= m_dipRect.right - ScrollBar::Width) return m_scrollBar.Paint(updateRect);

	// Background
	m_d2.pBrush->SetColor(Colors::WATCH_BACKGROUND);
	m_d2.pD2DContext->FillRectangle(m_dipRect, m_d2.pBrush);

	// Column headers
	float left = m_dipRect.left;
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	for (size_t i = 0; i < m_columnLayouts.size(); i++)
	{
		m_d2.pD2DContext->DrawTextLayout(
			D2D1::Point2F(left + m_hTextPad, m_dipRect.top),
			m_columnLayouts[i],
			m_d2.pBrush
		);
		left += m_columnWidths[i];
		if (left >= m_dipRect.right) break;
	}

	// Item texts
	for (size_t i = m_currLine; i < m_items.size() && i < m_currLine + m_visibleLines; i++)
		m_items[i]->Paint(m_dipRect); // pass watchlist rect as updateRect to force repaint

	// Internal lines
	m_d2.pBrush->SetColor(Colors::DULL_LINE);
	for (float line : m_vLines)
	{
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(line, m_dipRect.top),
			D2D1::Point2F(line, m_dipRect.bottom),
			m_d2.pBrush,
			0.5
		);
	}
	for (float line : m_hLines)
	{
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(m_dipRect.left, line),
			D2D1::Point2F(m_dipRect.right, line),
			m_d2.pBrush,
			0.5
		);
	}

	// Scrollbar
	m_scrollBar.Paint(updateRect);

	// Header dividing line
	m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	m_d2.pD2DContext->DrawLine(
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
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(m_dipRect.left, y),
			D2D1::Point2F(m_dipRect.right - ScrollBar::Width, y),
			m_d2.pBrush,
			DPIScale::py()
		);
	}
}

bool Table::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
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
			if (IsVisible(i) || (!m_emptyEnd && IsVisible(i-1)))
				m_hover = i;
			else
				m_hover = -2; // don't set to -1 to avoid confusion with init
			::InvalidateRect(m_hwnd, &m_pixRect, false);
		}
		handeled = true;
	}

	handeled = m_scrollBar.OnMouseMove(cursor, wParam, handeled) || handeled;
	for (size_t i = m_currLine; i < m_items.size() && i < m_currLine + m_visibleLines; i++)
		handeled = m_items[i]->OnMouseMove(cursor, wParam, handeled) || handeled;

	ProcessCTPMessages();
	return handeled;
}

bool Table::OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	if (!handeled && inRect(cursor, m_dipRect))
	{
		m_scrollBar.OnMouseWheel(cursor, wParam, handeled);
		ProcessCTPMessages();
		return true;
	}
	return false;
}

bool Table::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	handeled = m_scrollBar.OnLButtonDown(cursor, handeled) || handeled;
	for (size_t i = m_currLine; i < m_items.size() && i < m_currLine + m_visibleLines; i++)
	{
		handeled = m_items[i]->OnLButtonDown(cursor, handeled) || handeled;
	}

	if (!handeled && inRect(cursor, m_dipRect)) // Prepare flags for dragging -- but not dragging yet!
	{
		int i = GetItem(cursor.y);
		if (IsVisible(i))
		{
			if (m_emptyEnd && i == m_items.size() - 1) return false; // can't select empty item at end
			m_LButtonDown = i;
			m_hover = -1;
			m_ignoreSelection = false;
			return true;
		}
	}
	ProcessCTPMessages();
	return handeled;
}

void Table::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (!inRect(cursor, m_dipRect)) return;
	if (!m_scrollBar.OnLButtonDown(cursor, false))
	{
		int i = GetItem(cursor.y);
		if (IsVisible(i)) m_items[i]->OnLButtonDblclk(cursor, wParam);
		else if (i < (int)m_currLine) // double click on header
		{
			auto it = lower_bound(m_vLines.begin(), m_vLines.end(), cursor.x);
			if (it == m_vLines.end()) return;
			size_t column = it - m_vLines.begin();
			SortByColumn(column);
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
	}
	ProcessCTPMessages();
}

// If dragging an element, does the insertion above the drop location
void Table::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	m_scrollBar.OnLButtonUp(cursor, wParam);
	for (size_t i = m_currLine; i < m_items.size() && i < m_currLine + m_visibleLines; i++)
		m_items[i]->OnLButtonUp(cursor, wParam);

	if (m_LButtonDown >= 0)	// check for drag + drop
	{
		int iNew = GetItem(cursor.y);
		bool draw = false;

		if (IsVisible(iNew) || (!m_emptyEnd && IsVisible(iNew - 1)))
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

bool Table::OnChar(wchar_t c, LPARAM lParam)
{
	for (size_t i = m_currLine; i < m_items.size() && i < m_currLine + m_visibleLines; i++)
	{
		if (m_items[i]->OnChar(c, lParam)) return true;
	}
	//ProcessCTPMessages();
	return false;
}

bool Table::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = false;
	for (size_t i = m_currLine; i < m_items.size() && i < m_currLine + m_visibleLines; i++)
	{
		if (m_items[i]->OnKeyDown(wParam, lParam)) out = true;
	}
	ProcessCTPMessages();
	return out;
}

void Table::OnTimer(WPARAM wParam, LPARAM lParam)
{
	for (size_t i = m_currLine; i < m_items.size() && i < m_currLine + m_visibleLines; i++)
		m_items[i]->OnTimer(wParam, lParam);
	//ProcessCTPMessages();
}

void Table::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		ProcessTableCTP(msg);
	}
	if (!m_messages.empty()) m_messages.clear();
}

void Table::ProcessTableCTP(ClientMessage const & msg)
{
	switch (msg.imsg)
	{
	case CTPMessage::SCROLLBAR_SCROLL:
	{
		m_currLine += msg.iData;
		if (m_hover >= 0) m_hover += msg.iData;
		if (!IsVisible(m_hover)) m_hover = -2;
		CalculatePositions();
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		break;
	}
	case CTPMessage::MOUSE_CAPTURED: // from scrollbox
		m_parent->PostClientMessage(this, msg.msg, msg.imsg, msg.iData, msg.dData);
		// change sender to this to process scrollbox messages
		break;
	}
}

void Table::SetColumns(std::vector<std::wstring> const & names, std::vector<float> const & widths)
{
	if (names.size() != widths.size()) throw ws_exception(L"Table::SetColumns size mismatch");
	m_columnHeaders = names;
	m_columnWidths = widths;
}

void Table::Load(std::vector<TableRowItem*> const & items)
{
	for (auto x : m_items) if (x) delete x;
	m_items = items;
}

// Set size of TableRowItems, horizontal divider lines
void Table::CalculatePositions()
{
	m_currLine = m_scrollBar.SetSteps(m_items.size(), m_visibleLines, ScrollBar::SetPosMethod::MaintainOffsetTop);

	m_hLines.clear();
	float top = m_dipRect.top + m_headerHeight;
	for (size_t i = m_currLine; i < m_items.size() && i < m_currLine + m_visibleLines; i++)
	{
		m_items[i]->SetSize(D2D1::RectF(
			m_dipRect.left,
			top,
			m_dipRect.right,
			top + m_rowHeight
		));

		top += m_rowHeight;
		m_hLines.push_back(top); // i.e. bottom of item i
	}
}

// Create column headers, vertical divider lines
void Table::CreateTextLayouts()
{
	float left = m_dipRect.left;
	m_vLines.resize(m_columnHeaders.size());
	for (auto x : m_columnLayouts) SafeRelease(&x);
	m_columnLayouts.resize(m_columnHeaders.size());

	for (size_t i = 0; i < m_columnHeaders.size(); i++)
	{
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_columnHeaders[i].c_str(),		// The string to be laid out and formatted.
			m_columnHeaders[i].size(),		// The length of the string.
			m_d2.pTextFormats[m_format],	// The text format to apply to the string (contains font information, etc).
			m_columnWidths[i] - 2 * m_hTextPad, // The width of the layout box.
			m_headerHeight,					// The height of the layout box.
			&m_columnLayouts[i]				// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) throw Error(L"CreateTextLayout failed");

		left += m_columnWidths[i];
		m_vLines[i] = left; // i.e. right sisde of column i
	}
}

// Move item iOld to iNew, shifting everything in between appropriately.
// If iNew < iOld, moves iOld to above the current item at iNew.
// If iNew > iOld, moves iOld to below the current item at iNew.
// No bounds check. Make sure iOld and iNew are valid indices.
void Table::MoveItem(int iOld, int iNew)
{
	if (iOld == iNew) return;
	TableRowItem *temp = m_items[iOld];

	if (iNew < iOld) // need to shift stuff down
	{
		for (int j = iOld; j > iNew; j--) // j is new position of j-1
			m_items[j] = m_items[j - 1];
	}
	else // need to shift stuff up
	{
		for (int j = iOld; j < iNew; j++) // j is new position of j+1
			m_items[j] = m_items[j + 1];
	}

	m_items[iNew] = temp;

	CalculatePositions();
}

void Table::SortByColumn(size_t iColumn)
{
	if (iColumn >= m_columnHeaders.size()) throw std::invalid_argument("SortByColumn invalid column");

	auto end = m_items.end();
	if (m_emptyEnd) end--; // don't sort empty item

	if (iColumn == m_sortColumn) // sort descending
	{
		std::sort(m_items.begin(), end, [iColumn](TableRowItem const * i1, TableRowItem const * i2)
		{
			return i1->GetData(iColumn) > i2->GetData(iColumn);
		});
		m_sortColumn = -1;
	}
	else // sort ascending
	{
		std::sort(m_items.begin(), end, [iColumn](TableRowItem const * i1, TableRowItem const * i2)
		{
			return i1->GetData(iColumn) < i2->GetData(iColumn);
		});
		m_sortColumn = iColumn;
	}

	// Reset position of everything
	CalculatePositions();
}


