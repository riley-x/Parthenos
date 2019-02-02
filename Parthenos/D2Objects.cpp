#include "stdafx.h"
#include "D2Objects.h"
#include "utilities.h"

HRESULT D2Objects::CreateFactories()
{
	// Create a Direct2D factory
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
	if (SUCCEEDED(hr))
	{
		DPIScale::Initialize(pFactory);

		// Create a DirectWrite factory.
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(pDWriteFactory),
			reinterpret_cast<IUnknown **>(&pDWriteFactory)
		);
	}
	return hr;
}

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
		if (SUCCEEDED(hr))
		{
			const D2D1_COLOR_F color = D2D1::ColorF(0.15f, 0.15f, 0.16f);
			hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
		}

	}
	return hr;
}

void D2Objects::DiscardFactories()
{
	SafeRelease(&pFactory);
	SafeRelease(&pDWriteFactory);
}

void D2Objects::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBrush);
	SafeRelease(&pTextFormat);
}

