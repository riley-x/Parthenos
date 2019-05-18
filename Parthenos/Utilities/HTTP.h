#pragma once
#include "../stdafx.h"


std::string SendHTTPSRequest_GET(const std::wstring& _server,
	const std::wstring& _page,
	const std::wstring& _params = L"");
