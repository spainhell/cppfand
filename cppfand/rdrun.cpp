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
	if (!Back && !RunBool(AD->Chk->Bool))
	{
		SetMsgPar(RunShortStr(AD->Chk->TxtZ));
		WrLLF10Msg(110);
		result = false;
	}
	return result;
}

bool RunAddUpdte1(char Kind, void* CRold, bool Back, AddD* StopAD, LinkD* notLD)
{
	int N2, N2old;
	char Kind2, Kind2old;
	FileD* CF2 = nullptr;
	void* CR2 = nullptr;
	void* CR2old = nullptr; void* p = nullptr;
	double R, Rold;
	bool b;
	auto result = true;
	FileD* CF = CFile;
	void* CR = CRecPtr;
	MarkStore(p);
	AddD* ADback = nullptr;

	for (AddD* add : CFile->Add) {
		if (add == StopAD) {
			ReleaseStore(p);
			return result;
		}
		if ((notLD != nullptr) && (add->LD == notLD)) goto label1;
		if (add->Assign) {
			if (Assign(add)) goto label1;
			else goto fail;
		}

		R = RunReal(CFile, add->Frml, CRecPtr);
		if (Kind == '-') R = -R;
		Rold = 0;
		if (Kind == 'd') {
			CRecPtr = CRold;
			Rold = RunReal(CFile, add->Frml, CRecPtr);
		}
		ADback = add;
		CF2 = add->File2;
		N2 = 0; N2old = 0;
		if (R != 0.0) {
			CRecPtr = CR;
			if (!Link(add, N2, Kind2)) goto fail;
			CR2 = CRecPtr;
		}
		if (Rold != 0.0) {
			CFile = CF;
			CRecPtr = CRold;
			if (!Link(add, N2old, Kind2old)) goto fail;
			CR2old = CRecPtr;
			if (N2old == N2)
			{
				R = R - Rold;
				if (R == 0.0) goto label1;
				N2old = 0;
			}
		}
		if ((N2 == 0) && (N2old == 0)) goto label1;
		CFile = CF2;
		if (N2old != 0) {
			if (!Add(add, CR2old, -Rold, Back)) goto fail;
		}
		if (N2 != 0) {
			if (!Add(add, CR2, R, Back)) goto fail;
		}
		if ((N2old != 0) && !TransAdd(add, CF, CR, CR2old, N2old, Kind2old, false)) goto fail;
		if ((N2 != 0) && !TransAdd(add, CF, CR, CR2, N2, Kind2, false)) {
			if (N2old != 0) b = TransAdd(add, CF, CR, CR2old, N2old, Kind2old, true);
			goto fail;
		}
		if (N2old != 0) WrUpdRec(add, CF, CR, CR2old, N2old);
		if (N2 != 0) WrUpdRec(add, CF, CR, CR2, N2);
	label1:
		ReleaseStore(p);
		CFile = CF;
		CRecPtr = CR;
	}
	return result;
fail:
	ReleaseStore(p);
	CFile = CF;
	CRecPtr = CR;
	result = false;
	if (ADback != nullptr) {
		b = RunAddUpdte1(Kind, CRold, true, ADback, notLD);  /* backtracking */
	}
	return result;
}

void CrIndRec()
{
	CFile->CreateRec(CFile->FF->NRecs + 1, CRecPtr);
	CFile->RecallRec(CFile->FF->NRecs, CRecPtr);
}

bool Link(AddD* AD, int& N, char& Kind2)
{
	void* CR;
	auto result = true;
	LinkD* LD = AD->LD;
	Kind2 = 'd';

	if (LD != nullptr) {
		if (LinkUpw(LD, N, false)) return result;
		SetMsgPar(LD->RoleName);
	}
	else {
		if (!LinkLastRec(AD->File2, N, false)
#ifdef FandSQL
			&& !CFile->IsSQLFile
#endif
			) {
			CFile->IncNRecs(1);
			CFile->WriteRec(1, CRecPtr);
		}
		return result;
	}
	Kind2 = '+';
	if ((AD->Create == 2) || (AD->Create == 1) && PromptYN(132)) {
#ifdef FandSQL
		if (CFile->IsSQLFile) Strm1->InsertRec(false, true) else
#endif
		{
			CFile->ClearDeletedFlag(CRecPtr);
			if ((LD != nullptr) && (CFile->FF->file_type == FileType::INDEX)) {
				CrIndRec();
				N = CFile->FF->NRecs;
			}
			else {
				CFile->CreateRec(N, CRecPtr);
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
	ReleaseStore(CRold);
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
	if (!RunBool(AD->Bool)) return true;
	FieldDescr* F = AD->Field;
	FrmlElem* Z = AD->Frml;

	switch (F->frml_type) {
	case 'R': {
		R = RunReal(CFile, Z, CRecPtr);
		break;
	}
	case 'S': {
		if (F->field_type == FieldType::TEXT) S = RunStdStr(CFile, Z, CRecPtr);
		else ss = RunShortStr(Z);
		break;
	}
	default: {
		B = RunBool(Z);
		break;
	}
	}

	if (!Link(AD, N2, Kind2)) { return false; }
	switch (F->frml_type) {
	case 'R': {
		CFile->saveR(F, R, CRecPtr);
		break;
	}
	case 'S': {
		if (F->field_type == FieldType::TEXT) CFile->saveS(F, S, CRecPtr);
		else CFile->saveS(F, ss, CRecPtr);
		break;
	}
	default: {
		CFile->saveB(F, B, CRecPtr);
		break;
	}
	}
	CFile->WriteRec(N2, CRecPtr);
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
