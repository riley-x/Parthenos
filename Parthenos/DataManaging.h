#pragma once

#include "stdafx.h"
#include "utilities.h"

const std::wstring ROOTDIR(L"C:/Users/Riley/Documents/Finances/Parthenos/"); // C:/Users/Riley/Documents/Finances/Parthenos/

///////////////////////////////////////////////////////////
// --- Stock Information ---

enum class apiSource { iex, alpha };

enum class iexLSource { real, delayed, close, prevclose };

typedef struct TimeSeries_struct
{
	date_t date;
	double prices;
} TimeSeries;

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
	std::wstring ticker;

	inline std::wstring to_wstring() const
	{
		return ticker + L": "
			+ L", Date: "			+ TimeToWString(latestUpdate)
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

typedef std::vector<std::pair<Quote, Stats>> QStats;

double GetPrice(std::wstring ticker);
Quote GetQuote(std::wstring ticker);
Stats GetStats(std::wstring ticker);
std::pair<Quote, Stats> GetQuoteStats(std::wstring ticker);
QStats GetBatchQuoteStats(std::vector<std::wstring> tickers);
std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source = apiSource::iex, size_t last_n = 0);
std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source , size_t last_n, date_t & lastCloseDate);

// returns a.date < b.date
inline bool OHLC_Compare(const OHLC & a, const OHLC & b)
{
	return a.date < b.date;
}

inline std::vector<double> GetMarketPrices(QStats const & qstats)
{
	std::vector<double> out;
	out.reserve(qstats.size());
	for (auto & x : qstats)	out.push_back(x.first.latestPrice);
	return out;
}

// Assumes 'qstats' is ordered by ticker
inline QStats::const_iterator FindQStat(QStats const & qstats, std::wstring ticker)
{
	QStats::const_iterator it = std::lower_bound(qstats.begin(), qstats.end(), ticker,
		[](std::pair<Quote, Stats> const & qs, std::wstring const & ticker)
		{ return qs.first.ticker < ticker; }
	);
	return it;
}

///////////////////////////////////////////////////////////
namespace PortfolioObjects
{
	const size_t maxTickerLen = 11;
}

///////////////////////////////////////////////////////////
// --- Transactions ---

enum class TransactionType : char { Transfer, Stock, Dividend, Interest, Fee, PutShort, PutLong, CallShort, CallLong };

std::vector<std::wstring> const TRANSACTIONTYPE_STRINGS = { L"Transfer", L"Stock", L"Dividend", L"Interest", L"Fee",
	L"Short Put", L"Long Put", L"Short Call", L"Long Call" };

inline std::wstring to_wstring(TransactionType t)
{
	return TRANSACTIONTYPE_STRINGS[static_cast<char>(t)];
}

inline TransactionType TransactionStringToEnum(std::wstring const & s)
{
	auto it = std::find(TRANSACTIONTYPE_STRINGS.begin(), TRANSACTIONTYPE_STRINGS.end(), s);
	if (it == TRANSACTIONTYPE_STRINGS.end()) throw std::invalid_argument("Bad transaction");
	return static_cast<TransactionType>(it - TRANSACTIONTYPE_STRINGS.begin());
}

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

typedef struct Transaction_struct
{
	char account;				// 1
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
		return L"Account: " + std::to_wstring(static_cast<int>(account))
			+ L", Type: " + std::to_wstring(static_cast<int>(type))
			+ L", Ticker: " + std::wstring(ticker)
			+ L", n: " + std::to_wstring(n)
			+ L", Date: " + DateToWString(date)
			+ L", Expiration: " + DateToWString(expiration)
			+ L", Value: " + std::to_wstring(value)
			+ L", Price: " + std::to_wstring(price)
			+ L", Strike: " + std::to_wstring(strike)
			+ L"\n";
	}
} Transaction;

std::vector<Transaction> CSVtoTransactions(std::wstring filepath);


///////////////////////////////////////////////////////////
// --- Holdings ---

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
	char account;
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

// A vector sorted by tickers. Each ticker has a std::vector<Holdings> with the following elements:
//		[0]: A TickerInfo struct with the ticker and the number of accounts
// For each account:
//		[1]: A HoldingsHeader struct with the number of stock lots and option lots
//		[2]: The stock lots
//		[3]: The option lots
typedef std::vector<std::vector<Holdings>> NestedHoldings;

void AddTransactionToHoldings(NestedHoldings & holdings, Transaction const & transaction);
NestedHoldings FullTransactionsToHoldings(std::vector<Transaction> const & transactions);
NestedHoldings FlattenedHoldingsToTickers(std::vector<Holdings> const & holdings);
void PrintTickerHoldings(std::vector<Holdings> const & h);

inline void PrintFlattenedHoldings(std::vector<Holdings> const & h)
{
	auto holdings = FlattenedHoldingsToTickers(h);
	for (auto const & i : holdings)
	{
		PrintTickerHoldings(i);
	}
}

inline std::vector<std::wstring> GetTickers(NestedHoldings const & holdings, bool cash = false)
{
	std::vector<std::wstring> out;
	out.reserve(holdings.size());
	for (auto const & x : holdings)
	{
		std::wstring ticker(x[0].tickerInfo.ticker);
		if (!cash && ticker == L"CASH") continue;
		out.push_back(ticker);
	}
	return out;
}

inline double GetIntrinsicValue(Option const & opt, double latest)
{
	switch (opt.type)
	{
	case TransactionType::PutShort:
		return (latest < opt.strike) ? opt.n * (latest - opt.strike) : 0.0;
	case TransactionType::PutLong:
		return (latest < opt.strike) ? opt.n * (opt.strike - latest) : 0.0;
	case TransactionType::CallShort:
		return (latest > opt.strike) ? opt.n * (opt.strike - latest) : 0.0;
	case TransactionType::CallLong:
		return (latest > opt.strike) ? opt.n * (latest - opt.strike) : 0.0;
	default:
		return 0.0;
	}
}


///////////////////////////////////////////////////////////
// --- Positions ---

// Current position for a stock at a given day
typedef struct Position_struct
{
	int n;					
	int shares_collateral;
	double cash_collateral;
	double avgCost;			// of shares
	double marketPrice;		// for CASH, cash from transfers
	double realized_held;	// == sum_lots ( lot.realized ), shares only. For CASH, cash from interest
	double realized_unheld; // == head.sumReal + proceeds of any open option positions. For CASH, net cash from transactions
	double unrealized;
	double APY;
	std::wstring ticker;
	std::vector<Option> options; // p/l already included in above, but useful to keep around

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

std::vector<Position> HoldingsToPositions(NestedHoldings const & holdings,
	char account, date_t date, std::vector<double> prices);

// returns a.front().ticker < ticker
inline bool Positions_Compare(const Position a, std::wstring const & ticker)
{
	return std::wstring(a.ticker) < ticker;
}

inline std::vector<std::wstring> GetTickers(std::vector<Position> const & positions, bool cash = false)
{
	std::vector<std::wstring> out;
	out.reserve(positions.size());
	for (auto const & x : positions)
	{
		if (!cash && x.ticker == L"CASH") continue;
		out.push_back(x.ticker);
	}
	return out;
}

inline std::vector<double> GetMarketValues(std::vector<Position> const & positions, bool cash = false)
{
	std::vector<double> out;
	out.reserve(positions.size());
	for (auto const & x : positions)
	{
		if (x.ticker == L"CASH")
		{
			if (cash) out.push_back(x.marketPrice + x.realized_held);
		}
		else out.push_back(x.n * x.marketPrice);
	}
	return out;
}

// (Free cash, total transfers)
inline std::pair<double, double> GetCash(std::vector<Position> const & positions)
{
	for (auto const & x : positions)
	{
		if (x.ticker == L"CASH")
			return { x.marketPrice + x.realized_held + x.realized_unheld, x.marketPrice };
	}
	return { 0, 0 };
}


///////////////////////////////////////////////////////////
// --- Equity history ---

std::vector<TimeSeries> CalculateFullEquityHistory(char account, std::vector<Transaction> const & trans);
void UpdateEquityHistory(std::vector<TimeSeries> & hist, std::vector<Position> const & positions, QStats const & qstats);
