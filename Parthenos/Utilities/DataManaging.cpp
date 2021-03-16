#include "../stdafx.h"
#include "DataManaging.h"
#include "FileIO.h"
#include "HTTP.h"

using namespace jsonette;

///////////////////////////////////////////////////////////
// --- Forward declarations ---

void AddCustomTransactionToHoldings(std::vector<Holdings>& holdings, Transaction const& t);
void AddTransactionToTickerHoldings(Holdings& h, Transaction const& t);
void MergeHoldings(Holdings& target, Holdings const& mergee);
double getAndUpdateCollateral(AccountHoldings& h, Transaction const& t);

void collateOptions(Position& p);

///////////////////////////////////////////////////////////////////////////////
//	                              Transactions			            		 //
///////////////////////////////////////////////////////////////////////////////

// For writing to file
struct Transaction_FILE
{
	Transaction_FILE() = default;
	Transaction_FILE(Transaction const & t) :
		account(t.account),
		type(t.type),
		tax_lot(t.tax_lot),
		n(t.n),
		date(t.date),
		expiration(t.expiration),
		value(t.value),
		price(t.price),
		strike(t.strike)
	{
		if (t.ticker.size() > PortfolioObjects::maxTickerLen)
			throw ws_exception(L"Ticker " + t.ticker + L" too long!");
		wcscpy_s(ticker, PortfolioObjects::maxTickerLen + 1, t.ticker.data());
	}

	operator Transaction() const
	{
		Transaction t;
		t.type = type;
		t.account = account;
		t.tax_lot = tax_lot;
		t.n = n;
		t.date = date;
		t.expiration = expiration;
		t.value = value;
		t.price = price;
		t.strike = strike;
		t.ticker = ticker;
		return t;
	}

	char account;                           // 1
	TransactionType type;					// 2
	short tax_lot;                          // 4    for sales - index into lots - 0 if FIFO (for stock) - Options must be fully qualified index
	wchar_t ticker[12] = {};				// 28   must be all uppercase
	int n;                                  // 32   positive for open, negative for close. Number of contracts for options
	date_t date;                            // 36
	date_t expiration;                      // 40   for stock dividends, this is the ex date
	double value;                           // 48   include fees here
	double price;                           // 56   don't include fees here
	double strike;                          // 64
};

std::vector<Transaction> readTransactions(std::wstring const& filepath)
{
	FileIO transFile;
	transFile.Init(filepath);
	transFile.Open(GENERIC_READ);
	std::vector<Transaction_FILE> trans = transFile.Read<Transaction_FILE>();
	transFile.Close();

	std::vector<Transaction> ts;
	ts.reserve(trans.size());
	for (Transaction_FILE const& t : trans)
		ts.emplace_back(t);

	return ts;
}

std::vector<Transaction> readTransactions_JSON(std::wstring const & filepath)
{
	FileIO transFile;
	transFile.Init(filepath);
	transFile.Open(GENERIC_READ);
	JSON j(transFile.ReadText());
	transFile.Close();

	std::vector<Transaction> trans;
	trans.reserve(j.get_arr_size());
	for (JSON const& j : j.get_arr())
		trans.push_back(j);

	return trans;
}

void writeTransactions(std::wstring const& filepath, std::vector<Transaction> const& trans)
{
	std::vector<Transaction_FILE> tf;
	tf.reserve(trans.size());
	for (Transaction const& t : trans)
		tf.emplace_back(t);

	FileIO transFile;
	transFile.Init(filepath);
	transFile.Open();
	transFile.Write(tf.data(), sizeof(Transaction_FILE) * tf.size());
	transFile.Close();
}

void writeTransactions_JSON(std::wstring const & filepath, std::vector<Transaction> const& trans)
{
	std::string json("[");
	for (Transaction const & t : trans)
	{
		json.append(::w2s(t.to_json()));
		json.append(",");
	}
	json.append("]");

	std::string towrite(JSON(json).to_string());

	FileIO transFile;
	transFile.Init(filepath);
	transFile.Open();
	transFile.Write(towrite.data(), towrite.size() * sizeof(char));
	transFile.Close();
}

///////////////////////////////////////////////////////////////////////////////
//	                      Transcations --> Holdings		            		 //
///////////////////////////////////////////////////////////////////////////////


// Returned vector is sorted by ticker
std::vector<Holdings> FullTransactionsToHoldings(std::vector<Transaction> const& transactions)
{
	std::vector<Holdings> holdings; 

	for (Transaction const& t : transactions)
	{
		AddTransactionToHoldings(holdings, t);
	}

	return holdings;
}

// Assumes holdings is sorted by ticker
void AddTransactionToHoldings(std::vector<Holdings>& holdings, Transaction const& t)
{
	if (t.type == TransactionType::Custom) 
		return AddCustomTransactionToHoldings(holdings, t);

	// See if ticker is present already
	auto it = std::lower_bound(holdings.begin(), holdings.end(), t.ticker,
		[](Holdings const& h1, std::wstring const & t) { return h1.ticker < t; }
	);

	if (it == holdings.end() || std::wstring(it->ticker) != t.ticker)
	{
		// insert (rare, so using vectors is better than a list)
		Holdings h;
		h.ticker = t.ticker;
		it = holdings.insert(it, h);
	}

	AddTransactionToTickerHoldings(*it, t);
}

// Adds t to h, assuming t is an open position in this stock
void addLot(AccountHoldings& h, Transaction const& t)
{
	Lot lot;
	lot.type = t.type;
	lot.date = t.date;
	lot.n = t.n;
	lot.price = t.price;
	lot.strike = t.strike;
	lot.expiration = t.expiration;
	lot.fees = t.value + t.n * t.price * (isShort(t.type) ? -1 : 1) * (isOption(t.type) ? 100 : 1); 
	lot.collateral = getAndUpdateCollateral(h, t);

	h.lots.push_back(lot);
}

// Search through lots to assign realized to lot
void addDividend(AccountHoldings& h, Transaction const& t)
{
	int nshares = static_cast<int>(round(t.value / t.price));
	double leftover = t.value;

	for (Lot& lot : h.lots)
	{
		if (lot.date >= t.expiration) break; // >= ex-div date
		if (lot.type != TransactionType::Stock) continue;

		lot.dividends += t.price;
		nshares -= lot.n;
		leftover -= lot.n * t.price;
	}

	if (nshares < 0)
		throw ws_exception(L"AddTransactionToTickerHoldings() dividend negative nshares: " + std::to_wstring(nshares));

	h.realized += leftover; // give extra to header (includes partial cents)
}

// Assumes t is a close position in this account
// TODO t.tax_lot, this is FIFO
void closePosition(AccountHoldings& h, Transaction const& trans)
{
	Transaction t(trans); // copy to subtract from incrementally 
	t.n = -t.n; // n is negative in close transactions
	for (size_t i = 0; i < h.lots.size(); i++)
	{
		Lot& lot = h.lots[i];
		if (lot.type != t.type || lot.expiration != t.expiration || lot.strike != t.strike) continue;

		int n = min(lot.n, t.n);
		lot.n -= n; // lot.n = 0 is mark for removal
		t.n -= n;

		double pl = n * (t.price - lot.price) * (isOption(t.type) ? 100 : 1) * (isShort(t.type) ? -1 : 1);
		t.value -= n * t.price * (isOption(t.type) ? 100 : 1) * (isShort(t.type) ? -1 : 1);
		h.realized += pl + n * lot.dividends;

		int days_held = max(1, ::DateDiff(t.date, lot.date));
		h.sumWeights += lot.collateral * n * days_held;

		if (t.n == 0) break;
	}
	h.realized += t.value; // leftovers: rounding errors and fees

	// Delete closed lots
	for (size_t i = h.lots.size(); i-- != 0; )
	{
		if (h.lots[i].n == 0)
		{
			h.realized += h.lots[i].fees;
			h.lots.erase(h.lots.begin() + i);
		}
	}
}


// Updates the holdings for a single ticker with the transaction.
void AddTransactionToTickerHoldings(Holdings& h, Transaction const& t)
{
	if (t.ticker != h.ticker)
		throw ws_exception(L"AddTransactionToTickerHoldings wrong tickers:" + t.ticker + L" " + h.ticker);
	if (static_cast<size_t>(t.account) >= h.accts.size())
		h.accts.resize(1ll + t.account);

	// Don't add lots for cash
	if (t.ticker == L"CASH")
	{
		if (t.type == TransactionType::Transfer)
			h.accts[t.account].sumWeights += t.value;
		else if (t.type == TransactionType::Interest)
			h.accts[t.account].realized += t.value;
		else
			throw ws_exception(L"Unrecognized transaction:" + t.to_wstring());
		return;
	}

	// Case on open/close position
	if (t.n > 0) addLot(h.accts[t.account], t);
	else if (t.n < 0) closePosition(h.accts[t.account], t);
	else // misc transactions
	{
		switch (t.type)
		{
		case TransactionType::Dividend:
			addDividend(h.accts[t.account], t);
			break;
		case TransactionType::Fee: // Incorporated into t.value on a sale already
			break;
		default:
			throw ws_exception(L"Unrecognized transaction:" + t.to_wstring());
			break;
		}
	}
}


// Handles custom transactions, such as spin-offs, mergers, and stock splits
// TODO add transaction type for above
void AddCustomTransactionToHoldings(std::vector<Holdings>& holdings, Transaction const& t)
{
	if (std::wstring(t.ticker) == L"RTN-UTX") // RTN-UTX merger
	{
		// Find position of RTN and RTX
		auto rtn = holdings.end();
		auto rtx = holdings.end();
		for (auto it = holdings.begin(); it != holdings.end(); it++)
		{
			if (it->ticker == L"RTN") rtn = it;
			else if (it->ticker == L"RTX") rtx = it;
		}
		if (rtn == holdings.end() || rtx == holdings.end()) return;
		MergeHoldings(*rtx, *rtn);
		holdings.erase(rtn);
	}
	else
	{
		throw ws_exception(L"Unkown custom transaction: " + std::wstring(t.ticker));
	}
}


// In the case of a stock merger, add all return information from 'mergee' into 'target'.
// The merger is indicated in the transactions via a sale of mergee stock and purchase of target stock.
void MergeHoldings(Holdings& target, Holdings const & mergee)
{
	if (target.accts.size() < mergee.accts.size())
		target.accts.resize(mergee.accts.size());

	for (size_t acc = 0; acc < mergee.accts.size(); acc++)
	{
		target.accts[acc].realized += mergee.accts[acc].realized;
		target.accts[acc].sumWeights += mergee.accts[acc].sumWeights;
	}
}


// This function assumes t is being added to h (but not yet) and
// returns the collateral of the position. If part of a spread, the collateral 
// for the long position is also updated to the true collateral.
//
// Note matched short positions have collateral 0 to not double count with the
// long position.
//
// TODO calendar spreads
double getAndUpdateCollateral(AccountHoldings & h, Transaction const & t)
{
	if (!isOption(t.type)) return t.price;
	if (!isShort(t.type)) return t.price * 100;

	// First check if part of spread: long position must preceed short (i.e. last lot)
	if (!h.lots.empty())
	{
		Lot& lot = h.lots.back();
		if (lot.type == opposingType(t.type) && lot.expiration == t.expiration
			&& lot.date == t.date && lot.n == t.n)
		{
			double collat = std::abs(t.strike - lot.strike) * 100;
			lot.collateral = collat;
			return 0; // collateral is 0, since cost is included in collateral of long
		}
	}

	// CSP
	if (t.type == TransactionType::PutShort) return t.strike * 100;
	// CC -- collateral is 0, since cost is included in collateral of shares
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//	                                Holdings                                 //
///////////////////////////////////////////////////////////////////////////////


std::vector<Holdings> readHoldings(std::wstring const& filepath)
{
	FileIO file;
	file.Init(filepath);
	file.Open(GENERIC_READ);
	JSON j(file.ReadText());
	file.Close();

	std::vector<Holdings> holds;
	holds.reserve(j.get_arr_size());
	for (JSON const& j : j.get_arr())
		holds.push_back(j);

	return holds;
}

void writeHoldings(std::wstring const& filepath, std::vector<Holdings> const& holds)
{
	std::wstringstream ss;
	ss << L"[";
	for (size_t i = 0; i < holds.size(); i++)
	{
		if (i > 0) ss << L",";
		ss << holds[i];
	}
	ss << L"]";

	std::string towrite(JSON(::w2s(ss.str())).to_string());

	FileIO file;
	file.Init(filepath);
	file.Open();
	file.Write(towrite.data(), towrite.size() * sizeof(char));
	file.Close();
}

double getThetaValue(Lot const& opt, date_t date)
{
	if (!isOption(opt.type)) return 0;

	int days_tot = ::DateDiff(opt.expiration, opt.date);
	int days_held = ::DateDiff(date, opt.date);

	return opt.price * opt.n * 100 * days_held / days_tot * (isShort(opt.type) ? 1 : -1);
}

double getTransfers(std::vector<Holdings> const& hold, char account)
{
	for (Holdings const& h : hold)
		if (h.ticker == L"CASH")
			return h.accts[account].sumWeights;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//							Holdings -> Positions                            //
///////////////////////////////////////////////////////////////////////////////


// This helper function returns values without averaging out
//		* avgCost contains the total cost
//		* APY contains sumWeights
// Some other members are not filled
//		* ticker
//		* price
Position holdingsToRawPosition(AccountHoldings const& header, date_t date, double price, bool cash)
{
	// Variables to fill
	Position p;

	// Get realized from closed lots
	p.realized += header.realized;
	if (cash) p.avgCost += header.sumWeights;
	else p.APY += header.sumWeights;

	// Add up all lots
	for (size_t iLot = 0; iLot < header.lots.size(); iLot++)
	{
		Lot const& lot = header.lots[iLot];
		p.cashEffect += getCashEffect(lot);
		p.APY += lot.n * lot.collateral * max(1, ::DateDiff(date, lot.date));
		if (lot.type == TransactionType::Stock)
		{
			p.n += lot.n;
			p.avgCost += lot.n * lot.price; // divide by p.n at end
			p.dividends += lot.dividends * lot.n + lot.fees;
			p.unrealized += (price - lot.price) * lot.n;
		}
		else
		{
			OptionPosition op_pos; // Simply copy info for now, then collate at end
			op_pos.date = lot.date;
			op_pos.type = transToOptionType(lot.type);
			op_pos.shares = lot.n * 100;
			op_pos.expiration = lot.expiration;
			op_pos.strike = lot.strike;
			op_pos.price = lot.price;

			// TODO add unrealized for theta effect, STO, include options in APY?
			p.unrealized += GetIntrinsicValue(lot, price) + getThetaValue(lot, date);
			p.options.push_back(op_pos);
		}
	}

	return p;
}



// Calculates returns and APY using 'date' as end date. Set 'account' = -1 to use all accounts.
// 'prices' should contain the latest market price in the same order as the tickers in holdings, except CASH.
// Note this function does not truncate transactions to 'date'!
std::vector<Position> HoldingsToPositions(std::vector<Holdings> const & holdings,
	char account, date_t date, std::map<std::wstring, double> const & prices)
{
	std::vector<Position> positions;
	double net_transactions = 0;

	for (Holdings h : holdings)
	{
		// Check this account has info on this ticker
		if (account != -1)
		{
			if (static_cast<size_t>(account) >= h.accts.size()) continue;
			if (h.accts[account].lots.empty() && h.accts[account].realized == 0) continue;
		}

		// Object to fill
		Position p;
		p.ticker = h.ticker;
		if (p.ticker != L"CASH") p.marketPrice = prices.at(p.ticker);

		// Loop over accounts and add positions together
		unsigned start = (account == -1) ? 0 : account;
		unsigned end = (account == -1) ? static_cast<unsigned>(h.accts.size()) : start + 1;
		for (unsigned iAccount = start; iAccount < end; iAccount++)
		{
			AccountHoldings const& header = h.accts[iAccount];
			if (header.lots.empty() && header.realized == 0) continue; // empty

			Position p_acc = holdingsToRawPosition(header, date, p.marketPrice, p.ticker == L"CASH");
			p.n += p_acc.n;
			p.avgCost += p_acc.avgCost;
			p.realized += p_acc.realized;
			p.dividends += p_acc.dividends;
			p.unrealized += p_acc.unrealized;
			p.cashEffect += p_acc.cashEffect;
			p.APY += p_acc.APY;
			p.options.insert(p.options.end(), p_acc.options.begin(), p_acc.options.end());
		} 

		// Correct the "raw" position
		if (p.n > 0) p.avgCost = p.avgCost / p.n;
		if (p.APY > 0) p.APY = (p.realized + p.dividends + p.unrealized) / p.APY * 365;
		if (p.ticker != L"CASH") net_transactions += p.cashEffect + p.realized;
		collateOptions(p);

		// Add ticker
		positions.push_back(p);
	} 

	// update cash with transaction total
	for (auto& x : positions) 
		if (x.ticker == L"CASH") x.cashEffect = net_transactions;

	return positions;
}



// HoldingsToPositions above simply adds single options. This function searches through 
// the option chain to combine the naked options into spreads, as well as updating 
// the position's collaterals. The long position should be before the short position.
// TODO calendar spreads
void collateOptions(Position& p)
{
	std::vector<OptionPosition> ops; // Create a new list

	// Add options from the end (should be time-ordered). When on a short option,
	// invalidate corresponding long options by decrementing their shares.
	for (auto shor = p.options.rbegin(); shor != p.options.rend(); shor++)
	{
		if (shor->type == OptionType::CSP)
		{
			for (auto lon = shor + 1; lon != p.options.rend(); lon++)
			{
				if (lon->type != OptionType::LP || lon->date != shor->date 
					|| lon->expiration != shor->expiration || lon->shares == 0) continue;

				if (lon->strike > shor->strike) // Put debit spread
				{
					// TODO
				}
				else // Put credit spread
				{
					OptionPosition pos = {};
					pos.type = OptionType::PCS;
					pos.date = shor->date;
					pos.shares = min(lon->shares, shor->shares);
					pos.expiration = shor->expiration;
					pos.strike = shor->strike;
					pos.strike2 = lon->strike;
					pos.price = shor->price - lon->price;
					pos.cash_collateral = pos.shares * (pos.strike - pos.strike2);
					ops.push_back(pos);

					shor->shares -= pos.shares;
					lon->shares -= pos.shares;
				}
				if (shor->shares == 0) break;
			}

			if (shor->shares > 0) // Cash secured put
			{
				shor->cash_collateral = shor->shares * shor->strike;
				ops.push_back(*shor);
			}
		}
		else if (shor->type == OptionType::CC)
		{
			for (auto lon = shor + 1; lon != p.options.rend(); lon++)
			{
				if (lon->type != OptionType::LC || lon->expiration != shor->expiration || lon->shares == 0) continue;

				if (lon->strike < shor->strike) // Call debit spread
				{
					// TODO
				}
				else // Call credit spread
				{
					// TODO
				}
				if (shor->shares == 0) break;
			}

			if (shor->shares > 0) // Covered call
			{
				shor->shares_collateral = shor->shares;
				ops.push_back(*shor);
			}
		}
		else if (shor->shares > 0) // Long options that haven't yet been matched
		{
			ops.push_back(*shor);
		}
	} // End reverse loop over options 

	p.options = ops;
}



double getThetaValue(OptionPosition const& opt, date_t date)
{
	int days_tot = ::DateDiff(opt.expiration, opt.date);
	int days_held = ::DateDiff(date, opt.date);

	return opt.price * opt.shares * days_held / days_tot * (isShort(opt.type) ? 1 : -1);
}


double getTotalPL(std::vector<Position> const& pos)
{
	double equity = 0;
	for (Position const& p : pos)
	{
		if (p.ticker == L"CASH") equity += p.realized;
		else equity += p.realized + p.dividends + p.unrealized;
	}
	return equity;
}

double getNetTransfers(std::vector<Position> const& pos)
{
	for (Position const& p : pos)
	{
		if (p.ticker == L"CASH") return p.avgCost;
	}
	return 0;
}

double getLiquidatingValue(std::vector<Position> const& pos)
{
	double equity = 0;
	for (Position const& p : pos)
	{
		if (p.ticker == L"CASH") equity += p.avgCost + p.realized;
		else equity += p.realized + p.dividends + p.unrealized;
	}
	return equity;
}



///////////////////////////////////////////////////////////////////////////////
//					               Equity                                    //
///////////////////////////////////////////////////////////////////////////////


struct TickerData
{
	TickerData(std::wstring const& ticker, date_t init_date, date_t last_date, date_t ex_div) 
	{
		if (ex_div >= 0 && init_date > ex_div) // slightly over strict
			ohlc = GetOHLC(ticker, apiSource::iex, 0, last_date);
		else
			ohlc = GetOHLC(ticker, apiSource::alpha, 0, last_date);
		i = FindDateOHLC(ohlc, init_date);
	}

	std::vector<OHLC> ohlc;
	size_t i;

	double getPrice(date_t date) const
	{
		if (i >= ohlc.size()) return std::nan("");
		if (ohlc[i].date != date) return std::nan("");
		return ohlc[i].close;
	}
};


double getTotalPL(std::vector<Holdings> const & h, char account, date_t date, date_t last_date,
	std::map<std::wstring, TickerData>& tickerData, QStats const & qstats)
{
	std::map<std::wstring, double> prices;
	for (std::wstring ticker : GetTickers(h))
	{
		auto it = tickerData.find(ticker);
		if (it == tickerData.end())
		{
			auto data = FindQStat(qstats, ticker);
			auto res = tickerData.try_emplace(ticker, ticker, date, last_date, data == qstats.end() ? -1 : data->second.exDividendDate);
			it = res.first;
		}
		double price = it->second.getPrice(date);
		if (std::isnan(price)) return price;
		prices[ticker] = price;
	}

	std::vector<Position> pos(HoldingsToPositions(h, account, date, prices));
	return getTotalPL(pos);
}

std::vector<TimeSeries> CalculateFullEquityHistory(char account, std::vector<Transaction> const & trans, QStats const & qstats)
{
	std::vector<TimeSeries> out;
	if (trans.empty()) return out;

	// Get start date / transaction
	date_t start_date = 0;
	size_t iTrans = 0;
	for (iTrans; iTrans < trans.size(); iTrans++)
	{
		if (trans[iTrans].account == account)
		{
			start_date = trans[iTrans].date;
			break;
		}
	}
	if (start_date == 0) return out;

	// End date from voo in qstats
	auto voo_stats = FindQStat(qstats, L"VOO");
	if (voo_stats == qstats.end()) throw ws_exception(L"getTotalPL() Didn't find qstats for voo");
	date_t last_date = GetDate(voo_stats->first.closeTime);

	// Get list of dates from voo
	TickerData voo(L"VOO", start_date, last_date, -1);
	std::vector<date_t> dates;
	dates.reserve(voo.ohlc.size() - voo.i);
	for (size_t i = voo.i; i < voo.ohlc.size(); i++)
		dates.push_back(voo.ohlc[i].date);

	std::map<std::wstring, TickerData> tickerData;
	tickerData.insert({ L"VOO", voo });

	std::vector<Holdings> h;
	for (date_t date : dates)
	{
		// Loop over transactions of the current date
		while (iTrans < trans.size() && trans[iTrans].date <= date)
		{
			Transaction const & t = trans[iTrans];
			if (t.account == account || trans[iTrans].type == TransactionType::Custom)
			{
				if (t.date < date) throw ws_exception(
					L"CalculateFullEquityHistory transaction date expected:" + DateToWString(date) + L"\n" + t.to_wstring()
				);
				AddTransactionToHoldings(h, t);
			}
			iTrans++;
		}

		double equity = getTotalPL(h, account, date, last_date, tickerData, qstats);
		if (std::isnan(equity))
		{
			OutputDebugString((L"Breaking on date " + DateToWString(date)).c_str());
			break;
		}
		out.push_back({ date, equity });

		for (auto & x : tickerData) x.second.i++;
	}

	return out;
}


// Assumes holdings has not changed since last history entry!!!!
void UpdateEquityHistory(std::vector<TimeSeries>& hist, char account, std::vector<Holdings> const & holdings, QStats const& qstats)
{
	// Get start/end date
	date_t start_date = hist.back().date;
	auto voo_stats = FindQStat(qstats, L"VOO");
	if (voo_stats == qstats.end()) throw ws_exception(L"getTotalPL() Didn't find qstats for voo");
	date_t last_date = GetDate(voo_stats->first.closeTime);

	// Get list of dates from voo
	TickerData voo(L"VOO", start_date, last_date, -1);
	voo.i++; // move to next new date
	std::vector<date_t> dates;
	dates.reserve(voo.ohlc.size() - voo.i);
	for (size_t i = voo.i; i < voo.ohlc.size(); i++)
		dates.push_back(voo.ohlc[i].date);

	// Store ticker ohlc data
	std::map<std::wstring, TickerData> tickerData;
	tickerData.insert({ L"VOO", voo });

	for (date_t date : dates)
	{
		double equity = getTotalPL(holdings, account, date, last_date, tickerData, qstats);
		if (std::isnan(equity))
		{
			OutputDebugString((L"Breaking on date " + DateToWString(date)).c_str());
			break;
		}
		hist.push_back({ date, equity });

		for (auto& x : tickerData) x.second.i++;
	}

}


///////////////////////////////////////////////////////////////////////////////
//	                          Plays (Deprecate)                              //
///////////////////////////////////////////////////////////////////////////////


// Assumes no partial closes
std::vector<Play> GetOptionPerformance(std::vector<Transaction> const & trans)
{
	std::vector<Play> plays;
	//for (size_t i = 0; i < trans.size(); i++)
	//{
	//	Transaction const& t = trans[i];
	//	if (!isOption(t.type)) continue;

	//	if (t.n > 0) // Open
	//	{
	//		Play p = {};
	//		p.type = t.type;
	//		p.n = t.n;
	//		p.start = t.date;
	//		p.expiration = t.expiration;
	//		p.price_enter = t.price;
	//		p.strike = t.strike;
	//		p.collateral = GetCollateral(trans, i);
	//		p.ticker = t.ticker;

	//		plays.push_back(p);
	//	}
	//	else // Close
	//	{
	//		for (Play & p : plays)
	//		{
	//			if (p.end == 0 && p.ticker == t.ticker && p.type == t.type 
	//				&& p.expiration == t.expiration && p.strike == t.strike)
	//			{
	//				p.end = t.date;
	//				p.price_exit = t.price;
	//				p.assignment_cost = GetAssignment(p, trans, i);
	//				break;
	//			}
	//		}
	//	}
	//}

	return plays;
}

// Returns the collateral of an open option transaction (index iOpen).
// For long positions, the collateral is the purchase price.
// For short puts, the collateral is the strike.
// For short calls, a mini transactions->holdings is done to find the cost basis of currently held shares.
double GetCollateral(std::vector<Transaction> const& trans, size_t iOpen)
{
	return 0;
	//if (!isShort(trans[iOpen].type))
	//	return 100 * trans[iOpen].n * trans[iOpen].price;
	//if (trans[iOpen].type == TransactionType::PutShort)
	//	return 100 * trans[iOpen].n * trans[iOpen].strike;

	//// Short call
	//std::wstring ticker(trans[iOpen].ticker);
	//std::vector<std::vector<Holdings>> holdings;
	//for (size_t i = 0; i < iOpen; i++)
	//{
	//	if (
	//		trans[i].ticker == ticker 
	//		&& trans[i].account == trans[iOpen].account 
	//		&& trans[i].type == TransactionType::Stock
	//		)
	//		AddTransactionToHoldings(holdings, trans[i]);
	//}
	//ReduceSalesLots(holdings[0], 1, MkDate(9999, 99, 99));

	//double costBasis = 0; // add up cost of shares, divide at end
	//int shares = 100 * trans[iOpen].n; // decrement as we iterate through lots
	//for (auto it = holdings[0].rbegin(); it != holdings[0].rend(); it++)
	//{
	//	if (it->lot.n >= shares) // all accounted for
	//	{
	//		costBasis += it->lot.price * shares;
	//		break;
	//	}
	//	else
	//	{
	//		costBasis += it->lot.price * it->lot.n;
	//		shares -= it->lot.n;
	//	}
	//}
	//return costBasis / (100. * trans[iOpen].n);
}


// Returns the unrealized profit/loss from an assigned option close, if applicable.
double GetAssignment(Play& p, std::vector<Transaction> const& trans, size_t iClose)
{
	// Price is zero in case of assignment OR expiration. If assignment, the next
	// transaction should be the stock resolution.
	if (trans[iClose].price > 0) return 0;
	if (iClose + 1 >= trans.size()) return 0;
	Transaction const& t_next = trans[iClose + 1];
	if (t_next.ticker != p.ticker || abs(t_next.n) != p.n * 100 || t_next.price != p.strike) return 0;

	// Get OHLC for date of assignment
	int days_to_get;
	date_t lastCloseDate = -1;
	std::vector<OHLC> ohlc(GetSavedOHLC(p.ticker, apiSource::alpha, 0, days_to_get, lastCloseDate));
	size_t iOhlc = FindDateOHLC(ohlc, t_next.date);
	if (iOhlc == ohlc.size()) return 0;

	switch (p.type)
	{
	case TransactionType::CallLong:
	case TransactionType::PutShort:
		return (ohlc[iOhlc].close - p.strike) * p.n * 100;
	case TransactionType::CallShort:
	case TransactionType::PutLong:
		return (p.strike - ohlc[iOhlc].close) * p.n * 100;
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//	                            Print Functions                              //
///////////////////////////////////////////////////////////////////////////////

Transaction::Transaction(jsonette::JSON const& json)
{
	type = TransactionStringToEnum(json["type"]);
	account = json["account"];
	tax_lot = json["lot"];
	n = json["n"];
	date = json["date"];
	expiration = json["expiration"];
	value = json["value"];
	price = json["price"];
	strike = json["strike"];
	ticker = json["ticker"];
}

std::wstring Transaction::to_wstring() const
{
	wchar_t buffer[300];
	swprintf_s(buffer, _countof(buffer),
		L"%s: %s (Account: %d) %s, n: %d, Value: %.2lf, Price: %.4lf, Expiration: %s, Strike: %.2lf",
		DateToWString(date).c_str(), ticker.c_str(), static_cast<int>(account), ::to_wstring(type).c_str(),
		n, value, price, DateToWString(expiration).c_str(), strike
	);
	return std::wstring(buffer);
}

std::wstring Transaction::to_wstring(std::vector<std::wstring> const& accounts) const
{
	wchar_t buffer[300];
	swprintf_s(buffer, _countof(buffer),
		L"%s: %s (%s) %s, n: %d, Value: %.2lf, Price: %.4lf, Expiration: %s, Strike: %.2lf",
		DateToWString(date).c_str(), ticker.c_str(), accounts[account].c_str(), ::to_wstring(type).c_str(),
		n, value, price, DateToWString(expiration).c_str(), strike
	);
	return std::wstring(buffer);
}

std::wstring Transaction::to_json() const
{
	std::wstringstream ss;
	ss << L"{\"type\": " << L"\"" << ::to_wstring(type) << L"\""
		<< L",\"account\": " << account
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

std::wstring Play::to_wstring() const
{
	return L"Ticker: " + ticker
		+ L", n: " + std::to_wstring(n)
		+ L", type: " + std::to_wstring(static_cast<char>(type))
		+ L", start: " + DateToWString(start)
		+ L", end : " + DateToWString(end)
		+ L", expiration : " + DateToWString(expiration)
		+ L", price_enter: " + std::to_wstring(price_enter)
		+ L", price_exit: " + std::to_wstring(price_exit)
		+ L", strike: " + std::to_wstring(strike)
		+ L", collateral: " + std::to_wstring(collateral)
		+ L", assignment_cost: " + std::to_wstring(assignment_cost)
		+ L"\n";
}

Lot::Lot(jsonette::JSON const& json)
{
	type = TransactionStringToEnum(json["type"]);
	n = json["n"];
	date = json["date"];
	expiration = json["expiration"];
	price = json["price"];
	strike = json["strike"];
	dividends = json["dividends"];
	fees = json["fees"];
	collateral = json["collateral"];
}

std::wstring Lot::to_json() const
{
	std::wstringstream ss;
	ss << *this;
	return ss.str();
}

std::wostream& operator<<(std::wostream& os, const Lot& l)
{
	os << L"{\"type\":" << L"\"" << ::to_wstring(l.type) << L"\""
		<< L",\"n\":" << l.n
		<< L",\"date\":" << l.date
		<< L",\"expiration\":" << l.expiration
		<< L",\"price\":" << l.price
		<< L",\"strike\":" << l.strike
		<< L",\"dividends\":" << l.dividends
		<< L",\"fees\":" << l.fees
		<< L",\"collateral\":" << l.collateral
		<< L"}";
	return os;
}

std::wstring Lot::to_wstring() const
{
	std::wstring out = ::to_wstring(type);
	if (isOption(type))
		out += FormatMsg(L"-%.2f-%u", strike, expiration);
	out += L" (" + DateToWString(date) + L")"
		+ L": " + std::to_wstring(n)
		+ L" @ $" + std::to_wstring(price)
		+ L", div: " + std::to_wstring(dividends)
		+ L", fee: " + std::to_wstring(fees)
		+ L", col: " + std::to_wstring(collateral);

	return out;
}

AccountHoldings::AccountHoldings(jsonette::JSON const& json)
{
	sumWeights = json["sumWeights"];
	realized = json["realized"];
	lots.reserve(json["lots"].get_arr_size());
	for (JSON const& j : json["lots"].get_arr())
		lots.push_back(j);
}

std::wostream& operator<<(std::wostream& os, const AccountHoldings& h)
{
	os << L"{\"sumWeights\":" << h.sumWeights
		<< L",\"realized\":" << h.realized
		<< L",\"lots\":[";
	for (size_t i = 0; i < h.lots.size(); i++)
	{
		if (i != 0) os << ",";
		os << h.lots[i];
	}
	os << L"]}";
	return os;
}

std::wstring AccountHoldings::to_json() const
{
	std::wstringstream ss;
	ss << *this;
	return ss.str();
}

std::wstring AccountHoldings::to_wstring() const
{
	std::wstring out = L"\trealized: " + std::to_wstring(realized)
		+ L", sumWeights: " + std::to_wstring(sumWeights)
		+ L"\n";
	for (Lot const& lot : lots)
	{
		out.append(L"\t\t");
		out.append(lot.to_wstring());
		out.append(L"\n");
	}
	return out;
}

Holdings::Holdings(jsonette::JSON const& json)
{
	ticker = ::s2w(json["ticker"]);
	accts.reserve(json["accts"].get_arr_size());
	for (JSON const& j : json["accts"].get_arr())
		accts.push_back(j);
}

std::wostream& operator<<(std::wostream& os, const Holdings& h)
{
	os << L"{\"ticker\":" << L"\"" << h.ticker << L"\""
		<< L",\"accts\":[";
	for (size_t i = 0; i < h.accts.size(); i++)
	{
		if (i != 0) os << ",";
		os << h.accts[i];
	}
	os << L"]}";
	return os;
}


std::wstring Holdings::to_json() const
{
	std::wstringstream ss;
	ss << *this;
	return ss.str();
}

std::wstring Holdings::to_wstring() const
{
	std::wstring out;
	out.append(L"-------------------------------------------\n");
	out.append(ticker + L", Accounts: " + std::to_wstring(accts.size()) + L"\n");

	for (AccountHoldings const & acc : accts)
	{
		out.append(acc.to_wstring());
	}
	return out;
}