#pragma once

#include "stdafx.h"
#include "utilities.h"

bool FileExists(LPCTSTR szPath);

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


	void Init(std::wstring filename);
	void Open(DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE);
	void Close();
	bool Write(LPCVOID data, DWORD nBytes);

	template <typename T>
	std::vector<T> Read()
	{
		BOOL bErrorFlag;
		LARGE_INTEGER fileSize_struct;
		LONGLONG fileSize;

		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			OutputMessage(L"Handle not initialized!\n");
			return std::vector<T>();
		}

		bErrorFlag = GetFileSizeEx(m_hFile, &fileSize_struct);
		if (bErrorFlag == 0)
		{
			OutputMessage(L"Couldn't get filesize.\n");
			return std::vector<T>();
		}
		fileSize = fileSize_struct.QuadPart;

		char *ReadBuffer = new char[static_cast<UINT>(fileSize)];
		DWORD dwBytesRead = 0;

		if (m_access == GENERIC_READ)
		{
			OVERLAPPED ol = { 0 };
			bErrorFlag = ReadFileEx(m_hFile, ReadBuffer, static_cast<DWORD>(fileSize), &ol, CompletionRoutine);
			if (bErrorFlag == FALSE)
			{
				OutputError(L"Unable to read from file.\n");
				delete[] ReadBuffer;
				return std::vector<T>();
			}

			// Shouldn't sleep? Check https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-sleepex
			DWORD sleepResult = SleepEx(2000, TRUE); // 2 seconds
			if (sleepResult == 0) // timeout
			{
				OutputMessage(L"Warning: sleep timed-out\n");
			}
		}
		else // Synchronous R/W
		{
			bErrorFlag = ReadFile(m_hFile, ReadBuffer, static_cast<DWORD>(fileSize), &g_bytes, NULL);
			if (bErrorFlag == FALSE)
			{
				OutputError(L"Unable to read from file.\n");
				delete[] ReadBuffer;
				return std::vector<T>();
			}
		}

		if (g_bytes != fileSize)
		{
			OutputMessage(L"Warning: Improper amount of bytes read: %lu out of %lld.\n", g_bytes, fileSize);
		}
		std::vector<T> out(reinterpret_cast<T*>(ReadBuffer), reinterpret_cast<T*>(ReadBuffer + g_bytes));
		delete[] ReadBuffer;
		return out;
	}

private:
	std::wstring m_filename;
	HANDLE m_hFile;
	DWORD m_access = 0;

};