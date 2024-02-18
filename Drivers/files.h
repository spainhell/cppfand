#pragma once
#include <string>
#include "windows.h"

/**
 * \brief Open or create file \n https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
 * \param path Full path to the file
 * \param error Operation error - 0 if SUCCESS
 * \param access_mode GENERIC_READ, GENERIC_WRITE, or both (GENERIC_READ | GENERIC_WRITE)
 * \param share_mode 0 (Exclusive), FILE_SHARE_DELETE, FILE_SHARE_READ, FILE_SHARE_WRITE
 * \param create_mode CREATE_ALWAYS, CREATE_NEW, OPEN_ALWAYS, OPEN_EXISTING, TRUNCATE_EXISTING
 * \param file_attr FILE_ATTRIBUTE_NORMAL, FILE_ATTRIBUTE_READONLY, FILE_ATTRIBUTE_ARCHIVE, ...
 * \return File Handle
 */
HANDLE OpenF(const std::string& path, DWORD& error, DWORD access_mode, DWORD share_mode = 0, DWORD create_mode = OPEN_ALWAYS, DWORD file_attr = FILE_ATTRIBUTE_NORMAL);

DWORD ReadF(HANDLE hFile, void* buffer, size_t length, DWORD& error);

bool WriteF(HANDLE hFile, void* buffer, size_t length, DWORD& error);

/**
 * \brief Set File Pointer
 * \param hFile File Handle
 * \param error Operation error - 0 if SUCCESS
 * \param distance number of bytes to move
 * \param method 0 - begin, 1 - current, 2 - end
 * \return true if success, otherwise false
 */
bool SeekF(HANDLE hFile, DWORD& error, long distance, DWORD method = 0);

/**
 * \brief Get File Position
 * \param hFile File Handle
 * \param error Operation error - 0 if SUCCESS
 * \return pointer position
 */
long PosF(HANDLE hFile, DWORD& error);

/**
 * \brief Truncate File
 * \param hFile File Handle
 * \param error Operation error - 0 if SUCCESS
 * \param distance New file length | -1 for actual position
 * \return true if success, otherwise false
 */
bool TruncF(HANDLE hFile, DWORD& error, long distance = -1);

/**
 * \brief File Size
 * \param hFile File Handle
 * \param error Operation error - 0 if SUCCESS
 * \return File size or 0 if error
 */
long SizeF(HANDLE hFile, DWORD& error);

/**
 * \brief 
 * \param hFile 
 * \param error 
 * \return 
 */
bool FlushF(HANDLE hFile, DWORD& error);

/**
 *
 */
bool CloseF(HANDLE& hFile, DWORD& error);

/**
 *
 */
long GetFileAttr(const std::string& path, DWORD& error);

/**
 *
 */
bool SetFileAttr(const std::string& path, DWORD& error, long attributes);

/**
 *
 */
long GetDiskFree(char drive, DWORD& error);