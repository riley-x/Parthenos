#include "../stdafx.h"
#include "Button.h"

///////////////////////////////////////////////////////////
// --- Button ---

bool Button::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	if (!handeled && inRect(cursor, m_dipRect))
	{
		if (m_parent) m_parent->PostClientMessage(this, m_name, CTPMessage::BUTTON_DOWN);
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////
// --- IconButton ---

void IconButton::Paint(D2D1_RECT_F updateRect)
{
	if (m_iBitmap != -1 && m_d2.pD2DBitmaps[m_iBitmap])
	{
		m_d2.pD2DContext->DrawBitmap(
			m_d2.pD2DBitmaps[m_iBitmap],
			m_dipRect
		);
	}
	else
	{
		AppItem::Paint(updateRect);
	}
}


///////////////////////////////////////////////////////////
// --- DropMenuButton ---

void DropMenuButton::SetSize(D2D1_RECT_F dipRect)
{
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	// Text
	if (m_dynamic) // text layout is tight so can't just use m_dipRect
	{
		float pad = (m_dipRect.bottom - m_dipRect.top - m_menu.m_fontSize) / 2.0f;
		m_textRect = D2D1::RectF(
			m_dipRect.left + 2.0f,
			m_dipRect.top + pad,
			0, // unused
			0  // unused
		);
	}
	else
	{
		m_textRect = m_dipRect;
	}

	// Icon
	if (m_dynamic)
	{
		float pad = (m_dipRect.bottom - m_dipRect.top - 8.0f) / 2.0f;
		m_iconRect = D2D1::RectF(
			m_dipRect.right - 2.0f - 8.0f,
			m_dipRect.top + pad,
			m_dipRect.right - 2.0f,
			m_dipRect.bottom - pad
		);
	}

	// Menu
	m_menu.SetSize(D2D1::RectF(
		dipRect.left,
		dipRect.bottom + 1.0f,
		0, 0 // menu calculates these
	));
}

// owner of button should call paint on popup
void DropMenuButton::Paint(D2D1_RECT_F updateRect)
{
	if (m_dynamic)
	{
		// Draw bounding box
		if (m_active)
			m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
		else
			m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
		m_d2.pD2DContext->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f);

		// Arrow icon
		if (m_d2.pD2DBitmaps[GetResourceIndex(IDB_DOWNARROWHEAD)])
		{
			m_d2.pD2DContext->DrawBitmap(
				m_d2.pD2DBitmaps[GetResourceIndex(IDB_DOWNARROWHEAD)],
				m_iconRect
			);
		}
	}

	// Text
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	m_d2.pD2DContext->DrawTextLayout(
		D2D1::Point2F(m_textRect.left, m_textRect.top),
		m_activeLayout,
		m_d2.pBrush
	);

	// owner of button should call paint on popup
}

bool DropMenuButton::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	bool out = false;
	if (m_active)
	{
		int i;
		std::wstring str;
		if (m_menu.OnLButtonDown(cursor, i, str)) // return true only if item selected; no other selectable space
		{
			if (m_dynamic) SetActive(i);
			m_parent->PostClientMessage(this, str, CTPMessage::DROPMENU_SELECTED);
			out = true;
		}
		// Deactivate on click, always
		m_active = false;
		m_menu.Show(false);
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	}
	else if (!handeled && inRect(cursor, m_dipRect))
	{
		m_active = true;
		m_menu.Show();
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		out = true;
	}
	return out;
}

void DropMenuButton::SetText(std::wstring text, float width, float height)
{
	if (m_dynamic) return; // Use SetActive instead

	m_name = text;
	m_d2.pTextFormats[m_menu.m_format]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	SafeRelease(&m_activeLayout);
	HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
		text.c_str(),		// The string to be laid out and formatted.
		text.size(),		// The length of the string.
		m_d2.pTextFormats[m_menu.m_format],   // The text format
		width,				// The width of the layout box.
		height,				// The height of the layout box.
		&m_activeLayout		// The IDWriteTextLayout interface pointer.
	);
	m_d2.pTextFormats[m_menu.m_format]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}

void DropMenuButton::SetActive(size_t i)
{
	if (m_dynamic)
	{
		m_activeSelection = i;
		m_activeLayout = m_menu.GetLayout(i);
		m_name = m_menu.GetItem(i);
	}
}

///////////////////////////////////////////////////////////
// --- TextButton ---

void TextButton::Paint(D2D1_RECT_F updateRect)
{
	// Fill
	if (m_fill)
	{
		m_d2.pBrush->SetColor(m_fillColor);
		m_d2.pD2DContext->FillRectangle(m_dipRect, m_d2.pBrush);
	}

	// Border
	if (m_border)
	{
		m_d2.pBrush->SetColor(m_borderColor);
		m_d2.pD2DContext->DrawRectangle(m_dipRect, m_d2.pBrush);
	}

	// Text
	m_d2.pTextFormats[m_format]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_d2.pBrush->SetColor(m_textColor);
	m_d2.pD2DContext->DrawText(
		m_name.c_str(),
		m_name.size(),
		m_d2.pTextFormats[m_format],
		m_dipRect,
		m_d2.pBrush
	);
	m_d2.pTextFormats[m_format]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}

