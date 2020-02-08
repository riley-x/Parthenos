#include "../stdafx.h"
#include "EditTransactionWindow.h"
#include "../Utilities/FileIO.h"
#include "../AppItems/TransactionEditor.h"


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
	std::vector<Transaction> trans = transFile.Read<Transaction>();
	transFile.Close();

	// Create title bar
	float titleBarBottom = DPIScale::SnapToPixelY(m_titleBarHeight);
	m_titleBar = new TitleBar(m_hwnd, m_d2, this);
	m_titleBar->SetSize(D2D1::RectF(0, 0, dipRect.right, titleBarBottom));

	// Create menu bar
	float menuBarBottom = DPIScale::SnapToPixelY(m_titleBarHeight + m_menuBarHeight);
	m_menuBar = new MenuBar(m_hwnd, m_d2, this, menuBarBottom - titleBarBottom);
	std::vector<std::wstring> menus = { L"File" };
	std::vector<std::vector<std::wstring>> items = { { L"Save" } };
	std::vector<std::vector<size_t>> divisions = { {} };
	m_menuBar->SetMenus(menus, items, divisions);
	m_menuBar->SetSize(D2D1::RectF(0, titleBarBottom, dipRect.right, menuBarBottom));

	// Create table
	m_table = new Table(m_hwnd, m_d2, this, false);
	m_table->SetColumns(m_labels, m_widths, m_types);
	std::vector<TableRowItem*> rows;
	for (size_t i = 0; i < trans.size(); i++)
	{
		TransactionEditor *temp = new TransactionEditor(m_hwnd, m_d2, m_table, m_accounts);
		temp->SetInfo(trans[i]);
		rows.push_back(temp);
	}
	m_table->Load(rows);
	m_table->SetSize(D2D1::RectF(0, menuBarBottom, dipRect.right, dipRect.bottom));

	m_items = { m_titleBar, m_table, m_menuBar };
}


void EditTransactionWindow::PaintSelf(D2D1_RECT_F windowRect, D2D1_RECT_F updateRect)
{
	// Repaint everything
	m_d2.pD2DContext->Clear(Colors::MAIN_BACKGROUND);

	for (auto item : m_items)
		item->Paint(windowRect);

	if (m_activeButton) m_activeButton->GetMenu()->Paint(windowRect);
}

void EditTransactionWindow::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		if (ProcessPopupWindowCTP(msg)) continue;

		switch (msg.imsg)
		{
		case CTPMessage::TEXTBOX_DEACTIVATED:
		case CTPMessage::TEXTBOX_ENTER: // ignore
			break;
		case CTPMessage::DROPMENU_ACTIVE:
		{
			m_activeButton = static_cast<DropMenuButton*>(msg.sender);
			D2D1_RECT_F menuRect = m_activeButton->GetMenu()->GetDIPRect();
			if (menuRect.bottom > m_dipRect.bottom)
				m_activeButton->SetOrientationDown(false);
			else if (menuRect.top < m_dipRect.top)
				m_activeButton->SetOrientationDown(true);
			break;
		}
		case CTPMessage::DROPMENU_SELECTED:
		case CTPMessage::ITEM_SCROLLED: // from table
		{
			if (m_activeButton)
			{
				m_activeButton->Deactivate();
				m_activeButton = nullptr;
			}
			break;
		}
		case CTPMessage::MENUBAR_SELECTED:
		{
			if (msg.iData == 0) // File
			{
				if (msg.msg == L"Save")
				{
					std::vector<Transaction> trans;
					std::vector<TableRowItem*> const & rows = m_table->GetItems();
					for (TableRowItem *item : rows)
					{
						TransactionEditor *tedit = dynamic_cast<TransactionEditor*>(item);
						trans.push_back(tedit->GetTransaction());
					}

					//OutputMessage(L"hi\n");
					//if (trans.size() != m_transactions.size()) OutputMessage(L"uhoh\n");
					//for (size_t i = 0; i < trans.size(); i++)
					//{
					//	if (trans[i] != m_transactions[i])
					//	{
					//		OutputMessage(L"%d\n\t%s\n\t%s\n", i, trans[i].to_wstring(), m_transactions[i].to_wstring());
					//	}
					//}

					// Save transactions
					FileIO transFile;
					transFile.Init(m_filepath);
					transFile.Open(GENERIC_READ);
					transFile.Write(trans.data(), sizeof(Transaction)*trans.size());
					transFile.Close();
				}
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


