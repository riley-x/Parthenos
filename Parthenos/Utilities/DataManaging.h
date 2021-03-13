#pragma once

//#define TEST_IEX

#include "../stdafx.h"
#include "utilities.h"
#include "API.h"
#include "jsonette.h"


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

struct Transaction_OLD
{
	char account;                           // 1
	TransactionType type;           // 2
	short tax_lot;                          // 4    for sales - index into lots - 0 if FIFO (for stock) - Options must be fully qualified index
	wchar_t ticker[12] = {};        // 28   must be all uppercase
	int n;                                          // 32   positive for open, negative for close. Number of contracts for options
	date_t date;                            // 36
	date_t expiration;                      // 40   for stock dividends, this is the ex date
	double value;                           // 48   include fees here
	double price;                           // 56   don't include fees here
	double strike;                          // 64

	std::wstring to_json() const
	{
		std::wstringstream ss;
		ss << L"{\"type\": " << L"\"" << ::to_wstring(type) << L"\""
			<< L",\"account\": " << (int)account
			<< L",\"lot\": " << tax_lot
			<< L",\"n\": " << n
			<< L",\"date\": " << date
			<< L",\"expiration\": " << expiration
			<< L",\"value\": " << value
			<< L",\"price\": " << price
			<< L",\"strike\": " << strike
			<< L",\"ticker\": " << L"\"" << ticker << L"\""
			<< L"}";
		return ss.str();
	}
};

struct Transaction
{
	Transaction() = default;
	Transaction(jsonette::JSON const& json);

	TransactionType type = TransactionType::Custom;
	int account = 0;			// index into accounts.json
	int tax_lot = 0;			// for sales - index into lots - 0 if FIFO (for stock) - Options must be fully qualified index
	int n = 0;					// positive for open, negative for close. Number of contracts for options
	date_t date = 0;			// 
	date_t expiration = 0;		// for stock dividends, this is the ex date
	double value = 0;			// include fees here
	double price = 0;			// don't include fees here
	double strike = 0;			// 
	std::wstring ticker;		// must be all uppercase

	std::wstring to_wstring() const;
	std::wstring to_wstring(std::vector<std::wstring> const & accounts) const;
	std::wstring to_json() const;

	// For type==Custom, ticker is a tag that defines the custom operation,
	// which is hard-coded in the add transaction functions.
};

std::vector<Transaction> readTransactions(std::wstring const & filepath);
void writeTransactions(std::wstring const& filepath, std::vector<Transaction> const& trans);


///////////////////////////////////////////////////////////////////////////////
//                                 Holdings                                  //
///////////////////////////////////////////////////////////////////////////////


struct Lot
{
	Lot() = default;
	Lot(jsonette::JSON const& json);

	TransactionType type = TransactionType::Custom;
	int n = 0;				// shares or contracts
	date_t date = 0;
	date_t expiration = 0;
	double price = 0;
	double strike = 0;
	double dividends = 0;	// this is PER SHARE, i.e. should not change with n
	double fees = 0;		// value + price * n, includes fees and rounding errors

	std::wstring to_json() const;
};

struct AccountHoldings
{
	AccountHoldings() = default;
	AccountHoldings(jsonette::JSON const& json);

	double sumWeights = 0; // sum_transactions (cost) ; divide sumReal1Y by this to get weighted APY. Also total transfers for CASH
	double sumReal1Y = 0; // realized * 365 / days_held    ==    cost * (realized/cost) / (days_held/365)
	double sumReal = 0; // realized
	std::vector<Lot> lots; 

	std::wstring to_json() const;
};

struct Holdings
{
	Holdings() = default;
	Holdings(jsonette::JSON const& json);

	std::wstring ticker; // all uppercase
	std::vector<AccountHoldings> accts;

	std::wstring to_json() const;
};

std::vector<Holdings> readHoldings(std::wstring const& filepath);
void writeHoldings(std::wstring const& filepath, std::vector<Holdings> const& holds);


///////////////////////////////////////
// Transaction --> Holdings

void AddTransactionToHoldings(std::vector<Holdings> & holdings, Transaction const & transaction);
std::vector<Holdings> FullTransactionsToHoldings(std::vector<Transaction> const & transactions);


///////////////////////////////////////
// Misc utilities

inline std::vector<std::wstring> GetTickers(std::vector<Holdings> const & holdings, bool cash = false)
{
	std::vector<std::wstring> out;
	out.reserve(holdings.size());
	for (auto const & x : holdings)
	{
		if (!cash && x.ticker == L"CASH") continue;
		out.push_back(x.ticker);
	}
	return out;
}

inline double GetIntrinsicValue(Lot const & opt, double latest)
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

inline double getCashEffect(Lot const& lot)
{
	if (isOption(lot.type))
		return lot.price * lot.n * 100 * (isShort(lot.type) ? 1 : -1) + lot.fees;
	return lot.n * (lot.dividends - lot.price) + lot.fees;
}

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
	double avgCost;			// of shares
	double marketPrice;		// for CASH, cash from transfers
	double realized_held;	// == sum(lot.realized) for shares. For CASH, cash from interest
	double realized_unheld; // == head.sumReal + proceeds of open options. For CASH, net cash from transactions
	double unrealized;		// includes intrinsic value gain(long) / loss(short) from options, and theta decay
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
