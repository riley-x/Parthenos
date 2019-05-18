#pragma once

#include "../stdafx.h"
#include "AppItem.h"
#include "Table.h"
#include "TextBox.h"
#include "Button.h"
#include "../Utilities/DataManaging.h"

class TransactionEditor : public TableRowItem
{
public:
	TransactionEditor(HWND hwnd, D2Objects const & d2, Table *parent, std::vector<std::wstring> accounts);
	~TransactionEditor();

	// AppItem
	void SetSize(D2D1_RECT_F dipRect);
	void Paint(D2D1_RECT_F updateRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnChar(wchar_t c, LPARAM lParam);
	bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM wParam, LPARAM lParam);

	// Interface
	std::wstring GetData(size_t iColumn) const;
	void SetInfo(Transaction const & t);
	//void GetTransaction() const;

private:
	// State
	const size_t m_nFields = 10;

	// Objects
	DropMenuButton	*m_accountButton;
	DropMenuButton  *m_transactionTypeButton;
	TextBox			*m_dateBox;
	TextBox			*m_tickerBox;
	TextBox			*m_nsharesBox;
	TextBox			*m_priceBox;
	TextBox			*m_valueBox;
	TextBox			*m_expirationBox;
	TextBox			*m_strikeBox;
	TextBox			*m_taxLotBox;
};