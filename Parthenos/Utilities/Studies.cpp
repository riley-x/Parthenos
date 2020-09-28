#include "../stdafx.h"
#include "Studies.h"


std::pair<std::vector<date_t>, std::vector<double>> SMA(std::vector<OHLC> const& ohlc, size_t filter_size)
{
	return SMA(ohlc, 0, ohlc.size(), filter_size);
}

std::pair<std::vector<date_t>, std::vector<double>> SMA(
	std::vector<OHLC> const & ohlc,
	size_t filter_size,
	size_t iStart,
	size_t iEnd)
{
	size_t accStart = (iStart > filter_size) ? iStart - filter_size : 0; // where to start accumulating
	
	std::vector<date_t> dates(iEnd - iStart);
	std::vector<double> sma(iEnd - iStart);
	double sum = 0;
	int days = 0;

	for (size_t i = accStart; i < iEnd; i++)
	{
		sum += ohlc[i].close;
		if (i >= accStart + filter_size)
			sum -= ohlc[i - filter_size].close;
		else
			days++;

		if (i >= iStart)
		{
			dates[i - iStart] = ohlc[i].date;
			sma[i - iStart] = sum / days;
		}
	}
	return { dates, sma };
}
