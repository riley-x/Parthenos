#include "../stdafx.h"
#include "DataManaging.h"
#include "FileIO.h"
#include "HTTP.h"

using namespace jsonette;

///////////////////////////////////////////////////////////
// --- Forward declarations ---

double GetCollateral(std::vector<Transaction> const& trans, size_t iOpen);
double GetAssignment(Play& p, std::vector<Transaction> const& trans, size_t iClose);

void AddCustomTransactionToHoldings(std::vector<Holdings>& holdings, Transaction const& t);
void AddTransactionToTickerHoldings(Holdings& h, Transaction const& t);
void ReduceSalesLots(std::vector<Holdings> & h, size_t i_header, date_t end);
size_t GetPurchaseLot(std::vector<Holdings> const & h, size_t i_header, size_t n);
inline bool Holdings_Compare(const std::vector<Holdings>& a, std::wstring const & ticker);

inline double GetAPY(double gain, double cost_in, date_t start_date, date_t end_date);
inline double GetWeightedAPY(double gain, date_t start_date, date_t end_date);

void collateOptions(Position& p);


///////////////////////////////////////////////////////////////////////////////
//	                              Transactions			            		 //
///////////////////////////////////////////////////////////////////////////////

std::vector<Transaction> readTransactions(std::wstring const & filepath)
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

void writeTransactions(std::wstring const & filepath, std::vector<Transaction> const& trans)
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
		[](Holdings const& h1, Holdings const& h2) { return h1.ticker < h2.ticker; }
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
void addLot(Holdings& h, Transaction const& t)
{
	if (static_cast<size_t>(t.account) >= h.accts.size())
		h.accts.resize(t.account);

	Lot lot;
	lot.type = t.type;
	lot.date = t.date;
	lot.n = t.n;
	lot.price = t.price;
	lot.strike = t.strike;
	lot.expiration = t.expiration;
	lot.fees = t.value + t.n * t.price * (isShort(t.type) ? -1 : 1); 

	h.accts[t.account].lots.push_back(lot);
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

	h.sumReal += leftover; // give extra to header (includes partial cents)
}

// Assumes t is a close position in this account
// TODO t.tax_lot, this is FIFO
void closePosition(AccountHoldings& h, Transaction const& t)
{
	int n_close = -t.n; // n is negative in close transactions
	for (size_t i = 0; i < h.lots.size(); i++)
	{
		Lot& lot = h.lots[i];
		if (lot.type != t.type || lot.expiration != t.expiration || lot.strike != t.strike) continue;

		int n = min(lot.n, n_close);
		lot.n -= n; // lot.n = 0 is mark for removal
		n_close -= n;

		double realized = t.value - n * lot.price * (isOption(t.type) ? 100 : 1) * (isShort(t.type) ? -1 : 1);
		realized += n * lot.dividends;

		time_t time_held = min(86400, DateToTime(t.date) - DateToTime(lot.date));
		h.sumWeights += lot.price * n * (isOption(t.type) ? 100 : 1);
		h.sumReal += realized;
		h.sumReal1Y += realized * 365.0 * 86400.0 / time_held;

		if (n_close == 0) break;
	}

	// Delete closed lots
	for (size_t i = h.lots.size(); i-- != 0; )
	{
		if (h.lots[i].n == 0)
		{
			h.sumReal += h.lots[i].fees;
			h.lots.erase(h.lots.begin() + i);
		}
	}
}


// Updates the holdings for a single ticker with the transaction.
void AddTransactionToTickerHoldings(Holdings& h, Transaction const& t)
{
	if (t.ticker != h.ticker)
		throw ws_exception(L"AddTransactionToTickerHoldings wrong tickers:" + t.ticker + L" " + h.ticker);
	if (t.n <= 0 && static_cast<size_t>(t.account) >= h.accts.size())
		throw ws_exception(L"AddTransactionToTickerHoldings Bad account:" + t.to_wstring());

	// Don't add lots for cash
	if (t.ticker == L"CASH")
	{
		if (t.type == TransactionType::Transfer)
			h.accts[t.account].sumWeights += t.value;
		else if (t.type == TransactionType::Interest)
			h.accts[t.account].sumReal += t.value;
		else
			throw ws_exception(L"Unrecognized transaction:" + t.to_wstring());
		return;
	}

	// Case on open/close position
	if (t.n > 0) addLot(h, t);
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
void AddCustomTransactionToHoldings(std::vector<Holdings>& holdings, Transaction const& t);
{
	if (std::wstring(t.ticker) == L"RTN-UTX") // RTN-UTX merger
	{
		// Find position of RTN and RTX
		NestedHoldings::iterator rtn = holdings.end();
		NestedHoldings::iterator rtx = holdings.end();
		for (auto it = holdings.begin(); it != holdings.end(); it++)
		{
			std::wstring ticker(it->front().tickerInfo.ticker);
			if (ticker == L"RTN") rtn = it;
			else if (ticker == L"RTX") rtx = it;
		}

		MergeHoldings(*rtx, *rtn);
		holdings.erase(rtn);
	}
	else
	{
		throw ws_exception(L"Unkown custom transaction: " + std::wstring(t.ticker));
	}
}


///////////////////////////////////////////////////////////////////////////////
//	                                Holdings                                 //
///////////////////////////////////////////////////////////////////////////////



// Reduces sales against purchases for stock lots with date prior to end.
void ReduceSalesLots(std::vector<Holdings>& h, size_t i_header, date_t end)
{
	HoldingHeader *header = &(h.at(i_header).head);
	size_t istart = i_header + 1;
	size_t iend = i_header + 1; // exclusive

	while (iend <= i_header + header->nLots)
	{
		if (h.at(iend).lot.date >= end) break;
		iend++;
	}

	// Loop over sales
	for (size_t i = istart; i < iend; i++)
	{
		TaxLot *sell = &(h.at(i).lot);
		if (sell->active != -1) continue;

		size_t ibuy = istart;
		if (sell->tax_lot > 0)
			ibuy = GetPurchaseLot(h, i_header, sell->tax_lot);

		bool done = false; // finished with this sale entry

		// Loop over buys
		while (!done && ibuy != i)
		{
			TaxLot *buy = &(h.at(ibuy).lot);
			if (buy->active != 1)
			{
				ibuy++;
				continue;
			}

			time_t time_held = min(86400, DateToTime(sell->date) - DateToTime(buy->date));
			double realized = 0;
			int n;
			if (buy->n >= -sell->n)
			{
				n = -sell->n;

				realized = buy->realized * n / static_cast<double>(buy->n);
				buy->realized -= realized;
				realized += sell->realized - buy->price * n;

				sell->active = 0;
				header->nLots--; // need to subtract here before deletion invalidates the header pointer
				if (n == buy->n)
				{
					buy->active = 0;
					header->nLots--;
				}
				buy->n -= n;
				done = true;
			}
			else
			{
				if (sell->tax_lot > 0) throw std::invalid_argument("Specified tax lot for sale doesn't match");

				n = buy->n;

				realized = buy->realized;
				realized += (sell->price - buy->price) * n;
				sell->n += n;
				sell->realized -= sell->price * n;

				buy->active = 0;
				header->nLots--;

				done = false;
				ibuy++;
			}
			header->sumWeights += buy->price * n;
			header->sumReal += realized;
			header->sumReal1Y += realized * 365.0 * 86400.0 / time_held;
		}
		if (!done) OutputMessage(L"ReduceSalesLots entries didn't line up\n");
	}


}


// Returns an index into h to the 'n'th active stock purchase lot
size_t GetPurchaseLot(std::vector<Holdings> const & h, size_t i_header, size_t n)
{
	HoldingHeader const *header = &(h.at(i_header).head);
	size_t istart = i_header + 1;
	size_t iend = i_header + header->nLots + 1; // exclusive

	size_t i;
	for (i = istart; i < iend; i++)
	{
		if (h.at(i).lot.active == 1)
		{
			if (n == 0) return i;
			n--;
		}
	}
	return 0;
}


// returns a.front().ticker < ticker
inline bool Holdings_Compare(const std::vector<Holdings>& a, std::wstring const & ticker)
{
	if (a.empty()) throw std::invalid_argument("Holdings_Compare empty vector");
	return std::wstring(a.front().tickerInfo.ticker) < ticker;
}


// In the case of a stock merger, add all return information from 'mergee' into 'target'.
// The merger is indicated in the transactions via a sale of mergee stock and purchase of target stock.
void MergeHoldings(std::vector<Holdings> & target, std::vector<Holdings> mergee)
{
	int nAccounts = mergee.front().tickerInfo.nAccounts;
	size_t i_header = 1;
	for (int acc = 0; acc < nAccounts; acc++)
	{
		// First reduce all sales in mergee
		ReduceSalesLots(mergee, i_header, MkDate(9999, 0, 0));
		HoldingHeader & mergee_head = mergee[i_header].head;

		// Add results to target
		size_t i_target_header = GetHeaderIndex(target, mergee_head.account);
		if (i_target_header == (size_t)-1) throw ws_exception(L"MergeHoldings() didn't find account");

		HoldingHeader & target_head = target[i_target_header].head;
		target_head.sumReal += mergee_head.sumReal;
		target_head.sumReal1Y += mergee_head.sumReal1Y;
		target_head.sumWeights += mergee_head.sumWeights;

		// Move to next account in mergee
		i_header += mergee[i_header].head.nLots + mergee[i_header].head.nOptions + 1;
	}
}


///////////////////////////////////////////////////////////////////////////////
//							  Positions and Equity							 //
///////////////////////////////////////////////////////////////////////////////


// Calculates returns and APY using 'date' as end date. Set 'account' = -1 to use all accounts.
// 'prices' should contain the latest market price in the same order as the tickers in holdings, except CASH.
// Note this function does not truncate transactions to 'date'!
std::vector<Position> HoldingsToPositions(std::vector<Holdings> const & holdings,
	char account, date_t date, std::vector<double> prices)
{
	std::vector<Position> out;
	double net_transactions = 0.0f;

	// Loop over tickers
	size_t i = 0; // index into prices, discounting cash
	for (std::vector<Holdings> h : holdings)
	{
		if (h.empty()) throw std::invalid_argument("Holdings is empty");

		Position temp = {};
		temp.ticker = h[0].tickerInfo.ticker;
		if (temp.ticker != L"CASH")
		{
			temp.marketPrice = prices[i];
			i++;
		}

		// store running weighted numerator in temp.APY -- divide by sumWeights at end
		double sumWeights = 0;

		// Loop over accounts
		int nAccounts = h[0].tickerInfo.nAccounts;
		int iHeader = 1;
		int iAccount;
		for (iAccount = 0; iAccount < nAccounts; iAccount++)
		{
			if (iHeader > static_cast<int>(holdings.size()))
				throw std::invalid_argument("Headers misformed");

			if (account >= 0 && h[iHeader].head.account != account)
			{
				iHeader += h[iHeader].head.nLots + h[iHeader].head.nOptions + 1;
				continue;
			}

			ReduceSalesLots(h, iHeader, MkDate(9999, 0, 0)); // h is a copy, so ok the reduce everything
			HoldingHeader const & header = h[iHeader].head;

			if (temp.ticker == L"CASH")
			{
				temp.marketPrice += header.sumWeights;
				temp.realized_held += header.sumReal;
				if (account >= 0) break;
				iHeader += h[iHeader].head.nLots + h[iHeader].head.nOptions + 1;
				continue;
			}

			sumWeights += header.sumWeights;
			temp.realized_unheld += header.sumReal;
			temp.APY += header.sumReal1Y;

			// Loop over stock lots
			int iLot;
			for (iLot = iHeader + 1; iLot < iHeader + header.nLots + 1; iLot++)
			{
				TaxLot const & lot = h.at(iLot).lot;
				if (lot.active != 1) OutputMessage(L"Found inactive lot in HoldingsToPositions\n");

				temp.n += lot.n;
				temp.avgCost += lot.n * lot.price; // divide by temp.n at end
				temp.realized_held += lot.realized;
				double unrealized = (temp.marketPrice - lot.price) * lot.n;
				temp.unrealized += unrealized;
				temp.APY += GetWeightedAPY(unrealized + lot.realized, lot.date, date);
				net_transactions += -lot.n * lot.price;
			}

			sumWeights += temp.avgCost; // this is total cost right now

			// Loop over options lots;
			for (iLot = iHeader + header.nLots + 1;
				iLot < iHeader + header.nLots + header.nOptions + 1; iLot++)
			{
				Option const & opt = h.at(iLot).option;
				OptionPosition op_pos = {}; // Simply copy info for now, then collate at end
				op_pos.shares = opt.n;
				op_pos.expiration = opt.expiration;
				op_pos.strike = opt.strike;
				op_pos.price = opt.price;
				//op_pos.realized = opt.realized;

				switch (opt.type)
				{
				case TransactionType::PutShort:
					temp.realized_unheld += opt.realized;
					temp.APY += GetWeightedAPY(opt.realized, opt.date, opt.expiration); // approximate hold to expiration
					sumWeights += opt.strike * opt.n; // approximate weight as collateral of option
					op_pos.type = OptionType::CSP;
					break;
				case TransactionType::PutLong:
					temp.realized_unheld += -opt.price * opt.n + opt.realized;
					temp.APY += GetWeightedAPY(-opt.price * opt.n, opt.date, date);
					sumWeights += opt.price * opt.n;
					op_pos.type = OptionType::LP;
					break;
				case TransactionType::CallShort:
					temp.realized_unheld += opt.realized;
					temp.APY += GetWeightedAPY(opt.realized, opt.date, opt.expiration); // approximate hold to expiration
					sumWeights += opt.strike * opt.n; // approximate weight as strike of option
					op_pos.type = OptionType::CC;
					break;
				case TransactionType::CallLong:
					temp.realized_unheld += -opt.price * opt.n + opt.realized;
					temp.APY += GetWeightedAPY(-opt.price * opt.n, opt.date, date);
					sumWeights += opt.price * opt.n;
					op_pos.type = OptionType::LC;
					break;
				default:
					OutputMessage(L"Bad option type in HoldingsToPositions\n");
				}
				double unrealized = GetIntrinsicValue(opt, temp.marketPrice);
				temp.unrealized += unrealized;
				temp.APY += GetWeightedAPY(unrealized, opt.date, date);
				temp.options.push_back(op_pos);
			}

			if (account >= 0) break;
			iHeader += h[iHeader].head.nLots + h[iHeader].head.nOptions + 1;
		} // end loop over accounts
		if (account >= 0 && iAccount == nAccounts) continue; // no info for this ticker for this account
		if (temp.n > 0) temp.avgCost = temp.avgCost / temp.n;
		if (sumWeights > 0) temp.APY = temp.APY / sumWeights;
		collateOptions(temp);
		out.push_back(temp);

		if (temp.ticker != L"CASH")
		{
			net_transactions += temp.realized_held;
			net_transactions += temp.realized_unheld;
		}
	} // end loop over tickers

	// update cash with transaction total
	for (auto & x : out) if (x.ticker == L"CASH") x.realized_unheld = net_transactions;
	return out;
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
				if (lon->type != OptionType::LP || lon->expiration != shor->expiration || lon->shares == 0) continue;

				if (lon->strike > shor->strike) // Put debit spread
				{
					// TODO
				}
				else // Put credit spread
				{
					OptionPosition pos = {};
					pos.type = OptionType::PCS;
					pos.shares = min(lon->shares, shor->shares);
					pos.expiration = shor->expiration;
					pos.strike = shor->strike;
					pos.strike2 = lon->strike;
					pos.price = shor->price - lon->price;
					pos.cash_collateral = pos.shares * (pos.strike - pos.strike2);
					ops.push_back(pos);

					shor->shares -= pos.shares;
					lon->shares -= pos.shares;
					p.cash_collateral += pos.cash_collateral;
				}
				if (shor->shares == 0) break;
			}

			if (shor->shares > 0) // Cash secured put
			{
				shor->cash_collateral = shor->shares * shor->strike;
				p.cash_collateral += shor->cash_collateral;
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
				p.shares_collateral += shor->shares;
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


namespace EquityHistoryHelper
{
	// For each ticker, cummulate net transaction costs and net shares.
	struct TickerHelper
	{
		int n = 0; // net shares
		size_t iDate = 0; // index into ohlc for current date
		std::wstring ticker;
		std::vector<OptionPosition> opts; // use to calculate unrealized p/l
		std::vector<OHLC> ohlc;
	};

	// Assumes iDates set appropriately. Curr_date used only for options
	double GetEquity(std::vector<TickerHelper> & helper, date_t curr_date)
	{
		double out = 0.0;
		for (auto & x : helper)
		{
			if (x.iDate >= x.ohlc.size())
			{
				throw ws_exception(
					L"GetEquity date out of bounds: " + x.ticker + L" " +
						DateToWString(x.ohlc.back().date) + L", VOO " + DateToWString(curr_date)
				);
			}
			else if (x.ohlc[x.iDate].date != curr_date)
			{
				throw ws_exception(
					L"GetEquity date mistmatch: " + x.ticker + L" " +
					DateToWString(x.ohlc[x.iDate].date) + L", VOO " + DateToWString(curr_date)
				);
			}
			out += x.n * x.ohlc[x.iDate].close;
			for (auto & opt : x.opts)
			{
				if (curr_date < opt.expiration)
					out += GetIntrinsicValue(opt, x.ohlc[x.iDate].close);
			}
		}
		return out;
	}

	// Given a transaction, updates the helper, adding a new ticker entry if needed
	void HelperAddTransaction(std::vector<TickerHelper> & helper, Transaction const & t, double & cash,
		date_t curr_date, date_t lastCloseDate)
	{
		std::wstring tticker(t.ticker);
		auto it = std::find_if(helper.begin(), helper.end(),
			[&tticker](TickerHelper const & h) { return h.ticker == tticker; }
		);
		if (it == helper.end())
		{
			TickerHelper temp;
			temp.ticker = tticker;
			temp.n = 0;
			temp.ohlc = GetOHLC(tticker, apiSource::alpha, 1000, lastCloseDate);
			temp.iDate = FindDateOHLC(temp.ohlc, curr_date);
			helper.push_back(temp);
			it = helper.end() - 1;
		}

		// update helper/cash with transaction
		cash += t.value;
		if (isOption(t.type))
		{
			OptionPosition temp; // only care about type, n, expiration, and strike
			temp.type = transToOptionType(t.type);
			temp.shares = t.n * 100; // FIXME if not always 100 / contract
			temp.expiration = t.expiration;
			temp.strike = t.strike;
			it->opts.push_back(temp);
		}
		else
		{
			it->n += t.n;
		}
	}

}

// This will almost always crash because Alpha only allows 5 api calls per minute.
// Could launch a separate thread that sleeps for a minute
//
// Does not include transfers.
std::vector<TimeSeries> CalculateFullEquityHistory(char account, std::vector<Transaction> const & trans)
{
	std::vector<TimeSeries> out;
	if (trans.empty()) return out;

	// Get start date / transaction
	date_t curr_date = 0;
	size_t iTrans = 0;
	for (iTrans; iTrans < trans.size(); iTrans++)
	{
		if (trans[iTrans].account == account)
		{
			curr_date = trans[iTrans].date;
			break;
		}
	}
	if (curr_date == 0) return out;

	std::vector<EquityHistoryHelper::TickerHelper> helper;
	EquityHistoryHelper::TickerHelper temp;
	temp.ticker = L"VOO"; // use VOO to keep track of trading days
	temp.ohlc = GetOHLC(L"VOO", apiSource::alpha, 1000);
	temp.iDate = FindDateOHLC(temp.ohlc, curr_date);
	helper.push_back(temp);

	date_t lastCloseDate = temp.ohlc.back().date;
	double cash = 0; // net transactions, including options and transfers
	while (helper[0].iDate < helper[0].ohlc.size())
	{
		curr_date = helper[0].ohlc[helper[0].iDate].date;

		// Loop over transactions of the current date
		while (iTrans < trans.size() && trans[iTrans].date <= curr_date)
		{
			Transaction const & t = trans[iTrans];
			if (t.date < curr_date)
			{
				throw ws_exception(
					L"CalculateFullEquityHistory transaction date doesn't match VOO:\n" + t.to_wstring()
				);
			}
			if (t.account == account)
			{
				if (std::wstring(t.ticker) == L"CASH")
				{
					if (t.type != TransactionType::Transfer) cash += t.value;
				}
				else HelperAddTransaction(helper, trans[iTrans], cash, curr_date, lastCloseDate);
			}
			iTrans++;
		}

		double equity = GetEquity(helper, curr_date) + cash;
		out.push_back({ curr_date, equity });

		for (auto & x : helper) x.iDate++;
	}

	return out;
}

// TODO reformulate everything with a list of dates to start with.
void UpdateEquityHistory(std::vector<TimeSeries>& hist, std::vector<Position> const & positions, QStats const & qstats)
{
	if (hist.empty() || positions.empty()) 
		throw std::invalid_argument("UpdateEquityHistory passed empty vector(s)");

	std::vector<EquityHistoryHelper::TickerHelper> helper;
	helper.reserve(positions.size() - 1);
	double cash = 0;
	date_t lastHistDate = hist.back().date;
	date_t lastCloseDate = 0;

	for (Position const & p : positions)
	{
		if (p.ticker == L"CASH")
		{
			cash = p.realized_held + p.realized_unheld;
			continue;
		}
		apiSource source = apiSource::alpha;
		auto it = FindQStat(qstats, p.ticker);
		if (it != qstats.end())
		{
			if (lastCloseDate == 0 && it->first.closeTime != 0) lastCloseDate = GetDate(it->first.closeTime);
			if (lastHistDate > it->second.exDividendDate) source = apiSource::iex;
			// this is a bit too strict since exDivDate could be in the future, but would have to check
			// ex div history instead
		}

		EquityHistoryHelper::TickerHelper temp;
		temp.ticker = p.ticker;
		temp.n = p.n;
		temp.ohlc = GetOHLC(p.ticker, source, 1000, lastCloseDate); // lastCloseDate is updated by reference
		temp.opts = p.options;
		helper.push_back(temp);

		if (lastHistDate == lastCloseDate) return; // short circuit so we don't have to loop through all the positions
	}

	// Use VOO to find the next date, and set iDate in the helpers.
	date_t currDate;
	size_t iCurrDate_0 = FindDateOHLC(helper[0].ohlc, lastHistDate) + 1;
	if (iCurrDate_0 < helper[0].ohlc.size())
		currDate = helper[0].ohlc[iCurrDate_0].date;
	else 
		return;
	for (auto& h : helper)
		h.iDate = FindDateOHLC(h.ohlc, currDate);

	// Loop through days, using VOO as a tracker of market days
	while (helper[0].iDate < helper[0].ohlc.size())
	{
		currDate = helper[0].ohlc[helper[0].iDate].date;
		double equity = GetEquity(helper, currDate) + cash;
		hist.push_back({ currDate, equity });

		for (auto & x : helper) x.iDate++;
	}
}


// Weight is cost_in, so cancels
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


///////////////////////////////////////////////////////////////////////////////
//	                                 Plays                                   //
///////////////////////////////////////////////////////////////////////////////


// Assumes no partial closes
std::vector<Play> GetOptionPerformance(std::vector<Transaction> const & trans)
{
	std::vector<Play> plays;
	for (size_t i = 0; i < trans.size(); i++)
	{
		Transaction const& t = trans[i];
		if (!isOption(t.type)) continue;

		if (t.n > 0) // Open
		{
			Play p = {};
			p.type = t.type;
			p.n = t.n;
			p.start = t.date;
			p.expiration = t.expiration;
			p.price_enter = t.price;
			p.strike = t.strike;
			p.collateral = GetCollateral(trans, i);
			p.ticker = t.ticker;

			plays.push_back(p);
		}
		else // Close
		{
			for (Play & p : plays)
			{
				if (p.end == 0 && p.ticker == t.ticker && p.type == t.type 
					&& p.expiration == t.expiration && p.strike == t.strike)
				{
					p.end = t.date;
					p.price_exit = t.price;
					p.assignment_cost = GetAssignment(p, trans, i);
					break;
				}
			}
		}
	}

	return plays;
}

// Returns the collateral of an open option transaction (index iOpen).
// For long positions, the collateral is the purchase price.
// For short puts, the collateral is the strike.
// For short calls, a mini transactions->holdings is done to find the cost basis of currently held shares.
double GetCollateral(std::vector<Transaction> const& trans, size_t iOpen)
{
	if (!isShort(trans[iOpen].type))
		return 100 * trans[iOpen].n * trans[iOpen].price;
	if (trans[iOpen].type == TransactionType::PutShort)
		return 100 * trans[iOpen].n * trans[iOpen].strike;

	// Short call
	std::wstring ticker(trans[iOpen].ticker);
	std::vector<std::vector<Holdings>> holdings;
	for (size_t i = 0; i < iOpen; i++)
	{
		if (
			trans[i].ticker == ticker 
			&& trans[i].account == trans[iOpen].account 
			&& trans[i].type == TransactionType::Stock
			)
			AddTransactionToHoldings(holdings, trans[i]);
	}
	ReduceSalesLots(holdings[0], 1, MkDate(9999, 99, 99));

	double costBasis = 0; // add up cost of shares, divide at end
	int shares = 100 * trans[iOpen].n; // decrement as we iterate through lots
	for (auto it = holdings[0].rbegin(); it != holdings[0].rend(); it++)
	{
		if (it->lot.n >= shares) // all accounted for
		{
			costBasis += it->lot.price * shares;
			break;
		}
		else
		{
			costBasis += it->lot.price * it->lot.n;
			shares -= it->lot.n;
		}
	}
	return costBasis / (100. * trans[iOpen].n);
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