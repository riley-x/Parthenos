#pragma once
#include "stdafx.h"
#include "BaseWindow.h"

class Parthenos;

class TitleBar : public BaseWindow<TitleBar> 
{
public:
	static DWORD const title_bar_style = WS_CHILD; // Consider WS_CLIPSIBLINGS

	TitleBar(HWND hwnd_parent, HINSTANCE hInstance, Parthenos *parent);

	// Call TitleBar::Resize() after Create() 
	BOOL Create(int childID);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Resize(RECT pRect);

	HWND GetHWnd() { return m_hwnd; }

private:
	HWND		m_hWndParent;
	int			m_ID;
	Parthenos*	m_parent; // TODO: template this

	LRESULT OnPaint();
};