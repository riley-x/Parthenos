#include "stdafx.h"
#include "MessageScrollBox.h"

void MessageScrollBox::SetSize(D2D1_RECT_F dipRect)
{
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(dipRect);

	// Test
	m_scrollBar.SetSteps(WHEEL_DELTA / 3, 100, 30);
	// Set 1 line per minStep, 3 lines per detent

	m_scrollBar.SetSize(dipRect); // scroll bar auto sets width
}

void MessageScrollBox::Paint(D2D1_RECT_F updateRect)
{
	AppItem::Paint(updateRect);
	m_scrollBar.Paint(updateRect);
}

bool MessageScrollBox::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	bool out = m_scrollBar.OnMouseMove(cursor, wParam, handeled);
	ProcessCTPMessages();
	return out;
}

bool MessageScrollBox::OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	if (!handeled && inRect(cursor, m_dipRect))
	{
		m_scrollBar.OnMouseWheel(cursor, wParam, handeled);
		ProcessCTPMessages();
		return true;
	}
	return false;
}

bool MessageScrollBox::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	if (handeled || !inRect(cursor, m_dipRect)) return false;
	if (!m_scrollBar.OnLButtonDown(cursor, handeled))
	{
		// TODO
	}

	ProcessCTPMessages();
	return true;
}

void MessageScrollBox::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	OnLButtonDown(cursor, false);
}

void MessageScrollBox::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	m_scrollBar.OnLButtonUp(cursor, wParam);
	ProcessCTPMessages();
}

void MessageScrollBox::ProcessCTPMessages()
{
	for (auto const & msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::SCROLLBAR_SCROLL:
			// TODO
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			break;
		case CTPMessage::MOUSE_CAPTURED: // always from scrollbox
			m_scrollCapturedMouse = msg.iData;
			m_parent->PostClientMessage(this, msg.msg, msg.imsg, msg.iData, msg.dData);
		default:
			break;
		}
	}

	if (!m_messages.empty()) m_messages.clear();
}