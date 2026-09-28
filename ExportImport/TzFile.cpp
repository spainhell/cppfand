#include "TzFile.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <windows.h>

#include "../Common/CommonVariables.h"
#include "../Core/access.h"
#include "../Core/base.h"
#include "../Core/Catalog.h"
#include "../Core/GlobalVariables.h"
#include "../Core/legacy.h"
#include "../Core/oaccess.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"

namespace
{
	// DOS mask: '*' any characters, '?' one character, case insensitive
	bool WildcardMatch(const char* s, const char* m)
	{
		const char* star = nullptr;
		const char* ss = nullptr;
		while (*s != '\0') {
			if (*m == '*') {
				star = m++;
				ss = s;
			}
			else if (*m == '?' || toupper((uint8_t)*m) == toupper((uint8_t)*s)) {
				m++;
				s++;
			}
			else if (star != nullptr) {
				m = star + 1;
				s = ++ss;
			}
			else {
				return false;
			}
		}
		while (*m == '*') m++;
		return *m == '\0';
	}
}

TzFile::TzFile(bool BkUp, bool compress, bool SubDirO, bool OverwrO, int Ir, const std::string& aDir) : TyFile(compress)
{
	SaveFiles();
	ForAllFDs(ForAllFilesOperation::close_passive_fd);
	IsBackup = BkUp;
	SubDirOpt = SubDirO;
	OverwrOpt = OverwrO;
	Vol = catalog->GetVolume(Ir);
	CPath = catalog->GetPathName(Ir);
	Path = FExpand(CPath);
	drive_letter = Path.empty() ? '\0' : Path[0];
	Dir = aDir.empty() ? GetDir(0) : FExpand(aDir);
	AddBackSlash(Dir);
}

void TzFile::Close()
{
	if (Handle != nullptr) {
		CloseArchive();
	}
}

int32_t TzFile::GetWPtr()
{
	const int32_t result = (int32_t)table_.size();
	table_.resize(table_.size() + 4, 0);
	return result;
}

void TzFile::StoreWPtr(int32_t Pos, int32_t N)
{
	memcpy(table_.data() + Pos, &N, 4);
}

int32_t TzFile::StoreWStr(const std::string& s)
{
	const int32_t result = (int32_t)table_.size();
	const size_t len = s.length() > 255 ? 255 : s.length();
	table_.push_back((uint8_t)len);
	table_.insert(table_.end(), s.begin(), s.begin() + len);
	return result;
}

int32_t TzFile::ReadWPtr(int32_t Pos) const
{
	if (Pos < 0 || (size_t)Pos + 4 > table_.size()) {
		SetMsgPar(Path);
		RunError(883);
	}
	int32_t n;
	memcpy(&n, table_.data() + Pos, 4);
	return n;
}

std::string TzFile::ReadWStr(int32_t& Pos) const
{
	if (Pos < 0 || (size_t)Pos >= table_.size() || (size_t)Pos + 1 + table_[Pos] > table_.size()) {
		SetMsgPar(Path);
		RunError(883);
	}
	const uint8_t len = table_[Pos];
	std::string s((const char*)table_.data() + Pos + 1, len);
	Pos += len + 1;
	return s;
}

int32_t TzFile::StoreDirD(const std::string& RDir)
{
	const int32_t result = GetWPtr(); // next directory
	GetWPtr();                        // 1st file name
	GetWPtr();                        // count of files
	StoreWStr(RDir);
	return result;
}

// checks (RESTOREM with SUBDIR: creates) the directory; returns its path with '\'
std::string TzFile::SetDir(const std::string& RDir)
{
	std::string d = Dir + RDir;
	SetMsgPar(d);
	DelBackSlash(d);
	const DWORD attr = GetFileAttributesA(d.c_str());
	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		if (!IsBackup && SubDirOpt) {
			if (!CreateDirectoryA(d.c_str(), nullptr)) {
				RunError(644);
			}
		}
		else {
			RunError(703);
		}
	}
	return Dir + RDir;
}

bool TzFile::MatchesMask(const std::string& name) const
{
	if (masks_.empty()) return true;
	for (const std::string& mask : masks_) {
		// name without extension matches "*.*" and "NAME.*" as in DOS
		if (WildcardMatch(name.c_str(), mask.c_str())
			|| (name.find('.') == std::string::npos && WildcardMatch((name + ".").c_str(), mask.c_str()))) {
			return true;
		}
	}
	return false;
}

void TzFile::Get1Dir(int32_t D, int32_t& DLast)
{
	int32_t i = D + 12;
	const std::string RDir = ReadWStr(i);
	const std::string p = Dir + RDir + "*.*";
	std::vector<std::string> sub_dirs;
	int32_t n = 0;

	WIN32_FIND_DATAA fd;
	HANDLE hf = FindFirstFileA(p.c_str(), &fd);
	if (hf == INVALID_HANDLE_VALUE) {
		const DWORD err = GetLastError();
		if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES) {
			SetMsgPar(p);
			RunError(904);
		}
	}
	else {
		do {
			const std::string name = fd.cFileName;
			if (name.length() > 255) continue;
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				if (name != "." && name != "..") sub_dirs.push_back(name);
			}
			else if ((fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0
				&& MatchesMask(name)
				&& _stricmp((Dir + RDir + name).c_str(), Path.c_str()) != 0 /* not the archive itself */) {
				i = StoreWStr(name);
				if (n == 0) StoreWPtr(D + 4, i);
				n++;
			}
		} while (FindNextFileA(hf, &fd));
		FindClose(hf);
	}

	StoreWPtr(D + 8, n);
	StoreWPtr(DLast, 0);
	if (!SubDirOpt) return;
	for (const std::string& sub_dir : sub_dirs) {
		i = StoreDirD(RDir + sub_dir + "\\");
		StoreWPtr(DLast, i);
		DLast = i;
	}
}

void TzFile::GetDirs(const std::string& mask)
{
	// masks separated by spaces or commas
	masks_.clear();
	size_t j = 0;
	while (j < mask.length()) {
		while (j < mask.length() && (mask[j] == ' ' || mask[j] == ',')) j++;
		const size_t start = j;
		while (j < mask.length() && mask[j] != ' ' && mask[j] != ',') j++;
		if (j > start) masks_.push_back(mask.substr(start, std::min<size_t>(j - start, 255)));
	}

	table_.clear();
	int32_t d = StoreDirD("");
	int32_t d_last = d;
	do {
		Get1Dir(d, d_last);
		d = ReadWPtr(d);
	} while (d != 0);

	size_t pos = 0;
	WrH((uint32_t)table_.size(), [&](uint8_t* buf, size_t len) {
		memcpy(buf, table_.data() + pos, len);
		pos += len;
		});
}

// reads one item <4 B size><data> of the archive
void TzFile::RdH(const std::function<void(const uint8_t*, size_t)>& write)
{
	auto fill = [&]() {
		if (iBuf == lBuf) {
			ReadBuf();
			if (lBuf == 0) {
				// the archive is shorter than expected
				HandleError = ERROR_HANDLE_EOF;
				TestErr();
			}
		}
		};

	uint32_t sz;
	uint8_t* a = (uint8_t*)&sz;
	for (size_t i = 0; i < 4; i++) {
		fill();
		a[i] = buffer1[iBuf];
		iBuf++;
	}
	while (sz > 0) {
		fill();
		size_t n = lBuf - iBuf;
		if (sz < n) n = sz;
		if (write != nullptr) write(&buffer1[iBuf], n);
		iBuf += n;
		sz -= (uint32_t)n;
	}
}

// writes one item <4 B size><data> into the archive
void TzFile::WrH(uint32_t Sz, const std::function<void(uint8_t*, size_t)>& read)
{
	memcpy(buffer1, &Sz, 4);
	size_t j = 4;
	size_t max = BufSize - 4;
	RunMsgOn('C', (int32_t)Sz);
	uint32_t i = 0;
	do {
		size_t n = max;
		if (Sz - i < n) n = Sz - i;
		i += (uint32_t)n;
		if (n > 0) read(&buffer1[j], n);
		lBuf = j + n;
		WriteBuf(false);
		j = 0;
		max = BufSize;
		RunMsgN((int32_t)i);
	} while (i != Sz);
	RunMsgOff();
}

void TzFile::ProcFileList()
{
	int32_t d = 0;
	do {
		const int32_t d_next = ReadWPtr(d);
		int32_t n = ReadWPtr(d + 8);
		int32_t i = d + 12;
		const std::string r_dir = ReadWStr(i);
		const std::string dir = SetDir(r_dir);
		if (n > 0) i = ReadWPtr(d + 4);
		while (n > 0) {
			const std::string file_name = ReadWStr(i);
			n--;
			CPath = dir + file_name;
			CVol = "";
			HANDLE h = nullptr;
			try {
				if (IsBackup) {
					h = OpenH(CPath, _isOldFile, RdOnly);
					TestCPathError();
					WrH((uint32_t)FileSizeH(h), [&](uint8_t* buf, size_t len) { ReadH(h, len, buf); });
				}
				else {
					bool skip = false;
					if (!OverwrOpt) {
						h = OpenH(CPath, _isNewFile, Exclusive);
						if (HandleError == ERROR_FILE_EXISTS) {
							SetMsgPar(CPath);
							if (PromptYN(780)) h = OpenH(CPath, _isOverwriteFile, Exclusive);
							else skip = true;
						}
					}
					else {
						h = OpenH(CPath, _isOverwriteFile, Exclusive);
					}
					if (skip) {
						RdH(nullptr);
					}
					else {
						TestCPathError();
						RdH([&](const uint8_t* buf, size_t len) { WriteH(h, len, buf); });
					}
				}
			}
			catch (...) {
				CloseH(&h);
				throw;
			}
			CloseH(&h);
		}
		d = d_next;
	} while (d != 0);
}

void TzFile::Backup(const std::string& mask)
{
	MountVol(true);
	Rewrite();
	InitBufOutp();
	GetDirs(mask);
	ProcFileList();
	WriteBuf(true);
}

void TzFile::Restore()
{
	MountVol(true);
	Reset();
	if (Size == 0) {
		CloseArchive();
	}
	else {
		InitBufInp();
		table_.clear();
		RdH([&](const uint8_t* buf, size_t len) { table_.insert(table_.end(), buf, buf + len); });
		ProcFileList();
	}
	RunMsgOff();
}
