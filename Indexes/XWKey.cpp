#include "XWKey.h"
#include <memory>
#include "../cppfand/access.h"
#include "../cppfand/base.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/legacy.h"
#include "XPage.h"


void XWKey::Open(KeyFldD* KF, bool Dupl, bool Intvl)
{
	KFlds = KF;
	Duplic = Dupl;
	InWork = true;
	IntervalTest = Intvl;
	NR = 0;
	//XPage* p = (XPage*)GetStore(sizeof(p)); 
	auto p = std::make_unique<XPage>();
	IndexRoot = GetXFile()->NewPage(p.get());
	p->IsLeaf = true;
	GetXFile()->WrPage(p.get(), IndexRoot);
	//ReleaseStore(p);
	IndexLen = 0;
	while (KF != nullptr) {
		if (KF->FldD != nullptr) IndexLen += KF->FldD->NBytes;
		KF = (KeyFldD*)KF->pChain;
	}
}

void XWKey::Close()
{
	ReleaseTree(IndexRoot, true);
	IndexRoot = 0;
}

void XWKey::Release()
{
	ReleaseTree(IndexRoot, false);
	NR = 0;
}

void XWKey::ReleaseTree(longint Page, bool IsClose)
{
	if ((Page == 0) || (Page > GetXFile()->MaxPage)) return;
	auto p = std::make_unique<XPage>();
	GetXFile()->RdPage(p.get(), Page);
	if (!p->IsLeaf) {
		WORD n = p->NItems;
		for (WORD i = 1; i <= n; i++) {
			XItemNonLeaf* item = (XItemNonLeaf*)p->GetItem(i);
			ReleaseTree(item->DownPage, IsClose);
			GetXFile()->RdPage(p.get(), Page);
		}
		if (p->GreaterPage != 0) ReleaseTree(p->GreaterPage, IsClose);
	}
	if ((Page != IndexRoot) || IsClose)
		GetXFile()->ReleasePage(p.get(), Page);
	else {
		p->Clean(); //FillChar(p.get(), XPageSize, 0);
		p->IsLeaf = true;
		GetXFile()->WrPage(p.get(), Page);
	}
	//ReleaseStore(p);
}

void XWKey::OneRecIdx(KeyFldD* KF, longint N)
{
	Open(KF, true, false);
	Insert(N, true);
	NR++;
}

void XWKey::InsertAtNr(longint I, longint RecNr)
{
	XString x;
	x.PackKF(KFlds);
	NR++;
	NrToPath(I);
	InsertOnPath(x, RecNr);
}

longint XWKey::InsertGetNr(longint RecNr)
{
	XString x; longint n;
	NR++; x.PackKF(KFlds);
	Search(x, true, n);
	auto result = PathToNr();
	InsertOnPath(x, RecNr);
	return result;
}

void XWKey::DeleteAtNr(longint I)
{
	NrToPath(I);
	DeleteOnPath();
	NR--;
}

void XWKey::AddToRecNr(longint RecNr, integer Dif)
{
	if (NRecs() == 0) return;
	NrToPath(1);
	auto p = std::make_unique<XPage>();
	size_t item = XPath[XPathN].I;
	do {
		GetXFile()->RdPage(p.get(), XPath[XPathN].Page);
		integer n = p->NItems - XPath[XPathN].I + 1;
		while (n > 0) {
			XItem* x = p->GetItem(XPath[XPathN].I++);
			longint nn = x->GetN();
			if (nn >= RecNr) {
				x->PutN(nn + Dif);
			}
			n--;
		}
		GetXFile()->WrPage(p.get(), XPath[XPathN].Page);
		XPath[XPathN].Page = p->GreaterPage;
		XPath[XPathN].I = 1;
	} while (XPath[XPathN].Page != 0);
	//ReleaseStore(p);
}
