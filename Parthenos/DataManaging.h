#pragma once

#include "stdafx.h"
#include "utilities.h"

const std::wstring ROOTDIR(L"C:/Users/Riley/Documents/Finances/Parthenos/"); // C:/Users/Riley/Documents/Finances/Parthenos/


enum class apiSource { iex, alpha };

enum class iexLSource { real, delayed, close, prevclose };

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
	int latestVolume;
	int avgTotalVolume;
	iexLSource latestSource;
	double open;
	double close;
	double latestPrice;
	double previousClose;
	double change;
	double changePercent;
	time_t latestUpdate;
	time_t closeTime;

	inline std::wstring to_wstring() const
	{
		return L"Date: "			+ TimeToWString(latestUpdate)
			+ L", Open: "			+ std::to_wstring(open)
			+ L", Close: "			+ std::to_wstring(close)
			+ L", latestPrice: "	+ std::to_wstring(latestPrice)
			+ L", latestSource: "	+ std::to_wstring(static_cast<int>(latestSource))
			+ L", latestVolume: "	+ std::to_wstring(latestVolume)
			+ L", avgTotalVolume: " + std::to_wstring(avgTotalVolume)
			+ L"\nlatestUpdate: "	+ std::to_wstring(latestUpdate)
			+ L", previousClose: "	+ std::to_wstring(previousClose)
			+ L", change: "			+ std::to_wstring(change)
			+ L", changePercent: "	+ std::to_wstring(changePercent)
			+ L", closeTime: "		+ TimeToWString(closeTime)
			+ L"\n";
	}
} Quote;


typedef struct Stats_struct {
	date_t exDividendDate;
	double beta;
	double week52high;
	double week52low;
	double dividendRate;
	double dividendYield;
	double year1ChangePercent;

	inline std::wstring to_wstring() const
	{
		return L"Beta: "			+ std::to_wstring(beta)
			+ L", 52WeekHigh: "		+ std::to_wstring(week52high)
			+ L", 52WeekLow: "		+ std::to_wstring(week52low)
			+ L", dividendRate: "	+ std::to_wstring(dividendRate)
			+ L", dividendYield: "	+ std::to_wstring(dividendYield)
			+ L", exDividendDate: " + DateToWString(exDividendDate)
			+ L", 1YearChange%: "	+ std::to_wstring(year1ChangePercent)
			+ L"\n";
	}
} Stats;


std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source = apiSource::iex, size_t last_n = 0);
Quote GetQuote(std::wstring ticker);
Stats GetStats(std::wstring ticker);
bool OHLC_Compare(const OHLC & a, const OHLC & b);


enum class Account : char { Robinhood, Arista };
enum class TransactionType : char { Transfer, Stock, Dividend, Interest, Fee, PutShort, PutLong, CallShort, CallLong };

typedef struct Transaction_struct {
	Account account;			// 1
	TransactionType type;		// 2
	wchar_t ticker[13] = {};	// 28
	int n;						// 32
	date_t date;				// 36
	date_t expiration;			// 40
	double value;				// 48
	double price;				// 56
	double strike;				// 64

	static size_t const maxTickerLen = 12;
	inline std::wstring to_wstring() const
	{
		return L"Account: "		+ std::to_wstring(static_cast<int>(account))
			+ L", Type: "		+ std::to_wstring(static_cast<int>(type))
			+ L", Ticker: "		+ std::wstring(ticker)
			+ L", n: "			+ std::to_wstring(n)
			+ L", Date: "		+ std::to_wstring(date)
			+ L", Expiration: " + DateToWString(expiration)
			+ L", Value: "		+ std::to_wstring(value)
			+ L", Price: "		+ std::to_wstring(price)
			+ L", Strike: "		+ std::to_wstring(strike)
			+ L"\n";
	}
} Transaction;


std::vector<Transaction> CSVtoTransactions(std::wstring filepath);