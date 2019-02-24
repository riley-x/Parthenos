#include "stdafx.h"
#include "PopupWindow.h"
#include "Parthenos.h"


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
	//case WM_CLOSE: {
	//	if (MessageBox(m_hwnd, L"Really quit?", L"Parthenos", MB_OKCANCEL) == IDOK)
	//	{
	//		::DestroyWindow(m_hwnd);
	//	}
	//	return 0;
	//}
	case WM_DESTROY:
		m_d2.DiscardGraphicsResources();
		m_d2.DiscardDeviceIndependentResources();
		m_created = FALSE;
		return 0;
	case WM_PAINT:
		return OnPaint();
	case WM_SIZE:
		return 0;
	case WM_MOUSEMOVE:
		return OnMouseMove(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_MOUSELEAVE:
		m_titleBar->SetMouseOn(TitleBar::Buttons::NONE);
		m_mouseTrack.Reset(m_hwnd);
		return 0;
	case WM_MOUSEHOVER:
		// TODO: Handle the mouse-hover message.
		m_mouseTrack.Reset(m_hwnd);
		return 0;
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

///////////////////////////////////////////////////////////
// --- Message Handlers ---
// These pretty much just pass to m_items, calling ProcessCTPMessages
// as needed

LRESULT PopupWindow::OnCreate()
{
	m_d2.hwndParent = m_hwnd;
	HRESULT hr = m_d2.CreateDeviceIndependentResources();
	if (FAILED(hr))
	{
		throw Error(L"OnCreate failed!");
		return -1;  // Fail CreateWindowEx.
	}

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


LRESULT PopupWindow::OnMouseMove(POINT cursor, WPARAM wParam)
{
	Cursor::isSet = false;
	m_mouseTrack.OnMouseMove(m_hwnd);  // Start tracking.

	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);

	bool handeled = false;
	for (auto item : m_items)
	{
		handeled = item->OnMouseMove(dipCursor, wParam, handeled) || handeled;
	}

	if (!Cursor::isSet) ::SetCursor(Cursor::hArrow);
	return 0;
}

LRESULT PopupWindow::OnLButtonDown(POINT cursor, WPARAM wParam)
{
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	for (auto item : m_items)
	{
		item->OnLButtonDown(dipCursor);
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
	for (auto item : m_items)
	{
		item->OnLButtonUp(dipCursor, wParam);
	}
	ProcessCTPMessages();

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
		if (Timers::nActiveP1[i] == 1)
		{
			BOOL err = ::KillTimer(m_hwnd, i);
			if (err == 0)  OutputError(L"Kill timer failed");
			Timers::nActiveP1[i] = 0;
		}
	}
	//ProcessCTPMessages();
	return 0;
}


///////////////////////////////////////////////////////////
// --- AddTransactionWindow ---

void AddTransactionWindow::PreShow()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");
	D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

	m_inputLeft = (dipRect.left + dipRect.right) / 2.0f;

	// Create AppItems
	m_titleBar = new TitleBar(m_hwnd, m_d2, this);
	m_accountButton = new DropMenuButton(m_hwnd, m_d2, this);
	m_transactionTypeButton = new DropMenuButton(m_hwnd, m_d2, this);
	m_taxLotBox = new TextBox(m_hwnd, m_d2, this);
	m_tickerBox = new TextBox(m_hwnd, m_d2, this);
	m_nsharesBox = new TextBox(m_hwnd, m_d2, this);
	m_dateBox = new TextBox(m_hwnd, m_d2, this);
	m_expirationBox = new TextBox(m_hwnd, m_d2, this);
	m_valueBox = new TextBox(m_hwnd, m_d2, this);
	m_priceBox = new TextBox(m_hwnd, m_d2, this);
	m_strikeBox = new TextBox(m_hwnd, m_d2, this);

	m_items.reserve(11);

	m_items.push_back(m_titleBar);
	m_items.push_back(m_accountButton);
	m_items.push_back(m_transactionTypeButton);
	m_items.push_back(m_dateBox);
	m_items.push_back(m_tickerBox);
	m_items.push_back(m_nsharesBox);
	m_items.push_back(m_priceBox);
	m_items.push_back(m_valueBox);
	m_items.push_back(m_expirationBox);
	m_items.push_back(m_strikeBox);
	m_items.push_back(m_taxLotBox);

	m_accountButton->SetItems(m_accounts);
	m_transactionTypeButton->SetItems(TRANSACTIONTYPE_STRINGS);
	m_accountButton->SetActive(0);
	m_transactionTypeButton->SetActive(0);

	for (size_t i = 0; i < m_items.size(); i++) // skip TitleBar
	{
		m_items[i]->SetSize(CalculateItemRect(i, dipRect));
	}

}


void AddTransactionWindow::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::TITLEBAR_CLOSE:
			SendMessage(m_hwnd, WM_CLOSE, 0, 0);
			break;
		case CTPMessage::TITLEBAR_MAXRESTORE: // Do nothing
			break;
		case CTPMessage::TITLEBAR_MIN:
			ShowWindow(m_hwnd, SW_MINIMIZE);
			break;
		default:
			OutputMessage(L"Unknown message %d received\n", static_cast<int>(msg.imsg));
			break;
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

D2D1_RECT_F AddTransactionWindow::CalculateItemRect(size_t i, D2D1_RECT_F const & dipRect)
{
	if (i == 0) // TitleBar
	{
		return D2D1::RectF(
			0.0f,
			0.0f,
			dipRect.right,
			DPIScale::SnapToPixelY(m_titleBarHeight)
		);
	}
	else
	{
		return D2D1::RectF(
			m_inputLeft, // Flush right
			m_inputTop + (i - 1) * (m_itemHeight + m_itemVPad),
			m_inputLeft + m_itemWidth,
			m_inputTop + (i - 1) * (m_itemHeight + m_itemVPad) + m_itemHeight
		);
	}
}


LRESULT AddTransactionWindow::OnPaint()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");
	D2D1_RECT_F fullRect = DPIScale::PixelsToDips(rc);

	HRESULT hr = m_d2.CreateGraphicsResources(m_hwnd);
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		//D2D1_RECT_F dipRect = DPIScale::PixelsToDips(ps.rcPaint);
		m_d2.pRenderTarget->BeginDraw();

		// Repaint everything
		m_d2.pRenderTarget->Clear(Colors::MAIN_BACKGROUND);

		for (auto item : m_items)
			item->Paint(fullRect);
		DrawTexts();

		m_accountButton->GetMenu().Paint(fullRect);
		m_transactionTypeButton->GetMenu().Paint(fullRect);

		hr = m_d2.pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			m_d2.DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}

	return 0;
}

void AddTransactionWindow::DrawTexts()
{
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);

	std::wstring title(L"Add a transaction:");
	m_d2.pRenderTarget->DrawTextW(
		title.c_str(),
		title.size(),
		m_d2.pTextFormats[D2Objects::Formats::Segoe12],
		D2D1::RectF(10.0f, m_titleBarHeight + 10.0f, 200.0f, m_titleBarHeight + 24.0f),
		m_d2.pBrush
	);

	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	for (size_t i = 0; i < m_labels.size(); i++)
	{
		D2D1_RECT_F dipRect = D2D1::RectF(
			0.0f,
			m_inputTop + i * (m_itemHeight + m_itemVPad),
			m_inputLeft - m_labelHPad,
			m_inputTop + i * (m_itemHeight + m_itemVPad) + m_itemHeight
		);
		m_d2.pRenderTarget->DrawText(
			m_labels[i].c_str(),
			m_labels[i].size(),
			m_d2.pTextFormats[D2Objects::Formats::Segoe12],
			dipRect,
			m_d2.pBrush
		);
	}
	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}

