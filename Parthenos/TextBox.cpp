#include "stdafx.h"
#include "TextBox.h"
#include "Chart.h"

TextBox::~TextBox()
{
	SafeRelease(&m_pTextLayout);
}

void TextBox::Paint(D2D1_RECT_F updateRect)
{
	// Draw highlight
	if (m_selection)
	{
		m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
		m_d2.pRenderTarget->FillRectangle(D2D1::RectF(
			min(m_fpos, m_fstart),
			m_dipRect.top,
			max(m_fpos, m_fstart),
			m_dipRect.bottom
		), m_d2.pBrush);
	}

	// Text
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	m_d2.pRenderTarget->DrawTextLayout(
		D2D1::Point2F(m_dipRect.left + m_leftOffset, m_dipRect.top),
		m_pTextLayout,
		m_d2.pBrush
	);

	// Draw caret
	if (m_active && m_flash)
	{
		m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(m_fpos, m_dipRect.top),
			D2D1::Point2F(m_fpos, m_dipRect.bottom),
			m_d2.pBrush,
			1.0f
		);
	}

	// Draw bounding box
	if (m_border)
	{
		if (m_active)
			m_d2.pBrush->SetColor(Colors::BRIGHT_LINE);
		else
			m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
		m_d2.pRenderTarget->DrawRectangle(m_dipRect, m_d2.pBrush, 0.5f);
	}
}

void TextBox::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (m_mouseSelection && (wParam & MK_LBUTTON))
	{
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
		int new_ipos = hitTestMetrics.textPosition + isTrailingHit; // cursor should be placed before m_text[end]
		if (new_ipos != m_ipos)
		{
			if (!m_selection)
			{
				m_selection = true;
				m_istart = m_ipos;
				m_fstart = m_fpos;
			}
			m_ipos = new_ipos;
			m_fpos = hitTestMetrics.left + static_cast<float>(isTrailingHit) * hitTestMetrics.width
				+ m_dipRect.left + m_leftOffset;

			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
	}
	else if (inRect(cursor, m_dipRect) && !m_mouseOver)
	{
		m_mouseOver = true;
		Cursor::SetCursor(Cursor::hIBeam);
	}
	else if (m_mouseOver && !inRect(cursor, m_dipRect))
	{
		m_mouseOver = false;
		Cursor::SetCursor(Cursor::hArrow);
	}
}

bool TextBox::OnLButtonDown(D2D1_POINT_2F cursor)
{
	if (!(cursor.x >= m_dipRect.left &&
		cursor.x <= m_dipRect.right &&
		cursor.y >= m_dipRect.top &&
		cursor.y <= m_dipRect.bottom))
	{
		if (m_active)
		{
			Deactivate();
			m_selection = false;
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
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
	int oldpos = m_ipos;
	m_ipos = hitTestMetrics.textPosition + isTrailingHit; // cursor should be placed before m_text[end]
	m_fpos = hitTestMetrics.left + static_cast<float>(isTrailingHit) * hitTestMetrics.width
		+ m_dipRect.left + m_leftOffset;
	
	if (!m_active || oldpos != m_ipos || m_selection)
	{
		m_selection = false;
		Activate();
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	}
	m_mouseSelection = true; // flag OnMouseMove to track

	return true;
}

void TextBox::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (inRect(cursor, m_dipRect))
	{
		if (m_selection && m_istart == 0 && m_ipos == m_text.size()) return;
		Activate();
		m_selection = true;
		m_istart = 0;
		m_ipos = m_text.size();

		float dummy_y;
		DWRITE_HIT_TEST_METRICS dummy_metrics;
		HRESULT hr = m_pTextLayout->HitTestTextPosition(
			m_istart,
			FALSE,
			&m_fstart,
			&dummy_y,
			&dummy_metrics
		);
		m_fstart += m_dipRect.left + m_leftOffset;
		if (FAILED(hr)) OutputMessage(L"HitTestTextPosition failed\n");

		hr = m_pTextLayout->HitTestTextPosition(
			m_ipos,
			FALSE,
			&m_fpos,
			&dummy_y,
			&dummy_metrics
		);
		m_fpos += m_dipRect.left + m_leftOffset;
		if (FAILED(hr)) OutputMessage(L"HitTestTextPosition failed\n");

		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	}
}

void TextBox::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	if (m_mouseSelection) m_mouseSelection = false;
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
		if (m_selection)
		{
			DeleteSelection();
			break;
		}
		if (m_text.length() == 0 || m_ipos == 0) break;
		m_text.erase(m_ipos - 1, 1);
		CreateTextLayout();
		MoveCaret(-1);
		break;
	}
	default:   // displayable character
	{
		if (m_selection) DeleteSelection(false);
		if (m_text.length() >= m_maxChars)
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
		if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			if (!m_selection)
			{
				m_selection = true;
				m_istart = m_ipos;
				m_fstart = m_fpos;
			}
		}
		else
		{
			m_selection = false;
		}
		MoveCaret(-1);
		return true;
	case VK_RIGHT:
	{
		if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			if (!m_selection)
			{
				m_selection = true;
				m_istart = m_ipos;
				m_fstart = m_fpos;
			}
		}
		else
		{
			m_selection = false;
		}
		MoveCaret(1);
		return true;
	}
	case VK_HOME:
		if (m_selection) m_selection = false;
		MoveCaret(-m_ipos);
		return true;
	case VK_END:
		if (m_selection) m_selection = false;
		MoveCaret(m_text.size() - m_ipos);
		return true;
	case VK_DELETE:
		if (m_selection)
		{
			DeleteSelection();
		}
		else
		{
			if (m_ipos == m_text.length()) return true;
			m_text.erase(m_ipos, 1);
			CreateTextLayout();
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
		return true;
	case VK_RETURN:
		m_parent->ReceiveMessage(m_text, 0); // will call SetText
		return true;
	case VK_INSERT:
		return false;
	default:
		return false;
	}

	return false;
}

void TextBox::OnTimer(WPARAM wParam, LPARAM lParam)
{
	if (wParam == Timers::IDT_CARET)
	{
		m_flash = !m_flash;
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	}
}


void TextBox::SetText(std::wstring text)
{
	if (text.length() > m_maxChars)
	{
		OutputMessage(L"%s too long\n", text);
		return;
	}
	m_text = text;
	Deactivate();
	m_selection = false;
	CreateTextLayout();
}

void TextBox::Activate()
{
	m_active = true;
	if (!Timers::active[Timers::IDT_CARET])
	{
		UINT_PTR err = ::SetTimer(m_hwnd, Timers::IDT_CARET, Timers::CARET_TIME, NULL);
		if (err == 0) OutputError(L"Set timer failed");
		else Timers::active[Timers::IDT_CARET] = true;
	}
}

void TextBox::Deactivate()
{
	m_active = false;
	if (Timers::active[Timers::IDT_CARET])
	{
		BOOL err = ::KillTimer(m_hwnd, Timers::IDT_CARET);
		if (err == 0)  OutputError(L"Kill timer failed");
		else Timers::active[Timers::IDT_CARET] = false;
	}
}

void TextBox::CreateTextLayout()
{
	SafeRelease(&m_pTextLayout);
	std::wstring str = String();
	HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
		str.c_str(),					// The string to be laid out and formatted.
		str.size(),						// The length of the string.
		m_d2.pTextFormats[m_format],	// The text format to apply to the string (contains font information, etc).
		m_dipRect.right - m_dipRect.left - 2 * m_leftOffset, // The width of the layout box.
		m_dipRect.bottom - m_dipRect.top, // The height of the layout box.
		&m_pTextLayout					// The IDWriteTextLayout interface pointer.
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
	if (FAILED(hr)) OutputMessage(L"HitTestTextPosition failed\n");

	if (m_selection && m_ipos == m_istart) m_selection = false;

	::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}

void TextBox::DeleteSelection(bool invalidate)
{
	if (m_ipos > m_istart)
	{
		m_text.erase(m_istart, m_ipos - m_istart);
		m_ipos = m_istart;
		m_fpos = m_fstart;
	}
	else
	{
		m_text.erase(m_ipos, m_istart - m_ipos);
	}
	m_selection = false;

	if (invalidate)
	{
		CreateTextLayout();
		::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
	}
}