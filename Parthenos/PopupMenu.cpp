#include "stdafx.h"
#include "PopupMenu.h"

PopupMenu::~PopupMenu()
{
	for (auto x : m_pTextLayouts)
	{
		SafeRelease(&x);
	}
}

void PopupMenu::SetSize(D2D1_RECT_F dipRect)
{
	m_dipRect = dipRect;
	m_dipRect.right = m_dipRect.left + m_width;
	m_dipRect.bottom = m_dipRect.top + m_height;

	m_pixRect = DPIScale::DipsToPixels(m_dipRect);
	m_borderRect = DPIScale::SnapToPixel(m_dipRect, true);	
}

void PopupMenu::Paint(D2D1_RECT_F updateRect)
{
	if (!m_active) return;
	// if (!overlapRect(m_dipRect, updateRect)) return;
	// If whatever the popupmenu is overlapping repaints, then it will cover the popup.
	
	// Fill
	m_d2.pBrush->SetColor(Colors::MENU_BACKGROUND);
	m_d2.pD2DContext->FillRectangle(m_dipRect, m_d2.pBrush);
	
	// Highlight
	if (m_highlight >= 0)
	{
		m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
		m_d2.pD2DContext->FillRectangle(D2D1::RectF(
			m_dipRect.left,
			m_dipRect.top + getTop(m_highlight),
			m_dipRect.right,
			m_dipRect.top + getTop(m_highlight + 1)
		), m_d2.pBrush);
	}

	// Text
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	for (size_t i = 0; i < m_pTextLayouts.size(); i++)
	{
		m_d2.pD2DContext->DrawTextLayout(
			D2D1::Point2F(m_dipRect.left + 4.0f, m_dipRect.top + getTop(i) + m_vPad),
			m_pTextLayouts[i],
			m_d2.pBrush
		);
	}

	// Divisions
	m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	for (float y : m_divisionYs)
	{
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(m_dipRect.left + 4.0f, m_dipRect.top + y),
			D2D1::Point2F(m_dipRect.right - 10.0f, m_dipRect.top + y),
			m_d2.pBrush,
			1.0f,
			m_d2.pHairlineStyle
		);
	}

	// Border
	m_d2.pD2DContext->DrawRectangle(m_borderRect, m_d2.pBrush, 1.0f, m_d2.pHairlineStyle);
}

bool PopupMenu::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	if (!m_active) return false;
	if (inRect(cursor, m_dipRect) && !handeled)
	{
		int highlight = getInd(cursor.y - m_dipRect.top);
		if (highlight != m_highlight)
		{
			m_highlight = highlight;
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
		return true;
	}
	else if (m_highlight >= 0)
	{
		m_highlight = -1;
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		return false;
	}
	return false;
}

// returns true when a selection is made and i, str are populated
// returns true only if item selected; no other selectable space
bool PopupMenu::OnLButtonDown(D2D1_POINT_2F cursor, int & selection, std::wstring & str)
{
	if (!m_active) return false;

	if (inRect(cursor, m_dipRect))
	{
		selection = getInd(cursor.y - m_dipRect.top);
		str = m_items[selection];
		return true;
	}
	else
	{
		// Do not hide! Owner should do that
		return false;
	}
}

void PopupMenu::Show(bool show)
{
	m_active = show;
	m_highlight = -1;
	::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}

// Creates the text layout and calculates the width and height of the menu
void PopupMenu::SetItems(std::vector<std::wstring> const & items)
{
	if (m_items.size() > 0) return; // only set once

	m_items = items;
	for (auto x : m_pTextLayouts) SafeRelease(&x);
	m_pTextLayouts.resize(items.size());

	for (size_t i = 0; i < items.size(); i++)
	{
		// TODO instead of 500.0f for the width, do another loop after calculating widths
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			items[i].c_str(),		// The string to be laid out and formatted.
			items[i].size(),		// The length of the string.
			m_d2.pTextFormats[m_format],   // The text format to apply to the string (contains font information, etc).
			500.0f,					// The width of the layout box.
			m_fontSize,				// The height of the layout box.
			&m_pTextLayouts[i]		// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) throw Error(L"CreateTextLayout failed");
	}

	float max_width = 0;
	DWRITE_TEXT_METRICS metrics;
	for (auto layout : m_pTextLayouts)
	{
		layout->GetMetrics(&metrics);
		if (metrics.width > max_width) max_width = metrics.width;
	}

	m_width = 50.0f * ceil((max_width + 20.0f) / 50.0f); // 20 DIP pad
	m_height = getTop(items.size());
}

void PopupMenu::SetDivisions(std::vector<size_t> const & divs)
{
	m_divisionYs.clear();
	for (size_t div : divs)
	{
		if (div < m_items.size()) 
			m_divisionYs.push_back(DPIScale::SnapToPixelY(getTop(div)) - DPIScale::hpy());
	}
}



