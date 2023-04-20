#include "XKey.h"
#include <memory>

#include "files.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/obaseww.h"
#include "../fandio/FandXFile.h"
#include "XPage.h"
#include "../Logging/Logging.h"
#include "../pascal/asm.h"
#include "../Common/compare.h"

XKey::XKey()
{
}

XKey::XKey(const XKey& orig)
{
	if (orig.Chain != nullptr) Chain = new XKey(*orig.Chain);
	//if (orig.KFlds != nullptr) KFlds = new KeyFldD(*orig.KFlds, copyFlds);
	KFlds = orig.KFlds;
	IntervalTest = orig.IntervalTest;
	Duplic = orig.Duplic;
	InWork = orig.InWork;
	IndexRoot = orig.IndexRoot;
	IndexLen = orig.IndexLen;
	NR = orig.NR;
	Alias = orig.Alias;
}

XKey::XKey(unsigned char* inputStr)
{
	size_t index = 0;
	Chain = reinterpret_cast<XKey*>(*(unsigned int*)&inputStr[index]); index += 4;
	KFlds = reinterpret_cast<KeyFldD*>(*(unsigned int*)&inputStr[index]); index += 4;
	IntervalTest = *(bool*)&inputStr[index]; index++;
	Duplic = *(bool*)&inputStr[index]; index++;
	InWork = *(bool*)&inputStr[index]; index++;
	IndexRoot = *(unsigned short*)&inputStr[index]; index += 2;
	IndexLen = *(unsigned char*)&inputStr[index]; index++;
	NR = *(int*)&inputStr[index]; index += 4;
	// !!! TODO: jinak bude chybet ALIAS 
	// Alias = reinterpret_cast<std::string*>(*(unsigned int*)&inputStr[index + 1]); index += 4;

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

int XKey::NRecs()
{
	if (InWork) return NR;
	return CFile->FF->XF->NRecs;
}

bool XKey::Search(XString& XX, bool AfterEqu, int& RecNr)
{
	std::string s = XX.S;
	return this->Search(s, AfterEqu, RecNr);
}

bool XKey::Search(std::string const X, bool AfterEqu, int& RecNr)
{
	bool searchResult = false;
	unsigned short iItem = 0;
	XItem* x = nullptr;
	size_t iItemIndex = 0;
	char result = '\0';
	auto p = std::make_unique<XPage>();
	XPathN = 1;
	int page = IndexRoot;
	AfterEqu = AfterEqu && Duplic;

	while (true) {
		XPath[XPathN].Page = page;
		GetXFile()->RdPage(p.get(), page); // je nactena asi cela stranka indexu

		unsigned short o = p->Off();
		unsigned short nItems = p->NItems;
		if (nItems == 0) {
			RecNr = CFile->FF->NRecs + 1;
			XPath[1].I = 1;
			searchResult = false;
			delete x; x = nullptr;
			return searchResult;
		}

		result = XKeySearch(p->A, X, iItem, iItemIndex, nItems, o, AfterEqu);

		XPath[XPathN].I = iItem;

		if (p->IsLeaf) {
			x = new XItemLeaf(&p->A[iItemIndex]);
			if (iItem > nItems) {
				RecNr = CFile->FF->NRecs + 1;
			}
			else {
				RecNr = x->GetN();
			}
			if (result == _equ) {
				if (((RecNr == 0) || (RecNr > CFile->FF->NRecs))) {
					GetXFile()->Err(833);
				}
				else {
					searchResult = true;
				}
			}
			else {
				searchResult = false;
			}
			delete x; x = nullptr;
			return searchResult;
		}
		else {
			x = new XItemNonLeaf(&p->A[iItemIndex]);
		}
		if (iItem > nItems) {
			page = p->GreaterPage;
		}
		else {
			page = ((XItemNonLeaf*)x)->DownPage;
		}
		XPathN++;
		delete x; x = nullptr;
	}
}

bool XKey::SearchInterval(XString& XX, bool AfterEqu, int& RecNr)
{
	return Search(XX, AfterEqu, RecNr) || IntervalTest && (RecNr <= CFile->FF->NRecs);
}

int XKey::PathToNr()
{
	int n = 0;
	auto p = std::make_unique<XPage>();
	
	for (unsigned short j = 1; j <= XPathN - 1; j++) {
		GetXFile()->RdPage(p.get(), XPath[j].Page);
		for (unsigned short i = 1; i <= XPath[j].I - 1; i++) {
			XItem* x = p->GetItem(i);
			n += x->GetN();
		}
	}
	n += XPath[XPathN].I;
	if (n > NRecs() + 1) {
		GetXFile()->Err(834);
	}
	return n;
}

void XKey::NrToPath(int I)
{
	auto log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "XKey::NrToPath(%i)", I);

	auto p = std::make_unique<XPage>();
	int page = IndexRoot;
	XPathN = 0;

	while (true) {
		GetXFile()->RdPage(p.get(), page);
		XPathN++;
		XPath[XPathN].Page = page;
		if (p->IsLeaf) {
			if (I > p->NItems + 1) {
				GetXFile()->Err(837);
			}
			XPath[XPathN].I = I;
			//ReleaseStore(p);
			return;
		}
		else {
			// Non Leaf
			bool next = false;
			for (unsigned short j = 1; j <= p->NItems; j++) {
				XItem* x = p->GetItem(j);
				if (I <= x->GetN()) {
					XPath[XPathN].I = j;
					page = ((XItemNonLeaf*)x)->DownPage;
					next = true;
					break;
				}
				I -= x->GetN();
			}
			if (next) {
				continue;
			}

			XPath[XPathN].I = p->NItems + 1;
			page = p->GreaterPage;
		}
	}
}

int XKey::PathToRecNr()
{
	structXPath* X = &XPath[XPathN];
	auto p = std::make_unique<XPage>();

	GetXFile()->RdPage(p.get(), X->Page);
	XItem* pxi = p->GetItem(X->I);

	int recnr = pxi->GetN();
	int result = recnr;
	if ((recnr == 0) || (recnr > CFile->FF->NRecs)) {
		GetXFile()->Err(835);
	}
	//ReleaseStore(p);
	return result;
}

bool XKey::RecNrToPath(XString& XX, int RecNr)
{
	bool result = false;
	XItem* x = nullptr; int n = 0;
	XX.PackKF(KFlds);
	Search(XX, false, n);
	auto p = std::make_unique<XPage>();
	size_t item = 1;

	structXPath* xPath = &XPath[XPathN];

	GetXFile()->RdPage(p.get(), xPath->Page);
	x = p->GetItem(xPath->I);
	if (x == nullptr) {
		return result;
	}

	std::string xxS = XX.S;
	if (p->GetKey(xPath->I) != xxS) {
		return result;
	}

	while (true) {
		if (x->GetN() == RecNr) {
			result = true;
			return result;
		}
		xPath->I++;
		if (xPath->I > p->NItems) {
			if (IncPath(XPathN - 1, xPath->Page)) {
				xPath->I = 1;
				GetXFile()->RdPage(p.get(), xPath->Page);
				x = p->GetItem(xPath->I);
				xxS = XX.S;
				if (p->GetKey(xPath->I) != xxS) {
					return result;
				}
				continue;
			}
		}
		else {
			x = p->GetItem(xPath->I);
			if (x->GetL() != 0) {
				return result;
			}
			continue;
		}
		break;
	}

	return result;
}

bool XKey::IncPath(unsigned short J, int& Pg)
{
	auto p = std::make_unique<XPage>();
	bool result = false;
	structXPath* X = &XPath[J];
	if (J == 0) { goto label2; } /* !!! with XPath[J] do!!! */
	{
	label1:
		GetXFile()->RdPage(p.get(), X->Page);
		if (X->I > p->NItems) {
			if (IncPath(J - 1, X->Page)) {
				X->I = 0;
				goto label1;
			}
			else {
				goto label2;
			}
		}
		X->I++;
		if (X->I > p->NItems)
			if (p->GreaterPage == 0) {
				X->I = 0;
				if (IncPath(J - 1, X->Page)) goto label1;
				goto label2;
			}
			else {
				Pg = p->GreaterPage;
			}
		else {
			XItem* item = p->GetItem(X->I);
			Pg = ((XItemNonLeaf*)item)->DownPage;
		}
	}
	result = true;
label2:
	//ReleaseStore(p);
	return result;
}

int XKey::NrToRecNr(int I)
{
	auto log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "XKey::NrToRecNr(%i)", I);
	NrToPath(I);
	return PathToRecNr();
}

pstring XKey::NrToStr(int I)
{
	pstring result;
	auto p = std::make_unique<XPage>();
	NrToPath(I);
	GetXFile()->RdPage(p.get(), XPath[XPathN].Page);
	result = p->GetKey(I);
	return result;
}

int XKey::RecNrToNr(int RecNr)
{
	XString x;
	if (RecNrToPath(x, RecNr)) return PathToNr();
	else return 0;
}

//bool XKey::FindNr(XString& X, int& IndexNr)
//{
//	int n;
//	bool result = Search(X, false, n);
//	IndexNr = PathToNr();
//	return result;
//}

bool XKey::FindNr(std::string const X, int& IndexNr)
{
	int n;
	bool result = Search(X, false, n);
	IndexNr = PathToNr();
	return result;
}

void XKey::InsertOnPath(XString& XX, int RecNr)
{
	unsigned short i = 0, j = 0;
	int page = 0, page1 = 0, uppage = 0, downpage = 0;
	XItem* x = nullptr;
	int n = 0, upsum = 0;

	auto p = std::make_unique<XPage>();
	auto p1 = std::make_unique<XPage>();
	auto upp = std::make_unique<XPage>();
	for (j = XPathN; j >= 1; j--) {
		page = XPath[j].Page;
		GetXFile()->RdPage(p.get(), page);
		i = XPath[j].I;
		if (p->IsLeaf) {
			InsertLeafItem(XX, p.get(), upp.get(), page, i, RecNr, uppage);
		}
		else {
			if (i <= p->NItems) {
				x = p->GetItem(i);
				n = x->GetN() + 1;
				if (uppage != 0) n -= upsum;
				x->PutN(n);
			}
			if (uppage != 0) {
				downpage = uppage;
				InsertNonLeafItem(XX, p.get(), upp.get(), page, i, uppage, upsum, downpage);
			}
		}
		GetXFile()->WrPage(p.get(), page);
		if (uppage != 0) {
			GetXFile()->WrPage(upp.get(), uppage);
			upsum = upp->SumN();
			if (upp->IsLeaf) ChainPrevLeaf(p1.get(), uppage);
		}
	}
	if (uppage != 0) {
		page1 = GetXFile()->NewPage(p1.get());
		p1->GreaterPage = page1;
		p1->InsDownIndex(1, uppage, upp.get());
		GetXFile()->WrPage(p.get(), page1);
		GetXFile()->WrPage(p1.get(), page);
		if (upp->IsLeaf) {
			upp->GreaterPage = page1;
			GetXFile()->WrPage(upp.get(), uppage);
		}
	}
}

void XKey::InsertNonLeafItem(XString& XX, XPage* P, XPage* UpP, int Page, unsigned short I, int& UpPage, unsigned int upSum, unsigned int downPage)
{
	size_t Xlen = 0;
	P->InsertItem(upSum, downPage, I, XX.S);
	UpPage = 0;
	if (P->Overflow()) {
		// page is too long -> will be divided
		printf("XKey::InsertItem overflow");
		UpPage = GetXFile()->NewPage(UpP);
		P->SplitPage(UpP, Page);
		// TODO: NUTNO DORESIT, CO SE TADY DEJE - puvodni kod byl delsi
		XX.S = UpP->GetKey(UpP->NItems);
	}
}

void XKey::InsertLeafItem(XString& XX, XPage* P, XPage* UpP, int Page, unsigned short I, int RecNr, int& UpPage)
{
	P->InsertItem(RecNr, I, XX.S);
	UpPage = 0;
	if (P->Overflow()) {
		// page is too long -> will be divided
		UpPage = GetXFile()->NewPage(UpP);
		P->SplitPage(UpP, Page);
		// TODO: NUTNO DORESIT, CO SE TADY DEJE - puvodni kod byl delsi
		XX.S = UpP->GetKey(UpP->NItems);
	}
}

void XKey::ChainPrevLeaf(XPage* P, int N)
{
	int page = 0;
	unsigned short i = 0, j = 0;
	for (j = XPathN - 1; j >= 1; j--)
		if (XPath[j].I > 1) {
			GetXFile()->RdPage(P, XPath[j].Page);
			i = XPath[j].I - 1;
			while (true) {
				page = ((XItemNonLeaf*)P->GetItem(i))->DownPage;
				GetXFile()->RdPage(P, page);
				if (P->IsLeaf) {
					P->GreaterPage = N;
					GetXFile()->WrPage(P, page);
					return;
				}
				i = P->NItems;
			}
		}
}

bool XKey::Insert(int RecNr, bool Try)
{
	int N = 0, XNr = 0; XString x;
	x.PackKF(KFlds);
	if (Search(x, true, N)) {
		if (Try) {
			return false;
		}
		else {
			XFNotValid();
			CFileError(CFile, 822);
		}
	}
	InsertOnPath(x, RecNr);
	return true;
}

void XKey::DeleteOnPath()
{
	int page = 0;
	int page1 = 0;
	int page2 = 0;
	int uppage = 0;
	bool released = false;
	XPage* p = new XPage();
	XPage* p1 = new XPage();
	XPage* p2 = new XPage();
	for (unsigned short j = XPathN; j >= 1; j--) {
		page = XPath[j].Page;
		GetXFile()->RdPage(p, page);
		unsigned short i = XPath[j].I;
		if (p->IsLeaf) {
			// Leaf
			p->Delete(i);
		}
		else if (p2->Underflow()) {
			// Non Leaf and underflow
			GetXFile()->WrPage(p2, uppage);
			unsigned short i1 = i - 1;
			unsigned short i2 = i;
			if (i1 == 0) { i1 = 1; i2 = 2; }
			XIDown(p, p1, i1, page1);
			XIDown(p, p2, i2, page2);
			BalancePages(p1, &p2, released);
			GetXFile()->WrPage(p1, page1);
			p->Delete(i1);
			if (released) {
				GetXFile()->ReleasePage(p2, page2);
				if (i1 > p->NItems) p->GreaterPage = page1;
				else {
					p->InsDownIndex(i1, page1, p1);
					p->Delete(i2);
				}
			}
			else {
				GetXFile()->WrPage(p2, page2);
				p->InsDownIndex(i1, page1, p1);
				if (i2 <= p->NItems) {
					p->Delete(i2);
					p->InsDownIndex(i2, page2, p2);
				}
			}
		}
		else {
			// Non Leaf
			if (p2->Overflow()) {
				page1 = GetXFile()->NewPage(p1);
				p2->SplitPage(p1, uppage);
				GetXFile()->WrPage(p1, page1);
				p->InsDownIndex(i, page1, p1); i++;
			}
			GetXFile()->WrPage(p2, uppage);
			if (i <= p->NItems) {
				p->Delete(i);
				p->InsDownIndex(i, uppage, p2);
			}
		}
		uppage = page;
		XPage* px = p2;
		p2 = p;
		p = px;
	}
	if (p2->Overflow()) {
		page1 = GetXFile()->NewPage(p1);
		p2->SplitPage(p1, uppage);
		page = GetXFile()->NewPage(p);
		p->GreaterPage = page;
		p->InsDownIndex(1, page1, p1);
		GetXFile()->WrPage(p1, page1);
		GetXFile()->WrPage(p, uppage);
		GetXFile()->WrPage(p2, page);
	}
	else {
		page1 = p2->GreaterPage;
		if ((p2->NItems == 0) && (page1 > 0)) {
			GetXFile()->RdPage(p1, page1);

			// kopie p1 do upp
			delete p2; p2 = nullptr;
			p2 = new XPage(*p1);

			GetXFile()->ReleasePage(p1, page1);
		}
		GetXFile()->WrPage(p2, uppage);
	}
	delete p;
	delete p1;
	delete p2;
}

void XKey::BalancePages(XPage* P1, XPage** P2, bool& Released)
{
	int n = P1->GreaterPage;
	P1->AddPage(*P2);
	unsigned short sz = P1->ItemsSize();
	if (sz <= XPageSize - 7) { // header = 7B (1 + 4 + 2)
		Released = true;
	}
	else {
		Released = false;
		// will copy P1 into P2
		delete* P2;
		*P2 = new XPage(*P1);
		// will clean P1
		P1->Clean();
		// will move part of items into P1
		(*P2)->SplitPage(P1, n);
	}
}

void XKey::XIDown(XPage* p, XPage* p1, unsigned short i, int& page1)
{
	if (i > p->NItems) {
		page1 = p->GreaterPage;
	}
	else {
		page1 = ((XItemNonLeaf*)p->GetItem(i))->DownPage;
	}
	GetXFile()->RdPage(p1, page1);
}

bool XKey::Delete(int RecNr)
{
	XString xx;
	bool b = RecNrToPath(xx, RecNr);
	if (b) DeleteOnPath();
	return b;
}

bool SearchKey(XString& XX, XKey* Key, int& NN)
{
	int R = 0;
	XString x;

	bool bResult = false;
	int L = 1;
	short Result = _gt;
	NN = CFile->FF->NRecs;
	int N = NN;
	if (N == 0) return bResult;
	KeyFldD* KF = Key->KFlds;

	do {
		if (Result == _gt) {
			R = N;
		}
		else {
			L = N + 1;
		}
		N = (L + R) / 2;
		CFile->ReadRec(N, CRecPtr);
		x.PackKF(KF);
		Result = CompStr(x.S, XX.S);
	} while (!((L >= R) || (Result == _equ)));

	if ((N == NN) && (Result == _lt)) NN++;
	else {
		if (Key->Duplic && (Result == _equ)) {
			while (N > 1) {
				N--;
				CFile->ReadRec(N, CRecPtr);
				x.PackKF(KF);
				if (CompStr(x.S, XX.S) != _equ) {
					N++;
					CFile->ReadRec(N, CRecPtr);
					break;
				}
			}
		}
		NN = N;
	}
	if ((Result == _equ) || Key->IntervalTest && (Result == _gt))
		bResult = true;
	return bResult;
}

int XNRecs(std::vector<XKey*>& K)
{
	if (CFile->FF->file_type == FileType::INDEX && !K.empty()) {
		TestXFExist();
		return CFile->FF->XF->NRecs;
	}
	return CFile->FF->NRecs;
}

void TryInsertAllIndexes(int RecNr)
{
	TestXFExist();
	XKey* lastK = nullptr;
	for (auto& K : CFile->Keys) {
		lastK = K;
		if (!K->Insert(RecNr, true)) {
			goto label1;
		}
	}
	CFile->FF->XF->NRecs++;
	return;

label1:
	for (auto& K1 : CFile->Keys) {
		if (K1 == lastK) {
			break;
		}
		K1->Delete(RecNr);
	}
	SetDeletedFlag(CFile->FF, CRecPtr);
	CFile->WriteRec(RecNr, CRecPtr);

	if (CFile->FF->XF->FirstDupl) {
		SetMsgPar(CFile->Name);
		WrLLF10Msg(828);
		CFile->FF->XF->FirstDupl = false;
	}
}

void DeleteAllIndexes(int RecNr)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "DeleteAllIndexes(%i)", RecNr);

	for (auto& K : CFile->Keys) {
		K->Delete(RecNr);
	}
}

void DeleteXRec(int RecNr, bool DelT)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "DeleteXRec(%i, %s)", RecNr, DelT ? "true" : "false");
	TestXFExist();
	DeleteAllIndexes(RecNr);
	if (DelT) CFile->DelAllDifTFlds(CRecPtr, nullptr);
	SetDeletedFlag(CFile->FF, CRecPtr);
	CFile->WriteRec(RecNr, CRecPtr);
	CFile->FF->XF->NRecs--;
}

void OverWrXRec(int RecNr, void* P2, void* P)
{
	XString x, x2;
	CRecPtr = P2;
	if (DeletedFlag(CFile->FF, CRecPtr)) {
		CRecPtr = P;
		CFile->RecallRec(RecNr, CRecPtr);
		return;
	}
	TestXFExist();

	for (auto& K : CFile->Keys) {
		CRecPtr = P;
		x.PackKF(K->KFlds);
		CRecPtr = P2;
		x2.PackKF(K->KFlds);
		if (x.S != x2.S) {
			K->Delete(RecNr);
			CRecPtr = P;
			K->Insert(RecNr, false);
		}
	}

	CRecPtr = P;
	CFile->WriteRec(RecNr, CRecPtr);
}
