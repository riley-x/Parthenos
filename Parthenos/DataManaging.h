#pragma once

#include "stdafx.h"
#include "utilities.h"

const std::wstring ROOTDIR(L"C:/Users/Riley/Documents/Finances/Parthenos/"); // C:/Users/Riley/Documents/Finances/Parthenos/

///////////////////////////////////////////////////////////
// --- OHLC ---

enum class apiSource { iex, alpha };

enum class iexLSource { real, delayed, close, prevclose };

typedef struct OHLC_struct 
{
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

typedef struct Quote_struct 
{
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


typedef struct Stats_struct 
{
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

double GetPrice(std::wstring ticker);
Quote GetQuote(std::wstring ticker);
Stats GetStats(std::wstring ticker);
std::pair<Quote, Stats> GetQuoteStats(std::wstring ticker);
std::vector<std::pair<Quote, Stats>> GetBatchQuoteStats(std::vector<std::wstring> tickers);
std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source = apiSource::iex, size_t last_n = 0);
bool OHLC_Compare(const OHLC & a, const OHLC & b);

///////////////////////////////////////////////////////////
// --- Portfolio ---

enum class Account : char { Robinhood, Arista, All };
enum class TransactionType : char { Transfer, Stock, Dividend, Interest, Fee, PutShort, PutLong, CallShort, CallLong };

inline bool isOption(TransactionType type)
{
	switch (type)
	{
	case TransactionType::PutShort:
	case TransactionType::PutLong:
	case TransactionType::CallShort:
	case TransactionType::CallLong:
		return true;
	default:
		return false;
	}
}

inline bool isShort(TransactionType type)
{
	switch (type)
	{
	case TransactionType::PutShort:
	case TransactionType::CallShort:
		return true;
	default:
		return false;
	}
}

namespace PortfolioObjects
{
	const size_t maxTickerLen = 11;
}

typedef struct Transaction_struct 
{
	Account account;			// 1
	TransactionType type;		// 2
	short tax_lot;				// 4	for sales - index into lots - 0 if FIFO (for stock) - Options must be fully qualified index
	wchar_t ticker[12] = {};	// 28
	int n;						// 32	positive for open, negative for close. Number of contracts for options
	date_t date;				// 36
	date_t expiration;			// 40	for stock dividends, this is the ex date
	double value;				// 48	include fees here
	double price;				// 56	don't include fees here
	double strike;				// 64

	inline std::wstring to_wstring() const
	{
		return L"Account: "		+ std::to_wstring(static_cast<int>(account))
			+ L", Type: "		+ std::to_wstring(static_cast<int>(type))
			+ L", Ticker: "		+ std::wstring(ticker)
			+ L", n: "			+ std::to_wstring(n)
			+ L", Date: "		+ DateToWString(date)
			+ L", Expiration: " + DateToWString(expiration)
			+ L", Value: "		+ std::to_wstring(value)
			+ L", Price: "		+ std::to_wstring(price)
			+ L", Strike: "		+ std::to_wstring(strike)
			+ L"\n";
	}
} Transaction;

// Store sell lots in addition to buy lots to properly wait for dividends
typedef struct TaxLot_struct 
{
	int active; // 1 = buy, -1 = sale
	int n;
	int tax_lot; // for sales, index of active lot to sell (MUST PROCESS SALES IN ORDER)
	date_t date;
	double price; 
	double realized; // i.e. dividends, sales, fees
	// 32 bytes

	inline std::wstring to_wstring() const
	{
		return L"n: "			+ std::to_wstring(n)
			+ L", active: "		+ std::to_wstring(active)
			+ L", tax_lot: "	+ std::to_wstring(tax_lot)
			+ L", date: "		+ DateToWString(date)
			+ L", price: "		+ std::to_wstring(price)
			+ L", realized: "	+ std::to_wstring(realized)
			+ L"\n";
	}
} TaxLot;

typedef struct Option_struct
{
	TransactionType type;
	char PAD[3];
	int n; // shares underlying, not contracts
	date_t date;
	date_t expiration;
	float price;
	float strike;
	float realized; // from sell-to-open or partial sell-to-close
	float RESERVED;
	// 32 bytes

	inline std::wstring to_wstring() const
	{
		return L"type: "		+ std::to_wstring(static_cast<int>(type))
			+ L", n: "			+ std::to_wstring(n)
			+ L", date: "		+ DateToWString(date)
			+ L", expiration: " + DateToWString(expiration)
			+ L", price: "		+ std::to_wstring(price)
			+ L", strike: "		+ std::to_wstring(strike)
			+ L", realized: "	+ std::to_wstring(realized)
			+ L"\n";
	}
} Option;

typedef struct HoldingHeader_struct
{
	Account account;
	char PAD[3];
	short nLots;
	short nOptions;
	// Running sums from sold lots to calculate APY. DOESN'T include stored lots
	double sumWeights; // sum_transactions (cost) ; divide sumReal1Y by this to get weighted APY
	double sumReal1Y; // realized * 365 / days_held    ==    cost * (realized/cost) / (days_held/365)
	double sumReal; // realized
	// 32 bytes
	
	// For cash, sumWeights = cash from transfers, sumReal is interest, else 0

	inline std::wstring to_wstring() const
	{
		return L"Account: "		+ std::to_wstring(static_cast<int>(account))
			+ L", nLots: "		+ std::to_wstring(nLots)
			+ L", nOptions: "	+ std::to_wstring(nOptions)
			+ L", sumWeights: " + std::to_wstring(sumWeights)
			+ L", sumReal1Y: "	+ std::to_wstring(sumReal1Y)
			+ L", sumReal: "	+ std::to_wstring(sumReal)
			+ L"\n";
	}
} HoldingHeader;

typedef struct TickerInfo_struct
{
	int nAccounts;
	wchar_t ticker[14];
} TickerInfo;

typedef union Holdings_union
{
	TickerInfo tickerInfo;
	TaxLot lot;
	HoldingHeader head;
	Option option;
} Holdings;

typedef struct Position_struct
{
	int n;					
	int shares_collateral;
	double cash_collateral;
	double avgCost;			// of shares
	double marketPrice;		// for CASH, cash from transfers
	double realized_held;	// == sum_lots ( lot.realized ), shares only. For CASH, cash from interest
	double realized_unheld; // == head.sumReal + proceeds of any open option positions
	double unrealized;
	double APY;
	std::wstring ticker;

	inline std::wstring to_wstring() const
	{
		return L"Ticker: "		+ ticker
			+ L", n: "			+ std::to_wstring(n)
			+ L", avgCost: "	+ std::to_wstring(avgCost)
			+ L", price: "		+ std::to_wstring(marketPrice)
			+ L", realized (held): "	+ std::to_wstring(realized_held)
			+ L", realized (unheld): "	+ std::to_wstring(realized_unheld)
			+ L", unrealized: "	+ std::to_wstring(unrealized)
			+ L", APY: "		+ std::to_wstring(APY)
			+ L", collat (shares): "	+ std::to_wstring(shares_collateral)
			+ L", collat (cash): "		+ std::to_wstring(cash_collateral)
			+ L"\n";
	}
} Position;



inline double GetWeightedAPY(double gain, date_t start_date, date_t end_date)
{
	time_t time_held = DateToTime(end_date) - DateToTime(start_date);
	if (time_held <= 0) return gain * 365.0; // assume 1 day
	return gain * 365.0 * 86400.0 / time_held;
}
inline double GetAPY(double gain, double cost_in, date_t start_date, date_t end_date)
{
	time_t time_held = DateToTime(end_date) - DateToTime(start_date);
	if (time_held <= 0) return (gain / cost_in) * 365.0; // assume 1 day
	return (gain / cost_in) * 365.0 * 86400.0 / time_held;
}

std::vector<Transaction> CSVtoTransactions(std::wstring filepath);
void AddTransactionToHoldings(std::vector<std::vector<Holdings>> & holdings, Transaction const & transaction);
std::vector<std::vector<Holdings>> FullTransactionsToHoldings(std::vector<Transaction> const & transactions);
std::vector<std::vector<Holdings>> FlattenedHoldingsToTickers(std::vector<Holdings> const & holdings);
std::vector<Position> HoldingsToPositions(std::vector<std::vector<Holdings>> const & holdings, Account account, date_t date);

void PrintTickerHoldings(std::vector<Holdings> const & h);
inline void PrintFlattenedHoldings(std::vector<Holdings> const & h)
{
	auto holdings = FlattenedHoldingsToTickers(h);
	for (auto const & i : holdings)
	{
		PrintTickerHoldings(i);
	}
}

inline std::vector<std::wstring> GetTickers(std::vector<Position> const & positions)
{
	std::vector<std::wstring> out;
	out.reserve(positions.size());
	for (auto const & x : positions)
	{
		if (x.ticker == L"CASH") continue;
		out.push_back(x.ticker);
	}
	return out;
}