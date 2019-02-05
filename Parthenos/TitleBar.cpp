#include "stdafx.h"
#include "Parthenos.h"
#include "TitleBar.h"
#include "utilities.h"

float const TitleBar::iconPad = 2.0f;

TitleBar::TitleBar()
{
	m_dipRect.left	 = 0;
	m_dipRect.top	 = 0;
	m_dipRect.bottom = 30;

	m_pixRect.left	 = 0;
	m_pixRect.top	 = 0;
	m_pixRect.bottom = DPIScale::DipsToPixelsY(m_dipRect.bottom);

	for (int i = 0; i < nIcons; i++)
	{
		m_CommandIconRects[i] = D2D1::RectF(
			0.0f,
			6.0f,
			0.0f,
			24.0f // paint fixed 24x18 DIPs instead of pixels
		);
	}
}

void TitleBar::Paint(D2Objects const & d2)
{

	d2.pBrush->SetColor(D2D1::ColorF(0.15f, 0.16f, 0.15f, 1.0f));
	d2.pRenderTarget->FillRectangle(m_dipRect, d2.pBrush);

	for (int i = 0; i < nIcons; i++)
	{
		if (d2.pD2DBitmaps[i])
			d2.pRenderTarget->DrawBitmap(d2.pD2DBitmaps[i], m_CommandIconRects[i]);
	}

	//GetClientRect(m_hwnd, &clientRect);
	//OutputMessage(L"Titlebar Rect: %ld %ld\n", clientRect.right, clientRect.bottom);

	//D2D1_SIZE_F size = d2.pRenderTarget->GetSize();
	//OutputMessage(L"Render Target: %3.1f %3.1f\n\n\n", size.width, size.height);

}


// 'pRect': client RECT of parent
void TitleBar::Resize(RECT pRect)
{
	float right = DPIScale::PixelsToDipsX(pRect.right);
	m_pixRect.right = pRect.right;
	m_dipRect.right = right;

	float width = 24.0f;
	for (int i = 0; i < nIcons; i++)
	{
		m_CommandIconRects[i].right = (right - 2) - (width + 2*iconPad)*i - iconPad;
		m_CommandIconRects[i].left = m_CommandIconRects[i].right - width;
	}
}

LRESULT TitleBar::HitTest(D2D1_POINT_2F cursor)
{
	// Don't actually return the LRESULT values on handling messages. (Bad functionality).
	// Just use pre-defined macros for convenience. 
	if (cursor.y > m_dipRect.top && cursor.y < m_dipRect.bottom)
	{
		if (cursor.x > m_CommandIconRects[0].left - iconPad)
			return HTCLOSE;
		else if (cursor.x > m_CommandIconRects[1].left - iconPad)
			return HTMAXBUTTON;
		else if (cursor.x > m_CommandIconRects[2].left - iconPad)
			return HTMINBUTTON;
		else
			return HTCAPTION;
	}
	return HTCLIENT;
}
