#include "stdafx.h"
#include "AddTransactionWindow.h"

void AddTransactionWindow::PreShow()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");
	D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

	m_center = (dipRect.left + dipRect.right) / 2.0f;
	m_inputLeft = (dipRect.left + dipRect.right - m_itemWidth) / 2.0f;

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
	m_ok = new TextButton(m_hwnd, m_d2, this);
	m_cancel = new TextButton(m_hwnd, m_d2, this);

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

void AddTransactionWindow::PaintSelf(D2D1_RECT_F windowRect, D2D1_RECT_F updateRect)
{
	// Repaint everything
	m_d2.pD2DContext->Clear(Colors::MAIN_BACKGROUND);

	for (auto item : m_items)
		item->Paint(windowRect);
	DrawTexts(windowRect);

	m_accountButton->GetMenu().Paint(windowRect);
	m_transactionTypeButton->GetMenu().Paint(windowRect);
}

void AddTransactionWindow::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		if (ProcessPopupWindowCTP(msg)) continue;

		switch (msg.imsg)
		{
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
		m_parent->PostClientMessage(this, L"Add transaction failed.\n", CTPMessage::PRINT);
	}
	return nullptr;
}
