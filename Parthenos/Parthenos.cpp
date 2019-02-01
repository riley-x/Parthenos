// Parthenos.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include "Parthenos.h"
#include "utilities.h"

#include <windowsx.h>

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Main entry point
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	
	// Initialize strings from string table
	WCHAR szTitle[MAX_LOADSTRING];            // the title bar text
	WCHAR szClass[MAX_LOADSTRING];            // the window class name
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); // "Parthenos"
	LoadStringW(hInstance, IDC_PARTHENOS, szClass, MAX_LOADSTRING); // "PARTHENOS"

    // Perform application initialization:
	Parthenos win(szClass);
	if (!win.Create(hInstance,
					szTitle, 
					CS_DBLCLKS,
					IDI_PARTHENOS,
					IDI_SMALL,
					IDC_PARTHENOS,
					300,200,1000,600
	))
	{
		return -1;
	}
	ShowWindow(win.Window(), nCmdShow);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PARTHENOS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

LRESULT Parthenos::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	if (BorderlessWindow<Parthenos>::handle_message(uMsg, wParam, lParam, ret)) {
		return ret;
	}

	switch (uMsg)
	{
	case WM_CREATE: {
		hCursor = LoadCursor(NULL, IDC_ARROW);

		// Create a Direct2D factory
		HRESULT hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
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
		if (FAILED(hr))
		{
			return -1;  // Fail CreateWindowEx.
		}
		return 0;
	}
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
		OnPaint();
		return 0;

	//case WM_SIZE:
	//	Resize();
	//	return 0;
	//case WM_TIMER:
	//	if (wParam == HALF_SECOND_TIMER) InvalidateRect(m_hwnd, NULL, FALSE);   // invalidate whole window
	//	return 0;
	//case WM_LBUTTONDOWN:
	//	OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
	//	return 0;
	//case WM_LBUTTONUP:
	//	OnLButtonUp();
	//	return 0;
	//case WM_MOUSEMOVE:
	//	OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
	//	return 0;
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
			const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
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

void Parthenos::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);

		pRenderTarget->BeginDraw();

		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
		//for (auto it = ellipses.rbegin(); it != ellipses.rend(); it++)
		//{
		//	pBrush->SetColor((*it)->color);
		//	pRenderTarget->FillEllipse((*it)->ellipse, pBrush);
		//}

		//pRenderTarget->DrawEllipse(ellipse, pBlackBrush);

		// Draw hands
		//SYSTEMTIME time;
		//GetLocalTime(&time);
		//// 60 minutes = 30 degrees, 1 minute = 0.5 degree
		//const float fHourAngle = (360.0f / 12) * (time.wHour) + (time.wMinute * 0.5f);
		//const float fMinuteAngle = (360.0f / 60) * (time.wMinute) + (time.wSecond * 360.f / 3600);
		//const float fSecondAngle = (360.0f / 60) * (time.wSecond);
		//DrawClockHand(0.6f, fHourAngle, 6);
		//DrawClockHand(0.85f, fMinuteAngle, 4);
		//DrawClockHand(0.9f, fSecondAngle, 2);
		//// Restore the identity transformation.
		//pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

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