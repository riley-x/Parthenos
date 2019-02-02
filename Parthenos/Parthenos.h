#pragma once

#include "stdafx.h"
#include "resource.h"
#include "BorderlessWindow.h"


class Parthenos : public BorderlessWindow<Parthenos>
{
	// Variables 

	ID2D1Factory            *pFactory;
	ID2D1HwndRenderTarget   *pRenderTarget;
	ID2D1SolidColorBrush    *pBrush;
	IDWriteFactory          *pDWriteFactory;
	IDWriteTextFormat		*pTextFormat;

	D2D1_POINT_2F			ptMouseStart;
	HCURSOR					hCursor;

	// Methods

	HRESULT CreateGraphicsResources();
	void    DiscardGraphicsResources();

	void    OnPaint();
	void	Resize();
	//void	OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	//void	OnLButtonUp();
	//void	OnMouseMove(int pixelX, int pixelY, DWORD flags);

	//void    Resize();

public:

	Parthenos() : BorderlessWindow(), pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
		pDWriteFactory(NULL), pTextFormat(NULL),
		ptMouseStart(D2D1::Point2F()), hCursor(NULL)
	{}

	Parthenos(PCWSTR szClassName) : BorderlessWindow(szClassName), pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
		pDWriteFactory(NULL), pTextFormat(NULL),
		ptMouseStart(D2D1::Point2F()), hCursor(NULL)
	{}

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};