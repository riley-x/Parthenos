#include "stdafx.h"
#include "ScrollBar.h"

ScrollBar::ScrollBar(HWND hwnd, D2Objects const & d2, CTPMessageReceiver * parent)
	: AppItem(hwnd, d2), m_upArrow(hwnd, d2), m_dnArrow(hwnd, d2), m_parent(parent)
{
	m_upArrow.SetIcon(GetResourceIndex(IDB_DOWNARROWHEAD)); // TODO
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
	if (m_mouseOn == moUp)
	{
		m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
		m_d2.pRenderTarget->FillRectangle(m_upArrow.GetClickRect(), m_d2.pBrush);
	}
	else if (m_mouseOn == moDown)
	{
		m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
		m_d2.pRenderTarget->FillRectangle(m_dnArrow.GetClickRect(), m_d2.pBrush);
	}

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

bool ScrollBar::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	// dragging (redundant checks)
	if (m_drag && (wParam & MK_LBUTTON))
	{
		ScrollTo(CalculatePos(cursor.y, m_dragOffset));
		return true;
	}

	// else handle hover
	if (handeled || !inRect(cursor, m_dipRect))
	{
		if (m_mouseOn != moNone)
		{
			m_mouseOn = moNone;
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
		return false;
	}

	MouseOn mo = HitTest(cursor.y);
	if (mo != m_mouseOn)
	{
		m_mouseOn = mo;
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	}
	return true;
}

// cursor and handeled are irrelevant; parent should check
bool ScrollBar::OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	static int cumm_delta = 0;
	cumm_delta += GET_WHEEL_DELTA_WPARAM(wParam);

	int negSteps = cumm_delta / m_minStep; // positive delta = towards user = scroll down
	cumm_delta -= negSteps * m_minStep;
	ScrollTo(m_currPos - negSteps);

	return true;
}

// TODO should start timer to catch holding up/down buttons
bool ScrollBar::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	if (handeled || !inRect(cursor, m_dipRect)) return false;
	switch (HitTest(cursor.y))
	{
	case moUp:
		ScrollTo(m_currPos - 1);
		break;
	case moDown:
		ScrollTo(m_currPos + 1);
		break;
	case moBar:
		m_drag = true;
		m_dragOffset = cursor.y - m_barRect.top;
		m_mouseOn = moBar;
		m_parent->PostClientMessage(this, L"", CTPMessage::MOUSE_CAPTURED, 1);
		break;
	case moNone:
	{
		float offset = (m_barRect.bottom - m_barRect.top) / 2.0f;
		ScrollTo(CalculatePos(cursor.y, offset));
		m_mouseOn = moBar;
		break;
	}
	}

	return true;
}

void ScrollBar::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	OnLButtonDown(cursor, false);
}

void ScrollBar::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (m_drag)
	{
		m_drag = false;
		m_mouseOn = (inRect(cursor, m_dipRect)) ? HitTest(cursor.y) : moNone;
		m_parent->PostClientMessage(this, L"", CTPMessage::MOUSE_CAPTURED, -1);
	}
}

void ScrollBar::CalculateBarRect()
{
	float fullLength = m_dipRect.bottom - m_dipRect.top - 2.0f * m_width;
	float barLength = (float)m_visibleSteps / (float)m_totalSteps * fullLength;
	float barTop = (float)m_currPos / (float)m_totalSteps * fullLength;
	m_barRect = D2D1::RectF(
		m_dipRect.left,
		m_dipRect.top + m_width + barTop,
		m_dipRect.right,
		m_dipRect.top + m_width + barTop + barLength
	);
}

int ScrollBar::CalculatePos(float y, float offset)
{
	float fullLength = m_dipRect.bottom - m_dipRect.top - 2.0f * m_width;
	float dipPos = y - offset - (m_dipRect.top + m_width);
	return static_cast<int>(roundf(dipPos / fullLength * (float)m_totalSteps));
}

void ScrollBar::ScrollTo(int newPos)
{
	if (newPos < 0) newPos = 0;
	if (newPos > m_totalSteps - m_visibleSteps) newPos = m_totalSteps - m_visibleSteps;

	if (m_currPos != newPos)
	{
		m_parent->PostClientMessage(this, L"", CTPMessage::SCROLLBAR_SCROLL, newPos - m_currPos);
		m_currPos = newPos;
		CalculateBarRect();
	}
}

// y client DIP
ScrollBar::MouseOn ScrollBar::HitTest(float y)
{
	if (y < m_dipRect.top + m_width) return moUp;
	else if (y > m_dipRect.bottom - m_width) return moDown;
	else if (y > m_barRect.top && y < m_barRect.bottom) return moBar;
	else return moNone;
}
