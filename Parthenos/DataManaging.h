#pragma once

#include "stdafx.h"
#include "utilities.h"

enum class iexLSource 
{
	real,
	delayed,
	close,
	prevclose
};

typedef struct OHLC_struct {
	double open;
	double high;
	double low;
	double close;
	date_t date;
	uint32_t volume;

	std::wstring to_wstring() const
	{
		return L"Date: "	+ DateToWString(date)
			+ L", Open: "	+ std::to_wstring(open)
			+ L", High: "	+ std::to_wstring(high)
			+ L", Low: "	+ std::to_wstring(low)
			+ L", Close: "	+ std::to_wstring(close)
			+ L", Volume: " + std::to_wstring(volume)
			+ L"\n";
	}
} OHLC;

typedef struct Quote_struct {
	double open;
	double close;
	double latestPrice;
	iexLSource latestSource;
	time_t latestUpdate;
	int latestVolume;
	int avgTotalVolume;

	std::wstring to_wstring() const
	{
		return L"Date: "			+ TimeToWString(latestUpdate)
			+ L", Open: "			+ std::to_wstring(open)
			+ L", Close: "			+ std::to_wstring(close)
			+ L", latestPrice: "	+ std::to_wstring(latestPrice)
			+ L", latestSource: "	+ std::to_wstring(static_cast<int>(latestSource))
			+ L", latestVolume: "	+ std::to_wstring(latestVolume)
			+ L", avgTotalVolume: " + std::to_wstring(avgTotalVolume)
			+ L", latestUpdate: "	+ std::to_wstring(latestUpdate)
			+ L"\n";
	}
} Quote;

std::vector<OHLC> GetOHLC(std::wstring ticker);
Quote GetQuote(std::wstring ticker);
