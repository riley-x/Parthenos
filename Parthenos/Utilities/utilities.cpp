#include "../stdafx.h"
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

std::map<void*, Timers::WndTimers*> Timers::WndTimersMap;

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

void SPrintExceptionHelper(std::wstring & out, const std::exception & e, int level = 0)
{
	std::wstring appendee;
	bool ws_ex = (e.what() == "Wide-string exception");
	if (ws_ex)
	{
		try { // see if exception has wstring message
			const ws_exception & wse = dynamic_cast<const ws_exception &>(e);
			appendee = wse.ws_what();
		}
		catch (const std::bad_cast &) { 
			ws_ex = false;
		}
	}

	if (!ws_ex) // use what() and convert to wstring
	{
		size_t ssize = strlen(e.what());
		size_t wssize;
		appendee.resize(ssize + 1, L' ');
		errno_t err = mbstowcs_s(&wssize, &appendee[0], appendee.size(), e.what(), _TRUNCATE);
		if (!err) appendee.resize(wssize);
		else appendee.clear();
	}

	out.append(std::wstring(level * 3, '-') + appendee + L'\n');

	// Rethrow a nested exception
	try {
		std::rethrow_if_nested(e);
	}
	catch (const std::exception& e) {
		SPrintExceptionHelper(out, e, level + 1);
	}
	catch (...) {}
}

std::wstring SPrintException(const std::exception & e)
{
	std::wstring out = L"Error: ";
	SPrintExceptionHelper(out, e);
	return out;
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

// h [0, 360), s [0,1], v [0,1]
D2D1_COLOR_F Colors::HSVtoRGB(float hsv[3])
{
	float c = hsv[2] * hsv[1];
	int i = static_cast<int>(floor(hsv[0] / 60.0f));
	float ff = hsv[0] / 60.0f - i;
	
	float p = hsv[2] * (1.0f - hsv[1]);
	float q = hsv[2] * (1.0f - (hsv[1] * ff));
	float t = hsv[2] * (1.0f - (hsv[1] * (1.0f - ff)));

	switch (i) {
	case 0:
		return D2D1::ColorF(hsv[2], t, p);
	case 1:
		return D2D1::ColorF(q, hsv[2], p);
	case 2:
		return D2D1::ColorF(p, hsv[2], t);
	case 3:
		return D2D1::ColorF(p, q, hsv[2]);
	case 4:
		return D2D1::ColorF(t, p, hsv[2]);
	case 5:
	default:
		return D2D1::ColorF(hsv[2], p, q);
	}
}

D2D1_COLOR_F Colors::Randomizer(std::wstring str)
{
	str = str + str;
	float hsv[3] = { 60.0f, 0.5f, 0.5f };

	for (size_t i = 0; i < str.size(); i++)
	{
		float x = (static_cast<float>(str[i]) - 65.0f) / 25.0f; // assumes A = 65
		if (x < 0 || x > 1) break;
		hsv[0] += x * 240;
		if (i % 3 > 0) hsv[i % 3] = (hsv[i % 3] + 5 * x) / 6.0f;
	}
	size_t i = 0;
	hsv[0] = hsv[0] - 360.0f * floor(hsv[0] / 360.0f);
	hsv[1] = 0.3f + 0.6f * hsv[1]; // (0.3, 0.9) value
	hsv[2] = 0.4f + 0.35f * hsv[2]; // (0.4, 0.75) value

	// get rid of yellow shades
	while (30.0f <= hsv[0] && hsv[0] <= 70.0f)
	{
		float x = (static_cast<float>(str[i % str.size()]) - 65.0f) / 25.0f; // assumes A = 65
		if (x < 0 || x > 1) break;
		hsv[0] += x * 320.0f;
		hsv[0] = hsv[0] - 360.0f * floor(hsv[0] / 360.0f);
		i++;
	}

	return HSVtoRGB(hsv);
}

std::vector<D2D1_COLOR_F> Colors::Randomizer(std::vector<std::wstring> const & strs)
{
	std::vector<D2D1_COLOR_F> out;
	out.reserve(strs.size());
	for (const std::wstring & str : strs)
		out.push_back(Randomizer(str));
	return out;
}
