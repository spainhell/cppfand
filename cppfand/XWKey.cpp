#include "XWKey.h"
#include "access.h"
#include "base.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "XPage.h"


void XWKey::Open(KeyFldD* KF, bool Dupl, bool Intvl)
{
	KFlds = KF;
	Duplic = Dupl;
	InWork = true;
	Intervaltest = Intvl;
	NR = 0;
	//XPage* p = (XPage*)GetStore(sizeof(p)); 
	XPage* p = new XPage();
	IndexRoot = XF()->NewPage(p);
	p->IsLeaf = true;
	XF()->WrPage(p, IndexRoot);
	ReleaseStore(p);
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
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	XF()->RdPage(p, Page);
	if (!p->IsLeaf) {
		WORD n = p->NItems;
		for (WORD i = 1; i <= n; i++) {
			ReleaseTree(*(p->XI(i, p->IsLeaf)->DownPage), IsClose);
			XF()->RdPage(p, Page);
		}
		if (p->GreaterPage != 0) ReleaseTree(p->GreaterPage, IsClose);
	}
	if ((Page != IndexRoot) || IsClose)
		XF()->ReleasePage(p, Page);
	else {
		FillChar(p, XPageSize, 0);
		p->IsLeaf = true;
		XF()->WrPage(p, Page);
	}
	ReleaseStore(p);
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
	XPage* p = new XPage(); // (XPage*)GetStore(sizeof(*p));
	/* !!! with XPath[XPathN] do!!! */
	longint pg = XPath[XPathN].Page;
	integer j = XPath[XPathN].I;
	do {
		XF()->RdPage(p, pg);
		integer n = p->NItems - j + 1;
		XItem* x = p->XI(j, p->IsLeaf);
		while (n > 0) {
			longint nn = x->GetN();
			if (nn >= RecNr) x->PutN(nn + Dif);
			auto prevX = x;
			x = x->Next(oLeaf, p->IsLeaf);
			delete prevX; prevX = nullptr;
			n--;
		}
		XF()->WrPage(p, pg);
		pg = p->GreaterPage;
		j = 1;
	} while (pg != 0);
	ReleaseStore(p);
}