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
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, int & selection, std::wstring & str); // don't need to pass handeled since always on top

	void Show(bool show = true);
	void SetItems(std::vector<std::wstring> const & items);
	void SetDivisions(std::vector<size_t> const & divs);
	inline std::wstring GetItem(size_t i) const { if (i < m_items.size()) return m_items[i]; else return L""; }
	inline IDWriteTextLayout* GetLayout(size_t i) const { if (i < m_pTextLayouts.size()) return m_pTextLayouts[i]; else return NULL; }

	float const m_vPad = 3.0f;
	float const m_fontSize = 12.0f;
	D2Objects::Formats const m_format = D2Objects::Segoe12;
private:
	// Data
	std::vector<std::wstring> m_items;

	// Flags
	bool	m_active;
	int		m_highlight = -1;

	// Paramters
	float	m_width;
	float	m_height;

	// Drawing
	D2D1_RECT_F m_borderRect;
	std::vector<IDWriteTextLayout*> m_pTextLayouts;
	std::vector<float> m_divisionYs;

	// Helpers

	// get DIP coordinate relative to m_dipRect.top of top of bounding box of item i.
	// top of text is at getTop(i) + m_vPad
	inline float getTop(size_t i)
	{
		return i * (m_fontSize + 2 * m_vPad);
	}

	// get index given offset in dips from m_dipRect.top
	inline int getInd(float y)
	{
		return static_cast<int>(y / (m_fontSize + 2 * m_vPad));
	}

	// Deleted
	PopupMenu(const PopupMenu&) = delete; // non construction-copyable
	PopupMenu& operator=(const PopupMenu&) = delete; // non copyable
};