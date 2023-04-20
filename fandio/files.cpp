#include "files.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/Coding.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/wwmix.h"
#include "../Common/compare.h"


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

void SetCPathMountVolSetNet(FileUseMode UM)
{
	SetCPathVol(CFile);
	CFile->FF->UMode = UM;
	CFile->FF->Drive = (BYTE)TestMountVol(CPath[0]);
	if (!IsNetCVol() || (CFile == Chpt))
		switch (UM) {
		case RdShared: CFile->FF->UMode = RdOnly; break;
		case Shared: CFile->FF->UMode = Exclusive; break;

		case Closed:
		case RdOnly:
		case Exclusive: break;
		}
	else if ((UM == Shared) && EquUpCase(CVol, "#R")) {
		CFile->FF->UMode = RdShared;
	}
}

bool OpenF1(FileD* file_d, const std::string& path, FileUseMode UM)
{
	WORD n;
	bool result = true;
	file_d->FF->LMode = NullMode;
	SetCPathMountVolSetNet(UM);
	const bool b = (file_d == Chpt) || (file_d == CatFD->GetCatalogFile());
	if (b && (IsTestRun || IsInstallRun) && ((GetFileAttr(CPath, HandleError) & 0b00000001/*RdOnly*/) != 0)) {
		SetFileAttr(CPath, HandleError, GetFileAttr(CPath, HandleError) & 0b00100110);
		if (HandleError == 5) HandleError = 79;
		TestCFileError(file_d);
		file_d->FF->WasRdOnly = true;
	}
	while (true) {
		file_d->FF->Handle = OpenH(CPath, _isoldfile, file_d->FF->UMode);
		if ((HandleError != 0) && file_d->FF->WasRdOnly) {
			SetFileAttr(CPath, HandleError, (GetFileAttr(CPath, HandleError) & 0b00100111) | 0b00000001 /*RdONly*/);
			TestCFileError(file_d);
		}
		if ((HandleError == 5) && (file_d->FF->UMode == Exclusive)) {
			file_d->FF->UMode = RdOnly;
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
	if (file_d->FF->TF != nullptr) {
		CPath = CExtToT(CDir, CName, CExt);
		if (file_d->FF->WasRdOnly) {
			SetFileAttr(CPath, HandleError, GetFileAttr(CPath, HandleError) & 0b00100110); // 0x26 = archive + hidden + system
		}
		while (true) {
			file_d->FF->TF->Handle = OpenH(CPath, _isoldfile, file_d->FF->UMode);
			if (HandleError == 2) {
				if (file_d->FF->TF->Format == file_d->FF->TF->DbtFormat) {
					file_d->FF->TF->Format = file_d->FF->TF->FptFormat;
					CExt = ".FPT";
					CPath = CDir + CName + CExt;
					continue;
				}
				if (file_d->IsDynFile) {
					CloseClearH(&file_d->FF->Handle);
					result = false;
					return result;
				}
			}
			break;
		}
		if (HandleError != 0) {
			n = HandleError;
			CloseClearHCFile(file_d->FF);
			HandleError = n;
			TestCPathError();
			return result;
		}
	}

	// open index file (*.X__)
	if (file_d->FF->file_type == FileType::INDEX) {
		CPath = CExtToX(CDir, CName, CExt);
		while (true) {
			file_d->FF->XF->Handle = OpenH(CPath, _isoldfile, file_d->FF->UMode);
			if (HandleError == 2) {
				file_d->FF->XF->Handle = OpenH(CPath, _isoverwritefile, Exclusive);
				if (HandleError != 0) {
					n = HandleError;
					CloseClearHCFile(file_d->FF);
					HandleError = n;
					TestCPathError();
					return result;
				}
				file_d->FF->XF->SetNotValid();
				CloseH(&file_d->FF->XF->Handle);
				continue;
			}
			if (HandleError != 0) {
				n = HandleError;
				CloseClearHCFile(file_d->FF);
				HandleError = n;
				TestCPathError();
			}
			if (file_d->FF->XF != nullptr && FileSizeH(file_d->FF->XF->Handle) < 512) {
				file_d->FF->XF->SetNotValid();
			}
			break;
		}
	}
	return result;
}

bool OpenF2(FileD* file_d, const std::string& path)
{
	wwmix ww;

	int FS = 0, n = 0, l = 0;
	WORD Signum = 0, rLen = 0;
	LockMode md = NullMode;
	FS = FileSizeH(file_d->FF->Handle);
	file_d->FF->NRecs = 0;
	bool result = false;
	if (FS < file_d->FF->FrstDispl) goto label1;
	rLen = file_d->FF->RdPrefix();
	n = (FS - file_d->FF->FrstDispl) / file_d->FF->RecLen;
	if (rLen != 0xffff) {
		if (file_d->IsDynFile) {
			CloseClearHCFile(file_d->FF);
			return result;
		}
		else {
			if (CatFD->OldToNewCat(FS)) {
				goto label3;
			}
			FileMsg(file_d, 883, ' ');
			l = file_d->FF->NRecs * rLen + file_d->FF->FrstDispl;
			if (l == FS || !PromptYN(885)) {
				CloseGoExit(file_d->FF);
			}
			if (file_d->FF->NRecs == 0 || l >> CachePageShft != FS >> CachePageShft) {
				WrLLF10Msg(886);
				file_d->FF->NRecs = n;
			}
			goto label2;
		}
	}
	else {
	}
	if (n < file_d->FF->NRecs) {
		SetCPathVol(file_d);
		SetMsgPar(CPath);
		if (PromptYN(882)) {
			file_d->FF->NRecs = n;
		label1:
			if (file_d->FF->IsShared() && (file_d->FF->LMode < ExclMode)) {
				file_d->ChangeLockMode(ExclMode, 0, false);
			}
			file_d->FF->LMode = ExclMode;
		label2:
			SetUpdHandle(file_d->FF->Handle);
			file_d->FF->WrPrefix();
		}
		else {
			CloseGoExit(file_d->FF);
		}
	}
label3:
	if (file_d->FF->TF != nullptr) {
		if (FS < file_d->FF->FrstDispl) {
			file_d->FF->TF->SetEmpty();
		}
		else {
			file_d->FF->TF->RdPrefix(true);
			if ((file_d->FF->file_type == FileType::RDB) && !IsActiveRdb(file_d) && !Coding::HasPassword(file_d, 1, "")) {
				FileMsg(file_d, 616, ' ');
				CloseGoExit(file_d->FF);
			}
		}
	}
	if (file_d->FF->file_type == FileType::INDEX) {
		if (FS < file_d->FF->FrstDispl) {
			file_d->FF->XF->SetNotValid();
		}
		else {
			RdWrCache(READ, file_d->FF->XF->Handle, file_d->FF->XF->NotCached(), 0, 2, &Signum);
			file_d->FF->XF->RdPrefix();
			if (
				!file_d->FF->XF->NotValid && ((Signum != 0x04FF) || (file_d->FF->XF->NRecsAbs != file_d->FF->NRecs)
					|| (file_d->FF->XF->FreeRoot > file_d->FF->XF->MaxPage)
					|| (((file_d->FF->XF->MaxPage + 1) << XPageShft) > FileSizeH(file_d->FF->XF->Handle)))
				|| (file_d->FF->XF->NrKeys != 0) && (file_d->FF->XF->NrKeys != file_d->GetNrKeys()))
			{
				if (!EquUpCase(GetEnv("FANDMSG830"), "NO")) {
					FileMsg(file_d, 830, 'X');
				}
				if (file_d->FF->IsShared() && (file_d->FF->LMode < ExclMode)) {
					file_d->ChangeLockMode(ExclMode, 0, false);
				}
				file_d->FF->LMode = ExclMode;
				file_d->FF->XF->SetNotValid();
			}
		}
	}
	file_d->SeekRec(0);
	return true;
}

bool OpenF(FileD* file_d, const std::string& path, FileUseMode UM)
{
	bool result = true;
	if (file_d->FF->Handle != nullptr) return result;
	if (OpenF1(file_d, path, UM)) {
		if (
#ifdef FandSQL
			!IsSQLFile &&
#endif
			file_d->FF->IsShared()) {
			file_d->ChangeLockMode(RdMode, 0, false);
			file_d->FF->LMode = RdMode;
		}
		result = OpenF2(file_d, path);
		file_d->OldLockMode(NullMode);
	}
	else result = false;
	return result;
}