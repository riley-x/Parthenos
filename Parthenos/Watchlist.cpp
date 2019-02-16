#include "stdafx.h"
#include "Watchlist.h"
#include "TitleBar.h"
#include "utilities.h"

Watchlist::~Watchlist()
{
	for (auto item : m_items) if (item) delete item;
	for (auto layout : m_pTextLayouts) SafeRelease(&layout);
}

// Flush left with fixed width
void Watchlist::Init(float width)
{
	m_dipRect.left = 0.0f;
	m_dipRect.top = TitleBar::height;
	m_dipRect.right = width;
	m_pixRect.left = 0;
	m_pixRect.top = DPIScale::DipsToPixelsY(m_dipRect.top);
	m_pixRect.right = DPIScale::DipsToPixelsX(width);

	m_rightBorder = DPIScale::SnapToPixelX(m_dipRect.right - DPIScale::PixelsToDipsX(1));
}

void Watchlist::Paint(D2D1_RECT_F updateRect)
{
	// When invalidating, converts to pixels then back to DIPs -> updateRect has smaller values
	// than when invalidated.
	if (updateRect.bottom <= m_dipRect.top || updateRect.left + 1 > m_dipRect.right) return;

	m_d2.pBrush->SetColor(Colors::WATCH_BACKGROUND);
	m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);
	m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	m_d2.pRenderTarget->DrawLine(
		D2D1::Point2F(m_rightBorder, m_dipRect.top),
		D2D1::Point2F(m_rightBorder, m_dipRect.bottom),
		m_d2.pBrush,
		DPIScale::PixelsToDipsX(1)
	);
}

void Watchlist::Resize(RECT pRect, D2D1_RECT_F pDipRect)
{
	m_dipRect.bottom = pDipRect.bottom;
	m_pixRect.bottom = pRect.bottom;
}
