#pragma once
#include "stdafx.h"
#include "resource.h"

inline int GetResourceIndex(int resource)
{
	switch (resource)
	{
	case IDB_CLOSE:
		return 0;
	case IDB_MAXIMIZE:
		return 1;
	case IDB_MINIMIZE:
		return 2;
	case IDB_RESTORE:
		return 3;
	case IDB_PARTHENOS_WHITE:
		return 4;
	case IDB_CANDLESTICK:
		return 5;
	case IDB_LINE:
		return 6;
	case IDB_ENVELOPE:
		return 7;
	case IDB_DOWNARROWHEAD:
		return 8;
	default:
		return -1;
	}
}

class D2Objects {
public:
	HWND					hwndParent				= NULL;

	////////////////////////////////////////////////////////
	// Data

	// WIC
	static const int nIcons = 9;
	int resource_ids[nIcons] = { IDB_CLOSE, IDB_MAXIMIZE, IDB_MINIMIZE, IDB_RESTORE, IDB_PARTHENOS_WHITE,
		IDB_CANDLESTICK, IDB_LINE, IDB_ENVELOPE, IDB_DOWNARROWHEAD };

	////////////////////////////////////////////////////////
	// Pointers

	// Rendering 
	ID2D1Factory            *pFactory				= NULL;
	ID2D1HwndRenderTarget   *pRenderTarget			= NULL;
	ID2D1SolidColorBrush    *pBrush					= NULL;
	ID2D1StrokeStyle		*pDashedStyle			= NULL;

	// Direct Write pointers
	IDWriteFactory          *pDWriteFactory			= NULL;
	IDWriteTextFormat		*pTextFormat_10p		= NULL; // 10 point (DIPs)
	IDWriteTextFormat		*pTextFormat_14p		= NULL; // 14 point (DIPs)
	IDWriteTextFormat		*pTextFormat_18p		= NULL; // 18 point (DIPs)

	// WIC pointers
	IWICImagingFactory      *pIWICFactory						= NULL;
	IWICFormatConverter		*pConvertedSourceBitmaps[nIcons]	= {}; 
	ID2D1Bitmap				*pD2DBitmaps[nIcons]				= {};
	
	////////////////////////////////////////////////////////
	// Functions

	HRESULT CreateDeviceIndependentResources();
	HRESULT LoadResourcePNG(int resource, IWICFormatConverter * pConvertedSourceBitmap);
	HRESULT	CreateGraphicsResources(HWND hwnd);
	void	DiscardDeviceIndependentResources();
	void	DiscardGraphicsResources();

};
