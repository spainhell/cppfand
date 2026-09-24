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
	// Path the file was opened with by OpenH ("" for an unknown handle)
	std::string OpenedPath(HANDLE handle);
}
