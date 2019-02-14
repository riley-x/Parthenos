#include "stdafx.h"
#include "TextBox.h"


void TextBox::Paint(D2D1_RECT_F updateRect)
{
	m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f);
}
