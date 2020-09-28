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
	size_t filter_size,
	size_t iStart,
	size_t iEnd
);