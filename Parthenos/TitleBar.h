#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "Button.h"

class Parthenos;

class TitleBar : public AppItem
{
public:
	TitleBar(HWND hwnd, D2Objects const & d2, Parthenos *parent)
		: AppItem(hwnd, d2), m_parent(parent), m_tabButtons(hwnd, d2) {}
	~TitleBar() { SafeRelease(&m_bracketGeometries[0]); SafeRelease(&m_bracketGeometries[1]); }

	void Init();
	void Resize(RECT pRect, D2D1_RECT_F pDipRect);
	void Paint(D2D1_RECT_F updateRect);
	void OnMouseMoveP(POINT cursor, WPARAM wParam);
	bool OnLButtonDownP(POINT cursor);

	enum class Buttons { NONE, CAPTION, MIN, MAXRESTORE, CLOSE, PORTFOLIO, CHART };

	Buttons HitTest(POINT cursor);
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
	inline void SetActiveTab(Buttons button)
	{
		m_tabButtons.SetActive(ButtonToWString(button));
	}

	static float const iconHPad; // 6 DIPs
	static float const height; // 30 DIPs

	int bottom() { return m_pixRect.bottom; }
	static std::wstring ButtonToWString(Buttons button);
	static Buttons WStringToButton(std::wstring name);

private:
	// Objects
	Parthenos			*m_parent;
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
};