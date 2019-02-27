#pragma once

///////////////////////////////////////////////////////////
// --- Messaging --- 
std::wstring OutputError(const std::wstring & msg);
std::system_error Error(const std::wstring & msg);
void OutputMessage(const std::wstring format, ...);

///////////////////////////////////////////////////////////
// --- Datetime --- 
typedef uint32_t date_t; // 10000*yyyy + 100*mm + dd
date_t const DATE_T_1M = 100;
date_t const DATE_T_1YR = 10000;

BOOL SystemTimeToEasternTime(SYSTEMTIME const * sysTime, SYSTEMTIME * eastTime);

time_t TruncateToDay(time_t time);
std::wstring TimeToWString(time_t time);
time_t DateToTime(date_t date);
date_t GetDate(time_t time);
date_t GetCurrentDate();
inline date_t MkDate(int year, int month, int day) { return 10000 * year + 100 * month + day; }
inline int GetYear(date_t date) { return (date / 10000); }
inline int GetMonth(date_t date) { return (date / 100) % 100; }
inline int GetDay(date_t date) { return date % 100; }
inline void SetMonth(date_t & date, int month) 
{ 
	int year = GetYear(date); int day = GetDay(date);
	date = MkDate(year, month, day);
}
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

// When invalidating, converts to pixels then back to DIPs -> updateRect has smaller values
// than when invalidated.
inline bool overlapRect(D2D1_RECT_F targetRect, D2D1_RECT_F updateRect)
{
	return (updateRect.right - 1 > targetRect.left && updateRect.left + 1 < targetRect.right)
		&& (updateRect.bottom - 1 > targetRect.top && updateRect.top + 1 < targetRect.bottom);

}

// Sees if x overlaps m horizontally, erring on returning false. 
// Assumes x is a rect created from InvalidateRect, in which case its values err on the small side. 
inline bool overlapHRect(D2D1_RECT_F x, D2D1_RECT_F m)
{
	return x.right > m.left && x.left <= m.right;
}

inline bool equalRect(D2D1_RECT_F r1, D2D1_RECT_F r2)
{
	return r1.left == r2.left && r1.top == r2.top && r1.right == r2.right && r1.bottom == r2.bottom;
}

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

// with negative in from of the $ sign, commas, and 2 decimal places
inline std::wstring FormatDollar(double x)
{
	wchar_t buffer[30];
	swprintf_s(buffer, _countof(buffer), L"%.2lf", x);
	std::wstring out(buffer);

	int insertPosition = out.length() - 6;
	while (insertPosition > 0) {
		out.insert(insertPosition, L",");
		insertPosition -= 3;
	}

	insertPosition = (x < 0) ? 1 : 0;
	out.insert(insertPosition, L"$");
	return out;
}

// Returns the indices of v sorted based on the the values of v. Does not sort v itself
template <typename T, class Compare=std::less<>>
inline std::vector<size_t> sort_indexes(const std::vector<T> &v, Compare comp = {}) {

	// initialize original index locations
	std::vector<size_t> idx(v.size());
	std::iota(idx.begin(), idx.end(), 0);

	// sort indexes based on comparing values in v
	std::sort(idx.begin(), idx.end(),
		[&](size_t i1, size_t i2) {return comp(v[i1], v[i2]); });

	return idx;
}

// Given 'keys' mapping to equal sized 'vals', returns a vector containing those values corresponding
// to keys in 'filter'. Assumes 'keys' and 'filter' are ordered in the same sense. If 'filter' contains
// a value not in 'keys', the function will return values up to that filter.
template <typename T1, typename T2>
std::vector<T2> FilterByKeyMatch(std::vector<T1> keys, std::vector<T2> vals, std::vector<T1> filter)
{
	std::vector<T2> out;
	out.reserve(filter.size());

	size_t ikey = 0;
	size_t ifilter = 0;
	while (ifilter < filter.size() && ikey < keys.size())
	{
		if (filter[ifilter] == keys[ikey])
		{
			out.push_back(vals[ikey]);
			ifilter++;
		}
		ikey++;
	}

	return out;
}

namespace Colors
{
	const D2D1_COLOR_F HIGHLIGHT		= D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f);
	const D2D1_COLOR_F MAIN_BACKGROUND	= D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f);
	const D2D1_COLOR_F WATCH_BACKGROUND = D2D1::ColorF(0.16f, 0.16f, 0.16f, 1.0f);
	const D2D1_COLOR_F AXES_BACKGROUND	= D2D1::ColorF(0.15f, 0.15f, 0.15f, 1.0f);
	const D2D1_COLOR_F TITLE_BACKGROUND = D2D1::ColorF(0.13f, 0.13f, 0.135f, 1.0f);
	const D2D1_COLOR_F MENU_BACKGROUND	= D2D1::ColorF(0.1f, 0.1f, 0.1f, 1.0f);

	const D2D1_COLOR_F BRIGHT_LINE		= D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f);
	const D2D1_COLOR_F MEDIUM_LINE		= D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f);
	const D2D1_COLOR_F DULL_LINE		= D2D1::ColorF(0.25f, 0.25f, 0.25f, 1.0f);

	const D2D1_COLOR_F ALMOST_WHITE		= D2D1::ColorF(0.95f, 0.95f, 0.95f, 1.0f);
	const D2D1_COLOR_F MAIN_TEXT		= D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f);

	const D2D1_COLOR_F ACCENT			= D2D1::ColorF(0.8f, 0.0f, 0.5f, 1.0f);
	const D2D1_COLOR_F PURPLE			= D2D1::ColorF(0x8000C0);
	const D2D1_COLOR_F GREEN			= D2D1::ColorF(0x008040);
}

namespace Timers
{
	const int n_timers = 1;
	enum { IDT_CARET = 1 }; // no zero

	struct WndTimers
	{
		int nActiveP1[n_timers + 1] = {}; // extra entry for easy indexing
		// Set 0 for deleted, 1 to flag deletion (delays a little)
	};
	extern std::map<void*, WndTimers*> WndTimersMap; // Global map so don't have to pass timers to every object

	const UINT CARET_TIME = 750; // 0.75 seconds
}

namespace Cursor
{
	extern bool isSet; // refresh each WM_MOUSEMOVE
	extern HCURSOR active;
	const HCURSOR hArrow = LoadCursor(NULL, IDC_ARROW);
	const HCURSOR hIBeam = LoadCursor(NULL, IDC_IBEAM);

	inline void SetCursor(HCURSOR cursor) 
	{
		isSet = true;
		active = cursor;
		::SetCursor(cursor);
	}
 }


///////////////////////////////////////////////////////////
class DPIScale
{
	static float scaleX;
	static float scaleY;
	static float dpiX;
	static float dpiY;

	// Half / full pixel in DIPs
	static float halfPX;
	static float halfPY;
	static float fullPX;
	static float fullPY;

public:

	static inline void Initialize(ID2D1Factory *pFactory)
	{
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;

		halfPX = PixelsToDipsX(0.5);
		halfPY = PixelsToDipsY(0.5);
		fullPX = PixelsToDipsX(1);
		fullPY = PixelsToDipsY(1);
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

	static inline POINT DipsToPixels(D2D1_POINT_2F point)
	{
		POINT out;
		out.x = DipsToPixelsX(point.x);
		out.y = DipsToPixelsY(point.y);
		return out;
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
		return round(x * scaleX) / scaleX; 
	}

	static inline float SnapToPixelY(float y)
	{
		return round(y * scaleY) / scaleY;
	}

	static inline void SnapToPixel(D2D1_RECT_F & rect, bool inwards = true)
	{
		rect.left = SnapToPixelX(rect.left) - (1.0f - 2.0f * inwards) * halfPX;
		rect.top = SnapToPixelY(rect.top) - (1.0f - 2.0f * inwards) * halfPY;
		rect.right = SnapToPixelX(rect.right) + (1.0f - 2.0f * inwards) * halfPX;
		rect.bottom = SnapToPixelY(rect.bottom) + (1.0f - 2.0f * inwards) * halfPY;
	}

	// NOTE: when drawing lines, need to align to 0.5 boundaries. Use hp() below.
	// Use D2D1_STROKE_TRANSFORM_TYPE_HAIRLINE in future
	static inline float hpx() { return halfPX; }
	static inline float hpy() { return halfPY; }
	static inline float px() { return fullPX; }
	static inline float py() { return fullPY; }
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