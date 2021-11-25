#include "XWKey.h"
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
	Intervaltest = Intvl;
	NR = 0;
	//XPage* p = (XPage*)GetStore(sizeof(p)); 
	auto p = std::make_unique<XPage>();
	IndexRoot = XF()->NewPage(p.get());
	p->IsLeaf = true;
	XF()->WrPage(p.get(), IndexRoot);
	//ReleaseStore(p);
	IndexLen = 0;
	while (KF != nullptr) {
		if (KF->FldD != nullptr) IndexLen += KF->FldD->NBytes;
		KF = (KeyFldD*)KF->Chain;
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
	if ((Page == 0) || (Page > XF()->MaxPage)) return;
	auto p = std::make_unique<XPage>();
	XF()->RdPage(p.get(), Page);
	if (!p->IsLeaf) {
		WORD n = p->NItems;
		for (WORD i = 1; i <= n; i++) {
			XItemNonLeaf* item = (XItemNonLeaf*)p->XI(i);
			ReleaseTree(item->DownPage, IsClose);
			XF()->RdPage(p.get(), Page);
		}
		if (p->GreaterPage != 0) ReleaseTree(p->GreaterPage, IsClose);
	}
	if ((Page != IndexRoot) || IsClose)
		XF()->ReleasePage(p.get(), Page);
	else {
		p->Clean(); //FillChar(p.get(), XPageSize, 0);
		p->IsLeaf = true;
		XF()->WrPage(p.get(), Page);
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
	longint pg = XPath[XPathN].Page;
	integer j = XPath[XPathN].I;
	size_t item = j;
	do {
		XF()->RdPage(p.get(), pg);
		integer n = p->NItems - j + 1;
		while (n > 0) {
			XItem* x = p->XI(j++);
			longint nn = x->GetN();
			if (nn >= RecNr) x->PutN(nn + Dif);
			n--;
		}
		XF()->WrPage(p.get(), pg);
		pg = p->GreaterPage;
		j = 1;
	} while (pg != 0);
	//ReleaseStore(p);
}
