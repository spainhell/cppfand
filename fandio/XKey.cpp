#include <memory>

#include "XKey.h"
#include "XPage.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"
#include "../fandio/FandXFile.h"
#include "../Logging/Logging.h"
#include "../pascal/asm.h"


XKey::XKey(FileD* parent)
{
	parent_ = parent;
	CB = parent->get_callbacks();
}

XKey::XKey(const XKey& orig)
{
	//TODO: if (orig.Chain != nullptr) Chain = new XKey(*orig.Chain);
	parent_ = orig.parent_;
	//TODO: if (orig.KFlds != nullptr) KFlds = new KeyFldD(*orig.KFlds, copyFlds);
	KFlds = orig.KFlds;
	IntervalTest = orig.IntervalTest;
	Duplic = orig.Duplic;
	InWork = orig.InWork;
	IndexRoot = orig.IndexRoot;
	IndexLen = orig.IndexLen;
	NR = orig.NR;
	Alias = orig.Alias;
	CB = orig.CB;
}

/**
 * \brief Returns working or regular index file
 * \return Pointer to working or regular index file
 */
FandXFile* XKey::GetXFile(FileD* file_d)
{
	if (InWork) return &XWork;
	return file_d->FF->XF;
}

int XKey::NRecs()
{
	if (InWork) return NR;
	return parent_->FF->XF->NRecs;
}

bool XKey::Search(FileD* file_d, XString& XX, bool AfterEqu, int& RecNr)
{
	std::string s = XX.S;
	return this->Search(file_d, s, AfterEqu, RecNr);
}

bool XKey::Search(FileD* file_d, std::string const X, bool AfterEqu, int& RecNr)
{
	bool searchResult = false;
	unsigned short iItem = 0;
	XItem* x = nullptr;
	size_t iItemIndex = 0;
	char result = '\0';
	std::unique_ptr<XPage> p = std::make_unique<XPage>();
	XPathN = 1;
	int page = IndexRoot;
	AfterEqu = AfterEqu && Duplic;

	while (true) {
		XPath[XPathN].Page = page;
		GetXFile(file_d)->RdPage(p.get(), page); // je nactena asi cela stranka indexu

		unsigned short o = p->Off();
		unsigned short nItems = p->NItems;
		if (nItems == 0) {
			RecNr = parent_->FF->NRecs + 1;
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
				RecNr = parent_->FF->NRecs + 1;
			}
			else {
				RecNr = x->GetN();
			}

			if (result == _equ) {
				if (((RecNr == 0) || (RecNr > parent_->FF->NRecs))) {
					GetXFile(file_d)->Err(833);
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

bool XKey::SearchInterval(FileD* file_d, XString& XX, bool AfterEqu, int& RecNr)
{
	return Search(file_d, XX, AfterEqu, RecNr) || IntervalTest && (RecNr <= parent_->FF->NRecs);
}

int XKey::PathToNr(FileD* file_d)
{
	int n = 0;
	std::unique_ptr<XPage> p = std::make_unique<XPage>();
	
	for (unsigned short j = 1; j <= XPathN - 1; j++) {
		GetXFile(file_d)->RdPage(p.get(), XPath[j].Page);
		for (unsigned short i = 1; i <= XPath[j].I - 1; i++) {
			XItem* x = p->GetItem(i);
			n += x->GetN();
		}
	}
	n += XPath[XPathN].I;
	if (n > NRecs() + 1) {
		GetXFile(file_d)->Err(834);
	}
	return n;
}

void XKey::NrToPath(FileD* file_d, int I)
{
	auto log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "XKey::NrToPath(%i)", I);

	auto p = std::make_unique<XPage>();
	int page = IndexRoot;
	XPathN = 0;

	while (true) {
		GetXFile(file_d)->RdPage(p.get(), page);
		XPathN++;
		XPath[XPathN].Page = page;
		if (p->IsLeaf) {
			if (I > p->NItems + 1) {
				GetXFile(file_d)->Err(837);
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

int XKey::PathToRecNr(FileD* file_d)
{
	structXPath* X = &XPath[XPathN];
	auto p = std::make_unique<XPage>();

	GetXFile(file_d)->RdPage(p.get(), X->Page);
	XItem* pxi = p->GetItem(X->I);

	int recnr = pxi->GetN();
	int result = recnr;
	if ((recnr == 0) || (recnr > parent_->FF->NRecs)) {
		GetXFile(file_d)->Err(835);
	}
	//ReleaseStore(p);
	return result;
}

bool XKey::RecNrToPath(FileD* file_d, XString& XX, int RecNr, void* record)
{
	bool result = false;
	XItem* x = nullptr; int n = 0;
	XX.PackKF(file_d, KFlds, record);
	Search(file_d, XX, false, n);
	auto p = std::make_unique<XPage>();
	size_t item = 1;

	structXPath* xPath = &XPath[XPathN];

	GetXFile(file_d)->RdPage(p.get(), xPath->Page);
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
			if (IncPath(file_d, XPathN - 1, xPath->Page)) {
				xPath->I = 1;
				GetXFile(file_d)->RdPage(p.get(), xPath->Page);
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

bool XKey::IncPath(FileD* file_d, unsigned short J, int& Pg)
{
	auto p = std::make_unique<XPage>();
	bool result = false;
	structXPath* X = &XPath[J];
	if (J == 0) { goto label2; } /* !!! with XPath[J] do!!! */
	{
	label1:
		GetXFile(file_d)->RdPage(p.get(), X->Page);
		if (X->I > p->NItems) {
			if (IncPath(file_d, J - 1, X->Page)) {
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
				if (IncPath(file_d, J - 1, X->Page)) goto label1;
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

int XKey::NrToRecNr(FileD* file_d, int I)
{
	auto log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "XKey::NrToRecNr(%i)", I);
	NrToPath(file_d, I);
	return PathToRecNr(file_d);
}

std::string XKey::NrToStr(FileD* file_d, int I)
{
	std::unique_ptr<XPage> p = std::make_unique<XPage>();
	NrToPath(file_d, I);
	GetXFile(file_d)->RdPage(p.get(), XPath[XPathN].Page);
	return p->GetKey(I);
}

int XKey::RecNrToNr(FileD* file_d, int RecNr, void* record)
{
	XString x;
	if (RecNrToPath(file_d, x, RecNr, record)) return PathToNr(file_d);
	else return 0;
}

bool XKey::FindNr(FileD* file_d, std::string const X, int& IndexNr)
{
	int n;
	bool result = Search(file_d, X, false, n);
	IndexNr = PathToNr(file_d);
	return result;
}

void XKey::InsertOnPath(FileD* file_d, XString& XX, int RecNr)
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
		GetXFile(file_d)->RdPage(p.get(), page);
		i = XPath[j].I;
		if (p->IsLeaf) {
			InsertLeafItem(file_d, XX, p.get(), upp.get(), page, i, RecNr, uppage);
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
				InsertNonLeafItem(file_d, XX, p.get(), upp.get(), page, i, uppage, upsum, downpage);
			}
		}
		GetXFile(file_d)->WrPage(p.get(), page);
		if (uppage != 0) {
			GetXFile(file_d)->WrPage(upp.get(), uppage);
			upsum = upp->SumN();
			if (upp->IsLeaf) ChainPrevLeaf(file_d, p1.get(), uppage);
		}
	}
	if (uppage != 0) {
		page1 = GetXFile(file_d)->NewPage(p1.get());
		p1->GreaterPage = page1;
		p1->InsDownIndex(1, uppage, upp.get());
		GetXFile(file_d)->WrPage(p.get(), page1);
		GetXFile(file_d)->WrPage(p1.get(), page);
		if (upp->IsLeaf) {
			upp->GreaterPage = page1;
			GetXFile(file_d)->WrPage(upp.get(), uppage);
		}
	}
}

void XKey::InsertNonLeafItem(FileD* file_d, XString& XX, XPage* P, XPage* UpP, int Page, unsigned short I, int& UpPage, unsigned int upSum, unsigned int downPage)
{
	size_t Xlen = 0;
	P->InsertItem(upSum, downPage, I, XX.S);
	UpPage = 0;
	if (P->Overflow()) {
		// page is too long -> will be divided
		//printf("XKey::InsertItem overflow");
		UpPage = GetXFile(file_d)->NewPage(UpP);
		P->SplitPage(UpP, Page);
		// TODO: NUTNO DORESIT, CO SE TADY DEJE - puvodni kod byl delsi
		XX.S = UpP->GetKey(UpP->NItems);
	}
}

void XKey::InsertLeafItem(FileD* file_d, XString& XX, XPage* P, XPage* UpP, int Page, unsigned short I, int RecNr, int& UpPage)
{
	P->InsertItem(RecNr, I, XX.S);
	UpPage = 0;
	if (P->Overflow()) {
		// page is too long -> will be divided
		UpPage = GetXFile(file_d)->NewPage(UpP);
		P->SplitPage(UpP, Page);
		// TODO: NUTNO DORESIT, CO SE TADY DEJE - puvodni kod byl delsi
		XX.S = UpP->GetKey(UpP->NItems);
	}
}

void XKey::ChainPrevLeaf(FileD* file_d, XPage* P, int N)
{
	int page = 0;
	unsigned short i = 0, j = 0;
	for (j = XPathN - 1; j >= 1; j--)
		if (XPath[j].I > 1) {
			GetXFile(file_d)->RdPage(P, XPath[j].Page);
			i = XPath[j].I - 1;
			while (true) {
				page = ((XItemNonLeaf*)P->GetItem(i))->DownPage;
				GetXFile(file_d)->RdPage(P, page);
				if (P->IsLeaf) {
					P->GreaterPage = N;
					GetXFile(file_d)->WrPage(P, page);
					return;
				}
				i = P->NItems;
			}
		}
}

bool XKey::Insert(FileD* file_d, int RecNr, bool Try, void* record)
{
	int N = 0, XNr = 0; XString x;
	x.PackKF(file_d, KFlds, record);
	if (Search(file_d, x, true, N)) {
		if (Try) {
			return false;
		}
		else {
			int result = parent_->FF->XFNotValid();
			if (result != 0) {
				CB->runError(result);
			}
			parent_->CFileError(822);
		}
	}
	InsertOnPath(file_d, x, RecNr);
	return true;
}

void XKey::DeleteOnPath(FileD* file_d)
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
		GetXFile(file_d)->RdPage(p, page);
		unsigned short i = XPath[j].I;
		if (p->IsLeaf) {
			// Leaf
			p->Delete(i);
		}
		else if (p2->Underflow()) {
			// Non Leaf and underflow
			GetXFile(file_d)->WrPage(p2, uppage);
			unsigned short i1 = i - 1;
			unsigned short i2 = i;
			if (i1 == 0) { i1 = 1; i2 = 2; }
			XIDown(file_d, p, p1, i1, page1);
			XIDown(file_d, p, p2, i2, page2);
			BalancePages(p1, &p2, released);
			GetXFile(file_d)->WrPage(p1, page1);
			p->Delete(i1);
			if (released) {
				GetXFile(file_d)->ReleasePage(p2, page2);
				if (i1 > p->NItems) p->GreaterPage = page1;
				else {
					p->InsDownIndex(i1, page1, p1);
					p->Delete(i2);
				}
			}
			else {
				GetXFile(file_d)->WrPage(p2, page2);
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
				page1 = GetXFile(file_d)->NewPage(p1);
				p2->SplitPage(p1, uppage);
				GetXFile(file_d)->WrPage(p1, page1);
				p->InsDownIndex(i, page1, p1); i++;
			}
			GetXFile(file_d)->WrPage(p2, uppage);
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
		page1 = GetXFile(file_d)->NewPage(p1);
		p2->SplitPage(p1, uppage);
		page = GetXFile(file_d)->NewPage(p);
		p->GreaterPage = page;
		p->InsDownIndex(1, page1, p1);
		GetXFile(file_d)->WrPage(p1, page1);
		GetXFile(file_d)->WrPage(p, uppage);
		GetXFile(file_d)->WrPage(p2, page);
	}
	else {
		page1 = p2->GreaterPage;
		if ((p2->NItems == 0) && (page1 > 0)) {
			GetXFile(file_d)->RdPage(p1, page1);

			// kopie p1 do upp
			delete p2; p2 = nullptr;
			p2 = new XPage(*p1);

			GetXFile(file_d)->ReleasePage(p1, page1);
		}
		GetXFile(file_d)->WrPage(p2, uppage);
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

void XKey::XIDown(FileD* file_d, XPage* p, XPage* p1, unsigned short i, int& page1)
{
	if (i > p->NItems) {
		page1 = p->GreaterPage;
	}
	else {
		page1 = ((XItemNonLeaf*)p->GetItem(i))->DownPage;
	}
	GetXFile(file_d)->RdPage(p1, page1);
}

bool XKey::Delete(FileD* file_d, int RecNr, void* record)
{
	XString xx;
	bool b = RecNrToPath(file_d, xx, RecNr, record);
	if (b) DeleteOnPath(file_d);
	return b;
}

void XKey::CalcIndexLen()
{
	IndexLen = 0;
	for (const KeyFldD* key_field : KFlds) {
		if (key_field->FldD != nullptr) {
			IndexLen += key_field->FldD->NBytes;
		}
	}
}
