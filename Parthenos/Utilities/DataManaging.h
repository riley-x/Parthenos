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

inline TransactionType opposingType(TransactionType type)
{
	switch (type)
	{
	case TransactionType::PutShort:
		return TransactionType::PutLong;
	case TransactionType::PutLong:
		return TransactionType::PutShort;
	case TransactionType::CallShort:
		return TransactionType::CallLong;
	case TransactionType::CallLong:
		return TransactionType::CallShort;
	default:
		return TransactionType::Custom;
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
void writeTransactions_JSON(std::wstring const& filepath, std::vector<Transaction> const& trans);

std::vector<std::pair<date_t, double>> getTransfers(std::vector<Transaction> const& trans, int account = -1);

// Returns the weight for calculating APY returns from initial deposits.
double getCashWeight(int account, std::vector<Transaction> const& trans, date_t date);

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
	double price = 0;		// for ITM options, this can be set to the extrinsic price only
	double strike = 0;
	double dividends = 0;	// this is PER SHARE, i.e. should not change with n
	double fees = 0;		// value + price * n, includes fees and rounding errors, intrinsic value from opening ITM options
	double collateral = 0;	// PER SHARE/CONTRACT. long positions may have collateral != price due to spreads

	std::wstring to_json() const;
	std::wstring to_wstring() const;
};

// Holdings per ticker per account
struct AccountHoldings
{
	AccountHoldings() = default;
	AccountHoldings(jsonette::JSON const& json);

	double sumWeights = 0; // sum of (cost * days) in closed lots; divide gains by this to get weighted APY. Also total transfers for CASH
	double realized = 0; // realized gains in closed lots, also rounding errors from dividends
	std::vector<Lot> lots; 

	std::wstring to_json() const;
	std::wstring to_wstring() const;
};

// Holdings per ticker
struct Holdings
{
	Holdings() = default;
	Holdings(jsonette::JSON const& json);

	std::wstring ticker; // all uppercase
	std::vector<AccountHoldings> accts; // sized to the largest account index that has data, in-between accounts are default constructed

	std::wstring to_json() const;
	std::wstring to_wstring() const;
};

std::wostream& operator<<(std::wostream& os, const Lot& l);
std::wostream& operator<<(std::wostream& os, const AccountHoldings& h);
std::wostream& operator<<(std::wostream& os, const Holdings& h);

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
		return (latest < opt.strike) ? (latest - opt.strike) * 100 * opt.n : 0.0;
	case TransactionType::PutLong:
		return (latest < opt.strike) ? (opt.strike - latest) * 100 * opt.n : 0.0;
	case TransactionType::CallShort:
		return (latest > opt.strike) ? (opt.strike - latest) * 100 * opt.n : 0.0;
	case TransactionType::CallLong:
		return (latest > opt.strike) ? (latest - opt.strike) * 100 * opt.n : 0.0;
	default:
		return 0.0;
	}
}

// Returns the gain/loss from theta on short/long options approximating linear decay
double getThetaValue(Lot const& opt, date_t date);

inline double getCashEffect(Lot const& lot)
{
	if (isOption(lot.type))
		return lot.price * lot.n * 100 * (isShort(lot.type) ? 1 : -1) + lot.fees;
	return lot.n * (lot.dividends - lot.price) + lot.fees;
}

double getTransfers(std::vector<Holdings> const& h, char account);

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

inline bool isShort(OptionType t)
{
	switch (t)
	{
	case OptionType::CSP:
	case OptionType::CC:
	case OptionType::PCS:
		return true;
	default:
		return false;
	}
}

struct OptionPosition
{
	OptionType type = OptionType::undefined;
	unsigned shares = 0; // shares underlying
	unsigned shares_collateral = 0; // For short positions
	date_t date = 0;
	date_t expiration = 0;
	double strike = 0; // primary
	double strike2 = 0; // ancillary
	double price = 0;
	double cash_collateral = 0;

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
	int n = 0;					// Shares only		
	double avgCost = 0;			// Of shares. For CASH, cash from transfers 
	double marketPrice = 0;		// 
	double realized = 0;		// == header.realized, proceeds from closed lots ONLY, no STO proceeds. For CASH, cash from interest
	double dividends = 0;		// Sum of dividends and fees from open lots
	double unrealized = 0;		// includes intrinsic value, STO proceeds, and theta decay from options
	double cashEffect = 0;		// cash effect from open lots (dividends + fees - purchase price). For CASH, stores sum of cash effects + realized from other holdings
	double APY = 0;
	std::wstring ticker;
	std::vector<OptionPosition> options; // p/l already included in above, but useful to keep around

	inline std::wstring to_wstring() const
	{
		return L"Ticker: "		+ ticker
			+ L", n: "			+ std::to_wstring(n)
			+ L", avgCost: "	+ std::to_wstring(avgCost)
			+ L", price: "		+ std::to_wstring(marketPrice)
			+ L", realized: "	+ std::to_wstring(realized)
			+ L", dividends: "	+ std::to_wstring(dividends)
			+ L", unrealized: "	+ std::to_wstring(unrealized)
			+ L", cashEffect: " + std::to_wstring(cashEffect)
			+ L", APY: "		+ std::to_wstring(APY)
			+ L"\n";
	}
};

std::vector<Position> HoldingsToPositions(std::vector<Holdings> const & holdings,
	char account, date_t date, std::map<std::wstring, double> const& prices);

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
		if (cash && x.ticker == L"CASH") out.push_back(x.avgCost + x.realized);
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
			return { x.avgCost + x.realized + x.cashEffect, x.avgCost };
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

double getThetaValue(OptionPosition const& opt, date_t date);
double getTotalPL(std::vector<Position> const& pos);
double getNetTransfers(std::vector<Position> const& pos);
double getLiquidatingValue(std::vector<Position> const& pos);

///////////////////////////////////////////////////////////
// --- Equity history ---

std::vector<TimeSeries> CalculateFullEquityHistory(char account, std::vector<Transaction> const & trans, QStats const& qstats);
void UpdateEquityHistory(std::vector<TimeSeries> & hist, char account, std::vector<Holdings> const& holdings, QStats const& qstats);


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
