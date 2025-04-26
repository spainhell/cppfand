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


void lock_excl_and_write_prefix(FileD* file_d)
{
	if (file_d->IsShared() && (file_d->GetLMode() < ExclMode)) {
		file_d->ChangeLockMode(ExclMode, 0, false);
	}
	file_d->SetLMode(ExclMode);
	file_d->SetUpdateFlag();
	file_d->WrPrefix();
}

void CreateF(FileD* file_d)
{
	std::string path = SetPathMountVolumeSetNet(file_d, Exclusive);
	HANDLE h = OpenH(path, _isOverwriteFile, Exclusive);
	file_d->SetHandle(h);
	TestCFileError(file_d);
	file_d->SetNRecs(0);

	if (file_d->HasTextFile()) {
		path = file_d->CExtToT(CDir, CName, CExt);
		file_d->CreateT(path);
	}

	if (file_d->HasIndexFile() && file_d->FF->file_type == FandFileType::INDEX) {
		path = CExtToX(CDir, CName, CExt);
		file_d->FF->XF->Handle = OpenH(path, _isOverwriteFile, Exclusive);
		file_d->FF->XF->TestErr(); /*SetNotValid*/
		file_d->FF->XF->SetEmpty(file_d->FF->NRecs, file_d->GetNrKeys());
	}

	file_d->SeekRec(0);
	file_d->SetUpdateFlag();
}

bool OpenCreateF(FileD* file_d, const std::string& path, FileUseMode UM)
{
	if (!OpenF(file_d, path, UM)) {
		CreateF(file_d);
		if ((UM == Shared) || (UM == RdShared)) {
			file_d->WrPrefixes();

			if (file_d->FileType == DataFileType::FandFile) {
				SaveCache(0, file_d->FF->Handle);
				CloseClearH(&file_d->FF->Handle);

				if (file_d->FF->file_type == FandFileType::INDEX) {
					CloseClearH(&file_d->FF->XF->Handle);
				}

				if (file_d->FF->TF != nullptr) {
					CloseClearH(&file_d->FF->TF->Handle);
				}
			}
			else if (file_d->FileType == DataFileType::DBF) {
				SaveCache(0, file_d->DbfF->Handle);
				CloseClearH(&file_d->DbfF->Handle);

				if (file_d->DbfF->TF != nullptr) {
					CloseClearH(&file_d->DbfF->TF->Handle);
				}
			}
			else {
				// other types don't have index or text files
			}
			
			// TODO: here is probably an issue with file caching
			// wait 100 ms before re-open
			//std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
	file_d->Close();
	GoExit(MsgLine);
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
	file_d->SetUMode(UM);
	file_d->SetDrive((BYTE)TestMountVol(path[0]));
	if (!IsNetCVol() || (file_d == Chpt))
		switch (UM) {
		case RdShared: file_d->SetUMode(RdOnly); break;
		case Shared: file_d->SetUMode(Exclusive); break;

		case Closed:
		case RdOnly:
		case Exclusive: break;
		}
	else if ((UM == Shared) && EquUpCase(CVol, "#R")) {
		file_d->SetUMode(RdShared);
	}
	CPath = path;
	return path;
}

std::string SetPathAndVolume(FileD* file_d, char pathDelim)
{
	bool isRdb = false;

	CVol = "";
	if (file_d->FileType == DataFileType::FandFile && file_d->FF->file_type == FandFileType::CAT) {
		CDir = GetEnv("FANDCAT");
		if (CDir.empty()) {
			CDir = TopDataDir.empty() ? TopRdbDir : TopDataDir;
		}
		AddBackSlash(CDir);
		CName = CatFDName;
		CExt = ".CAT";
		goto finish;
	}

	if (file_d->CatIRec != 0) {
		catalog->GetPathAndVolume(file_d, file_d->CatIRec, CPath, CVol);
		FSplit(CPath, CDir, CName, CExt);
		if (file_d->Name == "@") {
			CName = file_d->Name;
			goto finish;
		}
		else {
			goto finish;
		}
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

	if (SetContextDir(file_d, CDir, isRdb)) {
		// do nothing
	}
	else {
		if (file_d == HelpFD) {
			CDir = FandDir;
#ifdef FandRunV 
			CName = "UFANDHLP";
#else
			CName = "FANDHLP";
#endif
			goto finish;
		}
		CExt = ".100";
		if (CRdb != nullptr) {
			CDir = CRdb->DataDir;
		}
		else {
			CDir = "";
		}
	}

	AddBackSlash(CDir);
	CName = file_d->Name;

finish:
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
