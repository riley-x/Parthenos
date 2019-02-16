#pragma once

///////////////////////////////////////////////////////////
// --- Messaging --- 
std::wstring OutputError(const std::wstring & msg);
std::system_error Error(const std::wstring & msg);
void OutputMessage(const std::wstring format, ...);

///////////////////////////////////////////////////////////
// --- Datetime --- 
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

///////////////////////////////////////////////////////////
// --- Misc --- 
inline bool inRect(D2D1_POINT_2F cursor, D2D1_RECT_F rect)
{
	return cursor.x >= rect.left &&
		cursor.x <= rect.right &&
		cursor.y >= rect.top &&
		cursor.y <= rect.bottom;
}

// Sees if x overlaps m horizontally, erring on returning false. 
// Assumes x is a rect created from InvalidateRect, in which case its values err on the small side. 
inline bool overlapHRect(D2D1_RECT_F x, D2D1_RECT_F m)
{
	return x.right > m.left && x.left <= m.right;
}

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}


namespace Colors
{
	const D2D1_COLOR_F MAIN_BACKGROUND = D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f);
	const D2D1_COLOR_F AXES_BACKGROUND = D2D1::ColorF(0.15f, 0.15f, 0.15f, 1.0f);
	const D2D1_COLOR_F MENU_BACKGROUND = D2D1::ColorF(0.1f, 0.1f, 0.1f, 1.0f);
	const D2D1_COLOR_F HIGHLIGHT = D2D1::ColorF(0.25f, 0.25f, 0.25f, 1.0f);
	const D2D1_COLOR_F DULL_LINE = D2D1::ColorF(0.25f, 0.25f, 0.25f, 1.0f);
	const D2D1_COLOR_F MEDIUM_LINE = D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f);
	const D2D1_COLOR_F BRIGHT_LINE = D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f);
	const D2D1_COLOR_F MAIN_TEXT = D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f);

	const D2D1_COLOR_F ACCENT = D2D1::ColorF(0.8f, 0.0f, 0.5f, 1.0f);
}

namespace Timers
{
	const int n_timers = 1;
	enum { IDT_CARET = 1 }; // no zero
	extern bool active[n_timers + 1]; // extra entry for easy indexing

	const UINT CARET_TIME = 750; // 0.75 seconds
}

///////////////////////////////////////////////////////////
class DPIScale
{
	static float scaleX;
	static float scaleY;
	static float dpiX;
	static float dpiY;

public:

	static inline void Initialize(ID2D1Factory *pFactory)
	{
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;
	}

	static inline D2D1_POINT_2F PixelsToDips(POINT p)
	{
		return D2D1::Point2F(static_cast<float>(p.x) / scaleX, static_cast<float>(p.y) / scaleY);
	}

	static inline D2D1_RECT_F PixelsToDips(RECT rc)
	{
		return D2D1::RectF(
			PixelsToDipsX(rc.left),
			PixelsToDipsY(rc.top),
			PixelsToDipsX(rc.right),
			PixelsToDipsY(rc.bottom)
		);
	}

	template <typename T>
	static inline float PixelsToDipsX(T x)
	{
		return static_cast<float>(x) / scaleX;
	}

	template <typename T>
	static inline float PixelsToDipsY(T y)
	{
		return static_cast<float>(y) / scaleY;
	}

	static inline RECT DipsToPixels(D2D1_RECT_F rect)
	{
		RECT out;
		out.left = DipsToPixelsX(rect.left);
		out.top = DipsToPixelsY(rect.top);
		out.right = DipsToPixelsX(rect.right);
		out.bottom = DipsToPixelsY(rect.bottom);
		return out;
	}

	template <typename T>
	static inline int DipsToPixelsX(T x)
	{
		return static_cast<int>(static_cast<float>(x) * scaleX);
	}

	template <typename T>
	static inline int DipsToPixelsY(T y)
	{
		return static_cast<int>(static_cast<float>(y) * scaleY);
	}

	static inline float SnapToPixelX(float x)
	{
		return (floor(x * scaleX + 0.5f) - 0.5f) / scaleX; // pixels seem to be aligned to 0.5 boundaries...
	}
};

///////////////////////////////////////////////////////////
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