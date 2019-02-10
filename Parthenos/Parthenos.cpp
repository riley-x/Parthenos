// Parthenos.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include "Parthenos.h"
#include "utilities.h"
#include "TitleBar.h"
#include "HTTP.h"
#include "DataManaging.h"

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
	if (BorderlessWindow<Parthenos>::handle_message(uMsg, wParam, lParam, ret)) {
		if (ret == HTCLIENT && uMsg == WM_NCHITTEST) {
			// BorderlessWindow only handles resizing, catch everything else here
			ret = OnNCHitTest(POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		}
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
		m_d2.DiscardGraphicsResources();
		m_d2.DiscardFactories();
		CoUninitialize();
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		return OnPaint();
	case WM_SIZE:
		return OnSize(wParam);
	case WM_LBUTTONDOWN:
		return OnLButtonDown(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			(DWORD)wParam
		);
	//case WM_LBUTTONUP:
	//	OnLButtonUp();
	//	return 0;
	case WM_MOUSEMOVE:
		return OnMouseMove(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, 
			static_cast<DWORD>(wParam)
		);
	//case WM_LBUTTONDBLCLK:
	//	return 0;
	case WM_MOUSELEAVE:
		m_titleBar.MouseOn(HTNOWHERE, m_hwnd);
		m_mouseTrack.Reset(m_hwnd);
		return 0;
	case WM_MOUSEHOVER:

		// TODO: Handle the mouse-hover message.

		m_mouseTrack.Reset(m_hwnd);
		return 0;
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

LRESULT Parthenos::OnNCHitTest(POINT cursor) {
	//RECT window;
	//if (!::GetWindowRect(m_hwnd, &window)) Error("GetWindowRect failed");
	if (!::ScreenToClient(m_hwnd, &cursor)) Error(L"ScreenToClient failed");

	LRESULT ret = m_titleBar.HitTest(cursor);
	switch (ret)
	{
	case HTMINBUTTON:
	case HTMAXBUTTON:
	case HTCLOSE:
		m_titleBar.MouseOn(ret, m_hwnd);
		return HTCLIENT;
	case HTCAPTION:
		m_titleBar.MouseOn(HTNOWHERE, m_hwnd);
		return ret;
	default:
		m_titleBar.MouseOn(HTNOWHERE, m_hwnd);
		return HTCLIENT;
	}
}

LRESULT Parthenos::OnSize(WPARAM wParam)
{
	if (wParam == SIZE_MAXIMIZED)
		m_titleBar.Maximize(true);
	else if (wParam == SIZE_RESTORED)
		m_titleBar.Maximize(false);
	if (m_d2.pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		m_d2.pRenderTarget->Resize(size);
		m_titleBar.Resize(rc);

		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	return 0;
}

LRESULT Parthenos::OnCreate()
{
	hCursor = LoadCursor(NULL, IDC_ARROW);
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr))
	{
		hr = m_d2.CreateFactories();
	}
	if (FAILED(hr))
	{
		throw Error(L"OnCreate failed!");
		return -1;  // Fail CreateWindowEx.
	}
	return 0;
}

void Parthenos::PreShow()
{
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	m_titleBar.Init();
	m_titleBar.Resize(rc);

	m_chart.Init(350.0f);
	m_chart.Resize(rc);

	//std::vector<OHLC> test = GetOHLC(L"aapl");
	//for (auto item : test)
	//{
	//	OutputDebugString(item.to_wstring().c_str());
	//}
}

LRESULT Parthenos::OnPaint()
{
	HRESULT hr = m_d2.CreateGraphicsResources(m_hwnd);
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		m_d2.pRenderTarget->BeginDraw();

		m_d2.pRenderTarget->Clear(D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f));

		m_titleBar.Paint(m_d2);
		m_chart.Paint(m_d2);

		//D2D1_SIZE_F size = m_d2.pRenderTarget->GetSize();
		//const float x = size.width;
		//const float y = size.height;


		//D2D1_RECT_F layoutRect = D2D1::RectF(0.f, 0.f, 200.f, 200.f);
		//wchar_t msg[32];
		//swprintf_s(msg, L"DPI: %3.1f %3.1f\n\n\n", DPIScale::dpiX, DPIScale::dpiY);
		//OutputDebugString(msg);
		////PCWSTR msg = L"Hellow World";
		//m_d2.pRenderTarget->DrawText(
		//	msg,
		//	wcslen(msg),
		//	pTextFormat,
		//	layoutRect,
		//	pBrush
		//);

		hr = m_d2.pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			m_d2.DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}

	return 0;
}


LRESULT Parthenos::OnLButtonDown(POINT cursor, DWORD flags)
{
	LRESULT ret = m_titleBar.HitTest(cursor);
	switch (ret)
	{
	case HTMINBUTTON:
		ShowWindow(m_hwnd, SW_MINIMIZE);
		break;
	case HTMAXBUTTON:
		if (maximized()) ShowWindow(m_hwnd, SW_RESTORE);
		else ShowWindow(m_hwnd, SW_MAXIMIZE);
		break;
	case HTCLOSE:
		SendMessage(m_hwnd, WM_CLOSE, 0, 0);
		break;
	}
	return 0;
}

LRESULT Parthenos::OnMouseMove(POINT cursor, DWORD flags)
{
	::SetCursor(hCursor);
	m_mouseTrack.OnMouseMove(m_hwnd);  // Start tracking.
	
	//D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);

	return 0;

	//if ((flags & MK_LBUTTON) && Selection())
}

/*
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