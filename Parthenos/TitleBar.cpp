#include "stdafx.h"
#include "Parthenos.h"
#include "TitleBar.h"
#include "utilities.h"

float const TitleBar::iconHPad = 6.0f;
float const TitleBar::height = 30.0f;

// This must be called AFTER DPIScale is initialized
void TitleBar::Init()
{
	m_dipRect.left = 0;
	m_dipRect.top = 0;
	m_dipRect.bottom = height;

	m_pixRect.left = 0;
	m_pixRect.top = 0;
	m_pixRect.bottom = DPIScale::DipsToPixelsY(height);

	for (int i = 0; i < nIcons; i++)
	{
		float icon_height = 24; // pixels
		float pad = ceil((m_pixRect.bottom - icon_height) / 2.0f); 
		// round or else D2D interpolates pixels
		m_CommandIconRects[i] = D2D1::RectF(0, pad, 0, pad + icon_height);
	}

	m_TitleIconRect = D2D1::RectF(3.0f, 3.0f, 27.0f, 27.0f);
}

void TitleBar::Paint(D2Objects const & d2)
{

	d2.pBrush->SetColor(D2D1::ColorF(0.15f, 0.16f, 0.15f, 1.0f));
	d2.pRenderTarget->FillRectangle(m_dipRect, d2.pBrush);

	// Paint titlebar icon
	if (d2.pD2DBitmaps[4])
	{
		d2.pRenderTarget->DrawBitmap(
			d2.pD2DBitmaps[4],
			m_TitleIconRect
		);
	}

	// Paint highlight of command icons
	d2.pRenderTarget->SetDpi(96.0f, 96.0f); // paint pixels, not DIPs

	int mouse_on = -1;
	if (m_mouseOn == HTCLOSE) mouse_on = 0;
	else if (m_mouseOn == HTMAXBUTTON) mouse_on = 1;
	else if (m_mouseOn == HTMINBUTTON) mouse_on = 2;
	if (mouse_on >= 0)
	{
		d2.pBrush->SetColor(D2D1::ColorF(0.25f, 0.25f, 0.25f, 1.0f));
		d2.pRenderTarget->FillRectangle(D2D1::RectF(
			m_CommandIconRects[mouse_on].left - iconHPad,
			static_cast<float>(m_pixRect.top),
			m_CommandIconRects[mouse_on].right + iconHPad,
			static_cast<float>(m_pixRect.bottom)
		), d2.pBrush);
	}

	// Paint command icons
	int bitmap_ind[2][3] = { {0,1,2}, {0,3,2} }; // index into d2.pD2DBitmaps depending on max vs. restore
	for (int i = 0; i < nIcons; i++)
	{
		if (d2.pD2DBitmaps[i])
			d2.pRenderTarget->DrawBitmap(
				d2.pD2DBitmaps[bitmap_ind[static_cast<int>(m_maximized)][i]], 
				m_CommandIconRects[i]
			);
	}
	d2.pRenderTarget->SetDpi(0, 0); // restore DIPs

	//D2D1_SIZE_U size = d2.pD2DBitmaps[0]->GetPixelSize();
	//OutputMessage(L"%u %u\n\n", size.width, size.height);

	//GetClientRect(m_hwnd, &clientRect);
	//OutputMessage(L"Titlebar Rect: %ld %ld\n", clientRect.right, clientRect.bottom);

	//D2D1_SIZE_F size = d2.pRenderTarget->GetSize();
	//OutputMessage(L"Render Target: %3.1f %3.1f\n\n\n", size.width, size.height);
}


// 'pRect': client RECT of parent
void TitleBar::Resize(RECT pRect)
{ 
	m_pixRect.right = pRect.right;
	m_dipRect.right = DPIScale::PixelsToDipsX(pRect.right);;

	float width = 32;
	for (int i = 0; i < nIcons; i++)
	{
		m_CommandIconRects[i].right = pRect.right - (width + 2*iconHPad)*i - iconHPad;
		m_CommandIconRects[i].left = m_CommandIconRects[i].right - width;
	}
}

// Do hit test for titlebar / command icons in pixel coordinates.
// Don't actually return the LRESULT values on handling messages. (Bad functionality).
// Just use pre-defined macros for convenience. 
LRESULT TitleBar::HitTest(POINT cursor)
{
	if (cursor.y > m_pixRect.top && cursor.y < m_pixRect.bottom)
	{
		if (cursor.x > m_CommandIconRects[0].left - iconHPad)
			return HTCLOSE;
		else if (cursor.x > m_CommandIconRects[1].left - iconHPad)
			return HTMAXBUTTON;
		else if (cursor.x > m_CommandIconRects[2].left - iconHPad)
			return HTMINBUTTON;
		else
			return HTCAPTION;
	}
	return HTCLIENT;
}

void TitleBar::MouseOn(LRESULT button, HWND p_hwnd)
{
	if (m_mouseOn != button)
	{
		m_mouseOn = button;
		InvalidateRect(p_hwnd, &m_pixRect, FALSE);
	}
}