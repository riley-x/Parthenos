#include "stdafx.h"
#include "DataManaging.h"
#include "FileIO.h"
#include "HTTP.h"

#include <ctime>

const std::wstring ROOTDIR(L"C:/Users/Riley/Documents/Finances/Parthenos/"); // C:/Users/Riley/Documents/Finances/Parthenos/
const std::wstring IEXHOST(L"api.iextrading.com"); // api.iextrading.com

std::vector<OHLC> GetOHLC(std::wstring ticker, std::wstring range)
{
	std::wstring filename = ROOTDIR + ticker + L".ohlc";
	bool exists = FileExists(filename.c_str()); // Check before create file
	FileIO ohlcFile;
	ohlcFile.Init(filename);
	ohlcFile.Open();

	if (exists)
	{
		std::vector<OHLC> ohlcData = ohlcFile.Read<OHLC>();
		uint64_t lastTime = ohlcData.at(ohlcData.size()-1).time;


	}

	return std::vector<OHLC>();
}

Quote GetQuote(std::wstring ticker)
{
	std::string json = SendHTTPSRequest_GET(IEXHOST, L"1.0/stock/aapl/quote", 
		L"filter=open,close,latestPrice,latestSource,latestUpdate,latestVolume,avgTotalVolume");

	Quote quote;
	char buffer[50] = { 0 };

	std::string format = R"({"open":%lf,"close":%lf,"latestPrice":%lf,"latestSource":"%49[^"]","latestUpdate":%I64u,)";
	format += R"("latestVolume":%d,"avgTotalVolume":%d})";

	int n = sscanf_s(json.c_str(), format.c_str(), &quote.open, &quote.close, &quote.latestPrice, buffer, 
		static_cast<unsigned>(_countof(buffer)), &quote.latestUpdate, &quote.latestVolume, &quote.avgTotalVolume);
	if (n != 7)
	{
		OutputMessage(L"sscanf_s failed! Only read %d/%d\n", n, 7);
		//OutputDebugString(std::to_wstring(quote.latestUpdate).c_str()); OutputDebugString(L"\n");
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

	return quote;
}


