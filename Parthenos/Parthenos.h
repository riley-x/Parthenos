#pragma once

#include "stdafx.h"
#include "resource.h"
#include "BorderlessWindow.h"
#include "D2Objects.h"
#include "TitleBar.h"


class Parthenos : public BorderlessWindow<Parthenos>
{
	TitleBar m_titleBar;
	D2Objects m_d2;

	LRESULT	OnCreate();
	LRESULT OnPaint();
	LRESULT OnMouseMove(POINT mouse, DWORD flags);
	LRESULT OnSize();
	LRESULT OnNCHitTest(POINT cursor);
	//void	OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	//void	OnLButtonUp();
	//void	OnMouseMove(int pixelX, int pixelY, DWORD flags);

	//void    Resize();

public:

	//D2D1_POINT_2F			ptMouseStart;
	HCURSOR					hCursor;

	Parthenos() : BorderlessWindow() {}
	Parthenos(PCWSTR szClassName) : BorderlessWindow(szClassName) {}

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	// Handle initializations here instead of create
	// ClientRect doesn't have correct size during WM_CREATE
	void PreShow(); 

};