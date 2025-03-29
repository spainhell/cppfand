#include "files.h"

#include <chrono>
#include <thread>

#include "../Core/GlobalVariables.h"
#include "../Core/Coding.h"
#include "../Core/obaseww.h"
#include "../Core/oaccess.h"
#include "../Core/wwmix.h"
#include "../Common/compare.h"
#include "../Common/textfunc.h"


bool OpenF(FileD* file_d, const std::string& path, FileUseMode UM)
{
	bool result = true;
	if (file_d->IsOpen()) return result;
	if (OpenF1(file_d, path, UM)) {
		if (file_d->IsShared()) {
			file_d->ChangeLockMode(RdMode, 0, false);
			file_d->SetLMode(RdMode);
		}
		result = OpenF2(file_d, path);
		file_d->OldLockMode(NullMode);
	}
	else {
		result = false;
	}
	return result;
}

bool OpenF1(FileD* file_d, const std::string& path, FileUseMode UM)
{
	WORD n;
	bool result = true;
	file_d->SetLMode(NullMode);
	SetPathMountVolumeSetNet(file_d, UM);
	const bool b = (file_d == Chpt) || (file_d == catalog->GetCatalogFile());
	if (b && (IsTestRun || IsInstallRun) && ((GetFileAttr(CPath, HandleError) & 0b00000001/*RdOnly*/) != 0)) {
		SetFileAttr(CPath, HandleError, GetFileAttr(CPath, HandleError) & 0b00100110);
		if (HandleError == 5) HandleError = 79;
		TestCFileError(file_d);
		file_d->SetWasRdOnly(true);
	}
	while (true) {
		HANDLE h = OpenH(CPath, _isOldFile, file_d->GetUMode());
		file_d->SetHandle(h);
		if ((HandleError != 0) && file_d->GetWasRdOnly()) {
			SetFileAttr(CPath, HandleError, (GetFileAttr(CPath, HandleError) & 0b00100111) | 0b00000001 /*RdONly*/);
			TestCFileError(file_d);
		}
		if ((HandleError == 5) && (file_d->GetUMode() == Exclusive)) {
			file_d->SetUMode(RdOnly);
			continue;
		}
		if (HandleError == 2) {
			result = false;
			return result;
		}
		break;
	}
#ifndef FandNetV
	if ((HandleError == 5 || HandleError == 0x21) &&
		((CVol == '#') || (CVol == "##") || SEquUpcase(CVol, "#R"))) CFileError(842);
#endif
	TestCFileError(file_d);

	// open text file (.T__)
	if (file_d->HasTextFile()) {
		CPath = file_d->CExtToT(CDir, CName, CExt);
		if (file_d->GetWasRdOnly()) {
			SetFileAttr(CPath, HandleError, GetFileAttr(CPath, HandleError) & 0b00100110); // 0x26 = archive + hidden + system
		}
		while (true) {
			HANDLE h = OpenH(CPath, _isOldFile, file_d->GetUMode());
			file_d->SetHandleT(h);
			if (HandleError == 2) {
				if (file_d->FileType == DataFileType::DBF && file_d->DbfF->TF->Format == DbfTFile::DbtFormat) {
					file_d->DbfF->TF->Format = DbfTFile::FptFormat;
					CExt = ".FPT";
					CPath = CDir + CName + CExt;
					continue;
				}
				if (file_d->IsDynFile) {
					if (file_d->FileType == DataFileType::FandFile) {
						CloseClearH(&file_d->FF->Handle);
					}
					else if (file_d->FileType == DataFileType::DBF) {
						CloseClearH(&file_d->DbfF->Handle);
					}
					result = false;
					return result;
				}
			}
			break;
		}

		if (HandleError != 0) {
			n = HandleError;
			file_d->Close();
			HandleError = n;
			TestCPathError();
			return result;
		}
	}

	// open index file (*.X__)
	if (file_d->HasIndexFile()) {
		CPath = CExtToX(CDir, CName, CExt);
		while (true) {
			file_d->FF->XF->Handle = OpenH(CPath, _isOldFile, file_d->FF->UMode);
			if (HandleError == 2) {
				file_d->FF->XF->Handle = OpenH(CPath, _isOverwriteFile, Exclusive);
				if (HandleError != 0) {
					n = HandleError;
					file_d->FF->Close(); //CloseClearH(file_d->FF);
					HandleError = n;
					TestCPathError();
					return result;
				}
				file_d->FF->XF->SetNotValid(file_d->FF->NRecs, file_d->GetNrKeys());
				CloseH(&file_d->FF->XF->Handle);
				continue;
			}
			if (HandleError != 0) {
				n = HandleError;
				file_d->FF->Close(); //CloseClearH(file_d->FF);
				HandleError = n;
				TestCPathError();
			}
			if (file_d->FF->XF != nullptr && FileSizeH(file_d->FF->XF->Handle) < 512) {
				file_d->FF->XF->SetNotValid(file_d->FF->NRecs, file_d->GetNrKeys());
			}
			break;
		}
	}
	return result;
}

void check_T_file(FileD* file_d, int file_size)
{
	if (file_d->FF->TF != nullptr) {
		if (file_size < file_d->FF->FirstRecPos) {
			file_d->FF->TF->SetEmpty();
		}
		else {
			file_d->FF->TF->RdPrefix(true);
			if ((file_d->FF->file_type == FandFileType::RDB)
				&& !file_d->IsActiveRdb()
				&& !Coding::HasPassword(file_d, 1, ""))
			{
				FileMsg(file_d, 616, ' ');
				CloseGoExit(file_d->FF);
			}
		}
	}
}

void check_X_file(FileD* file_d, int file_size)
{
	if (file_d->FileType == DataFileType::FandFile && file_d->FF->file_type == FandFileType::INDEX) {
		if (file_size < file_d->FF->FirstRecPos) {
			file_d->FF->XF->SetNotValid(file_d->FF->NRecs, file_d->GetNrKeys());
		}
		else {
			WORD Signum = 0;
			file_d->FF->XF->ReadData(0, 2, &Signum);
			file_d->FF->XF->RdPrefix();

			Fand0File* ff = file_d->FF;
			if (
				!ff->XF->NotValid && ((Signum != 0x04FF)
					|| (ff->XF->NRecsAbs != ff->NRecs)
					|| (ff->XF->FreeRoot > ff->XF->MaxPage)
					|| ((ff->XF->MaxPage + 1) << XPageShft) > FileSizeH(ff->XF->Handle))
				|| (ff->XF->NrKeys != 0) && (ff->XF->NrKeys != file_d->GetNrKeys()))
			{

				if (!EquUpCase(GetEnv("FANDMSG830"), "NO")) {
					FileMsg(file_d, 830, 'X');
				}

				if (ff->IsShared() && (ff->LMode < ExclMode)) {
					file_d->ChangeLockMode(ExclMode, 0, false);
				}

				ff->LMode = ExclMode;
				ff->XF->SetNotValid(ff->NRecs, file_d->GetNrKeys());
			}
		}
	}
	else {
		// this data file doesn't have index file
	}
}

void lock_excl_and_write_prefix(FileD* file_d)
{
	if (file_d->FF->IsShared() && (file_d->FF->LMode < ExclMode)) {
		file_d->ChangeLockMode(ExclMode, 0, false);
	}
	file_d->FF->LMode = ExclMode;
	file_d->FF->SetUpdateFlag();
	file_d->FF->WrPrefix();
}

bool OpenF2(FileD* file_d, const std::string& path)
{
	int file_size = file_d->GetFileSize();
	file_d->SetNRecs(0);

	if (file_size < file_d->GetFirstRecPos()) {
		lock_excl_and_write_prefix(file_d);
	}
	else {
		uint16_t rLen = file_d->RdPrefix();
		int32_t n = (file_size - file_d->GetFirstRecPos()) / file_d->GetRecLen();

		if (rLen != 0xffff) {
			if (file_d->IsDynFile) {
				file_d->Close(); //CloseClearH(file_d->FF);
				return false;
			}
			else {
				if (catalog->OldToNewCat(file_size)) {
					check_T_file(file_d, file_size);
					check_X_file(file_d, file_size);
					file_d->SeekRec(0);
					return true;
				}

				FileMsg(file_d, 883, ' ');
				int l = file_d->GetNRecs() * rLen + file_d->GetFirstRecPos();

				if (l == file_size || !PromptYN(885)) {
					CloseGoExit(file_d->FF);
				}

				if (file_d->GetNRecs() == 0 || l >> CachePageShft != file_size >> CachePageShft) {
					WrLLF10Msg(886);
					file_d->SetNRecs(n);
				}

				file_d->SetUpdateFlag();
				file_d->WrPrefix();
			}
		}
		else {
			if (n < file_d->GetNRecs()) {
				SetPathAndVolume(file_d);
				SetMsgPar(CPath);
				if (PromptYN(882)) {
					file_d->SetNRecs(n);
					lock_excl_and_write_prefix(file_d);
				}
				else {
					CloseGoExit(file_d->FF);
				}
			}
		}
	}

	check_T_file(file_d, file_size);
	check_X_file(file_d, file_size);

	file_d->SeekRec(0);
	return true;
}

void CreateF(FileD* file_d)
{
	std::string path = SetPathMountVolumeSetNet(file_d, Exclusive);
	file_d->FF->Handle = OpenH(path, _isOverwriteFile, Exclusive);
	TestCFileError(file_d);
	file_d->FF->NRecs = 0;

	if (file_d->FF->TF != nullptr) {
		path = file_d->CExtToT(CDir, CName, CExt);
		file_d->FF->TF->Create(path);
	}

	if (file_d->FF->file_type == FandFileType::INDEX) {
		path = CExtToX(CDir, CName, CExt);
		file_d->FF->XF->Handle = OpenH(path, _isOverwriteFile, Exclusive);
		file_d->FF->XF->TestErr(); /*SetNotValid*/
		file_d->FF->XF->SetEmpty(file_d->FF->NRecs, file_d->GetNrKeys());
	}

	file_d->SeekRec(0);
	file_d->FF->SetUpdateFlag(); //SetUpdHandle(file_d->FF->Handle);
}

bool OpenCreateF(FileD* file_d, const std::string& path, FileUseMode UM)
{
	if (!OpenF(file_d, path, UM)) {
		CreateF(file_d);
		if ((UM == Shared) || (UM == RdShared)) {
			file_d->FF->WrPrefixes();
			SaveCache(0, file_d->FF->Handle);
			CloseClearH(&file_d->FF->Handle);

			if (file_d->FF->file_type == FandFileType::INDEX) {
				CloseClearH(&file_d->FF->XF->Handle);
			}

			if (file_d->FF->TF != nullptr) {
				CloseClearH(&file_d->FF->TF->Handle);
			}

			// TODO: here is probably an issue with file caching
			// wait 100 ms before re-open
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			OpenF(file_d, path, UM);
		}
	}
	return true;
}

void CopyH(HANDLE h1, HANDLE h2)
{
	const WORD BufSize = 32768;
	void* p = new BYTE[BufSize];
	int sz = FileSizeH(h1);
	SeekH(h1, 0);
	SeekH(h2, 0);
	while (sz > BufSize) {
		ReadH(h1, BufSize, p);
		WriteH(h2, BufSize, p);
		sz -= BufSize;
	}
	ReadH(h1, sz, p);
	WriteH(h2, sz, p);
	CloseH(&h1);
	MyDeleteFile(CPath);
	ReleaseStore(&p);
}

void CFileError(FileD* file_d, int N)
{
	FileMsg(file_d, N, '0');
	CloseGoExit(file_d->FF);
}

void TestCFileError(FileD* file_d)
{
	if (HandleError != 0) {
		CFileError(file_d, 700 + HandleError);
	}
}

std::string SetPathMountVolumeSetNet(FileD* file_d, FileUseMode UM)
{
	std::string path = SetPathAndVolume(file_d);
	file_d->FF->UMode = UM;
	file_d->FF->Drive = (BYTE)TestMountVol(path[0]);
	if (!IsNetCVol() || (file_d == Chpt))
		switch (UM) {
		case RdShared: file_d->FF->UMode = RdOnly; break;
		case Shared: file_d->FF->UMode = Exclusive; break;

		case Closed:
		case RdOnly:
		case Exclusive: break;
		}
	else if ((UM == Shared) && EquUpCase(CVol, "#R")) {
		file_d->FF->UMode = RdShared;
	}
	CPath = path;
	return path;
}

std::string SetPathAndVolume(FileD* file_d, char pathDelim)
{
	int i = 0;
	bool isRdb = false;

	CVol = "";
	if (file_d->FF->file_type == FandFileType::CAT) {
		CDir = GetEnv("FANDCAT");
		if (CDir.empty()) {
			CDir = TopDataDir.empty() ? TopRdbDir : TopDataDir;
		}
		AddBackSlash(CDir);
		CName = CatFDName;
		CExt = ".CAT";
		goto label4;
	}
	i = file_d->CatIRec;
	if (i != 0) {
		catalog->GetPathAndVolume(file_d, i, CPath, CVol);
		FSplit(CPath, CDir, CName, CExt);
		if (file_d->Name == "@") goto label3;
		goto label4;
	}

	switch (file_d->FileType) {
	case DataFileType::FandFile: {
		switch (file_d->FF->file_type) {
		case FandFileType::RDB: {
			CExt = ".RDB";
			break;
		}
		case FandFileType::FAND8: {
			CExt = ".DTA";
			break;
		}
		default: {
			CExt = ".000";
			break;
		}
		}
		break;
	}
	case DataFileType::DBF: {
		CExt = ".DBF";
		break;
	}
	default:
		// other types don't have an extension
		break;
	}

	if (SetContextDir(file_d, CDir, isRdb)) goto label2;
	if (file_d == HelpFD) {
		CDir = FandDir;
#ifdef FandRunV 
		CName = "UFANDHLP";
#else
		CName = "FANDHLP";
#endif
		goto label4;
	}
	CExt = ".100";
	if (CRdb != nullptr) {
		CDir = CRdb->DataDir;
	}
	else {
		CDir = "";
	}
label2:
	AddBackSlash(CDir);
label3:
	CName = file_d->Name;
label4:
	if (pathDelim == '/') ReplaceChar(CDir, '\\', '/');
	if (pathDelim == '\\') ReplaceChar(CDir, '/', '\\');
	CPath = CDir + CName + CExt;
	return CPath;
}

std::string SetPathForH(HANDLE handle)
{
	RdbD* RD = CRdb;
	while (RD != nullptr) {
		for (FileD* fd : RD->v_files) {
			if (fd->FF->Handle == handle) {
				SetPathAndVolume(fd);
				return CPath;
			}

			if (fd->FF->XF != nullptr && fd->FF->XF->Handle == handle) {
				SetPathAndVolume(fd);
				CPath = CExtToX(CDir, CName, CExt);
				return CPath;
			}

			if (fd->FF->TF != nullptr && fd->FF->TF->Handle == handle) {
				SetPathAndVolume(fd);
				CPath = fd->CExtToT(CDir, CName, CExt);
				return CPath;
			}
		}
		RD = RD->ChainBack;
	}
	ReadMessage(799);
	CPath = MsgLine;
	return CPath;
}
