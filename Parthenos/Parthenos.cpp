// Parthenos.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include "Parthenos.h"
#include "utilities.h"
#include "TitleBar.h"
#include "HTTP.h"
#include "DataManaging.h"
#include "PopupWindow.h"

#include <windowsx.h>

///////////////////////////////////////////////////////////
// --- Forward declarations ---

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


///////////////////////////////////////////////////////////
// --- Interface functions ---

Parthenos::Parthenos(PCWSTR szClassName)
	: BorderlessWindow(szClassName)
{	
	

}

Parthenos::~Parthenos()
{
	for (auto item : m_allItems)
		if (item) delete item;
	if (m_addTWin) delete m_addTWin;
	if (m_okWin) delete m_okWin;
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
		// TODO this should really should pass to OnMouseMove or OnMouseLeave...
		m_titleBar->SetMouseOn(TitleBar::Buttons::NONE);
		m_mouseTrack.Reset(m_hwnd);
		return 0;
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
		// Get account names, calculate positions, get stats, etc.
		InitData();
	}
	catch (const std::exception & e) {
		std::wstring out = SPrintException(e);
		if (m_msgBox) m_msgBox->Print(out);
		else OutputDebugString(out.c_str());
	}

	// Data dependent initializations
	InitItemsWithData();

	// Size-dependent initializations
	m_chart->Draw(L"AAPL");
}


///////////////////////////////////////////////////////////
// --- Data Management ---

void Parthenos::InitData()
{
	// TODO hardcoded
	m_accountNames = { L"Robinhood", L"Arista" };
	m_currAccount = 0;

	// Read Holdings
	FileIO holdingsFile;
	holdingsFile.Init(ROOTDIR + L"port.hold");
	holdingsFile.Open(GENERIC_READ);
	std::vector<Holdings> out = holdingsFile.Read<Holdings>();
	holdingsFile.Close();
	NestedHoldings holdings = FlattenedHoldingsToTickers(out);

	// Populate data members
	m_tickers = GetTickers(holdings); // all tickers
	m_stats = GetBatchQuoteStats(m_tickers);

	m_accounts.resize(m_accountNames.size() + 1);
	for (size_t i = 0; i < m_accountNames.size(); i++)
		m_accounts[i].name = m_accountNames[i];
	m_accounts.back().name = L"All Accounts";

	CalculatePositions(holdings);
	CalculateReturns();
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
NestedHoldings Parthenos::CalculateHoldings() const
{
	// Read transaction history
	FileIO transFile;
	transFile.Init(ROOTDIR + L"hist.trans");
	transFile.Open(GENERIC_READ);
	std::vector<Transaction> trans = transFile.Read<Transaction>();;
	transFile.Close();

	// Transaction -> Holdings
	NestedHoldings holdings = FullTransactionsToHoldings(trans);
	std::vector<Holdings> out;
	for (auto const & x : holdings) out.insert(out.end(), x.begin(), x.end());

	// Write
	FileIO holdingsFile;
	holdingsFile.Init(ROOTDIR + L"port.hold");
	holdingsFile.Open();
	holdingsFile.Write(out.data(), sizeof(Holdings) * out.size());
	holdingsFile.Close();

	return holdings;
}

// Holdings -> Positions
void Parthenos::CalculatePositions(NestedHoldings const & holdings)
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
			double returns = p.realized_held + p.realized_unheld + p.unrealized;
			double perc = (p.n == 0) ? 0.0 : (p.realized_held + p.unrealized) / (p.avgCost * p.n) * 100.0;
			acc.returnsBarData.push_back({ returns, Colors::Randomizer(p.ticker) });
			if (perc != 0) acc.returnsPercBarData.push_back({ perc, Colors::Randomizer(p.ticker) });
			acc.tickers.push_back(p.ticker);
		}
	}
}

// Calculate historical performance
void Parthenos::CalculateHistories()
{
	for (size_t i = 0; i < m_accountNames.size(); i++)
	{
		Account & acc = m_accounts[i];
		bool exists = FileExists((ROOTDIR + acc.name + L".hist").c_str());

		FileIO histFile;
		histFile.Init(ROOTDIR + acc.name + L".hist");
		histFile.Open();
		std::vector<TimeSeries> portHist;

		if (!exists)
		{
			// Read transaction history
			FileIO transFile;
			transFile.Init(ROOTDIR + L"hist.trans");
			transFile.Open(GENERIC_READ);
			std::vector<Transaction> trans = transFile.Read<Transaction>();;
			transFile.Close();

			// Calculate full equity history
			portHist = CalculateFullEquityHistory((char)i, trans);

			// Write to file
			histFile.Write(portHist.data(), portHist.size() * sizeof(TimeSeries));
		}
		else // assumes that holdings haven't changed since last update (add transaction will properly invalidate history)
		{
			portHist = histFile.Read<TimeSeries>();
			UpdateEquityHistory(portHist, acc.positions, m_stats); 
			histFile.Write(portHist.data(), portHist.size() * sizeof(TimeSeries));
		}
		histFile.Close();

		// Add to account for plotting
		acc.histDate.reserve(portHist.size());
		acc.histEquity.reserve(portHist.size());
		double cash_in = GetCash(acc.positions).second;
		for (auto const & x : portHist)
		{
			acc.histDate.push_back(x.date);
			acc.histEquity.push_back(x.prices + cash_in);
		}
	}

	// Sum all accounts for "All".
	Account & accAll = m_accounts.back();
	std::vector<size_t> iAccDate(m_accounts.size() - 1); // index into each account's histDate
	std::vector<double> cashIn(m_accounts.size() - 1);
	size_t maxSize = 0;
	size_t iAccMax; // index of account with maximum history length
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

	accAll.histDate.reserve(maxSize);
	accAll.histEquity.reserve(maxSize);
	while (iAccDate[iAccMax] < m_accounts[iAccMax].histDate.size())
	{
		date_t date = m_accounts[iAccMax].histDate[iAccDate[iAccMax]];
		double val = 0;
		for (size_t iAcc = 0; iAcc < m_accounts.size() - 1; iAcc++)
		{
			Account & acc = m_accounts[iAcc];
			size_t iDate = iAccDate[iAcc];
			if (acc.histDate[iDate] == date)
			{
				val += acc.histEquity[iDate] - cashIn[iAcc];
				iAccDate[iAcc]++;
			}
		}
		accAll.histDate.push_back(date);
		accAll.histEquity.push_back(val + totCashIn);
	}
}

void Parthenos::AddTransaction(Transaction t)
{
	// Update transaction history
	FileIO transFile;
	transFile.Init(ROOTDIR + L"hist.trans");
	transFile.Open();
	transFile.Append(&t, sizeof(Transaction));
	transFile.Close();

	// Update holdings
	FileIO holdingsFile;
	holdingsFile.Init(ROOTDIR + L"port.hold");
	holdingsFile.Open();
	std::vector<Holdings> flattenedHoldings = holdingsFile.Read<Holdings>();
	NestedHoldings holdings = FlattenedHoldingsToTickers(flattenedHoldings);
	AddTransactionToHoldings(holdings, t);
	std::vector<Holdings> out;
	for (auto const & x : holdings) out.insert(out.end(), x.begin(), x.end());
	holdingsFile.Write(out.data(), sizeof(Holdings) * out.size());
	holdingsFile.Close();

	// Update data
	m_tickers = GetTickers(holdings);
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
	UpdateEquityHistory(portHist, m_accounts[t.account].positions, m_stats);

	// Write equity history
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


///////////////////////////////////////////////////////////
// --- Child Management ---

// Size independent initializations
void Parthenos::InitItems()
{
	m_titleBar->SetTabs({ L"Portfolio", L"Returns", L"Chart" });
	m_titleBar->SetActive(L"Chart");

	std::vector<Column> portColumns = {
		{60.0f, Column::Ticker, L""},
		{60.0f, Column::Last, L"%.2lf"},
		{60.0f, Column::ChangeP, L"%.2lf"},
		{50.0f, Column::Shares, L"%d"},
		{60.0f, Column::AvgCost, L"%.2lf"},
		{60.0f, Column::Equity, L"%.2lf"},
		{60.0f, Column::ReturnsT, L"%.2lf"},
		{60.0f, Column::ReturnsP, L"%.2lf"},
		{60.0f, Column::APY, L"%.2lf"},
		{60.0f, Column::ExDiv, L""},
	};
	m_watchlist->SetColumns(); // use defaults
	m_portfolioList->SetColumns(portColumns);

	m_returnsAxes->SetXAxisPos(0.0f);
	m_returnsAxes->SetYAxisPos(std::nanf(""));
	m_returnsAxes->SetHoverStyle(Axes::HoverStyle::snap);
	m_returnsPercAxes->SetXAxisPos(0.0f);
	m_returnsPercAxes->SetYAxisPos(std::nanf(""));
	m_returnsPercAxes->SetHoverStyle(Axes::HoverStyle::snap);
	m_eqHistoryAxes->SetYAxisPos(std::nanf(""));
	m_eqHistoryAxes->SetHoverStyle(Axes::HoverStyle::snap);

}

void Parthenos::InitItemsWithData()
{
	std::vector<std::wstring> menus = { L"File", L"Account", L"Transactions", L"Stocks" };
	std::vector<std::vector<std::wstring>> items = {
		{ L"Print Transaction History", L"Print Holdings", L"Print Equity History", L"Update Last Equity History Entry" },
		m_accountNames, // Add "all" below
		{ L"Add...", L"Recalculate All" },
		{ L"Print OHLC", L"Delete Last OHLC Entry" }
	};
	items[1].push_back(L"All Accounts");
	std::vector<std::vector<size_t>> divisions = { {3}, {m_accountNames.size()}, {}, {} };

	m_menuBar->SetMenus(menus, items, divisions);
	m_menuBar->Refresh();

	if ((size_t)m_currAccount < m_accounts.size())
	{
		std::vector<std::wstring> tickers = GetTickers(m_accounts[m_currAccount].positions);
		std::vector<std::pair<Quote, Stats>> stats = FilterByKeyMatch(m_tickers, m_stats, tickers);
		m_watchlist->Load(tickers, m_accounts[m_currAccount].positions, stats);
		m_watchlist->Refresh();
		UpdatePortfolioPlotters(m_currAccount, false);
	}
}

void Parthenos::ProcessCTPMessages()
{
	for (ClientMessage & msg : m_messages)
	{
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
			ProcessMenuMessage(msg);
			break;
		}
		case CTPMessage::WINDOW_CLOSED:
		{
			PopupWindow *win = reinterpret_cast<PopupWindow*>(msg.sender);
			if (msg.msg == L"AddTransactionWindow" && win == m_addTWin)
			{
				delete m_addTWin;
				m_addTWin = nullptr;
			}
			else if (msg.msg == L"ConfirmationWindow" && win == m_okWin)
			{
				delete m_okWin;
				m_okWin = nullptr;
			}
			else
			{
				std::wstring err = L"Error: WINDOW_CLOSED couldn't match window\n";
				if (m_msgBox) m_msgBox->Print(err);
				else OutputMessage(err);
			}
			break;
		}
		case CTPMessage::WINDOW_ADDTRANSACTION_P:
		{
			Transaction *t = reinterpret_cast<Transaction*>(msg.sender);
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
				m_mouseCaptured = msg.sender;
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
	}
	if (!m_messages.empty()) m_messages.clear();
}

void Parthenos::ProcessMenuMessage(ClientMessage & msg)
{
	if (msg.iData == 0) // File
	{
		if (msg.msg == L"Print Transaction History")
		{
			// Read transaction history
			FileIO transFile;
			transFile.Init(ROOTDIR + L"hist.trans");
			transFile.Open(GENERIC_READ);
			std::vector<Transaction> trans = transFile.Read<Transaction>();;
			transFile.Close();

			std::wstring out;
			for (Transaction const & t : trans)
				out.append(t.to_wstring(m_accountNames));
			m_msgBox->Overwrite(out);
		}
		else if (msg.msg == L"Print Holdings")
		{
			// Read Holdings
			FileIO holdingsFile;
			holdingsFile.Init(ROOTDIR + L"port.hold");
			holdingsFile.Open(GENERIC_READ);
			std::vector<Holdings> out = holdingsFile.Read<Holdings>();
			holdingsFile.Close();

			NestedHoldings holdings = FlattenedHoldingsToTickers(out);
			m_msgBox->Overwrite(NestedHoldingsToWString(holdings));
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
		}
	}
	else if (msg.iData == 1) // Account
	{
		int account = AccountToIndex(msg.msg);
		if (account < 0) account = m_accounts.size() - 1; // last entry is 'all'
		if (account != m_currAccount)
		{
			m_currAccount = account;
			UpdatePortfolioPlotters(account);
		}
	}
	else if (msg.iData == 2) // Transactions
	{
		if (msg.msg == L"Add...")
		{
			if (!m_addTWin)
			{
				m_addTWin = new AddTransactionWindow();
				BOOL ok = m_addTWin->Create(m_hInstance);
				if (!ok) OutputError(L"Create AddTransaction window failed");
				m_addTWin->SetParent(this, m_hwnd);
				m_addTWin->SetAccounts(m_accountNames);
				m_addTWin->PreShow();
				ShowWindow(m_addTWin->Window(), SW_SHOW);
			}
		}
		else if (msg.msg == L"Recalculate All")
		{
			if (msg.sender) // sent by menubar -> ask confirmation
			{
				if (!m_okWin) // shouldn't already exist
				{
					m_okWin = new ConfirmationWindow();
					BOOL ok = m_okWin->Create(m_hInstance);
					if (!ok) OutputError(L"Create AddTransaction window failed");

					m_okWin->SetParent(this, m_hwnd);
					m_okWin->SetText(L"Recalculate holdings from full transaction history?");
					msg.sender = nullptr;
					m_okWin->SetMessage(msg);
					m_okWin->PreShow();

					ShowWindow(m_okWin->Window(), SW_SHOW);
				}
			}
			else if (!msg.sender) // sent from confirmation window
			{
				m_msgBox->Print(L"Recalculating holdings\n");
				NestedHoldings holdings = CalculateHoldings();
				CalculatePositions(holdings);
			}
		}
	}
	else if (msg.iData == 3) // Stocks
	{
		if (msg.msg == L"Print OHLC")
		{
			msg.imsg = CTPMessage::NONE; // "delete" this message but don't mess up the for loop
			std::wstring file = LaunchFileDialog();

			FileIO ohlcFile;
			ohlcFile.Init(file);
			ohlcFile.Open(GENERIC_READ);
			std::vector<OHLC> ohlcData = ohlcFile.ReadEnd<OHLC>(100); // Get last 100 for now

			std::wstring out;
			for (OHLC const & x : ohlcData) out.append(x.to_wstring());
			if (m_msgBox) m_msgBox->Overwrite(out);

		}
		else if (msg.msg == L"Delete Last OHLC Entry")
		{
			msg.imsg = CTPMessage::NONE; // "delete" this message but don't mess up the for loop
			std::wstring file = LaunchFileDialog();

			FileIO ohlcFile;
			ohlcFile.Init(file);
			ohlcFile.Open();
			std::vector<OHLC> ohlcData = ohlcFile.Read<OHLC>(); // Get last 100 for now
			ohlcData.erase(ohlcData.end() - 1);
			ohlcFile.Write(ohlcData.data(), ohlcData.size() * sizeof(OHLC));

			std::wstring out;
			for (OHLC const & x : ohlcData) out.append(x.to_wstring());
			if (m_msgBox) m_msgBox->Overwrite(out);

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

void Parthenos::LoadPieChart()
{
	Account const & acc = m_accounts[m_currAccount];
	std::vector<double> market_values = GetMarketValues(acc.positions);
	std::vector<std::wstring> tickers = GetTickers(acc.positions);

	std::vector<double> data; 
	std::vector<std::wstring> short_labels;
	std::vector<std::wstring> long_labels;
	std::vector<D2D1_COLOR_F> colors;

	data.reserve(tickers.size() + 1); // Add one for cash
	short_labels.reserve(tickers.size() + 1);
	long_labels.reserve(tickers.size() + 2); // Add another for default label
	colors.reserve(tickers.size() + 1);

	// Get total equity
	double sum = std::accumulate(market_values.begin(), market_values.end(), 0.0);
	std::pair<double, double> cash = GetCash(acc.positions);
	sum += cash.first;

	// Default long label
	wchar_t buffer[100];
	swprintf_s(buffer, _countof(buffer), L"%s\n%s", acc.name.c_str(), FormatDollar(sum).c_str());
	long_labels.push_back(std::wstring(buffer));

	// Sort by market value
	std::vector<size_t> inds = sort_indexes(market_values, std::greater<>{});
	for (size_t i = 0; i < market_values.size(); i++)
	{
		if (market_values[inds[i]] <= 0.01) break;
		double value = market_values[inds[i]];
		std::wstring ticker = tickers[inds[i]];

		data.push_back(value);
		short_labels.push_back(ticker);
		long_labels.push_back(MakeLongLabel(ticker, value, 100.0 * value / sum));
		colors.push_back(Colors::Randomizer(ticker));
	}

	// Cash
	data.push_back(cash.first);
	short_labels.push_back(L"CASH");
	long_labels.push_back(MakeLongLabel(L"CASH", cash.first, 100.0 * cash.first / sum));
	colors.push_back(Colors::GREEN);

	m_pieChart->Load(data, colors, short_labels, long_labels);
}

// Updates all items that track the current account portfolio. 
// i.e. call this when the account changes or a transaction is added.
void Parthenos::UpdatePortfolioPlotters(char account, bool init)
{
	if (m_currAccount != account) return;
	Account const & acc = m_accounts[account];
	std::vector<std::wstring> tickers = GetTickers(acc.positions);

	m_portfolioList->Load(tickers, acc.positions, FilterByKeyMatch(m_tickers, m_stats, tickers));
	if (!init) m_portfolioList->Refresh();

	LoadPieChart();
	if (!init) m_pieChart->Refresh();

	wchar_t buffer[100];
	double returns = acc.histEquity.back() - acc.histEquity.front();
	swprintf_s(buffer, _countof(buffer), L"%s: %s (%.2lf%%)", acc.name.c_str(), 
		FormatDollar(returns).c_str(), returns / acc.histEquity.back() * 100.0);
	m_eqHistoryAxes->SetTitle(buffer);
	m_eqHistoryAxes->SetXAxisPos((float)acc.histEquity[0]);
	m_eqHistoryAxes->Clear();
	m_eqHistoryAxes->Line(acc.histDate.data(),
		acc.histEquity.data(),
		acc.histDate.size(),
		Colors::ALMOST_WHITE
	);

	m_returnsAxes->Clear();
	m_returnsPercAxes->Clear();
	m_returnsAxes->SetXLabels(acc.tickers, false);
	std::vector<std::wstring> perc_tickers(acc.tickers.begin(), acc.tickers.begin() + acc.returnsPercBarData.size());
	m_returnsPercAxes->SetXLabels(perc_tickers, false);
	m_returnsAxes->Bar(
		acc.returnsBarData.data(), 
		acc.returnsBarData.size(), 
		acc.tickers
	);
	m_returnsPercAxes->Bar(
		acc.returnsPercBarData.data(),
		acc.returnsPercBarData.size(),
		perc_tickers
	);

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
		throw Error(L"OnCreate failed!");
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

	m_okWin = new ConfirmationWindow(); // temp
	BOOL ok = m_okWin->Register(args);
	if (!ok) OutputError(L"Register Popup failed");
	delete m_okWin;
	m_okWin = nullptr;

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
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		m_d2.DiscardGraphicsResources();
		HRESULT hr = m_d2.CreateGraphicsResources(m_hwnd);
		if (FAILED(hr))
		{
			OutputError(L"CreateGraphicsResources failed");
			return 0;
		}

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