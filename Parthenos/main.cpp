#include "stdafx.h"
#include "Windows/Parthenos.h"
#include "Utilities/utilities.h"

// Main entry point
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Initialize strings from string table
	const int max_loadstring = 100;
	WCHAR szTitle[max_loadstring];            // the title bar text
	WCHAR szClass[max_loadstring];            // the window class name
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, max_loadstring); // "Parthenos"
	LoadStringW(hInstance, IDC_PARTHENOS, szClass, max_loadstring); // "PARTHENOS"

	// Perform application initialization:
	Parthenos win(szClass);

	WndCreateArgs args;
	args.hInstance = hInstance;
	args.classStyle = CS_DBLCLKS;
	args.hbrBackground = CreateSolidBrush(RGB(0.2,0.2,0.2));
	//args.lpszMenuName = MAKEINTRESOURCE(IDC_PARTHENOS);
	args.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PARTHENOS));
	args.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
	args.lpWindowName = szTitle;
	args.x = 50;
	args.y = 50;
	args.nWidth = 1800;
	args.nHeight = 900;

	win.Register(args);
	if (!win.Create(args))
	{
		return -1;
	}
	win.PreShow();
	ShowWindow(win.Window(), nCmdShow);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PARTHENOS));


	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}