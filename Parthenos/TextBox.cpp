#include "stdafx.h"
#include "TextBox.h"
#include "Chart.h"
#include "Colors.h"

TextBox::~TextBox()
{
	SafeRelease(&m_pTextLayout);
}

void TextBox::Paint(D2D1_RECT_F updateRect)
{
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f);

	if (m_update)
	{
		CreateTextLayout();
		m_update = false;
	}
	m_d2.pRenderTarget->DrawTextLayout(
		D2D1::Point2F(m_dipRect.left + 2, m_dipRect.top),
		m_pTextLayout,
		m_d2.pBrush
	);
}

bool TextBox::OnLButtonDown(D2D1_POINT_2F cursor)
{
	if (!(cursor.x >= m_dipRect.left &&
		cursor.x <= m_dipRect.right &&
		cursor.y >= m_dipRect.top &&
		cursor.y <= m_dipRect.bottom))
	{
		return false;
	}

	DWRITE_HIT_TEST_METRICS hitTestMetrics;
	BOOL isTrailingHit;
	BOOL isInside;

	m_pTextLayout->HitTestPoint(
		cursor.x - (m_dipRect.left + 2),
		cursor.y - m_dipRect.top,
		&isTrailingHit,
		&isInside,
		&hitTestMetrics
	);
	int end = hitTestMetrics.textPosition + isTrailingHit; // cursor should be placed before m_text[end]
	OutputMessage(L"%d %d %c\n", hitTestMetrics.textPosition, isTrailingHit, m_text.at(hitTestMetrics.textPosition + isTrailingHit));
	return false;
}

std::wstring TextBox::String()
{
	if (m_update)
	{
		m_text = L"MSFT";
		// TODO
	}
	return m_text;
}

void TextBox::CreateTextLayout()
{
	SafeRelease(&m_pTextLayout);
	std::wstring str = String();
	HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
		str.c_str(),			// The string to be laid out and formatted.
		str.size(),				// The length of the string.
		m_d2.pTextFormat_18p,   // The text format to apply to the string (contains font information, etc).
		Chart::m_labelBoxWidth - 4, // The width of the layout box.
		Chart::m_commandSize,   // The height of the layout box.
		&m_pTextLayout			// The IDWriteTextLayout interface pointer.
	);
	if (FAILED(hr)) Error(L"CreateTextLayout failed");
}
