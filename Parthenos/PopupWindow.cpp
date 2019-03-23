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
	case WM_DESTROY:
		m_d2.DiscardGraphicsResources();
		m_d2.DiscardLifetimeResources();
		m_created = FALSE;
		m_parent->PostClientMessage(reinterpret_cast<AppItem*>(this), m_name, CTPMessage::WINDOW_CLOSED);
		// TODO remove m_hwnd from timers
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
	return 0;
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
	bool handeled = false;
	for (auto item : m_items)
	{
		handeled = item->OnLButtonDown(dipCursor, handeled) || handeled;
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


///////////////////////////////////////////////////////////
// --- AddTransactionWindow ---

void AddTransactionWindow::PreShow()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");
	D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

	m_center = (dipRect.left + dipRect.right) / 2.0f;
	m_inputLeft = (dipRect.left + dipRect.right - m_itemWidth) / 2.0f;

	// Create AppItems
	m_titleBar		= new TitleBar(m_hwnd, m_d2, this);
	m_accountButton			= new DropMenuButton(m_hwnd, m_d2, this);
	m_transactionTypeButton = new DropMenuButton(m_hwnd, m_d2, this);
	m_taxLotBox		= new TextBox(m_hwnd, m_d2, this);
	m_tickerBox		= new TextBox(m_hwnd, m_d2, this);
	m_nsharesBox	= new TextBox(m_hwnd, m_d2, this);
	m_dateBox		= new TextBox(m_hwnd, m_d2, this);
	m_expirationBox = new TextBox(m_hwnd, m_d2, this);
	m_valueBox		= new TextBox(m_hwnd, m_d2, this);
	m_priceBox		= new TextBox(m_hwnd, m_d2, this);
	m_strikeBox		= new TextBox(m_hwnd, m_d2, this);
	m_ok			= new TextButton(m_hwnd, m_d2, this);
	m_cancel		= new TextButton(m_hwnd, m_d2, this);

	m_items = { m_titleBar, m_accountButton, m_transactionTypeButton, m_dateBox, m_tickerBox, m_nsharesBox, m_priceBox,
		m_valueBox, m_expirationBox, m_strikeBox, m_taxLotBox, m_ok, m_cancel };

	m_accountButton->SetItems(m_accounts);
	m_accountButton->SetActive(0);
	m_transactionTypeButton->SetItems(TRANSACTIONTYPE_STRINGS);
	m_transactionTypeButton->SetActive(0);
	m_ok->SetName(L"Ok");
	m_cancel->SetName(L"Cancel");

	for (size_t i = 0; i < m_items.size(); i++)
	{
		m_items[i]->SetSize(CalculateItemRect(i, dipRect));
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
		m_d2.pD2DContext->BeginDraw();

		// Repaint everything
		m_d2.pD2DContext->Clear(Colors::MAIN_BACKGROUND);

		for (auto item : m_items)
			item->Paint(fullRect);
		DrawTexts(fullRect);

		m_accountButton->GetMenu().Paint(fullRect);
		m_transactionTypeButton->GetMenu().Paint(fullRect);

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
	if (FAILED(hr)) OutputHRerr(hr, L"TransactionWindow OnPaint failed");

	return 0;
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
		case CTPMessage::TEXTBOX_DEACTIVATED:
		case CTPMessage::TEXTBOX_ENTER:
		case CTPMessage::DROPMENU_SELECTED:
			break; // Do nothing
		case CTPMessage::BUTTON_DOWN:
			if (msg.sender == m_ok)
			{
				Transaction *t = CreateTransaction();
				m_parent->PostClientMessage(reinterpret_cast<AppItem*>(t), L"", CTPMessage::WINDOW_ADDTRANSACTION_P);
				PostMessage(m_phwnd, WM_APP, 0, 0);
			}
			SendMessage(m_hwnd, WM_CLOSE, 0, 0); // Close on both ok and close
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
	else if (i == 11) // Ok
	{
		return D2D1::RectF(
			m_center - m_buttonHPad - m_buttonWidth,
			dipRect.bottom - m_buttonVPad - m_buttonHeight,
			m_center - m_buttonHPad,
			dipRect.bottom - m_buttonVPad
		);
	}
	else if (i == 12) // Cancel
	{
		return D2D1::RectF(
			m_center + m_buttonHPad,
			dipRect.bottom - m_buttonVPad - m_buttonHeight,
			m_center + m_buttonHPad + m_buttonWidth,
			dipRect.bottom - m_buttonVPad
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

void AddTransactionWindow::DrawTexts(D2D1_RECT_F fullRect)
{
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);

	std::wstring title(L"Add a transaction:");
	m_d2.pD2DContext->DrawTextW(
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
			fullRect.left,
			m_inputTop + i * (m_itemHeight + m_itemVPad),
			m_inputLeft - m_labelHPad,
			m_inputTop + i * (m_itemHeight + m_itemVPad) + m_itemHeight
		);
		m_d2.pD2DContext->DrawText(
			m_labels[i].c_str(),
			m_labels[i].size(),
			m_d2.pTextFormats[D2Objects::Formats::Segoe12],
			dipRect,
			m_d2.pBrush
		);
	}

	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	for (size_t i = 0; i < extra_inds.size(); i++)
	{
		D2D1_RECT_F dipRect = D2D1::RectF(
			m_inputLeft + m_itemWidth + m_labelHPad,
			m_inputTop + extra_inds[i] * (m_itemHeight + m_itemVPad),
			fullRect.right,
			m_inputTop + extra_inds[i] * (m_itemHeight + m_itemVPad) + m_itemHeight
		);
		m_d2.pD2DContext->DrawText(
			extra_labels[i].c_str(),
			extra_labels[i].size(),
			m_d2.pTextFormats[D2Objects::Formats::Segoe12],
			dipRect,
			m_d2.pBrush
		);
	}
}

Transaction* AddTransactionWindow::CreateTransaction()
{
	Transaction *t = new Transaction();
	try {
		t->account = m_accountButton->GetSelection();
		t->type = TransactionStringToEnum(m_transactionTypeButton->Name());
		t->tax_lot = stoi(m_taxLotBox->String());
		
		std::wstring ticker = m_tickerBox->String();
		std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::toupper);
		wcscpy_s(t->ticker, PortfolioObjects::maxTickerLen + 1, ticker.c_str());
		
		t->n = stoi(m_nsharesBox->String());
		t->date = stoi(m_dateBox->String());
		t->expiration = stoi(m_expirationBox->String());
		t->value = stod(m_valueBox->String());
		t->price = stod(m_priceBox->String());
		t->strike = stod(m_strikeBox->String());

		return t;
	}
	catch (const std::exception& ia) {
		if (t) delete t;
		OutputDebugStringA(ia.what()); OutputDebugStringA("\n");
	}
	return nullptr;
}


///////////////////////////////////////////////////////////
// --- ConfirmationWindow ---

void ConfirmationWindow::PreShow()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");
	D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

	m_center = (dipRect.left + dipRect.right) / 2.0f;

	// Create AppItems
	m_titleBar = new TitleBar(m_hwnd, m_d2, this);
	m_ok = new TextButton(m_hwnd, m_d2, this);
	m_cancel = new TextButton(m_hwnd, m_d2, this);

	m_items = { m_titleBar, m_ok, m_cancel };

	m_ok->SetName(L"Ok");
	m_cancel->SetName(L"Cancel");

	for (auto item : m_items)
	{
		item->SetSize(CalculateItemRect(item, dipRect));
	}
}

LRESULT ConfirmationWindow::OnPaint()
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
		m_d2.pD2DContext->BeginDraw();

		// Repaint everything
		m_d2.pD2DContext->Clear(Colors::MAIN_BACKGROUND);
		for (auto item : m_items)
			item->Paint(fullRect);

		m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
		m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_d2.pD2DContext->DrawTextW(
			m_text.c_str(),
			m_text.size(),
			m_d2.pTextFormats[D2Objects::Formats::Segoe12],
			D2D1::RectF(fullRect.left, m_titleBarHeight, fullRect.right, fullRect.bottom - m_buttonVPad - m_buttonHeight),
			m_d2.pBrush
		);
		m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

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

void ConfirmationWindow::ProcessCTPMessages()
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
		case CTPMessage::BUTTON_DOWN:
			if (msg.sender == m_ok)
			{
				m_parent->PostClientMessage(m_msg);
				PostMessage(m_phwnd, WM_APP, 0, 0);
			}
			SendMessage(m_hwnd, WM_CLOSE, 0, 0); // Close on both ok and close
			break;
		default:
			OutputMessage(L"Unknown message %d received\n", static_cast<int>(msg.imsg));
			break;
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}

D2D1_RECT_F ConfirmationWindow::CalculateItemRect(AppItem *item, D2D1_RECT_F const & dipRect)
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
	else if (item == m_ok)
	{
		return D2D1::RectF(
			m_center - m_buttonHPad - m_buttonWidth,
			dipRect.bottom - m_buttonVPad - m_buttonHeight,
			m_center - m_buttonHPad,
			dipRect.bottom - m_buttonVPad
		);
	}
	else if (item == m_cancel)
	{
		return D2D1::RectF(
			m_center + m_buttonHPad,
			dipRect.bottom - m_buttonVPad - m_buttonHeight,
			m_center + m_buttonHPad + m_buttonWidth,
			dipRect.bottom - m_buttonVPad
		);
	}
	else
	{
		return D2D1::RectF(0, 0, 0, 0);
	}
}
