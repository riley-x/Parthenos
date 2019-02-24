#pragma once
#include "stdafx.h"
#include "BorderlessWindow.h"
#include "utilities.h"
#include "D2Objects.h"

class Parthenos;



enum class PopupType { TransactionAdd };



void CreatePopupWindow(void *win, HINSTANCE hInstance, PopupType type);


class AddTransactionWindow : public BorderlessWindow<AddTransactionWindow>
{
public:
	AddTransactionWindow(PCWSTR szClassName) : BorderlessWindow(szClassName) {}
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	BOOL Create(WndCreateArgs & args);
private:
	bool m_created = false; // only allow create once
};