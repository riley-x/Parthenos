#include "stdafx.h"
#include "Button.h"

bool Button::OnLButtonDown(D2D1_POINT_2F cursor)
{
	if (inRect(cursor, m_dipRect))
	{
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
		m_d2.pRenderTarget->DrawBitmap(
			m_d2.pD2DBitmaps[m_iBitmap],
			m_dipRect
		);
	}
	else
	{
		AppItem::Paint(updateRect);
	}
}

bool IconButton::OnLButtonDown(D2D1_POINT_2F cursor)
{
	if (cursor.x >= m_clickRect.left &&
		cursor.x <= m_clickRect.right &&
		cursor.y >= m_clickRect.top &&
		cursor.y <= m_clickRect.bottom)
	{
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////
// --- DropMenuButton ---

void DropMenuButton::SetSize(D2D1_RECT_F dipRect)
{
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	// text
	float pad = (m_dipRect.bottom - m_dipRect.top - m_menu.m_fontSize) / 2.0f;
	m_textRect = D2D1::RectF(
		m_dipRect.left + 2.0f,
		m_dipRect.top + pad,
		m_dipRect.right - 2.0f,
		m_dipRect.bottom - pad
	);

	// icon
	pad = (m_dipRect.bottom - m_dipRect.top - 8.0f) / 2.0f;
	m_iconRect = D2D1::RectF(
		m_dipRect.right - 2.0f - 8.0f,
		m_dipRect.top + pad,
		m_dipRect.right - 2.0f,
		m_dipRect.bottom - pad
	);

	// menu
	m_menu.SetSize(D2D1::RectF(
		dipRect.left,
		dipRect.bottom + 1.0f,
		0, 0 // menu calculates these
	));
}

// owner of button should call paint on popup
void DropMenuButton::Paint(D2D1_RECT_F updateRect)
{
	// Draw bounding box
	if (m_active)
		m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
	else
		m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f);

	// Text
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	m_d2.pRenderTarget->DrawTextLayout(
		D2D1::Point2F(m_textRect.left, m_textRect.top),
		m_activeLayout,
		m_d2.pBrush
	);

	// Arrow icon
	if (m_d2.pD2DBitmaps[GetResourceIndex(IDB_DOWNARROWHEAD)])
	{
		m_d2.pRenderTarget->DrawBitmap(
			m_d2.pD2DBitmaps[GetResourceIndex(IDB_DOWNARROWHEAD)],
			m_iconRect
		);
	}

	// owner of button should call paint on popup
}

bool DropMenuButton::OnLButtonDown(D2D1_POINT_2F cursor)
{

	if (m_active)
	{
		int i;
		std::wstring str;
		if (m_menu.OnLButtonDown(cursor, i, str)) // return true only if item selected; no other selectable space
		{
			SetActive(i);
			m_parent->ReceiveMessage(str, CTPMessage::DROPMENU_SELECTED);
		}
		m_active = false;
		m_menu.Show(false);
	}
	else if (inRect(cursor, m_dipRect))
	{
		m_active = true;
		m_menu.Show();
	}
	return false;
}

void DropMenuButton::SetActive(size_t i)
{
	m_activeLayout = m_menu.GetLayout(i);
}
