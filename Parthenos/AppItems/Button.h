#pragma once

#include "../stdafx.h"
#include "AppItem.h"
#include "PopupMenu.h"

class Button : public AppItem
{
public:
	// AppItem
	using AppItem::AppItem;
	virtual ~Button() {}

	virtual bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);

	// Interface
	virtual inline bool HitTest(D2D1_POINT_2F cursor) { return inRect(cursor, m_dipRect); }
	virtual inline void SetClickRect(D2D1_RECT_F rect) { return; }
	virtual inline D2D1_RECT_F GetClickRect() { return m_dipRect; }
	void SetName(std::wstring name) { m_name = name; }
	std::wstring Name() const { return m_name; }

protected:
	std::wstring m_name;
};

class IconButton : public Button
{
public:
	// AppItem
	using Button::Button;
	void Paint(D2D1_RECT_F updateRect);
	inline bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled) 
	{
		if (handeled) return false;
		return inRect(cursor, m_clickRect);
	}

	// Interface
	inline void SetIcon(int i) { m_iBitmap = i; } // index into d2.pD2DBitmaps
	inline void SetClickRect(D2D1_RECT_F rect) { m_clickRect = rect; }
	inline D2D1_RECT_F GetClickRect() { return m_clickRect; }

private:
	int m_iBitmap = -1;
	D2D1_RECT_F m_clickRect; // include padding
};

// Contains buttons that mutually exclude each other
class ButtonGroup : public AppItem
{
public:
	// AppItem
	using AppItem::AppItem;
	~ButtonGroup() { for (auto button : m_buttons) if (button) delete button; }
	inline void Paint(D2D1_RECT_F updateRect) { for (auto button : m_buttons) button->Paint(updateRect); }

	// Interface
	inline void Add(Button * button) { m_buttons.push_back(button); }
	inline size_t Size() const { return m_buttons.size(); } 
	inline std::vector<Button*> Buttons() const { return m_buttons; };

	// Does not allow clicking the current active button
	inline bool OnLButtonDown(D2D1_POINT_2F cursor, std::wstring & name, bool handeled)
	{
		if (handeled) return false;
		for (int i = 0; i < static_cast<int>(m_buttons.size()); i++)
		{
			if (i != m_active && m_buttons[i]->OnLButtonDown(cursor, handeled))
			{
				name = m_buttons[i]->Name();
				m_active = i;
				return true;
			}
		}
		return false;
	}

	inline bool HitTest(D2D1_POINT_2F cursor, std::wstring & name)
	{
		for (int i = 0; i < static_cast<int>(m_buttons.size()); i++)
		{
			if (m_buttons[i]->HitTest(cursor))
			{
				name = m_buttons[i]->Name();
				return true;
			}
		}
		return false;
	}

	inline bool SetActive(int i) 
	{ 
		if (i < static_cast<int>(m_buttons.size()))
		{
			if (i == m_active) return false;
			m_active = i;
			return true;
		}
		else
		{
			OutputMessage(L"%d exceeds ButtonGroup size\n", i);
			return false;
		}
	}
	inline bool SetActive(std::wstring const & name)
	{
		for (int i = 0; i < static_cast<int>(m_buttons.size()); i++)
		{
			if (m_buttons[i]->Name() == name)
			{
				if (m_active == i) return false;
				m_active = i;
				return true;
			}
		}
		OutputMessage(L"Button %s not found\n", name);
		return false;
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
	inline bool GetActive(Button* & item) const
	{
		if (m_active >= 0)
		{
			item = m_buttons[m_active];
			return true;
		}
		return false;
	}
	inline bool GetActiveRect(D2D1_RECT_F & rect) const
	{ 
		if (m_active >= 0)
		{
			rect = m_buttons[m_active]->GetDIPRect();
			return true;
		}
		return false;
	}
	inline bool GetActiveClickRect(D2D1_RECT_F & rect) const
	{
		if (m_active >= 0)
		{
			rect = m_buttons[m_active]->GetClickRect();
			return true;
		}
		return false;
	}

private:
	ButtonGroup(const ButtonGroup&) = delete; // non construction-copyable
	ButtonGroup& operator=(const ButtonGroup&) = delete; // non copyable

	int m_active = -1;
	std::vector<Button*> m_buttons;
};

// Creates a drop down menu when clicked. Initialize with SetItems and either SetText or SetActive, before SetSize().
// Make sure the owner of the button calls GetMenu()->Paint() so the popup can be on top.
class DropMenuButton : public Button
{
public:
	// Constructors
	DropMenuButton(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent, 
		bool dynamic = true, bool border = true) 
		: Button(hwnd, d2, parent), m_menu(hwnd, d2, this), m_dynamic(dynamic), m_border(border) {}
	~DropMenuButton() { if (!m_dynamic) SafeRelease(&m_activeLayout); }
	
	// AppItem overrides
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect); // owner of button should call paint on popup
	inline bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled = false) { return m_menu.OnMouseMove(cursor, wParam, handeled); }
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);

	// Interface
	void SetText(std::wstring text, float width, float height); // For non-dynamic, the text of the button
	inline void SetItems(std::vector<std::wstring> const & items) { m_menu.SetItems(items); }
	void SetActive(int i);
	inline void SetBorder(bool border) { m_border = border; }
	void SetOrientationDown(bool down);
	void Deactivate(); // Hides the menu

	inline int GetSelection() const { return m_activeSelection; }
	inline std::wstring GetText() const { return m_name; }
	inline PopupMenu* GetMenu() { return &m_menu; }
	inline bool IsActive() const { return m_active; }

private:
	// Objects
	PopupMenu m_menu;

	// State
	int m_activeSelection = 0;
	IDWriteTextLayout* m_activeLayout; // text stored in m_name

	// Flags
	bool m_active = false;
	bool m_menuDown = true;
	bool m_border;
	bool m_dynamic; // Does the text change with selection?

	// Layout
	D2D1_RECT_F m_textRect;
	D2D1_RECT_F m_iconRect; // down arrow
};

class TextButton : public Button
{
public:
	using Button::Button;

	void Paint(D2D1_RECT_F updateRect);

	inline void SetBorderColor(bool hasBorder, D2D1_COLOR_F color = Colors::MEDIUM_LINE)
	{
		m_border = hasBorder; 
		m_borderColor = color;
	}
	inline void SetFillColor(bool hasFill, D2D1_COLOR_F color)
	{
		m_fill = hasFill;
		m_fillColor = color;
	}
	inline void SetFormat(D2Objects::Formats format) { m_format = format; }
	inline void SetTextColor(D2D1_COLOR_F color) { m_textColor = color; }
private:
	bool m_border = true;
	bool m_fill = false;

	D2D1_COLOR_F m_borderColor = Colors::MEDIUM_LINE;
	D2D1_COLOR_F m_fillColor;
	D2D1_COLOR_F m_textColor = Colors::MAIN_TEXT;
	D2Objects::Formats m_format = D2Objects::Segoe12;
};