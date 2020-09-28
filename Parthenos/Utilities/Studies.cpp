#include "../stdafx.h"
#include "Studies.h"


std::pair<std::vector<date_t>, std::vector<double>> SMA(std::vector<OHLC> const& ohlc, size_t filter_size)
{
	return SMA(ohlc, 0, ohlc.size(), filter_size);
}

std::pair<std::vector<date_t>, std::vector<double>> SMA(
	std::vector<OHLC> const & ohlc,
	size_t iStart,
	size_t iEnd,
	size_t filter_size)
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

std::pair<std::vector<date_t>, std::vector<double>> RSI(std::vector<OHLC> const& ohlc, size_t filter_size)
{
	return RSI(ohlc, 0, ohlc.size(), filter_size);
}


std::pair<std::vector<date_t>, std::vector<double>> RSI(
	std::vector<OHLC> const& ohlc,
	size_t iStart,
	size_t iEnd,
	size_t filter_size
)
{
	size_t accStart = (iStart > filter_size + 1) ? iStart - filter_size - 1 : 0; // where to start accumulating
	
	std::vector<date_t> dates(iEnd - iStart);
	std::vector<double> rsi(iEnd - iStart);

	double gain = 0; // These store the sum / exponential weighted average
	double loss = 0;

	for (size_t i = accStart + 1; i < iEnd; i++)
	{
		double change = ohlc[i].close - ohlc[i - 1].close;

		if (i < accStart + filter_size + 1) // Use SMA for the first 14 days
		{
			if (change < 0) loss -= change;
			else gain += change;
		}
		else // Use SMMA for the remaining days
		{
			gain /= filter_size;
			loss /= filter_size;
			gain = (filter_size - 1) * gain + max(change, 0); // No need for /14 here. Cancels in RS ratio,
			loss = (filter_size - 1) * loss - min(change, 0); // and handled for next loop above.
		}

		if (i >= iStart)
		{
			dates[i - iStart] = ohlc[i].date;
			if (loss == 0) rsi[i - iStart] = 100;
			else
			{
				double rs = gain / loss;
				rsi[i - iStart] = 100 - 100 / (1 + rs);
				//OutputMessage(L"%lf\n", rsi[i - iStart]);
			}
		}
	}

	return { dates, rsi };
}