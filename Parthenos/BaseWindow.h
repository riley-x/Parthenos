#pragma once

#include "stdafx.h"

// Argument struct to pass to BaseWindow::Register/Create
struct WndCreateArgs {
	HINSTANCE hInstance;

	// WNDCLASSEX only

	UINT classStyle = CS_HREDRAW | CS_VREDRAW;
	HCURSOR hCursor = 0;
	HBRUSH hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	LPCWSTR lpszMenuName = 0;
	HICON hIcon = 0;
	HICON hIconSm = 0;

	// CreateWindowEx only

	PCWSTR lpWindowName = L"BaseWindow";
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;
	DWORD dwExStyle = 0;
	int x = CW_USEDEFAULT;
	int y = CW_USEDEFAULT;
	int nWidth = CW_USEDEFAULT;
	int nHeight = CW_USEDEFAULT;
	HWND hWndParent = 0;
	HMENU hMenu = 0;
};

template <class DERIVED_TYPE>
class BaseWindow
{
public:
	BaseWindow() : m_wsClassName(L"BASE WINDOW"), m_hwnd(NULL), m_hInstance(NULL) { }
	BaseWindow(PCWSTR pClassName) : m_wsClassName(pClassName), m_hwnd(NULL), m_hInstance(NULL) { }

	HWND Window() const { return m_hwnd; }

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

	BOOL Register(WndCreateArgs & args)
	{
		WNDCLASSEXW wcex{};

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = args.classStyle;
		wcex.lpfnWndProc = DERIVED_TYPE::WindowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = args.hInstance;
		wcex.hCursor = args.hCursor;
		wcex.hbrBackground = args.hbrBackground;
		wcex.lpszClassName = ClassName();
		wcex.lpszMenuName = args.lpszMenuName;
		wcex.hIcon = args.hIcon;
		wcex.hIconSm = args.hIconSm;

		m_hInstance = args.hInstance;
		ATOM atom = RegisterClassExW(&wcex);

		return (atom ? TRUE : FALSE);
	}

	BOOL Create(WndCreateArgs & args) {
		m_hwnd = CreateWindowEx(
			args.dwExStyle, ClassName(), args.lpWindowName, args.dwStyle, args.x, args.y,
			args.nWidth, args.nHeight, args.hWndParent, args.hMenu, args.hInstance, this
		);

		return (m_hwnd ? TRUE : FALSE);
	}

protected:

	PCWSTR ClassName() const { return m_wsClassName.c_str(); }
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	std::wstring m_wsClassName;
	HWND m_hwnd;
	HINSTANCE m_hInstance;
};

