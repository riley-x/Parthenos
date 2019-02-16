#include "stdafx.h"
#include "Button.h"

bool Button::OnLButtonDown(D2D1_POINT_2F cursor)
{
	if (cursor.x >= m_dipRect.left &&
		cursor.x <= m_dipRect.right &&
		cursor.y >= m_dipRect.top &&
		cursor.y <= m_dipRect.bottom)
	{
		return true;
	}
	return false;
}

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

	m_menu.SetSize(D2D1::RectF(
		dipRect.left,
		dipRect.bottom,
		0, 0 // menu calculates these
	));
}

//void DropMenuButton::Paint(D2D1_RECT_F updateRect)
//{
//}

bool DropMenuButton::OnLButtonDown(D2D1_POINT_2F cursor)
{
	return false;
}

void DropMenuButton::SetActive(size_t i)
{
}
