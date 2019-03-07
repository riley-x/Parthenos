#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "Button.h"


class MenuBar : public AppItem
{
public:
	MenuBar(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent, float height);
	~MenuBar();

	// AppItem overrides
	void Paint(D2D1_RECT_F updateRect);
	void SetSize(D2D1_RECT_F dipRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void ProcessCTPMessages();

	// Interface
	void SetMenus(std::vector<std::wstring> const & menus, 
		std::vector<std::vector<std::wstring>> const & items,
		std::vector<std::vector<size_t>> const & divisions);
	void Refresh();

private:
	// Objects
	CTPMessageReceiver			 *m_parent;
	std::vector<DropMenuButton*> m_buttons;

	// Layout
	float const m_height;
	float const m_hpad = 7.0f;
	std::vector<float> m_widths;

	// Deleted
	MenuBar(const MenuBar&) = delete; // non construction-copyable
	MenuBar& operator=(const MenuBar&) = delete; // non copyable
};