#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "Button.h"

class Parthenos;


class MenuBar : public AppItem
{
public:
	MenuBar(HWND hwnd, D2Objects const & d2, Parthenos *parent, float height);
	~MenuBar();

	// AppItem overrides
	void Paint(D2D1_RECT_F updateRect);
	void SetSize(D2D1_RECT_F dipRect);
	void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnLButtonDown(D2D1_POINT_2F cursor);
	void ProcessMessages();

private:
	// Objects
	Parthenos					 *m_parent;
	std::vector<DropMenuButton*> m_buttons;

	// Parameters
	static const int	m_nButtons = 3;
	const std::wstring  m_texts[m_nButtons] = { L"File", L"Account", L"Transaction" };
	const float			m_widths[m_nButtons] = { 30.0f, 60.0f, 70.0f };

	// Drawing
	float m_rBorder;

	// Deleted
	MenuBar(const MenuBar&) = delete; // non construction-copyable
	MenuBar& operator=(const MenuBar&) = delete; // non copyable
};