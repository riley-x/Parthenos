#pragma once
#include "stdafx.h"


class D2Objects {
public:
	ID2D1Factory            *pFactory				= NULL;
	ID2D1HwndRenderTarget   *pRenderTarget			= NULL;
	ID2D1SolidColorBrush    *pBrush					= NULL;

	IDWriteFactory          *pDWriteFactory			= NULL;
	IDWriteTextFormat		*pTextFormat			= NULL;

	// WIC interface pointers.
	// TODO: ADD DISCARDS
	IWICImagingFactory      *pIWICFactory			= NULL;
	IWICStream				*pIWICStream			= NULL;
	IWICBitmapDecoder		*pIDecoder				= NULL;
	IWICBitmapFrameDecode	*pIDecoderFrame			= NULL;
	IWICFormatConverter		*pConvertedSourceBitmap = NULL;
	ID2D1Bitmap				*pD2DBitmap				= NULL;
	
	HRESULT CreateFactories();
	HRESULT	CreateGraphicsResources(HWND hwnd);
	void	DiscardFactories();
	void	DiscardGraphicsResources();
};
