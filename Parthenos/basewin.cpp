#include "stdafx.h"
#include "basewin.h"

namespace {
	// we cannot just use WS_POPUP style
	// WS_THICKFRAME: without this the window cannot be resized and so aero snap, de-maximizing and minimizing won't work
	// WS_SYSMENU: enables the context menu with the move, close, maximize, minize... commands (shift + right-click on the task bar item)
	// WS_CAPTION: enables aero minimize animation/transition
	// WS_MAXIMIZEBOX, WS_MINIMIZEBOX: enable minimize/maximize
	enum class Style : DWORD {
		windowed = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		aero_borderless = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
		basic_borderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX
	};

}