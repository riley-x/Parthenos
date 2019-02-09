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
	time_t time; // unix time.
	uint64_t volume;

	std::wstring to_string()
	{
		return L"Date: "	+ toWString(time)
			+ L", Open: "	+ std::to_wstring(open)
			+ L", High: "	+ std::to_wstring(high)
			+ L", Low: "	+ std::to_wstring(low)
			+ L", Close: "	+ std::to_wstring(close)
			+ L", Volume: " + std::to_wstring(volume)
			+ L", Time: "	+ std::to_wstring(time)
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
} Quote;

std::vector<OHLC> GetOHLC(std::wstring ticker);
Quote GetQuote(std::wstring ticker);
