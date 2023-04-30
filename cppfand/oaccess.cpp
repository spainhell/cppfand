#include "oaccess.h"

#include "base.h"
#include "compile.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "obaseww.h"
#include "../fandio/FandTFile.h"
#include "../fandio/FandXFile.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "../fandio/files.h"

void OpenXWorkH()
{
	CVol = "";
	FileOpenMode m = _isOldNewFile;
	if (XWork.MaxPage == 0) m = _isOverwriteFile;
	CPath = FandWorkXName;
	XWork.Handle = OpenH(CPath, m, Exclusive);
	XWork.TestErr();
	if (FileSizeH(XWork.Handle) == 0) { XWork.FreeRoot = 0; XWork.MaxPage = 0; }
}

void OpenTWorkH()
{
	CVol = "";
	if (TWork.MaxPage == 0) {
		CPath = FandWorkTName;
		TWork.IsWork = true;
		TWork.Create();
	}
	else {
		CPath = FandWorkTName;
		CVol = "";
		TWork.Handle = OpenH(CPath, _isOldNewFile, Exclusive);
		TWork.TestErr();
	}
}

void SaveFiles()
{
	if (!CacheExist()) return;

	// save catalog
	FileD* catalog_file = CatFD->GetCatalogFile();
	catalog_file->FF->WrPrefixes();

	ForAllFDs(ForAllFilesOperation::close);

	bool b = SaveCache(0, catalog_file->FF->Handle);
	FlushHandles();

	if (!b) GoExit();
}

void CloseFANDFiles(bool FromDML)
{
	RdbD* RD = CRdb;
	while (RD != nullptr) {
		CFile = RD->FD;
		while (CFile != nullptr) {
			if (!FromDML) {
				CFile->FF->ExLMode = CFile->FF->LMode;
			}
			CloseFile(CFile);
			CFile = CFile->pChain;
		}
		RD = RD->ChainBack;
	}
	if (CRdb != nullptr) {
		CFile = CatFD->GetCatalogFile();
		CloseFile(CFile);
	}
	CFile = HelpFD;
	CloseFile(CFile);
	CloseH(&TWork.Handle);
	CloseH(&XWork.Handle);
}

void OpenFANDFiles(bool FromDML)
{
	RdbD* RD = nullptr;
	LockMode md = NullMode;

	OpenXWorkH();
	OpenTWorkH();
	CFile = HelpFD;
	OpenF(CFile, CPath, RdOnly);
	if (CRdb == nullptr) return;
	CFile = CatFD->GetCatalogFile();
	OpenF(CFile, CPath, Exclusive);
	RD = CRdb;
	while (RD != nullptr) {
		CFile = RD->FD;
		if (IsTestRun) {
			OpenF(CFile, CPath, Exclusive);
		}
		else {
			OpenF(CFile, CPath, RdOnly);
		}
		CFile = CFile->pChain;
		while (!FromDML && (CFile != nullptr)) {
			if (CFile->FF->ExLMode != NullMode) {
				OpenF(CFile, CPath, Shared);
				md = CFile->NewLockMode(CFile->FF->ExLMode);
			}
			CFile = CFile->pChain;
		}
		RD = RD->ChainBack;
	}

}

void CreateF(FileD* file_d)
{
	SetCPathMountVolSetNet(file_d, Exclusive);
	file_d->FF->Handle = OpenH(CPath, _isOverwriteFile, Exclusive);
	TestCFileError(file_d);
	file_d->FF->NRecs = 0;
	if (file_d->FF->TF != nullptr) {
		CPath = CExtToT(CDir, CName, CExt);
		file_d->FF->TF->Create();
	}
	if (file_d->FF->file_type == FileType::INDEX) {
		CPath = CExtToX(CDir, CName, CExt);
		file_d->FF->XF->Handle = OpenH(CPath, _isOverwriteFile, Exclusive);
		file_d->FF->XF->TestErr(); /*SetNotValid*/
		file_d->FF->XF->SetEmpty(file_d->FF->NRecs, file_d->GetNrKeys());
	}
	file_d->SeekRec(0);
	SetUpdHandle(file_d->FF->Handle);
}

bool OpenCreateF(FileD* file_d, FileUseMode UM)
{
	if (!OpenF(file_d, CPath, UM)) {
		CreateF(file_d);
		if ((UM == Shared) || (UM == RdShared)) {
			file_d->FF->WrPrefixes();
			SaveCache(0, file_d->FF->Handle);
			CloseClearH(&file_d->FF->Handle);
			if (file_d->FF->file_type == FileType::INDEX) {
				CloseClearH(&file_d->FF->XF->Handle);
			}
			if (file_d->FF->TF != nullptr) {
				CloseClearH(&file_d->FF->TF->Handle);
			}
			OpenF(file_d, CPath, UM);
		}
	}
	return true;
}

LockMode RewriteF(FileD* file_d, const bool Append)
{
	LockMode result;
	if (Append) {
		result = file_d->NewLockMode(CrMode);
		file_d->SeekRec(file_d->FF->NRecs);
		if (file_d->FF->XF != nullptr) {
			file_d->FF->XF->FirstDupl = true;
			CFile->FF->TestXFExist();
		}
		return result;
	}
	result = file_d->NewLockMode(ExclMode);
	file_d->FF->NRecs = 0;
	file_d->SeekRec(0);
	SetUpdHandle(file_d->FF->Handle);

	int notValid = file_d->FF->XFNotValid();
	if (notValid != 0) {
		RunError(notValid);
	}

	if (file_d->FF->file_type == FileType::INDEX) file_d->FF->XF->NoCreate = true;
	if (file_d->FF->TF != nullptr) file_d->FF->TF->SetEmpty();
	return result;
}

void TruncF(FileD* file_d)
{
	if (file_d->FF->UMode == RdOnly) return;
	LockMode md = file_d->NewLockMode(RdMode);
	TruncH(file_d->FF->Handle, file_d->FF->UsedFileSize());
	if (HandleError != 0) {
		FileMsg(CFile, 700 + HandleError, '0');
	}
	if (file_d->FF->TF != nullptr) {
		TruncH(file_d->FF->TF->Handle, file_d->FF->TF->UsedFileSize());
		file_d->FF->TF->TestErr();
	}
	if (file_d->FF->file_type == FileType::INDEX) {
		int sz = file_d->FF->XF->UsedFileSize();
		if (file_d->FF->XF->NotValid) sz = 0;
		TruncH(file_d->FF->XF->Handle, sz);
		file_d->FF->XF->TestErr();
	}
	file_d->OldLockMode(md);
}

void CloseFile(FileD* file_d)
{
	if (file_d->FF->Handle == nullptr) return;
	if (file_d->FF->IsShared()) {
		file_d->OldLockMode(NullMode);
	}
	else {
		file_d->FF->WrPrefixes();
	}
	SaveCache(0, file_d->FF->Handle);
	TruncF(file_d);
	if (file_d->FF->file_type == FileType::INDEX) {
		if (file_d->FF->XF->Handle != nullptr) {
			CloseClearH(&file_d->FF->XF->Handle);
			if (!file_d->FF->IsShared()) {
				if (file_d->FF->XF->NotValid) {
					SetCPathVol(file_d);
					CPath = CExtToX(CDir, CName, CExt);
					MyDeleteFile(CPath);
				}
				else if ((file_d->FF->XF->NRecs == 0) || file_d->FF->NRecs == 0) {
					file_d->FF->NRecs = 0;
					SetCPathVol(file_d);
					CPath = CExtToX(CDir, CName, CExt);
					MyDeleteFile(CPath);
				}
			}
		}
	}
	// zavreni souboru volnych textu .T00
	if (file_d->FF->TF != nullptr) {
		if (file_d->FF->TF->Handle != nullptr) {
			CloseClearH(&file_d->FF->TF->Handle);
			if (HandleError == 0) file_d->FF->TF->Handle = nullptr; // soubor byl uspesne uzavren
			if ((!file_d->FF->IsShared()) && (file_d->FF->NRecs == 0) && (file_d->FF->file_type != FileType::DBF)) {
				SetCPathVol(file_d);
				CPath = CExtToT(CDir, CName, CExt);
				MyDeleteFile(CPath);
			}
		}
	}
	CloseClearH(&file_d->FF->Handle);
	if (HandleError == 0) file_d->FF->Handle = nullptr;
	file_d->FF->LMode = NullMode;
	if (!file_d->FF->IsShared() && (file_d->FF->NRecs == 0) && (file_d->FF->file_type != FileType::DBF)) {
		SetCPathVol(file_d);
		MyDeleteFile(CPath);
	}
	if (file_d->FF->WasRdOnly) {
		file_d->FF->WasRdOnly = false;
		SetCPathVol(file_d);
		SetFileAttr(CPath, HandleError, (GetFileAttr(CPath, HandleError) & 0x27) | 0x01); // {RdOnly; }
		if (file_d->FF->TF != nullptr) {
			CPath = CExtToT(CDir, CName, CExt);
			SetFileAttr(CPath, HandleError, (GetFileAttr(CPath, HandleError) & 0x27) | 0x01); //  {RdOnly; }
		}
	}
}


void CloseFilesAfter(FileD* FD)
{
	CFile = FD;

	while (CFile != nullptr) {
		CloseFile(CFile);
		CFile = CFile->pChain;
	}
}

bool ActiveRdbOnDrive(WORD D)
{
	auto result = true;
	RdbD* R = CRdb;
	while (R != nullptr)
	{
		if (R->FD->FF->Drive == D) return result;
		R = R->ChainBack;
	}
	result = false;
	return result;
}

void CloseFilesOnDrive(WORD D)
{
	RdbD* R = CRdb;
	FileD* CF = CFile;
	while (R != nullptr) {
		CFile = R->FD;
		while (CFile != nullptr) {
			if (CFile->FF->Drive == D) {
				CloseFile(CFile);
			}
			CFile = CFile->pChain;
		}
		R = R->ChainBack;
	}
	CFile = CF;
}

WORD TestMountVol(char DriveC)
{
	WORD D = 0, i = 0;
	std::string Vol;
	pstring Drive(1);
	Drive = "A";

	if (IsNetCVol()) return 0;
	D = toupper(DriveC) - '@';
	if (D >= FloppyDrives) {
		if (!CDir.empty() && toupper(CDir[0]) == spec.CPMdrive) {
			D = FloppyDrives;
		}
		else {
			return 0;
		}
	}

	const std::string MountedVolD = MountedVol[D - 1];
	if (CVol.empty() || EquUpCase(MountedVolD, CVol)) {
		goto label3;
	}

	Drive[1] = DriveC;
	if (ActiveRdbOnDrive(D)) {
		SetMsgPar(Drive, CVol, MountedVol[D - 1]);
		RunError(812);
	}
	Vol = CVol;
	CloseFilesOnDrive(D);
	CVol = Vol;
label1:
	F10SpecKey = VK_ESCAPE;
	SetMsgPar(Drive, CVol);
	WrLLF10Msg(808);
	if (Event.Pressed.KeyCombination() == __ESC) { if (PromptYN(21)) { GoExit(); } }
	else goto label1;
	//if (D == FloppyDrives) FindFirst(Drive + ":\\*.VOL", 0, S);
	//else FindFirst(Drive + ":\\*.*", VolumeID, S);
	switch (DosError()) {
	case 18/*label missing*/: { WrLLF10Msg(809); goto label1; break; }
	case 0: break;
	default: WrLLF10Msg(810); goto label1; break;
	}
	//i = S.Name.First('.');
	//if (D == FloppyDrives) S.Name.Delete(i, 255);
	//else if (i != 0) S.Name.Delete(i, 1);
	//if (!SEquUpcase(S.Name, CVol))
	//{
	//	SetMsgPar(S.Name); WrLLF10Msg(817); goto label1;
	//}
// label2:
	MountedVol[D - 1] = CVol;
label3:
	return D;
}

void ReleaseDrive(WORD D)
{
	pstring Drive(1);
	if (MountedVol[D - 1].empty()) return;
	if (D == FloppyDrives) Drive = spec.CPMdrive;
	else Drive = char(D + '@');
	if (ActiveRdbOnDrive(D)) {
		SetMsgPar(Drive);
		RunError(813);
	}
	CloseFilesOnDrive(D);
	SetMsgPar(MountedVol[D - 1], Drive);
	WrLLF10Msg(818);
	MountedVol[D - 1] = "";
}

void SetCPathForH(FILE* handle)
{
	RdbD* RD = nullptr;
	FileD* cf = nullptr;
	cf = CFile;
	RD = CRdb;
	while (RD != nullptr) {
		CFile = RD->FD;
		while (CFile != nullptr) {
			if (CFile->FF->Handle == handle) {
				SetCPathVol(CFile);
				CFile = cf;
				return;
			}
			if (CFile->FF->XF != nullptr && CFile->FF->XF->Handle == handle) {
				SetCPathVol(CFile);
				CPath = CExtToX(CDir, CName, CExt);
				CFile = cf;
				return;
			}
			if (CFile->FF->TF != nullptr && CFile->FF->TF->Handle == handle) {
				SetCPathVol(CFile);
				CPath = CExtToT(CDir, CName, CExt);
				CFile = cf;
				return;
			}
			CFile = CFile->pChain;
		}
		RD = RD->ChainBack;
	}
	ReadMessage(799);
	CPath = MsgLine;
	CFile = cf;
}

bool SetContextDir(FileD* file_d, std::string& D, bool& IsRdb)
{
	bool result = true;
	RdbD* R = CRdb;
	IsRdb = false;
	while (R != nullptr) {
		FileD* F = R->FD;
		if ((file_d == F) && (file_d->CatIRec != 0)) {
			D = R->RdbDir;
			IsRdb = true;
			return result;
		}
		while (F != nullptr) {
			if (file_d == F) {
				if ((file_d == R->HelpFD) || (file_d->FF->file_type == FileType::RDB))  //.RDB
					D = R->RdbDir;
				else D = R->DataDir;
				return result;
			}
			F = F->pChain;
		}
		R = R->ChainBack;
	}
	return false;
}

std::string SetCPathVol(FileD* file_d, char pathDelim)
{
	int i = 0;
	bool isRdb = false;

	CVol = "";
	if (file_d->FF->file_type == FileType::CAT) {
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
		CatFD->GetCPathForCat(file_d, i, CPath, CVol);
		if (file_d->Name == "@") goto label3;
		goto label4;
	}
	switch (file_d->FF->file_type) {
	case FileType::RDB: CExt = ".RDB"; break;
	case FileType::FAND8: CExt = ".DTA"; break;
	case FileType::DBF: CExt = ".DBF"; break;
	default: CExt = ".000";
	}
	if (SetContextDir(file_d, CDir, isRdb)) goto label2;
	if (file_d == HelpFD) {
		CDir = FandDir;
		CName =
#ifdef FandRunV
			"UFANDHLP";
#else
			"FANDHLP";
#endif
		goto label4;
	}
	CExt = ".100";
	if (CRdb != nullptr) CDir = CRdb->DataDir;
	else CDir = "";
label2:
	AddBackSlash(CDir);
label3:
	CName = CFile->Name;
label4:
	if (pathDelim == '/') ReplaceChar(CDir, '\\', '/');
	if (pathDelim == '\\') ReplaceChar(CDir, '/', '\\');
	CPath = CDir + CName + CExt;
	return CPath;
}

void SetTxtPathVol(std::string& Path, int CatIRec)
{
	if (CatIRec != 0) {
		CVol = CatFD->GetVolume(CatIRec);
		CPath = FExpand(CatFD->GetPathName(CatIRec));
		FSplit(CPath, CDir, CName, CExt);
	}
	else {
		CPath = FExpand(Path);
		CVol = "";
	}
}

void SetTempCExt(char Typ, bool IsNet)
{
	char Nr;
	if (Typ == 'T') {
		Nr = '2';
		switch (CFile->FF->file_type) {
		case FileType::RDB: CExt = ".TTT"; break;
		case FileType::DBF: CExt = ".DBT"; break;
		}
	}
	else {
		Nr = '1';
		switch (CFile->FF->file_type) {
		case FileType::RDB: CExt = ".RDB"; break;
		case FileType::DBF: CExt = ".DBF"; break;
		}
	}
	if (CExt.length() < 2) CExt = ".0";
	CExt[1] = Nr;
	if (IsNet) CPath = WrkDir + CName + CExt; /* work files are local */
	else CPath = CDir + CName + CExt;
}

void CopyH(FILE* h1, FILE* h2)
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
	ReleaseStore(p);
}

void TestDelErr(std::string& P)
{
	if (HandleError != 0) {
		SetMsgPar(P);
		RunError(827);
	}
}
