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
}

void PopupMenu::Paint(D2D1_RECT_F updateRect)
{
}

void PopupMenu::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam)
{
}

bool PopupMenu::OnLButtonDown(D2D1_POINT_2F cursor)
{
	return false;
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

	m_width = max(200.0f, 100.0f * ceil((14.0f * max_size + 50.0f) / 100.0f));

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
}
