#pragma once
#include "stdafx.h"


std::wstring SendHTTPSRequest_GET(const std::wstring& _server,
	const std::wstring& _page,
	const std::wstring& _params = L"");
