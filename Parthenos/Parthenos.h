#pragma once

#include "stdafx.h"
#include "resource.h"
#include "basewin.h"


class Parthenos : public BaseWindow<Parthenos>
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
	//void	OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	//void	OnLButtonUp();
	//void	OnMouseMove(int pixelX, int pixelY, DWORD flags);

	//void    Resize();

public:

	Parthenos() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
		pDWriteFactory(NULL), pTextFormat(NULL),
		ptMouseStart(D2D1::Point2F()), hCursor(NULL)
	{}

	Parthenos(PCWSTR szClassName) : BaseWindow(szClassName), pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
		pDWriteFactory(NULL), pTextFormat(NULL),
		ptMouseStart(D2D1::Point2F()), hCursor(NULL)
	{}

	//PCWSTR  ClassName() const { return L"Parthenos"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};