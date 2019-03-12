#pragma once
#include "stdafx.h"
#include "BorderlessWindow.h"
#include "utilities.h"
#include "D2Objects.h"
#include "AppItem.h"
#include "TitleBar.h"
#include "TextBox.h"
#include "DataManaging.h"



class PopupWindow : public BorderlessWindow<PopupWindow>, public CTPMessageReceiver
{
public:
	PopupWindow(std::wstring const & name) : BorderlessWindow(L"PARTHENOSPOPUP"), m_name(name) {}
	~PopupWindow() 
	{ 
		for (auto item : m_items) if (item) delete item;
		for (int i = 1; i < Timers::n_timers + 1; i++)
		{
			if (m_timers.nActiveP1[i] == 1)
			{
				BOOL err = ::KillTimer(m_hwnd, i);
				if (err == 0) OutputError(L"Kill timer failed");
				m_timers.nActiveP1[i] = 0;
			}
		}
	}

	BOOL Create(HINSTANCE hInstance);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	virtual void PreShow() = 0; // Handle AppItem creation here
	inline void SetParent(CTPMessageReceiver *parent, HWND phwnd) { m_parent = parent; m_phwnd = phwnd; }

protected:
	// Parent
	HWND					m_phwnd;
	CTPMessageReceiver		*m_parent;

	// Objects
	std::vector<AppItem*>	m_items;
	TitleBar				*m_titleBar;
	Timers::WndTimers		m_timers;

	// Resources 
	D2Objects m_d2;

	// Layout
	float const	m_titleBarHeight = 30.0f;

	// Virtuals
	virtual void		ProcessCTPMessages() = 0;
	virtual LRESULT		OnPaint() = 0;

private:
	// Name
	std::wstring m_name;

	// Flags
	bool m_created = false; // only allow create once

	// Resources
	MouseTrackEvents m_mouseTrack;

	// Message responses
	LRESULT	OnCreate();
	LRESULT OnNCHitTest(POINT cursor);
	LRESULT OnMouseMove(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonDown(POINT cursor, WPARAM wParam);
	LRESULT OnLButtonDblclk(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonUp(POINT cursor, WPARAM wParam);
	LRESULT OnChar(wchar_t c, LPARAM lParam);
	bool	OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
};


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
	std::vector<size_t> const		extra_inds = { 2, 4, 6, 7 };
	std::vector<std::wstring> const	extra_labels = { L"YYYYMMDD", L"signed", L"signed", L"YYYYMMDD" };

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
	LRESULT	OnPaint();

	void ProcessCTPMessages();
	D2D1_RECT_F CalculateItemRect(size_t i, D2D1_RECT_F const & dipRect);
	void DrawTexts(D2D1_RECT_F fullRect);
	Transaction *CreateTransaction();
};


// Initialize with text and a message to send to parent when user presses "OK"
class ConfirmationWindow : public PopupWindow
{
public:
	ConfirmationWindow() : PopupWindow(L"ConfirmationWindow") {}

	void PreShow();
	inline void SetText(std::wstring const & text) { m_text = text; }
	inline void SetMessage(ClientMessage const & msg) { m_msg = msg; }
private:

	// Items
	TextButton		*m_ok;
	TextButton		*m_cancel;

	// Data
	std::wstring	m_text;
	ClientMessage	m_msg; // message to send to parent if click OK

	// Layout
	float		m_center;
	float const	m_buttonHPad = 10.0f;
	float const	m_buttonVPad = 30.0f;
	float const	m_buttonWidth = 50.0f;
	float const	m_buttonHeight = 20.0f;

	// Functions
	LRESULT	OnPaint();

	void ProcessCTPMessages();
	D2D1_RECT_F CalculateItemRect(AppItem *item, D2D1_RECT_F const & dipRect);
};