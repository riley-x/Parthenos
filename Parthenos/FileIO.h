#pragma once

#include "stdafx.h"
#include "utilities.h"

namespace {
	DWORD g_error = 0;
	DWORD g_bytes = 0;


	VOID CALLBACK CompletionRoutine(
		__in  DWORD dwErrorCode,
		__in  DWORD dwNumberOfBytesTransfered,
		__in  LPOVERLAPPED lpOverlapped)
	{
		g_error = dwErrorCode;
		g_bytes = dwNumberOfBytesTransfered;
	}
}

class FileIO {
public:
	~FileIO() { Close(); }


	void Init(std::wstring filename, HWND phwnd);
	void Open(DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE);
	void Close();
	bool Write(LPCVOID data, DWORD nBytes);

	template <typename T>
	std::vector<T> Read()
	{
		static const int bufferSize = 10000;
		DWORD dwBytesRead = 0;
		char ReadBuffer[bufferSize] = { 0 };

		if (m_access == GENERIC_READ)
		{
			OVERLAPPED ol = { 0 };
			BOOL bErrorFlag = ReadFileEx(m_hFile, ReadBuffer, bufferSize - 1, &ol, CompletionRoutine);
			if (bErrorFlag == FALSE)
			{
				OutputMessage(L"Unable to read from file.\n");
				return std::vector<T>();
			}
			DWORD sleepResult = SleepEx(2000, TRUE); // 2 seconds
			// Shouldn't sleep? Check https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-sleepex
			if (sleepResult == 0) // timeout
			{
				OutputMessage(L"Warning: sleep timed-out\n");
			}
			if (g_bytes > 0 && g_bytes <= bufferSize - 1)
			{
				//ReadBuffer[g_bytes] = '\0';
				//OutputDebugStringA(ReadBuffer);
				//OutputMessage(L"\n%lu\n", g_bytes);

				return std::vector<T>(reinterpret_cast<T*>(ReadBuffer), reinterpret_cast<T*>(ReadBuffer + g_bytes));
			}

		}

		return std::vector<T>();
	}

private:
	HWND m_phwnd;
	std::wstring m_filename;
	HANDLE m_hFile;
	DWORD m_access = 0;

};