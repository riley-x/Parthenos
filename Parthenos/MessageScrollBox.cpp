#include "stdafx.h"
#include "MessageScrollBox.h"

void MessageScrollBox::SetSize(D2D1_RECT_F dipRect)
{
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(dipRect);

	// Test
	m_scrollBar.SetSteps(100, 30, 0);

	m_scrollBar.SetSize(dipRect); // scroll bar auto sets width
}

void MessageScrollBox::Paint(D2D1_RECT_F updateRect)
{
	AppItem::Paint(updateRect);
	m_scrollBar.Paint(updateRect);
}
