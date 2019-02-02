#include "stdafx.h"
#include "utilities.h"
#include <stdarg.h>

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;
float DPIScale::dpiX = 96.0f;
float DPIScale::dpiY = 96.0f;

std::system_error Error(const std::string & msg)
{
	DWORD error = GetLastError();
	std::string outmsg = "Error " + std::to_string(error) + ": " + msg + "\n";
	OutputDebugStringA(outmsg.c_str());
	return std::system_error(
		std::error_code(::GetLastError(), std::system_category()),
		outmsg
	);
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
