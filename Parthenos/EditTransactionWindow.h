#pragma once

#include "stdafx.h"
#include "PopupWindow.h"
#include "Table.h"
#include "DataManaging.h"


class EditTransactionWindow : public PopupWindow
{
public:
	EditTransactionWindow() : PopupWindow(L"EditTransactionWindow") {}

	void PreShow();
	inline void SetAccounts(std::vector<std::wstring> const & accounts) { m_accounts = accounts; }
	inline void SetFilepath(std::wstring const & fp) { m_filepath = fp; }

private:
	// Items
	Table *m_table;
	std::vector<std::wstring> const m_labels = { L"Account:", L"Transaction:", L"Date:", L"Ticker:",
		L"Shares:", L"Price:", L"Value:",
		L"Ex Date:", L"Strike:", L"Tax Lot:" };
	std::vector<float> m_widths = { 100.0f, 100.0f, 80.0f, 70.0f, 50.0f, 70.0f, 70.0f, 80.0f, 50.0f, 50.0f };

	// Data
	std::wstring				m_filepath;
	std::vector<std::wstring>	m_accounts;
	std::vector<Transaction>	m_transactions;

	// Functions
	void PaintSelf(D2D1_RECT_F windowRect, D2D1_RECT_F updateRect);
	void ProcessCTPMessages();
};

