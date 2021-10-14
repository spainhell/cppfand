#include "XScan.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/runfrml.h"
#include "sort.h"

void AddFFs(XKey* K, pstring& s)
{
	WORD l = MinW(K->IndexLen + 1, 255);
	for (WORD i = s.length() + 1; i <= l; i++) s[i] = 0xff;
	s[0] = (char)l;
}

/// asi vytvori XStringy pro zacatek a konec (rozsah) vyhledavani
/// pokud se hleda interval v klici
void CompKIFrml(XKey* K, KeyInD* KI, bool AddFF)
{
	XString x;
	while (KI != nullptr) {
		bool b = x.PackFrml(KI->FL1, K->KFlds);
		KI->X1 = x.S;
		if (KI->FL2 != nullptr) x.PackFrml(KI->FL2, K->KFlds);
		if (AddFF) AddFFs(K, x.S);
		KI->X2 = x.S;
		KI = (KeyInD*)KI->Chain;
	}
}

XScan::XScan(FileD* aFD, XKey* aKey, KeyInD* aKIRoot, bool aWithT)
{
	FD = aFD; Key = aKey; KIRoot = aKIRoot; withT = aWithT;
#ifdef FandSQL
	if (aFD->IsSQLFile) {
		if ((aKey != nullptr) && aKey->InWork) { P = (XPage*)GetStore(XPageSize); Kind = 3; }
		else Kind = 4;
	}
	else
#endif
	{
		if (aKey != nullptr) {
			//P = (XPage*)GetStore(XPageSize);
			P = new XPage();
			Kind = 1;
			if (aKIRoot != nullptr) Kind = 2;
		}
	}
}

void XScan::Reset(FrmlElem* ABool, bool SQLFilter)
{
	KeyInD* k = nullptr; longint n = 0; XString xx; bool b = false;
	CFile = FD;
	Bool = ABool;
	if (SQLFilter) {
		if (CFile->IsSQLFile) hasSQLFilter = true;
		else Bool = nullptr;
	}
	switch (Kind) {
	case 0: NRecs = CFile->NRecs; break;
	case 1:
	case 3: {
		if (Key != nullptr) {
			if (!Key->InWork) TestXFExist();
			NRecs = Key->NRecs();
		}
		break;
	}
	case 2: {
		if (!Key->InWork) TestXFExist();
		CompKIFrml(Key, KIRoot, true);
		NRecs = 0;
		k = KIRoot;
		while (k != nullptr) {
			XString a1;
			XString b2;
			a1.S = k->X1;
			b2.S = k->X2;
			// vyhleda 1. zaznam odpovidajici klici 
			Key->FindNr(a1, k->XNrBeg);
			// vyhleda posledni zaznam odpovidajici klici
			b = Key->FindNr(b2, n);
			k->N = 0;
			if (n >= k->XNrBeg) k->N = n - k->XNrBeg + b;
			NRecs += k->N;
			k = (KeyInD*)k->Chain;
		}
		break;
	}
#ifdef FandSQL
	case 4: { CompKIFrml(Key, KIRoot, false); New(SQLStreamPtr(Strm), Init); IRec = 1; break; }
#endif
	}
	SeekRec(0);
}

void XScan::ResetSort(KeyFldD* aSK, FrmlPtr& BoolZ, LockMode OldMd, bool SQLFilter)
{
	LockMode m;
	if (Kind == 4) {
		SK = aSK;
		if (SQLFilter)
		{
			Reset(BoolZ, true);
			BoolZ = nullptr;
		}
		else Reset(nullptr, false);
		return;
	}
	if (aSK != nullptr) {
		Reset(BoolZ, false);
		ScanSubstWIndex(this, aSK, 'S');
		BoolZ = nullptr;
	}
	else Reset(nullptr, false);
	/* !!! with CFile^ do!!! */
	if (CFile->NotCached()) {
		switch (Kind) {
		case 0: { m = NoCrMode; if (CFile->XF != nullptr) m = NoExclMode; break; }
		case 1: { m = OldMd; if (Key->InWork) m = NoExclMode; break; }
		default: return;
		}
		m = LockMode(MaxW(m, OldMd));
		if (m != OldMd) ChangeLMode(m, 0, true);
	}
}

void XScan::SubstWIndex(WKeyDPtr WK)
{
	Key = WK;
	if (Kind != 3) Kind = 1;
	if (P == nullptr) P = new XPage(); // (XPage*)GetStore(XPageSize);
	NRecs = Key->NRecs();
	Bool = nullptr;
	SeekRec(0);
	TempWX = true;
}

void XScan::ResetOwner(XString* XX, FrmlPtr aBool)
{
	longint n;
	bool b;
	CFile = FD;
	Bool = aBool;
#ifdef FandSQL
	if (Kind = 4) {           /* !on .SQL with Workindex */
		KIRoot = GetZStore(sizeof(KIRoot^));
		KIRoot->X1 = StoreStr(XX->S); KIRoot->X2 = StoreStr(XX->S);
		New(SQLStreamPtr(Strm), Init); IRec = 1
	}
	else
#endif
	{
		TestXFExist();
		KIRoot = new KeyInD(); // (KeyInD*)GetZStore(sizeof(*KIRoot));
		Key->FindNr(*XX, KIRoot->XNrBeg);
		AddFFs(Key, XX->S);
		b = Key->FindNr(*XX, n);
		NRecs = n - KIRoot->XNrBeg + b;
		KIRoot->N = NRecs; Kind = 2;
	}
	SeekRec(0);
}

bool EquKFlds(KeyFldD* KF1, KeyFldD* KF2)
{
	bool result = false;
	while (KF1 != nullptr) {
		if ((KF2 == nullptr) || (KF1->CompLex != KF2->CompLex) || (KF1->Descend != KF2->Descend)
			|| (KF1->FldD->Name != KF2->FldD->Name)) return result;
		KF1 = (KeyFldD*)KF1->Chain;
		KF2 = (KeyFldD*)KF2->Chain;
	}
	if (KF2 != nullptr) return false;
	return true;
}

void XScan::ResetOwnerIndex(LinkDPtr LD, LocVar* LV, FrmlPtr aBool)
{
	WKeyDPtr k;
	CFile = FD; TestXFExist(); Bool = aBool; OwnerLV = LV; Kind = 2;
	if (!EquKFlds(WKeyDPtr(LV->RecPtr)->KFlds, LD->ToKey->KFlds)) RunError(1181);
	SeekRec(0);
}

#ifdef FandSQL
void XScan::ResetSQLTxt(FrmlPtr Z)
{
	LongStrPtr s;
	New(SQLStreamPtr(Strm), Init); s = RunLongStr(Z);
	SQLStreamPtr(Strm)->InpResetTxt(s); ReleaseStore(s);
	eof = false;
}
#endif

void XScan::ResetLV(void* aRP)
{
	Strm = aRP; Kind = 5; NRecs = 1;
}

void XScan::Close()
{
	CFile = FD;
#ifdef FandSQL
	if (Kind = 4) /* !!! with SQLStreamPtr(Strm)^ do!!! */ { InpClose; Done; }
#endif
	if (TempWX) WKeyDPtr(Key)->Close();
}

void XScan::SeekRec(longint I)
{
	KeyInD* k = nullptr;
	FrmlElem* z = nullptr;
	CFile = FD;

#ifdef FandSQL
	if (Kind == 4) {
		if (I != IRec) /* !!! with SQLStreamPtr(Strm)^ do!!! */
		{
			if (NotFrst) InpClose; NotFrst = true;
			if (hasSQLFilter) z = Bool else z = nullptr;
			InpReset(Key, SK, KIRoot, z, withT);
			EOF = AtEnd; IRec = 0; NRecs = 0x20000000;
		}
		return;
	}
#endif
	if ((Kind == 2) && (OwnerLV != nullptr)) {
		IRec = 0;
		NRecs = 0x20000000;
		iOKey = 0;
		NextIntvl();
		eof = I >= NRecs;
		return;
	}
	IRec = I;
	eof = I >= NRecs;
	if (!eof) {
		switch (Kind) {
		case 1:
		case 3: {
			Key->NrToPath(I + 1); /* !!! with XPath[XPathN] do!!! */
			SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
			break;
		}
		case 2: {
			k = KIRoot;
			while (I >= k->N) { I -= k->N; k = (KeyInD*)k->Chain; }
			KI = k;
			SeekOnKI(I);
			break;
		}
		}
	}
}

void XScan::SeekOnKI(longint I)
{
	NOfKI = KI->N - I;
	Key->NrToPath(KI->XNrBeg + I);
	/* !!! with XPath[XPathN] do!!! */
	SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
}

void XScan::SeekOnPage(longint Page, WORD I)
{
	Key->XF()->RdPage(P, Page);
	NOnPg = P->NItems - I + 1;
	if (Kind == 2) {
		if (NOnPg > NOfKI) NOnPg = NOfKI;
		NOfKI -= NOnPg;
	}
	//X = P->XI(I);
	_item = I;
}

void XScan::NextIntvl()
{
	XString xx;
	bool b = false;
	longint n = 0, nBeg = 0;

	if (OwnerLV != nullptr) {
		XWKey* k = (XWKey*)OwnerLV->RecPtr; // bude toto fungovat?
		while (iOKey < k->NRecs()) {
			iOKey++;
			CFile = OwnerLV->FD;
			xx.S = k->NrToStr(iOKey);
			CFile = FD;
			Key->FindNr(xx, nBeg);
			AddFFs(Key, xx.S);
			b = Key->FindNr(xx, n);
			n = n - nBeg + b;
			if (n > 0) {
				NOfKI = n;
				Key->NrToPath(nBeg); /* !!! with XPath[XPathN] do!!! */
				SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
				return;
			};
		}
		NRecs = IRec; /*EOF*/
	}
	else {
		do { KI = (KeyInD*)KI->Chain; } while (!((KI == nullptr) || (KI->N > 0)));
		if (KI != nullptr) SeekOnKI(0);
	}
}

void XScan::GetRec()
{
	XString xx;
	CFile = FD;
	size_t item = 0;
#ifdef FandSQL
	if (Kind == 4) {
		repeat EOF = !SQLStreamPtr(Strm)->GetRec
			until EOF || hasSQLFilter || RunBool(Bool);
		inc(IRec); return;
	}
#endif
label1:
	eof = IRec >= NRecs;
	if (!eof) {
		IRec++;
		switch (Kind) {
		case 0: { RecNr = IRec; goto label2; break; }
		case 1:
		case 2: {
			RecNr = P->XI(_item)->GetN();
			NOnPg--;
			if (NOnPg > 0) {
				_item++;
			}
			else if ((Kind == 2) && (NOfKI == 0)) NextIntvl();
			else if (P->GreaterPage > 0) SeekOnPage(P->GreaterPage, 1);
		label2:
			ReadRec(CFile, RecNr, CRecPtr);
			if (DeletedFlag()) goto label1;
		label3:
			if (!RunBool(Bool)) goto label1;
			break;
		}
#ifdef FandSQL
		case 3: {
			NOnPg--;
			xx.S = P->StrI(P->NItems - NOnPg);
			if ((NOnPg == 0) && (P->GreaterPage > 0)) SeekOnPage(P->GreaterPage, 1);
			if (!Strm1->SelectXRec(Key, @xx, _equ, withT)) goto label1;
			goto label3;
			break;
		}
#endif
		case 5:
		{
			Move(Strm, CRecPtr, CFile->RecLen + 1);
			break;
		}
		}
	}
}
