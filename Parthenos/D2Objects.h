#pragma once
#include "stdafx.h"
#include "resource.h"


class D2Objects {
public:
	HWND					hwndParent				= NULL;

	////////////////////////////////////////////////////////
	// Data

	// Direct Write
	static const int nFormats = 4;
	enum Formats { Segoe10, Segoe12, Segoe14, Segoe18 };

	// WIC
	static const int nIcons = 9;
	int resource_ids[nIcons] = { IDB_CLOSE, IDB_MAXIMIZE, IDB_MINIMIZE, IDB_RESTORE, IDB_PARTHENOS_WHITE,
		IDB_CANDLESTICK, IDB_LINE, IDB_ENVELOPE, IDB_DOWNARROWHEAD };

	////////////////////////////////////////////////////////
	// Pointers

	// Rendering 
	ID2D1Factory1           *pFactory				= NULL;
	ID2D1HwndRenderTarget   *pRenderTarget			= NULL;
	ID2D1SolidColorBrush    *pBrush					= NULL;
	ID2D1StrokeStyle		*pDashedStyle			= NULL;
	ID2D1StrokeStyle1		*pFixedTransformStyle	= NULL;

	// Direct Write pointers
	IDWriteFactory          *pDWriteFactory			= NULL;
	IDWriteTextFormat		*pTextFormats[nFormats] = {};

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

inline float FontSize(D2Objects::Formats font)
{
	switch (font)
	{
	case D2Objects::Segoe10:
		return 10.0f;
	case D2Objects::Segoe12:
		return 12.0f;
	case D2Objects::Segoe14:
		return 14.0f;
	case D2Objects::Segoe18:
		return 18.0f;
	default:
		OutputDebugString(L"FontSize font not recognized\n");
		return 0.0f;
	}
}