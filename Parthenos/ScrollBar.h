#pragma once
#include "stdafx.h"
#include "AppItem.h"
#include "Button.h"

class ScrollBar : public AppItem
{
public:
	// Statics
	static float const Width;

	ScrollBar(HWND hwnd, D2Objects const & d2, CTPMessageReceiver *parent);

	// AppItem
	void SetSize(D2D1_RECT_F dipRect); // width is fixed to 10 dips; only uses dipRect.right
	void Paint(D2D1_RECT_F updateRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);

	// Interface

	// minStep should be a fraction of WHEEL_DELTA, while the other steps indicate the number
	// of minSteps.
	inline void SetSteps(size_t minStep, size_t totalSteps, size_t visibleSteps, int initStep = 0)
	{
		m_minStep = minStep;
		m_totalSteps = (totalSteps < visibleSteps) ? visibleSteps : totalSteps;
		m_visibleSteps = visibleSteps;
		m_currPos = initStep;
	}
	inline void Refresh() { CalculateBarRect(); }

private:
	// Parent
	CTPMessageReceiver *m_parent;

	// State
	enum MouseOn { moNone, moUp, moBar, moDown };
	size_t m_minStep = WHEEL_DELTA; // minimum scroll step (fraction of WHEEL_DELTA), i.e. from clicking 
	size_t m_totalSteps = 0; // i.e. 0 would indicate no scrolling. Number of minSteps.
	size_t m_visibleSteps = 0; // affects size of scroll bar. Number of minSteps.
	int m_currPos = 0; // current position of top of scroll bar. Assert currStep + visibleSteps <= totalSteps. Int for easy bounds check
	MouseOn m_mouseOn = moNone;
	bool m_drag = false;
	float m_dragOffset; // when start dragging, offset in DIPs from top of bar

	// Objects
	IconButton m_upArrow;
	IconButton m_dnArrow;
	D2D1_RECT_F m_barRect;

	// Helpers
	void CalculateBarRect();

	// calculates the position, in minSteps, given a location on the scroll bar. 
	// offset is offset of y from top of bar
	int CalculatePos(float y, float offset);

	void ScrollTo(int newPos); // let this do bounds check
	MouseOn HitTest(float y);
};