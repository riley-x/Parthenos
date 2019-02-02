#pragma once

#include "stdafx.h"
#include "resource.h"
#include "BorderlessWindow.h"
#include "TitleBar.h"


class Parthenos : public BorderlessWindow<Parthenos>
{
	TitleBar *m_titleBar;

	LRESULT	OnCreate();
	LRESULT OnPaint();
	LRESULT OnMouseMove(int pixelX, int pixelY, DWORD flags);
	LRESULT OnSize();
	LRESULT OnNCHitTest(int x, int y);
	//void	OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	//void	OnLButtonUp();
	//void	OnMouseMove(int pixelX, int pixelY, DWORD flags);

	//void    Resize();

public:

	ID2D1Factory            *pFactory;
	ID2D1HwndRenderTarget   *pRenderTarget;
	ID2D1SolidColorBrush    *pBrush;
	IDWriteFactory          *pDWriteFactory;
	IDWriteTextFormat		*pTextFormat;

	//D2D1_POINT_2F			ptMouseStart;
	HCURSOR					hCursor;

	Parthenos() : BorderlessWindow() {}
	Parthenos(PCWSTR szClassName) : BorderlessWindow(szClassName) {}
	~Parthenos() { delete m_titleBar; }

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	HRESULT CreateGraphicsResources();
	void    DiscardGraphicsResources();

};