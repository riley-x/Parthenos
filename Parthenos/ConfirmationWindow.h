#pragma once

#include "stdafx.h"
#include "PopupWindow.h"
#include "Button.h"

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
	void PaintSelf(D2D1_RECT_F windowRect, D2D1_RECT_F updateRect);

	void ProcessCTPMessages();
	D2D1_RECT_F CalculateItemRect(AppItem *item, D2D1_RECT_F const & dipRect);
};