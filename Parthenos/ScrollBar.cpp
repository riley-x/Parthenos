#include "stdafx.h"
#include "ScrollBar.h"

ScrollBar::ScrollBar(HWND hwnd, D2Objects const & d2, CTPMessageReceiver * parent)
	: AppItem(hwnd, d2), m_upArrow(hwnd, d2), m_dnArrow(hwnd, d2), m_parent(parent)
{
	m_upArrow.SetIcon(GetResourceIndex(IDB_DOWNARROWHEAD));
	m_dnArrow.SetIcon(GetResourceIndex(IDB_DOWNARROWHEAD));
}

void ScrollBar::SetSize(D2D1_RECT_F dipRect)
{
	m_dipRect = dipRect;
	m_dipRect.left = dipRect.right - m_width;
	m_pixRect = DPIScale::DipsToPixels(dipRect);

	// width-2 DIPs for icon
	m_upArrow.SetSize(D2D1::RectF(
		m_dipRect.left + 1.0f, m_dipRect.top + 2.0f, m_dipRect.right - 1.0f, m_dipRect.top + m_width
	));
	m_upArrow.SetClickRect(D2D1::RectF(
		m_dipRect.left, m_dipRect.top, m_dipRect.right, m_dipRect.top + m_width
	));
	m_dnArrow.SetSize(D2D1::RectF(
		m_dipRect.left + 1.0f, m_dipRect.bottom - m_width, m_dipRect.right - 1.0f, m_dipRect.bottom - 2.0f
	));
	m_dnArrow.SetClickRect(D2D1::RectF(
		m_dipRect.left, m_dipRect.bottom - m_width, m_dipRect.right, m_dipRect.bottom
	));


	CalculateBarRect();
}

void ScrollBar::Paint(D2D1_RECT_F updateRect)
{
	// Background
	m_d2.pBrush->SetColor(Colors::MAIN_BACKGROUND);
	m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);

	// Button Highlight
	// TODO

	// Buttons
	m_upArrow.Paint(updateRect);
	m_dnArrow.Paint(updateRect);

	// Bar
	if (m_mouseOn == moBar)
	{
		m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
		m_d2.pRenderTarget->FillRectangle(m_barRect, m_d2.pBrush);
	}
	else
	{
		m_d2.pBrush->SetColor(Colors::SCROLL_BAR);
		m_d2.pRenderTarget->FillRectangle(m_barRect, m_d2.pBrush);
	}
}

void ScrollBar::CalculateBarRect()
{
	float fullLength = m_dipRect.bottom - m_dipRect.top - 2.0f * m_width;
	float barLength = (float)m_visibleSteps / (float)m_totalSteps * fullLength;
	float barTop = (float)m_currStep / (float)m_totalSteps * fullLength;
	m_barRect = D2D1::RectF(
		m_dipRect.left,
		m_dipRect.top + m_width + barTop,
		m_dipRect.right,
		m_dipRect.top + m_width + barTop + barLength
	);
}
