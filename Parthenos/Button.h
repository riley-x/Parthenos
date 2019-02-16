#pragma once

#include "stdafx.h"
#include "AppItem.h"
#include "PopupMenu.h"

class Button : public AppItem
{
public:
	using AppItem::AppItem;

	bool OnLButtonDown(D2D1_POINT_2F cursor);
	void OnLButtonUp(D2D1_POINT_2F cursor) { return; }
	virtual void SetClickRect(D2D1_RECT_F rect) { return; }

	std::wstring m_name;
protected:
	bool m_isDown = false; // pressed
};

class IconButton : public Button
{
public:
	using Button::Button;
	void Paint(D2D1_RECT_F updateRect);
	bool OnLButtonDown(D2D1_POINT_2F cursor);
	inline void SetIcon(size_t i) { m_iBitmap = i; } // index into d2.pD2DBitmaps
	inline void SetClickRect(D2D1_RECT_F rect) { m_clickRect = rect; }
private:
	int m_iBitmap = -1;
	D2D1_RECT_F m_clickRect; // include padding
};

// Contains buttons that mutually exclude each other
class ButtonGroup : public AppItem
{
public:
	using AppItem::AppItem;
	~ButtonGroup() { for (auto button : m_buttons) if (button) delete button; }

	inline void Add(Button * button) { m_buttons.push_back(button); }
	inline size_t Size() const { return m_buttons.size(); } 

	inline void Paint(D2D1_RECT_F updateRect) { for (auto button : m_buttons) button->Paint(updateRect); }
	inline bool OnLButtonDown(D2D1_POINT_2F cursor, std::wstring & name)
	{
		for (int i = 0; i < static_cast<int>(m_buttons.size()); i++)
		{
			if (m_buttons[i]->OnLButtonDown(cursor))
			{
				name = m_buttons[i]->m_name;
				m_active = i;
				return true;
			}
		}
		return false;
	}

	inline void SetActive(int i) 
	{ 
		if (i < static_cast<int>(m_buttons.size())) m_active = i; 
		else OutputMessage(L"%d exceeds ButtonGroup size\n", i);
	}
	inline void SetActive(std::wstring const & name)
	{
		for (size_t i = 0; i < m_buttons.size(); i++)
		{
			if (m_buttons[i]->m_name == name)
			{
				m_active = i;
				return;
			}
		}
		OutputMessage(L"Button %s not found\n", name);
	}
	inline void SetSize(size_t i, D2D1_RECT_F const & rect)
	{
		if (i < m_buttons.size()) m_buttons[i]->SetSize(rect);
		else OutputMessage(L"%u exceeds ButtonGroup size\n", i);
	}
	inline void SetClickRect(size_t i, D2D1_RECT_F rect) 
	{ 
		if (i < m_buttons.size()) m_buttons[i]->SetClickRect(rect);
		else OutputMessage(L"%u exceeds ButtonGroup size\n", i);
	}
	inline bool GetActiveRect(D2D1_RECT_F & rect) const
	{ 
		if (m_active >= 0)
		{
			rect = m_buttons[m_active]->GetDIPRect();
			return true;
		}
		else return false;
	}

private:
	int m_active;
	std::vector<Button*> m_buttons;
};

// Creates a drop down menu when clicked
class DropMenuButton : public Button
{
public:
	DropMenuButton(HWND hwnd, D2Objects const & d2, AppItem *parent) :
		Button(hwnd, d2), m_menu(hwnd, d2), m_parent(parent) {};
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	inline void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam) { m_menu.OnMouseMove(cursor, wParam); }
	bool OnLButtonDown(D2D1_POINT_2F cursor);

	inline void SetText(std::wstring text) { m_text = text; }
	inline void SetItems(std::vector<std::wstring> const & items) { m_menu.SetItems(items); }
	void SetActive(size_t i);
private:
	PopupMenu m_menu;
	AppItem *m_parent;
	std::wstring m_text;

	D2D1_RECT_F m_textRect;
	D2D1_RECT_F m_iconRect;
	IDWriteTextLayout* m_activeLayout;
};