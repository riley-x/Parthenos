#pragma once
#include "../stdafx.h"
#include "BorderlessWindow.h"
#include "../Utilities/utilities.h"
#include "../Utilities/D2Objects.h"
#include "../AppItems/AppItem.h"
#include "../AppItems/TitleBar.h"


class PopupWindow : public BorderlessWindow<PopupWindow>, public CTPMessageReceiver
{
public:
	PopupWindow(std::wstring const & name) : BorderlessWindow(L"PARTHENOSPOPUP"), m_name(name) {}
	virtual ~PopupWindow();

	BOOL Create(HINSTANCE hInstance);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	virtual void PreShow() = 0; // Handle AppItem creation here
	inline void SetParent(CTPMessageReceiver *parent, HWND phwnd) { m_parent = parent; m_phwnd = phwnd; }

protected:
	// Parent
	HWND					m_phwnd;
	CTPMessageReceiver		*m_parent;

	// Objects
	std::vector<AppItem*>	m_items;
	TitleBar				*m_titleBar;
	Timers::WndTimers		m_timers;

	// Resources 
	D2Objects m_d2;

	// Layout
	D2D1_RECT_F	m_dipRect;
	float const	m_titleBarHeight = 30.0f;

	// Helpers
	bool ProcessPopupWindowCTP(ClientMessage const & msg);

	// Virtuals
	virtual void PaintSelf(D2D1_RECT_F windowRect, D2D1_RECT_F updateRect) = 0;
	virtual void ProcessCTPMessages() = 0;

private:
	// State
	std::wstring m_name;

	// Flags
	bool m_created = false; // only allow create once
	AppItem *m_mouseCaptured = nullptr;

	// Resources
	MouseTrackEvents m_mouseTrack;

	// Message responses
	LRESULT	OnCreate();
	LRESULT OnNCHitTest(POINT cursor);
	LRESULT OnSize(WPARAM wParam);
	LRESULT	OnPaint();
	LRESULT OnMouseMove(POINT cursor, WPARAM wParam);
	LRESULT OnMouseWheel(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonDown(POINT cursor, WPARAM wParam);
	LRESULT OnLButtonDblclk(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonUp(POINT cursor, WPARAM wParam);
	LRESULT OnChar(wchar_t c, LPARAM lParam);
	bool	OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
};


