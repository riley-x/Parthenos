#include "stdafx.h"
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

// Overwrites file, with truncating
bool FileIO::Write(LPCVOID data, DWORD nBytes)
{
	return _Write(data, nBytes, false);
}

// Appends data to current file
bool FileIO::Append(LPCVOID data, DWORD nBytes)
{
	return _Write(data, nBytes, true);
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
