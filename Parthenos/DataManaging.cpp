#include "stdafx.h"
#include "DataManaging.h"
#include "FileIO.h"
#include "HTTP.h"

const std::wstring IEXHOST(L"api.iextrading.com"); // api.iextrading.com
const std::wstring ALPHAHOST(L"www.alphavantage.co");
const std::wstring ALPHAKEY(L"FJHB2XGE0A43171J"); // FJHB2XGE0A43171J

const std::wstring QUOTEFILTERS(L"open, close, latestPrice, latestSource, latestUpdate, latestVolume, avgTotalVolume,"
	L"previousClose,change,changePercent,closeTime");
const std::wstring STATSFILTERS(L"beta,week52high,week52low,dividendRate,dividendYield,year1ChangePercent,exDividendDate");

///////////////////////////////////////////////////////////
// --- Forward declarations ---

Quote parseFilteredQuote(std::string const & json, std::wstring const & ticker);
Stats parseFilteredStats(std::string const & json);
std::pair<Quote, Stats> parseQuoteStats(std::string const & json, std::wstring const & ticker);
std::vector<OHLC> GetOHLC_IEX(std::wstring ticker, size_t last_n, date_t & lastCloseDate);
std::vector<OHLC> GetOHLC_Alpha(std::wstring ticker, size_t last_n, date_t & lastCloseDate);
std::vector<OHLC> parseIEXChart(std::string json, date_t latest_date = 0);
std::vector<OHLC> parseAlphaChart(std::string json, date_t latestDate = 0);
OHLC parseIEXChartItem(std::string json);
OHLC parseAlphaChartItem(std::string json);
size_t FindDateOHLC(std::vector<OHLC> const & ohlc, date_t date); // returns ohlc.size() on fail

Transaction parseTransactionItem(std::string str);

void AddTransactionToTickerHoldings(std::vector<Holdings> & h, Transaction const & t);
void ReduceSalesLots(std::vector<Holdings> & h, size_t i_header, date_t end);
size_t GetPurchaseLot(std::vector<Holdings> const & h, size_t i_header, size_t n);
inline bool Holdings_Compare(const std::vector<Holdings>& a, std::wstring const & ticker);

inline double GetAPY(double gain, double cost_in, date_t start_date, date_t end_date);
inline double GetWeightedAPY(double gain, date_t start_date, date_t end_date);


///////////////////////////////////////////////////////////
// --- Stock Information Interface Functions ---

double GetPrice(std::wstring ticker)
{
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/" + ticker + L"/price");
	if (json.empty())
	{
		OutputMessage(L"GetPrice HTTP request empty\n");
		return 0;
	}

	double price;
	int n = sscanf_s(json.c_str(), "%lf", &price);
	if (n != 1) throw std::invalid_argument("GetPrice failed");

	return price;
}

// Queries IEX for a quote with fields
//		open, close, latestPrice, latestSource, latestUpdate 
//		latestVolume, avgTotalVolume, previousClose, change, changePercent, closeTime
//		(timestamps truncated to seconds)
// Throws std::invalid_argument if failed
Quote GetQuote(std::wstring ticker)
{
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/" + ticker + L"/quote",
		L"filter=" + QUOTEFILTERS);
	if (json.empty())
	{
		OutputMessage(L"GetQuote HTTP request empty\n");
		return {};
	}

	return parseFilteredQuote(json, ticker);
}

// Queries IEX for key stats.
// Throws std::invalid_argument if failed
Stats GetStats(std::wstring ticker)
{
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/" + ticker + L"/stats",
		L"filter=" + STATSFILTERS);
	if (json.empty())
	{
		OutputMessage(L"GetStats HTTP request empty\n");
		return {};
	}

	return parseFilteredStats(json);
}

std::pair<Quote, Stats> GetQuoteStats(std::wstring ticker)
{
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/" + ticker + L"/batch",
		L"types=quote,stats&filter=" + QUOTEFILTERS + L"," + STATSFILTERS);
	if (json.empty())
	{
		OutputMessage(L"GetQuoteStats HTTP request empty\n");
		return {};
	}

	return parseQuoteStats(json, ticker);
}

std::vector<std::pair<Quote, Stats>> GetBatchQuoteStats(std::vector<std::wstring> tickers)
{
	std::vector<std::pair<Quote, Stats>> out;

	std::wstring ticks;
	for (auto & x : tickers)
	{
		std::transform(x.begin(), x.end(), x.begin(), ::tolower);
		ticks.append(x + L",");
	}
	
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/market/batch",
		L"symbols=" + ticks + L"&types=quote,stats&filter=" + QUOTEFILTERS + L"," + STATSFILTERS);
	if (json.empty())
	{
		OutputMessage(L"GetBatchQuoteStats HTTP request empty\n");
		return out;
	}

	out.reserve(tickers.size());

	size_t start = 0;
	size_t end = 1; // skip first opening {
	for (size_t i = 0; i < tickers.size(); i++)
	{
		start = json.find("{", end);
		end = json.find("}}", start) + 2;
		out.push_back(parseQuoteStats(json.substr(start, end - start), tickers[i]));
	}

	return out;
}

std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source, size_t last_n)
{
	date_t lastCloseDate = 0;
	return GetOHLC(ticker, source, last_n, lastCloseDate);
}

// Reads, writes, and fetches data as necessary.
// 'last_n' is how many elements to read from disk, not the size of output,
// which gets appended to after fetching from the API. If 'lastCloseDate' != 0,
// uses that as the latest date to estimate how much data to get; otherwise gets
// the last close date from iex.
std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source, size_t last_n, date_t & lastCloseDate)
{
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
	std::vector<OHLC> out;
	try {
		switch (source)
		{
		case apiSource::alpha:
			out = GetOHLC_Alpha(ticker, last_n, lastCloseDate);
			break;
		case apiSource::iex:
		default:
			out = GetOHLC_IEX(ticker, last_n, lastCloseDate);
			break;
		}
	}
	catch (const std::invalid_argument& ia) {
		OutputDebugStringA(ia.what()); OutputDebugStringA("\n");
	}
	return out;
}


///////////////////////////////////////////////////////////
// --- Transactions and Positions Interface Functions ---

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

void AddTransactionToHoldings(NestedHoldings & holdings, Transaction const & t)
{
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

void PrintTickerHoldings(std::vector<Holdings> const & h)
{
	if (h.empty()) return;
	OutputMessage(L"-------------------------------------------\n");
	OutputMessage(L"%s, Accounts: %d\n", h[0].tickerInfo.ticker, h[0].tickerInfo.nAccounts);

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
		OutputDebugString(header->to_wstring().c_str());
		OutputMessage(L"\tLots:\n");
		for (int i = 1; i <= header->nLots; i++)
		{
			OutputMessage(L"\t\t%s", h[i_header + i].lot.to_wstring().c_str());
		}
		OutputMessage(L"\tOptions:\n");
		for (int i = header->nLots + 1; i <= header->nLots + header->nOptions; i++)
		{
			OutputMessage(L"\t\t%s\n", h[i_header + i].option.to_wstring().c_str());
		}
	}
}


///////////////////////////////////////////////////////////
// --- Positions and Equity Interface Functions ---

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
			for (iLot = iHeader + header.nLots + header.nOptions + 1;
				iLot < iHeader + header.nLots + header.nOptions + 1; iLot++)
			{
				Option const & opt = h.at(iLot).option;
				temp.options.push_back(opt);

				switch (opt.type)
				{
				case TransactionType::PutShort:
					temp.realized_unheld += opt.realized;
					temp.APY += GetWeightedAPY(opt.realized, opt.date, opt.expiration); // approximate hold to expiration
					temp.cash_collateral += opt.strike * opt.n;
					sumWeights += opt.strike * opt.n; // approximate weight as collateral of option
					break;
				case TransactionType::PutLong:
					temp.realized_unheld += -opt.price * opt.n;
					temp.APY += GetWeightedAPY(-opt.price * opt.n, opt.date, date);
					sumWeights += opt.price * opt.n;
					break;
				case TransactionType::CallShort:
					temp.realized_unheld += opt.realized;
					temp.APY += GetWeightedAPY(opt.realized, opt.date, opt.expiration); // approximate hold to expiration
					temp.shares_collateral += opt.n;
					sumWeights += opt.strike * opt.n; // approximate weight as strike of option
					break;
				case TransactionType::CallLong:
					temp.realized_unheld += -opt.price * opt.n;
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
			if (x.iDate > x.ohlc.size() || x.ohlc[x.iDate].date != curr_date)
			{
				OutputMessage(L"Error: GetEquityHistory date mismatch: %u\n", curr_date);
				throw std::invalid_argument("EquityHistory transaction date doesn't match VOO");
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
// Does not include transactions.
std::vector<TimeSeries> CalculateFullEquityHistory(char account, std::vector<Transaction> const & trans)
{
	std::vector<TimeSeries> out;
	if (trans.empty()) return out;

	try {

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
					OutputMessage(L"Error: EquityHistory transaction date doesn't match VOO:\n%s", t.to_wstring().c_str());
					throw std::invalid_argument("EquityHistory transaction date doesn't match VOO");
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
	}
	catch (const std::invalid_argument& ia) {
		OutputDebugStringA(ia.what()); OutputDebugStringA("\n");
	}

	return out;
}

void UpdateEquityHistory(std::vector<TimeSeries>& hist, std::vector<Position> const & positions, QStats const & qstats)
{
	if (hist.empty() || positions.empty()) return OutputMessage(L"UpdateEquityHistory passed empty vector(s)\n");

	std::vector<EquityHistoryHelper::TickerHelper> helper;
	helper.reserve(positions.size() - 1);
	double cash = 0;
	date_t currDate = hist.back().date;
	date_t lastCloseDate = 0;

	try {
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
				if (lastCloseDate == 0) lastCloseDate = GetDate(it->first.closeTime);
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
	catch (const std::invalid_argument& ia) {
		OutputDebugStringA(ia.what()); OutputDebugStringA("\n");
	}
}


///////////////////////////////////////////////////////////
// --- Stock Information Helper Functions ---

Quote parseFilteredQuote(std::string const & json, std::wstring const & ticker)
{
	Quote quote;
	char buffer[50] = { 0 };

	const char format[] = R"({"open":%lf,"close":%lf,"latestPrice":%lf,"latestSource":"%49[^"]","latestUpdate":%I64d,)"
		R"("latestVolume":%d,"avgTotalVolume":%d,"previousClose":%lf,"change":%lf,"changePercent":%lf,"closeTime":%I64d})";

	int n = sscanf_s(json.c_str(), format, &quote.open, &quote.close, &quote.latestPrice, buffer,
		static_cast<unsigned>(_countof(buffer)), &quote.latestUpdate, &quote.latestVolume, &quote.avgTotalVolume,
		&quote.previousClose, &quote.change, &quote.changePercent, &quote.closeTime
	);
	if (n != 11)
	{
		OutputMessage(L"sscanf_s failed! Only read %d/%d\n", n, 11);
		throw std::invalid_argument("GetQuote failed");
	}

	if (strcmp(buffer, "IEX real time price") == 0)
		quote.latestSource = iexLSource::real;
	else if (strcmp(buffer, "15 minute delayed price") == 0)
		quote.latestSource = iexLSource::delayed;
	else if (strcmp(buffer, "Close") == 0)
		quote.latestSource = iexLSource::close;
	else if (strcmp(buffer, "Previous close") == 0)
		quote.latestSource = iexLSource::prevclose;
	else
	{
		OutputMessage(L"Couldn't decipher latestSource %hs\n", buffer);
		throw std::invalid_argument("GetQuote failed");
	}

	quote.latestUpdate /= 1000; // remove milliseconds
	quote.closeTime /= 1000;
	quote.ticker = ticker;
	std::transform(quote.ticker.begin(), quote.ticker.end(), quote.ticker.begin(), ::toupper);
	return quote;
}

Stats parseFilteredStats(std::string const & json)
{
	Stats stats;
	char buffer[50] = { 0 };

	const char format[] = R"({"beta":%lf,"week52high":%lf,"week52low":%lf,"dividendRate":%lf,"dividendYield":%lf,)"
		R"("year1ChangePercent":%lf,"exDividendDate":"%49[^"]"})";

	int n = sscanf_s(json.c_str(), format, &stats.beta, &stats.week52high, &stats.week52low, &stats.dividendRate,
		&stats.dividendYield, &stats.year1ChangePercent, buffer, static_cast<unsigned>(_countof(buffer))
	);
	if (n == 6) // exDividendDate failed
	{
		stats.exDividendDate = 0;
		return stats;
	}
	else if (n != 7)
	{
		OutputMessage(L"GetStats sscanf_s failed! Only read %d/%d\n", n, 7);
		throw std::invalid_argument("GetQuote failed");
	}

	int year, month, date;
	n = sscanf_s(buffer, "%d-%d-%d", &year, &month, &date);
	if (n != 3)
	{
		OutputMessage(L"GetStats sscanf_s 2 failed! Only read %d/%d\n", n, 3);
		throw std::invalid_argument("praseIEXChartItem failed");
	}
	stats.exDividendDate = MkDate(year, month, date);

	return stats;
}

std::pair<Quote, Stats> parseQuoteStats(std::string const & json, std::wstring const & ticker)
{
	size_t start = json.find("{", 1); // skip first opening {
	size_t end = json.find("}", start) + 1; // include closing }
	Quote quote = parseFilteredQuote(json.substr(start, end - start), ticker);

	start = json.find("{", end);
	end = json.find("}", start) + 1;
	Stats stats = parseFilteredStats(json.substr(start, end - start));

	return { quote, stats };
}

// Reads saved data from 'filename', initializing 'ohlcFile' and populating 'days_to_get'.
//
// 'days_to_get' is only approximate; doesn't account for trading vs. non-trading days.
// Sets 'days_to_get' == -1 if there's no saved data. If lastCloseDate != 0, uses it to estimate
// 'days_to_get'. Otherwise finds last close date from iex.
//
// Only returns the 'last_n' entries from file.
std::vector<OHLC> GetSavedOHLC(std::wstring const & ticker, std::wstring const & filename, 
	FileIO & ohlcFile, size_t last_n, int & days_to_get, date_t & lastCloseDate)
{
	bool exists = FileExists(filename.c_str()); // Check before create file

	ohlcFile.Init(filename);
	ohlcFile.Open();

	std::vector<OHLC> ohlcData;
	days_to_get = -1;

	if (exists)
	{
		if (last_n == 0)
			ohlcData = ohlcFile.Read<OHLC>();
		else
			ohlcData = ohlcFile.ReadEnd<OHLC>(last_n);
		if (ohlcData.empty()) throw std::invalid_argument("saved data empty");
		date_t latestDate = ohlcData.back().date;

		if (lastCloseDate == 0)
		{
			Quote quote = GetQuote(ticker);
			lastCloseDate = GetDate(quote.closeTime);
		}

		if (lastCloseDate > latestDate)
			days_to_get = ApproxDateDiff(lastCloseDate, latestDate);
		else
			days_to_get = 0;
	}
	return ohlcData;
}

std::vector<OHLC> GetOHLC_IEX(std::wstring const ticker, size_t last_n, date_t & lastCloseDate)
{
	std::wstring filename = ROOTDIR + ticker + L".iex.ohlc";
	FileIO ohlcFile;
	int days_to_get;
	std::vector<OHLC> ohlcData = GetSavedOHLC(ticker, filename, ohlcFile, last_n, days_to_get, lastCloseDate);
	std::wstring chart_range;

	if (days_to_get == 0)
		return ohlcData;
	else if (days_to_get == -1)
		chart_range = L"5y";
	else if (days_to_get <= 28)
		chart_range = L"1m";
	else if (days_to_get <= 3 * 28)
		chart_range = L"3m";
	else if (days_to_get <= 6 * 28)
		chart_range = L"6m";
	else if (days_to_get <= 365)
		chart_range = L"1y";
	else if (days_to_get <= 2 * 365)
		chart_range = L"2y";
	else
		chart_range = L"5y";

	date_t latestDate = 0;
	if (ohlcData.size() > 0)
		latestDate = ohlcData.back().date;

	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/" + ticker + L"/chart/" + chart_range,
		L"filter=date,open,high,low,close,volume");
	if (json.empty())
	{
		OutputMessage(L"GetOHLC_IEX HTTP request empty\n");
		return ohlcData;
	}

	std::vector<OHLC> extra = parseIEXChart(json, latestDate);

	if (days_to_get == -1)
	{
		ohlcData = extra;
		ohlcFile.Write(reinterpret_cast<const void*>(ohlcData.data()), sizeof(OHLC) * ohlcData.size());
	}
	else
	{
		bool err = ohlcFile.Append(reinterpret_cast<const void*>(extra.data()), extra.size() * sizeof(OHLC));
		if (!err) OutputMessage(L"Append OHLC failed\n");
		ohlcData.insert(ohlcData.end(), extra.begin(), extra.end());
	}

	return ohlcData;
}

std::vector<OHLC> GetOHLC_Alpha(std::wstring ticker, size_t last_n, date_t & lastCloseDate)
{
	std::wstring filename = ROOTDIR + ticker + L".alpha.ohlc";
	FileIO ohlcFile;
	int days_to_get;
	std::vector<OHLC> ohlcData = GetSavedOHLC(ticker, filename, ohlcFile, last_n, days_to_get, lastCloseDate);
	
	std::wstring outputsize;
	if (days_to_get == 0)
		return ohlcData;
	else if (days_to_get == -1 || days_to_get > 100)
	{
		OutputDebugString(L"Fetching 20 years from AlphaVantage\n");
		outputsize = L"full";
	}
	else
		outputsize = L"compact";

	date_t latestDate = 0;
	if (ohlcData.size() > 0)
		latestDate = ohlcData.back().date;

	std::string json = SendHTTPSRequest_GET(ALPHAHOST, L"query", 
		L"function=TIME_SERIES_DAILY&symbol=" + ticker + L"&outputsize=" + outputsize + L"&apikey=" + ALPHAKEY);
	if (json.empty())
	{
		OutputMessage(L"GetOHLC_Alpha HTTP request empty\n");
		return ohlcData;
	}

	std::vector<OHLC> extra = parseAlphaChart(json, latestDate);
	std::reverse(extra.begin(), extra.end());

	// Write to file
	DWORD nBytes = sizeof(OHLC) * extra.size();

	// AlphaVantage adds the current trading day to the time series, even if not closed yet.
	// Pass on this data, but don't write it to file.
	SYSTEMTIME utc, now;
	GetSystemTime(&utc);
	if (!SystemTimeToEasternTime(&utc, &now)) OutputMessage(L"SystemTimeToEasternTime failed\n");
	if (extra.back().date == MkDate(now.wYear, now.wMonth, now.wDay) &&
		(now.wHour < 16 || (now.wHour == 16 && now.wMinute < 1))) // give 1 minute leway
	{
		nBytes -= sizeof(OHLC);
	}

	if (days_to_get == -1)
	{
		ohlcData = extra;
		if (nBytes > 0)
			ohlcFile.Write(reinterpret_cast<const void*>(ohlcData.data()), nBytes);
	}
	else if (extra.size() > 0)
	{
		if (nBytes > 0)
		{
			bool err = ohlcFile.Append(reinterpret_cast<const void*>(extra.data()), nBytes);
			if (!err) OutputMessage(L"Append OHLC failed\n");
		}
		ohlcData.insert(ohlcData.end(), extra.begin(), extra.end());
	}

	return ohlcData;
}

// Returns OHLC data for days after latestDate. latestDate == 0 to 
// get all entries.
std::vector<OHLC> parseIEXChart(std::string json, date_t latestDate)
{
	std::vector<OHLC> out;
	std::string delim = "},{";

	size_t start = 2; // skip opening [{ 
	size_t end = json.find(delim);
	while (end != std::string::npos)
	{
		OHLC temp = parseIEXChartItem(json.substr(start, end - start));
		if (latestDate == 0 || temp.date > latestDate)
			out.push_back(temp);
		start = end + delim.length();
		end = json.find(delim, start);
	}
	out.push_back(parseIEXChartItem(json.substr(start, json.size() - 2 - start))); // skip closing }]

	return out;
}

// Returns OHLC data for days after latestDate. latestDate == 0 to 
// get all entries.
std::vector<OHLC> parseAlphaChart(std::string json, date_t latestDate)
{
	std::vector<OHLC> out;
	std::string delim = "},\n";

	size_t start = 1; // skip opening {
	size_t end = 0;
	for (int i = 0; i < 2; i++)
	{
		start = json.find('{', start) + 1; // next { for meta data, next { for time series
		end = json.find(delim, start); // first }, closes meta data, next }, closes first date data
	}
	
	while (end != std::string::npos)
	{
		OHLC temp = parseAlphaChartItem(json.substr(start, end - start));
		if (latestDate == 0 || temp.date > latestDate)
			out.push_back(temp);
		else // done reading new data
			return out;
		start = end + delim.length();
		end = json.find(delim, start);
	}
	OHLC temp = parseAlphaChartItem(json.substr(start, json.size()));
	if (latestDate == 0 || temp.date > latestDate)
		out.push_back(temp);

	return out;
}

// With curly braces removed:
// "date":"2018-12-31","open":157.8529,"high":158.6794,"low":155.8117,"close":157.0663,"volume":35003466
OHLC parseIEXChartItem(std::string json)
{
	OHLC out;
	char date_buffer[50] = { 0 };
	const char format[] = R"("date":"%49[^"]","open":%lf,"high":%lf,"low":%lf,"close":%lf,"volume":%I32u)";

	int n = sscanf_s(json.c_str(), format, date_buffer, static_cast<unsigned>(_countof(date_buffer)),
		&out.open, &out.high, &out.low, &out.close, &out.volume);
	if (n != 6)
	{
		OutputMessage(L"parseIEXChart sscanf_s failed! Only read %d/%d\n", n, 6);
		throw std::invalid_argument("praseIEXChartItem failed");
	}

	// parse date
	int year, month, date;
	n = sscanf_s(date_buffer, "%d-%d-%d", &year, &month, &date);
	if (n != 3)
	{
		OutputMessage(L"parseIEXChart sscanf_s 2 failed! Only read %d/%d\n", n, 3);
		throw std::invalid_argument("praseIEXChartItem failed");
	}
	out.date = MkDate(year, month, date);

	return out;
}

/*
		"2018-09-18": {
			"1. open": "112.1900",
			"2. high": "113.6950",
			"3. low": "111.7200",
			"4. close": "113.2100",
			"5. volume": "22170934"
*/
OHLC parseAlphaChartItem(std::string json)
{
	OHLC out; 
	std::string token;
	std::string delim = "\"";

	int i = 0;
	size_t start = 0;
	size_t end = json.find(delim, start);
	while (end != std::string::npos)
	{
		token = json.substr(start, end - start);
		// i == 0: spaces
		if (i == 1)
		{
			int year, month, date;
			int n = sscanf_s(token.c_str(), "%d-%d-%d", &year, &month, &date);
			if (n != 3)
			{
				OutputMessage(L"parseAlphaChartItem sscanf_s failed! Only read %d/%d\n", n, 3);
				OutputDebugStringA(json.c_str()); OutputDebugString(L"\n");
				throw std::invalid_argument("parseAlphaChartItem failed");
			}
			out.date = MkDate(year, month, date);
		}
		// i == 2: open bracket and new lines
		// i == 3: "1. open"
		// i == 4: ": "
		else if (i == 5)
		{
			int n = sscanf_s(token.c_str(), "%lf", &out.open);
			if (n != 1)	throw std::invalid_argument("parseAlphaChartItem failed");
		}
		else if (i == 9)
		{
			int n = sscanf_s(token.c_str(), "%lf", &out.high);
			if (n != 1) throw std::invalid_argument("parseAlphaChartItem failed");
		}
		else if (i == 13)
		{
			int n = sscanf_s(token.c_str(), "%lf", &out.low);
			if (n != 1) throw std::invalid_argument("parseAlphaChartItem failed");
		}
		else if (i == 17)
		{
			int n = sscanf_s(token.c_str(), "%lf", &out.close);
			if (n != 1) throw std::invalid_argument("parseAlphaChartItem failed");
		}
		else if (i == 21)
		{
			int n = sscanf_s(token.c_str(), "%I32u", &out.volume);
			if (n != 1) throw std::invalid_argument("parseAlphaChartItem failed");
			break;
		}
		start = end + delim.length();
		end = json.find(delim, start);
		i++;
	}
	return out;
}

size_t FindDateOHLC(std::vector<OHLC> const & ohlc, date_t date)
{
	auto it = std::lower_bound(ohlc.begin(), ohlc.end(), date,
		[](OHLC const & ohlc, date_t date) { return ohlc.date < date; }
	);
	return static_cast<size_t>(it - ohlc.begin());
}


///////////////////////////////////////////////////////////
// --- Transactions Helper Functions ---

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


///////////////////////////////////////////////////////////
// --- Holdings Helper Functions ---

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
			temp.lot.realized = 0.0;

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
			ReduceSalesLots(h, i_header, t.expiration);

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
			if (nshares != 0) OutputMessage(L"Shares did not equal dividend payout\n");
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
		if (isOption(t.type))
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
					realized = -(opt->n * opt->price) + t.value;
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
		// Don't delete stock lots in case between ex-dividend and payout.
		// Add a negative lot to the chain and delete on dividend receipt instead.
		{
			if (t.type != TransactionType::Stock)
				return OutputMessage(L"Unrecognized transaction:\n%s", t.to_wstring().c_str());

			header->nLots++;

			Holdings temp;
			temp.lot.active = -1;
			temp.lot.n = t.n;
			temp.lot.date = t.date;
			temp.lot.tax_lot = t.tax_lot;
			temp.lot.price = t.price;
			temp.lot.realized = t.value;

			h.insert(h.begin() + i_header + header->nLots, temp);
		}
	}
}

// Reduces sales against purchases for stock lots with date prior to end
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

			time_t time_held = DateToTime(sell->date) - DateToTime(buy->date);
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


///////////////////////////////////////////////////////////
// --- Positions Helper Functions ---

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


///////////////////////////////////////////////////////////
// --- Struct Printing Functions ---


std::wstring Transaction_struct::to_wstring() const
{
	wchar_t buffer[300];
	swprintf_s(buffer, _countof(buffer),
		L"%s: %s (Account: %d) %s, n: %d, Value: %.2lf, Price: %.4lf, Expiration: %s, Strike: %.2lf\n",
		DateToWString(date).c_str(), ticker, static_cast<int>(account), ::to_wstring(type).c_str(),
		n, value, price, DateToWString(expiration).c_str(), strike
	);
	return std::wstring(buffer);
}

std::wstring Transaction_struct::to_wstring(std::vector<std::wstring> const & accounts) const
{
	wchar_t buffer[300];
	swprintf_s(buffer, _countof(buffer),
		L"%s: %s (%s) %s, n: %d, Value: %.2lf, Price: %.4lf, Expiration: %s, Strike: %.2lf\n",
		DateToWString(date).c_str(), ticker, accounts[account].c_str(), ::to_wstring(type).c_str(),
		n, value, price, DateToWString(expiration).c_str(), strike
	);
	return std::wstring(buffer);
}