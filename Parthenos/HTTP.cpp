#include "stdafx.h"

#include <WinInet.h>
#pragma comment (lib, "Wininet.lib")

#include "utilities.h"


// Retrieving Headers Using a Constant
BOOL PrintRequestHeader(HINTERNET hHttp)
{
	LPVOID lpOutBuffer = NULL;
	DWORD dwSize = 0;

retry:
	// This call will fail on the first pass, because
	// no buffer is allocated. HTTP_QUERY_RAW_HEADERS_CRLF
	if (!HttpQueryInfo(hHttp, HTTP_QUERY_RAW_HEADERS_CRLF | HTTP_QUERY_FLAG_REQUEST_HEADERS,
		(LPVOID)lpOutBuffer, &dwSize, NULL))
	{
		if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
		{
			// Code to handle the case where the header isn't available.
			OutputError(L"HttpQuery: Header not found");
			return TRUE;
		}
		else
		{
			// Check for an insufficient buffer.
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				// Allocate the necessary buffer.
				lpOutBuffer = new char[dwSize];

				// Retry the call.
				goto retry;
			}
			else
			{
				// Error handling code.
				OutputError(L"HttpQueryInfo error");
				if (lpOutBuffer)
				{
					delete[] lpOutBuffer;
				}
				return FALSE;
			}
		}
	}

	if (lpOutBuffer)
	{
		OutputDebugString((wchar_t*)lpOutBuffer);
		delete[] lpOutBuffer;
	}

	return TRUE;
}


// send https request
std::string SendHTTPSRequest_GET(const std::wstring& _server,
	const std::wstring& _page,
	const std::wstring& _params = L"")
{
	char szData[1024];
	std::string out("");

	// initialize WinInet
	HINTERNET hInternet = ::InternetOpen(L"WinInet Test", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet != NULL)
	{
		// open HTTP session
		HINTERNET hConnect = ::InternetConnect(hInternet, _server.c_str(), INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
		if (hConnect != NULL)
		{
			std::wstring request = _page + (_params.empty() ? L"" : (L"?" + _params));
			LPCWSTR rgpszAcceptTypes[] = { L"text/*", NULL };

			// open request
			HINTERNET hRequest = ::HttpOpenRequest(hConnect, L"GET", request.c_str(), L"HTTP/1.1", NULL, rgpszAcceptTypes, INTERNET_FLAG_SECURE, 1);
			if (hRequest != NULL)
			{
				// send request
				BOOL isSend = ::HttpSendRequest(hRequest, NULL, 0, NULL, 0);
				if (isSend)
				{
					// read data
					DWORD dwByteRead;
					while (::InternetReadFile(hRequest, reinterpret_cast<void*>(szData), sizeof(szData) - 1, &dwByteRead))
					{
						// break cycle if on end
						if (dwByteRead == 0) break;

						// save result
						szData[dwByteRead] = 0;
						out.append(szData);
					}
				}
				else
				{
					OutputError(L"Send failed");
				}
				//PrintRequestHeader(hRequest);

				// close request
				::InternetCloseHandle(hRequest);
			}
			// close session
			::InternetCloseHandle(hConnect);
		}
		// close WinInet
		::InternetCloseHandle(hInternet);
	}

	return out;
	//return std::wstring(out.begin(), out.end());
}

