#include "../stdafx.h"
#include "TransactionEditor.h"

TransactionEditor::TransactionEditor(HWND hwnd, D2Objects const & d2, Table *parent, std::vector<std::wstring> accounts)
	: TableRowItem(hwnd, d2, parent)
{
	// Create AppItems
	m_accountButton			= new DropMenuButton(m_hwnd, m_d2, this, true, false);
	m_transactionTypeButton = new DropMenuButton(m_hwnd, m_d2, this, true, false);
	m_dateBox		= new TextBox(m_hwnd, m_d2, this, m_parent->m_format, m_parent->m_hTextPad, false);
	m_tickerBox		= new TextBox(m_hwnd, m_d2, this, m_parent->m_format, m_parent->m_hTextPad, false);
	m_nsharesBox	= new TextBox(m_hwnd, m_d2, this, m_parent->m_format, m_parent->m_hTextPad, false);
	m_priceBox		= new TextBox(m_hwnd, m_d2, this, m_parent->m_format, m_parent->m_hTextPad, false);
	m_valueBox		= new TextBox(m_hwnd, m_d2, this, m_parent->m_format, m_parent->m_hTextPad, false);
	m_expirationBox = new TextBox(m_hwnd, m_d2, this, m_parent->m_format, m_parent->m_hTextPad, false);
	m_strikeBox		= new TextBox(m_hwnd, m_d2, this, m_parent->m_format, m_parent->m_hTextPad, false);
	m_taxLotBox		= new TextBox(m_hwnd, m_d2, this, m_parent->m_format, m_parent->m_hTextPad, false);

	m_items = { m_accountButton, m_transactionTypeButton, m_dateBox, m_tickerBox, m_nsharesBox, m_priceBox,
		m_valueBox, m_expirationBox, m_strikeBox, m_taxLotBox };

	m_accountButton->SetItems(accounts);
	m_accountButton->SetActive(0);
	m_transactionTypeButton->SetItems(TRANSACTIONTYPE_STRINGS);
	m_transactionTypeButton->SetActive(0);
}

void TransactionEditor::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(dipRect, m_dipRect)) return;
	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(m_dipRect);

	std::vector<float> widths = m_parent->GetColumnWidths();
	if (widths.size() != m_nFields)
		throw ws_exception(L"TransactionEditor::CreateTextLayouts() Parent table has invalid number of columns");

	float left = m_dipRect.left;
	for (size_t i = 0; i < m_nFields; i++)
	{
		m_items[i]->SetSize(D2D1::RectF(left, m_dipRect.top, left + widths[i], m_dipRect.bottom));
		left += widths[i];
	}
}

void TransactionEditor::Paint(D2D1_RECT_F updateRect)
{
	if (!overlapRect(m_dipRect, updateRect)) return;
	for (AppItem *item : m_items) item->Paint(updateRect);
}

bool TransactionEditor::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	for (AppItem *item : m_items)
		handeled = item->OnMouseMove(cursor, wParam, handeled) || handeled;
	return handeled;
}

bool TransactionEditor::OnLButtonDown(D2D1_POINT_2F cursor, bool handeled)
{
	for (AppItem *item : m_items)
		handeled = item->OnLButtonDown(cursor, handeled) || handeled;
	ProcessCTPMessages();
	return handeled;
}

void TransactionEditor::OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam)
{
	for (AppItem *item : m_items) item->OnLButtonDblclk(cursor, wParam);
}

void TransactionEditor::OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam)
{
	for (AppItem *item : m_items) item->OnLButtonUp(cursor, wParam);
}

bool TransactionEditor::OnChar(wchar_t c, LPARAM lParam)
{
	c = towupper(c);
	for (AppItem *item : m_items) 
		if (item->OnChar(c, lParam)) return true;
	return false;
}

bool TransactionEditor::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = false;
	for (AppItem *item : m_items)
		if (item->OnKeyDown(wParam, lParam)) out = true;
	//ProcessCTPMessages();
	return out;
}

void TransactionEditor::OnTimer(WPARAM wParam, LPARAM lParam)
{
	for (AppItem *item : m_items) item->OnTimer(wParam, lParam);
}

void TransactionEditor::ProcessCTPMessages()
{
	for (ClientMessage msg : m_messages)
	{
		switch (msg.imsg)
		{
		case CTPMessage::DROPMENU_ACTIVE:
		case CTPMessage::DROPMENU_SELECTED: // forward
			m_parent->PostClientMessage(msg);
			break;
		default:
			OutputMessage(L"Unused message: %d\n", msg.imsg);
		}
	}
	if (!m_messages.empty()) m_messages.clear();

}

double TransactionEditor::GetData(size_t iColumn) const
{
	if (iColumn < 2)
	{
		DropMenuButton *but = static_cast<DropMenuButton*>(m_items[iColumn]);
		return (double)but->GetSelection();
	}
	else if (iColumn == 3) // ticker
	{
		return 0.0;
	}
	else if (iColumn < m_items.size())
	{
		TextBox *box = static_cast<TextBox*>(m_items[iColumn]);
		return stod(box->String());
	}
	else return 0.0;
}

std::wstring TransactionEditor::GetString(size_t iColumn) const
{
	if (iColumn < 2)
	{
		DropMenuButton *but = static_cast<DropMenuButton*>(m_items[iColumn]);
		return but->Name();
	}
	else if (iColumn < m_items.size())
	{
		TextBox *box = static_cast<TextBox*>(m_items[iColumn]);
		return box->String();
	}
	else return L"";
}

// Formats to [2,4] decimal places.
std::wstring PriceFormat(double p)
{
	std::wstring out = FormatMsg(L"%.4f", p);
	size_t i = 1;
	for (; i <= 2; i++)
		if (out[out.size() - i] != L'0') return out.substr(0, out.length() - i + 1);
	return out.substr(0, out.length() - i + 1);
}

void TransactionEditor::SetInfo(Transaction const & t)
{
	m_accountButton->SetActive(t.account);
	m_transactionTypeButton->SetActive(static_cast<char>(t.type));
	m_dateBox->SetText(std::to_wstring(t.date));
	m_tickerBox->SetText(t.ticker);
	m_nsharesBox->SetText(std::to_wstring(t.n));
	m_priceBox->SetText(PriceFormat(t.price));
	m_valueBox->SetText(FormatMsg(L"%.2f", t.value));
	m_expirationBox->SetText(std::to_wstring(t.expiration));
	m_strikeBox->SetText(FormatMsg(L"%.2f", t.strike));
	m_taxLotBox->SetText(std::to_wstring(t.tax_lot));
}

