#include "stdafx.h"
#include "D2Objects.h"
#include "utilities.h"

HRESULT D2Objects::CreateDeviceIndependentResources()
{
	// Create a Direct2D factory
	//HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
	HRESULT hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED, 
		__uuidof(ID2D1Factory1),
		NULL, 
		reinterpret_cast<void**>(&pFactory)
	);

	// Initialize DPI settings
	if (SUCCEEDED(hr))
	{
		DPIScale::Initialize(pFactory);
	}
	// Create dashed stroke style
	if (SUCCEEDED(hr))
	{
		D2D1_STROKE_STYLE_PROPERTIES strokeStyleProperties = D2D1::StrokeStyleProperties(
			D2D1_CAP_STYLE_FLAT,		// The start cap.
			D2D1_CAP_STYLE_FLAT,		// The end cap.
			D2D1_CAP_STYLE_FLAT,		// The dash cap.
			D2D1_LINE_JOIN_MITER,		// The line join.
			10.0f,						// The miter limit.
			D2D1_DASH_STYLE_DASH,		// The dash style.
			0.0f						// The dash offset.
		);

		hr = pFactory->CreateStrokeStyle(strokeStyleProperties, NULL, 0, &pDashedStyle);
	}
	// Create fixed transform style
	if (SUCCEEDED(hr))
	{
		D2D1_STROKE_STYLE_PROPERTIES1 strokeStyleProperties = D2D1::StrokeStyleProperties1(
			D2D1_CAP_STYLE_FLAT,				// The start cap.
			D2D1_CAP_STYLE_FLAT,				// The end cap.
			D2D1_CAP_STYLE_FLAT,				// The dash cap.
			D2D1_LINE_JOIN_MITER,				// The line join.
			10.0f,								// The miter limit.
			D2D1_DASH_STYLE_SOLID,				// The dash style.
			0.0f,								// The dash offset.
			D2D1_STROKE_TRANSFORM_TYPE_FIXED	// The transform type.
		);

		hr = pFactory->CreateStrokeStyle(strokeStyleProperties, NULL, 0, &pFixedTransformStyle);
	}
	// Create hairline style
	if (SUCCEEDED(hr))
	{
		D2D1_STROKE_STYLE_PROPERTIES1 strokeStyleProperties = D2D1::StrokeStyleProperties1(
			D2D1_CAP_STYLE_FLAT,				// The start cap.
			D2D1_CAP_STYLE_FLAT,				// The end cap.
			D2D1_CAP_STYLE_FLAT,				// The dash cap.
			D2D1_LINE_JOIN_MITER,				// The line join.
			10.0f,								// The miter limit.
			D2D1_DASH_STYLE_SOLID,				// The dash style.
			0.0f,								// The dash offset.
			D2D1_STROKE_TRANSFORM_TYPE_HAIRLINE	// The transform type.
		);

		hr = pFactory->CreateStrokeStyle(strokeStyleProperties, NULL, 0, &pHairlineStyle);
	}
	// Create a DirectWrite factory
	if (SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(pDWriteFactory),
			reinterpret_cast<IUnknown **>(&pDWriteFactory)
		);
	}
	// Create text formats
	for (int i = 0; i < nFormats; i++)
	{
		IDWriteTextFormat **ppTextFormat = &pTextFormats[i];
		if (SUCCEEDED(hr))
		{
			hr = pDWriteFactory->CreateTextFormat(
				L"Segoe UI",
				NULL,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				FontSize(static_cast<D2Objects::Formats>(i)),
				L"", //locale
				ppTextFormat
			);
		}
		// Set default alignment
		if (SUCCEEDED(hr))
		{
			if (i == Segoe10)
			{
				(*ppTextFormat)->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				(*ppTextFormat)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			}
			else if (i == Segoe18)
			{
				(*ppTextFormat)->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				(*ppTextFormat)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			}
			else
			{
				(*ppTextFormat)->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				(*ppTextFormat)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			}
		}
	}
	// Create WIC factory
	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pIWICFactory)
		);
	}
	// Create source bitmaps for icons
	// This contains the converted bitmap data (?) and should be kept
	for (int i = 0; i < nIcons; i++)
	{
		if (SUCCEEDED(hr))
			hr = pIWICFactory->CreateFormatConverter(&pConvertedSourceBitmaps[i]);
	}
	
	// Initialize the converted bitmaps
	for (int i = 0; i < nIcons; i++)
	{
		if (SUCCEEDED(hr))
			hr = LoadResourcePNG(resource_ids[i], pConvertedSourceBitmaps[i]);
	}
	return hr;
}

HRESULT D2Objects::LoadResourcePNG(int resource, IWICFormatConverter *pConvertedSourceBitmap)
{
	// https://docs.microsoft.com/en-us/windows/desktop/wic/-wic-bitmapsources-howto-loadfromresource

	// Variables
	HRSRC					imageResHandle		= NULL;
	DWORD					imageFileSize;
	HGLOBAL					imageResDataHandle	= NULL;
	void					*pImageFile			= NULL;
	IWICStream				*pIWICStream		= NULL;
	IWICBitmapDecoder		*pIDecoder			= NULL;
	IWICBitmapFrameDecode	*pIDecoderFrame		= NULL;

	// Locate the resource in the application's executable.
	imageResHandle = FindResource(
		NULL,							// This component.
		MAKEINTRESOURCE(resource),		// Resource name.
		L"PNG");						// Resource type.
	HRESULT hr = (imageResHandle ? S_OK : E_FAIL);

	// Get the handle to the resource data.
	if (SUCCEEDED(hr)) {
		imageResDataHandle = LoadResource(NULL, imageResHandle);
		hr = (imageResDataHandle ? S_OK : E_FAIL);
	}
	// Retrieve the memory pointer.
	// This doesn't lock anything, just gets the pointer. No free necessary.
	if (SUCCEEDED(hr)) {
		pImageFile = LockResource(imageResDataHandle);
		hr = (pImageFile ? S_OK : E_FAIL);
	}
	// Calculate the size in bytes.
	if (SUCCEEDED(hr)) {
		imageFileSize = SizeofResource(NULL, imageResHandle);
		hr = (imageFileSize ? S_OK : E_FAIL);
	}
	// Create a WIC stream to map onto the memory.
	if (SUCCEEDED(hr)) {
		hr = pIWICFactory->CreateStream(&pIWICStream);
	}
	// Initialize the stream with the memory pointer and size.
	if (SUCCEEDED(hr)) {
		hr = pIWICStream->InitializeFromMemory(
			reinterpret_cast<BYTE*>(pImageFile),
			imageFileSize);
	}
	// Create a decoder for the stream.
	if (SUCCEEDED(hr)) {
		hr = pIWICFactory->CreateDecoderFromStream(
			pIWICStream,                   // The stream to use to create the decoder
			NULL,                          // Do not prefer a particular vendor
			WICDecodeMetadataCacheOnLoad,  // Cache metadata when needed
			&pIDecoder);                   // Pointer to the decoder
	}
	// Retrieve the initial frame.
	if (SUCCEEDED(hr)) {
		hr = pIDecoder->GetFrame(0, &pIDecoderFrame);
	}
	// Initialize the data to the frame, format converted to 32bppPBGRA.
	if (SUCCEEDED(hr))
	{
		hr = pConvertedSourceBitmap->Initialize(
			pIDecoderFrame,					// Input bitmap to convert
			GUID_WICPixelFormat32bppPBGRA,  // Destination pixel format
			WICBitmapDitherTypeNone,        // Specified dither pattern
			NULL,                           // Specify a particular palette 
			0.f,                            // Alpha threshold
			WICBitmapPaletteTypeCustom      // Palette translation type
		);
	}

	SafeRelease(&pIWICStream);
	SafeRelease(&pIDecoder);
	SafeRelease(&pIDecoderFrame);
	return hr;
}


// Creates pRenderTarget, pBrush, and pD2DBitmaps
HRESULT D2Objects::CreateGraphicsResources(HWND hwnd)
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, size), &pRenderTarget);

		if (SUCCEEDED(hr))
		{
			const D2D1_COLOR_F color = D2D1::ColorF(0.15f, 0.15f, 0.16f);
			hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
		}
		// Re-create D2DBitmap it from the source bitmap.
		for (int i = 0; i < nIcons; i++)
		{
			if (SUCCEEDED(hr))
				hr = pRenderTarget->CreateBitmapFromWicBitmap(pConvertedSourceBitmaps[i], NULL, &pD2DBitmaps[i]);
		}
	}

	return hr;
}


void D2Objects::DiscardDeviceIndependentResources()
{
	SafeRelease(&pFactory);
	SafeRelease(&pDWriteFactory);
	for (int i = 0; i < nFormats; i++)
	{
		SafeRelease(&pTextFormats[i]);
	}
	SafeRelease(&pIWICFactory);
	SafeRelease(&pDashedStyle);
	SafeRelease(&pFixedTransformStyle);
	SafeRelease(&pHairlineStyle);

	for (int i = 0; i < nIcons; i++)
	{
		SafeRelease(&pConvertedSourceBitmaps[i]);
	}
}

void D2Objects::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBrush);
	
	for (int i = 0; i < nIcons; i++)
	{
		SafeRelease(&pD2DBitmaps[i]);
	}
}

