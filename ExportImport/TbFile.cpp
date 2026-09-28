#include "TbFile.h"

#include <cctype>
#include <windows.h>

#include "../Common/FileD.h"
#include "../Common/compare.h"
#include "../Common/CommonVariables.h"
#include "../Core/access.h"
#include "../Core/base.h"
#include "../Core/Catalog.h"
#include "../Core/GlobalVariables.h"
#include "../Core/legacy.h"
#include "../Core/oaccess.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"
#include "../fandbase/files.h"

namespace
{
	// next archive number from the list after the archive path ("A:\ZAL 2 3" -> 02, 03)
	std::string NextArchiveNr(const std::string& numbers, size_t& pos)
	{
		while (pos < numbers.length() && !isdigit((uint8_t)numbers[pos])) pos++;
		const size_t start = pos;
		while (pos < numbers.length() && isdigit((uint8_t)numbers[pos])) pos++;
		std::string nr = numbers.substr(start, pos - start);
		if (nr.length() == 1) nr = "0" + nr;
		return nr;
	}
}

TbFile::TbFile(bool compress) : TyFile(compress)
{
}

void TbFile::ResetF()
{
	Path = Dir + FName + Ext;
	Reset();
}

void TbFile::RewriteF()
{
	Path = Dir + FName + Ext;
	Rewrite();
}

// extension of the next archive file: .001 .. .009 .00A .. .00Z .010 ..
void TbFile::NextExt()
{
	Ext[1] = '0';
	switch (Ext[3]) {
	case '9': {
		Ext[3] = 'A';
		break;
	}
	case 'Z': {
		Ext[3] = '0';
		if (Ext[2] == '9') Ext[2] = 'A';
		else Ext[2]++;
		break;
	}
	default: {
		Ext[3]++;
		break;
	}
	}
}

// file which is not open (CPath = its path)
void TbFile::BackupH()
{
	HANDLE h = OpenH(CPath, _isOldFile, RdOnly);
	if (HandleError == ERROR_FILE_NOT_FOUND) {
		// missing file -> empty archive file
		RewriteF();
		InitBufOutp();
	}
	else {
		TestCPathError();
		RewriteF();
		InitBufOutp();
		const int sz = FileSizeH(h);
		RunMsgOn('C', sz);
		int i = 0;
		while (i < sz) {
			size_t n = BufSize;
			if ((size_t)(sz - i) < n) n = sz - i;
			i += (int)n;
			ReadH(h, n, buffer1);
			lBuf = n;
			WriteBuf(false);
			RunMsgN(i);
		}
		CloseH(&h);
		RunMsgOff();
	}
	WriteBuf(true);
	CloseArchive();
}

void TbFile::RestoreH()
{
	const std::string s = CPath;
	ResetF();
	HANDLE h = OpenH(s, _isOverwriteFile, Exclusive);
	CPath = s;
	TestCPathError();
	InitBufInp();
	while (!eof) {
		WriteH(h, lBuf, buffer1);
		CPath = s;
		TestCPathError();
		ReadBuf();
	}
	CloseArchive();
	const int l = FileSizeH(h);
	CloseH(&h);
	if (l == 0) DeleteFile(s.c_str()); // the file was missing during backup
	RunMsgOff();
}

void TbFile::BackupHFD(HANDLE h)
{
	RewriteF();
	InitBufOutp();
	const int sz = FileSizeH(h);
	RunMsgOn('C', sz);
	SeekH(h, 0);
	int i = 0;
	while (i < sz) {
		size_t n = BufSize;
		if ((size_t)(sz - i) < n) n = sz - i;
		i += (int)n;
		ReadH(h, n, buffer1);
		lBuf = n;
		WriteBuf(false);
		RunMsgN(i);
	}
	WriteBuf(true);
	CloseArchive();
	RunMsgOff();
}

// the archive file is open (ResetF + InitBufInp)
void TbFile::RestoreHFD(HANDLE h)
{
	SeekH(h, 0);
	while (!eof) {
		WriteH(h, lBuf, buffer1);
		ReadBuf();
	}
	TruncF(h, HandleError);
	CloseArchive();
	RunMsgOff();
}

void TbFile::BackupFD(FileD* file_d)
{
	const LockMode md = file_d->NewLockMode(RdMode);
	BackupHFD(file_d->GetHandle());
	if (file_d->HasTextFile()) {
		Ext[1] = 'T';
		BackupHFD(file_d->GetHandleT());
	}
	file_d->OldLockMode(md);
}

void TbFile::RestoreFD(FileD* file_d)
{
	ResetF();
	InitBufInp();
	const FandFileType typ = file_d->GetFandFileType();
	if (lBuf >= 6 && (typ == FandFileType::FAND16 || typ == FandFileType::INDEX)) {
		uint16_t l;
		memcpy(&l, &buffer1[4], 2);
		if (file_d->GetRecLen() != l) {
			SetMsgPar(Path);
			RunError(883);
		}
	}

	// opens the file if it is closed (creates it if it does not exist)
	const LockMode md = file_d->NewLockMode(ExclMode);
	RestoreHFD(file_d->GetHandle());
	if (file_d->HasTextFile()) {
		Ext[1] = 'T';
		ResetF();
		InitBufInp();
		RestoreHFD(file_d->GetHandleT());
	}
	const int err = file_d->RdPrefixes();
	if (err != 0) file_d->CFileError(err);
	if (file_d->FileType == DataFileType::FandFile) {
		file_d->FF->XFNotValid();
	}
	file_d->OldLockMode(md);
}

void TbFile::Backup(bool isBackup, WORD Ir)
{
	IsBackup = isBackup;
	SaveFiles();
	std::string ArNr = catalog->GetArchive(Ir);
	Vol = catalog->GetVolume(Ir);
	Path = catalog->GetPathName(Ir);
	std::string numbers;
	const size_t j = Path.find(' ');
	if (j != std::string::npos) {
		numbers = Path.substr(j);
		Path.erase(j);
	}
	Path = FExpand(Path);
	std::string n, e;
	FSplit(Path, Dir, n, e);
	Ext = ".000";
	drive_letter = Dir.empty() ? '\0' : Dir[0];
	MountVol(true);

	size_t num_pos = 0;
	while (true) {
		for (int i = 1; i <= catalog->GetCatalogFile()->GetNRecs(); i++) {
			if (EquUpCase(catalog->GetRdbName(i), "ARCHIVES")) continue;
			if (catalog->GetArchive(i) != ArNr) continue;

			std::string d;
			FSplit(catalog->GetPathName(i), d, FName, e);
			NextExt();

			FileD* file_d = nullptr;
			ForAllFDs(ForAllFilesOperation::find_fd_for_i, &file_d, i);
			if (file_d != nullptr) {
				const bool was_open = file_d->IsOpen();
				if (IsBackup) BackupFD(file_d);
				else RestoreFD(file_d);
				if (!was_open) file_d->CloseFile();
			}
			else {
				CPath = FExpand(catalog->GetPathName(i));
				CVol = catalog->GetVolume(i);
				TestMountVol(CPath[0]);
				if (IsBackup) BackupH();
				else RestoreH();
			}
		}

		ArNr = NextArchiveNr(numbers, num_pos);
		if (ArNr.empty()) break;
	}
}
