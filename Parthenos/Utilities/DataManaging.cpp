#include "../stdafx.h"
#include "DataManaging.h"
#include "FileIO.h"
#include "HTTP.h"


///////////////////////////////////////////////////////////
// --- Forward declarations ---

Transaction parseTransactionItem(std::string str);
double GetCollateral(std::vector<Transaction> const& trans, size_t iOpen);
double GetAssignment(Play& p, std::vector<Transaction> const& trans, size_t iClose);

void AddTransactionToTickerHoldings(std::vector<Holdings> & h, Transaction const & t);
void ReduceSalesLots(std::vector<Holdings> & h, size_t i_header, date_t end);
size_t GetPurchaseLot(std::vector<Holdings> const & h, size_t i_header, size_t n);
inline bool Holdings_Compare(const std::vector<Holdings>& a, std::wstring const & ticker);

inline double GetAPY(double gain, double cost_in, date_t start_date, date_t end_date);
inline double GetWeightedAPY(double gain, date_t start_date, date_t end_date);


///////////////////////////////////////////////////////////////////////////////
//	                              Transactions			            		 //
///////////////////////////////////////////////////////////////////////////////


// CSV with columns date,account,ticker,n,value,price,type,expiration,strike
// separated by \r\n (generate from excel). Assumes tax_lot = 0.
std::vector<Transaction> CSVtoTransactions(std::wstring filepath)
{
	FileIO file;
	file.Init(filepath);
	file.Open(GENERIC_READ);
	std::vector<char> fileData = file.Read<char>();
	fileData.push_back('\0');

	// Converting to CSV seems to add 3 metacharacters in beginning
	std::string data(fileData.begin() + 3, fileData.end());
	std::vector<Transaction> out;

	std::string delim = "\r\n";
	size_t start = 0;
	size_t end = data.find(delim);
	while (end != std::string::npos)
	{
		Transaction temp = parseTransactionItem(data.substr(start, end - start));
		out.push_back(temp);
		start = end + delim.length();
		end = data.find(delim, start);
	}
	// No need to check final substr because \r\n at end of CSV

	return out;
}


// Columns date,account,ticker,n,value,price,type,expiration,strike
// Assumes tax_lot = 0
Transaction parseTransactionItem(std::string str)
{
	Transaction out;
	char account;
	char type;
	char buffer[50] = {};
	const char format[] = "%I32u,%hhd,%49[^,],%d,%lf,%lf,%hhd,%I32u,%lf";

	int n = sscanf_s(str.c_str(), format, &out.date, &account, buffer, static_cast<unsigned>(_countof(buffer)),
		&out.n, &out.value, &out.price, &type, &out.expiration, &out.strike);
	if (n != 9)
	{
		OutputMessage(L"parseTransactionItem sscanf_s failed! Only read %d/%d\n", n, 9);
	}

	out.tax_lot = 0;
	out.account = account;
	out.type = static_cast<TransactionType>(type);

	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, out.ticker, PortfolioObjects::maxTickerLen + 1, buffer, _TRUNCATE);

	return out;
}


///////////////////////////////////////////////////////////////////////////////
//	                      Transcations --> Holdings		            		 //
///////////////////////////////////////////////////////////////////////////////


void AddTransactionToHoldings(NestedHoldings & holdings, Transaction const & t)
{
	if (t.type == TransactionType::Custom) 
		return AddCustomTransactionToHoldings(holdings, t);

	// See if ticker is present already
	std::wstring ticker(t.ticker);
	auto it = std::lower_bound(holdings.begin(), holdings.end(), ticker, Holdings_Compare);
	if (it == holdings.end() ||
		std::wstring(it->front().tickerInfo.ticker) != ticker)
	{
		// insert (rare, so using vectors is better than a list)
		Holdings h_ticker;
		wcscpy_s(h_ticker.tickerInfo.ticker, PortfolioObjects::maxTickerLen + 1, t.ticker);
		h_ticker.tickerInfo.nAccounts = 0;

		std::vector<Holdings> temp = { h_ticker };
		it = holdings.insert(it, temp);
	}

	AddTransactionToTickerHoldings(*it, t);
}



// Updates the holdings for a single ticker with the transaction.
// Assumes h has the ticker struct
void AddTransactionToTickerHoldings(std::vector<Holdings> & h, Transaction const & t)
{
	if (h.size() < 1) throw std::invalid_argument("AddTransactionToHoldings no ticker!");
	if (std::wstring(t.ticker) != std::wstring(h.front().tickerInfo.ticker))
		throw std::invalid_argument("AddTransactionToHoldings wrong ticker");

	////////////////////////////////////////////////
	// Get the header for the proper account. 

	HoldingHeader *header = nullptr; // Pointer so can initialize below
	size_t i_header = 1; // 0 is the ticker 
	while (i_header < h.size())
	{
		header = &(h[i_header].head);
		if (header->account == t.account) break;
		i_header += header->nLots + header->nOptions + 1;
	}

	if (i_header == h.size()) // new account == new header
	{
		h[0].tickerInfo.nAccounts++;

		Holdings h_header = {}; // zero except for account; re-use code below
		h_header.head.account = t.account;

		h.push_back(h_header);
		header = &(h.back().head);
	}
	else if (i_header > h.size()) throw std::invalid_argument("AddTransactionToHoldings nLots wrong?");
	if (!header) return OutputMessage(L"header is null for some reason\n");

	////////////////////////////////////////////////
	// Don't add lots for cash
	if (std::wstring(t.ticker) == L"CASH")
	{
		if (t.type == TransactionType::Transfer)
			header->sumWeights += t.value;
		else if (t.type == TransactionType::Interest)
			header->sumReal += t.value;
		else
			OutputMessage(L"Unrecognized transaction:\n%s", t.to_wstring().c_str());
		return;
	}

	////////////////////////////////////////////////
	// Open position -> push back new lot
	if (t.n > 0)
	{
		if (isOption(t.type))
		{
			header->nOptions++;

			Option opt = {};
			opt.type = t.type;
			opt.n = t.n * 100; // FIXME if not always 100 / contract
			opt.date = t.date;
			opt.expiration = t.expiration;
			opt.price = static_cast<float>(t.price);
			opt.strike = static_cast<float>(t.strike);
			if (isShort(t.type)) opt.realized = static_cast<float>(t.value);
			else opt.realized = static_cast<float>(t.value) + opt.n * opt.price; // Catch rounding error / fees here

			Holdings temp;
			temp.option = opt;
			h.insert(h.begin() + i_header + header->nLots + header->nOptions, temp);
		}
		else
		{
			header->nLots++;

			Holdings temp;
			temp.lot.active = 1;
			temp.lot.n = t.n;
			temp.lot.date = t.date;
			temp.lot.tax_lot = 0;
			temp.lot.price = t.price;
			temp.lot.realized = t.value + t.n * t.price; // Catch rounding error here :(

			h.insert(h.begin() + i_header + header->nLots, temp);
		}
	}
	////////////////////////////////////////////////
	// Fee / dividend
	else if (t.n == 0)
	{
		switch (t.type)
		{
		case TransactionType::Dividend: // Search through lots to assign realized to lot
		{
			int nshares = static_cast<int>(round(t.value / t.price));
			double rounding_error = t.value;
			for (size_t i = i_header + 1; i < i_header + header->nLots + 1; i++)
			{
				TaxLot *lot = &(h.at(i).lot);
				if (lot->date >= t.expiration) break; // >= ex-div date

				lot->realized += lot->n * t.price;
				nshares -= lot->n;
				rounding_error -= lot->n * t.price;
			}
			if (nshares < 0)
				throw ws_exception(L"AddTransactionToTickerHoldings() dividend negative nshares: " + std::to_wstring(nshares));
			header->sumReal += rounding_error; // give extra to header (includes partial cents)
			break;
		}
		case TransactionType::Fee: // Incorporated into t.value on a sale already
		default:
			OutputMessage(L"Unrecognized transaction:\n%s", t.to_wstring().c_str());
			break;
		}
	}
	////////////////////////////////////////////////
	// Close position
	else
	{
		if (isOption(t.type)) // TODO reimplement this using reduceSalesLots, will break with multiple options in one stock
		{
			if (t.tax_lot > header->nOptions)
				return OutputMessage(L"Bad lot:\n%s", t.to_wstring().c_str());

			auto it = h.begin() + i_header + header->nLots + t.tax_lot + 1;
			Option *opt = &(it->option);
			if (opt->type != t.type || opt->expiration != t.expiration || opt->strike != t.strike)
				return OutputMessage(L"Bad lot:\n%s", t.to_wstring().c_str());

			time_t time_held = DateToTime(t.date) - DateToTime(opt->date);
			double realized;
			if (opt->n <= -t.n * 100) // delete this lot
			{
				if (isShort(t.type))
				{
					realized = opt->realized + t.value;
					header->sumWeights += opt->n * opt->strike; // use collateral as effective cost
				}
				else
				{
					realized = opt->realized - (opt->n * opt->price) + t.value;
					header->sumWeights += opt->n * opt->price;
				}
				header->sumReal += realized;
				header->sumReal1Y += realized * 365.0 * 86400.0 / time_held;
				header->nOptions--;
				h.erase(it); // must call after any pointer dereference
			}
			else // partial close
			{
				if (isShort(t.type))
				{
					// t.n < 0 and t.value < 0
					opt->realized += t.n * 100 * opt->price; // swap realized from opt to header
					realized = -t.n * 100 * opt->price + t.value;
					header->sumWeights += -t.n * 100 * opt->strike; // use collateral as effective cost
				}
				else
				{
					realized = t.n * 100 * opt->price + t.value; // t.n < 0 and t.value > 0
					header->sumWeights += -t.n * 100 * opt->price;
				}
				opt->n += t.n;
				header->sumReal += realized;
				header->sumReal1Y += realized * 365.0 * 86400.0 / time_held;
			}
		}
		else // stock
		{
			if (t.type != TransactionType::Stock)
				return OutputMessage(L"Unrecognized transaction:\n%s", t.to_wstring().c_str());

			// Add a negative lot to the chain and call ReduceSalesLots
			header->nLots++;

			Holdings temp;
			temp.lot.active = -1;
			temp.lot.n = t.n;
			temp.lot.date = t.date;
			temp.lot.tax_lot = t.tax_lot;
			temp.lot.price = t.price;
			temp.lot.realized = t.value;

			h.insert(h.begin() + i_header + header->nLots, temp);
			ReduceSalesLots(h, i_header, t.date + 1);
		}
	}
}



// Returns a vector sorted by tickers. Each ticker has a std::vector<Holdings> with the following elements:
//		[0]: A TickerInfo struct with the ticker and the number of accounts
// For each account:
//		[1]: A HoldingsHeader struct with the number of stock lots and option lots
//		[2]: The stock lots
//		[3]: The option lots
//
// Note that the stock lots contain sell information, denoted by lot.active == -1, to account for sales between
// the ex-dividend and payout dates. Call ReduceSalesLots to collapse these.
NestedHoldings FullTransactionsToHoldings(std::vector<Transaction> const & transactions)
{
	std::vector<std::vector<Holdings>> holdings; // Holdings for each ticker, sorted by ticker

	for (Transaction const & t : transactions)
	{
		AddTransactionToHoldings(holdings, t);
	}

	return holdings;
}


// Handles custom transactions, such as spin-offs, mergers, and stock splits
// TODO add transaction type for above
void AddCustomTransactionToHoldings(NestedHoldings & holdings, Transaction const & t)
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


// Get the index of the header of a specific account, for a ticker's holdings
size_t GetHeaderIndex(std::vector<Holdings> const & h, char acc)
{
	int nAccounts = h.front().tickerInfo.nAccounts;
	size_t i_header = 1;
	for (int iacc = 0; iacc < nAccounts; iacc++)
	{
		if (h[i_header].head.account == acc) return i_header;
		i_header += h[i_header].head.nLots + h[i_header].head.nOptions + 1;
	}
	return -1;
}


NestedHoldings FlattenedHoldingsToTickers(std::vector<Holdings> const & holdings)
{
	size_t i = 0;
	std::vector<std::vector<Holdings>> out;
	while (i < holdings.size())
	{
		std::wstring ticker(holdings[i].tickerInfo.ticker);
		int nAccounts = holdings[i].tickerInfo.nAccounts;
		size_t i_header = i + 1;
		for (int j = 0; j < nAccounts; j++)
		{
			HoldingHeader const *header = &(holdings[i_header].head);
			i_header += 1 + header->nLots + header->nOptions;
		}
		std::vector<Holdings> temp(holdings.begin() + i, holdings.begin() + i_header);
		out.push_back(temp);
		i = i_header;
	}
	if (i != holdings.size()) throw std::invalid_argument("FlattenedHoldings misformed");
	return out;
}

std::wstring TickerHoldingsToWString(std::vector<Holdings> const & h)
{
	std::wstring out;
	if (h.empty()) return out;
	out.append(L"-------------------------------------------\n");
	out.append(FormatMsg(L"%s, Accounts: %d\n", h[0].tickerInfo.ticker, h[0].tickerInfo.nAccounts));

	std::vector<size_t> headers;
	size_t i_header = 1; // 0 is the ticker 
	while (i_header < h.size())
	{
		headers.push_back(i_header);
		HoldingHeader const *header = &(h[i_header].head);
		i_header += header->nLots + header->nOptions + 1;
	}

	for (size_t i_header : headers)
	{
		HoldingHeader const *header = &(h[i_header].head);
		out.append(header->to_wstring());
		out.append(L"\tLots:\n");
		for (int i = 1; i <= header->nLots; i++)
		{
			out.append(FormatMsg(L"\t\t%s", h[i_header + i].lot.to_wstring().c_str()));
		}
		out.append(L"\tOptions:\n");
		for (int i = header->nLots + 1; i <= header->nLots + header->nOptions; i++)
		{
			out.append(FormatMsg(L"\t\t%s\n", h[i_header + i].option.to_wstring(true).c_str()));
		}
	}
	return out;
}


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

	// Delete inactive lots
	for (size_t i = iend - 1; i > i_header; i--)
	{
		if (h.at(i).lot.active == 0)
		{
			h.erase(h.begin() + i);
		}
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
std::vector<Position> HoldingsToPositions(NestedHoldings const & holdings,
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
				temp.options.push_back(opt);

				switch (opt.type)
				{
				case TransactionType::PutShort:
					temp.realized_held += opt.realized;
					temp.APY += GetWeightedAPY(opt.realized, opt.date, opt.expiration); // approximate hold to expiration
					temp.cash_collateral += opt.strike * opt.n;
					sumWeights += opt.strike * opt.n; // approximate weight as collateral of option
					break;
				case TransactionType::PutLong:
					temp.realized_unheld += -opt.price * opt.n + opt.realized;
					temp.APY += GetWeightedAPY(-opt.price * opt.n, opt.date, date);
					sumWeights += opt.price * opt.n;
					break;
				case TransactionType::CallShort:
					temp.realized_held += opt.realized;
					temp.APY += GetWeightedAPY(opt.realized, opt.date, opt.expiration); // approximate hold to expiration
					temp.shares_collateral += opt.n;
					sumWeights += opt.strike * opt.n; // approximate weight as strike of option
					break;
				case TransactionType::CallLong:
					temp.realized_unheld += -opt.price * opt.n + opt.realized;
					temp.APY += GetWeightedAPY(-opt.price * opt.n, opt.date, date);
					sumWeights += opt.price * opt.n;
					break;
				default:
					OutputMessage(L"Bad option type in HoldingsToPositions\n");
				}
				double unrealized = GetIntrinsicValue(opt, temp.marketPrice);
				temp.unrealized += unrealized;
				temp.APY += GetWeightedAPY(unrealized, opt.date, date);
			}

			if (account >= 0) break;
			iHeader += h[iHeader].head.nLots + h[iHeader].head.nOptions + 1;
		} // end loop over accounts
		if (account >= 0 && iAccount == nAccounts) continue; // no info for this ticker for this account
		if (temp.n > 0) temp.avgCost = temp.avgCost / temp.n;
		if (sumWeights > 0) temp.APY = temp.APY / sumWeights;
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


namespace EquityHistoryHelper
{
	// For each ticker, cummulate net transaction costs and net shares.
	struct TickerHelper
	{
		int n = 0; // net shares
		size_t iDate = 0; // index into ohlc for current date
		std::wstring ticker;
		std::vector<Option> opts; // use to calculate unrealized p/l
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
			Option temp; // only care about type, n, expiration, and strike
			temp.type = t.type;
			temp.n = t.n * 100; // FIXME if not always 100 / contract
			temp.expiration = t.expiration;
			temp.strike = static_cast<float>(t.strike);
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

void UpdateEquityHistory(std::vector<TimeSeries>& hist, std::vector<Position> const & positions, QStats const & qstats)
{
	if (hist.empty() || positions.empty()) 
		throw std::invalid_argument("UpdateEquityHistory passed empty vector(s)");

	std::vector<EquityHistoryHelper::TickerHelper> helper;
	helper.reserve(positions.size() - 1);
	double cash = 0;
	date_t currDate = hist.back().date;
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
			if (currDate > it->second.exDividendDate) source = apiSource::iex;
			// this is a bit too strict since exDivDate could be in the future, but would have to check
			// ex div history instead
		}

		EquityHistoryHelper::TickerHelper temp;
		temp.ticker = p.ticker;
		temp.n = p.n;
		temp.ohlc = GetOHLC(p.ticker, source, 1000, lastCloseDate); // lastCloseDate is updated by reference
		temp.iDate = FindDateOHLC(temp.ohlc, currDate) + 1; // start with next new day
		temp.opts = p.options;
		helper.push_back(temp);

		if (currDate == lastCloseDate) return; // short circuit so we don't have to loop through all the positions
	}

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

std::wstring Transaction::to_wstring() const
{
	wchar_t buffer[300];
	swprintf_s(buffer, _countof(buffer),
		L"%s: %s (Account: %d) %s, n: %d, Value: %.2lf, Price: %.4lf, Expiration: %s, Strike: %.2lf",
		DateToWString(date).c_str(), ticker, static_cast<int>(account), ::to_wstring(type).c_str(),
		n, value, price, DateToWString(expiration).c_str(), strike
	);
	return std::wstring(buffer);
}

std::wstring Transaction::to_wstring(std::vector<std::wstring> const& accounts) const
{
	wchar_t buffer[300];
	swprintf_s(buffer, _countof(buffer),
		L"%s: %s (%s) %s, n: %d, Value: %.2lf, Price: %.4lf, Expiration: %s, Strike: %.2lf",
		DateToWString(date).c_str(), ticker, accounts[account].c_str(), ::to_wstring(type).c_str(),
		n, value, price, DateToWString(expiration).c_str(), strike
	);
	return std::wstring(buffer);
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