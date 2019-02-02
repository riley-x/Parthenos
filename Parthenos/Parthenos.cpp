// Parthenos.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include "Parthenos.h"
#include "utilities.h"
#include "TitleBar.h"

#include <windowsx.h>


namespace {
	// Message handler for about box.
	INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(lParam);
		switch (message)
		{
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
		}
		return (INT_PTR)FALSE;
	}
}

LRESULT Parthenos::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = 0;
	if (uMsg == WM_NCHITTEST) { 
		// BorderlessWindow only handles resizing, catch everything else here
		ret = OnNCHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		if (ret != 0) return ret;
	}
	if (BorderlessWindow<Parthenos>::handle_message(uMsg, wParam, lParam, ret)) {
		return ret;
	}

	switch (uMsg)
	{
	case WM_CREATE:
		return OnCreate();
	//case WM_COMMAND: // Command items from application menu; accelerators
	//{
	//	int wmId = LOWORD(wParam);
	//	// Parse the menu selections:
	//	switch (wmId)
	//	{
	//	case IDM_ABOUT:
	//		DialogBox(m_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), m_hwnd, About);
	//		break;
	//	case IDM_EXIT:
	//		DestroyWindow(m_hwnd);
	//		break;
	//		/*
	//	case ID_DRAW_MODE:
	//		SetMode(DrawMode);
	//		break;

	//	case ID_SELECT_MODE:
	//		SetMode(SelectMode);
	//		break;
	//	case ID_TOGGLE_MODE:
	//		if (mode == DrawMode)
	//		{
	//			SetMode(SelectMode);
	//		}
	//		else
	//		{
	//			SetMode(DrawMode);
	//		}
	//		break;
	//		*/
	//	default:
	//		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	//	}
	//	return 0;
	//}

	//case WM_CLOSE: {
	//	if (MessageBox(m_hwnd, L"Really quit?", L"Parthenos", MB_OKCANCEL) == IDOK)
	//	{
	//		::DestroyWindow(m_hwnd);
	//	}
	//	return 0;
	//}
	case WM_DESTROY:
		DiscardGraphicsResources();
		SafeRelease(&pFactory);
		SafeRelease(&pDWriteFactory);
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		return OnPaint();
	case WM_SIZE:
		return OnSize();
	//case WM_LBUTTONDOWN:
	//	OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
	//	return 0;
	//case WM_LBUTTONUP:
	//	OnLButtonUp();
	//	return 0;
	case WM_MOUSEMOVE:
		return OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
	//case WM_LBUTTONDBLCLK:
	//	return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			SetCursor(hCursor);
			return TRUE;
		}
		break;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

LRESULT Parthenos::OnNCHitTest(int x, int y) {
	RECT window;
	if (!::GetWindowRect(this->m_hwnd, &window)) {
		return HTNOWHERE;
	}
	return 0;
	// Header should be another window for simplicity...
}

LRESULT Parthenos::OnSize()
{
	if (pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		pRenderTarget->Resize(size);
		m_titleBar->Resize(rc);

		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	return 0;
}

LRESULT Parthenos::OnCreate()
{
	hCursor = LoadCursor(NULL, IDC_ARROW);

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
	if (SUCCEEDED(hr))
	{

		m_titleBar = new TitleBar(m_hwnd, m_hInstance, this);
		if (!m_titleBar || !m_titleBar->Create(137)) {
			throw Error("titleBar create failed");
		}
		ShowWindow(m_titleBar->GetHWnd(), SW_SHOW);

		HWND hwndButton = CreateWindow(
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"OK",      // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
			100,         // x position 
			100,         // y position 
			30,        // Button width
			15,        // Button height
			m_hwnd,     // Parent window
			NULL,       // No menu.
			(HINSTANCE)GetWindowLong(m_hwnd, GWL_HINSTANCE),
			NULL);      // Pointer not needed.

	}
	if (FAILED(hr))
	{
		return -1;  // Fail CreateWindowEx.
	}
	return 0;
}

LRESULT Parthenos::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		pRenderTarget->BeginDraw();

		D2D1_SIZE_F size = pRenderTarget->GetSize();
		pRenderTarget->Clear(D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f));
		//const float x = size.width;
		//const float y = size.height;
		//pBrush->SetColor(D2D1::ColorF(0.5f, 0, 0, 1.0f));
		//pRenderTarget->FillRectangle(D2D1::RectF(
		//	0.0f,
		//	0.0f,
		//	size.width,
		//	30.0f
		//), pBrush);


		//D2D1_RECT_F layoutRect = D2D1::RectF(0.f, 0.f, 200.f, 200.f);
		//wchar_t msg[32];
		//swprintf_s(msg, L"DPI: %3.1f %3.1f\n\n\n", DPIScale::dpiX, DPIScale::dpiY);
		//OutputDebugString(msg);
		////PCWSTR msg = L"Hellow World";
		//pRenderTarget->DrawText(
		//	msg,
		//	wcslen(msg),
		//	pTextFormat,
		//	layoutRect,
		//	pBrush
		//);

		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}

	return 0;
}

LRESULT Parthenos::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
	const float dipX = DPIScale::PixelsToDipsX(pixelX);
	const float dipY = DPIScale::PixelsToDipsY(pixelY);

	::SetCursor(hCursor);

	return 0;
}

HRESULT Parthenos::CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);
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

void Parthenos::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBrush);
	SafeRelease(&pTextFormat);
}


/*
void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
	const float dipX = DPIScale::PixelsToDipsX(pixelX);
	const float dipY = DPIScale::PixelsToDipsY(pixelY);

	if (mode == DrawMode)
	{
		POINT pt = { pixelX, pixelY };

		if (DragDetect(m_hwnd, pt))
		{
			SetCapture(m_hwnd);

			// Start a new ellipse.
			InsertEllipse(dipX, dipY);
		}
	}
	else
	{
		ClearSelection();

		// TODO: add drag detect here
		if (HitTest(dipX, dipY))
		{
			SetCapture(m_hwnd);

			// Store offset from center
			ptMouseStart = Selection()->ellipse.point;
			ptMouseStart.x -= dipX;
			ptMouseStart.y -= dipY;

			SetMode(DragMode);
		}
	}
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
	const float dipX = DPIScale::PixelsToDipsX(pixelX);
	const float dipY = DPIScale::PixelsToDipsY(pixelY);

	if ((flags & MK_LBUTTON) && Selection())
	{
		if (mode == DrawMode)
		{
			// Resize the ellipse.
			const float width = (dipX - ptMouseStart.x) / 2;
			const float height = (dipY - ptMouseStart.y) / 2;
			const float x1 = ptMouseStart.x + width;
			const float y1 = ptMouseStart.y + height;

			Selection()->ellipse = D2D1::Ellipse(D2D1::Point2F(x1, y1), width, height);
		}
		else if (mode == DragMode)
		{
			// Move the ellipse.
			Selection()->ellipse.point.x = dipX + ptMouseStart.x;
			Selection()->ellipse.point.y = dipY + ptMouseStart.y;
		}
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

void MainWindow::OnLButtonUp()
{
	if ((mode == DrawMode) && Selection())
	{
		ClearSelection();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	else if (mode == DragMode)
	{
		SetMode(SelectMode);
	}
	ReleaseCapture();
}
*/