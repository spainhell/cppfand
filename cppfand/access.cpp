#include "access.h"

#include "Coding.h"
#include "../pascal/real48.h"
#include "../pascal/asm.h"
#include "compile.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "olongstr.h"
#include "runfrml.h"
#include "../fandio/FandFile.h"
#include "../Logging/Logging.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"

void ResetCFileUpdH()
{
	ResetUpdHandle(CFile->FF->Handle);
	if (CFile->FF->file_type == FileType::INDEX) ResetUpdHandle(CFile->FF->XF->Handle);
	if (CFile->FF->TF != nullptr) ResetUpdHandle(CFile->FF->TF->Handle);
}

void ClearCacheCFile()
{
	// cache nepouzivame
	return;
	/* !!! with CFile^ do!!! */
	/*ClearCacheH(CFile->Handle);
	if (CFile->file_type == INDEX) ClearCacheH(CFile->GetXFile->Handle);
	if (CFile->TF != nullptr) ClearCacheH(CFile->TF->Handle);*/
}



void CloseClearHCFile(FandFile* fand_file)
{
	CloseClearH(&fand_file->Handle);
	if (fand_file->file_type == FileType::INDEX) {
		CloseClearH(&fand_file->XF->Handle);
	}
	if (fand_file->TF != nullptr) {
		CloseClearH(&fand_file->TF->Handle);
	}
}

void CloseGoExit(FandFile* fand_file)
{
	CloseClearHCFile(fand_file);
	GoExit();
}

BYTE ByteMask[_MAX_INT_DIG];


//const BYTE FixS = 8;
//BYTE Fix[FixS];
//BYTE RealMask[DblS + 1];



void TestCPathError()
{
	WORD n;
	if (HandleError != 0) {
		n = 700 + HandleError;
		if ((n == 705) && (CPath[CPath.length()] == '\\')) n = 840;
		SetMsgPar(CPath);
		RunError(n);
	}
}

void NegateESDI()
{
	// asm  jcxz @2; @1:not es:[di].byte; inc di; loop @1; @2:
}

bool IsNullValue(void* p, WORD l)
{
	BYTE* pb = (BYTE*)p;
	for (size_t i = 0; i < l; i++) {
		if (pb[i] != 0xFF) return false;
	}
	return true;
}

void DelTFld(FieldDescr* F)
{
	int n = CFile->loadT(F, CRecPtr);
	if (CFile->HasTWorkFlag(CRecPtr)) {
		TWork.Delete(n);
	}
	else {
		LockMode md = CFile->NewLockMode(WrMode);
		CFile->FF->TF->Delete(n);
		CFile->OldLockMode(md);
	}
	CFile->saveT(F, 0, CRecPtr);
}

void DelDifTFld(void* Rec, void* CompRec, FieldDescr* F)
{
	void* cr = CRecPtr;
	CRecPtr = CompRec;
	int n = CFile->loadT(F, CRecPtr);
	CRecPtr = Rec;
	if (n != CFile->loadT(F, CRecPtr)) DelTFld(F);
	CRecPtr = cr;
}

const WORD Alloc = 2048;



void ZeroAllFlds(FileD* file_d, void* record)
{
	FillChar(record, file_d->FF->RecLen, 0);
	for (auto& F : file_d->FldD) {
		if (((F->Flg & f_Stored) != 0) && (F->field_type == FieldType::ALFANUM)) {
			CFile->saveS(F, "", CRecPtr);
		}
	}
}

bool LinkLastRec(FileD* FD, int& N, bool WithT)
{
	CFile = FD;
	CRecPtr = FD->GetRecSpace();
	LockMode md = CFile->NewLockMode(RdMode);
	auto result = true;
#ifdef FandSQL
	if (FD->IsSQLFile)
	{
		if (Strm1->SelectXRec(nullptr, nullptr, _equ, WithT)) N = 1; else goto label1;
	}
	else
#endif
	{
		N = CFile->FF->NRecs;
		if (N == 0) {
		label1:
			ZeroAllFlds(CFile, CRecPtr);
			result = false;
			N = 1;
		}
		else CFile->ReadRec(N, CRecPtr);
	}
	CFile->OldLockMode(md);
	return result;
}

// ulozi hodnotu parametru do souboru
void AsgnParFldFrml(FileD* FD, FieldDescr* F, FrmlElem* Z, bool Ad)
{
	//#ifdef _DEBUG
	std::string FileName = FD->FullPath;
	std::string Varible = F->Name;
	//#endif
	void* p = nullptr; int N = 0; LockMode md; bool b = false;
	FileD* cf = CFile; void* cr = CRecPtr; CFile = FD;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		CRecPtr = GetRecSpace; ZeroAllFlds; AssgnFrml(F, Z, true, Ad);
		Strm1->UpdateXFld(nullptr, nullptr, F); ClearRecSpace(CRecPtr)
	}
	else
#endif
	{
		md = CFile->NewLockMode(WrMode);
		if (!LinkLastRec(CFile, N, true)) {
			CFile->IncNRecs(1);
			CFile->WriteRec(N, CRecPtr);
		}
		AssgnFrml(CFile, CRecPtr, F, Z, true, Ad);
		CFile->WriteRec(N, CRecPtr);
		CFile->OldLockMode(md);
	}
	ReleaseStore(CRecPtr);
	CFile = cf; CRecPtr = cr;
}

// zrejme zajistuje pristup do jine tabulky (cizi klic)
bool LinkUpw(LinkD* LD, int& N, bool WithT)
{
	FileD* ToFD = LD->ToFD;
	FileD* CF = CFile;
	void* CP = CRecPtr;
	XKey* K = LD->ToKey;

	XString x;
	x.PackKF(CFile, LD->Args, CRecPtr);

	CFile = ToFD;
	void* RecPtr = CFile->GetRecSpace();
	CRecPtr = RecPtr;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		LU = Strm1->SelectXRec(K, @X, _equ, WithT); N = 1;
		if (LU) goto label2; else goto label1;
	}
#endif
	const LockMode md = CFile->NewLockMode(RdMode);
	bool lu;
	if (ToFD->FF->file_type == FileType::INDEX) {
		CFile->FF->TestXFExist();
		lu = K->SearchInterval(CFile, x, false, N);
	}
	else if (CFile->FF->NRecs == 0) {
		lu = false;
		N = 1;
	}
	else {
		lu = CFile->FF->SearchKey(x, K, N, CRecPtr);
	}

	if (lu) {
		CFile->ReadRec(N, CRecPtr);
	}
	else {
		bool b = false;
		double r = 0.0;
		ZeroAllFlds(CFile, CRecPtr);
		const KeyFldD* KF = K->KFlds;
		for (auto& arg : LD->Args) {
			FieldDescr* F = arg->FldD;
			FieldDescr* F2 = KF->FldD;
			CFile = CF;
			CRecPtr = CP;
			if ((F2->Flg & f_Stored) != 0)
				switch (F->frml_type) {
				case 'S': {
					x.S = CFile->loadOldS(F, CRecPtr);
					CFile = ToFD;
					CRecPtr = RecPtr;
					CFile->saveS(F2, x.S, CRecPtr);
					break;
				}
				case 'R': {
					r = CFile->loadR(F, CRecPtr);
					CFile = ToFD; CRecPtr = RecPtr;
					CFile->saveR(F2, r, CRecPtr);
					break;
				}
				case 'B': {
					b = CFile->loadB(F, CRecPtr);
					CFile = ToFD; CRecPtr = RecPtr;
					CFile->saveB(F2, b, CRecPtr);
					break;
				}
				}
			KF = KF->pChain;
		}
		CFile = ToFD;
		CRecPtr = RecPtr;
	}

	auto result = lu;
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		CFile->OldLockMode(md);
	return result;
}

void AssignNRecs(bool Add, int N)
{
	int OldNRecs; LockMode md;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if ((N = 0) && !Add) Strm1->DeleteXRec(nullptr, nullptr, false); return;
	}
#endif
	md = CFile->NewLockMode(DelMode);
	OldNRecs = CFile->FF->NRecs;
	if (Add) N = N + OldNRecs;
	if ((N < 0) || (N == OldNRecs)) goto label1;
	if ((N == 0) && (CFile->FF->TF != nullptr)) CFile->FF->TF->SetEmpty();
	if (CFile->FF->file_type == FileType::INDEX) {
		if (N == 0) {
			CFile->FF->NRecs = 0;
			SetUpdHandle(CFile->FF->Handle);
			int result = CFile->FF->XFNotValid();
			if (result != 0) {
				RunError(result);
			}
			goto label1;
		}
		else {
			SetMsgPar(CFile->Name);
			CFile->RunErrorM(md);
			RunError(821);
		}
	}
	if (N < OldNRecs) {
		CFile->DecNRecs(OldNRecs - N);
		goto label1;
	}
	CRecPtr = CFile->GetRecSpace();
	ZeroAllFlds(CFile, CRecPtr);
	CFile->SetDeletedFlag(CRecPtr);
	CFile->IncNRecs(N - OldNRecs);
	for (int i = OldNRecs + 1; i <= N; i++) {
		CFile->WriteRec(i, CRecPtr);
	}
	ReleaseStore(CRecPtr);
label1:
	CFile->OldLockMode(md);
}

void ClearRecSpace(void* p)
{
	void* cr = nullptr;
	if (CFile->FF->TF != nullptr) {
		cr = CRecPtr;
		CRecPtr = p;
		if (CFile->HasTWorkFlag(CRecPtr)) {
			for (auto& f : CFile->FldD) {
				if (((f->Flg & f_Stored) != 0) && (f->field_type == FieldType::TEXT)) {
					TWork.Delete(CFile->loadT(f, CRecPtr));
					CFile->saveT(f, 0, CRecPtr);
				}
			}
		}
		CRecPtr = cr;
	}
}

void DelTFlds()
{
	for (auto& F : CFile->FldD) {
		if (((F->Flg & f_Stored) != 0) && (F->field_type == FieldType::TEXT)) {
			DelTFld(F);
		}
	}
}

void CopyRecWithT(void* p1, void* p2)
{
	memcpy(p2, p1, CFile->FF->RecLen);
	for (auto& F : CFile->FldD) {
		if ((F->field_type == FieldType::TEXT) && ((F->Flg & f_Stored) != 0)) {
			FandTFile* tf1 = CFile->FF->TF;
			FandTFile* tf2 = tf1;
			CRecPtr = p1;
			if ((tf1->Format != FandTFile::T00Format)) {
				LongStr* s = CFile->loadLongS(F, CRecPtr);
				CRecPtr = p2;
				CFile->saveLongS(F, s, CRecPtr);
				ReleaseStore(s);
			}
			else {
				if (CFile->HasTWorkFlag(CRecPtr)) {
					tf1 = &TWork;
				}
				int pos = CFile->loadT(F, CRecPtr);
				CRecPtr = p2;
				if (CFile->HasTWorkFlag(CRecPtr)) {
					tf2 = &TWork;
				}
				pos = CopyTFString(tf2, CFile, tf1, pos);
				CFile->saveT(F, pos, CRecPtr);
			}
		}
	}
}

LocVar* LocVarBlkD::GetRoot()
{
	if (this->vLocVar.size() > 0) return this->vLocVar[0];
	return nullptr;
}

LocVar* LocVarBlkD::FindByName(std::string Name)
{
	for (auto& i : vLocVar) {
		if (EquUpCase(Name, i->Name)) return i;
	}
	return nullptr;
}

void* LocVarAd(LocVar* LV)
{
	return nullptr;
}

void DirMinusBackslash(pstring& D)
{
	if ((D.length() > 3) && (D[D.length() - 1] == '\\')) D[0]--;
}

int StoreInTWork(LongStr* S)
{
	return TWork.Store(S->A, S->LL);
}

LongStr* ReadDelInTWork(int Pos)
{
	auto result = TWork.Read(Pos);
	TWork.Delete(Pos);
	return result;
}

void ForAllFDs(ForAllFilesOperation op, FileD** file_d, WORD i)
{
	FileD* cf = CFile;
	RdbD* R = CRdb;
	while (R != nullptr) {
		CFile = R->FD;
		while (CFile != nullptr) {
			switch (op) {
			case ForAllFilesOperation::close: {
				CFile->Close();
				break;
			}
			case ForAllFilesOperation::save: {
				break;
			}
			case ForAllFilesOperation::clear_xf_update_lock: {
				CFile->FF->ClearXFUpdLock();
				break;
			}
			case ForAllFilesOperation::save_l_mode: {
				CFile->FF->ExLMode = CFile->FF->LMode;
				break;
			}
			case ForAllFilesOperation::set_old_lock_mode: {
				CFile->OldLockMode(CFile->FF->ExLMode);
				break;
			}
			case ForAllFilesOperation::close_passive_fd: {
				if ((CFile->FF->file_type != FileType::RDB) && (CFile->FF->LMode == NullMode)) {
					CloseFile(CFile);
				}
				break;
			}
			case ForAllFilesOperation::find_fd_for_i: {
				if ((*file_d == nullptr) && (CFile->CatIRec == i)) {
					*file_d = CFile;
				}
				break;
			}
			default:;
			}
			CFile = CFile->pChain;
		}
		R = R->ChainBack;
	}
	CFile = cf;
}

bool IsActiveRdb(FileD* FD)
{
	RdbD* R = CRdb;
	while (R != nullptr) {
		if (FD == R->FD) return true;
		R = R->ChainBack;
	}
	return false;
}

void ResetCompilePars()
{
	RdFldNameFrml = RdFldNameFrmlF;
	RdFunction = nullptr;
	ChainSumEl = nullptr;
	FileVarsAllowed = true;
	FDLocVarAllowed = false;
	IdxLocVarAllowed = false;
	PrevCompInp.clear();
}

std::string TranslateOrd(std::string text)
{
	std::string trans;
	for (size_t i = 0; i < text.length(); i++) {
		char c = CharOrdTab[(BYTE)text[i]];
#ifndef FandAng
		if (c == 0x49 && trans.length() > 0) {       // znak 'H'
			if (trans[trans.length() - 1] == 0x43) { // posledni znak ve vystupnim retezci je 'C' ?
				trans[trans.length() - 1] = 0x4A;    // na vstupu bude 'J' jako 'CH'
				continue;
			}
		}
#endif
		trans += c;
	}
	return trans;
}

std::string CExtToX(const std::string dir, const std::string name, std::string ext)
{
	ext[1] = 'X';
	return dir + name + ext;
}

std::string CExtToT(const std::string& dir, const std::string& name, std::string ext)
{
	if (EquUpCase(ext, ".RDB")) ext = ".TTT";
	else if (EquUpCase(ext, ".DBF")) {
		if (CFile->FF->TF->Format == FandTFile::FptFormat) {
			ext = ".FPT";
		}
		else {
			ext = ".DBT";
		}
	}
	else ext[1] = 'T';
	return dir + name + ext;
}