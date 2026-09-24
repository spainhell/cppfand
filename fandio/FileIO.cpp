#include "FileIO.h"

#include <cerrno>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <utility>
#include <windows.h>

#include "FilePath.h"
#include "Messages.h"
#include "../fandbase/files.h"
#include "../Logging/Logging.h"

unsigned long HandleError = 0; // r229

namespace
{
	bool current_volume_is_net()
	{
		return fandio::IsNetVolume(fandio::CurrentVolume());
	}

	// files opened by OpenH (handle -> path), for messages and debugging
	std::map<HANDLE, std::string> opened_files_;
}

namespace fandio
{
	std::string OpenedPath(HANDLE handle)
	{
		auto it = opened_files_.find(handle);
		return it != opened_files_.end() ? it->second : std::string();
	}
}

long SeekH(HANDLE handle, size_t offset)
{
	if (handle == nullptr) fandio::RaiseError(705);
	return SeekF(handle, HandleError, offset, 0);
}

size_t ReadH(HANDLE handle, size_t length, void* buffer)
{
	return ReadF(handle, buffer, length, HandleError);
}

HANDLE OpenH(const std::string& path, FileOpenMode Mode, FileUseMode UM)
{
	// $3C vytvori nebo prepise soubor
	// $3D otevira exitujici soubor
	// $5B vytvori novy soubor - pokud jiz exituje, vyhodi chybu
	//
	// bit 0: read-only, 1: hidden, 2: system, 3: volume label, 4: reserved, must be zero (directory)
	//     5: archive bit, 7: if set, file is shareable under Novell NetWare
	//
	// pri 'IsNetCVol' se chova jinak
	// RdOnly $20, RdShared $40, Shared $42, Exclusive $12

	std::string txt[] = { "Close", "OpRd", "OpRs", "OpSh", "OpEx" };

	//if (CardHandles == files) RunError(884);
	fandio::LockWait wait{ .kind = fandio::LockWaitKind::Open, .path = path, .mode = txt[UM] };

	HANDLE handle;
	DWORD access_mode = 0;
	DWORD share_mode = 0;
	DWORD create_mode = 0;

	switch (Mode) {
	case _isOldFile: {
		create_mode = OPEN_EXISTING;
		break;
	}
	case _isOldNewFile: {
		create_mode = OPEN_EXISTING;
		break;
	}
	case _isOverwriteFile: {
		create_mode = CREATE_ALWAYS;
		break;
	}
	case _isNewFile: {
		create_mode = CREATE_NEW;
		break;
	}
	}

	switch (UM) {
	case RdOnly: {
		access_mode = GENERIC_READ;
		share_mode = FILE_SHARE_READ;
		break;
	}
	case RdShared: {
		access_mode = GENERIC_READ;
		share_mode = FILE_SHARE_READ;
		break;
	}
	case Shared: {
		access_mode = GENERIC_READ | GENERIC_WRITE;
		share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;
	}
	case Exclusive: {
		access_mode = GENERIC_READ | GENERIC_WRITE;
		share_mode = 0;
		break;
	}
	default:;
	}

	while (true) {
		//HandleError = (WORD)fopen_s(&nFile, path.c_str(), openFlags.c_str());
		handle = OpenF(path, HandleError, access_mode, share_mode, create_mode, 128);
		if (handle == INVALID_HANDLE_VALUE) {
			handle = nullptr;
		}

		// https://docs.microsoft.com/en-us/cpp/c-runtime-library/errno-doserrno-sys-errlist-and-sys-nerr?view=vs-2019
		if (current_volume_is_net() && (HandleError == EACCES || HandleError == ENOLCK)) {
			// file locked by another user: wait (not cancellable) and try again
			fandio::WaitForLock(wait);
			continue;
		}

		if (HandleError == 0)
		{
			// TODO: HANDLE
			//SetHandle(handle);
			//if (Mode != _isOldFile) SetUpdHandle(handle);
		}

		else if (HandleError == ENOENT) {
			// No such file or directory
			if (/*Mode == _isOldFile ||*/ Mode == _isOldNewFile) {
				Mode = _isNewFile;
				create_mode = CREATE_NEW;
				continue;
			}
		}
		fandio::EndLockWait(wait);
		break;
	}

	SPDLOG_DEBUG("opening file {} '{}', error {}", handle, path, HandleError);

	if (handle != nullptr) {
		opened_files_[handle] = path;
	}

	return (FILE*)handle;
}

size_t WriteH(HANDLE handle, size_t length, const void* buffer)
{
	if (handle == nullptr) {
		fandio::RaiseError(706);
		return 0;
	}
	if (length <= 0) {
		return 0;
	}

	return WriteF(handle, buffer, length, HandleError);
}

long FileSizeH(HANDLE handle)
{
	long size = SizeF(handle, HandleError);
	return size;
}

void CloseH(HANDLE* handle)
{
	HANDLE h = *handle;

	if (*handle == nullptr) return;

	// uzavre soubor
	bool res = CloseF(*handle, HandleError);
	SPDLOG_DEBUG("closing file {}, error {}", h, HandleError);

	if (!res) {
		throw std::exception("Cannot close file!");
	}

	// vyradi z evidence
	if (opened_files_.erase(h) == 0) {
#ifdef _DEBUG
		// soubor v evidenci nebyl
		SPDLOG_WARN("closing file {}, but file wasn't in filesMap!", h);
#endif
	}
}

void CloseClearH(HANDLE* h)
{
	if (h == nullptr) return;
	CloseH(h);
}

void MyDeleteFile(const std::string& path)
{
	// smaze soubor - INT $41
	auto result = remove(path.c_str());
	if (result != 0) HandleError = result;
}

void RenameFile56(const std::string& OldPath, const std::string& NewPath, const bool Msg)
{
	// presouva nebo prejmenovava soubor
	// potom:
	auto result = rename(OldPath.c_str(), NewPath.c_str());
	if (result != 0) HandleError = result;
	if (Msg && HandleError != 0) {
		fandio::RaiseError(829, { OldPath, NewPath });
	}
}
