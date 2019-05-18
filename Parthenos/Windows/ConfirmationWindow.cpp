#include "../stdafx.h"
#include "ConfirmationWindow.h"

void ConfirmationWindow::PreShow()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");
	D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

	m_center = (dipRect.left + dipRect.right) / 2.0f;

	// Create AppItems
	m_titleBar = new TitleBar(m_hwnd, m_d2, this);
	m_ok = new TextButton(m_hwnd, m_d2, this);
	m_cancel = new TextButton(m_hwnd, m_d2, this);

	m_items = { m_titleBar, m_ok, m_cancel };

	m_ok->SetName(L"Ok");
	m_cancel->SetName(L"Cancel");

	for (auto item : m_items)
	{
		item->SetSize(CalculateItemRect(item, dipRect));
	}
}


void ConfirmationWindow::PaintSelf(D2D1_RECT_F windowRect, D2D1_RECT_F updateRect)
{
	// Repaint everything
	m_d2.pD2DContext->Clear(Colors::MAIN_BACKGROUND);
	for (auto item : m_items)
		item->Paint(windowRect);

	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_d2.pD2DContext->DrawTextW(
		m_text.c_str(),
		m_text.size(),
		m_d2.pTextFormats[D2Objects::Formats::Segoe12],
		D2D1::RectF(windowRect.left, m_titleBarHeight, windowRect.right, windowRect.bottom - m_buttonVPad - m_buttonHeight),
		m_d2.pBrush
	);
	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}


void ConfirmationWindow::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		if (ProcessPopupWindowCTP(msg)) continue;

		switch (msg.imsg)
		{
		case CTPMessage::BUTTON_DOWN:
			if (msg.sender == m_ok)
			{
				m_parent->PostClientMessage(m_msg);
				PostMessage(m_phwnd, WM_APP, 0, 0);
			}
			SendMessage(m_hwnd, WM_CLOSE, 0, 0); // Close on both ok and close
			break;
		default:
			OutputMessage(L"Unknown message %d received\n", static_cast<int>(msg.imsg));
			break;
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

D2D1_RECT_F ConfirmationWindow::CalculateItemRect(AppItem *item, D2D1_RECT_F const & dipRect)
{
	if (item == m_titleBar)
	{
		return D2D1::RectF(
			0.0f,
			0.0f,
			dipRect.right,
			DPIScale::SnapToPixelY(m_titleBarHeight)
		);
	}
	else if (item == m_ok)
	{
		return D2D1::RectF(
			m_center - m_buttonHPad - m_buttonWidth,
			dipRect.bottom - m_buttonVPad - m_buttonHeight,
			m_center - m_buttonHPad,
			dipRect.bottom - m_buttonVPad
		);
	}
	else if (item == m_cancel)
	{
		return D2D1::RectF(
			m_center + m_buttonHPad,
			dipRect.bottom - m_buttonVPad - m_buttonHeight,
			m_center + m_buttonHPad + m_buttonWidth,
			dipRect.bottom - m_buttonVPad
		);
	}
	else
	{
		return D2D1::RectF(0, 0, 0, 0);
	}
}