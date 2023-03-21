#include "oaccess.h"

#include "base.h"
#include "compile.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "obaseww.h"
#include "runfrml.h"
#include "../fandio/FandTFile.h"
#include "wwmix.h"
#include "../fandio/FandXFile.h"
#include "../ExportImport/ExportImport.h"
#include "../textfunc/textfunc.h"

void OpenXWorkH()
{
	CVol = "";
	FileOpenMode m = _isoldnewfile;
	if (XWork.MaxPage == 0) m = _isoverwritefile;
	CPath = FandWorkXName;
	XWork.Handle = OpenH(CPath, m, Exclusive);
	XWork.TestErr();
	if (FileSizeH(XWork.Handle) == 0) { XWork.FreeRoot = 0; XWork.MaxPage = 0; }
}

void OpenTWorkH()
{
	CVol = "";
	if (TWork.MaxPage == 0) {
		CPath = FandWorkTName; TWork.IsWork = true; TWork.Create();
	}
	else {
		CPath = FandWorkTName; CVol = "";
		TWork.Handle = OpenH(CPath, _isoldnewfile, Exclusive); TWork.TestErr();
	}
}

void SaveFD()
{
	WrPrefixes();
	if (CFile->FF->file_type == FileType::INDEX) CFile->FF->XF->NoCreate = false;
}

void SaveFiles()
{
	if (!CacheExist()) return;
	FileD* cf = CFile; CFile = CatFD;
	WrPrefixes();
	ForAllFDs(SaveFD);
	bool b = SaveCache(0, CFile->FF->Handle);
	FlushHandles();
	CFile = cf;
	if (!b) GoExit();
}

void ClosePassiveFD()
{
	if ((CFile->FF->file_type != FileType::RDB) && (CFile->FF->LMode == NullMode)) CloseFile();
}

void CloseFANDFiles(bool FromDML)
{
	RdbD* RD = CRdb;
	while (RD != nullptr) {
		CFile = RD->FD;
		while (CFile != nullptr) {
			if (!FromDML) CFile->FF->ExLMode = CFile->FF->LMode;
			CloseFile();
			CFile = CFile->pChain;
		}
		RD = RD->ChainBack;
	}
	if (CRdb != nullptr) {
		CFile = CatFD;
		CloseFile();
	}
	CFile = HelpFD;
	CloseFile();
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
	OpenF(RdOnly);
	if (CRdb == nullptr) return;
	CFile = CatFD;
	OpenF(Exclusive);
	RD = CRdb;
	while (RD != nullptr) {
		CFile = RD->FD;
		if (IsTestRun) OpenF(Exclusive);
		else OpenF(RdOnly);
		CFile = CFile->pChain;
		while (!FromDML && (CFile != nullptr)) {
			if (CFile->FF->ExLMode != NullMode) {
				OpenF(Shared);
				md = NewLMode(CFile, CFile->FF->ExLMode);
			}
			CFile = CFile->pChain;
		}
		RD = RD->ChainBack;
	}

}

void SetCPathMountVolSetNet(FileUseMode UM)
{
	SetCPathVol();
	CFile->FF->UMode = UM;
	CFile->FF->Drive = (BYTE)TestMountVol(CPath[1]);
	if (!IsNetCVol() || (CFile == Chpt))
		switch (UM) {
		case RdShared: CFile->FF->UMode = RdOnly; break;
		case Shared: CFile->FF->UMode = Exclusive; break;
		}
	else if ((UM == Shared) && EquUpCase(CVol, "#R")) {
		CFile->FF->UMode = RdShared;
	}
}

void TestCFileError()
{
	if (HandleError != 0) {
		CFileError(700 + HandleError);
	}
}

bool OpenF1(FileUseMode UM)
{
	WORD n;
	auto result = true;
	CFile->FF->LMode = NullMode;
	SetCPathMountVolSetNet(UM);
	bool b = (CFile == Chpt) || (CFile == CatFD);
	if (b && (IsTestRun || IsInstallRun) && ((GetFileAttr() & 0x1/*RdOnly*/) != 0)) {
		SetFileAttr(GetFileAttr() & 0x26);
		if (HandleError == 5) HandleError = 79;
		TestCFileError();
		CFile->FF->WasRdOnly = true;
	}
	while (true) {
		CFile->FF->Handle = OpenH(CPath, _isoldfile, CFile->FF->UMode);
		if ((HandleError != 0) && CFile->FF->WasRdOnly) {
			SetFileAttr((GetFileAttr() & 0x27) | 0x1/*RdONly*/);
			TestCFileError();
		}
		if ((HandleError == 5) && (CFile->FF->UMode == Exclusive)) {
			CFile->FF->UMode = RdOnly;
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
	TestCFileError();
	if (CFile->FF->TF != nullptr) {
		CPath = CExtToT(CDir, CName, CExt);
		if (CFile->FF->WasRdOnly) {
			SetFileAttr(GetFileAttr() & 0x26); // 0x26 = archive + hidden + system
		}
		while (true) {
			CFile->FF->TF->Handle = OpenH(CPath, _isoldfile, CFile->FF->UMode);
			if (HandleError == 2) {
				if (CFile->FF->TF->Format == CFile->FF->TF->DbtFormat) {
					CFile->FF->TF->Format = CFile->FF->TF->FptFormat;
					CExt = ".FPT";
					CPath = CDir + CName + CExt;
					continue;
				}
				if (CFile->IsDynFile) {
					CloseClearH(&CFile->FF->Handle);
					result = false;
					return result;
				}
			}
			break;
		}
		if (HandleError != 0) {
			n = HandleError;
			CloseClearHCFile();
			HandleError = n;
			TestCPathError();
			return result;
		}
	}
	if (CFile->FF->file_type == FileType::INDEX) {
		CPath = CExtToX(CDir, CName, CExt);
		while (true) {
			CFile->FF->XF->Handle = OpenH(CPath, _isoldfile, CFile->FF->UMode);
			if (HandleError == 2) {
				CFile->FF->XF->Handle = OpenH(CPath, _isoverwritefile, Exclusive);
				if (HandleError != 0) {
					n = HandleError;
					CloseClearHCFile();
					HandleError = n;
					TestCPathError();
					return result;
				}
				CFile->FF->XF->SetNotValid();
				CloseH(&CFile->FF->XF->Handle);
				continue;
			}
			if (HandleError != 0) {
				n = HandleError;
				CloseClearHCFile();
				HandleError = n;
				TestCPathError();
			}
			if (CFile->FF->XF != nullptr && FileSizeH(CFile->FF->XF->Handle) < 512) {
				CFile->FF->XF->SetNotValid();
			}
			break;
		}
	}
	return result;
}

bool OpenF2()
{
	wwmix ww;

	longint FS = 0, n = 0, l = 0;
	WORD Signum = 0, rLen = 0;
	LockMode md = NullMode;
	FS = FileSizeH(CFile->FF->Handle);
	CFile->FF->NRecs = 0;
	auto result = false;
	if (FS < CFile->FF->FrstDispl) goto label1;
	rLen = RdPrefix();
	n = (FS - CFile->FF->FrstDispl) / CFile->FF->RecLen;
	if (rLen != 0xffff) {
		if (CFile->IsDynFile) {
			CloseClearHCFile();
			return result;
		}
		else {
			if (OldToNewCat(FS)) {
				goto label3;
			}
			CFileMsg(883, ' ');
			l = CFile->FF->NRecs * rLen + CFile->FF->FrstDispl;
			if (l == FS || !PromptYN(885)) {
				CloseGoExit();
			}
			if (CFile->FF->NRecs == 0 || l >> CachePageShft != FS >> CachePageShft) {
				WrLLF10Msg(886);
				CFile->FF->NRecs = n;
			}
			goto label2;
		}
	}
	else {
	}
	if (n < CFile->FF->NRecs) {
		SetCPathVol(); SetMsgPar(CPath);
		if (PromptYN(882)) {
			CFile->FF->NRecs = n;
		label1:
			if (CFile->FF->IsShared() && (CFile->FF->LMode < ExclMode)) {
				ChangeLMode(CFile, ExclMode, 0, false);
			}
			CFile->FF->LMode = ExclMode;
		label2:
			SetUpdHandle(CFile->FF->Handle);
			WrPrefix();
		}
		else {
			CloseGoExit();
		}
	}
label3:
	if (CFile->FF->TF != nullptr) {
		if (FS < CFile->FF->FrstDispl) {
			CFile->FF->TF->SetEmpty();
		}
		else {
			CFile->FF->TF->RdPrefix(true);
			if ((CFile->FF->file_type == FileType::RDB) && !IsActiveRdb(CFile) && !ww.HasPassWord(CFile, 1, "")) {
				CFileMsg(616, ' ');
				CloseGoExit();
			}
		}
	}
	if (CFile->FF->file_type == FileType::INDEX) {
		if (FS < CFile->FF->FrstDispl) {
			CFile->FF->XF->SetNotValid();
		}
		else {
			RdWrCache(READ, CFile->FF->XF->Handle, CFile->FF->XF->NotCached(), 0, 2, &Signum);
			CFile->FF->XF->RdPrefix();
			if (
				!CFile->FF->XF->NotValid && ((Signum != 0x04FF) || (CFile->FF->XF->NRecsAbs != CFile->FF->NRecs)
					|| (CFile->FF->XF->FreeRoot > CFile->FF->XF->MaxPage)
					|| ((((longint)CFile->FF->XF->MaxPage + 1) << XPageShft) > FileSizeH(CFile->FF->XF->Handle)))
				|| (CFile->FF->XF->NrKeys != 0) && (CFile->FF->XF->NrKeys != CFile->GetNrKeys()))
			{
				if (!EquUpCase(GetEnv("FANDMSG830"), "NO")) {
					CFileMsg(830, 'X');
				}
				if (CFile->FF->IsShared() && (CFile->FF->LMode < ExclMode)) {
					ChangeLMode(CFile, ExclMode, 0, false);
				}
				CFile->FF->LMode = ExclMode;
				CFile->FF->XF->SetNotValid();
			}
		}
	}
	SeekRec(CFile, 0);
	return true;
}

bool OpenF(FileUseMode UM)
{
	bool result = true;
	if (CFile->FF->Handle != nullptr) return result;
	if (OpenF1(UM)) {
		if (
#ifdef FandSQL
			!IsSQLFile &&
#endif
			CFile->FF->IsShared()) {
			ChangeLMode(CFile, RdMode, 0, false);
			CFile->FF->LMode = RdMode;
		}
		result = OpenF2();
		OldLMode(CFile, NullMode);
	}
	else result = false;
	return result;
}

void CreateF()
{
	SetCPathMountVolSetNet(Exclusive);
	CFile->FF->Handle = OpenH(CPath, _isoverwritefile, Exclusive);
	TestCFileError();
	CFile->FF->NRecs = 0;
	if (CFile->FF->TF != nullptr) {
		CPath = CExtToT(CDir, CName, CExt);
		CFile->FF->TF->Create();
	}
	if (CFile->FF->file_type == FileType::INDEX) {
		CPath = CExtToX(CDir, CName, CExt);
		CFile->FF->XF->Handle = OpenH(CPath, _isoverwritefile, Exclusive);
		CFile->FF->XF->TestErr(); /*SetNotValid*/
		CFile->FF->XF->SetEmpty();
	}
	SeekRec(CFile, 0);
	SetUpdHandle(CFile->FF->Handle);
}

bool OpenCreateF(FileD* fileD, FileUseMode UM)
{
	if (!OpenF(UM)) {
		CreateF();
		if ((UM == Shared) || (UM == RdShared)) {
			WrPrefixes();
			SaveCache(0, CFile->FF->Handle);
			CloseClearH(&CFile->FF->Handle);
			if (CFile->FF->file_type == FileType::INDEX) CloseClearH(&CFile->FF->XF->Handle);
			if (CFile->FF->TF != nullptr) CloseClearH(&CFile->FF->TF->Handle);
			OpenF(UM);
		}
	}
	return true;
}

LockMode RewriteF(const bool Append)
{
	LockMode result;
	/* !!! with CFile^ do!!! */
	if (Append) {
		result = NewLMode(CFile, CrMode);
		SeekRec(CFile, CFile->FF->NRecs);
		if (CFile->FF->XF != nullptr) {
			CFile->FF->XF->FirstDupl = true;
			TestXFExist();
		}
		return result;
	}
	result = NewLMode(CFile, ExclMode);
	CFile->FF->NRecs = 0;
	SeekRec(CFile, 0);
	SetUpdHandle(CFile->FF->Handle);
	XFNotValid();
	if (CFile->FF->file_type == FileType::INDEX) CFile->FF->XF->NoCreate = true;
	if (CFile->FF->TF != nullptr) CFile->FF->TF->SetEmpty();
	return result;
}

void TruncF()
{
	if (CFile->FF->UMode == RdOnly) return;
	LockMode md = NewLMode(CFile, RdMode);
	TruncH(CFile->FF->Handle, CFile->FF->UsedFileSize());
	if (HandleError != 0) CFileMsg(700 + HandleError, '0');
	if (CFile->FF->TF != nullptr) {
		TruncH(CFile->FF->TF->Handle, CFile->FF->TF->UsedFileSize());
		CFile->FF->TF->TestErr();
	}
	if (CFile->FF->file_type == FileType::INDEX) {
		longint sz = CFile->FF->XF->UsedFileSize();
		if (CFile->FF->XF->NotValid) sz = 0;
		TruncH(CFile->FF->XF->Handle, sz);
		CFile->FF->XF->TestErr();
	}
	OldLMode(CFile, md);

}

void CloseFile()
{
	if (CFile->FF->Handle == nullptr) return;
	if (CFile->FF->IsShared()) {
		OldLMode(CFile, NullMode);
	}
	else {
		WrPrefixes();
	}
	SaveCache(0, CFile->FF->Handle);
	TruncF();
	if (CFile->FF->file_type == FileType::INDEX) {
		if (CFile->FF->XF->Handle != nullptr) {
			CloseClearH(&CFile->FF->XF->Handle);
			if (!CFile->FF->IsShared()) {
				if (CFile->FF->XF->NotValid) goto label1;
				if ((CFile->FF->XF->NRecs == 0) || CFile->FF->NRecs == 0) {
					CFile->FF->NRecs = 0;
				label1:
					SetCPathVol();
					CPath = CExtToX(CDir, CName, CExt);
					MyDeleteFile(CPath);
				}
			}
		}
	}
	// zavreni souboru volnych textu .T00
	if (CFile->FF->TF != nullptr) {
		if (CFile->FF->TF->Handle != nullptr) {
			CloseClearH(&CFile->FF->TF->Handle);
			if (HandleError == 0) CFile->FF->TF->Handle = nullptr; // soubor byl uspesne uzavren
			if ((!CFile->FF->IsShared()) && (CFile->FF->NRecs == 0) && (CFile->FF->file_type != FileType::DBF)) {
				SetCPathVol();
				CPath = CExtToT(CDir, CName, CExt);
				// MyDeleteFile(CPath);
			}
		}
	}
	CloseClearH(&CFile->FF->Handle);
	if (HandleError == 0) CFile->FF->Handle = nullptr;
	CFile->FF->LMode = NullMode;
	if (!CFile->FF->IsShared() && (CFile->FF->NRecs == 0) && (CFile->FF->file_type != FileType::DBF)) {
		SetCPathVol();
		// MyDeleteFile(CPath);
	}
	if (CFile->FF->WasRdOnly) {
		CFile->FF->WasRdOnly = false;
		SetCPathVol();
		SetFileAttr((GetFileAttr() & 0x27) | 0x01); // {RdONly; }
		if (CFile->FF->TF != nullptr) {
			CPath = CExtToT(CDir, CName, CExt);
			SetFileAttr((GetFileAttr() & 0x27) | 0x01); //  {RdONly; }
		}
	}
}


void CloseFilesAfter(FileD* FD)
{
	CFile = FD;

	while (CFile != nullptr) {
		CloseFile();
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
				CloseFile();
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
				SetCPathVol();
				CFile = cf;
				return;
			}
			if (CFile->FF->XF != nullptr && CFile->FF->XF->Handle == handle) {
				SetCPathVol();
				CPath = CExtToX(CDir, CName, CExt);
				CFile = cf;
				return;
			}
			if (CFile->FF->TF != nullptr && CFile->FF->TF->Handle == handle) {
				SetCPathVol();
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

WORD GetCatIRec(pstring Name, bool MultiLevel)
{
	longint i = 0; FileD* CF = nullptr; RdbD* R = nullptr; void* CR = nullptr;
	WORD result = 0;
	if (CatFD == nullptr || CatFD->FF->Handle == nullptr) return result;
	if (CRdb == nullptr) return result;
	CF = CFile; CR = CRecPtr; CFile = CatFD;
	CRecPtr = GetRecSpace(CFile->FF);
	R = CRdb;
label1:
	for (i = 1; i <= CatFD->FF->NRecs; i++)
	{
		CFile->ReadRec(i, CRecPtr);
		if (EquUpCase(OldTrailChar(' ', _ShortS(CatRdbName)), R->FD->Name) &&
			EquUpCase(OldTrailChar(' ', _ShortS(CatFileName)), Name))
		{
			result = i; goto label2;
		}
	}
	R = R->ChainBack;
	if ((R != nullptr) && MultiLevel) goto label1;
label2:
	CFile = CF;
	ReleaseStore(CRecPtr);
	CRecPtr = CR;
	return result;
}

WORD Generation()
{
	WORD i, j; pstring s(2);
	if (CFile->CatIRec == 0) return 0;
	RdCatPathVol(CFile->CatIRec);
	s = CExt.substr(3, 2);
	val(s, i, j);
	if (j == 0) return i;
	return 0;
}

void TurnCat(WORD Frst, WORD N, integer I)
{
	void* p; void* q; WORD j, last;
	if (CFile != nullptr) CloseFile();
	CFile = CatFD; p = GetRecSpace(CFile->FF); q = GetRecSpace(CFile->FF);
	CRecPtr = q; last = Frst + N - 1;
	if (I > 0)
		while (I > 0) {
			CFile->ReadRec(Frst, CRecPtr); CRecPtr = p;
			for (j = 1; j <= N - 1; j++) {
				CFile->ReadRec(Frst + j, CRecPtr);
				CFile->WriteRec(Frst + j - 1, CRecPtr);
			}
			CRecPtr = q;
			CFile->WriteRec(last, CRecPtr);
			I--;
		}
	else
		while (I < 0) {
			CFile->ReadRec(last, CRecPtr); CRecPtr = p;
			for (j = 1; j <= N - 1; j++) {
				CFile->ReadRec(last - j, CRecPtr);
				CFile->WriteRec(last - j + 1, CRecPtr);
			}
			CRecPtr = q;
			CFile->WriteRec(Frst, CRecPtr);
			I++;
		}
	ReleaseStore(p);
}

std::string RdCatField(WORD CatIRec, FieldDescr* CatF)
{
	FileD* CF = CFile;
	void* CR = CRecPtr;
	CFile = CatFD;
	CRecPtr = GetRecSpace(CatFD->FF);
	CFile->ReadRec(CatIRec, CRecPtr);
	std::string stdS = _StdS(CatF);
	std::string result = TrailChar(stdS, ' ');
	ReleaseStore(CRecPtr);
	CFile = CF;
	CRecPtr = CR;
	return result;
}

void WrCatField(WORD CatIRec, FieldDescr* CatF, std::string Txt)
{
	FileD* CF = CFile;
	void* CR = CRecPtr;
	CFile = CatFD;
	CRecPtr = GetRecSpace(CatFD->FF);
	CFile->ReadRec(CatIRec, CRecPtr);
	S_(CatF, Txt);
	CFile->WriteRec(CatIRec, CRecPtr);
	ReleaseStore(CRecPtr);
	CFile = CF;
	CRecPtr = CR;
}

void WrCatField(FileD* catFD, WORD CatIRec, FieldDescr* CatF, const std::string& Txt)
{
	BYTE* record = new BYTE[catFD->FF->RecLen];
	catFD->ReadRec(CatIRec, record);
	S_(CatF, Txt, record);
	catFD->WriteRec(CatIRec, record);
	delete[] record;
}

void RdCatPathVol(WORD CatIRec)
{
	CPath = FExpand(RdCatField(CatIRec, CatPathName));
	FSplit(CPath, CDir, CName, CExt);
	CVol = RdCatField(CatIRec, CatVolume);
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

void GetCPathForCat(WORD I)
{
	std::string d;
	bool isRdb;

	CVol = RdCatField(I, CatVolume);
	CPath = RdCatField(I, CatPathName);
	const bool setContentDir = SetContextDir(CFile, d, isRdb);
	if (setContentDir && CPath.length() > 1 && CPath[1] != ':') {
		if (isRdb) {
			FSplit(CPath, CDir, CName, CExt);
			AddBackSlash(d);
			CDir = d;
			CPath = CDir + CName + CExt;
			return;
		}
		if (CPath[0] == '\\') {
			CPath = d.substr(0, 2) + CPath;
		}
		else {
			AddBackSlash(d);
			CPath = d + CPath;
		}
	}
	else {
		CPath = FExpand(CPath);
	}
	FSplit(CPath, CDir, CName, CExt);
}

void SetCPathVol(char pathDelim)
{
	WORD i = 0;
	bool isRdb = false;

	CVol = "";
	if (CFile->FF->file_type == FileType::CAT) {
		CDir = GetEnv("FANDCAT");
		if (CDir.empty()) {
			CDir = TopDataDir.empty() ? TopRdbDir : TopDataDir;
		}
		AddBackSlash(CDir);
		CName = CatFDName;
		CExt = ".CAT";
		goto label4;
	}
	i = CFile->CatIRec;
	if (i != 0) {
		GetCPathForCat(i);
		if (CFile->Name == "@") goto label3;
		goto label4;
	}
	switch (CFile->FF->file_type) {
	case FileType::RDB: CExt = ".RDB"; break;
	case FileType::FAND8: CExt = ".DTA"; break;
	case FileType::DBF: CExt = ".DBF"; break;
	default: CExt = ".000";
	}
	if (SetContextDir(CFile, CDir, isRdb)) goto label2;
	if (CFile == HelpFD) {
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
}



void SetTxtPathVol(pstring* Path, WORD CatIRec)
{
	if (CatIRec != 0) {
		RdCatPathVol(CatIRec);
	}
	else {
		CPath = FExpand(*Path);
		CVol = "";
	}
}

void SetTxtPathVol(std::string& Path, WORD CatIRec)
{
	if (CatIRec != 0) {
		RdCatPathVol(CatIRec);
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

FileD* OpenDuplF(bool CrTF)
{
	integer Len = 0;
	SetCPathVol();
	bool net = IsNetCVol();
	FileD* OldFD = CFile;
	FileD* FD = new FileD(*OldFD);
	CFile = FD;

	SetTempCExt('0', net);
	CVol = "";
	FD->FullPath = CPath;
	FD->FF->Handle = OpenH(CPath, _isoverwritefile, Exclusive);
	TestCFileError();
	FD->FF->NRecs = 0;
	FD->IRec = 0;
	FD->FF->Eof = true;
	FD->FF->UMode = Exclusive;
	if (FD->FF->file_type == FileType::INDEX) {
		if (FD->FF->XF != nullptr) {
			delete FD->FF->XF;
		}
		FD->FF->XF = new FandXFile();
		FD->FF->XF->Handle = nullptr;
		FD->FF->XF->NoCreate = true;
		/*else xfile name identical with orig file*/
	}

	if (CrTF && (FD->FF->TF != nullptr)) {
		FD->FF->TF = new FandTFile();
		*FD->FF->TF = *OldFD->FF->TF;
		SetTempCExt('T', net);
		FD->FF->TF->Handle = OpenH(CPath, _isoverwritefile, Exclusive);
		FD->FF->TF->TestErr();
		FD->FF->TF->CompileAll = true;
		FD->FF->TF->SetEmpty();

	}
	return FD;
}

void CopyDuplF(FileD* TempFD, bool DelTF)
{
	FileD* cf = CFile;
	CFile = TempFD;
	WrPrefixes();
	CFile = cf;
	SaveCache(0, CFile->FF->Handle);
	SetTempCExt('0', true);
	CopyH(TempFD->FF->Handle, CFile->FF->Handle);
	if ((CFile->FF->TF != nullptr) && DelTF) {
		FILE* h1 = TempFD->FF->TF->Handle;
		FILE* h2 = CFile->FF->TF->Handle;
		SetTempCExt('T', true);
		*CFile->FF->TF = *TempFD->FF->TF;
		CFile->FF->TF->Handle = h2;
		CopyH(h1, h2);
	}
	RdPrefixes();
}

void CopyH(FILE* h1, FILE* h2)
{
	const WORD BufSize = 32768;
	void* p = new BYTE[BufSize];
	longint sz = FileSizeH(h1);
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

void SubstDuplF(FileD* TempFD, bool DelTF)
{
	//bool net;
	XFNotValid();
	SetCPathVol();
	if (IsNetCVol()) {
		CopyDuplF(TempFD, DelTF);
		return;
	}
	SaveCache(0, CFile->FF->Handle);
	FileD* PrimFD = CFile;
	std::string p = CPath;
	CPath = CExtToT(CDir, CName, CExt);
	std::string pt = CPath;

	CloseClearH(&PrimFD->FF->Handle);
	MyDeleteFile(p);
	TestDelErr(p);
	FileD* FD = PrimFD->pChain;
	FandTFile* MD = PrimFD->FF->TF;
	FandXFile* xf2 = PrimFD->FF->XF;
	FileUseMode um = PrimFD->FF->UMode;
	*PrimFD = *TempFD;
	PrimFD->pChain = FD;
	PrimFD->FF->XF = xf2;
	PrimFD->FF->UMode = um;
	CloseClearH(&PrimFD->FF->Handle);
	SetTempCExt('0', false);
	pstring ptmp = CPath;
	RenameFile56(ptmp, p, true);
	CPath = p;
	PrimFD->FF->Handle = OpenH(CPath, _isoldfile, PrimFD->FF->UMode);
	SetUpdHandle(PrimFD->FF->Handle);

	if ((MD != nullptr) && DelTF) {
		CloseClearH(&MD->Handle);
		MyDeleteFile(pt);
		TestDelErr(pt);
		*MD = *PrimFD->FF->TF;
		PrimFD->FF->TF = MD;
		CloseClearH(&MD->Handle);
		CPath = ptmp;
		SetTempCExt('T', false);
		RenameFile56(CPath, pt, true);
		CPath = pt;
		MD->Handle = OpenH(CPath, _isoldfile, PrimFD->FF->UMode);
		SetUpdHandle(MD->Handle);
	}
	PrimFD->FF->TF = MD;
}

void TestDelErr(std::string& P)
{
	if (HandleError != 0) {
		SetMsgPar(P);
		RunError(827);
	}
}

void DelDuplF(FileD* TempFD)
{
	CloseClearH(&TempFD->FF->Handle);
	SetCPathVol();
	SetTempCExt('0', CFile->FF->IsShared());
	MyDeleteFile(CPath);
}
