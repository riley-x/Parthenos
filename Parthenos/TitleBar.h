#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "Button.h"

class Parthenos;

class TitleBar : public AppItem
{
public:
	// Statics
	enum class Buttons { NONE, CAPTION, MIN, MAXRESTORE, CLOSE, TAB };
	static float const iconHPad; // 6 DIPs

	// Constructors
	TitleBar(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent)
		: AppItem(hwnd, d2), m_parent(parent), m_tabButtons(hwnd, d2) {}
	~TitleBar() { SafeRelease(&m_bracketGeometries[0]); SafeRelease(&m_bracketGeometries[1]); }

	// AppItem overides
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	void OnMouseMoveP(POINT cursor, WPARAM wParam);
	bool OnLButtonDownP(POINT cursor);

	// Interface
	inline Buttons HitTest(POINT cursor) { std::wstring dummy; return HitTest(cursor, dummy); }
	Buttons HitTest(POINT cursor, std::wstring & name);
	void SetTabs(std::vector<std::wstring> const & tabNames);
	void SetActive(std::wstring name) { m_tabButtons.SetActive(name); }
	inline void SetMaximized(bool isMax) { m_maximized = isMax; }
	inline void SetMouseOn(Buttons button) 
	{
		if (!(button == Buttons::MIN || button == Buttons::MAXRESTORE || button == Buttons::CLOSE))
			button = Buttons::NONE;
		if (m_mouseOn != button)
		{
			m_mouseOn = button;
			InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
	}
	inline int pBottom() { return m_pixRect.bottom; }


private:
	// Objects
	CTPMessageReceiver	*m_parent;
	ButtonGroup			m_tabButtons;

	// Parameters
	static int const	nIcons			= 3;
	float const			m_tabLeftStart	= 100.0f;
	float const			m_tabWidth		= 150.0f;
	float const			m_dividerVPad	= 5.0f;
	float const			m_bracketWidth	= 3.0f;

	// Flags
	bool				m_maximized = false;
	Buttons				m_mouseOn	= Buttons::NONE; // for icon highlighting

	// Drawing
	D2D1_RECT_F	m_CommandIconRects[nIcons]; // 32x24 command icons. These are PIXEL coordinates, not DIPs!!!
	D2D1_RECT_F m_TitleIconRect; // 48x48 Parthenos icon. In DIP coordinates.
	ID2D1PathGeometry* m_bracketGeometries[2] = {}; // left, right

	// Helpers
	CTPMessage ButtonToCTPMessage(Buttons button);
	void CreateBracketGeometry(int i);

	// Deleted
	TitleBar(const TitleBar&) = delete; // non construction-copyable
	TitleBar& operator=(const TitleBar&) = delete; // non copyable

};