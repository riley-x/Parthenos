#include "../stdafx.h"
#include "API.h"
#include "FileIO.h"
#include "HTTP.h"
#include "jsonette.h"

using namespace jsonette;

#ifdef TEST_IEX
const std::wstring IEXHOST(L"sandbox.iexapis.com");
const std::wstring IEXTOKEN(L"Tpk_385b81e3a3754032b6e9af9e67bc03c3");
#else
const std::wstring IEXHOST(L"cloud.iexapis.com");
const std::wstring IEXTOKEN(L"pk_d150988c47f74c71863b8501c0457b94");
#endif

const std::wstring ALPHAHOST(L"www.alphavantage.co");
const std::wstring ALPHAKEY(L"FJHB2XGE0A43171J");

const std::wstring TDHOST(L"api.tdameritrade.com");
std::wstring TDKEY;

const std::wstring QUOTEFILTERS(L"open,close,latestPrice,latestSource,latestUpdate,latestVolume,avgTotalVolume,"
	L"previousClose,change,changePercent,iexCloseTime");
// From IEX quote endpoint: All response attributes related to 15 minute delayed market-wide price data are only available to paid subscribers
// So this includes open, close, high, low.

const std::wstring STATSFILTERS(L"beta,week52high,week52low,ttmDividendRate,dividendYield,year1ChangePercent,"
	L"exDividendDate,nextEarningsDate");



///////////////////////////////////////////////////////////
// --- Forward declarations ---

Quote parseIEXQuote(std::string const& json, std::wstring const& ticker);
Quote parseTDQuote(std::string const& json, std::wstring const& ticker);
Stats parseFilteredStats(std::string const& json);
std::pair<Quote, Stats> parseQuoteStats(std::string const& json, std::wstring const& ticker);
std::vector<OHLC> GetOHLC_IEX(std::wstring ticker, size_t last_n, date_t& lastCloseDate);
std::vector<OHLC> GetOHLC_Alpha(std::wstring ticker, size_t last_n, date_t& lastCloseDate);
std::vector<OHLC> parseIEXChart(std::string json, date_t latest_date = 0);
std::vector<OHLC> parseAlphaChart(std::string json, date_t latestDate = 0);
OHLC parseIEXChartItem(std::string json);
OHLC parseAlphaChartItem(std::string json);


///////////////////////////////////////////////////////////
// --- Stock Information Interface Functions ---

double GetPrice(std::wstring ticker)
{
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"v1/stock/" + ticker + L"/price", L"token=" + IEXTOKEN);
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
Quote GetQuote(std::wstring ticker, apiSource source)
{

	switch (source)
	{
	case apiSource::iex:
	{
		std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
		std::string json = SendHTTPSRequest_GET(IEXHOST, L"v1/stock/" + ticker + L"/quote",
			L"filter=" + QUOTEFILTERS + L"&token=" + IEXTOKEN);
		if (json.empty())
		{
			OutputMessage(L"GetQuote HTTP request empty\n");
			return Quote();
		}
		return parseIEXQuote(json, ticker);
	}
	case apiSource::td:
	{
		std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::toupper);
		std::string json = SendHTTPSRequest_GET(TDHOST, L"v1/marketdata/quotes",
			L"apikey=" + TDKEY + L"&symbol=" + ticker);
		return parseTDQuote(json, ticker);
	}
	default:
		return Quote();
	}
}

// Queries IEX for key stats.
// Throws std::invalid_argument if failed
Stats GetStats(std::wstring ticker)
{
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"v1/stock/" + ticker + L"/stats",
		L"filter=" + STATSFILTERS + L"&token=" + IEXTOKEN);
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
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"v1/stock/" + ticker + L"/batch",
		L"types=quote,stats&filter=" + QUOTEFILTERS + L"," + STATSFILTERS + L"&token=" + IEXTOKEN);
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
	for (auto& x : tickers)
	{
		std::transform(x.begin(), x.end(), x.begin(), ::tolower);
		ticks.append(x + L",");
	}

	std::string json = SendHTTPSRequest_GET(IEXHOST, L"v1/stock/market/batch",
		L"symbols=" + ticks + L"&types=quote,stats&filter=" + QUOTEFILTERS + L"," + STATSFILTERS + L"&token=" + IEXTOKEN);
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
std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source, size_t last_n, date_t& lastCloseDate)
{
	try {
		std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
		std::vector<OHLC> out;

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

		return out;
	}
	catch (...) {
		std::throw_with_nested(ws_exception(L"GetOHLC failed: " + ticker));
	}
}


///////////////////////////////////////////////////////////
// --- Stock Information Helper Functions ---


Quote parseIEXQuote(std::string const& json, std::wstring const& ticker)
{
	Quote quote;
	static const int nFields = 11;
	char buffer[nFields][50] = { 0 };

	enum vars {
		open, close, latestPrice, latestSource, latestUpdate, latestVolume, avgTotalVolume, previousClose,
		change, changePercent, closeTime
	};

	const char format[] = R"({"open":%49[^,],"close":%49[^,],"latestPrice":%49[^,],"latestSource":%49[^,],)"
		R"("latestUpdate":%49[^,],"latestVolume":%49[^,],"avgTotalVolume":%49[^,],"previousClose":%49[^,],)"
		R"("change":%49[^,],"changePercent":%49[^,],"iexCloseTime":%49[^}]})";

	int n = sscanf_s(json.c_str(), format,
		buffer[0], static_cast<unsigned>(_countof(buffer[0])), // open
		buffer[1], static_cast<unsigned>(_countof(buffer[1])), // close
		buffer[2], static_cast<unsigned>(_countof(buffer[2])), // latestPrice
		buffer[3], static_cast<unsigned>(_countof(buffer[3])), // latestSource
		buffer[4], static_cast<unsigned>(_countof(buffer[4])), // latestUpdate
		buffer[5], static_cast<unsigned>(_countof(buffer[5])), // latestVolume
		buffer[6], static_cast<unsigned>(_countof(buffer[6])), // avgTotalVolume
		buffer[7], static_cast<unsigned>(_countof(buffer[7])), // previousClose
		buffer[8], static_cast<unsigned>(_countof(buffer[8])), // change
		buffer[9], static_cast<unsigned>(_countof(buffer[9])), // changePercent
		buffer[10], static_cast<unsigned>(_countof(buffer[10])) // closeTime
	);
	if (n != nFields)
	{
		OutputDebugStringA(json.c_str());
		throw ws_exception(FormatMsg(L"parseFilteredQuote sscanf_s 1 failed! Only read %d/%d\n", n, nFields));
	}

	if (!sscanf_s(buffer[open], "%lf", &quote.open)) quote.open = 0;
	if (!sscanf_s(buffer[close], "%lf", &quote.close)) quote.close = 0;
	if (!sscanf_s(buffer[latestPrice], "%lf", &quote.latestPrice)) quote.latestPrice = 0;
	if (!sscanf_s(buffer[latestUpdate], "%I64d", &quote.latestUpdate)) quote.latestUpdate = 0;
	if (!sscanf_s(buffer[latestVolume], "%d", &quote.latestVolume)) quote.latestVolume = 0;
	if (!sscanf_s(buffer[avgTotalVolume], "%d", &quote.avgTotalVolume)) quote.avgTotalVolume = 0;
	if (!sscanf_s(buffer[previousClose], "%lf", &quote.previousClose)) quote.previousClose = 0;
	if (!sscanf_s(buffer[change], "%lf", &quote.change)) quote.change = 0;
	if (!sscanf_s(buffer[changePercent], "%lf", &quote.changePercent)) quote.changePercent = 0;
	if (!sscanf_s(buffer[closeTime], "%I64d", &quote.closeTime)) quote.closeTime = 0;

	if (strcmp(buffer[latestSource], "\"IEX real time price\"") == 0)
		quote.latestSource = iexLSource::real;
	else if (strcmp(buffer[latestSource], "\"15 minute delayed price\"") == 0)
		quote.latestSource = iexLSource::delayed;
	else if (strcmp(buffer[latestSource], "\"Close\"") == 0)
		quote.latestSource = iexLSource::close;
	else if (strcmp(buffer[latestSource], "\"Previous close\"") == 0)
		quote.latestSource = iexLSource::prevclose;
	else
		quote.latestSource = iexLSource::null;

	quote.latestUpdate /= 1000; // remove milliseconds
	quote.closeTime /= 1000; // this is sometimes 0 (maybe before market open)?
	quote.ticker = ticker;
	std::transform(quote.ticker.begin(), quote.ticker.end(), quote.ticker.begin(), ::toupper);
	return quote;
}


Quote parseTDQuote(std::string const& json, std::wstring const& ticker)
{
	if (json.empty())
	{
		OutputMessage(L"GetQuote HTTP request empty\n");
		return Quote();
	}
	JSON quotes(json);
	JSON const& j = quotes[w2s(ticker)];

	Quote q;
	q.latestVolume = j["totalVolume"]; // ?
	q.avgTotalVolume = j["totalVolume"]; // ?
	q.open = j["openPrice"];
	q.high = j["highPrice"];
	q.low = j["lowPrice"];
	q.close = j["closePrice"];
	q.latestPrice = j["lastPrice"];
	q.previousClose = 0; // N/A
	q.change = j["markChangeInDouble"];
	q.changePercent = j["markPercentChangeInDouble"];
	q.latestUpdate = j["quoteTimeInLong"].get<time_t>() / 1000; // Remove milliseconds
	q.closeTime = 0; // N/A
	q.ticker = ticker;
	q.latestSource = iexLSource::null;

	return q;
}

Stats parseFilteredStats(std::string const& str)
{
	Stats stats = {};
	JSON json(str);
	
	if (JSON const* j = json.find2("beta")) stats.beta = *j;
	if (JSON const* j = json.find2("week52high")) stats.week52high = *j;
	if (JSON const* j = json.find2("week52low")) stats.week52low = *j;
	if (JSON const* j = json.find2("ttmDividendRate")) stats.dividendRate = *j;
	if (JSON const* j = json.find2("dividendYield")) stats.dividendYield = *j;
	if (JSON const* j = json.find2("year1ChangePercent")) stats.year1ChangePercent = *j;
	if (JSON const* j = json.find2("exDividendDate")) stats.exDividendDate = parseDate(*j, "%d-%d-%d");
	if (JSON const* j = json.find2("nextEarningsDate")) stats.nextEarningsDate = parseDate(*j, "%d-%d-%d");

	return stats;
}

std::pair<Quote, Stats> parseQuoteStats(std::string const& json, std::wstring const& ticker)
{
	Stats stats; Quote quote;
	bool quote_first = false;
	if (json.size() > 3 && (json[2] == 'q' || json[2] == 'Q')) quote_first = true;


	size_t start = json.find("{", 1); // skip first opening {
	size_t end = json.find("}", start) + 1; // include closing }
	if (quote_first)
		quote = parseIEXQuote(json.substr(start, end - start), ticker);
	else
		stats = parseFilteredStats(json.substr(start, end - start));

	start = json.find("{", end);
	end = json.find("}", start) + 1;
	if (!quote_first)
		quote = parseIEXQuote(json.substr(start, end - start), ticker);
	else
		stats = parseFilteredStats(json.substr(start, end - start));

	return { quote, stats };
}

// Reads saved data from 'filename', populating 'days_to_get'.
//
// 'days_to_get' is only approximate; doesn't account for trading vs. non-trading days.
// Sets 'days_to_get' == -1 if there's no saved data. If lastCloseDate != 0, uses it to estimate
// 'days_to_get'. Otherwise finds last close date from iex.
//
// Only returns the 'last_n' entries from file.
std::vector<OHLC> GetSavedOHLC(std::wstring ticker, apiSource source,
	size_t last_n, int& days_to_get, date_t& lastCloseDate)
{
	std::vector<OHLC> ohlcData;
	days_to_get = -1;

	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
	std::wstring filename;
	if (source == apiSource::iex) filename = ROOTDIR + ticker + L".iex.ohlc";
	else filename = ROOTDIR + ticker + L".alpha.ohlc";

	if (FileExists(filename.c_str()))
	{
		FileIO ohlcFile;
		ohlcFile.Init(filename);
		ohlcFile.Open(GENERIC_READ);

		if (last_n == 0)
			ohlcData = ohlcFile.Read<OHLC>();
		else
			ohlcData = ohlcFile.ReadEnd<OHLC>(last_n);
		if (!ohlcData.empty())
		{
			date_t latestDate = ohlcData.back().date;

			if (lastCloseDate == 0)
			{
				Quote quote = GetQuote(ticker);
				lastCloseDate = (quote.closeTime != 0) ? GetDate(quote.closeTime) :
					(quote.latestUpdate != 0) ? GetDate(quote.latestUpdate) :
					throw ws_exception(L"GetSavedOHLC() couldn't calculate lastCloseDate");
			}

			if (lastCloseDate > latestDate)
				days_to_get = ApproxDateDiff(lastCloseDate, latestDate);
			else
				days_to_get = 0;
		}
	}
	return ohlcData;
}

std::vector<OHLC> GetOHLC_IEX(std::wstring const ticker, size_t last_n, date_t& lastCloseDate)
{
	int days_to_get;
	std::vector<OHLC> ohlcData = GetSavedOHLC(ticker, apiSource::iex, last_n, days_to_get, lastCloseDate);
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

	std::string json = SendHTTPSRequest_GET(IEXHOST, L"v1/stock/" + ticker + L"/chart/" + chart_range,
		L"filter=date,open,high,low,close,volume&token=" + IEXTOKEN);
	if (json.empty())
	{
		OutputMessage(L"GetOHLC_IEX HTTP request empty\n");
		return ohlcData;
	}

	std::vector<OHLC> extra = parseIEXChart(json, latestDate);

	// Write to file
	DWORD nBytes = static_cast<DWORD>(sizeof(OHLC) * extra.size());
	if (nBytes > 0)
	{
		std::wstring filename = ROOTDIR + ticker + L".iex.ohlc";
		FileIO ohlcFile;
		ohlcFile.Init(filename);
		ohlcFile.Open();
		if (days_to_get == -1) // no saved data (ohlcData is currently empty)
		{
			ohlcFile.Write(reinterpret_cast<const void*>(extra.data()), nBytes);
			ohlcData = extra;
		}
		else
		{
			bool err = ohlcFile.Append(reinterpret_cast<const void*>(extra.data()), nBytes);
			if (!err) OutputMessage(L"Append OHLC failed\n");
			ohlcData.insert(ohlcData.end(), extra.begin(), extra.end());
		}
	}

	return ohlcData;
}

std::vector<OHLC> GetOHLC_Alpha(std::wstring ticker, size_t last_n, date_t& lastCloseDate)
{
	int days_to_get;
	std::vector<OHLC> ohlcData = GetSavedOHLC(ticker, apiSource::alpha, last_n, days_to_get, lastCloseDate);

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
	DWORD nBytes = static_cast<DWORD>(sizeof(OHLC) * extra.size());
	if (nBytes > 0)
	{
		std::wstring filename = ROOTDIR + ticker + L".alpha.ohlc";
		FileIO ohlcFile;
		ohlcFile.Init(filename);
		ohlcFile.Open();
		if (days_to_get == -1) // no saved data (ohlcData is currently empty)
		{
			ohlcFile.Write(reinterpret_cast<const void*>(extra.data()), nBytes);
			ohlcData = extra;
		}
		else
		{
			bool err = ohlcFile.Append(reinterpret_cast<const void*>(extra.data()), nBytes);
			if (!err) OutputMessage(L"Append OHLC failed\n");
			ohlcData.insert(ohlcData.end(), extra.begin(), extra.end());
		}
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
	OHLC temp = parseIEXChartItem(json.substr(start, json.size() - 2 - start)); // skip closing }]
	if (latestDate == 0 || temp.date > latestDate)
		out.push_back(temp);

	return out;
}

// Returns OHLC data for days after latestDate. latestDate == 0 to 
// get all entries.
std::vector<OHLC> parseAlphaChart(std::string json, date_t latestDate)
{
	std::vector<OHLC> out;
	std::string delim = "},\n";

	size_t start = 0;
	size_t end = 0;

	// AlphaVantage adds the current trading day to the time series, even if not closed yet.
	std::string key = R"("3. Last Refreshed": ")";
	start = json.find(key, start) + key.length();
	end = json.find("\",", start);
	int year, month, date, hour, min, sec;
	int n = sscanf_s(json.substr(start, end).c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &date, &hour, &min, &sec);
	if (n == 3) // Late in the day AlphaVantage doesn't display the hour/min/sec
	{
		hour = 16; // flag hour to be after close
	}
	else if (n != 6)
	{
		OutputDebugStringA(("JSON dump:\n" + json + "\n").c_str());
		throw ws_exception(
			FormatMsg(L"parseAlphaChart sscanf_s failed! Only read %d/%d\n", n, 6)
		);
	}
	date_t refreshDate = MkDate(year, month, date);

	start = json.find('{', start) + 1; // next { for time series
	end = json.find(delim, start); // closes first date data

	while (end != std::string::npos)
	{
		OHLC temp = parseAlphaChartItem(json.substr(start, end - start));
		if (latestDate == 0 || temp.date > latestDate)
		{
			if (!(temp.date == refreshDate && hour < 16))
				out.push_back(temp);
		}
		else // done reading new data
			return out;
		start = end + delim.length();
		end = json.find(delim, start);
	}
	OHLC temp = parseAlphaChartItem(json.substr(start, json.size() - start));
	if ((latestDate == 0 || temp.date > latestDate) && !(temp.date == refreshDate && hour < 16))
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
		throw std::invalid_argument("parseIEXChartItem failed");
	}

	// parse date
	int year, month, date;
	n = sscanf_s(date_buffer, "%d-%d-%d", &year, &month, &date);
	if (n != 3)
	{
		OutputMessage(L"parseIEXChart sscanf_s 2 failed! Only read %d/%d\n", n, 3);
		throw std::invalid_argument("parseIEXChartItem failed");
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

size_t FindDateOHLC(std::vector<OHLC> const& ohlc, date_t date)
{
	auto it = std::lower_bound(ohlc.begin(), ohlc.end(), date,
		[](OHLC const& ohlc, date_t date) { return ohlc.date < date; }
	);
	return static_cast<size_t>(it - ohlc.begin());
}

///////////////////////////////////////////////////////////
// --- Struct Printing Functions ---

std::wstring OHLC::to_wstring(bool mini) const
{
	if (mini)
	{
		wchar_t buffer[300];
		swprintf_s(buffer, _countof(buffer),
			L"Open: %s\nHigh: %s\nLow: %s\nClose: %s",
			FormatDollar(open).c_str(), FormatDollar(high).c_str(), FormatDollar(low).c_str(), FormatDollar(close).c_str()
		);
		return std::wstring(buffer);
	}
	else
	{
		return L"Date: " + DateToWString(date)
			+ L", Open: " + std::to_wstring(open)
			+ L", High: " + std::to_wstring(high)
			+ L", Low: " + std::to_wstring(low)
			+ L", Close: " + std::to_wstring(close)
			+ L", Volume: " + std::to_wstring(volume)
			+ L"\n";
	}
}

std::wstring Quote::to_wstring() const
{
	return ticker + L": "
		+ L", Date: " + TimeToWString(latestUpdate)
		+ L", Open: " + std::to_wstring(open)
		+ L", Close: " + std::to_wstring(close)
		+ L", latestPrice: " + std::to_wstring(latestPrice)
		+ L", latestSource: " + std::to_wstring(static_cast<int>(latestSource))
		+ L", latestVolume: " + std::to_wstring(latestVolume)
		+ L", avgTotalVolume: " + std::to_wstring(avgTotalVolume)
		+ L"\nlatestUpdate: " + std::to_wstring(latestUpdate)
		+ L", previousClose: " + std::to_wstring(previousClose)
		+ L", change: " + std::to_wstring(change)
		+ L", changePercent: " + std::to_wstring(changePercent)
		+ L", closeTime: " + TimeToWString(closeTime)
		+ L"\n";
}

std::wstring Stats::to_wstring() const
{
	return L"Beta: " + std::to_wstring(beta)
		+ L", 52WeekHigh: " + std::to_wstring(week52high)
		+ L", 52WeekLow: " + std::to_wstring(week52low)
		+ L", dividendRate: " + std::to_wstring(dividendRate)
		+ L", dividendYield: " + std::to_wstring(dividendYield)
		+ L", exDividendDate: " + DateToWString(exDividendDate)
		+ L", 1YearChange%: " + std::to_wstring(year1ChangePercent)
		+ L"\n";
}