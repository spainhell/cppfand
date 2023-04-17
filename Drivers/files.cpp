#include "files.h"

HANDLE OpenF(const std::string& path, DWORD& error, DWORD access_mode, DWORD share_mode, DWORD create_mode, DWORD file_attr)
{
	const HANDLE hFile = CreateFile(
		path.c_str(),
		access_mode,
		share_mode,
		nullptr,
		create_mode,
		file_attr,
		nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		error = GetLastError();
	}
	else {
		error = 0;
	}
	return hFile;
}

bool ReadF(HANDLE hFile, void* buffer, size_t length, DWORD& error)
{
	bool result;
	DWORD dwBytesRead;
	const bool readResult = ReadFile(hFile, buffer, length, &dwBytesRead, NULL);
	if (!readResult) {
		error = GetLastError();
		result = false;
	}
	else {
		error = 0;
		result = true;
	}
	return result;
}

bool WriteF(HANDLE hFile, void* buffer, size_t length, DWORD& error)
{
	bool result;
	DWORD dwBytesWritten;
	const bool writeResult = WriteFile(hFile, buffer, length, &dwBytesWritten, NULL);
	if (!writeResult) {
		error = GetLastError();
		result = false;
	}
	else {
		error = 0;
		result = true;
	}
	return result;
}

bool SeekF(HANDLE hFile, DWORD& error, long distance, DWORD method)
{
	bool result;
	const DWORD seekResult = SetFilePointer(hFile, distance, NULL, method);
	if (seekResult == INVALID_SET_FILE_POINTER) {
		error = GetLastError();
		result = false;
	}
	else {
		error = 0;
		result = true;
	}
	return result;
}

long PosF(HANDLE hFile, DWORD& error)
{
	long result;
	const DWORD seekResult = SetFilePointer(hFile, 0, NULL, 1);
	if (seekResult == INVALID_SET_FILE_POINTER) {
		error = GetLastError();
		result = -1;
	}
	else {
		error = 0;
		result = seekResult;
	}
	return result;
}

bool TruncF(HANDLE hFile, DWORD& error, long distance)
{
	bool result;

	if (distance < 0) {
		result = SetEndOfFile(hFile);
	}
	else {
		const DWORD seekResult = SetFilePointer(hFile, distance, NULL, 0);
		if (seekResult == INVALID_SET_FILE_POINTER) {
			result = false;
		}
		else {
			result = SetEndOfFile(hFile);
		}
	}

	if (result) {
		error = 0;
	}
	else {
		error = GetLastError();
	}

	return result;
}

long SizeF(HANDLE hFile, DWORD& error)
{
	DWORD result = GetFileSize(hFile, NULL);
	if (result == INVALID_FILE_SIZE) {
		error = GetLastError();
		result = 0;
	}
	else {
		error = 0;
	}
	return result;
}

bool CloseF(HANDLE& hFile, DWORD& error)
{
	const bool result = CloseHandle(hFile);
	if (!result) {
		error = GetLastError();
	}
	else {
		error = 0;
	}
	hFile = nullptr;
	return result;
}

long GetFileAttr(const std::string& path, DWORD& error)
{
	// ziska atributy souboru / adresare
	DWORD result = GetFileAttributesA(path.c_str());
	if (result == INVALID_FILE_ATTRIBUTES) {
		error = GetLastError();
		return -1;
	}
	else {
		error = 0;
		return (long)result;
	}
}

bool SetFileAttr(const std::string& path, DWORD& error, long attributes)
{
	// nastavi atributy souboru/adresare
	// TODO: toto je nesmysl, neodpovida WinAPI!!! :
	// 0 = read only, 1 = hidden file, 2 = system file, 3 = volume label, 4 = subdirectory,
	// 5 = written since backup, 8 = shareable (Novell NetWare)
	if (SetFileAttributesA(path.c_str(), attributes) == 0) {
		error = GetLastError();
		return false;
	}
	else {
		error = 0;
		return true;
	}
}
