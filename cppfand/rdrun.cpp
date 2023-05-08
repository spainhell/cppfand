#include "rdrun.h"

#include "AddD.h"
#include "ChkD.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "runfrml.h"
#include "models/Instr.h"
#include "../fandio/files.h"

std::vector<ConstListEl> OldMFlds;
std::vector<ConstListEl> NewMFlds;   /* Merge + Report*/
InpD* IDA[30];
short MaxIi;
XString OldMXStr;                  /* Merge */
OutpFD* OutpFDRoot;
OutpRD* OutpRDs;
bool Join;
bool PrintView;                  /* Report */
TextFile Rprt;		// puvodne text - souvisi s text. souborem
BlkD* RprtHd;
BlkD* PageHd;
BlkD* PageFt;
std::vector<FrmlElemSum*> PFZeroLst;
LvDescr* FrstLvM;
LvDescr* LstLvM; /* LstLvM->Ft=RF */
bool SelQuest;
EditD* EditDRoot;
FrmlElem* PgeSizeZ = nullptr, * PgeLimitZ = nullptr;
bool CompileFD, EditRdbMode;
LocVarBlkD LVBD;
std::string CalcTxt = "";
MergOpSt MergOpGroup = { _const, 0.0 };

bool EFldD::Ed(bool IsNewRec)
{
	return ((FldD->Flg & f_Stored) != 0) && (EdU || IsNewRec && EdN);
}

Instr::Instr(PInstrCode kind)
{
	this->Kind = kind;
}

void ResetLVBD()
{
	LVBD.pChain = nullptr;
	LVBD.vLocVar.clear();
	LVBD.NParam = 0;
	LVBD.Size = 2 * 4;
	LVBD.FceName = "";
}

bool Add(AddD* AD, void* RP, double R, bool Back)
{
	auto result = true;
	CRecPtr = RP;
	if (Back) R = -R;
	CFile->saveR(AD->Field, CFile->loadR(AD->Field, CRecPtr) + R, CRecPtr);
	if (AD->Chk == nullptr) return result;
	if (!Back && !RunBool(CFile, AD->Chk->Bool, CRecPtr))
	{
		SetMsgPar(RunShortStr(CFile, AD->Chk->TxtZ, CRecPtr));
		WrLLF10Msg(110);
		result = false;
	}
	return result;
}

bool RunAddUpdte1(char Kind, void* CRold, bool Back, AddD* StopAD, LinkD* notLD)
{
	int n2, n2_old;
	char kind2, kind2_old;
	FileD* cf2 = nullptr;
	BYTE* cr2 = nullptr;
	BYTE* cr2_old = nullptr;
	void* p = nullptr;
	bool b;
	bool result = true;
	double r;
	double r_old;
	FileD* originalCFile = CFile;
	void* originalCRecPtr = CRecPtr;
	MarkStore(p);
	AddD* ADback = nullptr;

	for (AddD* add : CFile->Add) {
		if (add == StopAD) {
			ReleaseStore(&p);
			return result;
		}
		if ((notLD != nullptr) && (add->LD == notLD)) {
			goto label1;
		}
		if (add->Assign) {
			if (Assign(add)) {
				goto label1;
			}
			else {
				goto fail;
			}
		}

		r = RunReal(CFile, add->Frml, CRecPtr);
		if (Kind == '-') {
			r = -r;
		}
		r_old = 0;
		if (Kind == 'd') {
			CRecPtr = CRold;
			r_old = RunReal(CFile, add->Frml, CRecPtr);
		}
		ADback = add;
		cf2 = add->File2;
		n2 = 0;
		n2_old = 0;
		if (r != 0.0) {
			CRecPtr = originalCRecPtr;
			if (!Link(CFile, add, n2, kind2, originalCRecPtr, &cr2)) {
				goto fail;
			}
			//CR2 = (BYTE*)CRecPtr;
		}
		if (r_old != 0.0) {
			CFile = originalCFile;
			CRecPtr = CRold;
			if (!Link(originalCFile, add, n2_old, kind2_old, CRold, &cr2_old)) {
				goto fail;
			}
			//CR2old = (BYTE*)CRecPtr;
			if (n2_old == n2) {
				r = r - r_old;
				if (r == 0.0) {
					goto label1;
				}
				n2_old = 0;
			}
		}
		if ((n2 == 0) && (n2_old == 0)) {
			goto label1;
		}
		CFile = cf2;
		if (n2_old != 0) {
			if (!Add(add, cr2_old, -r_old, Back)) {
				goto fail;
			}
		}
		if (n2 != 0) {
			if (!Add(add, cr2, r, Back)) {
				goto fail;
			}
		}
		if ((n2_old != 0) && !TransAdd(add, originalCFile, originalCRecPtr, cr2_old, n2_old, kind2_old, false)) {
			goto fail;
		}
		if ((n2 != 0) && !TransAdd(add, originalCFile, originalCRecPtr, cr2, n2, kind2, false)) {
			if (n2_old != 0) {
				b = TransAdd(add, originalCFile, originalCRecPtr, cr2_old, n2_old, kind2_old, true);
			}
			goto fail;
		}
		if (n2_old != 0) {
			WrUpdRec(add, originalCFile, originalCRecPtr, cr2_old, n2_old);
		}
		if (n2 != 0) {
			WrUpdRec(add, originalCFile, originalCRecPtr, cr2, n2);
		}
	label1:
		ReleaseStore(&p);
		delete[] cr2; cr2 = nullptr;
		delete[] cr2_old; cr2_old = nullptr;
		CFile = originalCFile;
		CRecPtr = originalCRecPtr;
	}
	return result;
fail:
	ReleaseStore(&p);
	delete[] cr2; cr2 = nullptr;
	delete[] cr2_old; cr2_old = nullptr;
	CFile = originalCFile;
	CRecPtr = originalCRecPtr;
	result = false;
	if (ADback != nullptr) {
		b = RunAddUpdte1(Kind, CRold, true, ADback, notLD);  /* backtracking */
	}
	return result;
}

void CrIndRec(FileD* file_d, void* record)
{
	file_d->CreateRec(file_d->FF->NRecs + 1, record);
	file_d->RecallRec(file_d->FF->NRecs, record);
}

bool Link(FileD* file_d, AddD* add_d, int& n, char& kind2, void* record, BYTE** linkedRecord)
{
	bool result = true;
	LinkD* ld = add_d->LD;
	kind2 = 'd';

	if (ld != nullptr) {
		if (LinkUpw(file_d, ld, n, false, record, linkedRecord)) {
			return result;
		}
		SetMsgPar(ld->RoleName);
	}
	else {
		if (!LinkLastRec(add_d->File2, n, false, linkedRecord)
#ifdef FandSQL
			&& !file_d->IsSQLFile
#endif
			) {
			file_d->IncNRecs(1);
			file_d->WriteRec(1, *linkedRecord);
		}
		return result;
	}
	kind2 = '+';
	if ((add_d->Create == 2) || (add_d->Create == 1) && PromptYN(132)) {
#ifdef FandSQL
		if (file_d->IsSQLFile) Strm1->InsertRec(false, true) else
#endif
		{
			file_d->ClearDeletedFlag(*linkedRecord);
			if ((ld != nullptr) && (file_d->FF->file_type == FileType::INDEX)) {
				CrIndRec(file_d, *linkedRecord);
				n = file_d->FF->NRecs;
			}
			else {
				file_d->CreateRec(n, *linkedRecord);
			}
		}
		return result;
	}
	WrLLF10Msg(119);
	result = false;
	return result;
}

bool TransAdd(AddD* AD, FileD* FD, void* RP, void* CRnew, int N, char Kind2, bool Back)
{
	void* CRold; XString x; LinkD* ld;
	if (CFile->Add.empty() /*== nullptr*/) { return true; }
	if (Kind2 == '+')
	{
		CRecPtr = CRnew; return RunAddUpdte1('+', nullptr, Back, nullptr, nullptr);
	}
	CRold = CFile->GetRecSpace();
	CRecPtr = CRold;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		ld = AD->LD; if (ld = nullptr) Strm1->SelectXRec(nullptr, nullptr, _equ, false)
		else {
			CFile = FD; CRecPtr = RP; x.PackKF(ld->Args);
			CFile = ld->ToFD; CRecPtr = CRold; Strm1->SelectXRec(ld->ToKey, @x, _equ, false)
		}
	}
	else
#endif
		CFile->ReadRec(N, CRecPtr);
	CRecPtr = CRnew;
	auto result = RunAddUpdte1('d', CRold, Back, nullptr, nullptr);
	ReleaseStore(&CRold);
	return result;
}

void WrUpdRec(AddD* AD, FileD* FD, void* RP, void* CRnew, int N)
{
	XString x; LinkD* ld;
	CRecPtr = CRnew;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		ld = AD->LD; if (ld = nullptr) Strm1->UpdateXFld(nullptr, nullptr, AD->Field)
		else {
			CFile = FD; CRecPtr = RP; x.PackKF(ld->Args);
			CFile = ld->ToFD; CRecPtr = CRnew; Strm1->UpdateXFld(ld->ToKey, @x, AD->Field)
		}
	}
	else
#endif
		CFile->WriteRec(N, CRecPtr);
}

bool Assign(AddD* AD)
{
	double R; std::string S;
	pstring ss; bool B;
	int Pos, N2; char Kind2;
	if (!RunBool(CFile, AD->Bool, CRecPtr)) return true;
	FieldDescr* F = AD->Field;
	FrmlElem* Z = AD->Frml;

	switch (F->frml_type) {
	case 'R': {
		R = RunReal(CFile, Z, CRecPtr);
		break;
	}
	case 'S': {
		if (F->field_type == FieldType::TEXT) S = RunStdStr(CFile, Z, CRecPtr);
		else ss = RunShortStr(CFile, Z, CRecPtr);
		break;
	}
	default: {
		B = RunBool(CFile, Z, CRecPtr);
		break;
	}
	}
	
	BYTE* linkedRecord = nullptr;

	if (!Link(CFile, AD, N2, Kind2, CRecPtr, &linkedRecord)) {
		delete[] linkedRecord; linkedRecord = nullptr;
		return false;
	}

	switch (F->frml_type) {
	case 'R': {
		CFile->saveR(F, R, linkedRecord);
		break;
	}
	case 'S': {
		if (F->field_type == FieldType::TEXT) CFile->saveS(F, S, linkedRecord);
		else CFile->saveS(F, ss, linkedRecord);
		break;
	}
	default: {
		CFile->saveB(F, B, linkedRecord);
		break;
	}
	}
	CFile->WriteRec(N2, linkedRecord);

	delete[] linkedRecord; linkedRecord = nullptr;
	return true;
}

bool LockForAdd(FileD* FD, WORD Kind, bool Ta, LockMode& md)
{
	LockMode md1; /*0-ExLMode,1-lock,2-unlock*/
	auto result = false;
	CFile = FD;
	for (AddD* AD : FD->Add) {
		/* !!! with AD^ do!!! */
		if (CFile != AD->File2) {
			CFile = AD->File2;
			switch (Kind) {
			case 0: {
				if (Ta) CFile->FF->TaLMode = CFile->FF->LMode;
				else CFile->FF->ExLMode = CFile->FF->LMode;
				break;
			}
			case 1: {
				md = WrMode;
				if (AD->Create > 0) md = CrMode;
				if (!CFile->TryLockMode(md, md1, 2)) return result;
				break;
			}
			case 2: {
				if (Ta) {
					CFile->OldLockMode(CFile->FF->TaLMode);
				}
				else {
					CFile->OldLockMode(CFile->FF->ExLMode);
				}
				break;
			}
			}
			if (!LockForAdd(CFile, Kind, Ta, md)) return result;
		}
	}
	result = true;
	return result;
}

bool RunAddUpdte(char Kind, void* CRold, LinkD* notLD)
{
	LockMode md;
	FileD* CF = CFile;
	LockForAdd(CF, 0, false, md);
	while (!LockForAdd(CF, 1, false, md)) {
		SetPathAndVolume(CFile);
		SetMsgPar(CPath, LockModeTxt[md]);
		LockForAdd(CF, 2, false, md);
		int w = PushWrLLMsg(825, false);
		KbdTimer(spec.NetDelay, 0);
		if (w != 0) PopW(w);
	}
	CFile = CF;
	bool result = RunAddUpdte1(Kind, CRold, false, nullptr, notLD);
	LockForAdd(CF, 2, false, md);
	CFile = CF;
	return result;
}

bool TestExitKey(WORD KeyCode, EdExitD* X)
{
	for (auto& key : X->Keys) {
		if (KeyCode == key.KeyCode) {
			EdBreak = key.Break;
			return true;
		}
	}
	return false;

	/*EdExKeyD* E = X->Keys;
	while (E != nullptr) {
		if (KeyCode == E->KeyCode) {
			EdBreak = E->Break;
			return true;
		}
		E = E->pChain;
	}
	return false;*/
}

void SetCompileAll()
{
	/* !!! with ChptTF^ do!!! */
	ChptTF->CompileAll = true;
	ChptTF->TimeStmp = Today() + CurrTime();
	SetUpdHandle(ChptTF->Handle);
}
