#include "TzFile.h"

#include <filesystem>
#include <regex>

#include "../Common/exprcmp.h"
#include "../Core/GlobalVariables.h"
#include "../Core/oaccess.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"

namespace fs = std::filesystem;

TzFile::TzFile(bool BkUp, bool NoCompr, bool SubDirO, bool OverwrO, int Ir, pstring aDir)
{
	SaveFiles();
	ForAllFDs(ForAllFilesOperation::close_passive_fd);
	//if NoCompr then inherited init(0) else inherited init(1);
	IsBackup = BkUp;
	SubDirOpt = SubDirO;
	OverwrOpt = OverwrO;
	OldDir = GetDir(0);
	Vol = catalog->GetVolume(Ir);
	CPath = catalog->GetPathName(Ir);
	Path = FExpand(CPath);
	drive_letter = Path[0];
	Dir = aDir;
	AddBackSlash(Dir);
	WBase = MaxWSize;
	WPos = WBase;
}

void TzFile::Close()
{
	if (Handle != nullptr) {
		MaxWSize = WBase;
		TruncF(WorkHandle, HandleError, MaxWSize);
		FlushF(WorkHandle, HandleError);
		CloseH(&Handle);
		ChDir(OldDir);
	}
}

int TzFile::GetWPtr()
{
	int result = WPos - WBase;
	WPos += 4;
	return result;
}

void TzFile::StoreWPtr(int Pos, int N)
{
	SeekH(WorkHandle, WBase + Pos);
	WriteH(WorkHandle, 4, &N);
}

int TzFile::StoreWStr(const std::string& s)
{
	SeekH(WorkHandle, WPos);
	uint8_t len = (uint8_t)s.length();
	WriteH(WorkHandle, 1, &len);
	WriteH(WorkHandle, s.length(), s.c_str());
	int result = WPos - WBase;
	WPos += s.length() + 1;
	return result;
}

int TzFile::ReadWPtr(int Pos)
{
	int n;
	SeekH(WorkHandle, WBase + Pos);
	ReadH(WorkHandle, 4, &n);
	return n;
}

std::string TzFile::ReadWStr(int& Pos)
{
	SeekH(WorkHandle, WBase + Pos);

	// get length of the string
	uint8_t len;
	ReadH(WorkHandle, 1, &len);

	// create buffer and read the string
	char buffer[256];
	ReadH(WorkHandle, len, buffer);

	Pos += len + 1;
	return std::string(buffer, len);
}

int TzFile::StoreDirD(std::string RDir)
{
	int result = GetWPtr();
	GetWPtr();
	GetWPtr();
	StoreWStr(RDir);
	return result;
}

void TzFile::SetDir(std::string RDir)
{
	std::string d = Dir + RDir;
	SetMsgPar(d);
	DelBackSlash(d);
label1:
	ChDir(d);
	if (IOResult() != 0) {
		if (!IsBackup && SubDirOpt) {
			MkDir(d);
			if (IOResult() != 0) {
				RunError(644);
			}
			goto label1;
		}
		else {
			RunError(703);
		}
	}
}

void TzFile::Get1Dir(int D, int& DLast)
{
	std::vector<std::string> dirs;

	int i = D + 12;
	std::string RDir = ReadWStr(i);
	std::string path_str_p = Dir + RDir + "*.*";

	int n = 0;

	if (!(DosError() == 0 || DosError() == 18)) {
		SetMsgPar(path_str_p);
		RunError(904);
	}

	for (const fs::directory_entry& entry : fs::directory_iterator(Dir + RDir)) {
		std::string fileName = entry.path().filename().string();
		if (entry.is_directory() && SubDirOpt) {
			// add subdir to the list if subdir processing is enabled
			dirs.push_back(fileName);
		}
		else {
			// process file
			if (v_masks_.empty()) {
				i = StoreWStr(fileName);
				if (n == 0) StoreWPtr(D + 4, i);
				n++;
			}
			else {
				for (std::string mask : v_masks_) {
					if (CmpStringWithMask(fileName, mask)) {
						i = StoreWStr(fileName);
						if (n == 0) StoreWPtr(D + 4, i);
						n++;
						break;
					}
					else {
						continue;
					}
				}
			}
		}
	}

	StoreWPtr(D + 8, n);
	StoreWPtr(DLast, 0);

	// process subdirs
	for (std::string subdir : dirs) {
		i = StoreDirD(RDir + subdir + "\\");
		StoreWPtr(DLast, i);
		DLast = i;
	}
}

//void TzFile::GetFilesInDir(const std::string& main_dir, const std::string& sub_dir)
//{
//	for (const fs::directory_entry& entry : fs::directory_iterator(main_dir + sub_dir)) {
//		std::string fileName = entry.path().filename().string();
//		if (entry.is_directory()) {
//			if (SubDirOpt) {
//				GetFilesInDir(main_dir, sub_dir + fileName + '\\');
//			}
//		}
//		else {
//			for (const std::string& mask : v_masks_) {
//				if (CmpStringWithMask(fileName, mask)) {
//					paths_[sub_dir].push_back(fileName);
//					break;
//				}
//			}
//		}
//	}
//}

void TzFile::GetDirs()
{
	int d = StoreDirD("");
	int dLast = d;
	do {
		Get1Dir(d, dLast);
		d = ReadWPtr(d);
	} while (d != 0);
	SeekH(WorkHandle, WBase);
	WrH(WorkHandle, WPos - WBase);
}

void TzFile::Reset()
{
	CVol = Vol;
	CPath = Path;
	Handle = OpenH(CPath, _isOldFile, RdOnly);
	Continued = false;
	/*if ((HandleError == 2) && Floppy) {
		inc(CPath[l - 2], 5);
		Handle = OpenH(_isOldFile, RdOnly);
		Continued = true;
	}*/
	TestCPathError();
	Size = FileSizeH(Handle);
	OrigSize = Size;
	RunMsgOn('C', Size);
}

void TzFile::Rewrite()
{
	CVol = Vol;
	CPath = Path;
	Handle = OpenH(CPath, _isOverwriteFile, Exclusive);
	TestCPathError();
	SpaceOnDisk = MyDiskFree(Floppy, drive_letter);
}

void TzFile::WriteBuf2()
{
	size_t i = 0;
	while (i < lBuf2) {
		if (SpaceOnDisk == 0) {
			CloseH(&Handle);
			CPath = Path;
			// TODO: change extension to *.5xx //inc(CPath[l - 2], 5);
			RenameFile56(Path, CPath, true);
			MountVol(false);
			Rewrite();
		}
		size_t n = lBuf2 - i;
		if (n > SpaceOnDisk) n = SpaceOnDisk;
		WriteH(Handle, n, &buffer2[i]);
		CPath = Path;
		TestCPathError();
		i += n;
		SpaceOnDisk -= (int32_t)n;
	}
	lBuf2 = 0;
}

void TzFile::RdH(HANDLE H, bool Skip)
{
	int sz;
	BYTE* a = (BYTE*)&sz;

	for (WORD i = 0; i <= 3; i++) {
		if (iBuf == lBuf) ReadBuf();
		a[i] = buffer1[iBuf];
		iBuf++;
	}
	while (sz > 0) {
		if (iBuf == lBuf) ReadBuf();
		WORD n = lBuf - iBuf;
		if (sz < n) n = sz;
		if (!Skip) WriteH(H, n, &buffer1[iBuf]);
		iBuf += n;
		sz -= n;
	}
}

void TzFile::WrH(HANDLE src_file, uint32_t file_size)
{
	RunMsgOn('C', (int32_t)file_size);

	size_t buf_index = 0;
	memcpy(&buffer1[buf_index], &file_size, 4);
	buf_index += 4;
	size_t free_space = BufSize - 4;
	uint32_t finished = 0;

	do {
		size_t rest;
		if (file_size - finished > free_space) rest = free_space;
		else rest = file_size - finished;

		if (rest > 0) {
			ReadH(src_file, rest, &buffer1[buf_index]);
		}

		lBuf = buf_index + rest;

		WriteBuf(false);

		buf_index = 0;
		free_space = BufSize;
		finished += rest;

		RunMsgN((int32_t)finished);
	} while (finished != file_size);

	RunMsgOff();
}

void TzFile::ProcFileList()
{
	void* p = nullptr;
	HANDLE h = nullptr;

	int d = 0;
	MarkStore(p);
	do {
		int dNext = ReadWPtr(d);
		int n = ReadWPtr(d + 8);
		int i = d + 12;
		std::string r_dir = ReadWStr(i);
		SetDir(r_dir);
		i = ReadWPtr(d + 4);
		while (n > 0) {
			std::string file_name = ReadWStr(i);
			n--;
			CPath = FExpand(file_name);
			CVol = "";
			if (IsBackup) {
				h = OpenH(CPath, _isOldFile, RdOnly);
				TestCPathError();
				WrH(h, FileSizeH(h));
			}
			else {
				bool skp = false;
				if (!OverwrOpt) {
					h = OpenH(CPath, _isNewFile, Exclusive);
					if (HandleError == 80) {
						SetMsgPar(CPath);
						if (PromptYN(780)) h = OpenH(CPath, _isOverwriteFile, Exclusive);
						else {
							skp = true;
							goto label1;
						}
					}
				}
				else h = OpenH(CPath, _isOverwriteFile, Exclusive);
				TestCPathError();
			label1:
				RdH(h, skp);
			}
			CloseH(&h);
		}
		d = dNext;
		ReleaseStore(&p);
	} while (d != 0);
}

void TzFile::Backup(std::string& mask)
{
	ParseMask(mask);
	MountVol(true);
	Rewrite();
	InitBufOutp();
	GetDirs();
	ProcFileList();
	WriteBuf(true);
}

void TzFile::Restore()
{
	MountVol(true);
	Reset();
	if (Size == 0) CloseH(&Handle);
	else {
		InitBufInp();
		SeekH(WorkHandle, WBase);
		RdH(WorkHandle, false);
		ProcFileList();
	}
	RunMsgOff();
}

void TzFile::ParseMask(const std::string& mask)
{
	v_masks_.clear();

	std::regex pattern("(\\\".*?\\\")|(\\S+)", std::regex_constants::icase);
	auto words_begin = std::sregex_iterator(mask.begin(), mask.end(), pattern);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		v_masks_.push_back(match.str());
	}
}