#include "stdafx.h"
#include "DataManaging.h"
#include "FileIO.h"
#include "HTTP.h"

#include <algorithm>

const std::wstring ROOTDIR(L"C:/Users/Riley/Documents/Finances/Parthenos/"); // C:/Users/Riley/Documents/Finances/Parthenos/
const std::wstring IEXHOST(L"api.iextrading.com"); // api.iextrading.com
const std::wstring ALPHAHOST(L"www.alphavantage.co");
const std::wstring ALPHAKEY(L"FJHB2XGE0A43171J"); // FJHB2XGE0A43171J

// --- Forward declarations ---

bool OHLC_Compare(const OHLC & a, const OHLC & b);
std::vector<OHLC> GetOHLC_IEX(std::wstring ticker, size_t last_n);
std::vector<OHLC> GetOHLC_Alpha(std::wstring ticker, size_t last_n);
std::vector<OHLC> parseIEXChart(std::string json, date_t latest_date = 0);
std::vector<OHLC> parseAlphaChart(std::string json, date_t latestDate = 0);
OHLC parseIEXChartItem(std::string json);
OHLC parseAlphaChartItem(std::string json);

// --- Interface Functions ---


// Queries IEX for a quote with fields
//		open, close, latestPrice, latestSource, latestUpdate (timestamp truncated to seconds)
//		latestVolume, avgTotalVolume
Quote GetQuote(std::wstring ticker)
{
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/" + ticker + L"/quote",
		L"filter=open,close,latestPrice,latestSource,latestUpdate,latestVolume,avgTotalVolume");

	Quote quote;
	char buffer[50] = { 0 };

	const char format[] = R"({"open":%lf,"close":%lf,"latestPrice":%lf,"latestSource":"%49[^"]","latestUpdate":%I64d,)"
		R"("latestVolume":%d,"avgTotalVolume":%d})";

	int n = sscanf_s(json.c_str(), format, &quote.open, &quote.close, &quote.latestPrice, buffer,
		static_cast<unsigned>(_countof(buffer)), &quote.latestUpdate, &quote.latestVolume, &quote.avgTotalVolume);
	if (n != 7)
	{
		OutputMessage(L"sscanf_s failed! Only read %d/%d\n", n, 7);
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
		OutputMessage(L"Couldn't decipher latestSource %hs\n", buffer);

	quote.latestUpdate /= 1000; // remove milliseconds
	return quote;
}


// Reads, writes, and fetches data as necessary
std::vector<OHLC> GetOHLC(std::wstring ticker, apiSource source, size_t last_n)
{
	std::vector<OHLC> out;
	switch (source)
	{
	case apiSource::alpha:
		out = GetOHLC_Alpha(ticker, last_n);
		break;
	case apiSource::iex:
	default:
		out = GetOHLC_IEX(ticker, last_n);
		break;
	}
	if (last_n > 0 && out.size() > last_n)
	{
		return std::vector<OHLC>(out.end() - last_n, out.end());
	}
	return out;
}



// --- Helper Functions ---

// Reads saved data from 'filename', initializing 'ohlcFile' and populating 'days_to_get'.
// Only returns the 'last_n' entries from file.
// Sets 'days_to_get' == -1 if there's no saved data.
std::vector<OHLC> GetSavedOHLC(std::wstring const & ticker, std::wstring const & filename, 
	FileIO & ohlcFile, size_t last_n, int & days_to_get)
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
		date_t latestDate = ohlcData.back().date;

		Quote quote = GetQuote(ticker);
		date_t quoteDay = GetDate(quote.latestUpdate);

		if (quoteDay > latestDate)
		{
			if (quote.latestSource == iexLSource::close) // When is prev close used?
				days_to_get = ApproxDateDiff(quoteDay, latestDate);
			else
				days_to_get = ApproxDateDiff(quoteDay, latestDate) - 1;
		}
		else
		{
			days_to_get = 0;
		}
	}
	return ohlcData;
}


std::vector<OHLC> GetOHLC_IEX(std::wstring const ticker, size_t last_n)
{
	std::wstring filename = ROOTDIR + ticker + L".iex.ohlc";
	FileIO ohlcFile;
	int days_to_get;
	std::vector<OHLC> ohlcData = GetSavedOHLC(ticker, filename, ohlcFile, last_n, days_to_get);
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


std::vector<OHLC> GetOHLC_Alpha(std::wstring ticker, size_t last_n)
{
	std::wstring filename = ROOTDIR + ticker + L".alpha.ohlc";
	FileIO ohlcFile;
	int days_to_get;
	std::vector<OHLC> ohlcData = GetSavedOHLC(ticker, filename, ohlcFile, last_n, days_to_get);
	
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
	std::vector<OHLC> extra = parseAlphaChart(json, latestDate);
	std::reverse(extra.begin(), extra.end());

	
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
		start = end + delim.length();
		end = json.find(delim, start);
	}
	out.push_back(parseAlphaChartItem(json.substr(start, json.size())));

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
	}

	// parse date
	int year, month, date;
	n = sscanf_s(date_buffer, "%d-%d-%d", &year, &month, &date);
	if (n != 3)
	{
		OutputMessage(L"parseIEXChart sscanf_s 2 failed! Only read %d/%d\n", n, 3);
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
			if (n != 3) OutputMessage(L"parseIEXChart sscanf_s %d failed! Only read %d/%d\n", i, n, 3);
			out.date = MkDate(year, month, date);
		}
		// i == 2: open bracket and new lines
		// i == 3: "1. open"
		// i == 4: ": "
		else if (i == 5)
		{
			int n = sscanf_s(token.c_str(), "%lf", &out.open);
			if (n != 1) OutputMessage(L"parseIEXChart sscanf_s %d failed! Only read %d/%d\n", i, n, 1);
		}
		else if (i == 9)
		{
			int n = sscanf_s(token.c_str(), "%lf", &out.high);
			if (n != 1) OutputMessage(L"parseIEXChart sscanf_s %d failed! Only read %d/%d\n", i, n, 1);
		}
		else if (i == 13)
		{
			int n = sscanf_s(token.c_str(), "%lf", &out.low);
			if (n != 1) OutputMessage(L"parseIEXChart sscanf_s %d failed! Only read %d/%d\n", i, n, 1);
		}
		else if (i == 17)
		{
			int n = sscanf_s(token.c_str(), "%lf", &out.close);
			if (n != 1) OutputMessage(L"parseIEXChart sscanf_s %d failed! Only read %d/%d\n", i, n, 1);
		}
		else if (i == 21)
		{
			int n = sscanf_s(token.c_str(), "%I32u", &out.volume);
			if (n != 1) OutputMessage(L"parseIEXChart sscanf_s %d failed! Only read %d/%d\n", i, n, 1);
			break;
		}
		start = end + delim.length();
		end = json.find(delim, start);
		i++;
	}
	return out;
}

// returns a.date < b.date
bool OHLC_Compare(const OHLC & a, const OHLC & b)
{
	return a.date < b.date;
}