#include "XKey.h"

#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "obaseww.h"
#include "XFile.h"
#include "XPage.h"
#include "../Logging/Logging.h"
#include "../pascal/asm.h"

XKey::XKey()
{
}

XKey::XKey(const XKey& orig, bool copyFlds)
{
	if (orig.Chain != nullptr) Chain = new XKey(*orig.Chain);
	if (orig.KFlds != nullptr) KFlds = new KeyFldD(*orig.KFlds, copyFlds);
	Intervaltest = orig.Intervaltest;
	Duplic = orig.Duplic;
	InWork = orig.InWork;
	IndexRoot = orig.IndexRoot;
	IndexLen = orig.IndexLen;
	NR = orig.NR;
	Alias = orig.Alias;
}

XKey::XKey(BYTE* inputStr)
{
	size_t index = 0;
	Chain = reinterpret_cast<XKey*>(*(unsigned int*)&inputStr[index]); index += 4;
	KFlds = reinterpret_cast<KeyFldD*>(*(unsigned int*)&inputStr[index]); index += 4;
	Intervaltest = *(bool*)&inputStr[index]; index++;
	Duplic = *(bool*)&inputStr[index]; index++;
	InWork = *(bool*)&inputStr[index]; index++;
	IndexRoot = *(unsigned short*)&inputStr[index]; index += 2;
	IndexLen = *(unsigned char*)&inputStr[index]; index++;
	NR = *(longint*)&inputStr[index]; index += 4;
	Alias = reinterpret_cast<std::string*>(*(unsigned int*)&inputStr[index + 1]); index += 4;

	//unsigned int DisplOrFrml = *(unsigned int*)&inputStr[index]; index += 4;
	//if (DisplOrFrml > MaxTxtCols) {
	//	// jedna se o ukazatel
	//	Frml = reinterpret_cast<FrmlElem*>(DisplOrFrml);
	//}
	//else {
	//	// jedna se o delku
	//	Displ = DisplOrFrml;
	//}
	//Name[0] = inputStr[index]; index++;
	//memcpy(&Name[1], &inputStr[index], Name[0]); index += Name[0];
}

XWFile* XKey::XF()
{
	if (InWork) return &XWork;
	return CFile->XF;
}

longint XKey::NRecs()
{
	if (InWork) return NR;
	return CFile->XF->NRecs;
}

bool XKey::Search(XString& XX, bool AfterEqu, longint& RecNr)
{
	bool searchResult = false;
	XPage* p = nullptr;
	WORD iItem = 0;
	XItem* x = nullptr;
	size_t iItemIndex = 0;
	char result = '\0';
	p = new XPage(); // (XPage*)GetStore(XPageSize);
	XPathN = 1;
	longint page = IndexRoot;
	AfterEqu = AfterEqu && Duplic;
label1:
	XPath[XPathN].Page = page;
	XF()->RdPage(p, page); // je nactena asi cela stranka indexu

	WORD o = p->Off();
	WORD nItems = p->NItems;
	if (nItems == 0) {
		RecNr = CFile->NRecs + 1;
		XPath[1].I = 1;
		goto label2;
	}

	// * PUVODNI ASM
	result = XKeySearch2(p->A, &XX.S[0], iItem, iItemIndex, nItems, o, AfterEqu);
	// * KONEC PUVODNIHO ASM

	XPath[XPathN].I = iItem;
	x = new XItem(&p->A[iItemIndex], p->IsLeaf);
	if (p->IsLeaf) {
		if (iItem > nItems) RecNr = CFile->NRecs + 1;
		else RecNr = x->GetN();
		if (result == _equ)
			if
#ifdef FandSQL
				!CFile->IsSQLFile&&
#endif
				(((RecNr == 0) || (RecNr > CFile->NRecs))) XF()->Err(833);
			else searchResult = true;
		else
			label2:
		searchResult = false;
		ReleaseStore(p);
		delete x;
		return searchResult;
	}
	if (iItem > nItems) page = p->GreaterPage;
	else page = *(x->DownPage);
	XPathN++;
	delete x;
	goto label1;
}

bool XKey::Search(std::string const X, bool AfterEqu, longint& RecNr)
{
	bool searchResult = false;
	XPage* p = nullptr;
	WORD iItem = 0;
	XItem* x = nullptr;
	size_t iItemIndex = 0;
	char result = '\0';
	p = new XPage(); // (XPage*)GetStore(XPageSize);
	XPathN = 1;
	longint page = IndexRoot;
	AfterEqu = AfterEqu && Duplic;
label1:
	XPath[XPathN].Page = page;
	XF()->RdPage(p, page); // je nactena asi cela stranka indexu

	WORD o = p->Off();
	WORD nItems = p->NItems;
	if (nItems == 0) {
		RecNr = CFile->NRecs + 1;
		XPath[1].I = 1;
		goto label2;
	}

	// * PUVODNI ASM
	result = XKeySearch2(p->A, X, iItem, iItemIndex, nItems, o, AfterEqu);
	// * KONEC PUVODNIHO ASM

	XPath[XPathN].I = iItem;
	x = new XItem(&p->A[iItemIndex], p->IsLeaf);
	if (p->IsLeaf) {
		if (iItem > nItems) RecNr = CFile->NRecs + 1;
		else RecNr = x->GetN();
		if (result == _equ)
			if
#ifdef FandSQL
				!CFile->IsSQLFile&&
#endif
				(((RecNr == 0) || (RecNr > CFile->NRecs))) XF()->Err(833);
			else searchResult = true;
		else
			label2:
		searchResult = false;
		ReleaseStore(p);
		delete x;
		return searchResult;
	}
	if (iItem > nItems) page = p->GreaterPage;
	else page = *(x->DownPage);
	XPathN++;
	delete x;
	goto label1;
}

bool XKey::SearchIntvl(XString& XX, bool AfterEqu, longint& RecNr)
{
	return Search(XX, AfterEqu, RecNr) || Intervaltest && (RecNr <= CFile->NRecs);
}

longint XKey::PathToNr()
{
	longint n = 0;
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	for (WORD j = 1; j <= XPathN - 1; j++)
	{
		XF()->RdPage(p, XPath[j].Page);
		XItem* x = new XItem(p->A, p->IsLeaf); // XItemPtr(p->A);
		for (WORD i = 1; i <= XPath[j].I - 1; i++) {
			n += x->GetN();
			auto prevX = x;
			x = x->Next(oNotLeaf, p->IsLeaf);
			delete prevX; prevX = 0;
		}
		delete x; x = nullptr;
	}
	n += XPath[XPathN].I;
	if (n > NRecs() + 1) {
		XF()->Err(834);
	}
	ReleaseStore(p);
	return n;
}

void XKey::NrToPath(longint I)
{
	auto log = Logging::getInstance();
	log->log(loglevel::DEBUG, "XKey::NrToPath(%i)", I);

	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	longint page = IndexRoot;
	XPathN = 0;
label1:
	XF()->RdPage(p, page);
	XPathN++;
	XPath[XPathN].Page = page;
	if (p->IsLeaf) {
		if (I > p->NItems + 1) XF()->Err(837);
		XPath[XPathN].I = I;
		ReleaseStore(p);
		return;
	}
	XItem* x = new XItem(p->A, p->IsLeaf);
	for (WORD j = 1; j <= p->NItems; j++) {
		if (I <= x->GetN()) {
			XPath[XPathN].I = j;
			page = *(x->DownPage);
			delete x; x = nullptr;
			goto label1;
		}
		I -= x->GetN();
		auto prevX = x;
		x = x->Next(oNotLeaf, p->IsLeaf);
		delete prevX; prevX = nullptr;
	}
	delete x; x = nullptr;
	XPath[XPathN].I = p->NItems + 1;
	page = p->GreaterPage;
	goto label1;
}

longint XKey::PathToRecNr()
{
	/* !!! with XPath[XPathN] do!!! */
	auto X = XPath[XPathN];
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	XF()->RdPage(p, X.Page);
	auto pxi = p->XI(X.I, p->IsLeaf);
	longint recnr = pxi->GetN();
	longint result = recnr;
	if ((recnr == 0) || (recnr > CFile->NRecs)) {
		XF()->Err(835);
	}
	ReleaseStore(p);
	return result;
}

bool XKey::RecNrToPath(XString& XX, longint RecNr)
{
	bool result = false;
	XItem* x = nullptr; longint n = 0;
	XX.PackKF(KFlds);
	Search(XX, false, n);
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	/* !!! with XPath[XPathN] do!!! */
	{
		auto X = XPath[XPathN];
	label1:
		XF()->RdPage(p, X.Page);
		x = p->XI(X.I, p->IsLeaf);
		if (!(p->GetKey(X.I) == XX.S)) goto label3;
	label2:
		if (x->GetN() == RecNr) { result = true; goto label3; }
		X.I++;
		if (X.I > p->NItems) {
			if (IncPath(XPathN - 1, X.Page)) { X.I = 1; goto label1; }
		}
		else {
			x = x->Next(oLeaf, p->IsLeaf);
			if (x->GetL(oLeaf) != 0) goto label3;
			goto label2;
		}
	}
label3:
	ReleaseStore(p);
	return result;
}

bool XKey::IncPath(WORD J, longint& Pg)
{
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	bool result = false;
	auto X = XPath[J];
	if (J == 0) { goto label2; } /* !!! with XPath[J] do!!! */
	{
	label1:
		XF()->RdPage(p, X.Page);
		if (X.I > p->NItems)
			if (IncPath(J - 1, X.Page)) { X.I = 0; goto label1; }
			else goto label2;
		X.I++;
		if (X.I > p->NItems)
			if (p->GreaterPage == 0) {
				X.I = 0; if (IncPath(J - 1, X.Page)) goto label1; goto label2;
			}
			else Pg = p->GreaterPage;
		else Pg = *(p->XI(X.I, p->IsLeaf)->DownPage);
	}
	result = true;
label2:
	ReleaseStore(p);
	return result;
}

longint XKey::NrToRecNr(longint I)
{
	auto log = Logging::getInstance();
	log->log(loglevel::DEBUG, "XKey::NrToRecNr(%i)", I);
	NrToPath(I);
	return PathToRecNr();
}

pstring XKey::NrToStr(longint I)
{
	pstring result;
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	NrToPath(I);
	/* !!! with XPath[XPathN] do!!! */
	XF()->RdPage(p, XPath[XPathN].Page);
	result = p->GetKey(I);
	ReleaseStore(p);
	return result;
}

longint XKey::RecNrToNr(longint RecNr)
{
	XString x;
	if (RecNrToPath(x, RecNr)) return PathToNr();
	else return 0;
}

bool XKey::FindNr(XString& X, longint& IndexNr)
{
	longint n;
	auto result = Search(X, false, n);
	IndexNr = PathToNr();
	return result;
}

bool XKey::FindNr(std::string const X, longint& IndexNr)
{
	longint n;
	auto result = Search(X, false, n);
	IndexNr = PathToNr();
	return result;
}

void XKey::InsertOnPath(XString& XX, longint RecNr)
{

	WORD i = 0, j = 0;
	longint page = 0, page1 = 0, uppage = 0, downpage = 0;
	XItem* x = nullptr;
	longint n = 0, upsum = 0;

	XPage* p = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	XPage* p1 = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	XPage* upp = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	for (j = XPathN; j >= 1; j--) {
		page = XPath[j].Page;
		XF()->RdPage(p, page);
		i = XPath[j].I;
		if (p->IsLeaf) {
			InsertLeafItem(XX, p, upp, page, i, RecNr, uppage);
			// x->PutN(RecNr); // zapisuje se primo o radek vys!
		}
		else {
			if (i <= p->NItems) {
				x = p->XI(i, p->IsLeaf);
				n = x->GetN() + 1;
				if (uppage != 0) n -= upsum;
				x->PutN(n);
			}
			if (uppage != 0) {
				downpage = uppage;
				InsertItem(XX, p, upp, page, i, &x, uppage);
				*(x->DownPage) = downpage;
				x->PutN(upsum);
			}
		}
		XF()->WrPage(p, page);
		if (uppage != 0) {
			XF()->WrPage(upp, uppage);
			upsum = upp->SumN();
			if (upp->IsLeaf) ChainPrevLeaf(p1, uppage);
		}
	}
	if (uppage != 0) {
		page1 = XF()->NewPage(p1);
		p1->GreaterPage = page1;
		p1->InsDownIndex(1, uppage, upp);
		XF()->WrPage(p, page1);
		XF()->WrPage(p1, page);
		if (upp->IsLeaf) {
			upp->GreaterPage = page1;
			XF()->WrPage(upp, uppage);
		}
	}
	ReleaseStore(p);
}

void XKey::InsertItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, XItem** X, longint& UpPage)
{
	size_t Xlen = 0;
	P->InsertNonLeaf(I, &XX.S, X, Xlen);
	UpPage = 0;
	if (P->Overflow()) {
		printf("XKey::InsertItem overflow");
		/*UpPage = XF()->NewPage(UpP);
		P->SplitPage(UpP, Page);
		if (I <= UpP->NItems) *X = UpP->XI(I, P->IsLeaf);
		else *X = P->XI(I - UpP->NItems, P->IsLeaf);
		XX.S = UpP->StrI(UpP->NItems);*/
	}
}

void XKey::InsertLeafItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, int RecNr, longint& UpPage)
{
	P->InsertLeaf(RecNr, I, XX.S);
	UpPage = 0;
	if (P->ItemsSize() > sizeof(P->A)) {
		// printf("XKey::InsertLeafItem() PREKROCENA VELIKOST STRANKY");
		UpPage = XF()->NewPage(UpP);
		P->SplitPage(UpP, Page);
		// TODO: NUTNO DORESIT, CO SE TADY DEJE
		// *X byl puvodne parametr metody
		// if (I <= UpP->NItems) *X = UpP->XI(I, P->IsLeaf);
		// else *X = P->XI(I - UpP->NItems, P->IsLeaf);
		P->Serialize();
		UpP->Serialize();
		XX.S = UpP->GetKey(UpP->NItems);
	}
	else {
		// pregenerujeme data z vektoru do P->A
		P->Serialize();
	}
#if _DEBUG
	std::vector<pstring> vP;
	for(size_t i = 1; i <= P->NItems; i++) {
		vP.push_back(P->GetKey(i));
	}
	std::vector<pstring> vUpP;
	for (size_t i = 1; i <= UpP->NItems; i++) {
		vUpP.push_back(UpP->GetKey(i));
	}
	printf("");
#endif
}

void XKey::ChainPrevLeaf(XPage* P, longint N)
{
	longint page = 0;
	WORD i = 0, j = 0;
	for (j = XPathN - 1; j >= 1; j--)
		if (XPath[j].I > 1) {
			XF()->RdPage(P, XPath[j].Page);
			i = XPath[j].I - 1;
		label1:
			page = *(P->XI(i, P->IsLeaf)->DownPage);
			XF()->RdPage(P, page);
			if (P->IsLeaf) {
				P->GreaterPage = N;
				XF()->WrPage(P, page);
				return;
			}
			i = P->NItems;
			goto label1;
		}
}

bool XKey::Insert(longint RecNr, bool Try)
{
	longint N = 0, XNr = 0; XString x;
	x.PackKF(KFlds);
	if (Search(x, true, N)) {
		if (Try) { return false; }
		else
		{
			XFNotValid();
			CFileError(822);
		}
	}
	InsertOnPath(x, RecNr);
	return true;
}

void XKey::DeleteOnPath()
{
	longint page = 0;
	longint page1 = 0;
	longint page2 = 0;
	longint uppage = 0;
	void* pp = nullptr;
	XItem* x = nullptr;
	bool released = false;
	longint n = 0;

	MarkStore(pp);
	XPage* p = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	XPage* p1 = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	XPage* p2 = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	XPage* upp = p2;
	for (WORD j = XPathN; j >= 1; j--) {
		page = XPath[j].Page;
		XF()->RdPage(p, page);
		WORD i = XPath[j].I;
		if (p->IsLeaf) p->Delete(i);
		else if (upp->Underflow()) {
			XF()->WrPage(upp, uppage);
			WORD i1 = i - 1;
			WORD i2 = i;
			if (i1 == 0) { i1 = 1; i2 = 2; }
			XIDown(p, p1, i1, page1);
			XIDown(p, p2, i2, page2);
			BalancePages(p1, p2, released);
			XF()->WrPage(p1, page1);
			p->Delete(i1);
			if (released) {
				XF()->ReleasePage(p2, page2);
				if (i1 > p->NItems) p->GreaterPage = page1;
				else {
					p->InsDownIndex(i1, page1, p1);
					p->Delete(i2);
				}
			}
			else {
				XF()->WrPage(p2, page2);
				p->InsDownIndex(i1, page1, p1);
				if (i2 <= p->NItems) {
					p->Delete(i2);
					p->InsDownIndex(i2, page2, p2);
				}
			}
		}
		else {
			if (upp->Overflow()) {
				page1 = XF()->NewPage(p1);
				upp->SplitPage(p1, uppage);
				XF()->WrPage(p1, page1);
				p->InsDownIndex(i, page1, p1); i++;
			}
			XF()->WrPage(upp, uppage);
			if (i <= p->NItems) {
				p->Delete(i);
				p->InsDownIndex(i, uppage, upp);
			}
		}
		uppage = page;
		XPage* px = upp;
		upp = p;
		p = px;
	}
	if (upp->Overflow()) {
		page1 = XF()->NewPage(p1);
		upp->SplitPage(p1, uppage);
		page = XF()->NewPage(p);
		p->GreaterPage = page;
		p->InsDownIndex(1, page1, p1);
		XF()->WrPage(p1, page1);
		XF()->WrPage(p, uppage);
		XF()->WrPage(upp, page);
	}
	else {
		page1 = upp->GreaterPage;
		if ((upp->NItems == 0) && (page1 > 0)) {
			XF()->RdPage(p1, page1);
			Move(p1, upp, XPageSize);
			XF()->ReleasePage(p1, page1);
		}
		XF()->WrPage(upp, uppage);
	}
	ReleaseStore(pp);
}

void XKey::BalancePages(XPage* P1, XPage* P2, bool& Released)
{
	longint n = P1->GreaterPage;
	P1->AddPage(P2);
	WORD sz = P1->EndOff() - uintptr_t(P1);
	if (sz <= XPageSize) Released = true;
	else {
		Released = false;
		Move(P1, P2, sz);
		P2->SplitPage(P1, n);
	}
}

void XKey::XIDown(XPage* P, XPage* P1, WORD I, longint& Page1)
{
	if (I > P->NItems) Page1 = P->GreaterPage;
	else Page1 = *(P->XI(I, P->IsLeaf)->DownPage);
	XF()->RdPage(P1, Page1);
}

bool XKey::Delete(longint RecNr)
{
	XString xx;
	bool b = RecNrToPath(xx, RecNr);
	if (b) DeleteOnPath();
	return b;
}

bool SearchKey(XString& XX, XKey* Key, longint& NN)
{
	longint R = 0;
	XString x;

	auto bResult = false;
	longint L = 1;
	integer Result = _gt;
	NN = CFile->NRecs;
	longint N = NN;
	if (N == 0) return bResult;
	KeyFldD* KF = Key->KFlds;
	do {
		if (Result == _gt) R = N;
		else L = N + 1;
		N = (L + R) / 2;
		ReadRec(CFile, N, CRecPtr);
		x.PackKF(KF);
		Result = CompStr(x.S, XX.S);
	} while (!((L >= R) || (Result == _equ)));
	if ((N == NN) && (Result == _lt)) NN++;
	else {
		if (Key->Duplic && (Result == _equ))
			while (N > 1) {
				N--;
				ReadRec(CFile, N, CRecPtr);
				x.PackKF(KF);
				if (CompStr(x.S, XX.S) != _equ) {
					N++;
					ReadRec(CFile, N, CRecPtr);
					goto label1;
				}
			}
	label1:  NN = N;
	}
	if ((Result == _equ) || Key->Intervaltest && (Result == _gt))
		bResult = true;
	return bResult;
}

longint XNRecs(XKey* K)
{
	if ((CFile->Typ == 'X') && (K != nullptr))
	{
		TestXFExist();
		return CFile->XF->NRecs;
	}
	return CFile->NRecs;
}

void TryInsertAllIndexes(longint RecNr)
{
	void* p = nullptr;
	TestXFExist();
	MarkStore(p);
	XKey* K = CFile->Keys;
	while (K != nullptr) {
		if (!K->Insert(RecNr, true)) goto label1; K = K->Chain;
	}
	CFile->XF->NRecs++;
	return;
label1:
	ReleaseStore(p);
	XKey* K1 = CFile->Keys;
	while ((K1 != nullptr) && (K1 != K)) {
		K1->Delete(RecNr); K1 = K1->Chain;
	}
	SetDeletedFlag();
	WriteRec(CFile, RecNr, CRecPtr);
	/* !!! with CFile->XF^ do!!! */
	if (CFile->XF->FirstDupl) {
		SetMsgPar(CFile->Name);
		WrLLF10Msg(828);
		CFile->XF->FirstDupl = false;
	}
}

void DeleteAllIndexes(longint RecNr)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "DeleteAllIndexes(%i)", RecNr);

	XKey* K = CFile->Keys;
	while (K != nullptr) {
		K->Delete(RecNr);
		K = K->Chain;
	}
}

void DeleteXRec(longint RecNr, bool DelT)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "DeleteXRec(%i, %s)", RecNr, DelT ? "true" : "false");
	TestXFExist();
	DeleteAllIndexes(RecNr);
	if (DelT) DelAllDifTFlds(CRecPtr, nullptr);
	SetDeletedFlag();
	WriteRec(CFile, RecNr, CRecPtr);
	CFile->XF->NRecs--;
}

void OverWrXRec(longint RecNr, void* P2, void* P)
{
	XString x, x2;
	CRecPtr = P2;
	if (DeletedFlag()) { CRecPtr = P; RecallRec(RecNr); return; }
	TestXFExist();
	XKey* K = CFile->Keys;
	while (K != nullptr) {
		CRecPtr = P; x.PackKF(K->KFlds); CRecPtr = P2; x2.PackKF(K->KFlds);
		if (x.S != x2.S) {
			K->Delete(RecNr); CRecPtr = P; K->Insert(RecNr, false);
		}
		K = K->Chain;
	}
	CRecPtr = P;
	WriteRec(CFile, RecNr, CRecPtr);
}
