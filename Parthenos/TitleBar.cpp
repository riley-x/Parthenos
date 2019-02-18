#include "stdafx.h"
#include "Parthenos.h"
#include "TitleBar.h"
#include "utilities.h"
#include "Button.h"

float const TitleBar::iconHPad = 6.0f;
float const TitleBar::height = 30.0f;

// This must be called AFTER DPIScale is initialized
void TitleBar::Init()
{
	m_dipRect.left = 0;
	m_dipRect.top = 0;
	m_dipRect.bottom = DPIScale::SnapToPixelY(height);

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

	std::wstring names[2] = { L"Portfolio", L"Chart" };

	float left = m_tabLeftStart;
	for (int i = 0; i < 2; i++)
	{
		TextButton *temp = new TextButton(m_hwnd, m_d2);
		temp->SetName(names[i]);
		temp->SetSize(D2D1::RectF(
			left,
			m_dipRect.top,
			left + m_tabWidth,
			m_dipRect.bottom
		));
		temp->SetBorderColor(false);
		temp->SetFormat(D2Objects::Segoe18);
		m_tabButtons.Add(temp);
		left += m_tabWidth;
	}
}

// 'pRect': client RECT of parent
void TitleBar::Resize(RECT pRect, D2D1_RECT_F pDipRect)
{
	m_pixRect.right = pRect.right;
	m_dipRect.right = pDipRect.right;

	float width = 32;
	for (int i = 0; i < nIcons; i++)
	{
		m_CommandIconRects[i].right = pRect.right - (width + 2 * iconHPad)*i - iconHPad;
		m_CommandIconRects[i].left = m_CommandIconRects[i].right - width;
	}
}

void TitleBar::Paint(D2D1_RECT_F updateRect)
{
	// when invalidating, converts to pixels then back to DIPs -> updateRect has smaller values
	// then when invalidated. Add 1 to adjust.
	if (updateRect.top + 1 > m_dipRect.bottom) return;
	m_d2.pBrush->SetColor(Colors::TITLE_BACKGROUND);
	m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);

	// Paint titlebar icon
	if (m_d2.pD2DBitmaps[4])
	{
		m_d2.pRenderTarget->DrawBitmap(
			m_d2.pD2DBitmaps[4],
			m_TitleIconRect
		);
	}

	// Paint tab buttons
	m_tabButtons.Paint(updateRect);
	// TODO get active rect and highlight

	// Paint tab dividers
	m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
	for (size_t i = 1; i <= m_tabButtons.Size(); i++)
	{
		float x = m_tabLeftStart + i * m_tabWidth;
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(x, m_dipRect.top + 5.0f),
			D2D1::Point2F(x, m_dipRect.bottom - 5.0f),
			m_d2.pBrush,
			1.0f
		);
	}

	// Paint pixels, not DIPs, for command icons
	m_d2.pRenderTarget->SetDpi(96.0f, 96.0f); 

	// Paint highlight of command icons
	int mouse_on = -1;
	if (m_mouseOn == Buttons::CLOSE) mouse_on = 0;
	else if (m_mouseOn == Buttons::MAXRESTORE) mouse_on = 1;
	else if (m_mouseOn == Buttons::MIN) mouse_on = 2;
	if (mouse_on >= 0)
	{
		m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
		m_d2.pRenderTarget->FillRectangle(D2D1::RectF(
			m_CommandIconRects[mouse_on].left - iconHPad,
			static_cast<float>(m_pixRect.top),
			m_CommandIconRects[mouse_on].right + iconHPad,
			static_cast<float>(m_pixRect.bottom)
		), m_d2.pBrush);
	}

	// Paint command icons
	int bitmap_ind[2][3] = { {0,1,2}, {0,3,2} }; // index into d2.pD2DBitmaps depending on max vs. restore
	for (int i = 0; i < nIcons; i++)
	{
		if (m_d2.pD2DBitmaps[i])
			m_d2.pRenderTarget->DrawBitmap(
				m_d2.pD2DBitmaps[bitmap_ind[static_cast<int>(m_maximized)][i]], 
				m_CommandIconRects[i]
			);
	}

	// restore DIPs
	m_d2.pRenderTarget->SetDpi(0, 0); 
}

void TitleBar::OnMouseMoveP(POINT cursor, WPARAM wParam)
{
	Buttons button = HitTest(cursor);
	SetMouseOn(button);
}

bool TitleBar::OnLButtonDownP(POINT cursor)
{
	Buttons button = HitTest(cursor);
	switch (button)
	{
	case Buttons::CLOSE:
		m_parent->SendClientMessage(this, L"", CTPMessage::TITLEBAR_CLOSE);
		break;
	case Buttons::MAXRESTORE:
		m_parent->SendClientMessage(this, L"", CTPMessage::TITLEBAR_MAXRESTORE);
		break;
	case Buttons::MIN:
		m_parent->SendClientMessage(this, L"", CTPMessage::TITLEBAR_MIN);
		break;
	case Buttons::CAPTION:
	case Buttons::NONE:
	default:
		break;
	}

	// TODO pass to tab buttons
	return false;
}

TitleBar::Buttons TitleBar::HitTest(POINT cursor)
{
	// Since command icons are flush right, test right to left
	// Recall command icon rects are in pixels!
	if (cursor.y > m_pixRect.top && cursor.y < m_pixRect.bottom)
	{
		if (cursor.x > m_CommandIconRects[0].left - iconHPad)
			return Buttons::CLOSE;
		else if (cursor.x > m_CommandIconRects[1].left - iconHPad)
			return Buttons::MAXRESTORE;
		else if (cursor.x > m_CommandIconRects[2].left - iconHPad)
			return Buttons::MIN;
		else
			return Buttons::CAPTION;
	}
	return Buttons::NONE;
}
