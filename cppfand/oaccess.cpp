#include "oaccess.h"

#include "base.h"
#include "expimp.h"
#include "legacy.h"
#include "obaseww.h"
#include "runfrml.h"
#include "wwmix.h"

void OpenXWorkH()
{
	CVol = "";
	FileOpenMode m = _isoldnewfile;
	if (XWork.MaxPage == 0) m = _isoverwritefile;
	CPath = FandWorkXName;
	XWork.Handle = OpenH(m, Exclusive);
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
		TWork.Handle = OpenH(_isoldnewfile, Exclusive); TWork.TestErr();
	}
}

void SaveFD()
{
	WrPrefixes(); if (CFile->Typ == 'X') CFile->XF->NoCreate = false;
}

void SaveFiles()
{
	bool b = false; WORD i = 0; FileD* cf = nullptr;
	if (!CacheExist()) return;
	cf = CFile; CFile = CatFD; WrPrefixes();
	ForAllFDs(SaveFD); b = SaveCache(0); FlushHandles();
	CFile = cf; if (!b) GoExit();
}

void ClosePassiveFD()
{
	if ((CFile->Typ != '0') && (CFile->LMode == NullMode)) CloseFile();
}

void CloseFANDFiles(bool FromDML)
{
	RdbDPtr RD;
	RD = CRdb; while (RD != nullptr) {
		CFile = RD->FD;
		while (CFile != nullptr) {
			if (!FromDML) CFile->ExLMode = CFile->LMode;
			CloseFile();
			CFile = (FileD*)CFile->Chain;
		}
		RD = RD->ChainBack;
	}
	if (CRdb != nullptr) { CFile = CatFD; CloseFile(); }
	CFile = &HelpFD;
	CloseFile();
	CloseH(TWork.Handle);
	CloseH(XWork.Handle);
}

void OpenFANDFiles(bool FromDML)
{
	RdbD* RD = nullptr;
	LockMode md = NullMode;

	OpenXWorkH();
	OpenTWorkH();
	CFile = &HelpFD;
	OpenF(RdOnly);
	if (CRdb == nullptr) return;
	CFile = CatFD;
	OpenF(Exclusive);
	RD = CRdb;
	while (RD != nullptr) {
		CFile = RD->FD;
		if (IsTestRun) OpenF(Exclusive);
		else OpenF(RdOnly);
		CFile = (FileD*)CFile->Chain;
		while (!FromDML && (CFile != nullptr)) {
			/*with CFile^*/
			if (CFile->ExLMode != NullMode)
			{
				OpenF(Shared);
				md = NewLMode(CFile->ExLMode);
			}
			CFile = (FileD*)CFile->Chain;
		}
		RD = RD->ChainBack;
	}

}

void SetCPathMountVolSetNet(FileUseMode UM)
{
	SetCPathVol();
	/* !!! with CFile^ do!!! */
	CFile->UMode = UM;
	CFile->Drive = TestMountVol(CPath[1]);
	if (!IsNetCVol() || (CFile == Chpt))
		switch (UM) {
		case RdShared: CFile->UMode = RdOnly; break;
		case Shared: CFile->UMode = Exclusive; break;
		}
	else if ((UM == Shared) && SEquUpcase(CVol, "#R")) CFile->UMode = RdShared;
}

bool OpenF1(FileUseMode UM)
{
	bool b; WORD n;
	/* !!! with CFile^ do!!! */
	auto result = true; CFile->LMode = NullMode;
	SetCPathMountVolSetNet(UM);  b = (CFile == Chpt) || (CFile == CatFD);
	if (b && (IsTestRun || IsInstallRun)
		&& ((GetFileAttr() & 1/*RdOnly*/) != 0)) {
		SetFileAttr(GetFileAttr() & 0x26);
		if (HandleError == 5) HandleError = 79; TestCFileError(); CFile->WasRdOnly = true;
	}
label1:
	CFile->Handle = OpenH(_isoldfile, CFile->UMode);
	if ((HandleError != 0) && CFile->WasRdOnly) {
		SetFileAttr((GetFileAttr() & 0x27) | 0x1/*RdONly*/); TestCFileError();
	}
	if ((HandleError == 5) && (CFile->UMode == Exclusive)) { CFile->UMode = RdOnly; goto label1; }
	if (HandleError == 2) { result = false; return result; }
#ifndef FandNetV
	if ((HandleError == 5 || HandleError == 0x21) &&
		((CVol == '#') || (CVol == "##") || SEquUpcase(CVol, "#R"))) CFileError(842);
#endif
	TestCFileError();
	if (CFile->TF != nullptr) /* !!! with TF^ do!!! */ {
		CExtToT();
		if (CFile->WasRdOnly) SetFileAttr(GetFileAttr() & 0x26);
	label2:
		CFile->TF->Handle = OpenH(_isoldfile, CFile->UMode);
		if (HandleError == 2) {
			if (CFile->TF->Format == CFile->TF->DbtFormat) {
				CFile->TF->Format = CFile->TF->FptFormat;
				CExt = ".FPT";
				CPath = CDir + CName + CExt;
				goto label2;
			}
			if (CFile->IsDynFile) {
				CloseClearH(CFile->Handle);
				result = false; return result;
			}
		}
		if (HandleError != 0) goto label4;
	}
	if (CFile->Typ == 'X') /* !!! with XF^ do!!! */ {
		CExtToX();
	label3:
		CFile->XF->Handle = OpenH(_isoldfile, CFile->UMode);
		if (HandleError == 2) {
			CFile->XF->Handle = OpenH(_isoverwritefile, Exclusive);
			if (HandleError != 0) goto label4;
			CFile->XF->SetNotValid(); CloseH(CFile->XF->Handle);
			goto label3;
		}
		if (HandleError != 0) {
		label4:
			n = HandleError;
			CloseClearHCFile();
			HandleError = n;
			TestCPathError();
		}
		if (FileSizeH(CFile->XF->Handle) < 512) CFile->XF->SetNotValid();
	}
	return result;
}

bool OpenF2()
{
	wwmix ww;

	longint FS = 0, n = 0, l = 0;
	WORD Signum = 0, rLen = 0;
	LockMode md = NullMode;
	/* !!! with CFile^ do!!! */
	FS = FileSizeH(CFile->Handle); CFile->NRecs = 0;
	auto result = false;
	if (FS < CFile->FrstDispl) goto label1;
	rLen = RdPrefix();
	n = (FS - CFile->FrstDispl) / CFile->RecLen;
	if (rLen != 0xffff)
		if (CFile->IsDynFile) { CloseClearHCFile(); return result; }
		else {
			if (OldToNewCat(FS)) goto label3;
			CFileMsg(883, ' '); l = longint(CFile->NRecs) * rLen + CFile->FrstDispl;
			if ((l == FS) || !PromptYN(885)) CloseGoExit();
			if ((CFile->NRecs == 0) || (l >> CachePageShft != FS >> CachePageShft)) {
				WrLLF10Msg(886); CFile->NRecs = n;
			}
			goto label2;
		}
	if (n < CFile->NRecs) {
		SetCPathVol(); SetMsgPar(CPath);
		if (PromptYN(882)) {
			CFile->NRecs = n;
		label1:
			if (CFile->IsShared() && (CFile->LMode < ExclMode)) ChangeLMode(ExclMode, 0, false);
			CFile->LMode = ExclMode;
		label2:
			SetUpdHandle(CFile->Handle); WrPrefix();
		}
		else CloseGoExit();
	}
label3:
	if (CFile->TF != nullptr) {
		if (FS < CFile->FrstDispl) { CFile->TF->SetEmpty(); }
		else {
			CFile->TF->RdPrefix(true);
			if ((CFile->Typ == '0') && !IsActiveRdb(CFile)
				&& !ww.HasPassWord(CFile, 1, "")) {
				CFileMsg(616, ' '); CloseGoExit();
			}
		}
	}
	if (CFile->Typ == 'X') {/* !!! with XF^ do!!! */
		if (FS < CFile->FrstDispl) { CFile->XF->SetNotValid(); }
		else {
			RdWrCache(true, CFile->XF->Handle, CFile->XF->NotCached(), 0, 2, &Signum);
			CFile->XF->RdPrefix();
			if (!CFile->XF->NotValid &&
				((Signum != 0x04FF) || (CFile->XF->NRecsAbs != CFile->NRecs)
					|| (CFile->XF->FreeRoot > CFile->XF->MaxPage)
					|| ((longint(CFile->XF->MaxPage + 1) << XPageShft) > FileSizeH(CFile->XF->Handle)))
				|| (CFile->XF->NrKeys != 0) && (CFile->XF->NrKeys != CFile->GetNrKeys()))
			{
				if (!SEquUpcase(GetEnv("FANDMSG830"), "NO")) CFileMsg(830, 'X');
				if (CFile->IsShared() && (CFile->LMode < ExclMode)) ChangeLMode(ExclMode, 0, false);
				CFile->LMode = ExclMode;
				CFile->XF->SetNotValid();
			}
		}
	}
	SeekRec(0);
	return true;
}

bool OpenF(FileUseMode UM)
{
	bool result = true;
	if (CFile->Handle != nullptr) return result;
	if (OpenF1(UM))
	{
		if (
#ifdef FandSQL
			!IsSQLFile &&
#endif
			CFile->IsShared()) {
			ChangeLMode(RdMode, 0, false);
			CFile->LMode = RdMode;
		}
		result = OpenF2();
		OldLMode(NullMode);
	}
	else result = false;
	return result;
}

void CreateF()
{
	/* !!! with CFile^ do!!! */ {
		SetCPathMountVolSetNet(Exclusive);
		CFile->Handle = OpenH(_isoverwritefile, Exclusive);
		TestCFileError(); CFile->NRecs = 0;
		if (CFile->TF != nullptr) /* !!! with TF^ do!!! */ { CExtToT(); CFile->TF->Create(); }
		if (CFile->Typ == 'X') /* !!! with XF^ do!!! */ {
			CExtToX();
			CFile->XF->Handle = OpenH(_isoverwritefile, Exclusive);
			CFile->XF->TestErr(); /*SetNotValid*/
			CFile->XF->SetEmpty();
		}
		SeekRec(0); SetUpdHandle(CFile->Handle);
	}
}

bool OpenCreateF(FileUseMode UM)
{
	/* !!! with CFile^ do!!! */
	if (!OpenF(UM)) {
		CreateF(); 
		if ((UM == Shared) || (UM == RdShared)) {
			WrPrefixes(); 
			SaveCache(0); 
			CloseClearH(CFile->Handle);
			if (CFile->Typ == 'X') CloseClearH(CFile->XF->Handle);
			if (CFile->TF != nullptr) CloseClearH(CFile->TF->Handle);
			OpenF(UM);
		}
	}
	return true;
}

LockMode RewriteF(bool Append)
{
	LockMode result;
	/* !!! with CFile^ do!!! */
	if (Append) {
		result = NewLMode(CrMode);
		SeekRec(CFile->NRecs);
		if (CFile->XF != nullptr) { CFile->XF->FirstDupl = true; TestXFExist(); }
		return result;
	}
	result = NewLMode(ExclMode);
	CFile->NRecs = 0;
	SeekRec(0);
	SetUpdHandle(CFile->Handle);
	XFNotValid();
	if (CFile->Typ == 'X') CFile->XF->NoCreate = true;
	if (CFile->TF != nullptr) CFile->TF->SetEmpty();
	return result;
}

void TruncF()
{
	/* with CFile^ */
	LockMode md; longint sz;
	if (CFile->UMode == RdOnly) return;
	md = NewLMode(RdMode);
	TruncH(CFile->Handle, CFile->UsedFileSize());
	if (HandleError != 0) CFileMsg(700 + HandleError, '0');
	if (CFile->TF != nullptr)  /*with TF^*/ {
		TruncH(CFile->TF->Handle, CFile->TF->UsedFileSize());
		CFile->TF->TestErr();
	}
	if (CFile->Typ == 'X')  /*with XF^*/ {
		sz = CFile->XF->UsedFileSize();
		if (CFile->XF->NotValid) sz = 0;
		TruncH(CFile->XF->Handle, sz);
		CFile->XF->TestErr();
	}
	OldLMode(md);

}

void CloseFile()
{
	//with CFile^ do {
	if (CFile->Handle == nullptr) return;
	if (CFile->IsShared()) OldLMode(NullMode);
	else WrPrefixes();
	SaveCache(0);
	TruncF();
	if (CFile->Typ == 'X') { /*with XF^*/
		if (CFile->XF->Handle != nullptr) {
			CloseClearH(CFile->Handle);
			if (!CFile->IsShared())
			{
				if (CFile->XF->NotValid) goto label1;
				if ((CFile->XF->NRecs == 0) || CFile->NRecs == 0) {
					CFile->NRecs = 0;
				label1:
					SetCPathVol();
					CExtToX();
					MyDeleteFile(CPath);
				}
			}
		}
	}
	// zavreni souboru volnych textu .T00
	if (CFile->TF != nullptr) { /*with TF^*/
		if (CFile->TF->Handle != nullptr) {
			CloseClearH(CFile->TF->Handle);
			if (HandleError == 0) CFile->TF->Handle = nullptr; // soubor byl uspesne uzavren
			if ((!CFile->IsShared()) && (CFile->NRecs == 0) && (CFile->Typ != 'D')) {
				SetCPathVol();
				CExtToT();
				MyDeleteFile(CPath);
			}
		}
	}
	CloseClearH(CFile->Handle);
	if (HandleError == 0) CFile->Handle = nullptr;
	CFile->LMode = NullMode;
	if (!CFile->IsShared() && (CFile->NRecs == 0) && (CFile->Typ != 'D')) {
		SetCPathVol();
		MyDeleteFile(CPath);
	}
	if (CFile->WasRdOnly) {
		CFile->WasRdOnly = false;
		SetCPathVol();
		SetFileAttr((GetFileAttr() & 0x27) | 0x01); // {RdONly; }
		if (CFile->TF != nullptr) {
			CExtToT();
			SetFileAttr((GetFileAttr() & 0x27) | 0x01); //  {RdONly; }
		}
	}
}


void CloseFAfter(FileD* FD)
{
	CFile = FD;
	while (CFile != nullptr) { CloseFile(); CFile = (FileD*)CFile->Chain; }
}

bool ActiveRdbOnDrive(WORD D)
{
	auto result = true;
	RdbD* R = CRdb;
	while (R != nullptr)
	{
		if (R->FD->Drive == D) return result;
		R = R->ChainBack;
	}
	result = false;
	return result;
}

void CloseFilesOnDrive(WORD D)
{
	RdbDPtr R; FileDPtr CF; bool b;
	R = CRdb; CF = CFile;
	while (R != nullptr)
	{
		CFile = R->FD;
		while (CFile != nullptr)
		{
			if (CFile->Drive == D) CloseFile();
			CFile = (FileD*)CFile->Chain;
		}
		R = R->ChainBack;
	}
	CFile = CF;
}

WORD TestMountVol(char DriveC)
{
	//SearchRec S; 
	WORD D = 0, i = 0; pstring Vol;
	pstring Drive(1); Drive = "A";

	if (IsNetCVol()) return 0;
	D = toupper(DriveC) - '@';
	if (D >= FloppyDrives)
		if (toupper(CDir[1]) == spec.CPMdrive) D = FloppyDrives;
		else return 0;
	if ((CVol == "") || SEquUpcase(MountedVol[D], CVol)) goto label3;
	Drive[1] = DriveC;
	if (ActiveRdbOnDrive(D))
	{
		Set3MsgPar(Drive, CVol, MountedVol[D]); 
		RunError(812);
	}
	Vol = CVol; 
	CloseFilesOnDrive(D); 
	CVol = Vol;
label1:
	F10SpecKey = _ESC_; 
	Set2MsgPar(Drive, CVol); 
	WrLLF10Msg(808);
	if (KbdChar == _ESC_) if (PromptYN(21)) GoExit(); 
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
label2:
	MountedVol[D] = CVol;
label3:
	return D;
}

void ReleaseDrive(WORD D)
{
	pstring Drive(1);
	if (MountedVol[D] == "") return;
	if (D == FloppyDrives) Drive = spec.CPMdrive;
	else Drive = char(D + '@');
	if (ActiveRdbOnDrive(D)) { 
		SetMsgPar(Drive); 
		RunError(813); 
	}
	CloseFilesOnDrive(D); 
	Set2MsgPar(MountedVol[D], Drive); 
	WrLLF10Msg(818);
	MountedVol[D] = "";
}

void SetCPathForH(FILE* handle)
{
	RdbD* RD = nullptr; 
	FileD* cf = nullptr;
	cf = CFile; RD = CRdb;
	while (RD != nullptr)
	{
		CFile = RD->FD;
		while (CFile != nullptr)
		{
			if (CFile->Handle == handle) { SetCPathVol(); goto label1; }
			if ((CFile->XF != nullptr) && (CFile->XF->Handle == handle))
			{
				SetCPathVol(); 
				CExtToX(); 
				goto label1;
			}
			if ((CFile->TF != nullptr) && (CFile->TF->Handle == handle))
			{
				SetCPathVol(); 
				CExtToT(); 
				goto label1;
			}
			CFile = (FileD*)CFile->Chain;
		} RD = RD->ChainBack;
	}
	RdMsg(799);
	CPath = MsgLine;
label1:
	CFile = cf;
}

WORD GetCatIRec(pstring Name, bool MultiLevel)
{
	longint i = 0; FileD* CF = nullptr; RdbD* R = nullptr; void* CR = nullptr;
	WORD result = 0;
	if (CatFD == nullptr || CatFD->Handle == nullptr) return result;
	if (CRdb == nullptr) return result;
	CF = CFile; CR = CRecPtr; CFile = CatFD; 
	CRecPtr = GetRecSpace();
	R = CRdb;
label1:
	for (i = 1; i <= CatFD->NRecs; i++)
	{
		ReadRec(i);
		if (SEquUpcase(TrailChar(' ', _ShortS(CatRdbName)), R->FD->Name) &&
			SEquUpcase(TrailChar(' ', _ShortS(CatFileName)), Name))
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
	CFile = CatFD; p = GetRecSpace(); q = GetRecSpace();
	CRecPtr = q; last = Frst + N - 1;
	if (I > 0)
		while (I > 0) {
			ReadRec(Frst); CRecPtr = p;
			for (j = 1; j < N - 1; j++) { ReadRec(Frst + j); WriteRec(Frst + j - 1); }
			CRecPtr = q; WriteRec(last); I--;
		}
	else
		while (I < 0) {
			ReadRec(last); CRecPtr = p;
			for (j = 1; j < N - 1; j++) { ReadRec(last - j); WriteRec(last - j + 1); }
			CRecPtr = q; WriteRec(Frst); I++;
		}
	ReleaseStore(p);
}

pstring RdCatField(WORD CatIRec, FieldDPtr CatF)
{
	FileD* CF = CFile;
	void* CR = CRecPtr;
	CFile = CatFD;
	CRecPtr = GetRecSpace();
	ReadRec(CatIRec);
	auto result = TrailChar(' ', _ShortS(CatF));
	ReleaseStore(CRecPtr);
	CFile = CF;
	CRecPtr = CR;
	return result;
}

void WrCatField(WORD CatIRec, FieldDescr* CatF, pstring Txt)
{
	FileD* CF = CFile; 
	void* CR = CRecPtr; 
	CFile = CatFD; 
	CRecPtr = GetRecSpace();
	ReadRec(CatIRec);
	S_(CatF, Txt);
	WriteRec(CatIRec);
	ReleaseStore(CRecPtr);
	CFile = CF; 
	CRecPtr = CR;
}

void WrCatField(FileD* catFD, WORD CatIRec, FieldDescr* CatF, const std::string& Txt)
{
	BYTE* record = new BYTE[catFD->RecLen];
	ReadRec(catFD, CatIRec, record);
	S_(CatF, Txt, record);
	WriteRec(catFD, CatIRec, record);
	delete[] record;
}

void RdCatPathVol(WORD CatIRec)
{
	CPath = FExpand(RdCatField(CatIRec, CatPathName));
	FSplit(CPath, CDir, CName, CExt);
	CVol = RdCatField(CatIRec, CatVolume);
}

bool SetContextDir(pstring& D, bool& IsRdb)
{
	bool result = true;;
	RdbDPtr R = CRdb;
	IsRdb = false;
	while (R != nullptr) {
		FileDPtr F = R->FD;
		if ((CFile == F) && (CFile->CatIRec != 0)) {
			D = R->RdbDir;
			IsRdb = true;
			return result;
		};
		while (F != nullptr) {
			if (CFile == F) {
				if ((CFile == R->HelpFD) || (CFile->Typ == '0'))  //.RDB
					D = R->RdbDir;
				else D = R->DataDir;
				return result;
			}
			F = (FileD*)F->Chain;
		}
		R = R->ChainBack;
	}
	return false;
}

void GetCPathForCat(WORD I)
{
	pstring d;
	bool isRdb;

	CVol = RdCatField(I, CatVolume);
	CPath = RdCatField(I, CatPathName);
	bool setContentDir = SetContextDir(d, isRdb);
	if (CPath[2] != ':' && setContentDir) {
		if (isRdb) {
			FSplit(CPath, CDir, CName, CExt);
			AddBackSlash(d);
			CDir = d;
			CPath = CDir + CName + CExt; return;
		}
		if (CPath[1] == '\\') CPath = copy(d, 1, 2) + CPath;
		else {
			AddBackSlash(d); 
			CPath = d + CPath;
		}
	}
	else {
		CPath = FExpand(CPath);
		//CPath = d + CPath.substr(3, 255);
	}
	FSplit(CPath, CDir, CName, CExt);
}

void SetCPathVol()
{
	WORD i = 0;
	bool isRdb = false;

	CVol = "";
	if (CFile->Typ == 'C') {
		CDir = GetEnv("FANDCAT");
		if (CDir == "") {
			if (TopDataDir == "") CDir = TopRdbDir;
			else CDir = TopDataDir;
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
	switch (CFile->Typ) {
	case '0': CExt = ".RDB"; break;
	case '8': CExt = ".DTA"; break;
	case 'D': CExt = ".DBF"; break;
	default: CExt = ".000";
	}
	if (SetContextDir(CDir, isRdb)) goto label2;
	if (CFile == &HelpFD) {
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
	CPath = CDir + CName + CExt;
}

void SetTxtPathVol(pstring* Path, WORD CatIRec)
{
	if (CatIRec != 0) RdCatPathVol(CatIRec);
	else { CPath = FExpand(*Path); CVol = ""; }
}

void SetTempCExt(char Typ, bool IsNet)
{
	char Nr;
	if (Typ == 'T') {
		Nr = '2';
	switch (CFile->Typ) { case '0': CExt = ".TTT"; break; case 'D': CExt = ".DBT"; break; }
	}
	else {
		Nr = '1';
	switch (CFile->Typ) { case '0': CExt = ".RDB"; break; case 'D': CExt = ".DBF"; break; };
	}
	if (CExt.length() < 2) CExt = ".0"; CExt[2] = Nr;
	if (IsNet) CPath = WrkDir + CName + CExt; /* work files are local */
	else CPath = CDir + CName + CExt;
}

FileD* OpenDuplF(bool CrTF)
{
	FileDPtr OldFD, FD; WORD N; integer Len; bool net;
	SetCPathVol(); net = IsNetCVol();
	N = sizeof(FileD) - 1 + CFile->Name.length(); OldFD = CFile;
	FD = (FileD*)GetStore(N); Move(OldFD, FD, N); CFile = FD;
	/* !!! with FD^ do!!! */
	{ SetTempCExt('0', net); CVol = "";
	FD->Handle = OpenH(_isoverwritefile, Exclusive); TestCFileError();
	FD->NRecs = 0; FD->IRec = 0; FD->Eof = true; FD->UMode = Exclusive;
	if (FD->Typ == 'X')
	{
		FD->XF = (XFile*)GetStore(sizeof(XFile)); FD->XF->Handle = nullptr;
		FD->XF->NoCreate = true;
		/*else xfile name identical with orig file*/
	}; }
	if (CrTF && (FD->TF != nullptr))
	{
		FD->TF = (TFile*)GetStore(sizeof(TFile));
		Move(OldFD->TF, FD->TF, sizeof(TFile));
		/* !!! with FD->TF^ do!!! */
		{
			SetTempCExt('T', net);
			FD->TF->Handle = OpenH(_isoverwritefile, Exclusive); FD->TF->TestErr();
			FD->TF->CompileAll = true; FD->TF->SetEmpty();
			; }
		;
	}
	return FD;
}

void CopyDuplF(FileD* TempFD, bool DelTF)
{
	FILE* h1; FILE* h2; FileD* cf;
	cf = CFile; CFile = TempFD; WrPrefixes(); CFile = cf;
	SaveCache(0); SetTempCExt('0', true); CopyH(TempFD->Handle, CFile->Handle);
	if ((CFile->TF != nullptr) && DelTF) {
		h1 = TempFD->TF->Handle; h2 = CFile->TF->Handle; SetTempCExt('T', true);
		Move(TempFD->TF, CFile->TF, sizeof(TFile)); CFile->TF->Handle = h2;
		CopyH(h1, h2);
	}
	RdPrefixes();
}

void CopyH(FILE* h1, FILE* h2)
{
	const WORD BufSize = 32768;
	void* p; longint sz;
	// cache nepouzivame
	//ClearCacheH(h1); 
	//ClearCacheH(h2);
	p = GetStore(BufSize); 
	sz = FileSizeH(h1);
	SeekH(h1, 0); 
	SeekH(h2, 0);
	while (sz > BufSize) {
		ReadH(h1, BufSize, p); 
		WriteH(h2, BufSize, p); sz -= BufSize;
	}
	ReadH(h1, sz, p);
	WriteH(h2, sz, p);
	CloseH(h1); 
	MyDeleteFile(CPath); 
	ReleaseStore(p);
}

void SubstDuplF(FileD* TempFD, bool DelTF)
{
	FileD* PrimFD; FileD* FD;  TFile* MD;
	pstring p, ptmp, pt; XFile* xf2; FileUseMode um; bool net;

	XFNotValid(); SetCPathVol();
	if (IsNetCVol()) { CopyDuplF(TempFD, DelTF); return; }
	SaveCache(0); PrimFD = CFile; p = CPath; CExtToT(); pt = CPath;
	/* !!! with PrimFD^ do!!! */ {
		CloseClearH(PrimFD->Handle); MyDeleteFile(p); TestDelErr(&p);
		FD = (FileD*)PrimFD->Chain; MD = PrimFD->TF;
		xf2 = PrimFD->XF; um = PrimFD->UMode;
		Move(TempFD, PrimFD, sizeof(FileD) - 2);
		PrimFD->Chain = FD; PrimFD->XF = xf2; PrimFD->UMode = um;
		CloseClearH(PrimFD->Handle);
		SetTempCExt('0', false); ptmp = CPath;
		RenameFile56(ptmp, p, true);
		CPath = p;
		PrimFD->Handle = OpenH(_isoldfile, PrimFD->UMode);
		SetUpdHandle(PrimFD->Handle);
		if ((MD != nullptr) && DelTF) {
			CloseClearH(MD->Handle); MyDeleteFile(pt); TestDelErr(&pt);
			Move(PrimFD->TF, MD, sizeof(TFile)); PrimFD->TF = MD;
			CloseClearH(MD->Handle);
			CPath = ptmp; SetTempCExt('T', false);
			RenameFile56(CPath, pt, true);
			CPath = pt; MD->Handle = OpenH(_isoldfile, PrimFD->UMode);
			SetUpdHandle(MD->Handle);
		}
		PrimFD->TF = MD;
	}
}

void TestDelErr(pstring* P)
{
	if (HandleError != 0) { SetMsgPar(*P); RunError(827); }
}

void DelDuplF(FileD* TempFD)
{
	CloseClearH(TempFD->Handle);
	SetCPathVol();
	SetTempCExt('0', CFile->IsShared());
	MyDeleteFile(CPath);
}
