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
	unsigned short l = MinW(K->IndexLen + 1, 255);
	for (unsigned short i = s.length() + 1; i <= l; i++) s[i] = 0xff;
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
		KI = (KeyInD*)KI->pChain;
	}
}

XScan::~XScan()
{
	delete page_;
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
			page_ = new XPage();
			Kind = 1;
			if (aKIRoot != nullptr) Kind = 2;
		}
	}
}

void XScan::Reset(FrmlElem* ABool, bool SQLFilter)
{
	KeyInD* k = nullptr; int n = 0; XString xx; bool b = false;
	CFile = FD;
	Bool = ABool;
	if (SQLFilter) {
		if (CFile->IsSQLFile) hasSQLFilter = true;
		else Bool = nullptr;
	}
	switch (Kind) {
	case 0: NRecs = CFile->FF->NRecs; break;
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
			// vyhleda 1. zaznam odpovidajici klici 
			Key->FindNr(k->X1, k->XNrBeg);
			// vyhleda posledni zaznam odpovidajici klici
			b = Key->FindNr(k->X2, n);
			k->N = 0;
			if (n >= k->XNrBeg) k->N = n - k->XNrBeg + b;
			NRecs += k->N;
			k = (KeyInD*)k->pChain;
		}
		break;
	}
#ifdef FandSQL
	case 4: { CompKIFrml(Key, KIRoot, false); New(SQLStreamPtr(Strm), Init); IRec = 1; break; }
#endif
	}
	SeekRec(0);
}

void XScan::ResetSort(KeyFldD* aSK, FrmlElem* BoolZ, LockMode OldMd, bool SQLFilter)
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
	if (CFile->FF->NotCached()) {
		switch (Kind) {
		case 0: { m = NoCrMode; if (CFile->FF->XF != nullptr) m = NoExclMode; break; }
		case 1: { m = OldMd; if (Key->InWork) m = NoExclMode; break; }
		default: return;
		}
		m = LockMode(MaxW(m, OldMd));
		if (m != OldMd) CFile->ChangeLockMode(m, 0, true);
	}
}

void XScan::SubstWIndex(XWKey* WK)
{
	Key = WK;
	if (Kind != 3) Kind = 1;
	if (page_ == nullptr) {
		page_ = new XPage();
	}
	NRecs = Key->NRecs();
	Bool = nullptr;
	SeekRec(0);
	TempWX = true;
}

void XScan::ResetOwner(XString* XX, FrmlElem* aBool)
{
	int n;
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
		Key->FindNr(XX->S, KIRoot->XNrBeg);
		AddFFs(Key, XX->S);
		b = Key->FindNr(XX->S, n);
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
		KF1 = (KeyFldD*)KF1->pChain;
		KF2 = (KeyFldD*)KF2->pChain;
	}
	if (KF2 != nullptr) return false;
	return true;
}

void XScan::ResetOwnerIndex(LinkD* LD, LocVar* LV, FrmlElem* aBool)
{
	CFile = FD;
	TestXFExist();
	Bool = aBool;
	OwnerLV = LV;
	Kind = 2;
	if (!EquKFlds(((XWKey*)LV->RecPtr)->KFlds, LD->ToKey->KFlds)) {
		RunError(1181);
	}
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

void XScan::SeekRec(int I)
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
			while (I >= k->N) { I -= k->N; k = (KeyInD*)k->pChain; }
			KI = k;
			SeekOnKI(I);
			break;
		}
		}
	}
}

void XScan::SeekOnKI(int I)
{
	NOfKI = KI->N - I;
	Key->NrToPath(KI->XNrBeg + I);
	/* !!! with XPath[XPathN] do!!! */
	SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
}

void XScan::SeekOnPage(int pageNr, unsigned short i)
{
	Key->GetXFile()->RdPage(page_, pageNr);
	items_on_page_ = page_->NItems - i + 1;
	if (Kind == 2) {
		if (items_on_page_ > NOfKI) {
			items_on_page_ = NOfKI;
		}
		NOfKI -= items_on_page_;
	}
	//X = P->GetItem(I);
	_item = i;
}

void XScan::NextIntvl()
{
	XString xx;
	bool b = false;
	int n = 0, nBeg = 0;

	if (OwnerLV != nullptr) {
		XWKey* k = (XWKey*)OwnerLV->RecPtr; // TODO: bude toto fungovat?
		while (iOKey < k->NRecs()) {
			iOKey++;
			CFile = OwnerLV->FD;
			xx.S = k->NrToStr(iOKey);
			CFile = FD;
			Key->FindNr(xx.S, nBeg);
			AddFFs(Key, xx.S);
			b = Key->FindNr(xx.S, n);
			n = n - nBeg + b;
			if (n > 0) {
				NOfKI = n;
				Key->NrToPath(nBeg);
				SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
				return;
			}
		}
		NRecs = IRec; /*EOF*/
	}
	else {
		do {
			KI = (KeyInD*)KI->pChain;
		}
		while (!((KI == nullptr) || (KI->N > 0)));

		if (KI != nullptr) {
			SeekOnKI(0);
		}
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
			RecNr = page_->GetItem(_item)->GetN();
			items_on_page_--;
			if (items_on_page_ > 0) {
				_item++;
			}
			else if ((Kind == 2) && (NOfKI == 0)) NextIntvl();
			else if (page_->GreaterPage > 0) SeekOnPage(page_->GreaterPage, 1);
		label2:
			CFile->ReadRec(RecNr, CRecPtr);
			if (CFile->DeletedFlag(CRecPtr)) goto label1;
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
			Move(Strm, CRecPtr, CFile->FF->RecLen + 1);
			break;
		}
		}
	}
}
