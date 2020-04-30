#include "runedit1.h"

#include "access.h"
#include "fileacc.h"
#include "index.h"
#include "recacc.h"

longint CRec()
{
	return BaseRec + IRec - 1;
}

longint CNRecs()
{
	longint n;
	if (EdRecVar) { return 1; }
	if (Subset) n = WK->NRecs;
	else if (HasIndex) n = VK->NRecs;
	else n = CFile->NRecs;
	if (IsNewRec) n++;
	return n;
}

longint AbsRecNr(longint N)
{
	LockMode md;
	longint result = 0;
	if (EdRecVar
#ifdef FandSQL
		|| CFile->IsSQLFile
#endif
		) {
		if (IsNewRec) result = 0; else result = 1; return result;
	}
	if (IsNewRec) {
		if ((N == CRec()) && (N == CNRecs())) { result = 0; return result; }
		if (N > CRec()) N--;
	}
	if (SubSet) N = WK->NrToRecNr(N);
	else if (HasIndex) {
		md = NewLMode(RdMode); TestXFExist();
		N = VK->NrToRecNr(N); OldLMode(md);
	}
	result = N;
	return result;
}

longint LogRecNo(longint N)
{
	LockMode md;
	longint result = 0; if ((N <= 0) || (N > CFile->NRecs)) return result;
	md = NewLMode(RdMode);
	ReadRec(N);
	if (!DeletedFlag()) {
		if (SubSet) result = WK->RecNrToNr(N);
		else if (HasIndex) { TestXFExist(); result = VK->RecNrToNr(N); }
		else result = N;
	}
	OldLMode(md);
	return result;
}

bool IsSelectedRec(WORD I)
{
	XString x; void* cr; longint n;
	auto result = false;
	if ((E->SelKey == nullptr) || (I == IRec) && IsNewRec) return result;
	n = AbsRecNr(BaseRec + I - 1);
	cr = CRecPtr;
	if ((I == IRec) && WasUpdated) CRecPtr = E->OldRecPtr;
	result = E->SelKey->RecNrToPath(x, n);
	CRecPtr = cr;
	return result;
}

