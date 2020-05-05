#include "index.h"

#include "access.h"
#include "common.h"
#include "kbdww.h"
#include "memory.h"
#include "obaseww.h"
#include "recacc.h"
#include "sort.h"


void NegateESDI()
{
	// asm  jcxz @2; @1:not es:[di].byte; inc di; loop @1; @2:
}

void TestXFExist()
{
	XFile* xf = CFile->XF;
	if ((xf != nullptr) && xf->NotValid) 
	{
		if (xf->NoCreate) CFileError(819);
		CreateIndexFile();
	}
}

longint XNRecs(KeyDPtr K)
{
	if ((CFile->Typ == 'X') && (K != nullptr))
	{
		TestXFExist();
		return CFile->XF->NRecs;
	}
	return CFile->NRecs;
}

void RecallRec(longint RecNr)
{
	TestXFExist();
	CFile->XF->NRecs++;
	KeyDPtr K = CFile->Keys;
	while (K != nullptr) { K->Insert(RecNr, false); K = K->Chain; }
	ClearDeletedFlag();
	WriteRec(RecNr);
}

void TryInsertAllIndexes(longint RecNr)
{
	void* p = nullptr;
	TestXFExist();
	MarkStore(p);
	KeyDPtr K = CFile->Keys;
	while (K != nullptr) {
		if (not K->Insert(RecNr, true)) goto label1; K = K->Chain;
	}
	CFile->XF->NRecs++;
	return;
	label1:
	ReleaseStore(p);
	KeyDPtr K1 = CFile->Keys;
	while ((K1 != nullptr) && (K1 != K)) {
		K1->Delete(RecNr); K1 = K1->Chain;
	}
	SetDeletedFlag();
	WriteRec(RecNr);
	/* !!! with CFile->XF^ do!!! */
	if (CFile->XF->FirstDupl) {
		SetMsgPar(CFile->Name);
		WrLLF10Msg(828);
		CFile->XF->FirstDupl = false;
	}
}

void DeleteAllIndexes(longint RecNr)
{
	KeyDPtr K;
	K = CFile->Keys;
	while (K != nullptr) {
		K->Delete(RecNr);
		K = K->Chain;
	}
}

void DeleteXRec(longint RecNr, bool DelT)
{
	TestXFExist();
	DeleteAllIndexes(RecNr);
	if (DelT) DelAllDifTFlds(CRecPtr, nullptr);
	SetDeletedFlag();
	WriteRec(RecNr);
	CFile->XF->NRecs--;
}

void OverwrXRec(longint RecNr, void* P2, void* P)
{
	XString x, x2; KeyDPtr K;
	CRecPtr = P2;
	if (DeletedFlag()) { CRecPtr = P; RecallRec(RecNr); return; }
	TestXFExist();
	K = CFile->Keys;
	while (K != nullptr) {
		CRecPtr = P; x.PackKF(K->KFlds); CRecPtr = P2; x2.PackKF(K->KFlds);
		if (x.S != x2.S) {
			K->Delete(RecNr); CRecPtr = P; K->Insert(RecNr, false);
		}
		K = K->Chain;
	}
	CRecPtr = P;
	WriteRec(RecNr);
}

void AddFFs(KeyDPtr K, pstring& s)
{
	WORD l = MinW(K->IndexLen + 1, 255);
	for (WORD i = s.length() + 1; i < l; i++) s[i] = 0xff;
	s[0] = char(l);
}

void CompKIFrml(KeyDPtr K, KeyInD* KI, bool AddFF)
{
	XString x; bool b; integer i;
	while (KI != nullptr) {
		b = x.PackFrml(KI->FL1, K->KFlds);
		KI->X1 = &x.S;
		if (KI->FL2 != nullptr) x.PackFrml(KI->FL2, K->KFlds);
		if (AddFF) AddFFs(K, x.S);
		KI->X2 = &x.S;
		KI = KI->Chain;
	}
}

