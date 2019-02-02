#pragma once
#include "stdafx.h"

class D2Objects {
public:
	ID2D1Factory            *pFactory;
	ID2D1HwndRenderTarget   *pRenderTarget;
	ID2D1SolidColorBrush    *pBrush;
	IDWriteFactory          *pDWriteFactory;
	IDWriteTextFormat		*pTextFormat;

	HRESULT CreateFactories();
	HRESULT	CreateGraphicsResources(HWND hwnd);
	void	DiscardFactories();
	void	DiscardGraphicsResources();
};
