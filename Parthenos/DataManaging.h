#pragma once

#include "stdafx.h"

enum class iexLSource 
{
	real,
	delayed,
	close,
	prevclose
};

typedef struct OHLC_struct {
	uint64_t time; // unix time
	double open;
	double high;
	double low;
	double close;
} OHLC;

typedef struct Quote_struct {
	double open;
	double close;
	double latestPrice;
	iexLSource latestSource;
	uint64_t latestUpdate;
	int latestVolume;
	int avgTotalVolume;
} Quote;

std::vector<OHLC> GetOHLC(std::wstring ticker, std::wstring range);
Quote GetQuote(std::wstring ticker);
