#include "stdafx.h"
#include "utilities.h"
#include <stdarg.h>

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;
float DPIScale::dpiX = 96.0f;
float DPIScale::dpiY = 96.0f;
float DPIScale::halfPX = 0.5f;
float DPIScale::halfPY = 0.5f;
float DPIScale::fullPX = 1.0f;
float DPIScale::fullPY = 1.0f;

int Timers::nActiveP1[Timers::n_timers + 1] = {};

bool Cursor::isSet = false;
HCURSOR Cursor::active = Cursor::hArrow;

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
	wchar_t msg[1024];
	va_list args;
	va_start(args, format);
	vswprintf_s(msg, format.c_str(), args);
	va_end(args);

	OutputDebugString(msg);
}

BOOL SystemTimeToEasternTime(SYSTEMTIME const * sysTime, SYSTEMTIME * eastTime)
{
	DYNAMIC_TIME_ZONE_INFORMATION pdtzi; 
	EnumDynamicTimeZoneInformation(118, &pdtzi);
	// this indexes into the registry to get the timezone under
	// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time Zones
	if (pdtzi.Bias != 300) return FALSE;
	return SystemTimeToTzSpecificLocalTimeEx(&pdtzi, sysTime, eastTime);
}

time_t TruncateToDay(time_t time)
{
	struct tm out;
	errno_t err = localtime_s(&out, &time);
	if (err != 0) OutputMessage(L"Truncate localtime conversion failed: %d\n", err);

	out.tm_sec = 0;
	out.tm_min = 0;
	out.tm_hour = 0;

	return mktime(&out);
}

date_t GetDate(time_t time)
{
	struct tm out;
	errno_t err = localtime_s(&out, &time);
	if (err != 0) OutputMessage(L"GetDay localtime conversion failed: %d\n", err);

	return MkDate(out.tm_year + 1900, out.tm_mon + 1, out.tm_mday);
}

date_t GetCurrentDate()
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	return MkDate(t.wYear, t.wMonth, t.wDay);
}


std::wstring TimeToWString(time_t time)
{
	struct tm tm_time;
	errno_t err = localtime_s(&tm_time, &time);
	if (err != 0) OutputMessage(L"toWString localtime conversion failed: %d\n", err);

	wchar_t buffer[30] = {};
	size_t n = wcsftime(buffer, 30, L"%F", &tm_time);
	if (n == 0) OutputMessage(L"toWstring buffer exceeded\n");
	return std::wstring(buffer);
}

time_t DateToTime(date_t date)
{
	struct tm out;
	out.tm_year = GetYear(date) - 1900;
	out.tm_mon = GetMonth(date) - 1;
	out.tm_mday = GetDay(date);
	out.tm_sec = 0;
	out.tm_min = 0;
	out.tm_hour = 0;
	out.tm_isdst = -1;

	return _mkgmtime(&out);
}


// APPROXIMATE DIFFERENCE IN DAYS
// Typically larger than real difference
// returns a-b
int ApproxDateDiff(date_t a, date_t b)
{
	return (a / 10000 - b / 10000) * 365
		+ ((a / 100) % 100 - (b / 100) % 100) * 31
		+ (a % 100 - b % 100);
}


std::wstring DateToWString(date_t date)
{
	return std::to_wstring((date / 100) % 100) + L"/"
		+ std::to_wstring(date % 100) + L"/"
		+ std::to_wstring((date / 10000) % 100);
}
