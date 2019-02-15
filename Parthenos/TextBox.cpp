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
	if (m_active)
		m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
	else
		m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
	m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f);

	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	m_d2.pRenderTarget->DrawTextLayout(
		D2D1::Point2F(m_dipRect.left + m_leftOffset, m_dipRect.top),
		m_pTextLayout,
		m_d2.pBrush
	);

	if (m_active)
	{
		m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(m_fpos, m_dipRect.top),
			D2D1::Point2F(m_fpos, m_dipRect.bottom),
			m_d2.pBrush,
			1.0f
		);
	}
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
		cursor.x - (m_dipRect.left + m_leftOffset),
		cursor.y - m_dipRect.top,
		&isTrailingHit,
		&isInside,
		&hitTestMetrics
	);
	int m_oldpos = m_ipos;
	m_ipos = hitTestMetrics.textPosition + isTrailingHit; // cursor should be placed before m_text[end]
	m_fpos = hitTestMetrics.left + static_cast<float>(isTrailingHit) * hitTestMetrics.width
		+ m_dipRect.left + m_leftOffset;
	
	if (!m_active || m_oldpos != m_ipos) // remove this once have timer? may be too slow
	{
		RECT rect = DPIScale::DipsToPixels(m_dipRect);
		::InvalidateRect(m_hwnd, &rect, FALSE);
		m_active = true;
	}
	return true;
}

bool TextBox::OnChar(wchar_t c, LPARAM lParam)
{
	if (!m_active) return false;
	
	c = towupper(c);
	switch (c)
	{
	case 0x09: // tab
	case 0x0A: // linefeed 
	case 0x1B: // escape 
	case 0x0D: // carriage return (single line)
		//MessageBeep((UINT)-1);
		break;
	case 0x08: // backspace
	{
		if (m_text.length() == 0 || m_ipos == 0) break;
		m_text.erase(m_ipos - 1, 1);
		CreateTextLayout();
		MoveCaret(-1);
		break;
	}
	default:   // displayable character
	{
		if (m_text.length() >= 10)
		{
			MessageBeep((UINT)-1);
			break;
		}

		if (m_ipos == static_cast<int>(m_text.size()))
			m_text.append(1, c);
		else
			m_text.insert(m_ipos, 1, c);

		CreateTextLayout();
		MoveCaret(1);
		break;
	}
	}
	return true;
}

bool TextBox::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	if (!m_active) return false;

	switch (wParam)
	{
	case VK_LEFT:
		MoveCaret(-1);
		return true;
	case VK_RIGHT:
		MoveCaret(1);
		return true;
	case VK_HOME:
		MoveCaret(-m_ipos);
		return true;
	case VK_END:
		MoveCaret(m_text.size() - m_ipos);
		return true;
	case VK_DELETE:
		if (m_ipos == m_text.length()) return true;
		m_text.erase(m_ipos, 1);
		CreateTextLayout();
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		return true;
	case VK_INSERT:
		return false;
	default:
		return false;
	}

	return false;
}

std::wstring TextBox::String() const
{
	return m_text;
}

void TextBox::SetText(std::wstring text)
{
	if (text.length() > 10)
	{
		OutputMessage(L"%s too long\n", text);
		return;
	}
	m_text = text;
	m_active = false;
	CreateTextLayout();
}

void TextBox::CreateTextLayout()
{
	SafeRelease(&m_pTextLayout);
	std::wstring str = String();
	HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
		str.c_str(),			// The string to be laid out and formatted.
		str.size(),				// The length of the string.
		m_d2.pTextFormat_18p,   // The text format to apply to the string (contains font information, etc).
		Chart::m_labelBoxWidth - 2 * m_leftOffset, // The width of the layout box.
		Chart::m_commandSize,   // The height of the layout box.
		&m_pTextLayout			// The IDWriteTextLayout interface pointer.
	);
	if (FAILED(hr)) Error(L"CreateTextLayout failed");
}

// Moves the caret, with error checking, and invalidates
void TextBox::MoveCaret(int i)
{
	// silent so no extra bound check needed
	if (m_ipos + i > static_cast<int>(m_text.size())) return;
	if (m_ipos + i < 0) return;
	if (i == 0) return;

	m_ipos += i;
	float dummy_y;
	DWRITE_HIT_TEST_METRICS dummy_metrics;
	HRESULT hr = m_pTextLayout->HitTestTextPosition(
		m_ipos,
		FALSE,
		&m_fpos,
		&dummy_y,
		&dummy_metrics
	);
	m_fpos += m_dipRect.left + m_leftOffset;
	if (FAILED(hr)) Error(L"HitTestTextPosition failed");

	::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}