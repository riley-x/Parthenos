#include "stdafx.h"
#include "EditTransactionWindow.h"
#include "FileIO.h"
#include "TransactionEditor.h"



void EditTransactionWindow::PreShow()
{
	// Resize the window to be bigger than the normal PopupWindow
	BOOL ok = SetWindowPos(m_hwnd, HWND_TOP, 100, 100, DPIScale::DipsToPixelsX(800), DPIScale::DipsToPixelsY(500), 0);
	if (!ok) OutputError(L"EditTransactionWindow::PreShow() SetWindowPos failed");

	// Get the window rect
	RECT rc;
	ok = GetClientRect(m_hwnd, &rc);
	if (!ok) OutputError(L"EditTransactionWindow::PreShow() GetClientRect failed");
	D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

	// Load transactions
	FileIO transFile;
	transFile.Init(m_filepath);
	transFile.Open(GENERIC_READ);
	m_transactions = transFile.Read<Transaction>();
	transFile.Close();

	// Create title bar
	m_titleBar = new TitleBar(m_hwnd, m_d2, this);
	m_titleBar->SetSize(D2D1::RectF(0, 0, dipRect.right, DPIScale::SnapToPixelY(m_titleBarHeight)));

	// Create table
	m_table = new Table(m_hwnd, m_d2, this, true);
	m_table->SetColumns(m_labels, m_widths);
	std::vector<TableRowItem*> rows;
	for (size_t i = 0; i < m_transactions.size() + 1; i++)
	{
		TransactionEditor *temp = new TransactionEditor(m_hwnd, m_d2, m_table, m_accounts);
		if (i < m_transactions.size()) temp->SetInfo(m_transactions[i]);
		rows.push_back(temp);
	}
	m_table->Load(rows);
	m_table->SetSize(D2D1::RectF(0, DPIScale::SnapToPixelY(m_titleBarHeight), dipRect.right, dipRect.bottom));

	m_items = { m_titleBar, m_table };
}


void EditTransactionWindow::PaintSelf(D2D1_RECT_F windowRect, D2D1_RECT_F updateRect)
{
	// Repaint everything
	m_d2.pD2DContext->Clear(Colors::MAIN_BACKGROUND);

	for (auto item : m_items)
		item->Paint(windowRect);

	// TODO Paint menus here
}

void EditTransactionWindow::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		if (ProcessPopupWindowCTP(msg)) continue;

		switch (msg.imsg)
		{
		case CTPMessage::TEXTBOX_DEACTIVATED:
		case CTPMessage::TEXTBOX_ENTER:
			break;
		default:
			OutputMessage(L"Unknown message %d received\n", static_cast<int>(msg.imsg));
			break;
		}
	}
	if (!m_messages.empty()) m_messages.clear();
}


