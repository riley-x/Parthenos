#pragma once
#include "stdafx.h"


class D2Objects {
public:
	HWND					hwndParent				= NULL;

	ID2D1Factory            *pFactory				= NULL;
	ID2D1HwndRenderTarget   *pRenderTarget			= NULL;
	ID2D1SolidColorBrush    *pBrush					= NULL;

	// Direct Write pointers
	IDWriteFactory          *pDWriteFactory			= NULL;
	IDWriteTextFormat		*pTextFormat			= NULL;

	// WIC pointers
	static const int nIcons = 5; // Close, Max, Min, Restore, Parthenos
	IWICImagingFactory      *pIWICFactory						= NULL;
	IWICFormatConverter		*pConvertedSourceBitmaps[nIcons]	= {NULL}; 
	ID2D1Bitmap				*pD2DBitmaps[nIcons]				= {NULL};
	
	HRESULT CreateFactories();
	HRESULT LoadResourcePNG(int resource, IWICFormatConverter * pConvertedSourceBitmap);
	HRESULT	CreateGraphicsResources(HWND hwnd);
	void	DiscardFactories();
	void	DiscardGraphicsResources();

};
