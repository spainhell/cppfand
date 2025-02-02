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

int TzFile::StoreWStr(pstring s)
{
	SeekH(WorkHandle, WPos);
	WriteH(WorkHandle, s.length() + 1, &s);
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

pstring TzFile::ReadWStr(int& Pos)
{
	pstring s;
	SeekH(WorkHandle, WBase + Pos);
	ReadH(WorkHandle, 1, &s);
	ReadH(WorkHandle, s.length(), &s[1]);
	Pos += s.length() + 1;
	return s;
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

/*void TzFile::Get1Dir(std::vector<std::string>& Msk, int D, int& DLast)
{
	SearchRec SR;
	PathStr p;
	int i, n;
	pstring RDir;
	StringList sl;

	i = D + 12;
	RDir = ReadWStr(i);
	p = Dir + RDir + "*.*";
	FindFirst(p + 00, 0, SR);
	n = 0;
	if (!(DosError() == 0 || DosError() == 18)) {
		SetMsgPar(p);
		RunError(904);
	}
	while (DosError() == 0) {
		if (Msk == nullptr) {
		label1:
			i = StoreWStr(SR.name);
			if (n == 0) StoreWPtr(D + 4, i);
			n++;
		}
		else {
			sl = Msk;
			while (sl != nullptr) {
				if (EqualsMask(SR.name[1], length(SR.name), sl->S)) goto label1;
				sl = (StringListEl*)sl->pChain;
			}
		}
		FindNext(SR);
	}
	StoreWPtr(D + 8, n);
	StoreWPtr(DLast, 0);
	if (!SubDirOpt) return;
	FindFirst(p + 00, Directory, SR);
	while (DosError() == 0) {
		if (((SR.Attr && Directory) == Directory) && (SR.name[1] != '.')) {
			i = StoreDirD(RDir + SR.name + "\'");
			StoreWPtr(DLast, i);
			DLast = i;
		}
		FindNext(SR);
	}
}*/

void TzFile::GetFilesInDir(const std::string& main_dir, const std::string& sub_dir)
{
	for (const fs::directory_entry& entry : fs::directory_iterator(main_dir + sub_dir)) {
		std::string fileName = entry.path().filename().string();
		if (entry.is_directory()) {
			if (SubDirOpt) {
				GetFilesInDir(main_dir, sub_dir + fileName + '\\');
			}
		}
		else {
			for (const std::string& mask : v_masks_) {
				if (CmpStringWithMask(fileName, mask)) {
					paths_[sub_dir].push_back(fileName);
					break;
				}
			}
		}
	}
}

void TzFile::WriteSubdirFiles(const std::vector<std::string>& files, uint32_t& index)
{
	for (size_t i = 0; i < files.size(); i++) {
		std::string file_name = files.at(i);
		uint8_t path_len = file_name.length() <= 255 ? (uint8_t)file_name.length() : 255;
		WriteH(Handle, 1, &path_len);
		WriteH(Handle, path_len, (char*)file_name.c_str());
		index += path_len + 1;
	}
}

void TzFile::WriteSubdirRecord(const std::string& sub_dir, uint32_t& index, uint32_t next_rec_address, uint32_t file_desr_address, uint32_t files_count)
{
	WriteH(Handle, 4, &next_rec_address);
	WriteH(Handle, 4, &file_desr_address);
	WriteH(Handle, 4, &files_count);

	uint8_t path_len = sub_dir.length() <= 255 ? (uint8_t)sub_dir.length() : 255;
	WriteH(Handle, 1, &path_len);
	WriteH(Handle, path_len, (char*)sub_dir.c_str());

	// 4B next record, 4B files list address, 4B files count, 1B for path length, path length
	index += 13 + path_len;
}


void TzFile::GetDirs()
{
	paths_.clear();

	GetFilesInDir(Dir, "");

	// calculate total size of the header
	uint32_t header_size = 0;
	for (const auto& path : paths_) {
		header_size += 13 + path.first.length();
		for (const auto& file : path.second) {
			header_size += file.length() + 1;
		}
	}

	uint32_t index = 0;
	SeekH(Handle, 0);
	WriteH(Handle, 4, &header_size);
	index += 4;

	for (const auto& path : paths_) {
		if (path.first.empty()) {
			if (path == *paths_.rbegin()) {
				// this is the last item in the map -> next record address is 0, files list are stored immediately after the header
				WriteSubdirRecord(path.first, index, 0, 13, path.second.size());
			}
			else {
				// calculate size of all files
				uint32_t files_size = 0;
				for (const auto& file : path.second) {
					files_size += file.length() + 1;
				}
				WriteSubdirRecord(path.first, index, files_size, 13, path.second.size());
			}
			// for root directory files are stored immediately after the header
			WriteSubdirFiles(path.second, index);
		}
		else {
			if (path == *paths_.rbegin()) {

			}
			else
			{
				
			}
			WriteSubdirRecord(path.first, index, 0, 0, path.second.size());
		}
	}




	//std::vector<std::string> slRoot;
	//std::vector<std::string> sl;
	//pstring s;
	//WORD n;

	////slRoot = nullptr;
	//WORD l = mask.length();
	//WORD j = 0;
	//do {
	//	while ((j < l) && (mask[j] == ' ' || mask[j] == ',')) {
	//		j++;
	//	}
	//	n = l - j + 1;
	//	for (WORD i = j; i < l; i++) {
	//		if (mask[i] == ' ' || mask[i] == ',') {
	//			n = i - j;
	//			break;
	//		}
	//		if (n > 0) {
	//			n = MinW(n, 255);
	//			//sl = new StringListEl(); // GetZStore(5 + n);
	//			//ChainLast(slRoot, sl);
	//			//Move(&Mask->A[j], &sl->S[1], n);
	//			//sl->S[0] = char(n);
	//			std::string item = std::string(&mask[j], n);
	//			slRoot.push_back(item);
	//			j += n;
	//		}
	//	}
	//} while (n != 0);
	//int d = StoreDirD("");
	//int dLast = d;
	//do {
	//	Get1Dir(slRoot, d, dLast);
	//	d = ReadWPtr(d);
	//} while (d != 0);
	//SeekH(WorkHandle, WBase);
	//WrH(WorkHandle, WPos - WBase);
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

	for (auto& dir_pair : paths_)
	{
		for (size_t i = 0; i < dir_pair.second.size(); i++) {
			std::string f_name = dir_pair.second.at(i);
			CPath = FExpand(dir_pair.first + f_name);
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
						if (PromptYN(780)) {
							h = OpenH(CPath, _isOverwriteFile, Exclusive);
						}
						else {
							skp = true;
							RdH(h, skp);
							CloseH(&h);
							break;
						}
					}
				}
				else {
					h = OpenH(CPath, _isOverwriteFile, Exclusive);
				}
				TestCPathError();
				RdH(h, skp);
			}
			CloseH(&h);
		}
	}


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
