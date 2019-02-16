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
}

void PopupMenu::Paint(D2D1_RECT_F updateRect)
{
	if (!m_active) return;
	
	m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
	m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f);
	m_d2.pBrush->SetColor(Colors::MAIN_BACKGROUND);
	m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);
	
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	for (size_t i = 0; i < m_pTextLayouts.size(); i++)
	{
		m_d2.pRenderTarget->DrawTextLayout(
			D2D1::Point2F(m_dipRect.left + 1.0f, m_dipRect.top + i * (14.0f + m_vPad)),
			m_pTextLayouts[i],
			m_d2.pBrush
		);
	}
}

void PopupMenu::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam)
{
}

// returns true when a selection is made and i, str are populated
bool PopupMenu::OnLButtonDown(D2D1_POINT_2F cursor, int & i, std::wstring & str)
{
	return false;
	// Do not hide! Owner should do that
}

void PopupMenu::Show(bool show)
{
	m_active = show;
	::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}

void PopupMenu::SetItems(std::vector<std::wstring> const & items)
{
	if (m_items.size() > 0) return; // only set once

	m_items = items;
	m_pTextLayouts.resize(items.size());

	size_t max_size = 0;
	for (auto item : items)
	{
		if (item.size() > max_size) max_size = item.size();
	}

	m_width = max(50.0f, 50.0f * ceil(max_size * 18.0f / 50.0f));
	m_height = items.size() * (14.0f + m_vPad);

	for (size_t i = 0; i < items.size(); i++)
	{
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			items[i].c_str(),		// The string to be laid out and formatted.
			items[i].size(),		// The length of the string.
			m_d2.pTextFormat_14p,   // The text format to apply to the string (contains font information, etc).
			m_width,				// The width of the layout box.
			14.0f,					// The height of the layout box.
			&m_pTextLayouts[i]		// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) Error(L"CreateTextLayout failed");
	}

	//if (!m_pTextLayouts.empty())
	//{
	//	DWRITE_TEXT_METRICS metrics;
	//	HRESULT hr = m_pTextLayouts[0]->GetMetrics(&metrics);
	//}
}
