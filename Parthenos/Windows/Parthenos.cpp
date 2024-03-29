// Parthenos.cpp : Defines the entry point for the application.

#include "../stdafx.h"
#include "Parthenos.h"
#include "ConfirmationWindow.h"
#include "AddTransactionWindow.h"
#include "EditTransactionWindow.h"
#include "../Utilities/HTTP.h"
#include "../Utilities/DataManaging.h"
#include "../Utilities/FileIO.h"
#include "../Utilities/utilities.h"
#include "../Utilities/jsonette.h"

using namespace jsonette;

///////////////////////////////////////////////////////////
// --- Forward declarations ---

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


///////////////////////////////////////////////////////////
// --- Interface functions ---


Parthenos::Parthenos(PCWSTR szClassName) : 
	BorderlessWindow(szClassName)
{	
	//std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));
	//trans.resize(trans.size() - 1);
	//::writeTransactions(ROOTDIR + L"hist.trans", trans);

	//CalculateHoldings();

	//FileIO ohlcFile;
	//ohlcFile.Init(ROOTDIR + L"spy.iex.ohlc");
	//ohlcFile.Open();
	//std::vector<OHLC> ohlcData = ohlcFile.Read<OHLC>();
	//ohlcFile.Write(ohlcData.data(), sizeof(OHLC) * (ohlcData.size() - 5));
	//ohlcFile.Close();

	//std::vector<std::wstring> accts = { L"Robinhood", L"Arista", L"TD Ameritrade" };
	//for (std::wstring & acc : accts)
	//{
	//	if (FileExists((ROOTDIR + acc + L".hist").c_str()))
	//	{
	//		FileIO histFile;
	//		histFile.Init(ROOTDIR + acc + L".hist");
	//		histFile.Open();
	//		std::vector<TimeSeries> portHist;
	//		portHist = histFile.Read<TimeSeries>();
	//		portHist.erase(portHist.end() - 10);
	//		histFile.Write(portHist.data(), portHist.size() * sizeof(TimeSeries));
	//		histFile.Close();
	//	}
	//}
}

Parthenos::~Parthenos()
{
	for (auto item : m_allItems)
		if (item) delete item;
	for (PopupWindow *cwin : m_childWindows) if (cwin) delete cwin;

	for (int i = 1; i < Timers::n_timers + 1; i++)
	{
		if (m_timers.nActiveP1[i] == 1)
		{
			BOOL err = ::KillTimer(m_hwnd, i);
			if (err == 0) OutputError(L"Kill timer failed");
			m_timers.nActiveP1[i] = 0;
		}
	}
}

LRESULT Parthenos::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = 0;
	if (BorderlessWindow<Parthenos>::handle_message(uMsg, wParam, lParam, ret)) {
		if (ret == HTCLIENT && uMsg == WM_NCHITTEST) {
			// BorderlessWindow only handles resizing, catch everything else here
			ret = OnNCHitTest(POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		}
		return ret;
	}

	switch (uMsg)
	{
	case WM_CREATE:
		return OnCreate();
	case WM_COMMAND: // Command items from application menu; accelerators
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(m_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), m_hwnd, About);
			break;
		case IDA_COPY:
			OnCopy();
			break;
		default:
			return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
		}
		return 0;
	}
	//case WM_CLOSE: {
	//	if (MessageBox(m_hwnd, L"Really quit?", L"Parthenos", MB_OKCANCEL) == IDOK)
	//	{
	//		::DestroyWindow(m_hwnd);
	//	}
	//	return 0;
	//}
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
		lpMMI->ptMinTrackSize.x = 600;
		lpMMI->ptMinTrackSize.y = 600;
		return 0;
	}
	case WM_DESTROY:
		m_d2.DiscardGraphicsResources();
		m_d2.DiscardLifetimeResources();
		CoUninitialize();
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		return OnPaint();
	case WM_SIZE:
		return OnSize(wParam);
	case WM_MOUSEMOVE:
		return OnMouseMove(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_MOUSELEAVE:
	{
		// This message seems to be sent when screen turns off, even when m_bMouseTracking == false.
		// OnMouseMove() begins tracking, so don't call it or else infinite loop.
		D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(POINT{ -1, -1 });
		if (m_mouseCaptured)
		{
			m_mouseCaptured->OnMouseMove(dipCursor, wParam, false);
		}
		else
		{
			bool handeled = false;
			for (auto item : m_activeItems)
			{
				handeled = item->OnMouseMove(dipCursor, wParam, handeled) || handeled;
			}
		}
		m_mouseTrack.Reset(m_hwnd);
		return 0;
	}
	case WM_MOUSEHOVER:
		// TODO: Handle the mouse-hover message.
		m_mouseTrack.Reset(m_hwnd);
		return 0;
	case WM_MOUSEWHEEL:
		return OnMouseWheel(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			// Handled in WM_MOUSEMOVE
			return TRUE;
		}
		break;
	case WM_LBUTTONDOWN:
		return OnLButtonDown(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_LBUTTONDBLCLK:
		return OnLButtonDblclk(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_LBUTTONUP:
		return OnLButtonUp(
			POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			wParam
		);
	case WM_CHAR:
		return OnChar(static_cast<wchar_t>(wParam), lParam);
	case WM_KEYDOWN:
		if (OnKeyDown(wParam, lParam)) return 0;
		break; // pass on to DefWindowProc to process WM_CHAR messages...?
	case WM_TIMER:
		return OnTimer(wParam, lParam);
	case WM_APP:
		ProcessCTPMessages();
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void Parthenos::PreShow()
{
	// Calculate size-dependent layouts
	RECT rc;
	if (!GetClientRect(m_hwnd, &rc)) OutputError(L"GetClientRect failed");
	D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

	// Initialize AppItem size 
	// (WM_SIZE is received before this, but need to initialize size of inactive items too).
	for (auto item : m_allItems)
		item->SetSize(CalculateItemRect(item, dipRect));

	try {
		// Calculate positions, get stats, etc.
		InitRealtimeData();

		// Data dependent initializations
		InitItemsWithRealtimeData();
	}
	catch (const std::exception & e) {
		std::wstring out = SPrintException(e);
		if (m_msgBox) m_msgBox->Print(out);
		else OutputDebugString(out.c_str());
	}
}


///////////////////////////////////////////////////////////
// --- Data Management ---

void Parthenos::InitData()
{
	// Get account names
	FileIO accountsFile;
	accountsFile.Init(ROOTDIR + L"accounts.json");
	accountsFile.Open(GENERIC_READ);
	JSON json(accountsFile.ReadText());
	accountsFile.Close();

	std::vector<JSON> const & names = json["accounts"].get_arr();
	for (JSON const & name : names)
		m_accountNames.push_back(s2w(name.get_str()));
	m_currAccount = 0;

	TDKEY = ::s2w(json["apiKeys"]["TD"]);

	m_accounts.resize(m_accountNames.size() + 1);
	for (size_t i = 0; i < m_accountNames.size(); i++)
		m_accounts[i].name = m_accountNames[i];
	m_accounts.back().name = L"All Accounts";
}


// Data initializations dependent on realtime data (i.e. requires HTTP GET).
void Parthenos::InitRealtimeData()
{
	std::vector<Holdings> holdings(readHoldings(ROOTDIR + L"holdings.json"));

	m_tickers = GetTickers(holdings); // tickers from all accounts
	m_tickerColors = Colors::Randomizer(m_tickers);
	m_stats = GetBatchQuoteStats(m_tickers);

	CalculatePositions(holdings);
	CalculateReturns();

#ifdef TEST_IEX
	throw ws_exception(L"Testing IEX; breaking!");
	// Test gives garbled data; don't write anything to file
#endif

	CalculateHistories();
}


int Parthenos::AccountToIndex(std::wstring account)
{
	for (size_t i = 0; i < m_accountNames.size(); i++)
	{
		if (m_accountNames[i] == account) return static_cast<int>(i);
	}
	return -1; // also means "All"
}

// Calculates holdings from full transaction history
std::vector<Holdings> Parthenos::CalculateHoldings() const
{
	std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));
	std::vector<Holdings> holdings(FullTransactionsToHoldings(trans));
	::writeHoldings(ROOTDIR + L"holdings.json", holdings);
	return holdings;
}


// Holdings -> Positions
void Parthenos::CalculatePositions(std::vector<Holdings> const & holdings)
{
	date_t date = GetCurrentDate();
	for (size_t i = 0; i < m_accountNames.size(); i++)
	{
		std::vector<Position> positions = HoldingsToPositions(
			holdings, static_cast<char>(i), date, GetMarketPrices(m_stats));
		m_accounts[i].positions = positions;
	}
	std::vector<Position> positions = HoldingsToPositions(
		holdings, -1, date, GetMarketPrices(m_stats)); // all accounts
	m_accounts.back().positions = positions;
}


void Parthenos::CalculateReturns()
{
	for (Account & acc : m_accounts)
	{
		acc.returnsBarData.clear();
		acc.returnsPercBarData.clear();
		if (acc.positions.empty()) continue;
		acc.returnsBarData.reserve(acc.positions.size() - 1); // minus cash
		acc.returnsPercBarData.reserve(acc.positions.size() - 1); // minus cash

		// sort by decreasing equity
		std::vector<size_t> inds = sort_indexes(acc.positions,
			[](Position const & p1, Position const & p2) {return p1.n * p1.marketPrice > p2.n * p2.marketPrice; });

		for (size_t i : inds)
		{
			Position const & p = acc.positions[i];
			if (p.ticker == L"CASH") continue;
			double returns = p.realized + p.dividends + p.unrealized; 
			double cost_basis = p.avgCost * p.n;
			double perc = (cost_basis == 0) ? 0.0 : (p.dividends + p.unrealized) / cost_basis * 100.0; 
			D2D1_COLOR_F color = KeyMatch(m_tickers, m_tickerColors, p.ticker);

			acc.returnsBarData.push_back({ returns, color });
			acc.tickers.push_back(p.ticker);
			if (perc != 0)
			{
				acc.returnsPercBarData.push_back({ perc, color });
				acc.percTickers.push_back(p.ticker);
			}
		}
	}
}


// Calculate historical performance
void Parthenos::CalculateHistories()
{
	for (size_t i = 0; i < m_accountNames.size(); i++)
	{
		Account & acc = m_accounts[i];
		std::vector<TimeSeries> portHist(GetHist(i));

		// Add to Account object for plotting
		acc.histDate.clear();
		acc.histEquity.clear();
		acc.histDate.reserve(portHist.size());
		acc.histEquity.reserve(portHist.size());
		double cash_in_end = GetCash(acc.positions).second;
		for (TimeSeries const & x : portHist)
		{
			acc.histDate.push_back(x.date);
			acc.histEquity.push_back(x.prices + cash_in_end);
		}
	}

	CalculateAllHistory();
}

// Calculate equity history for "All Accounts" and update m_accounts.
void Parthenos::CalculateAllHistory()
{
	Account & accAll = m_accounts.back();

	// Find account with longest history, and total cash in.
	std::vector<double> cashIn(m_accounts.size() - 1);
	size_t maxSize = 0;
	size_t iAccMax = 0; // index of account with maximum history length
	for (size_t i = 0; i < m_accounts.size() - 1; i++)
	{
		cashIn[i] = GetCash(m_accounts[i].positions).second;
		if (m_accounts[i].histDate.size() > maxSize)
		{
			maxSize = m_accounts[i].histDate.size();
			iAccMax = i;
		}
	}
	double totCashIn = std::accumulate(cashIn.begin(), cashIn.end(), 0.0);

	// Clear and reserve
	accAll.histDate.clear();		accAll.histDate.reserve(maxSize);
	accAll.histEquity.clear();		accAll.histEquity.reserve(maxSize);

	// Fill the equity vectors
	std::vector<size_t> iAccDate(m_accounts.size() - 1); // index into each account's histDate
	while (iAccDate[iAccMax] < m_accounts[iAccMax].histDate.size())
	{
		date_t date = m_accounts[iAccMax].histDate[iAccDate[iAccMax]];
		double val = 0;
		for (size_t iAcc = 0; iAcc < m_accounts.size() - 1; iAcc++)
		{
			Account & acc = m_accounts[iAcc];
			size_t iDate = iAccDate[iAcc];
			if (iDate < acc.histDate.size() && acc.histDate[iDate] == date)
			{
				val += acc.histEquity[iDate] - cashIn[iAcc];
				iAccDate[iAcc]++;
			}
		}
		accAll.histDate.push_back(date);
		accAll.histEquity.push_back(val + totCashIn);
	}
}

// Reads account price history from file, or calculates full history if file DNE (and writes back).
std::vector<TimeSeries> Parthenos::GetHist(size_t i)
{
	std::vector<TimeSeries> portHist; // output

	Account & acc = m_accounts[i];
	bool exists = FileExists((ROOTDIR + acc.name + L".hist").c_str());

	FileIO histFile;
	histFile.Init(ROOTDIR + acc.name + L".hist");
	histFile.Open();

	if (!exists) // Calculate full history
	{
		// Read transaction history
		std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));

		// Calculate full equity history
		portHist = CalculateFullEquityHistory(static_cast<char>(i), trans, m_stats);

		// Write to file
		histFile.Write(portHist.data(), portHist.size() * sizeof(TimeSeries));
	}
	else // assumes that holdings haven't changed since last update (add transaction will properly invalidate history)
	{
		portHist = histFile.Read<TimeSeries>();
		std::vector<Holdings> holdings(::readHoldings(ROOTDIR + L"holdings.json"));
		try { // non-critical fail here - just show not up-to-date history
			UpdateEquityHistory(portHist, i, holdings, m_stats);
			histFile.Write(portHist.data(), portHist.size() * sizeof(TimeSeries));
		}
		catch (const std::exception & e) {
			if (m_msgBox) m_msgBox->Print(SPrintException(e));
		}
	}
	histFile.Close();

	return portHist;
}


void Parthenos::AddTransaction(Transaction t)
{
	// Update transaction history
	std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));
	trans.push_back(t);
	::writeTransactions(ROOTDIR + L"hist.trans", trans);

	// Update holdings
	std::vector<Holdings> holdings(::readHoldings(ROOTDIR + L"holdings.json"));
	AddTransactionToHoldings(holdings, t);
	::writeHoldings(ROOTDIR + L"holdings.json", holdings);

	try { // non-critical fail here - just show not up-to-date history

		// Update data
		m_tickers = GetTickers(holdings);
		m_tickerColors = Colors::Randomizer(m_tickers);
		m_stats = GetBatchQuoteStats(m_tickers);
		m_accounts[t.account].positions = HoldingsToPositions(holdings, t.account, GetCurrentDate(), GetMarketPrices(m_stats));
		m_accounts.back().positions = HoldingsToPositions(holdings, -1, GetCurrentDate(), GetMarketPrices(m_stats)); // all accounts

		// Read equity history
		FileIO histFile;
		histFile.Init(ROOTDIR + m_accountNames[t.account] + L".hist");
		histFile.Open();
		std::vector<TimeSeries> portHist;
		portHist = histFile.Read<TimeSeries>();

		// Update equity history
		auto it = std::lower_bound(portHist.begin(), portHist.end(), t.date,
			[](TimeSeries const & ts, date_t date) {return ts.date < date; }
		);
		portHist.erase(it, portHist.end());

		UpdateEquityHistory(portHist, t.account, holdings, m_stats);
		histFile.Write(portHist.data(), portHist.size() * sizeof(TimeSeries));
		histFile.Close();

		// Update hist data (TODO update all history too?)
		m_accounts[t.account].histDate.clear();
		m_accounts[t.account].histEquity.clear();
		double cash_in = GetCash(m_accounts[t.account].positions).second;
		for (auto const & x : portHist)
		{
			m_accounts[t.account].histDate.push_back(x.date);
			m_accounts[t.account].histEquity.push_back(x.prices + cash_in);
		}

		// Update plots
		if (m_currAccount == t.account || m_currAccount == m_accounts.size() - 1)
			UpdatePortfolioPlotters(m_currAccount);
	}
	catch (const std::exception & e) {
		if (m_msgBox) m_msgBox->Print(SPrintException(e));
	}


}

// Prints dividend summary for the current account collected by year
void Parthenos::PrintDividendSummary() const
{
	Account const& acc = m_accounts[m_currAccount];
	std::vector<Transaction> trans = GetTrans();
	int yrStart = GetYear(trans.front().date);
	int yrEnd = GetYear(trans.back().date);
	int nyr = yrEnd - yrStart + 1;

	// Dividends by ticker and year (value size nyr + 1, back() is total).
	std::map<std::wstring, std::vector<double>> dividends;
	for (Transaction const& t : trans)
	{
		if (t.type == TransactionType::Dividend 
			&& (m_currAccount == m_accounts.size() - 1 || t.account == m_currAccount))
		{
			if (dividends[t.ticker].empty()) dividends[t.ticker].resize(nyr + 1);
			dividends[t.ticker][GetYear(t.date) - yrStart] += t.value;
			dividends[t.ticker].back() += t.value;
		}
	}

	// Column Headers
	std::wstringstream ss;
	ss << std::fixed << std::setprecision(2);
	ss << L"\tAnn Div\tEff Yield\t";
	for (int i = yrStart; i <= yrEnd; i++) ss << i << L"\t";
	ss << L"Total\n";

	// Print Table
	double ann_div = 0;
	double total_cost = 0;
	std::vector<double> total_div(nyr + 1); // back() is all-time total
	for (Position const &p : acc.positions)
	{
		if (p.ticker == L"CASH" ) continue;
		ss << p.ticker << L"\t";

		// Print yield
		std::pair<Quote, Stats> stats = KeyMatch(m_tickers, m_stats, p.ticker);
		if (p.n != 0)
		{
			ann_div += stats.second.dividendRate * p.n;
			total_cost += p.avgCost * p.n;
			ss << stats.second.dividendRate * p.n << L"\t";
			ss << stats.second.dividendRate / p.avgCost * 100.0 << L"\t";
		}
		else
		{
			ss << "-\t-\t";
		}

		// Print yearly totals
		if (dividends.count(p.ticker))
		{
			for (int i = 0; i < nyr + 1; i++)
			{
				double x = dividends[p.ticker][i];
				if (x > 0.009) ss << x;
				else ss << L"-";
				total_div[i] += x;
				ss << L"\t";
			}
		}
		else for (int i = 0; i < nyr + 1; i++) ss << L"-\t";
		ss << L"\n";
	}

	// Print totals
	ss << L"Total\t";
	ss << ann_div << L"\t";
	ss << ann_div / total_cost * 100.0 << L"\t";
	for (double x : total_div)
		ss << x << L"\t";
	ss << L"\n";
	m_msgBox->Overwrite(ss.str());
}


// For importing into excel
void Parthenos::PrintOptionSummary() const
{
	m_msgBox->Print(L"Deprecated");

	//// Get plays
	//std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"trans.json"));
	//std::vector<Play> plays(GetOptionPerformance(trans));

	//// Get range of months
	//date_t start = plays.front().start;
	//date_t today = GetCurrentDate();
	//std::vector<date_t> dates = MakeMonthRange(start, today);

	//// Collect the returns
	//std::map<std::wstring, std::vector<double>> returns;
	//for (Play const& p : plays)
	//{
	//	if (!isShort(p.type)) continue; // just look at short options for now
	//	std::vector<double>& ticker_returns = returns[p.ticker];
	//	if (ticker_returns.empty()) ticker_returns.resize(dates.size());

	//	size_t offset = MonthDiff(p.start, start);
	//	size_t nMonths = 1ull + MonthDiff((p.end > 0) ? p.end : p.expiration, p.start);
	//	double pl = (p.price_enter - p.price_exit) * 100 * p.n;
	//	for (size_t i = offset; i < offset + nMonths && i < dates.size(); i++)
	//		ticker_returns[i] += pl / nMonths;
	//	// Ignore assignment cost here
	//}

	//// Print out
	//std::wstringstream ss;
	//for (auto& t_r : returns) ss << L'\t' << t_r.first;
	//ss << L'\n';
	//for (size_t i = 0; i < dates.size(); i++)
	//{
	//	ss << DateToWString(dates[i]);
	//	for (auto & t_r : returns)
	//		ss << L'\t' << t_r.second[i];
	//	ss << L'\n';
	//}

	//m_msgBox->Print(ss.str());
}


std::vector<Transaction> Parthenos::GetTrans() const
{
	std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));
	if (trans.empty()) throw ws_exception(L"Empty transactions");
	return trans;
}


///////////////////////////////////////////////////////////
// --- Child Management ---

// Size independent initializations
void Parthenos::InitItems()
{
	// Title bar
	m_titleBar->SetTabs({ L"Portfolio", L"Returns", L"Chart" });
	m_titleBar->SetActive(L"Chart");

	// Menu bar
	std::vector<std::wstring> menus = { L"File", L"Account", L"Transactions", L"Stocks" };
	std::vector<std::vector<std::wstring>> items = {
		{ L"Print Transaction History", L"Print Holdings", L"Print Equity History", L"Update Last Equity History Entry" },
		m_accountNames, // Add "All Accounts" below
		{ L"Add", L"Edit", L"Save to JSON", L"Recalculate All" },
		{ L"Print OHLC", L"Delete Last OHLC Entry", L"Print Dividend Summary", L"Print Option Summary" }
	};
	items[1].push_back(L"All Accounts");
	std::vector<std::vector<size_t>> divisions = { {3}, {m_accountNames.size()}, {3}, {} };
	m_menuBar->SetMenus(menus, items, divisions);
	m_menuBar->Refresh();

	// Load transaction history into chart
	std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));
	m_chart->LoadHistory(trans);

	// Watchlists
	std::vector<WatchlistColumn> portColumns = { // m_portfolioListWidth is 720, -15 for ScrollBar::Width
		{60.0f, WatchlistColumn::Ticker, L""},
		{60.0f, WatchlistColumn::Last, L"%.2lf"},
		{60.0f, WatchlistColumn::ChangeP, L"%.2lf"},
		{45.0f, WatchlistColumn::Shares, L"%d"},
		{60.0f, WatchlistColumn::AvgCost, L"%.2lf"},
		{60.0f, WatchlistColumn::Equity, L"%.2lf"},
		{60.0f, WatchlistColumn::ReturnsT, L"%.2lf"},
		{60.0f, WatchlistColumn::ReturnsP, L"%.2lf"},
		{60.0f, WatchlistColumn::Realized, L"%.2lf"},
		{60.0f, WatchlistColumn::APY, L"%.2lf"},
		{60.0f, WatchlistColumn::EarningsDate, L""},
		//{60.0f, WatchlistColumn::EffectiveYield, L"%.2lf"},
		{60.0f, WatchlistColumn::ExDiv, L""},
	};
	std::vector<WatchlistColumn> watchlistColumns = { // m_watchlistWidth = 350.0f;
		{95.0f, WatchlistColumn::Ticker, L""},
		{60.0f, WatchlistColumn::Last, L"%.2lf"},
		{60.0f, WatchlistColumn::ChangeP, L"%.2lf"},
		{60.0f, WatchlistColumn::Change1YP, L"%.2lf"},
		{60.0f, WatchlistColumn::EarningsDate, L""}

	};
	m_watchlist->SetColumns(watchlistColumns); // use defaults
	m_portfolioList->SetColumns(portColumns);

	// Returns plots
	m_returnsAxes->SetXAxisPos(0.0f);
	m_returnsAxes->SetYAxisPos(std::nanf(""));
	m_returnsAxes->SetHoverStyle(Axes::HoverStyle::snap);
	m_returnsPercAxes->SetXAxisPos(0.0f);
	m_returnsPercAxes->SetYAxisPos(std::nanf(""));
	m_returnsPercAxes->SetHoverStyle(Axes::HoverStyle::snap);
	m_eqHistoryAxes->SetYAxisPos(std::nanf(""));
	m_eqHistoryAxes->SetHoverStyle(Axes::HoverStyle::snap);
}

// Item initializations dependent on realtime data (i.e. requires HTTP GET).
void Parthenos::InitItemsWithRealtimeData()
{
	// Set initial watchlist and load portfolio trackers
	m_watchlist->Load(m_tickers, m_accounts.back().positions, m_stats);
	m_watchlist->Refresh();
	if ((size_t)m_currAccount < m_accounts.size())
	{
		//std::vector<std::wstring> tickers = GetTickers(m_accounts[m_currAccount].positions);
		//std::vector<std::pair<Quote, Stats>> stats = FilterByKeyMatch(m_tickers, m_stats, tickers);
		//m_watchlist->Load(tickers, m_accounts[m_currAccount].positions, stats);
		UpdatePortfolioPlotters(m_currAccount, false);
	}

	// Initial chart
	m_chart->Draw(L"VOO");
}

void Parthenos::ProcessCTPMessages()
{
	while (!m_messages.empty())
	{
		ClientMessage & msg = m_messages.front();
		bool pop_front = true; 
		// functions that launch file dialog must delete their CTPMessage before launching
		// flag main thread to not delete again
		
		switch (msg.imsg)
		{
		case CTPMessage::NONE:
			break;
		case CTPMessage::TITLEBAR_CLOSE:
			SendMessage(m_hwnd, WM_CLOSE, 0, 0);
			break;
		case CTPMessage::TITLEBAR_MAXRESTORE:
			if (maximized()) ShowWindow(m_hwnd, SW_RESTORE);
			else ShowWindow(m_hwnd, SW_MAXIMIZE);
			break;
		case CTPMessage::TITLEBAR_MIN:
			ShowWindow(m_hwnd, SW_MINIMIZE);
			break;
		case CTPMessage::TITLEBAR_TAB:
		{
			m_activeItems.clear();
			m_activeItems.push_back(m_titleBar);
			if (msg.msg == L"Portfolio")
			{
				m_activeItems.push_back(m_menuBar);
				m_activeItems.push_back(m_portfolioList);
				m_activeItems.push_back(m_pieChart);
				m_activeItems.push_back(m_msgBox);
				m_tab = TPortfolio;
			}
			else if (msg.msg == L"Returns")
			{
				m_activeItems.push_back(m_menuBar);
				m_activeItems.push_back(m_eqHistoryAxes);
				m_activeItems.push_back(m_returnsAxes);
				m_activeItems.push_back(m_returnsPercAxes);
				m_tab = TReturns;
			}
			else if (msg.msg == L"Chart")
			{
				m_activeItems.push_back(m_chart);
				m_activeItems.push_back(m_watchlist);
				m_tab = TChart;
			}

			RECT rc;
			GetClientRect(m_hwnd, &rc);
			D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);
			if (m_sizeChanged)
			{
				for (auto item : m_allItems)
					item->SetSize(CalculateItemRect(item, dipRect));
				m_sizeChanged = false;
			}
			CalculateDividingLines(dipRect);
			::InvalidateRect(m_hwnd, NULL, false);
			break;
		}
		case CTPMessage::WATCHLIST_SELECTED:
		{
			if (msg.sender != m_watchlist) break; // i.e. don't process messages from other lists
			if (std::find(m_activeItems.begin(), m_activeItems.end(), m_chart) != m_activeItems.end())
			{
				m_chart->Draw(msg.msg);
			}
			break;
		}
		case CTPMessage::MENUBAR_SELECTED:
		{
			ProcessMenuMessage(pop_front);
			break;
		}
		case CTPMessage::WINDOW_CLOSED:
		{
			PopupWindow *win = static_cast<PopupWindow*>(msg.sender);
			if (m_childWindows.erase(win)) delete win;
			break;
		}
		case CTPMessage::WINDOW_ADDTRANSACTION_P:
		{
			Transaction *t = static_cast<Transaction*>(msg.sender);
			if (!t) break;
			AddTransaction(*t);
			delete t;
			break;
		}
		case CTPMessage::MOUSE_CAPTURED: // click and hold
		{
			if (msg.iData < 0) // Release (ReleaseCapture() called in OnLButtonUp)
				m_mouseCaptured = nullptr;
			else // Capture (SetCapture() called in OnLButtonDown)
				m_mouseCaptured = static_cast<AppItem*>(msg.sender);
			break;
		}
		case CTPMessage::PRINT:
		{
			m_msgBox->Print(msg.msg);
			break;
		}
		default:
			OutputMessage(L"Unknown message %d received\n", static_cast<int>(msg.imsg));
			break;
		}
		if (pop_front) m_messages.pop_front();
	}
}

// Processes the first message in m_messages assuming it's a message from the menu bar
//
// @pop_front -- Set to false if ProcessCTPMessages() should not pop from the message queue,
//		i.e. because this function already did the pop.
void Parthenos::ProcessMenuMessage(bool & pop_front)
{
	ClientMessage & msg = m_messages.front();
	if (msg.iData == 0) // File
	{
		if (msg.msg == L"Print Transaction History")
		{
			std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));
			std::wstring out;
			for (Transaction const & t : trans)
				out.append(t.to_wstring(m_accountNames) + L"\n");
			m_msgBox->Overwrite(out);
		}
		else if (msg.msg == L"Print Holdings")
		{
			m_msgBox->Clear();
			std::vector<Holdings> holdings(::readHoldings(ROOTDIR + L"holdings.json"));
			for (Holdings const & h : holdings)
				m_msgBox->Print(h.to_wstring());
		}
		else if (msg.msg == L"Print Equity History")
		{
			std::wstring out;
			Account & acc = m_accounts[m_currAccount];
			for (size_t i = 0; i < acc.histDate.size(); i++)
				out.append(FormatMsg(L"%s: %s\n", DateToWString(acc.histDate[i]).c_str(), FormatDollar(acc.histEquity[i]).c_str()));
			m_msgBox->Overwrite(out);
		}
		else if (msg.msg == L"Update Last Equity History Entry")
		{
			Account & acc = m_accounts[m_currAccount];
			if (FileExists((ROOTDIR + acc.name + L".hist").c_str()))
			{
				FileIO histFile;
				histFile.Init(ROOTDIR + acc.name + L".hist");
				histFile.Open();
				std::vector<TimeSeries> portHist;
				portHist = histFile.Read<TimeSeries>();
				portHist.erase(portHist.end() - 1);
				histFile.Write(portHist.data(), portHist.size() * sizeof(TimeSeries));
				histFile.Close();
			}
			CalculateHistories();
			UpdatePortfolioPlotters(m_currAccount);
		}
	}
	else if (msg.iData == 1) // Account
	{
		int account = AccountToIndex(msg.msg);
		if (account < 0) account = static_cast<int>(m_accounts.size()) - 1; // last entry is 'all'
		if (account != m_currAccount)
		{
			m_currAccount = account;
			UpdatePortfolioPlotters(account);
		}
	}
	else if (msg.iData == 2) // Transactions
	{
		if (msg.msg == L"Add")
		{
			AddTransactionWindow *addTWin = new AddTransactionWindow();
			BOOL ok = addTWin->Create(m_hInstance);
			if (ok)
			{
				m_childWindows.insert(addTWin);
				addTWin->SetParent(this, m_hwnd);
				addTWin->SetAccounts(m_accountNames);
				addTWin->PreShow();
				ShowWindow(addTWin->Window(), SW_SHOW);
			}
			else
			{
				delete addTWin;
				OutputError(L"Create AddTransaction window failed");
			}
		}
		else if (msg.msg == L"Edit")
		{
			EditTransactionWindow *editTWin = new EditTransactionWindow();
			BOOL ok = editTWin->Create(m_hInstance);
			if (ok)
			{
				m_childWindows.insert(editTWin);
				editTWin->SetParent(this, m_hwnd);
				editTWin->SetAccounts(m_accountNames);
				editTWin->SetFilepath(ROOTDIR + L"hist.trans");
				editTWin->PreShow();
				ShowWindow(editTWin->Window(), SW_SHOW);
			}
			else
			{
				delete editTWin;
				OutputError(L"Create AddTransaction window failed");
			}
		}
		else if (msg.msg == L"Save to JSON")
		{
			std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));
			::writeTransactions_JSON(ROOTDIR + L"trans.json", trans);
			m_msgBox->Print(L"Transactions saved to " + ROOTDIR + L"trans.json\n");
		}
		else if (msg.msg == L"Recalculate All")
		{
			if (msg.sender) // sent by menubar -> ask confirmation
			{
				ConfirmationWindow *okWin = new ConfirmationWindow();
				BOOL ok = okWin->Create(m_hInstance);
				if (ok)
				{
					okWin->SetParent(this, m_hwnd);
					okWin->SetText(L"Recalculate holdings from full transaction history?");
					msg.sender = nullptr;
					okWin->SetMessage(msg);
					okWin->PreShow();
					ShowWindow(okWin->Window(), SW_SHOW);
				}
				else
				{
					delete okWin;
					OutputError(L"Create AddTransaction window failed");
				}
			}
			else if (!msg.sender) // sent from confirmation window
			{
				try 
				{
					m_msgBox->Print(L"Recalculating holdings...\n");
					std::vector<Holdings> holdings = CalculateHoldings();
					CalculatePositions(holdings);
					m_msgBox->Print(L"Done.\n");
				}
				catch (const ws_exception& e) 
				{
					m_msgBox->Print(e.ws_what());
				}
			}
		}
	}
	else if (msg.iData == 3) // Stocks
	{
		if (msg.msg == L"Print OHLC")
		{
			m_messages.pop_front(); // delete this message since file dialog blocks
			pop_front = false; // tell ProcessCTPMessages not to pop front again
			std::wstring file = LaunchFileDialog();
			if (!file.empty())
			{
				FileIO ohlcFile;
				ohlcFile.Init(file);
				ohlcFile.Open(GENERIC_READ);
				std::vector<OHLC> ohlcData = ohlcFile.ReadEnd<OHLC>(100); // Get last 100 for now

				std::wstring out;
				for (OHLC const& x : ohlcData) out.append(x.to_wstring());
				if (m_msgBox) m_msgBox->Overwrite(out);
			}
		}
		else if (msg.msg == L"Delete Last OHLC Entry")
		{
			pop_front = false;
			m_messages.pop_front(); // delete this message since file dialog blocks
			std::wstring file = LaunchFileDialog();
			if (!file.empty())
			{
				FileIO ohlcFile;
				ohlcFile.Init(file);
				ohlcFile.Open();
				std::vector<OHLC> ohlcData = ohlcFile.Read<OHLC>(); // Get last 100 for now
				ohlcData.erase(ohlcData.end() - 1);
				ohlcFile.Write(ohlcData.data(), ohlcData.size() * sizeof(OHLC));

				std::wstring out;
				for (OHLC const& x : ohlcData) out.append(x.to_wstring());
				if (m_msgBox) m_msgBox->Overwrite(out);
			}
		}
		else if (msg.msg == L"Print Dividend Summary")
		{
			PrintDividendSummary();
		}
		else if (msg.msg == L"Print Option Summary")
		{
			PrintOptionSummary();
		}
	}
}

D2D1_RECT_F Parthenos::CalculateItemRect(AppItem * item, D2D1_RECT_F const & dipRect)
{
	if (item == m_titleBar)
	{
		return D2D1::RectF(
			0.0f,
			0.0f,
			dipRect.right,
			m_titleBarBottom
		);
	}
	else if (item == m_chart)
	{
		return D2D1::RectF(
			m_watchlistRight,
			m_titleBarBottom,
			dipRect.right,
			dipRect.bottom
		);
	}
	else if (item == m_watchlist)
	{
		return D2D1::RectF(
			0.0f,
			m_titleBarBottom,
			m_watchlistRight,
			dipRect.bottom
		);
	}
	else if (item == m_portfolioList)
	{
		return D2D1::RectF(
			0.0f,
			m_menuBarBottom,
			m_portfolioListRight,
			m_halfBelowMenu
		);
	}
	else if (item == m_menuBar)
	{
		return D2D1::RectF(
			0.0f,
			m_titleBarBottom,
			dipRect.right,
			m_menuBarBottom
		);
	}
	else if (item == m_pieChart)
	{
		return D2D1::RectF(
			m_portfolioListRight,
			m_menuBarBottom,
			dipRect.right,
			dipRect.bottom
		);
	}
	else if (item == m_eqHistoryAxes)
	{
		return D2D1::RectF(
			dipRect.left,
			m_menuBarBottom,
			m_centerX,
			dipRect.bottom
		);
	}
	else if (item == m_returnsAxes)
	{
		return D2D1::RectF(
			m_centerX,
			m_menuBarBottom,
			dipRect.right,
			m_halfBelowMenu
		);
	}
	else if (item == m_returnsPercAxes)
	{
		return D2D1::RectF(
			m_centerX,
			m_halfBelowMenu,
			dipRect.right,
			dipRect.bottom
		);
	}
	else if (item == m_msgBox)
	{
		return D2D1::RectF(
			0.0f,
			m_halfBelowMenu,
			m_portfolioListRight,
			dipRect.bottom
		);
	}
	else
	{
		OutputMessage(L"CalculateItemRect: unknown item\n");
		return D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

void Parthenos::CalculateDividingLines(D2D1_RECT_F dipRect)
{
	m_dividingLines.clear();
	switch (m_tab)
	{
	case TPortfolio:
		m_dividingLines.push_back({
			D2D1::Point2F(m_portfolioListRight - DPIScale::hpx(), m_menuBarBottom),
			D2D1::Point2F(m_portfolioListRight - DPIScale::hpx(), dipRect.bottom)
			});
		m_dividingLines.push_back({
			D2D1::Point2F(dipRect.left, m_halfBelowMenu - DPIScale::hpy()),
			D2D1::Point2F(m_portfolioListRight, m_halfBelowMenu - DPIScale::hpy())
		});
		break;
	case TReturns:
	{
		m_dividingLines.push_back({
			D2D1::Point2F(m_centerX - DPIScale::hpx(), m_menuBarBottom),
			D2D1::Point2F(m_centerX - DPIScale::hpx(), dipRect.bottom)
		});
		break;
	}
	case TChart:
		m_dividingLines.push_back({
			D2D1::Point2F(m_watchlistRight - DPIScale::hpx(), m_titleBarBottom),
			D2D1::Point2F(m_watchlistRight - DPIScale::hpx(), dipRect.bottom)
			});
		break;
	}
}

inline std::wstring MakeLongLabel(std::wstring const & ticker, double val, double percent)
{
	wchar_t buffer[100];
	swprintf_s(buffer, _countof(buffer), L"%s\n%s, %.2lf%%", ticker.c_str(), FormatDollar(val).c_str(), percent);
	return std::wstring(buffer);
}

// Get data, colors, etc. then sort into inputs to pie chart
void Parthenos::LoadPieChart()
{
	// First get list of tickers and equities of all long positions.
	// Also, store collateral information to plot sliders.

	struct Collat
	{
		size_t i; // index into tickers
		std::wstring name;
		double value;
		D2D1_COLOR_F color;
	} temp_collat;

	double cash = 0.0;
	std::vector<std::wstring> tickers;
	std::vector<double> equities;
	std::vector<D2D1_COLOR_F> colors;
	std::vector<Collat> collaterals;

	Account const & acc = m_accounts[m_currAccount];
	for (Position const& p : acc.positions)
	{
		if (p.ticker == L"CASH")
		{
			cash = p.avgCost + p.realized + p.cashEffect;
			continue;
		}

		tickers.push_back(p.ticker);
		equities.push_back(p.n * p.marketPrice);
		colors.push_back(KeyMatch(m_tickers, m_tickerColors, p.ticker));

		for (OptionPosition const& opt : p.options)
		{
			switch (opt.type)
			{
			case OptionType::PCS:
			case OptionType::CSP:
			{
				temp_collat.name = p.ticker + L"-" + opt.to_wstring();
				temp_collat.i = -1;
				temp_collat.value = opt.cash_collateral;
				temp_collat.color = KeyMatch(m_tickers, m_tickerColors, p.ticker);
				collaterals.push_back(temp_collat);
				break;
			}
			case OptionType::CC:
			{
				temp_collat.name = p.ticker + L"-" + opt.to_wstring();
				temp_collat.i = ::GetIndex(tickers, p.ticker);
				temp_collat.value = p.marketPrice * opt.shares;
				temp_collat.color = Colors::RED;
				collaterals.push_back(temp_collat);
				break;
			}
			case OptionType::CDS:
			{
				// WARNING relies on strict ordering of long position before CDS position
				unsigned i = 0;
				for (i; i < tickers.size(); i++)
					if (tickers[i].find(p.ticker + L"-C") != std::wstring::npos) break; // TODO not matching expirations or strikes at all
				if (i == tickers.size()) throw ws_exception(L"LoadPieChart() couldn't find matching long position for a CDS");

				temp_collat.name = p.ticker + L"-" + opt.to_wstring();
				temp_collat.i = i;
				temp_collat.value = equities[i]; // TODO different n in long and short position
				temp_collat.color = Colors::RED;
				collaterals.push_back(temp_collat);
				break;
			}
			case OptionType::LC:
			case OptionType::LP:
			{
				tickers.push_back(p.ticker + L"-" + opt.to_wstring());
				equities.push_back(static_cast<double>(opt.price) * opt.shares + GetIntrinsicValue(opt, p.marketPrice));
				colors.push_back(Colors::Randomizer(tickers.back()));
				break;
			}
			}
		}
	}


	// Get pie total
	double sum = std::accumulate(equities.begin(), equities.end(), 0.0);
	sum += cash;

	// Pie inputs
	std::vector<double> data;					data.reserve(tickers.size() + 1); // Add one for cash
	std::vector<std::wstring> short_labels;		short_labels.reserve(tickers.size() + 1);
	std::vector<std::wstring> long_labels;		long_labels.reserve(tickers.size() + 2); // Add another for default label
	std::vector<D2D1_COLOR_F> wedge_colors;		wedge_colors.reserve(tickers.size() + 1);

	// Slider inputs
	std::vector<size_t> slider_pos;
	std::vector<double> slider_vals;
	std::vector<std::wstring> slider_labels;
	std::vector<D2D1_COLOR_F> slider_colors;

	// Default long label
	double equity = ::getLiquidatingValue(m_accounts[m_currAccount].positions);
	wchar_t buffer[100];
	swprintf_s(buffer, _countof(buffer), L"%s\n%s", acc.name.c_str(), FormatDollar(equity).c_str());
	long_labels.push_back(std::wstring(buffer));

	// Sort by market value
	std::vector<size_t> inds = sort_indexes(equities, std::greater<>{});
	for (size_t i = 0; i < equities.size(); i++)
	{
		if (equities[inds[i]] <= 0.01) break;
		double value = equities[inds[i]];
		std::wstring ticker = tickers[inds[i]];

		data.push_back(value);
		short_labels.push_back(ticker);
		long_labels.push_back(MakeLongLabel(ticker, value, 100.0 * value / sum));
		wedge_colors.push_back(colors[inds[i]]);
	}

	// Sliders
	std::sort(collaterals.begin(), collaterals.end(),
		[](Collat const & c1, Collat const & c2) {return c1.i < c2.i; });
	for (Collat const & x : collaterals)
	{
		if (x.i == -1) slider_pos.push_back(data.size()); // cash collateral
		else slider_pos.push_back(GetIndex(inds, x.i));
		slider_vals.push_back(x.value);
		slider_labels.push_back(MakeLongLabel(x.name, x.value, 100.0 * x.value / sum));
		slider_colors.push_back(x.color);
	}

	// Cash
	data.push_back(cash);
	short_labels.push_back(L"CASH");
	long_labels.push_back(MakeLongLabel(L"CASH", cash, 100.0 * cash / sum));
	wedge_colors.push_back(Colors::GREEN);

	m_pieChart->Load(data, wedge_colors, short_labels, long_labels);
	m_pieChart->LoadSliders(slider_pos, slider_vals, slider_colors, slider_labels);
}

// Updates all items that track the current account portfolio. 
// i.e. call this when the account changes or a transaction is added.
void Parthenos::UpdatePortfolioPlotters(char account, bool init)
{
	if (m_currAccount != account) return;
	Account const & acc = m_accounts[account];
	std::vector<Transaction> trans(::readTransactions(ROOTDIR + L"hist.trans"));
	std::vector<std::wstring> tickers = GetTickers(acc.positions);
	std::pair<double, double> cash = GetCash(acc.positions);

	// Portfolio list
	m_portfolioList->Load(tickers, acc.positions, FilterByKeyMatch(m_tickers, m_stats, tickers));
	if (!init) m_portfolioList->Refresh();

	// Pie chart
	LoadPieChart();
	if (!init) m_pieChart->Refresh();

	// Equity history panel
	wchar_t buffer[100];
	double returns = acc.histEquity.back() - cash.second;
	int acc_all = (account == m_accounts.size() - 1 ? -1 : account);
	double returns_annualized = returns / ::getCashWeight(acc_all, trans, acc.histDate.back()) * 100;
	swprintf_s(buffer, _countof(buffer), L"%s: %s (%.2lf%%, %.2lf%% normalized, %.2lf%% APY)", acc.name.c_str(), 
		FormatDollar(returns).c_str(), returns / cash.second * 100.0, 
		returns_annualized * ::DateDiff(acc.histDate.back(), acc.histDate.front()), returns_annualized * 365);
	m_eqHistoryAxes->SetTitle(buffer);
	m_eqHistoryAxes->SetXAxisPos((float)cash.second);
	m_eqHistoryAxes->Clear();
	m_eqHistoryAxes->Line(acc.histDate, acc.histEquity);

	// Returns and return % graphs
	m_returnsAxes->Clear();
	m_returnsPercAxes->Clear();
	m_returnsAxes->SetXLabels(acc.tickers, false);
	m_returnsPercAxes->SetXLabels(acc.percTickers, false);
	m_returnsAxes->Bar(acc.returnsBarData, acc.tickers);
	m_returnsPercAxes->Bar(acc.returnsPercBarData, acc.percTickers);

	::InvalidateRect(m_hwnd, NULL, FALSE);
}

// WARNING: The Show() method "blocks" but the file dialog window itself
// runs a message loop which will dispatch messages back to the main window. 
// Make sure calling function is in a state where it is ready to exit the 
// current message handling.
std::wstring Parthenos::LaunchFileDialog(bool save)
{
	std::wstring out;
	CComPtr<IFileOpenDialog> pFileOpen;

	// Create the FileOpenDialog object.
	HRESULT hr = pFileOpen.CoCreateInstance(__uuidof(FileOpenDialog));
	if (SUCCEEDED(hr))
	{
		// Show the Open dialog box.
		hr = pFileOpen->Show(NULL);

		// Get the file name from the dialog box.
		if (SUCCEEDED(hr))
		{
			CComPtr<IShellItem> pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				// Display the file name to the user.
				if (SUCCEEDED(hr))
				{
					out = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
			}
			// pItem goes out of scope.
		}
		// pFileOpen goes out of scope.
	}

	return out;
}


///////////////////////////////////////////////////////////
// --- Message handling functions ---

LRESULT Parthenos::OnCreate()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr))
	{
		hr = m_d2.CreateLifetimeResources(m_hwnd);
		m_d2.hwndParent = m_hwnd;
	}
	if (FAILED(hr))
	{
		OutputHRerr(hr, L"OnCreate failed!");
		return -1;  // Fail CreateWindowEx.
	}

	// Add timer object to global
	Timers::WndTimersMap.insert({ m_hwnd, &m_timers });

	// Register 'child' windows
	// Only need to register once. All popups use the same message handler that splits via virtuals
	WndCreateArgs args;
	args.hInstance = m_hInstance;
	args.classStyle = CS_DBLCLKS;
	args.hbrBackground = CreateSolidBrush(RGB(0, 0, 1));
	args.hIcon = LoadIcon(args.hInstance, MAKEINTRESOURCE(IDI_PARTHENOS));
	args.hIconSm = LoadIcon(args.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	ConfirmationWindow *temp_window = new ConfirmationWindow(); // use a ConfirmationWindow as a temp
	BOOL ok = temp_window->Register(args);
	if (!ok) OutputError(L"Register Popup failed");
	delete temp_window;

	// Calculate layouts
	m_titleBarBottom = DPIScale::SnapToPixelY(m_titleBarHeight);
	m_menuBarBottom = DPIScale::SnapToPixelY(m_titleBarHeight + m_menuBarHeight);
	m_watchlistRight = DPIScale::SnapToPixelX(m_watchlistWidth);
	m_portfolioListRight = DPIScale::SnapToPixelX(m_portfolioListWidth);

	// Create items
	m_titleBar = new TitleBar(m_hwnd, m_d2, this);
	m_chart = new Chart(m_hwnd, m_d2, this);
	m_watchlist = new Watchlist(m_hwnd, m_d2, this, true);
	m_portfolioList = new Watchlist(m_hwnd, m_d2, this, false);
	m_menuBar = new MenuBar(m_hwnd, m_d2, this, m_menuBarBottom - m_titleBarBottom);
	m_pieChart = new PieChart(m_hwnd, m_d2, this);
	m_eqHistoryAxes = new Axes(m_hwnd, m_d2, this);
	m_returnsAxes = new Axes(m_hwnd, m_d2, this);
	m_returnsPercAxes = new Axes(m_hwnd, m_d2, this);
	m_msgBox = new MessageScrollBox(m_hwnd, m_d2, this);

	m_allItems.push_back(m_titleBar);
	m_allItems.push_back(m_chart);
	m_allItems.push_back(m_watchlist);
	m_allItems.push_back(m_portfolioList);
	m_allItems.push_back(m_menuBar);
	m_allItems.push_back(m_pieChart);
	m_allItems.push_back(m_eqHistoryAxes);
	m_allItems.push_back(m_returnsAxes);
	m_allItems.push_back(m_returnsPercAxes);
	m_allItems.push_back(m_msgBox);
	m_activeItems.push_back(m_titleBar);
	m_activeItems.push_back(m_chart);
	m_activeItems.push_back(m_watchlist);

	// Get account names, etc.
	InitData();

	// Initialize AppItems
	InitItems();

	return 0;
}

LRESULT Parthenos::OnNCHitTest(POINT cursor) 
{
	if (!::ScreenToClient(m_hwnd, &cursor)) throw Error(L"ScreenToClient failed");

	TitleBar::Buttons but = m_titleBar->HitTest(cursor);
	if (but == TitleBar::Buttons::CAPTION)
		return HTCAPTION;
	else
		return HTCLIENT;
}

LRESULT Parthenos::OnSize(WPARAM wParam)
{
	if (wParam == SIZE_MINIMIZED)
		return 0;
	else if (wParam == SIZE_MAXIMIZED)
		m_titleBar->SetMaximized(true);
	else if (wParam == SIZE_RESTORED)
		m_titleBar->SetMaximized(false);

	m_sizeChanged = true;
	if (m_d2.pD2DContext != NULL)
	{
		m_d2.DiscardGraphicsResources();
		
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		D2D1_RECT_F dipRect = DPIScale::PixelsToDips(rc);

		m_halfBelowMenu = DPIScale::SnapToPixelY((dipRect.bottom + m_menuBarBottom) / 2.0f);
		m_centerX = DPIScale::SnapToPixelX(dipRect.right / 2.0f);
		CalculateDividingLines(dipRect);

		for (auto item : m_activeItems)
		{
			item->SetSize(CalculateItemRect(item, dipRect));
		}

		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	return 0;
}

LRESULT Parthenos::OnPaint()
{
	RECT rc;
	BOOL bErr = GetClientRect(m_hwnd, &rc);
	if (bErr == 0) OutputError(L"GetClientRect failed");

	HRESULT hr = m_d2.CreateGraphicsResources(m_hwnd);
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		D2D1_RECT_F dipRect = DPIScale::PixelsToDips(ps.rcPaint);
		m_d2.pD2DContext->BeginDraw();

		if (ps.rcPaint.left == 0 && ps.rcPaint.top == 0 &&
			ps.rcPaint.right == rc.right && ps.rcPaint.bottom == rc.bottom)
		{
			m_d2.pD2DContext->Clear(Colors::MAIN_BACKGROUND);
		}

		// reverse loop since stored in z-order == front needs to be painted last
		for (auto it = m_activeItems.rbegin(); it != m_activeItems.rend(); it++)
			(*it)->Paint(dipRect);

		for (Line_t const & l : m_dividingLines)
		{
			m_d2.pBrush->SetColor(Colors::MEDIUM_LINE);
			m_d2.pD2DContext->DrawLine(l.start, l.end, m_d2.pBrush, 1.0f, m_d2.pHairlineStyle);
		}
	

		hr = m_d2.pD2DContext->EndDraw();
		
		if (SUCCEEDED(hr))
		{
			// Present
			DXGI_PRESENT_PARAMETERS parameters = { 0 };
			parameters.DirtyRectsCount = 0;
			parameters.pDirtyRects = nullptr;
			parameters.pScrollRect = nullptr;
			parameters.pScrollOffset = nullptr;

			hr = m_d2.pDXGISwapChain->Present1(1, 0, &parameters);
		}
		
		EndPaint(m_hwnd, &ps);
	}
	if (FAILED(hr)) OutputHRerr(hr, L"OnPaint failed");

	return 0;
}

LRESULT Parthenos::OnMouseMove(POINT cursor, WPARAM wParam)
{
	Cursor::isSet = false;
	m_mouseTrack.OnMouseMove(m_hwnd);  // Start tracking.

	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	if (m_mouseCaptured)
	{
		m_mouseCaptured->OnMouseMove(dipCursor, wParam, false);
	}
	else
	{
		bool handeled = false;
		for (auto item : m_activeItems)
		{
			handeled = item->OnMouseMove(dipCursor, wParam, handeled) || handeled;
		}
	}

	if (!Cursor::isSet) ::SetCursor(Cursor::hArrow);

	ProcessCTPMessages();
	return 0;
}

LRESULT Parthenos::OnMouseWheel(POINT cursor, WPARAM wParam)
{
	if (!::ScreenToClient(m_hwnd, &cursor)) throw Error(L"ScreenToClient failed");
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	bool handeled = false;
	for (auto item : m_activeItems)
	{
		handeled = item->OnMouseWheel(dipCursor, wParam, handeled) || handeled;
	}

	//ProcessCTPMessages();
	return 0;
}

LRESULT Parthenos::OnLButtonDown(POINT cursor, WPARAM wParam)
{
	SetCapture(m_hwnd); // always?

	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	if (m_mouseCaptured)
	{
		m_mouseCaptured->OnLButtonDown(dipCursor, false);
	}
	else
	{
		bool handeled = false;
		for (auto item : m_activeItems)
		{
			handeled = item->OnLButtonDown(dipCursor, handeled) || handeled;
		}
	}
	ProcessCTPMessages();
	return 0;
}

LRESULT Parthenos::OnLButtonDblclk(POINT cursor, WPARAM wParam)
{
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	for (auto item : m_activeItems)
	{
		item->OnLButtonDblclk(dipCursor, wParam);
	}
	//ProcessCTPMessages();
	return 0;
}

LRESULT Parthenos::OnLButtonUp(POINT cursor, WPARAM wParam)
{
	D2D1_POINT_2F dipCursor = DPIScale::PixelsToDips(cursor);
	if (m_mouseCaptured)
	{
		m_mouseCaptured->OnLButtonUp(dipCursor, wParam);
	}
	else
	{
		for (auto item : m_activeItems)
		{
			item->OnLButtonUp(dipCursor, wParam);
		}
	}
	ProcessCTPMessages();
	
	ReleaseCapture();
	return 0;
}

LRESULT Parthenos::OnChar(wchar_t c, LPARAM lParam)
{
	for (auto item : m_activeItems)
	{
		if (item->OnChar(c, lParam)) break;
	}
	//ProcessCTPMessages();
	return 0;
}

bool Parthenos::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	bool out = false;
	for (auto item : m_activeItems)
	{
		if (item->OnKeyDown(wParam, lParam)) out = true;
	}
	//ProcessCTPMessages();
	return out;
}

LRESULT Parthenos::OnTimer(WPARAM wParam, LPARAM lParam)
{
	for (auto item : m_activeItems)
	{
		item->OnTimer(wParam, lParam);
	}

	for (int i = 1; i < Timers::n_timers + 1; i++)
	{
		if (m_timers.nActiveP1[i] == 1)
		{
			BOOL err = ::KillTimer(m_hwnd, i);
			if (err == 0) OutputError(L"Kill timer failed");
			m_timers.nActiveP1[i] = 0;
		}
	}
	//ProcessCTPMessages();
	return 0;
}

void Parthenos::OnCopy()
{
	for (auto item : m_activeItems)
	{
		if (item->OnCopy()) break; // short circuit
	}
}


///////////////////////////////////////////////////////////
// --- Other ---

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}