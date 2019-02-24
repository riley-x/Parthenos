#include "stdafx.h"
#include "PopupWindow.h"
#include "Parthenos.h"




void CreatePopupWindow(void *win, HINSTANCE hInstance, PopupType type)
{
	WndCreateArgs args;
	args.hInstance = hInstance;
	args.lpWindowName = L"Hiii";
	args.dwStyle = WS_OVERLAPPEDWINDOW;
	args.x = 200;
	args.y = 200;
	args.nWidth = 500;
	args.nHeight = 500;

	switch (type)
	{
	case PopupType::TransactionAdd:
	{
		AddTransactionWindow* twin = reinterpret_cast<AddTransactionWindow*>(win);
		BOOL ok = twin->Create(args);
		if (!ok) OutputError(L"Create TransactionAdd window failed");
		ShowWindow(twin->Window(), SW_SHOW);
		break;
	}
	}
}

LRESULT AddTransactionWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = 0;
	if (BorderlessWindow<AddTransactionWindow>::handle_message(uMsg, wParam, lParam, ret)) {
		if (ret == HTCLIENT && uMsg == WM_NCHITTEST) {
			// BorderlessWindow only handles resizing, catch everything else here
			ret = HTCAPTION;
			//ret = OnNCHitTest(POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		}
		return ret;
	}

	switch (uMsg)
	{
		//case WM_CREATE:
		//	return OnCreate();
			//case WM_CLOSE: {
			//	if (MessageBox(m_hwnd, L"Really quit?", L"Parthenos", MB_OKCANCEL) == IDOK)
			//	{
			//		::DestroyWindow(m_hwnd);
			//	}
			//	return 0;
			//}
	case WM_DESTROY:
		//m_d2.DiscardGraphicsResources();
		//m_d2.DiscardDeviceIndependentResources();
		m_created = FALSE;
		return 0;
		/*case WM_PAINT:*/
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

BOOL AddTransactionWindow::Create(WndCreateArgs & args)
{
	if (m_created) return FALSE;
	m_created = TRUE;
	return BorderlessWindow<AddTransactionWindow>::Create(args);
}
