#include "stdafx.h"
#include "Parthenos.h"
#include "TitleBar.h"
#include "utilities.h"


void TitleBar::Paint(D2Objects const & d2)
{
	d2.pBrush->SetColor(D2D1::ColorF(0.15f, 0.16f, 0.15f, 1.0f));
	d2.pRenderTarget->FillRectangle(D2D1::RectF(
		DPIScale::PixelsToDipsX(m_cRect.left),
		DPIScale::PixelsToDipsY(m_cRect.top),
		DPIScale::PixelsToDipsX(m_cRect.right),
		DPIScale::PixelsToDipsY(m_cRect.bottom)
	), d2.pBrush);


	// Draws an image and scales it to the current window size
	D2D1_RECT_F rectangle = D2D1::RectF(
		DPIScale::PixelsToDipsX(m_cRect.right) - 30.0f,
		DPIScale::PixelsToDipsY(m_cRect.top),
		DPIScale::PixelsToDipsX(m_cRect.right),
		DPIScale::PixelsToDipsY(m_cRect.bottom)
	);
	if (d2.pD2DBitmap)
	{
		d2.pRenderTarget->DrawBitmap(d2.pD2DBitmap, rectangle);
	}
	//GetClientRect(m_hwnd, &clientRect);
	//OutputMessage(L"Titlebar Rect: %ld %ld\n", clientRect.right, clientRect.bottom);

	//D2D1_SIZE_F size = d2.pRenderTarget->GetSize();
	//OutputMessage(L"Render Target: %3.1f %3.1f\n\n\n", size.width, size.height);

}


// 'pRect': client RECT of parent
void TitleBar::Resize(RECT pRect)
{
	int height = DPIScale::DipsToPixelsY(30.0f);
	m_cRect.right = pRect.right;
	m_cRect.bottom = height;

}

void TitleBar::LoadCommandIcons(D2Objects & d2)
{
	// https://docs.microsoft.com/en-us/windows/desktop/wic/-wic-bitmapsources-howto-loadfromresource

	// Locate the resource in the application's executable.
	imageResHandle = FindResource(
		NULL,							// This component.
		MAKEINTRESOURCE(IDB_CLOSE),		// Resource name.
		L"PNG");						// Resource type.
	HRESULT hr = (imageResHandle ? S_OK : E_FAIL);

	// Load the resource to the HGLOBAL.
	if (SUCCEEDED(hr)) {
		imageResDataHandle = LoadResource(NULL, imageResHandle);
		hr = (imageResDataHandle ? S_OK : E_FAIL);
	}
	// Lock the resource to retrieve memory pointer.
	if (SUCCEEDED(hr)) {
		pImageFile = LockResource(imageResDataHandle);
		hr = (pImageFile ? S_OK : E_FAIL);
	}
	// Calculate the size.
	if (SUCCEEDED(hr)) {
		imageFileSize = SizeofResource(NULL, imageResHandle);
		hr = (imageFileSize ? S_OK : E_FAIL);
	}
	// Create a WIC stream to map onto the memory.
	if (SUCCEEDED(hr)) {
		hr = d2.pIWICFactory->CreateStream(&d2.pIWICStream);
	}
	// Initialize the stream with the memory pointer and size.
	if (SUCCEEDED(hr)) {
		hr = d2.pIWICStream->InitializeFromMemory(
			reinterpret_cast<BYTE*>(pImageFile),
			imageFileSize);
	}
	// Create a decoder for the stream.
	if (SUCCEEDED(hr)) {
		hr = d2.pIWICFactory->CreateDecoderFromStream(
			d2.pIWICStream,                   // The stream to use to create the decoder
			NULL,                          // Do not prefer a particular vendor
			WICDecodeMetadataCacheOnLoad,  // Cache metadata when needed
			&d2.pIDecoder);                   // Pointer to the decoder
	}
	// Retrieve the initial frame.
	if (SUCCEEDED(hr)) {
		hr = d2.pIDecoder->GetFrame(0, &d2.pIDecoderFrame);
	}
	// Format convert the frame to 32bppPBGRA
	if (SUCCEEDED(hr))
	{
		hr = d2.pIWICFactory->CreateFormatConverter(&d2.pConvertedSourceBitmap);
	}
	if (SUCCEEDED(hr))
	{
		hr = d2.pConvertedSourceBitmap->Initialize(
			d2.pIDecoderFrame,                          // Input bitmap to convert
			GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
			WICBitmapDitherTypeNone,         // Specified dither pattern
			NULL,                            // Specify a particular palette 
			0.f,                             // Alpha threshold
			WICBitmapPaletteTypeCustom       // Palette translation type
		);
	}
	if (FAILED(hr))
	{
		throw Error("failed!");
	}

	// D2DBitmap may have been released due to device loss. 
	// If so, re-create it from the source bitmap
	if (d2.pConvertedSourceBitmap && !d2.pD2DBitmap)
	{
		d2.pRenderTarget->CreateBitmapFromWicBitmap(d2.pConvertedSourceBitmap, NULL, &d2.pD2DBitmap);
	}
}
