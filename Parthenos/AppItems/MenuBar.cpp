#include "../stdafx.h"
#include "MenuBar.h"
#include "PopupMenu.h"

MenuBar::MenuBar(HWND hwnd, D2Objects const & d2, CTPMessageReceiver * parent, float height)
	: AppItem(hwnd, d2, parent), m_height(height)
{
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
		m_d2.pD2DContext->FillRectangle(m_dipRect, m_d2.pBrush);

		// Buttons
		for (size_t i = 0; i < m_buttons.size(); i++)
		{
			if (m_buttons[i]->IsActive())
			{
				D2D1_RECT_F rect = m_buttons[i]->GetDIPRect();
				m_d2.pBrush->SetColor(Colors::HIGHLIGHT);
				m_d2.pD2DContext->FillRectangle(rect, m_d2.pBrush);
			}
			m_buttons[i]->Paint(updateRect);
		}
	}
	for (size_t i = 0; i < m_buttons.size(); i++)
	{
		m_buttons[i]->GetMenu()->Paint(updateRect);
	}
}

void MenuBar::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(m_dipRect, dipRect)) return;
	
	bool same_left = false, same_top = false;
	if (m_dipRect.left == dipRect.left) same_left = true;
	if (m_dipRect.top == dipRect.top) same_top = true;

	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(dipRect);

	if (!same_left || !same_top) Refresh();
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
		if (msg.imsg == CTPMessage::DROPMENU_SELECTED)
		{
			for (size_t i = 0; i < m_buttons.size(); i++)
			{
				if (msg.sender == m_buttons[i])
				{
					m_parent->PostClientMessage(this, msg.msg, CTPMessage::MENUBAR_SELECTED, (int)i);
					break;
				}
			}
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

void MenuBar::SetMenus(std::vector<std::wstring> const & menus, 
	std::vector<std::vector<std::wstring>> const & items, 
	std::vector<std::vector<size_t>> const & divisions)
{
	if (menus.size() != items.size() || menus.size() != divisions.size()) return;
	
	for (auto button : m_buttons) if (button) delete button;
	m_buttons.clear();
	m_widths.clear();

	for (size_t i = 0; i < menus.size(); i++)
	{
		float width = 100.0f;

		// Create a temp layout to get width
		IDWriteTextLayout *layout;
		DWRITE_TEXT_METRICS metrics;
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			menus[i].c_str(),	// The string to be laid out and formatted.
			static_cast<UINT32>(menus[i].size()),	// The length of the string.
			m_d2.pTextFormats[D2Objects::Formats::Segoe12],	// The text format to apply to the string
			500,				// The width of the layout box (make extra large since left aligned anyways)
			m_height,			// The height of the layout box.
			&layout				// The IDWriteTextLayout interface pointer.
		);
		if (SUCCEEDED(hr))
		{
			hr = layout->GetMetrics(&metrics);
			if (SUCCEEDED(hr)) width = metrics.width + 2 * m_hpad;
			SafeRelease(&layout);
		}

		DropMenuButton *temp = new DropMenuButton(m_hwnd, m_d2, this, false);
		temp->SetText(menus[i], width, m_height);
		temp->SetItems(items[i]);
		temp->GetMenu()->SetDivisions(divisions[i]);
		m_buttons.push_back(temp);
		m_widths.push_back(width);
	}
}

void MenuBar::SetMenuItems(size_t i, std::vector<std::wstring> const & items, std::vector<size_t> const & divisions)
{
	if (i >= m_buttons.size()) return;

	m_buttons[i]->SetItems(items);
	m_buttons[i]->GetMenu()->SetDivisions(divisions);
}

void MenuBar::Refresh()
{
	float left = m_dipRect.left;
	for (size_t i = 0; i < m_buttons.size(); i++)
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
