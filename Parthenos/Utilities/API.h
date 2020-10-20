#pragma once

//#define TEST_IEX

#include "../stdafx.h"
#include "utilities.h"

const std::wstring ROOTDIR(L"C:/Users/riley/Documents/Finances/Parthenos/"); // C:/Users/riley/Documents/Finances/Parthenos/
extern std::wstring TDKEY;


enum class apiSource { iex, alpha, td };

enum class iexLSource { real, delayed, close, prevclose, null };

struct TimeSeries
{
	date_t date;
	double prices;
};

struct OHLC
{
	double open;
	double high;
	double low;
	double close;
	date_t date;
	uint32_t volume;

	std::wstring to_wstring(bool mini = false) const;
};

struct Quote
{
	iexLSource latestSource;
	int latestVolume;
	int avgTotalVolume;
	double open;
	double high;
	double low;
	double close;
	double latestPrice;
	double previousClose;
	double change;
	double changePercent;
	time_t latestUpdate;
	time_t closeTime;
	std::wstring ticker;

	std::wstring to_wstring() const;
};

struct Stats
{
	date_t exDividendDate;
	date_t nextEarningsDate;
	double beta;
	double week52high;
	double week52low;
	double dividendRate;
	double dividendYield;
	double year1ChangePercent;

	std::wstring to_wstring() const;
};

typedef std::vector<std::pair<Quote, Stats>> QStats;

double GetPrice(std::wstring ticker);
Quote GetQuote(std::wstring ticker, apiSource source = apiSource::iex);
Stats GetStats(std::wstring ticker);
std::pair<Quote, Stats> GetQuoteStats(std::wstring ticker);
QStats GetBatchQuoteStats(std::vector<std::wstring> tickers);
std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source = apiSource::iex, size_t last_n = 0);
std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source, size_t last_n, date_t& lastCloseDate);
std::vector<OHLC> GetSavedOHLC(std::wstring ticker, apiSource source,
	size_t last_n, int& days_to_get, date_t& lastCloseDate);
size_t FindDateOHLC(std::vector<OHLC> const& ohlc, date_t date); // returns ohlc.size() on fail

// returns a.date < b.date
inline bool OHLC_Compare(const OHLC& a, const OHLC& b)
{
	return a.date < b.date;
}

inline std::vector<double> GetMarketPrices(QStats const& qstats)
{
	std::vector<double> out;
	out.reserve(qstats.size());
	for (auto& x : qstats)	out.push_back(x.first.latestPrice);
	return out;
}

// Assumes 'qstats' is ordered by ticker
inline QStats::const_iterator FindQStat(QStats const& qstats, std::wstring ticker)
{
	QStats::const_iterator it = std::lower_bound(qstats.begin(), qstats.end(), ticker,
		[](std::pair<Quote, Stats> const& qs, std::wstring const& ticker)
	{ return qs.first.ticker < ticker; }
	);
	return it;
}

