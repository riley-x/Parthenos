#include "stdafx.h"
#include "MenuBar.h"
#include "PopupMenu.h"

MenuBar::MenuBar(HWND hwnd, D2Objects const & d2, CTPMessageReceiver * parent, 
	std::vector<std::wstring> accounts, float height)
	: AppItem(hwnd, d2), m_parent(parent)
{
	DropMenuButton *temp = new DropMenuButton(hwnd, d2, this, false);
	temp->SetText(m_texts[0], m_widths[0], height);
	temp->SetItems({ L"TODO", L"HIII" });
	m_buttons.push_back(temp);

	accounts.push_back(L"All");
	temp = new DropMenuButton(hwnd, d2, this, false);
	temp->SetText(m_texts[1], m_widths[1], height);
	temp->SetItems(accounts);
	temp->GetMenu().SetDivisions({ accounts.size() - 1 });
	m_buttons.push_back(temp);

	temp = new DropMenuButton(hwnd, d2, this, false);
	temp->SetText(m_texts[2], m_widths[2], height);
	temp->SetItems({ L"Add...", L"Print History", L"Recalculate All" });
	temp->GetMenu().SetDivisions({ 1 });
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
		m_d2.pBrush->SetColor(Colors::TITLE_BACKGROUND);
		m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);

		// Buttons
		for (int i = 0; i < m_nButtons; i++)
		{
			if (m_buttons[i]->IsActive())
			{
				D2D1_RECT_F rect = m_buttons[i]->GetDIPRect();
				m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
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

bool MenuBar::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	for (auto button : m_buttons)
	{
		handeled = button->OnMouseMove(cursor, wParam, handeled) || handeled;
	}
	return handeled;
}

bool MenuBar::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	for (auto button : m_buttons)
	{
		handeled = button->OnLButtonDown(cursor, handeled) || handeled;
	}
	ProcessCTPMessages();
	return handeled;
}

void MenuBar::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		if (msg.imsg != CTPMessage::DROPMENU_SELECTED); // clear below
		else if (msg.sender == m_buttons[1]) // Account
			m_parent->PostClientMessage(this, msg.msg, CTPMessage::MENUBAR_ACCOUNT);
		else if (msg.msg == L"Add...") // Transaction->Add...
			m_parent->PostClientMessage(this, msg.msg, CTPMessage::MENUBAR_ADDTRANSACTION);
		else if (msg.msg == L"Print History") // Transaction->Print History
			m_parent->PostClientMessage(this, msg.msg, CTPMessage::MENUBAR_PRINTTRANSACTIONS);
		else if (msg.msg == L"Recalculate All") // Transaction->Recalculate
			m_parent->PostClientMessage(this, msg.msg, CTPMessage::MENUBAR_CALCHOLDINGS);
	}
	if (!m_messages.empty()) m_messages.clear();
}
