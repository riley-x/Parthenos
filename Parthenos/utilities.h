#pragma once

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

public:

	static float dpiX;
	static float dpiY;

	static void Initialize(ID2D1Factory *pFactory)
	{
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;
	}

	template <typename T>
	static D2D1_POINT_2F PixelsToDips(T x, T y)
	{
		return D2D1::Point2F(static_cast<float>(x) / scaleX, static_cast<float>(y) / scaleY);
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

};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;
float DPIScale::dpiX = 96.0f;
float DPIScale::dpiY = 96.0f;
