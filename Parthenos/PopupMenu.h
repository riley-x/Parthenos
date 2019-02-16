#pragma once

#include "stdafx.h"
#include "AppItem.h"


class PopupMenu : public AppItem
{
public:
	using AppItem::AppItem;
	~PopupMenu();
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	void OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnLButtonDown(D2D1_POINT_2F cursor, int & selection, std::wstring & str);

	void Show(bool show = true);
	void SetItems(std::vector<std::wstring> const & items);
	inline IDWriteTextLayout* GetLayout(size_t i) { if (i < m_pTextLayouts.size()) return m_pTextLayouts[i]; else return NULL; }
private:
	std::vector<std::wstring> m_items;
	std::vector<IDWriteTextLayout*> m_pTextLayouts;

	int m_highlight = -1;
	float m_width;
	float m_height;
	float const m_vPad = 2.0f;
	float const m_fontSize = 14.0f;
	bool m_active;
};