#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "Button.h"

class ScrollBar : public AppItem
{
public:
	ScrollBar(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent);

	// AppItem
	void SetSize(D2D1_RECT_F dipRect); // width is fixed to 10 dips; only uses dipRect.right
	void Paint(D2D1_RECT_F updateRect);
	//bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	//bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	//void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);

	// Interface
	inline void SetSteps(int totalSteps, int visibleSteps, int initStep = 0)
	{
		m_totalSteps = totalSteps;
		m_visibleSteps = visibleSteps;
		m_currStep = initStep;
	}
	inline void Refresh() { CalculateBarRect(); }

private:
	// Parent
	CTPMessageReceiver *m_parent;

	// Parameters
	float const m_width = 15.0f;

	// State
	enum MouseOn { moNone, moUp, moBar, moDown };
	int m_totalSteps; // i.e. 0 would indicate no scrolling. In fractions/multiples of WHEEL_DELTA (120)
	int m_visibleSteps; // affects size of scroll bar
	int m_currStep; // current position of top of scroll bar. Assert currStep + visibleSteps <= totalSteps
	int m_mouseOn = moNone;

	// Objects
	IconButton m_upArrow;
	IconButton m_dnArrow;
	D2D1_RECT_F m_barRect;

	// Helpers
	void CalculateBarRect();
};