#pragma once
#include <functional>
#include <string>

#include "FileEnums.h"

typedef void* HANDLE;

// Error of the last file operation (system error code, 0 = OK).
// Kept global for compatibility: CppFand reads it after I/O calls.
extern unsigned long HandleError;

HANDLE OpenH(const std::string& path, FileOpenMode Mode, FileUseMode UM);
long SeekH(HANDLE handle, size_t offset);
size_t ReadH(HANDLE handle, size_t length, void* buffer);
size_t WriteH(HANDLE handle, size_t length, const void* buffer);
long FileSizeH(HANDLE handle);
void CloseH(HANDLE* handle);
void CloseClearH(HANDLE* h);
void MyDeleteFile(const std::string& path);
void RenameFile56(const std::string& OldPath, const std::string& NewPath, bool Msg);

namespace fandio
{
	// Volume of the file being opened. CppFand keeps the volume of the last
	// resolved path in global state (CVol); OpenH asks for it to know whether
	// to wait for a file locked by another user on a network volume.
	// Default: empty volume (local).
	void SetCurrentVolumeQuery(std::function<std::string()> query);
}
