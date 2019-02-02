#include "stdafx.h"
#include "Parthenos.h"
#include "TitleBar.h"
#include "utilities.h"

TitleBar::TitleBar(HWND hwnd_parent, HINSTANCE hInstance, Parthenos * parent) 
	: BaseWindow(L"TITLEBAR")
{
	// Register class in static variable == only do once!
	static const BOOL registered = [&] {
		WndCreateArgs args;
		args.classStyle = 0; // CS_PARENTDC? https://docs.microsoft.com/en-us/windows/desktop/gdi/display-devices
		args.hInstance = hInstance;
		return Register(args);
	}();
	if (!registered) {
		throw Error("TitleBar failed register");
	}
	m_parent = parent;
	m_hWndParent = hwnd_parent;
}

BOOL TitleBar::Create(int childID)
{
	WndCreateArgs args;
	args.hInstance = m_hInstance;
	args.lpWindowName = L"TitleBar";
	args.dwStyle = title_bar_style;
	args.x = 0;
	args.y = 0;
	args.nWidth = 20;
	args.nHeight = 20;
	args.hWndParent = m_hWndParent;
	args.hMenu = reinterpret_cast<HMENU>(childID);

	m_ID = childID;

	return BaseWindow<TitleBar>::Create(args);
}

LRESULT TitleBar::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
	case WM_PAINT:
		return OnPaint();
	// case WM_SIZE: Not received

	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

	}
}

// 'pRect': client RECT of parent
void TitleBar::Resize(RECT pRect)
{
	int height = DPIScale::DipsToPixelsY(30.0f);
	::SetWindowPos(m_hwnd, NULL, 0, 0, pRect.right, height, 
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);

}

LRESULT TitleBar::OnPaint()
{
	HRESULT hr = m_parent->CreateGraphicsResources();
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		// Since we paint on the same rendertarget as parent, no need to beginpaint?
		m_parent->pRenderTarget->BeginDraw();

		RECT clientRect;
		if (GetClientRect(m_hwnd, &clientRect))
		{
			m_parent->pRenderTarget->FillRectangle(D2D1::RectF(
				DPIScale::PixelsToDipsX(clientRect.left),
				DPIScale::PixelsToDipsY(clientRect.top),
				DPIScale::PixelsToDipsX(clientRect.right),
				DPIScale::PixelsToDipsY(clientRect.bottom)
			), m_parent->pBrush);
		}

		GetClientRect(m_hwnd, &clientRect);
		//OutputMessage(L"Titlebar Rect: %ld %ld\n", clientRect.right, clientRect.bottom);

		D2D1_SIZE_F size = m_parent->pRenderTarget->GetSize();
		//OutputMessage(L"Render Target: %3.1f %3.1f\n\n\n", size.width, size.height);

		hr = m_parent->pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			m_parent->DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}

	return 0;
}
