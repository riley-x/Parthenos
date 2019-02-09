#pragma once


std::wstring OutputError(const std::wstring & msg);
std::system_error Error(const std::wstring & msg);
void OutputMessage(const std::wstring format, ...);

time_t TruncateToDay(time_t time);
int GetDay(time_t time);
std::wstring toWString(time_t time);


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