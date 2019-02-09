#include "stdafx.h"
#include "DataManaging.h"
#include "FileIO.h"
#include "HTTP.h"


const std::wstring ROOTDIR(L"C:/Users/Riley/Documents/Finances/Parthenos/"); // C:/Users/Riley/Documents/Finances/Parthenos/
const std::wstring IEXHOST(L"api.iextrading.com"); // api.iextrading.com

// Forward declarations

bool OHLC_Compare(const OHLC & a, const OHLC & b);


 // With curly braces removed:
 // "date":"2018-12-31","open":157.8529,"high":158.6794,"low":155.8117,"close":157.0663,"volume":35003466
OHLC parseIEXChartItem(std::string json)
{
	OHLC out;
	char date_buffer[50] = { 0 };
	const char format[] = R"("date":"%49[^"]","open":%lf,"high":%lf,"low":%lf,"close":%lf,"volume":%I64u)";

	int n = sscanf_s(json.c_str(), format, date_buffer, static_cast<unsigned>(_countof(date_buffer)), 
		&out.open, &out.high, &out.low, &out.close, &out.volume);
	if (n != 6)
	{
		OutputMessage(L"parseIEXChart sscanf_s failed! Only read %d/%d\n", n, 6);
	}

	// parse date
	int year;
	int month;
	struct tm date = {};
	n = sscanf_s(date_buffer, "%d-%d-%d", &year, &month, &date.tm_mday);
	if (n != 3)
	{
		OutputMessage(L"parseIEXChart sscanf_s 2 failed! Only read %d/%d\n", n, 3);
	}
	date.tm_year = year - 1900;
	date.tm_mon = month - 1;
	
	out.time = mktime(&date);
	return out;
}


std::vector<OHLC> parseIEXChart(std::string json)
{
	std::vector<OHLC> out;
	std::string token;
	std::string delim = "},{";
	
	size_t start = 2; // skip opening [{ 
	size_t end = json.find(delim);
	while (end != std::string::npos)
	{
		out.push_back(parseIEXChartItem(json.substr(start, end - start)));
		start = end + delim.length();
		end = json.find(delim, start);
	}
	out.push_back(parseIEXChartItem(json.substr(start, json.size() - 2 - start))); // skip closing }]

	return out;
}

// Reads, writes, and fetches data as necessary
std::vector<OHLC> GetOHLC(std::wstring ticker)
{
	std::wstring filename = ROOTDIR + ticker + L".ohlc";
	bool exists = FileExists(filename.c_str()); // Check before create file

	FileIO ohlcFile;
	ohlcFile.Init(filename);
	ohlcFile.Open();

	std::vector<OHLC> ohlcData;
	int days_to_get = 0; // 0 indicates no existing data -> get 5Y

	if (exists)
	{
		ohlcData = ohlcFile.Read<OHLC>();
		int latestDay = GetDay(ohlcData.back().time);
		Quote quote = GetQuote(ticker);
		int quoteDay = GetDay(quote.latestUpdate);
		if (quoteDay > latestDay)
		{
			if (quote.latestSource == iexLSource::close) // When is prev close used?
				days_to_get = quoteDay - latestDay;
			else if (quoteDay - 1 > latestDay)
				days_to_get = quoteDay - latestDay - 1;
			else // one day ahead, not closed yet
				return ohlcData;
		}
		else
		{
			return ohlcData;
		}
	}

	std::wstring chart_range;
	if (days_to_get == 0)
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

	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/" + ticker + L"/chart/" + chart_range,
		L"filter=date,open,high,low,close,volume");
	std::vector<OHLC> extra = parseIEXChart(json);

	if (days_to_get != 0)
	{
		auto next = std::lower_bound(extra.begin(), extra.end(), ohlcData.back(), OHLC_Compare);
		if (next == extra.end()) OutputMessage(L"lower_bound search found nothing...\n");
		else if ((*next).time == ohlcData.back().time) next++;
		if (next == extra.end()) OutputMessage(L"lower_bound search found nothing new...\n");

		int ind = next - extra.begin();
		bool err = ohlcFile.Append(reinterpret_cast<const void*>(extra.data() + ind), (extra.size() - ind) * sizeof(OHLC));
		if (!err) OutputMessage(L"Append OHLC failed\n");

		ohlcData.insert(ohlcData.end(), next, extra.end());
	}
	else
	{
		ohlcData = extra;
		ohlcFile.Write(reinterpret_cast<const void*>(ohlcData.data()), sizeof(OHLC) * ohlcData.size());
	}

	return ohlcData;
}

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

// returns a.time < b.time
bool OHLC_Compare(const OHLC & a, const OHLC & b)
{
	return a.time < b.time;
}