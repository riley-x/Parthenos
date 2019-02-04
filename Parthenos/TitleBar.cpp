#include "stdafx.h"
#include "Parthenos.h"
#include "TitleBar.h"
#include "utilities.h"


void TitleBar::Paint(D2Objects const & d2)
{
	d2.pBrush->SetColor(D2D1::ColorF(0.15f, 0.16f, 0.15f, 1.0f));
	d2.pRenderTarget->FillRectangle(D2D1::RectF(
		DPIScale::PixelsToDipsX(m_cRect.left),
		DPIScale::PixelsToDipsY(m_cRect.top),
		DPIScale::PixelsToDipsX(m_cRect.right),
		DPIScale::PixelsToDipsY(m_cRect.bottom)
	), d2.pBrush);


	// Draws an image and scales it to the current window size
	D2D1_RECT_F rectangle = D2D1::RectF(
		DPIScale::PixelsToDipsX(m_cRect.right) - 30.0f,
		DPIScale::PixelsToDipsY(m_cRect.top),
		DPIScale::PixelsToDipsX(m_cRect.right),
		DPIScale::PixelsToDipsY(m_cRect.bottom)
	);
	if (d2.pD2DBitmaps[0])
	{
		d2.pRenderTarget->DrawBitmap(d2.pD2DBitmaps[0], rectangle);
	}
	//GetClientRect(m_hwnd, &clientRect);
	//OutputMessage(L"Titlebar Rect: %ld %ld\n", clientRect.right, clientRect.bottom);

	//D2D1_SIZE_F size = d2.pRenderTarget->GetSize();
	//OutputMessage(L"Render Target: %3.1f %3.1f\n\n\n", size.width, size.height);

}


// 'pRect': client RECT of parent
void TitleBar::Resize(RECT pRect)
{
	int height = DPIScale::DipsToPixelsY(30.0f);
	m_cRect.right = pRect.right;
	m_cRect.bottom = height;

}