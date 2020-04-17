#include "oaccess.h"

#include "base.h"
#include "common.h"
#include "handle.h"
#include "legacy.h"
#include "memory.h"
#include "obaseww.h"
#include "recacc.h"
#include "runfrml.h"

void CloseFANDFiles(bool FromDML)
{
	RdbDPtr RD;
	RD = CRdb; while (RD != nullptr) {
		CFile = RD->FD;
		while (CFile != nullptr) {
			if (!FromDML) CFile->ExLMode = CFile->LMode;
			CloseFile();
			CFile = CFile->Chain;
		}
		RD = RD->ChainBack;
	}
	if (CRdb != nullptr) { CFile = CatFD; CloseFile(); }
	CFile = HelpFD;
	CloseFile();
	CloseH(TWork.Handle);
	CloseH(XWork.Handle);
}

void OpenFANDFiles(bool FromDML)
{
	RdbDPtr RD;
	LockMode md;

	OpenXWorkH();
	OpenTWorkH();
	CFile = HelpFD;
	OpenF(RdOnly);
	if (CRdb == nullptr) exit(0);
	CFile = CatFD;
	OpenF(Exclusive);
	RD = CRdb;
	while (RD != nullptr) {
		CFile = RD->FD;
		if (IsTestRun) OpenF(Exclusive);
		else OpenF(RdOnly);
		CFile = CFile->Chain;
		while (!FromDML && (CFile != nullptr)) {
			/*with CFile^*/
			if (CFile->ExLMode != NullMode)
			{
				OpenF(Shared);
				md = NewLMode(CFile->ExLMode);
			}
			CFile = CFile->Chain;
		}
		RD = RD->ChainBack;
	}

}

bool OpenF(FileUseMode UM)
{
	bool result = true;
	if (CFile->Handle != 0xFF) { exit(0); }
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

void TruncF()
{
	/* with CFile^ */
	LockMode md; longint sz;
	if (CFile->UMode == RdOnly) exit(0);
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
	if (CFile->Handle == 0xFF) exit(0);
	if (CFile->IsShared()) OldLMode(NullMode);
	else WrPrefixes();
	SaveCache(0);
	TruncF();
	if (CFile->Typ == 'X')  /*with XF^*/
		if (CFile->XF->Handle != 0xFF) {
			CloseClearH(CFile->Handle);
			if (!CFile->IsShared())
			{
				if (CFile->XF->NotValid) goto label1;
				if ((CFile->XF->NRecs == 0) || CFile->NRecs == 0) {
					CFile->NRecs = 0;
				label1:
					SetCPathVol();
					CExtToX();
					DeleteFile(CPath);
				}
			}
			if (CFile->TF != nullptr)  /*with TF^*/
				if (CFile->TF->Handle != 0xFF) {
					CloseClearH(CFile->TF->Handle);
					if ((!CFile->IsShared) && (CFile->NRecs == 0) && (CFile->Typ != 'D')) {
						SetCPathVol();
						CExtToT();
						DeleteFile(CPath);
					}
				}
			CloseClearH(CFile->Handle); CFile->LMode = NullMode;
			if (!CFile->IsShared && (CFile->NRecs == 0) && (CFile->Typ != 'D')) {
				SetCPathVol();
				DeleteFile(CPath);
			}
			if (CFile->WasRdOnly) {
				CFile->WasRdOnly = false;
				SetCPathVol();
				SetFileAttr((GetFileAttr && 0x27) || 0x01); // {RdONly; }
				if (CFile->TF != nullptr) {
					CExtToT();
					SetFileAttr((GetFileAttr && 0x27) || 0x1); //  {RdONly; }
				}
			}

		}
}

string RdCatField(WORD CatIRec, FieldDPtr CatF)
{
	FileDPtr CF; void* CR;

	CF = CFile; CR = CRecPtr; CFile = CatFD;
	CRecPtr = GetRecSpace();
	ReadRec(CatIRec);
	auto result = runfrml::TrailChar(' ', _shorts(CatF));
	ReleaseStore(CRecPtr); CFile = CF; CRecPtr = CR;
	return result;
}

bool SetContextDir(DirStr D, bool& IsRdb)
{
	RdbDPtr R; FileDPtr F;

	bool result = true;;
	R = CRdb;
	IsRdb = false;
	while (R != nullptr) {
		F = R->FD;
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
			F = F->Chain;
		}
		R = R->ChainBack;
	}
	return false;
}

void GetCPathForCat(WORD I)
{
	DirStr d;
	bool isRdb;

	CVol = RdCatField(I, CatVolume);
	CPath = RdCatField(I, CatPathName);
	if (CPath[2] != ':' && SetContextDir(d, isRdb)) {
		if (isRdb) {
			FSplit(CPath, CDir, CName, CExt);
			AddBackSlash(d);
			CDir = d;
			CPath = CDir + CName + CExt; return;
		}
		if (CPath[1] == '\\') CPath = copy(d, 1, 2) + CPath;
		else {
			AddBackSlash(d); CPath = d + CPath;
		}
	}
	else CPath = FExpand(CPath);
	FSplit(CPath, CDir, CName, CExt);
}

void SetCPathVol()
{
	WORD i;
	bool isRdb;

	CVol = "";
	if (CFile->Typ == 'C') {
		CDir = getenv("FANDCAT");
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
	case '0': CExt = '.RDB'; break;
	case '8': CExt = '.DTA'; break;
	case 'D': CExt = '.DBF'; break;
	default: CExt = '.000';
	}
	if (SetContextDir(CDir, isRdb)) goto label2;
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
	CDir = CRdb->DataDir;
label2:
	AddBackSlash(CDir);
label3:
	CName = CFile->Name;
label4:
	CPath = CDir + CName + CExt;
}
