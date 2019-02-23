// Parthenos.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include "Parthenos.h"
#include "utilities.h"
#include "TitleBar.h"
#include "HTTP.h"
#include "DataManaging.h"

#include <windowsx.h>


// Forward declarations
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

///////////////////////////////////////////////////////////
// --- Interface functions ---

Parthenos::~Parthenos()
{
	for (auto item : m_allItems)
		if (item) delete item;
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
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
		lpMMI->ptMinTrackSize.x = 600;
		lpMMI->ptMinTrackSize.y = 600;
		return 0;
	}
	case WM_DESTROY:
		m_d2.DiscardGraphicsResources();
		m_d2.DiscardDeviceIndependentResources();
		CoUninitialize();
		PostQuitMessage(0);
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
		// TODO this should really should pass to OnMouseMove or OnMouseLeave...
		m_titleBar->SetMouseOn(TitleBar::Buttons::NONE);
		m_mouseTrack.Reset(m_hwnd);
		return 0;
	case WM_MOUSEHOVER:
		// TODO: Handle the mouse-hover message.
		m_mouseTrack.Reset(m_hwnd);
		return 0;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			// Handled in WM_MOUSEMOVE
			return TRUE;
		}
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

void Parthenos::PreShow()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");
	D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

	for (auto item : m_allItems)
	{
		item->Init();
		item->SetSize(CalculateItemRect(item, dipRect));
	}

	m_chart->Draw(L"AAPL");
}

///////////////////////////////////////////////////////////
// --- Helper functions ---

void Parthenos::ProcessAppItemMessages()
{
	for (ClientMessage msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::TITLEBAR_CLOSE:
			SendMessage(m_hwnd, WM_CLOSE, 0, 0);
			break;
		case CTPMessage::TITLEBAR_MAXRESTORE:
			if (maximized()) ShowWindow(m_hwnd, SW_RESTORE);
			else ShowWindow(m_hwnd, SW_MAXIMIZE);
			break;
		case CTPMessage::TITLEBAR_MIN:
			ShowWindow(m_hwnd, SW_MINIMIZE);
			break;
		case CTPMessage::TITLEBAR_TAB:
		{
			switch (TitleBar::WStringToButton(msg.msg))
			{
			case TitleBar::Buttons::PORTFOLIO:
				m_activeItems.clear();
				m_activeItems.push_back(m_titleBar);
				m_activeItems.push_back(m_portfolioList);
				::InvalidateRect(m_hwnd, NULL, false);
				break;
			case TitleBar::Buttons::CHART:
				m_activeItems.clear();
				m_activeItems.push_back(m_titleBar);
				m_activeItems.push_back(m_chart);
				m_activeItems.push_back(m_watchlist);
				::InvalidateRect(m_hwnd, NULL, false);
				break;
			default:
				break;
			}
			break;
		}
		case CTPMessage::WATCHLIST_SELECTED:
		{
			if (msg.sender != m_watchlist) break; // i.e. don't process messages from other lists
			if (std::find(m_activeItems.begin(), m_activeItems.end(), m_chart) != m_activeItems.end())
			{
				m_chart->Draw(msg.msg);
			}
			break;
		}
		default:
			OutputMessage(L"Unknown message %d received\n", static_cast<int>(msg.imsg));
			break;
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

D2D1_RECT_F Parthenos::CalculateItemRect(AppItem * item, D2D1_RECT_F const & dipRect)
{
	if (item == m_titleBar)
	{
		return D2D1::RectF(
			0.0f,
			0.0f,
			dipRect.right,
			DPIScale::SnapToPixelY(m_titleBarHeight)
		);
	}
	else if (item == m_chart)
	{
		return D2D1::RectF(
			DPIScale::SnapToPixelX(m_watchlistWidth),
			DPIScale::SnapToPixelY(m_titleBarHeight),
			dipRect.right,
			dipRect.bottom
		);
	}
	else if (item == m_watchlist)
	{
		return D2D1::RectF(
			0.0f,
			DPIScale::SnapToPixelY(m_titleBarHeight),
			DPIScale::SnapToPixelX(m_watchlistWidth),
			dipRect.bottom
		);
	}
	else if (item == m_portfolioList)
	{
		return D2D1::RectF(
			0.0f,
			DPIScale::SnapToPixelY(m_titleBarHeight),
			m_portfolioListWidth,
			DPIScale::SnapToPixelY((dipRect.bottom + m_titleBarHeight) / 2.0f)
		);
	}
	else
	{
		OutputMessage(L"CalculateItemRect: unkown item\n");
		return D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
	}
}


///////////////////////////////////////////////////////////
// --- Message handling functions ---

LRESULT Parthenos::OnCreate()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr))
	{
		hr = m_d2.CreateDeviceIndependentResources();
		m_d2.hwndParent = m_hwnd;
	}
	if (FAILED(hr))
	{
		throw Error(L"OnCreate failed!");
		return -1;  // Fail CreateWindowEx.
	}

	m_titleBar = new TitleBar(m_hwnd, m_d2, this, TitleBar::Buttons::CHART);
	m_chart = new Chart(m_hwnd, m_d2);
	m_watchlist = new Watchlist(m_hwnd, m_d2, this);
	m_portfolioList = new Watchlist(m_hwnd, m_d2, this, false);

	m_allItems.push_back(m_titleBar);
	m_allItems.push_back(m_chart);
	m_allItems.push_back(m_watchlist);
	m_allItems.push_back(m_portfolioList);
	m_activeItems.push_back(m_titleBar);
	m_activeItems.push_back(m_chart);
	m_activeItems.push_back(m_watchlist);

	// Read Holdings
	FileIO holdingsFile;
	holdingsFile.Init(ROOTDIR + L"port.hold");
	holdingsFile.Open(GENERIC_READ);
	std::vector<Holdings> out = holdingsFile.Read<Holdings>();
	holdingsFile.Close();

	// Holdings -> Positions
	std::vector<Position> positions = HoldingsToPositions(
		FlattenedHoldingsToTickers(out), Account::Robinhood, GetCurrentDate());

	std::vector<std::wstring> tickers = GetTickers(positions);
	std::vector<Column> portColumns = {
		{90.0f, Column::Ticker, L""},
		{60.0f, Column::Last, L"%.2lf"},
		{60.0f, Column::ChangeP, L"%.2lf"},
		{60.0f, Column::Shares, L"%d"},
		{60.0f, Column::AvgCost, L"%.2lf"},
		{60.0f, Column::Realized, L"%.2lf"},
		{70.0f, Column::Unrealized, L"%.2lf"},
		{60.0f, Column::ReturnsP, L"%.2lf"},
		{60.0f, Column::APY, L"%.2lf"},
		{60.0f, Column::exDiv, L""}
	};

	m_watchlist->Load(tickers, std::vector<Column>(), positions);
	m_portfolioList->Load(tickers, portColumns, positions);

	//m_watchlist->Load({ L"AAPL", L"MSFT", L"NVDA" }, std::vector<Column>());
	//m_portfolioList->Load({ L"AAPL", L"MSFT", L"NVDA" }, std::vector<Column>());

	return 0;
}

LRESULT Parthenos::OnNCHitTest(POINT cursor) 
{
	if (!::ScreenToClient(m_hwnd, &cursor)) throw Error(L"ScreenToClient failed");

	TitleBar::Buttons but = m_titleBar->HitTest(cursor);
	if (but == TitleBar::Buttons::CAPTION)
		return HTCAPTION;
	else
		return HTCLIENT;
}

LRESULT Parthenos::OnPaint()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");

	HRESULT hr = m_d2.CreateGraphicsResources(m_hwnd);
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		D2D1_RECT_F dipRect = DPIScale::PixelsToDips(ps.rcPaint);
		m_d2.pRenderTarget->BeginDraw();

		if (ps.rcPaint.left == 0 && ps.rcPaint.top == 0 &&
			ps.rcPaint.right == rc.right && ps.rcPaint.bottom == rc.bottom)
		{
			m_d2.pRenderTarget->Clear(Colors::MAIN_BACKGROUND);
		}

		for (auto item : m_activeItems)
			item->Paint(dipRect);

		hr = m_d2.pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			m_d2.DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}

	return 0;
}

LRESULT Parthenos::OnSize(WPARAM wParam)
{
	if (wParam == SIZE_MAXIMIZED)
		m_titleBar->SetMaximized(true);
	else if (wParam == SIZE_RESTORED)
		m_titleBar->SetMaximized(false);
	if (m_d2.pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		m_d2.pRenderTarget->Resize(size);

		D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);


		for (auto item : m_activeItems)
		{
			item->SetSize(CalculateItemRect(item, dipRect));
		}

		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	return 0;
}

LRESULT Parthenos::OnMouseMove(POINT cursor, WPARAM wParam)
{
	//::SetCursor(Cursor::active);
	Cursor::isSet = false;
	m_mouseTrack.OnMouseMove(m_hwnd);  // Start tracking.

	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);

	for (auto item : m_activeItems)
	{
		if (item == m_titleBar) m_titleBar->OnMouseMoveP(cursor, wParam);
		else item->OnMouseMove(dipCursor, wParam);
	}

	if (!Cursor::isSet) ::SetCursor(Cursor::hArrow);

	//ProcessMessages();
	return 0;
}

LRESULT Parthenos::OnLButtonDown(POINT cursor, WPARAM wParam)
{
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	for (auto item : m_activeItems)
	{
		if (item == m_titleBar) m_titleBar->OnLButtonDownP(cursor);
		else item->OnLButtonDown(dipCursor);
	}
	
	ProcessAppItemMessages();
	return 0;
}

LRESULT Parthenos::OnLButtonDblclk(POINT cursor, WPARAM wParam)
{
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	for (auto item : m_activeItems)
	{
		item->OnLButtonDblclk(dipCursor, wParam);
	}
	//ProcessMessages();
	return 0;
}

LRESULT Parthenos::OnLButtonUp(POINT cursor, WPARAM wParam)
{
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	for (auto item : m_activeItems)
	{
		item->OnLButtonUp(dipCursor, wParam);
	}
	ProcessAppItemMessages();
	
	// ReleaseCapture();
	return 0;
}

LRESULT Parthenos::OnChar(wchar_t c, LPARAM lParam)
{
	for (auto item : m_activeItems)
	{
		if (item->OnChar(c, lParam)) break;
	}
	//ProcessMessages();
	return 0;
}

bool Parthenos::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = false;
	for (auto item : m_activeItems)
	{
		if (item->OnKeyDown(wParam, lParam)) out = true;
	}
	//ProcessMessages();
	return out;
}

LRESULT Parthenos::OnTimer(WPARAM wParam, LPARAM lParam)
{
	for (auto item : m_activeItems)
	{
		item->OnTimer(wParam, lParam);
	}

	for (int i = 1; i < Timers::n_timers + 1; i++)
	{
		if (Timers::nActiveP1[i] == 1)
		{
			BOOL err = ::KillTimer(m_hwnd, i);
			if (err == 0)  OutputError(L"Kill timer failed");
			Timers::nActiveP1[i] = 0;
		}
	}
	//ProcessMessages();
	return 0;
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