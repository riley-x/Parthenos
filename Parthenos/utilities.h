#pragma once

std::wstring OutputError(const std::wstring & msg);
std::system_error Error(const std::wstring & msg);
void OutputMessage(const std::wstring format, ...);

typedef uint32_t date_t; // 10000*yyyy + 100*mm + dd
time_t const DATE_T_1YR = 10000;

time_t TruncateToDay(time_t time);
std::wstring TimeToWString(time_t time);
date_t GetDate(time_t time);
date_t MkDate(int year, int month, int day); // make inline?
inline int GetYear(date_t date) { return (date / 10000); }
inline int GetMonth(date_t date) { return (date / 100) % 100; }
inline int GetDay(date_t date) { return date % 100; }
int ApproxDateDiff(date_t a, date_t b); // returns a-b
std::wstring DateToWString(date_t date);
inline std::wstring toMonthWString_Short(date_t date)
{
	switch (GetMonth(date))
	{
	case 1:
		return L"Jan";
	case 2:
		return L"Feb";
	case 3:
		return L"Mar";
	case 4:
		return L"Apr";
	case 5:
		return L"May";
	case 6:
		return L"Jun";
	case 7:
		return L"Jul";
	case 8:
		return L"Aug";
	case 9:
		return L"Sep";
	case 10:
		return L"Oct";
	case 11:
		return L"Nov";
	case 12:
		return L"Dec";
	default:
		return L"NUL";
	}
}

enum class HitTest
{
	in,
	left,
	right,
	above,
	below,
	nowhere
};

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

class DPIScale
{
	static float scaleX;
	static float scaleY;
	static float dpiX;
	static float dpiY;

public:

	static void Initialize(ID2D1Factory *pFactory)
	{
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;
	}

	static D2D1_POINT_2F PixelsToDips(POINT p)
	{
		return D2D1::Point2F(static_cast<float>(p.x) / scaleX, static_cast<float>(p.y) / scaleY);
	}

	static D2D1_RECT_F PixelsToDips(RECT rc)
	{
		return D2D1::RectF(
			DPIScale::PixelsToDipsX(rc.left),
			DPIScale::PixelsToDipsY(rc.top),
			DPIScale::PixelsToDipsX(rc.right),
			DPIScale::PixelsToDipsY(rc.bottom)
		);
	}

	template <typename T>
	static float PixelsToDipsX(T x)
	{
		return static_cast<float>(x) / scaleX;
	}

	template <typename T>
	static float PixelsToDipsY(T y)
	{
		return static_cast<float>(y) / scaleY;
	}

	template <typename T>
	static int DipsToPixelsX(T x)
	{
		return static_cast<int>(static_cast<float>(x) * scaleX);
	}

	template <typename T>
	static int DipsToPixelsY(T y)
	{
		return static_cast<int>(static_cast<float>(y) * scaleY);
	}


};

class MouseTrackEvents
{
	bool m_bMouseTracking;

public:
	MouseTrackEvents() : m_bMouseTracking(false) {}

	void OnMouseMove(HWND hwnd)
	{
		if (!m_bMouseTracking)
		{
			// Enable mouse tracking.
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.hwndTrack = hwnd;
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&tme);
			m_bMouseTracking = true;
		}
	}
	void Reset(HWND hwnd)
	{
		m_bMouseTracking = false;
	}
};