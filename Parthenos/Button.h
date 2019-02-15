#pragma once

#include "stdafx.h"
#include "AppItem.h"

class Button : public AppItem
{
public:
	using AppItem::AppItem;

	bool OnLButtonDown(D2D1_POINT_2F cursor);
	void OnLButtonUp(D2D1_POINT_2F cursor) { return; }

	std::wstring m_name;
protected:
	bool m_isDown = false; // pressed
};

class IconButton : public Button
{
public:
	using Button::Button;
	void Paint(D2D1_RECT_F updateRect);
	inline void SetIcon(size_t i) { m_iBitmap = i; } // index into d2.pD2DBitmaps
private:
	int m_iBitmap = -1;
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