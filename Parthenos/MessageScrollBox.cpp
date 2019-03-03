#include "stdafx.h"
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
	m_layoutRect.top = m_titleRect.bottom;
	m_layoutRect.left += 3.0f;
	m_layoutRect.right -= 3.0f + ScrollBar::Width;

	CreateTextLayout();

	// Set 1 line per minStep, 3 lines per detent
	m_scrollBar.SetSteps(WHEEL_DELTA / 3, m_metrics.lineCount, m_visibleLines);

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
		m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);

		// Title
		m_d2.pBrush->SetColor(Colors::AXES_BACKGROUND);
		m_d2.pRenderTarget->FillRectangle(m_titleRect, m_d2.pBrush);
		m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(m_titleRect.left, m_titleBorderY),
			D2D1::Point2F(m_titleRect.right, m_titleBorderY),
			m_d2.pBrush,
			1.0f,
			m_d2.pHairlineStyle
		);
		m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
		m_d2.pRenderTarget->DrawTextW(L" Output:", 7, m_d2.pTextFormats[D2Objects::Formats::Segoe12], m_titleRect, m_d2.pBrush);

		// Text
		if (m_pTextLayout)
		{
			m_d2.pRenderTarget->PushAxisAlignedClip(m_dipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			m_d2.pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, -m_linePos[m_currLine]));
			m_d2.pRenderTarget->DrawTextLayout(D2D1::Point2F(m_layoutRect.left, m_layoutRect.top), m_pTextLayout, m_d2.pBrush);
			m_d2.pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			m_d2.pRenderTarget->PopAxisAlignedClip();
		}
	}
	m_scrollBar.Paint(updateRect);
}

bool MessageScrollBox::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	bool out = m_scrollBar.OnMouseMove(cursor, wParam, handeled);
	if (!out)
	{
		// TODO
	}
	ProcessCTPMessages();
	return out;
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
	if (handeled || !inRect(cursor, m_dipRect)) return false;
	if (!m_scrollBar.OnLButtonDown(cursor, handeled))
	{
		// TODO
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
	ProcessCTPMessages();
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
			m_scrollCapturedMouse = msg.iData;
			m_parent->PostClientMessage(this, msg.msg, msg.imsg, msg.iData, msg.dData);
		default:
			break;
		}
	}

	if (!m_messages.empty()) m_messages.clear();
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
				m_visibleLines = static_cast<size_t>(roundf((m_dipRect.bottom - m_dipRect.top) / avgHeight));
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
