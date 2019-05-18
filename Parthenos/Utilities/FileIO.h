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
	bool Append(LPCVOID data, DWORD nBytes);

	template <typename T>
	std::vector<T> Read()
	{
		return _Read<T>(0, true);
	}

	template <typename T>
	std::vector<T> ReadEnd(size_t n)
	{
		DWORD bytesToRead = sizeof(T) * static_cast<DWORD>(n);
		return _Read<T>(bytesToRead, false);
	}


private:
	std::wstring m_filename;
	HANDLE m_hFile;
	DWORD m_access = 0;

	bool _Write(LPCVOID data, DWORD nBytes, bool append);

	// Reads from end of file - bytesToRead
	template <typename T>
	std::vector<T> _Read(DWORD bytesToRead, bool all = false)
	{
		BOOL bErrorFlag;

		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			OutputMessage(L"Handle not initialized!\n");
			return std::vector<T>();
		}

		// Get file size
		LONGLONG fileSize;
		LARGE_INTEGER fileSize_struct;

		bErrorFlag = GetFileSizeEx(m_hFile, &fileSize_struct);
		if (bErrorFlag == 0)
		{
			OutputMessage(L"Couldn't get filesize.\n");
			return std::vector<T>();
		}
		fileSize = fileSize_struct.QuadPart;

		if (all || bytesToRead > static_cast<DWORD>(fileSize))
			bytesToRead = static_cast<DWORD>(fileSize);

		char *ReadBuffer = new char[bytesToRead];

		if (m_access == GENERIC_READ)
		{
			OVERLAPPED ol = { 0 };
			ol.Offset = static_cast<DWORD>(fileSize) - bytesToRead;
			
			bErrorFlag = ReadFileEx(m_hFile, ReadBuffer, bytesToRead, &ol, CompletionRoutine);
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
			// Set file pointer
			LARGE_INTEGER offset; offset.QuadPart = 0;
			if (all)
			{
				bErrorFlag = SetFilePointerEx(m_hFile, offset, NULL, FILE_BEGIN);
			}
			else
			{
				offset.QuadPart = -static_cast<LONGLONG>(bytesToRead);
				bErrorFlag = SetFilePointerEx(m_hFile, offset, NULL, FILE_END);
			}
			if (bErrorFlag == FALSE)
			{
				OutputError(L"Seek failed");
				return std::vector<T>();
			}

			// Do the read
			bErrorFlag = ReadFile(m_hFile, ReadBuffer, bytesToRead, &g_bytes, NULL);
			if (bErrorFlag == FALSE)
			{
				OutputError(L"Unable to read from file.\n");
				delete[] ReadBuffer;
				return std::vector<T>();
			}
		}

		if (g_bytes != bytesToRead)
		{
			OutputMessage(L"Warning: Improper amount of bytes read: %lu out of %lu.\n", g_bytes, bytesToRead);
		}
		std::vector<T> out(reinterpret_cast<T*>(ReadBuffer), reinterpret_cast<T*>(ReadBuffer + g_bytes));
		delete[] ReadBuffer;
		return out;
	}
};