#include "stdafx.h"
#include "Chart.h"
#include "utilities.h"
#include "TitleBar.h"

void Chart::Init(float leftOffset)
{
	m_leftOffset   = leftOffset;
	m_dipRect.left = leftOffset;
	m_dipRect.top  = TitleBar::height;
}

void Chart::Paint(D2Objects const & d2)
{
	d2.pBrush->SetColor(D2D1::ColorF(0.8f, 0.0f, 0.0f, 1.0f));
	d2.pRenderTarget->DrawRectangle(m_dipRect, d2.pBrush, 1.0, NULL);
}

void Chart::Resize(RECT pRect)
{
	m_dipRect.right  = DPIScale::PixelsToDipsX(pRect.right);
	m_dipRect.bottom = DPIScale::PixelsToDipsY(pRect.bottom);
}

void Axes::SetBoundingRect(float left, float top, float right, float bottom)
{
	m_dipRect.left   = left;
	m_dipRect.top    = top;
	m_dipRect.right  = right;
	m_dipRect.bottom = bottom;
}

Axes::Axes(float ylabelWidth, float xlabelHeight)
{
}
