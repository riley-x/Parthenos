#include "stdafx.h"
#include "MenuBar.h"

MenuBar::MenuBar(HWND hwnd, D2Objects const & d2, Parthenos * parent)
	: AppItem(hwnd, d2), m_parent(parent)
{
	DropMenuButton *temp = new DropMenuButton(hwnd, d2, this, false);
	temp->SetText(m_texts[0], m_widths[0]);
	temp->SetItems({ L"TODO", L"Hello" });
	m_buttons.push_back(temp);

	temp = new DropMenuButton(hwnd, d2, this, false);
	temp->SetText(m_texts[1], m_widths[1]);
	temp->SetItems({ L"TODO", L"Hello" });
	m_buttons.push_back(temp);

	temp = new DropMenuButton(hwnd, d2, this, false);
	temp->SetText(m_texts[2], m_widths[2]);
	temp->SetItems({ L"Add" });
	m_buttons.push_back(temp);
}

MenuBar::~MenuBar()
{
	for (auto button : m_buttons) if (button) delete button;
}

void MenuBar::Paint(D2D1_RECT_F updateRect)
{
	if (overlapRect(m_dipRect, updateRect))
	{
		// Background
		m_d2.pBrush->SetColor(Colors::MAIN_BACKGROUND);
		m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);

		// Buttons
		for (int i = 0; i < m_nButtons; i++)
		{
			if (m_buttons[i]->IsActive())
			{
				D2D1_RECT_F rect = m_buttons[i]->GetDIPRect();
				m_d2.pBrush->SetColor(Colors::MENU_BACKGROUND);
				m_d2.pRenderTarget->FillRectangle(rect, m_d2.pBrush);
			}
			m_buttons[i]->Paint(updateRect);
		}
	}
	for (int i = 0; i < m_nButtons; i++)
	{
		m_buttons[i]->GetMenu().Paint(updateRect);
	}
}

void MenuBar::SetSize(D2D1_RECT_F dipRect)
{
	bool same_left = false;
	bool same_top = false;
	if (equalRect(m_dipRect, dipRect)) return;
	if (m_dipRect.left == dipRect.left) same_left = true;
	if (m_dipRect.top == dipRect.top) same_top = true;

	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(dipRect);

	if (!same_left || !same_top)
	{
		float left = m_dipRect.left;
		for (int i = 0; i < m_nButtons; i++)
		{
			m_buttons[i]->SetSize(D2D1::RectF(
				left,
				m_dipRect.top,
				left + m_widths[i],
				m_dipRect.bottom
			));
			left += m_widths[i];
		}
	}
}

void MenuBar::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam)
{
	for (auto button : m_buttons)
	{
		button->OnMouseMove(cursor, wParam);
	}
}

bool MenuBar::OnLButtonDown(D2D1_POINT_2F cursor)
{
	for (auto button : m_buttons)
	{
		button->OnLButtonDown(cursor);
	}
	ProcessMessages();
	return false;
}

void MenuBar::ProcessMessages()
{
	for (ClientMessage msg : m_messages)
	{
		if (msg.imsg != CTPMessage::DROPMENU_SELECTED) return;
		// Could == on the sender but not necessary if the names are distinct
		if (msg.msg == L"Add")
		{
			OutputMessage(L"hi!\n");
		}
		// else if ...
	}
	if (!m_messages.empty()) m_messages.clear();
}
