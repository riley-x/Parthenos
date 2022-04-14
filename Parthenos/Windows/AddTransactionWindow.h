#pragma once

#include "../stdafx.h"
#include "PopupWindow.h"
#include "../AppItems/TextBox.h"
#include "../Utilities/DataManaging.h"

class AddTransactionWindow : public PopupWindow
{
public:
	AddTransactionWindow() : PopupWindow(L"AddTransactionWindow") {}

	void PreShow();
	inline void SetAccounts(std::vector<std::wstring> const & accounts) { m_accounts = accounts; }
private:

	// Items
	DropMenuButton	*m_accountButton;
	DropMenuButton  *m_transactionTypeButton;
	TextBox			*m_taxLotBox;
	TextBox			*m_tickerBox;
	TextBox			*m_nsharesBox;
	TextBox			*m_dateBox;
	TextBox			*m_expirationBox;
	TextBox			*m_valueBox;
	TextBox			*m_priceBox;
	TextBox			*m_strikeBox;
	TextButton		*m_ok;
	TextButton		*m_cancel;

	// Data
	std::vector<std::wstring>		m_accounts;
	std::vector<std::wstring> const m_labels = { L"Account:", L"Transaction:", L"Date:", L"Ticker:",
		L"Shares/Contracts:", L"Price:", L"Value:",
		L"Ex Date:", L"Strike:", L"Tax Lot:" };
	std::vector<size_t> const		extra_inds = { 2, 4, 5, 6, 7 };
	std::vector<std::wstring> const	extra_labels = { L"YYYYMMDD", L"signed", L"extrinsic only", L"signed", L"YYYYMMDD" };

	// Layout
	float		m_center;
	float		m_inputLeft;
	float const	m_inputTop = 70.0f;
	float const	m_labelHPad = 10.0f;
	float const	m_itemHeight = 14.0f;
	float const	m_itemVPad = 6.0f;
	float const	m_itemWidth = 100.0f;
	float const	m_buttonHPad = 10.0f;
	float const	m_buttonVPad = 30.0f;
	float const	m_buttonWidth = 50.0f;
	float const	m_buttonHeight = 20.0f;

	// Functions
	void PaintSelf(D2D1_RECT_F windowRect, D2D1_RECT_F updateRect);
	void ProcessCTPMessages();
	D2D1_RECT_F CalculateItemRect(size_t i, D2D1_RECT_F const & dipRect);
	void DrawTexts(D2D1_RECT_F fullRect);
	Transaction *CreateTransaction();
};
