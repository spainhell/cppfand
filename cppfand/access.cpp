#include "access.h"


#include "fileacc.h"
#include "kbdww.h"
#include "memory.h"


void RunErrorM(LockMode Md, WORD N)
{
	OldLMode(Md);
	RunError(N);
}

pstring* FieldDMask(FieldDPtr F)
{
	return nullptr;
}

void* GetRecSpace()
{
	return GetZStore(CFile->RecLen + 2);
}

void* GetRecSpace2()
{
	return GetZStore2(CFile->RecLen + 2);
}

bool EquKFlds(KeyFldDPtr KF1, KeyFldDPtr KF2)
{
	bool result = false;
	while (KF1 != nullptr) {
		if ((KF2 == nullptr) || (KF1->CompLex != KF2->CompLex) || (KF1->Descend != KF2->Descend)
			|| (KF1->FldD->Name != KF2->FldD->Name)) return result;
		KF1 = KF1->Chain; KF2 = KF2->Chain;
	}
	if (KF2 != nullptr) return false;
	return true;
}

/// nedìlá nic, pùvodnì dìlal XOR 0xAA;
void Code(void* A, WORD L)
{
	return;
}

/// nedìlá nic
void XDecode(LongStrPtr S)
{
	return;
}

void CodingLongStr(LongStrPtr S)
{
	if (CFile->TF->LicenseNr == 0) Code(S->A, S->LL);
	else XDecode(S);
}

void DirMinusBackslash(pstring& D)
{
	if ((D.length() > 3) && (D[D.length() - 1] == '\\')) D[0]--;
}

longint StoreInTWork(LongStrPtr S)
{
	return TWork.Store(S);
}

LongStrPtr ReadDelInTWork(longint Pos)
{
	auto result = TWork.Read(1, Pos);
	TWork.Delete(Pos);
	return result;
}

void ForAllFDs(void(*procedure)())
{
	RdbDPtr R; FileDPtr cf;
	cf = CFile; R = CRdb;
	while (R != nullptr) {
		CFile = R->FD;
		while (CFile != nullptr) { procedure(); CFile = CFile->Chain; };
		R = R->ChainBack;
	}
	CFile = cf;
}

bool IsActiveRdb(FileDPtr FD)
{
	RdbDPtr R;
	R = CRdb; while (R != nullptr) {
		if (FD == R->FD) return true;
		R = R->ChainBack;
	}
	return false;
}

void ResetCompilePars()
{
	RdFldNameFrml = RdFldNameFrmlF();
	RdFunction = nullptr;
	ChainSumEl = nullptr;
	FileVarsAllowed = true;
	FDLocVarAllowed = false;
	IdxLocVarAllowed = false;
	PrevCompInp = nullptr;
}


