#include "../stdafx.h"
#include "PopupWindow.h"
#include "Parthenos.h"


PopupWindow::~PopupWindow()
{
	for (auto item : m_items) if (item) delete item;
	for (int i = 1; i < Timers::n_timers + 1; i++)
	{
		if (m_timers.nActiveP1[i] == 1)
		{
			BOOL err = ::KillTimer(m_hwnd, i);
			if (err == 0) OutputError(L"Kill timer failed");
			m_timers.nActiveP1[i] = 0;
		}
	}
}

BOOL PopupWindow::Create(HINSTANCE hInstance)
{
	if (m_created) return FALSE;
	m_created = TRUE;

	WndCreateArgs args;
	args.hInstance = hInstance;
	args.lpWindowName = L"AddTransaction";
	args.dwStyle = BorderlessWindow::aero_borderless_style ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME;
	args.x = 400;
	args.y = 200;
	args.nWidth = 500;
	args.nHeight = 500;

	return BorderlessWindow<PopupWindow>::Create(args);
}

LRESULT PopupWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCALCSIZE:
	{
		LRESULT ret = 0;
		BorderlessWindow<PopupWindow>::handle_message(uMsg, wParam, lParam, ret);
		return ret;
	}
	case WM_NCHITTEST: // don't pass to borderless since no resizing
		return OnNCHitTest(POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }); 
	case WM_CREATE:
		return OnCreate();
	case WM_DESTROY:
		m_d2.DiscardGraphicsResources();
		m_d2.DiscardLifetimeResources();
		m_created = FALSE;
		m_parent->PostClientMessage(this, m_name, CTPMessage::WINDOW_CLOSED);
		Timers::WndTimersMap.erase(m_hwnd);
		PostMessage(m_phwnd, WM_APP, 0, 0);
		return 0;
	case WM_PAINT:
		return OnPaint();
	case WM_SIZE:
		return OnSize(wParam);
	case WM_MOUSEMOVE:
		return OnMouseMove(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_MOUSELEAVE:
	{
		D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(POINT{ -1, -1 });
		if (m_mouseCaptured)
		{
			m_mouseCaptured->OnMouseMove(dipCursor, wParam, false);
		}
		else
		{
			bool handeled = false;
			for (auto item : m_items)
			{
				handeled = item->OnMouseMove(dipCursor, wParam, handeled) || handeled;
			}
		}
		m_mouseTrack.Reset(m_hwnd);
		return 0;
	}
	case WM_MOUSEHOVER:
		// TODO: Handle the mouse-hover message.
		m_mouseTrack.Reset(m_hwnd);
		return 0;
	case WM_MOUSEWHEEL:
		return OnMouseWheel(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)	return TRUE;
		break;
	case WM_LBUTTONDOWN:
		return OnLButtonDown(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_LBUTTONDBLCLK:
		return OnLButtonDblclk(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_LBUTTONUP:
		return OnLButtonUp(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_CHAR:
		return OnChar(static_cast<wchar_t>(wParam), lParam);
	case WM_KEYDOWN:
		if (OnKeyDown(wParam, lParam)) return 0;
		break; // pass on to DefWindowProc to process WM_CHAR messages...?
	case WM_TIMER:
		return OnTimer(wParam, lParam);
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

bool PopupWindow::ProcessPopupWindowCTP(ClientMessage const & msg)
{
	switch (msg.imsg)
	{
	case CTPMessage::TITLEBAR_CLOSE:
		SendMessage(m_hwnd, WM_CLOSE, 0, 0);
		return true;
	case CTPMessage::TITLEBAR_MAXRESTORE: // Do nothing
		return true;
	case CTPMessage::TITLEBAR_MIN:
		ShowWindow(m_hwnd, SW_MINIMIZE);
		return true;
	case CTPMessage::MOUSE_CAPTURED: // click and hold
	{
		if (msg.iData < 0) // Release (ReleaseCapture() called in OnLButtonUp)
			m_mouseCaptured = nullptr;
		else // Capture (SetCapture() called in OnLButtonDown)
			m_mouseCaptured = reinterpret_cast<AppItem*>(msg.sender);
		return true;
	}
	default:
		return false;
	}
}

///////////////////////////////////////////////////////////
// --- Message Handlers ---
// These pretty much just pass to m_items, calling ProcessCTPMessages
// as needed



LRESULT PopupWindow::OnCreate()
{
	HRESULT hr = m_d2.CreateLifetimeResources(m_hwnd);
	m_d2.hwndParent = m_hwnd;
	if (FAILED(hr))
	{
		throw Error(L"OnCreate failed!");
		return -1;  // Fail CreateWindowEx.
	}

	Timers::WndTimersMap.insert({ m_hwnd, &m_timers });

	return 0;
}

LRESULT PopupWindow::OnNCHitTest(POINT cursor)
{
	if (!::ScreenToClient(m_hwnd, &cursor)) throw Error(L"ScreenToClient failed");

	TitleBar::Buttons but = m_titleBar->HitTest(cursor);
	if (but == TitleBar::Buttons::CAPTION)
		return HTCAPTION;
	else
		return HTCLIENT;
}

LRESULT PopupWindow::OnSize(WPARAM wParam)
{
	if (m_d2.pD2DContext != NULL)
	{
		m_d2.DiscardGraphicsResources();

		// No need to pass to items since size is fixed

		InvalidateRect(m_hwnd, NULL, FALSE);
	}

	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"PopupWindow::OnPaint() GetClientRect failed");
	m_dipRect = DPIScale::PixelsToDips(rc);

	return 0;
}

LRESULT PopupWindow::OnPaint()
{
	HRESULT hr = m_d2.CreateGraphicsResources(m_hwnd);
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);

		D2D1_RECT_F updateRect = DPIScale::PixelsToDips(ps.rcPaint);
		m_d2.pD2DContext->BeginDraw();

		PaintSelf(m_dipRect, updateRect);

		hr = m_d2.pD2DContext->EndDraw();
		if (SUCCEEDED(hr))
		{
			// Present
			DXGI_PRESENT_PARAMETERS parameters = { 0 };
			parameters.DirtyRectsCount = 0;
			parameters.pDirtyRects = nullptr;
			parameters.pScrollRect = nullptr;
			parameters.pScrollOffset = nullptr;

			hr = m_d2.pDXGISwapChain->Present1(1, 0, &parameters);
		}

		EndPaint(m_hwnd, &ps);
	}
	if (FAILED(hr)) OutputHRerr(hr, L"ConfirmationWindow OnPaint failed");

	return 0;
}

LRESULT PopupWindow::OnMouseMove(POINT cursor, WPARAM wParam)
{
	Cursor::isSet = false;
	m_mouseTrack.OnMouseMove(m_hwnd);  // Start tracking.

	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	if (m_mouseCaptured)
	{
		m_mouseCaptured->OnMouseMove(dipCursor, wParam, false);
	}
	else
	{
		bool handeled = false;
		for (auto item : m_items)
		{
			handeled = item->OnMouseMove(dipCursor, wParam, handeled) || handeled;
		}
	}

	if (!Cursor::isSet) ::SetCursor(Cursor::hArrow);

	ProcessCTPMessages();
	return 0;
}

LRESULT PopupWindow::OnMouseWheel(POINT cursor, WPARAM wParam)
{
	if (!::ScreenToClient(m_hwnd, &cursor)) throw Error(L"ScreenToClient failed");
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	bool handeled = false;
	for (auto item : m_items)
	{
		handeled = item->OnMouseWheel(dipCursor, wParam, handeled) || handeled;
	}

	ProcessCTPMessages();
	return 0;
}

LRESULT PopupWindow::OnLButtonDown(POINT cursor, WPARAM wParam)
{
	SetCapture(m_hwnd); // always?

	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	if (m_mouseCaptured)
	{
		m_mouseCaptured->OnLButtonDown(dipCursor, false);
	}
	else
	{
		bool handeled = false;
		for (auto item : m_items)
		{
			handeled = item->OnLButtonDown(dipCursor, handeled) || handeled;
		}
	}
	ProcessCTPMessages();
	return 0;
}

LRESULT PopupWindow::OnLButtonDblclk(POINT cursor, WPARAM wParam)
{
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	for (auto item : m_items)
	{
		item->OnLButtonDblclk(dipCursor, wParam);
	}
	return 0;
}

LRESULT PopupWindow::OnLButtonUp(POINT cursor, WPARAM wParam)
{
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	if (m_mouseCaptured)
	{
		m_mouseCaptured->OnLButtonUp(dipCursor, wParam);
	}
	else
	{
		for (auto item : m_items)
		{
			item->OnLButtonUp(dipCursor, wParam);
		}
	}
	ProcessCTPMessages();

	ReleaseCapture();
	return 0;
}

LRESULT PopupWindow::OnChar(wchar_t c, LPARAM lParam)
{
	for (auto item : m_items)
	{
		if (item->OnChar(c, lParam)) break;
	}
	return 0;
}

bool PopupWindow::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = false;
	for (auto item : m_items)
	{
		if (item->OnKeyDown(wParam, lParam)) out = true;
	}
	//ProcessCTPMessages();
	return out;
}

LRESULT PopupWindow::OnTimer(WPARAM wParam, LPARAM lParam)
{
	for (auto item : m_items)
	{
		item->OnTimer(wParam, lParam);
	}

	for (int i = 1; i < Timers::n_timers + 1; i++)
	{
		if (m_timers.nActiveP1[i] == 1)
		{
			BOOL err = ::KillTimer(m_hwnd, i);
			if (err == 0)  OutputError(L"Kill timer failed");
			m_timers.nActiveP1[i] = 0;
		}
	}
	//ProcessCTPMessages();
	return 0;
}

