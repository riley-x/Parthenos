#pragma once
#include "../stdafx.h"
#include "AppItem.h"
#include "Button.h"

class ScrollBar : public AppItem
{
public:
	// Statics
	static float const Width;
	enum class SetPosMethod {Top, Bottom, MaintainOffsetTop, MaintainOffsetBottom};

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

	// minStep should be a fraction of WHEEL_DELTA
	inline void SetMinStep(int minStep) { m_minStep = minStep; }

	// steps are number of minSteps. returns current position
	size_t SetSteps(size_t totalSteps, size_t visibleSteps, SetPosMethod mtd = SetPosMethod::Top);
	inline void Refresh() { CalculateBarRect(); }

	// use this when scroll from something other than ScrollBar, like mouse selection.
	// steps is number of minSteps. returns new position. does not post message to parent.
	inline size_t Scroll(int steps)
	{
		ScrollTo(m_currPos + steps, false);
		return static_cast<size_t>(m_currPos);
	}

private:
	// State
	enum MouseOn { moNone, moUp, moBar, moDown };
	int m_minStep = WHEEL_DELTA; // minimum scroll step (fraction of WHEEL_DELTA), i.e. from clicking 
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

	void ScrollTo(int newPos, bool message = true); // let this do bounds check
	MouseOn HitTest(float y);
};