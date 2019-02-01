#pragma once

#include "stdafx.h"
#include "BaseWindow.h"
#include <windowsx.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

template <class DERIVED_TYPE>
class BorderlessWindow : public BaseWindow<DERIVED_TYPE> {
public:
	static DWORD const aero_borderless_style =
		WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;

	BorderlessWindow() : BaseWindow<DERIVED_TYPE>::BaseWindow() { }
	BorderlessWindow(PCWSTR szClassName) : BaseWindow<DERIVED_TYPE>::BaseWindow(szClassName) { }

	BOOL Create(
		HINSTANCE hInstance,
		PCWSTR lpWindowName,
		UINT classStyle = CS_HREDRAW | CS_VREDRAW,
		int hIcon = 0,
		int hIconSm = 0,
		int lpszMenuName = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = 0,
		HMENU hMenu = 0
	)
	{
		OutputDebugString(L"Hello\n");
		BOOL out = BaseWindow<DERIVED_TYPE>::Create(
			hInstance,
			lpWindowName,
			aero_borderless_style,
			0,
			classStyle,
			hIcon,
			hIconSm,
			lpszMenuName,
			x,
			y,
			nWidth,
			nHeight,
			hWndParent,
			hMenu
		);
		BorderlessWindow<DERIVED_TYPE>::set_shadow();
		return out;
	}

	/* Adjust client rect to not spill over monitor edges when maximized.
	 * rect(in/out): in: proposed window rect, out: calculated client rect
	 * Does nothing if the window is not maximized.
	 */
	auto adjust_maximized_client_rect(RECT& rect) -> void {
		if (!maximized()) return;

		auto monitor = ::MonitorFromWindow(this->m_hwnd, MONITOR_DEFAULTTONULL);
		if (!monitor) return;

		MONITORINFO monitor_info{};
		monitor_info.cbSize = sizeof(monitor_info);
		if (!::GetMonitorInfoW(monitor, &monitor_info)) return;

		// when maximized, make the client area fill just the monitor (without task bar) rect,
		// not the whole window rect which extends beyond the monitor.
		rect = monitor_info.rcWork;
	}

	auto maximized() -> bool {
		WINDOWPLACEMENT placement;
		if (!::GetWindowPlacement(this->m_hwnd, &placement)) {
			return false;
		}

		return placement.showCmd == SW_MAXIMIZE;
	}

	auto hit_test(POINT cursor) const->LRESULT {
		// identify borders and corners to allow resizing the window.
		// Note: On Windows 10, windows behave differently and
		// allow resizing outside the visible window frame.
		// This implementation does not replicate that behavior.
		const POINT border{
			::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
			::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
		};
		RECT window;
		if (!::GetWindowRect(this->m_hwnd, &window)) {
			return HTNOWHERE;
		}

		const auto drag = borderless_drag ? HTCAPTION : HTCLIENT;

		enum region_mask {
			client = 0b0000,
			left = 0b0001,
			right = 0b0010,
			top = 0b0100,
			bottom = 0b1000,
		};

		const auto result =
			left * (cursor.x < (window.left + border.x)) |
			right * (cursor.x >= (window.right - border.x)) |
			top * (cursor.y < (window.top + border.y)) |
			bottom * (cursor.y >= (window.bottom - border.y));

		switch (result) {
		case left: return borderless_resize ? HTLEFT : drag;
		case right: return borderless_resize ? HTRIGHT : drag;
		case top: return borderless_resize ? HTTOP : drag;
		case bottom: return borderless_resize ? HTBOTTOM : drag;
		case top | left: return borderless_resize ? HTTOPLEFT : drag;
		case top | right: return borderless_resize ? HTTOPRIGHT : drag;
		case bottom | left: return borderless_resize ? HTBOTTOMLEFT : drag;
		case bottom | right: return borderless_resize ? HTBOTTOMRIGHT : drag;
		case client: return drag;
		default: return HTNOWHERE;
		}
	}

	auto set_shadow() -> void {
		DWORD old_style = ::GetWindowLongPtrW(this->m_hwnd, GWL_STYLE);

		if (aero_borderless_style != old_style) {
			::SetWindowLongPtrW(this->m_hwnd, GWL_STYLE, static_cast<LONG>(aero_borderless_style));

			static const MARGINS shadow_state{ 1,1,1,1 };
			::DwmExtendFrameIntoClientArea(this->m_hwnd, &shadow_state);

			// redraw frame
			::SetWindowPos(this->m_hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
			::ShowWindow(this->m_hwnd, SW_SHOW);
		}
	
	}

	/* Returns 'true' if message is handled */
	auto handle_message(UINT msg, WPARAM wparam, LPARAM lparam) -> bool {
		switch (msg) {
		case WM_NCCALCSIZE: {
			if (wparam == TRUE) {
				auto& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam);
				adjust_maximized_client_rect(params.rgrc[0]);
				return 0;
			}
			break;
		}
		case WM_NCHITTEST: {
			// When we have no border or title bar, we need to perform our
			// own hit testing to allow resizing and moving.
			return hit_test(POINT{
				GET_X_LPARAM(lparam),
				GET_Y_LPARAM(lparam)
				});
			break;
		}
		default:
			return false;
		}
		return true;
	}

protected:
	bool borderless_resize = true; // should the window allow resizing by dragging the borders while borderless
	bool borderless_drag = true; // should the window allow moving my dragging the client area
	bool borderless_shadow = true; // should the window display a native aero shadow while borderless

};