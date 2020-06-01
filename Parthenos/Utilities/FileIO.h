#pragma once

#include "../stdafx.h"
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
	bool Write(LPCVOID data, size_t nBytes);
	bool Append(LPCVOID data, size_t nBytes);

	template <typename T>
	std::vector<T> Read()
	{
		DWORD bytesRead = 0;
		char *buffer = _Read(bytesRead, true);
		std::vector<T> out(reinterpret_cast<T*>(buffer), reinterpret_cast<T*>(buffer + bytesRead));
		delete buffer;
		return out;
	}

	template <typename T>
	std::vector<T> ReadEnd(size_t n)
	{
		DWORD bytesToRead = sizeof(T) * static_cast<DWORD>(n);
		char *buffer = _Read(bytesToRead, false);
		std::vector<T> out(reinterpret_cast<T*>(buffer), reinterpret_cast<T*>(buffer + bytesToRead));
		delete buffer;
		return out;
	}

	std::string ReadText()
	{
		DWORD bytesRead = 0;
		char *buffer = _Read(bytesRead, true);
		std::string out(buffer, bytesRead);
		//std::string out(buffer);
		delete buffer;
		return out;
	}


private:
	std::wstring m_filename;
	HANDLE m_hFile;
	DWORD m_access = 0;

	char* _Read(DWORD & bytesToRead, bool all = false);
	bool _Write(LPCVOID data, DWORD nBytes, bool append);
};