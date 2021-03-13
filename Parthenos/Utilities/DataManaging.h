#pragma once

//#define TEST_IEX

#include "../stdafx.h"
#include "utilities.h"
#include "API.h"


///////////////////////////////////////////////////////////
namespace PortfolioObjects
{
	const size_t maxTickerLen = 11;
}

///////////////////////////////////////////////////////////
// --- Transactions ---

enum class TransactionType : char { Transfer, Stock, Dividend, Interest, Fee, PutShort, PutLong, CallShort, CallLong, Custom };

std::vector<std::wstring> const TRANSACTIONTYPE_STRINGS = { L"Transfer", L"Stock", L"Dividend", L"Interest", L"Fee",
	L"Short Put", L"Long Put", L"Short Call", L"Long Call", L"Custom" };

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

inline std::wstring OptToLetter(TransactionType type)
{
	switch (type)
	{
	case TransactionType::CallLong:
	case TransactionType::CallShort:
		return L"C";
	case TransactionType::PutShort:
	case TransactionType::PutLong:
		return L"P";
	default:
		return L"";
	}
}

struct Transaction
{
	char account;				// 1
	TransactionType type;		// 2
	short tax_lot;				// 4	for sales - index into lots - 0 if FIFO (for stock) - Options must be fully qualified index
	wchar_t ticker[12] = {};	// 28	must be all uppercase
	int n;						// 32	positive for open, negative for close. Number of contracts for options
	date_t date;				// 36
	date_t expiration;			// 40	for stock dividends, this is the ex date
	double value;				// 48	include fees here
	double price;				// 56	don't include fees here
	double strike;				// 64

	std::wstring to_wstring() const;
	std::wstring to_wstring(std::vector<std::wstring> const & accounts) const;

	// For type==Custom, ticker is a tag that defines the custom operation,
	// which is hard-coded in the add transaction functions.
};

inline bool operator==(const Transaction &t1, const Transaction &t2)
{
	return t1.account == t2.account
		&& t1.type == t2.type
		&& t1.tax_lot == t2.tax_lot
		&& std::wstring(t1.ticker) == std::wstring(t2.ticker)
		&& t1.n == t2.n
		&& t1.date == t2.date
		&& t1.expiration == t2.expiration
		&& t1.value == t2.value
		&& t1.price == t2.price
		&& t1.strike == t2.strike;
}

inline bool operator!=(const Transaction &t1, const Transaction &t2)
{
	return !(t1 == t2);
}

std::vector<Transaction> CSVtoTransactions(std::wstring filepath);


///////////////////////////////////////////////////////////////////////////////
//                                 Holdings                                  //
///////////////////////////////////////////////////////////////////////////////
// Store holdings using a 32 byte union. This allows all holdings to be      //
// stored as a single vector. The holding header struct identifies the type  //
// of subsequent structs.													 //
//																			 //
// A std::vector<Holdings> has the following elements:						 //
//		[0]: A TickerInfo struct with the ticker and the number of accounts  //
// For each account:														 //
//		[1]: A AccountHeader struct with the number of lots					 //
//		[2]: The account's lots												 //
// The header for the next account is then after the last lot, etc.			 //
///////////////////////////////////////////////////////////////////////////////


struct Lot
{
	TransactionType type = TransactionType::Custom;
	char PAD[3] = {};
	int n = 0;				// shares or contracts
	date_t date = 0;
	date_t expiration = 0;
	float price = 0;
	float strike = 0;
	float dividends = 0;	// this is PER SHARE, i.e. should not change with n
	float fees = 0;			// value + price * n, includes fees and rounding errors

	inline std::wstring to_wstring(bool dump = false) const
	{
		if (!dump && isOption(type)) 
			return OptToLetter(type) + FormatMsg(L"%.2f-%u", strike, expiration);

		return L"type: "		+ std::to_wstring(static_cast<int>(type))
			+ L", n: "			+ std::to_wstring(n)
			+ L", date: "		+ DateToWString(date)
			+ L", expiration: " + DateToWString(expiration)
			+ L", price: "		+ std::to_wstring(price)
			+ L", strike: "		+ std::to_wstring(strike)
			+ L", dividends: "  + std::to_wstring(dividends)
			+ L", feeds: "		+ std::to_wstring(fees)
			+ L"\n";
	}
};

struct AccountHeader
{
	char account = 0;
	char PAD[3] = {};
	int nLots = 0;
	// Running sums from sold lots to calculate APY. DOESN'T include stored lots
	double sumWeights = 0; // sum_transactions (cost) ; divide sumReal1Y by this to get weighted APY
	double sumReal1Y = 0; // realized * 365 / days_held    ==    cost * (realized/cost) / (days_held/365)
	double sumReal = 0; // realized
	// 32 bytes
	
	// For cash, sumWeights = cash from transfers, sumReal is interest, else 0

	inline std::wstring to_wstring() const
	{
		return L"Account: "		+ std::to_wstring(static_cast<int>(account))
			+ L", nLots: "		+ std::to_wstring(nLots)
			+ L", sumWeights: " + std::to_wstring(sumWeights)
			+ L", sumReal1Y: "	+ std::to_wstring(sumReal1Y)
			+ L", sumReal: "	+ std::to_wstring(sumReal)
			+ L"\n";
	}
};

struct TickerInfo
{
	int nAccounts = 0;
	wchar_t ticker[14] = {};
	// 32 bytes
};

union Holdings
{
	Holdings() : tickerInfo() {}

	TickerInfo tickerInfo;
	AccountHeader head;
	Lot lot;
};

///////////////////////////////////////////////////////////
// Main Functions

void AddTransactionToHoldings(std::vector<Holdings>& holdings, Transaction const& transaction);
std::vector<Holdings> FullTransactionsToHoldings(std::vector<Transaction> const& transactions);

std::wstring HoldingsToWString(std::vector<Holdings> const& h);

///////////////////////////////////////////////////////////
// Utilities

double GetIntrinsicValue(Lot const& opt, double latest);

std::vector<size_t> GetTickerHeaders(std::vector<Holdings> const& h);
size_t GetTickerIndex(std::vector<Holdings> const& h, std::wstring const& ticker);
std::vector<std::wstring> GetTickers(std::vector<Holdings> const& holdings, bool cash = false);

std::vector<Holdings>::iterator getAccountHeader(std::vector<Holdings>& holdings, 
	std::vector<Holdings>::iterator tickerHeader, char acc);
//size_t GetAccountIndex(std::vector<Holdings> const& h, std::wstring const& ticker, char acc);
std::vector<size_t> GetAccountHeaders(std::vector<Holdings> const& h, std::wstring const& ticker, char acc);
std::vector<size_t> GetAccountHeaders(std::vector<Holdings> const& h, size_t tickerHeader, char acc);


std::vector<Holdings>::iterator addAccount(std::vector<Holdings>& holdings, std::vector<Holdings>::iterator tickerHeader, char acc);


// In the case of a stock merger, add all return information from 'mergee' into 'target'.
// The merger is indicated in the transactions via a sale of mergee stock and purchase of target stock.
void MergeHoldings(std::vector<Holdings> & target, std::vector<Holdings> mergee);


///////////////////////////////////////////////////////////////////////////////
//                                 Positions                                 //
///////////////////////////////////////////////////////////////////////////////

enum class OptionType { CSP, CC, PCS, LP, LC, undefined };

inline OptionType transToOptionType(TransactionType t)
{
	switch (t)
	{
	case TransactionType::CallLong: return OptionType::LC;
	case TransactionType::CallShort: return OptionType::CC;
	case TransactionType::PutLong: return OptionType::LP;
	case TransactionType::PutShort: return OptionType::CSP;
	default: return OptionType::undefined;
	}
}

struct OptionPosition
{
	OptionType type;
	unsigned shares; // shares underlying
	unsigned shares_collateral; // For short positions
	date_t expiration;
	double strike; // primary
	double strike2; // ancillary
	double price;
	// double realized; Probably not needed since realized calculated in position below, no need for separate per-option
	double cash_collateral;

	std::wstring to_wstring() const
	{
		switch (type)
		{
		case OptionType::CC:
			return FormatMsg(L"CC%.2f-%u", strike, expiration);
		case OptionType::CSP:
			return FormatMsg(L"CSP%.2f-%u", strike, expiration);
		case OptionType::LC:
			return FormatMsg(L"C%.2f-%u", strike, expiration);
		case OptionType::LP:
			return FormatMsg(L"P%.2f-%u", strike, expiration);
		case OptionType::PCS:
			return FormatMsg(L"PCS%.2f-%.2f-%u", strike, strike2, expiration);
		default:
			return L"";
		}
	}
};


// Current position for a stock at a given day
struct Position
{
	int n;					
	int shares_collateral; // currently unused
	double cash_collateral;
	double avgCost;			// of shares
	double marketPrice;		// for CASH, cash from transfers
	double realized_held;	// == sum(lot.realized) for shares. For CASH, cash from interest
	double realized_unheld; // == head.sumReal + proceeds of open options. For CASH, net cash from transactions
	double unrealized;		// includes intrinsic value gain(long) / loss(short) from options
	double APY;
	std::wstring ticker;
	std::vector<OptionPosition> options; // p/l already included in above, but useful to keep around

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
};

std::vector<Position> HoldingsToPositions(std::vector<Holdings> const & holdings,
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
		if (cash && x.ticker == L"CASH") out.push_back(x.marketPrice + x.realized_held);
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

inline std::vector<std::vector<OptionPosition>> GetOptions(std::vector<Position> const & positions)
{
	std::vector<std::vector<OptionPosition>> out;
	out.reserve(positions.size() - 1);
	for (auto const & x : positions) if (x.ticker != L"CASH") out.push_back(x.options);
	return out;
}

inline double GetIntrinsicValue(OptionPosition const& opt, double latest)
{
	switch (opt.type)
	{
	case OptionType::CSP:
		return (latest < opt.strike) ? opt.shares * (latest - opt.strike) : 0.0;
	case OptionType::LP:
		return (latest < opt.strike) ? opt.shares * (opt.strike - latest) : 0.0;
	case OptionType::CC:
		return (latest > opt.strike) ? opt.shares * (opt.strike - latest) : 0.0;
	case OptionType::LC:
		return (latest > opt.strike) ? opt.shares * (latest - opt.strike) : 0.0;
	case OptionType::PCS:
		return (latest < opt.strike) ? opt.shares * (max(latest, opt.strike2) - opt.strike) : 0.0;
	default:
		return 0.0;
	}
}


///////////////////////////////////////////////////////////
// --- Equity history ---

std::vector<TimeSeries> CalculateFullEquityHistory(char account, std::vector<Transaction> const & trans);
void UpdateEquityHistory(std::vector<TimeSeries> & hist, std::vector<Position> const & positions, QStats const & qstats);

///////////////////////////////////////////////////////////////////////////////
// Plays


struct Play
{
	TransactionType type;
	int n;
	date_t start;
	date_t end;
	date_t expiration;
	double price_enter;
	double price_exit;
	double strike;
	double collateral;
	double assignment_cost; // Unrealized P/L from assignment. +/- for long/short.
	std::wstring ticker;

	std::wstring to_wstring() const;
};


std::vector<Play> GetOptionPerformance(std::vector<Transaction> const & trans);
