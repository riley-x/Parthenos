#pragma once

#include "stdafx.h"
#include "Resource.h"

#define MAX_LOADSTRING 100

template <class DERIVED_TYPE>
class BaseWindow
{
public:
	BaseWindow() : m_wsClassName(L"BASE WINDOW"), m_hwnd(NULL), m_hInstance(NULL) { }
	BaseWindow(PCWSTR pClassName) : m_wsClassName(pClassName), m_hwnd(NULL), m_hInstance(NULL) { }

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DERIVED_TYPE *pThis = NULL;

		if (uMsg == WM_NCCREATE)
		{
			// Set user data pointer to self (`this` is passed as the application data)
			CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
			pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		}
		if (pThis)
		{
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		else
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	BOOL Create(
		HINSTANCE hInstance,
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		UINT classStyle = CS_HREDRAW | CS_VREDRAW,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = 0,
		HMENU hMenu = 0
	)
	{
		WNDCLASSEXW wcex{};

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = classStyle;
		wcex.lpfnWndProc = DERIVED_TYPE::WindowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PARTHENOS)); //TODO add arguments
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PARTHENOS);
		wcex.lpszClassName = ClassName();
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		RegisterClassExW(&wcex);

		m_hwnd = CreateWindowEx(
			dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, hInstance, this
		);
		m_hInstance = hInstance;

		return (m_hwnd ? TRUE : FALSE);
	}

	HWND Window() const { return m_hwnd; }



protected:

	PCWSTR  ClassName() const { return m_wsClassName.c_str(); }
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	std::wstring m_wsClassName;
	HWND m_hwnd;
	HINSTANCE m_hInstance;
};

