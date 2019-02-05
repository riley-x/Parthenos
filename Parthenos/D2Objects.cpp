#include "stdafx.h"
#include "D2Objects.h"
#include "utilities.h"
#include "resource.h"

HRESULT D2Objects::CreateFactories()
{
	// Create a Direct2D factory
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
	// Initialize DPI settings
	if (SUCCEEDED(hr))
	{
		DPIScale::Initialize(pFactory);
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
	// Create text format
	if (SUCCEEDED(hr))
	{
		hr = pDWriteFactory->CreateTextFormat(
			L"Arial",
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			12.0f * 96.0f / 72.0f,
			L"", //locale
			&pTextFormat
		);
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
	int resource_ids[nIcons] = { IDB_CLOSE, IDB_MAXIMIZE, IDB_MINIMIZE, IDB_RESTORE };
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


void D2Objects::DiscardFactories()
{
	SafeRelease(&pFactory);
	SafeRelease(&pDWriteFactory);
	SafeRelease(&pTextFormat);
	SafeRelease(&pIWICFactory);

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

