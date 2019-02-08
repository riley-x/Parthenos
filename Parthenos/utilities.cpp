#include "stdafx.h"
#include "utilities.h"
#include <stdarg.h>

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;
float DPIScale::dpiX = 96.0f;
float DPIScale::dpiY = 96.0f;

std::wstring OutputError(const std::wstring & msg)
{
	DWORD error = GetLastError();
	std::wstring outmsg = L"Error " + std::to_wstring(error) + L": " + msg + L"\n";
	OutputDebugString(outmsg.c_str());

	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf,
		0, NULL);
	OutputDebugStringW((LPWSTR)lpMsgBuf);

	return outmsg;
}


std::system_error Error(const std::wstring & msg)
{
	std::wstring outmsg = OutputError(msg);
	size_t len = outmsg.size();
	size_t bufferSize = len * sizeof(wchar_t);
	char *msgBuffer = new char[bufferSize];
	wcstombs_s(NULL, msgBuffer, bufferSize, outmsg.c_str(), _TRUNCATE);

	auto out_error = std::system_error(
		std::error_code(::GetLastError(), std::system_category()), msgBuffer
	);

	delete[] msgBuffer;
	return out_error;
}

void OutputMessage(const std::wstring format, ...)
{
	wchar_t msg[100];
	va_list args;
	va_start(args, format);
	vswprintf_s(msg, format.c_str(), args);
	va_end(args);

	OutputDebugString(msg);
}
