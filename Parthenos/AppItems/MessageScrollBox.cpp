#include "../stdafx.h"
#include "MessageScrollBox.h"

void MessageScrollBox::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(dipRect, m_dipRect)) return;
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	m_titleRect = m_dipRect;
	m_titleRect.bottom = DPIScale::SnapToPixelY(m_titleRect.top + m_titleHeight);
	m_titleBorderY = m_titleRect.bottom - DPIScale::hpy();

	m_layoutRect = m_dipRect;
	m_layoutRect.left += 7.0f;
	m_layoutRect.top = m_titleRect.bottom + 3.0f;
	m_layoutRect.right -= 7.0f + ScrollBar::Width;
	m_layoutRect.bottom -= 3.0f;

	CreateTextLayout();

	m_currLine = m_scrollBar.SetSteps(m_metrics.lineCount, m_visibleLines, ScrollBar::SetPosMethod::MaintainOffsetBottom);

	// scroll bar auto sets left
	m_scrollBar.SetSize(D2D1::RectF(0.0f, m_titleRect.bottom, m_dipRect.right, m_dipRect.bottom));
}

void MessageScrollBox::Paint(D2D1_RECT_F updateRect)
{
	if (!overlapRect(m_dipRect, updateRect)) return;
	if (updateRect.left < m_layoutRect.right)
	{
		// Background
		m_d2.pBrush->SetColor(Colors::WATCH_BACKGROUND);
		m_d2.pD2DContext->FillRectangle(m_dipRect, m_d2.pBrush);

		// Title
		m_d2.pBrush->SetColor(Colors::AXES_BACKGROUND);
		m_d2.pD2DContext->FillRectangle(m_titleRect, m_d2.pBrush);
		m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(m_titleRect.left, m_titleBorderY),
			D2D1::Point2F(m_titleRect.right, m_titleBorderY),
			m_d2.pBrush,
			1.0f,
			m_d2.pHairlineStyle
		);
		m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
		m_d2.pD2DContext->DrawTextW(L" Output:", 7, m_d2.pTextFormats[D2Objects::Formats::Segoe12], m_titleRect, m_d2.pBrush);

		
		// Text
		if (m_pTextLayout)
		{
			m_d2.pD2DContext->PushAxisAlignedClip(m_layoutRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			m_d2.pD2DContext->SetTransform(D2D1::Matrix3x2F::Translation(0, -m_linePos[m_currLine]));

			// Highlight
			if (m_iSelectStart != m_iSelectEnd)
			{
				m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
				for (auto const & metric : m_selectionMetrics)
				{
					m_d2.pD2DContext->FillRectangle(
						D2D1::RectF(metric.left, metric.top, metric.left + metric.width, metric.top + metric.height),
						m_d2.pBrush
					);
				}
			}

			m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
			m_d2.pD2DContext->DrawTextLayout(D2D1::Point2F(m_layoutRect.left, m_layoutRect.top), m_pTextLayout, m_d2.pBrush);


			m_d2.pD2DContext->SetTransform(D2D1::Matrix3x2F::Identity());
			m_d2.pD2DContext->PopAxisAlignedClip();
		}
	}
	m_scrollBar.Paint(updateRect);
}

bool MessageScrollBox::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	if (m_selection)
	{
		if (cursor.y < m_layoutRect.top || cursor.y > m_layoutRect.bottom)
		{
			if (!m_timerSet)
			{
				Timers::SetTimer(m_hwnd, Timers::IDT_SCROLLBAR);
				m_timerSet = true;
			}
			m_mouseInBox = false;
		} // let OnTimer handle the selection
		else
		{
			int i = HitTest(cursor);
			if (i != m_iSelectEnd)
			{
				m_iSelectEnd = i;
				GetSelectionMetrics();
				::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			}
			if (m_timerSet) m_mouseInBox = true;
		}

		Cursor::SetCursor(Cursor::hIBeam);
		handeled = true;
	}
	else
	{
		handeled = m_scrollBar.OnMouseMove(cursor, wParam, handeled) || handeled;

		D2D1_RECT_F temp = m_dipRect;
		temp.top = m_titleRect.bottom;
		if (!handeled && inRect(cursor, temp))
		{
			Cursor::SetCursor(Cursor::hIBeam);
			handeled = true;
		}
	}

	ProcessCTPMessages();
	return handeled;
}

bool MessageScrollBox::OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	if (!handeled && inRect(cursor, m_dipRect))
	{
		m_scrollBar.OnMouseWheel(cursor, wParam, handeled);
		ProcessCTPMessages();
		return true;
	}
	return false;
}

bool MessageScrollBox::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	if (handeled || !inRect(cursor, m_dipRect))
	{
		return false;
	}

	if (!m_scrollBar.OnLButtonDown(cursor, handeled))
	{
		m_selection = true;
		if (m_iSelectEnd != m_iSelectStart)
		{
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE); // remove current highlight
		}
		m_iSelectStart = HitTest(cursor);
		m_iSelectEnd = m_iSelectStart;
		m_parent->PostClientMessage(this, L"", CTPMessage::MOUSE_CAPTURED, 1);
	}

	ProcessCTPMessages();
	return true;
}

void MessageScrollBox::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	OnLButtonDown(cursor, false);
}

void MessageScrollBox::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	m_scrollBar.OnLButtonUp(cursor, wParam);
	if (m_selection)
	{
		m_selection = false;
		m_parent->PostClientMessage(this, L"", CTPMessage::MOUSE_CAPTURED, -1);
		int i = HitTest(cursor);
		if (i != m_iSelectEnd)
		{
			m_iSelectEnd = i;
			GetSelectionMetrics();
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
		if (m_timerSet)
		{
			Timers::UnsetTimer(m_hwnd, Timers::IDT_SCROLLBAR);
			m_timerSet = false;
		}
	}
	ProcessCTPMessages();
}

void MessageScrollBox::OnTimer(WPARAM wParam, LPARAM lParam)
{
	if (wParam == Timers::IDT_SCROLLBAR && m_timerSet && !m_mouseInBox)
	{
		POINT p;
		bool ok = GetCursorPos(&p);
		if (ok) ok = ScreenToClient(m_hwnd, &p);
		if (!ok) return OutputMessage(L"MessageScrollBox OnTimer get cursor failed\n");
		D2D1_POINT_2F cursor = DPIScale::PixelsToDips(p);

		static int nTicks = 0;
		nTicks++;
		int threshold = 4; // Timer is 50ms, change granularity depending on mouse distance
		int step = 0;

		if (cursor.y < m_layoutRect.top)
		{
			step = -1;
			if (m_layoutRect.top - cursor.y >= 30.0f) threshold = 1;
			if (m_layoutRect.top - cursor.y >= 60.0f) step = -2;
			cursor.y = m_layoutRect.top;
		}
		else if (cursor.y > m_layoutRect.bottom)
		{
			step = 1;
			if (cursor.y - m_layoutRect.bottom >= 30.0f) threshold = 1;
			if (cursor.y - m_layoutRect.bottom >= 60.0f) step = 2;
			cursor.y = m_layoutRect.bottom;
		}
		else // Inside layout rect, so let OnMouseMove handle. Shouldn't get here
		{
			nTicks = 0;
			return;
		}

		if (nTicks >= threshold)
		{
			m_currLine = m_scrollBar.Scroll(step);
			int i = HitTest(cursor);
			if (i != m_iSelectEnd)
			{
				m_iSelectEnd = i;
				GetSelectionMetrics();
				::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			}
			nTicks = 0;
		}
	}
}

bool MessageScrollBox::OnCopy()
{
	if (m_iSelectEnd == m_iSelectStart) return false;

	std::wstring out = m_text.substr(
		min(m_iSelectStart, m_iSelectEnd), 
		abs((int)m_iSelectEnd - (int)m_iSelectStart)
	);

	if (!OpenClipboard(m_hwnd)) return false;

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (out.size() + 1)); // plus null
	if (hMem == NULL)
	{
		CloseClipboard();
		return false;
	}
	memcpy(GlobalLock(hMem), out.data(), sizeof(wchar_t) * (out.size() + 1));
	GlobalUnlock(hMem);

	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();

	return true;
}

void MessageScrollBox::ProcessCTPMessages()
{
	for (auto const & msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::SCROLLBAR_SCROLL:
			m_currLine += msg.iData;
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			break;
		case CTPMessage::MOUSE_CAPTURED: // always from scrollbox
			m_parent->PostClientMessage(this, msg.msg, msg.imsg, msg.iData, msg.dData);
		default:
			break;
		}
	}

	if (!m_messages.empty()) m_messages.clear();
}

void MessageScrollBox::Print(std::wstring const & msg)
{
	m_text.append(msg);
	CreateTextLayout();
	m_currLine = m_scrollBar.SetSteps(m_metrics.lineCount, m_visibleLines, ScrollBar::SetPosMethod::MaintainOffsetBottom);
	m_scrollBar.Refresh();
	::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}

void MessageScrollBox::Overwrite(std::wstring const & msg)
{
	m_text = msg;
	CreateTextLayout();
	m_currLine = m_scrollBar.SetSteps(m_metrics.lineCount, m_visibleLines, ScrollBar::SetPosMethod::Bottom);
	m_scrollBar.Refresh();
	::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}

void MessageScrollBox::Clear()
{
	m_text.clear();
	CreateTextLayout();
	m_currLine = m_scrollBar.SetSteps(m_metrics.lineCount, m_visibleLines);
	m_scrollBar.Refresh();
	::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
}


// Creates the text layout and also calculates the metrics, line positions, and visible lines
void MessageScrollBox::CreateTextLayout()
{
	SafeRelease(&m_pTextLayout);
	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
		m_text.c_str(),					// The string to be laid out and formatted.
		m_text.size(),					// The length of the string.
		m_d2.pTextFormats[D2Objects::Formats::Segoe12],	// The text format to apply to the string
		m_layoutRect.right - m_layoutRect.left, // The width of the layout box.
		m_layoutRect.bottom - m_layoutRect.top, // The height of the layout box.
		&m_pTextLayout					// The IDWriteTextLayout interface pointer.
	);
	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	if (SUCCEEDED(hr))
	{
		hr = m_pTextLayout->GetMetrics(&m_metrics);
	}
	if (SUCCEEDED(hr))
	{
		UINT32 actualLineCount;
		DWRITE_LINE_METRICS *metrics = new DWRITE_LINE_METRICS[m_metrics.lineCount];
		hr = m_pTextLayout->GetLineMetrics(metrics, m_metrics.lineCount, &actualLineCount);
		if (actualLineCount != m_metrics.lineCount)
			OutputMessage(L"MessageBox CreateTextLayout line count mismatch: expected %u, got %u\n", m_metrics.lineCount, actualLineCount);

		if (SUCCEEDED(hr))
		{
			if (actualLineCount != 0)
			{
				m_linePos.resize(actualLineCount);
				float cum = 0;
				for (size_t i = 0; i < actualLineCount; i++)
				{
					m_linePos[i] = cum;
					cum += metrics[i].height;
				}

				float avgHeight = cum / (float)actualLineCount;
				m_visibleLines = static_cast<size_t>(floorf((m_layoutRect.bottom - m_layoutRect.top) / avgHeight));
			}
			else
			{
				m_metrics.lineCount = 0;
				m_visibleLines = 0;
				m_currLine = 0;
				m_linePos.clear();
			}
		}
		delete[] metrics;
	}
	if (FAILED(hr))
	{
		OutputError(L"MessageBox CreateTextLayout failed");
		m_metrics.lineCount = 0;
		m_visibleLines = 0;
		m_currLine = 0;
		m_linePos.clear();
	}
}

int MessageScrollBox::HitTest(D2D1_POINT_2F cursor)
{
	DWRITE_HIT_TEST_METRICS hitTestMetrics;
	BOOL isTrailingHit;
	BOOL isInside;

	m_pTextLayout->HitTestPoint(
		cursor.x - m_layoutRect.left,
		cursor.y - m_layoutRect.top + m_linePos[m_currLine],
		&isTrailingHit,
		&isInside,
		&hitTestMetrics
	);

	return hitTestMetrics.textPosition + isTrailingHit;
}

// Populates m_selectionMetrics based on iSelectStart and iSelectEnd
void MessageScrollBox::GetSelectionMetrics()
{
	UINT32 count;
	m_selectionMetrics.resize(m_metrics.lineCount * m_metrics.maxBidiReorderingDepth);
	HRESULT hr = m_pTextLayout->HitTestTextRange(
		min(m_iSelectStart, m_iSelectEnd),
		abs((int)m_iSelectEnd - (int)m_iSelectStart),
		m_layoutRect.left, // originX
		m_layoutRect.top, // originY
		m_selectionMetrics.data(),
		m_selectionMetrics.size(),
		&count
	);
	m_selectionMetrics.resize(count);

	if (hr == E_NOT_SUFFICIENT_BUFFER) // do again
	{
		HRESULT hr = m_pTextLayout->HitTestTextRange(
			min(m_iSelectStart, m_iSelectEnd),
			abs((int)m_iSelectEnd - (int)m_iSelectStart),
			m_layoutRect.left, // originX
			m_layoutRect.top, // originY
			m_selectionMetrics.data(),
			m_selectionMetrics.size(),
			&count
		);
	}

	if (FAILED(hr))
	{
		OutputError(L"MessageScrollBo::GetSelectionMetrics failed");
	}
}
