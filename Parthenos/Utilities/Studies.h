#pragma once

#include "../stdafx.h"
#include "API.h"

// Calculates the simple moving average from the given OHLC range and filter size.
// Returns the dates and SMA of the range.
std::pair<std::vector<date_t>, std::vector<double>> SMA(
	std::vector<OHLC> const& ohlc,
	size_t filter_size
);
// --- with range [iStart, iEnd)
std::pair<std::vector<date_t>, std::vector<double>> SMA(
	std::vector<OHLC> const& ohlc,
	size_t iStart,
	size_t iEnd,
	size_t filter_size
);

// Calculates the relative strength index from the given OHLC range and filter size.
// Returns the dates and RSI of the range.
std::pair<std::vector<date_t>, std::vector<double>> RSI(
	std::vector<OHLC> const& ohlc,
	size_t filter_size = 14
);
// --- with range [iStart, iEnd)
std::pair<std::vector<date_t>, std::vector<double>> RSI(
	std::vector<OHLC> const& ohlc,
	size_t iStart,
	size_t iEnd,
	size_t filter_size = 14
);


// Calculates the Bollinger Bands from the given OHLC range, period, and number of standard deviations.
// Returns the dates and (lower, upper) bands.
std::tuple<std::vector<date_t>, std::vector<double>, std::vector<double>> BollingerBands(
	std::vector<OHLC> const& ohlc,
	unsigned period = 20,
	unsigned dev = 2
);
// --- with range [iStart, iEnd) in ohlc.
std::tuple<std::vector<date_t>, std::vector<double>, std::vector<double>> BollingerBands(
	std::vector<OHLC> const& ohlc,
	size_t iStart,
	size_t iEnd,
	unsigned period = 20,
	unsigned dev = 2
);
