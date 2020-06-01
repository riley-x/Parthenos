#include "../stdafx.h"
#include "FileIO.h"
#include "utilities.h"

bool FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void FileIO::Close()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

void FileIO::Init(std::wstring filename)
{
	m_filename = filename;
}

void FileIO::Open(DWORD dwDesiredAccess)
{
	m_access = dwDesiredAccess;
	if (dwDesiredAccess == GENERIC_READ)
	{
		m_hFile = CreateFile(
			m_filename.c_str(),			// name of the write
			dwDesiredAccess,			// open for reading/writing
			FILE_SHARE_READ,			// share for reading
			NULL,						// default security
			OPEN_EXISTING,				// open existing file only
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,	// allow shared reading
			NULL						// no attr. template
		);
	}
	else
	{
		m_hFile = CreateFile(
			m_filename.c_str(),			// name of the write
			dwDesiredAccess,			// open for reading/writing
			0,							// do not share
			NULL,						// default security
			OPEN_ALWAYS,				// open or create
			FILE_ATTRIBUTE_NORMAL,		// normal file
			NULL						// no attr. template
		);
	}

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		OutputError(L"File " + m_filename + L" can't be opened");
	}
}

// Reads from end of file - bytesToRead
// Caller must delete returned array.
// Updates bytesToRead with bytes read.
char* FileIO::_Read(DWORD & bytesToRead, bool all)
{
	BOOL bErrorFlag;

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		OutputMessage(L"Handle not initialized!\n");
		return nullptr;
	}

	// Get file size
	LONGLONG fileSize;
	LARGE_INTEGER fileSize_struct;

	bErrorFlag = GetFileSizeEx(m_hFile, &fileSize_struct);
	if (bErrorFlag == 0)
	{
		OutputMessage(L"Couldn't get filesize.\n");
		return nullptr;
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
			return nullptr;
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
			delete[] ReadBuffer;
			return nullptr;
		}

		// Do the read
		bErrorFlag = ReadFile(m_hFile, ReadBuffer, bytesToRead, &g_bytes, NULL);
		if (bErrorFlag == FALSE)
		{
			OutputError(L"Unable to read from file.\n");
			delete[] ReadBuffer;
			return nullptr;
		}
	}

	if (g_bytes != bytesToRead)
	{
		OutputMessage(L"Warning: Improper amount of bytes read: %lu out of %lu.\n", g_bytes, bytesToRead);
	}
	bytesToRead = g_bytes;
	return ReadBuffer;
}

// Overwrites file, with truncating
bool FileIO::Write(LPCVOID data, size_t nBytes)
{
	return _Write(data, static_cast<DWORD>(nBytes), false);
}

// Appends data to current file
bool FileIO::Append(LPCVOID data, size_t nBytes)
{
	return _Write(data, static_cast<DWORD>(nBytes), true);
}

bool FileIO::_Write(LPCVOID data, DWORD nBytes, bool append)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		OutputMessage(L"Handle not initialized!\n");
		return false;
	}
	if (!(m_access & GENERIC_WRITE))
	{
		OutputMessage(L"File " + m_filename + L" does not have write access.\n");
		return false;
	}

	LARGE_INTEGER zero; zero.QuadPart = 0;
	BOOL bErrorFlag;
	if (append)
		bErrorFlag = SetFilePointerEx(m_hFile, zero, NULL, FILE_END);
	else
		bErrorFlag = SetFilePointerEx(m_hFile, zero, NULL, FILE_BEGIN);

	if (bErrorFlag == FALSE)
	{
		OutputError(L"Seek failed");
		return false;
	}

	DWORD bytesWritten;
	bErrorFlag = ::WriteFile(
		m_hFile,			// open file handle
		data,				// start of data to write
		nBytes,				// number of bytes to write
		&bytesWritten,		// number of bytes that were written
		NULL				// no overlapped structure
	);

	if (bErrorFlag == FALSE)
	{
		OutputError(L"Unable to write to file.");
		return false;
	}
	else if (bytesWritten != nBytes)
	{
		OutputError(L"Error: dwBytesWritten != dwBytesToWrite");
		return false;
	}

	if (!append)
	{
		bErrorFlag = SetEndOfFile(m_hFile);
		if (bErrorFlag == FALSE)
		{
			OutputError(L"Unable to truncate file.");
			return false;
		}
	}

	return true;
}
