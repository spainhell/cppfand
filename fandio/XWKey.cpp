#include "XWKey.h"
#include <memory>
#include "../Core/access.h"
#include "../Core/base.h"
#include "../Core/FieldDescr.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"
#include "../Core/legacy.h"
#include "XPage.h"


void XWKey::Open(FileD* file_d, KeyFldD* KF, bool Dupl, bool Intvl)
{
	KFlds = KF;
	Duplic = Dupl;
	InWork = true;
	IntervalTest = Intvl;
	NR = 0;
	//XPage* p = (XPage*)GetStore(sizeof(p)); 
	auto p = std::make_unique<XPage>();
	IndexRoot = GetXFile(file_d)->NewPage(p.get());
	p->IsLeaf = true;
	GetXFile(file_d)->WrPage(p.get(), IndexRoot);
	//ReleaseStore(p);
	IndexLen = 0;
	while (KF != nullptr) {
		if (KF->FldD != nullptr) IndexLen += KF->FldD->NBytes;
		KF = KF->pChain;
	}
}

void XWKey::Close(FileD* file_d)
{
	ReleaseTree(file_d, IndexRoot, true);
	IndexRoot = 0;
}

void XWKey::Release(FileD* file_d)
{
	ReleaseTree(file_d, IndexRoot, false);
	NR = 0;
}

void XWKey::ReleaseTree(FileD* file_d, int Page, bool IsClose)
{
	if ((Page == 0) || (Page > GetXFile(file_d)->MaxPage)) return;
	auto p = std::make_unique<XPage>();
	GetXFile(file_d)->RdPage(p.get(), Page);
	if (!p->IsLeaf) {
		unsigned short n = p->NItems;
		for (unsigned short i = 1; i <= n; i++) {
			XItemNonLeaf* item = (XItemNonLeaf*)p->GetItem(i);
			ReleaseTree(file_d, item->DownPage, IsClose);
			GetXFile(file_d)->RdPage(p.get(), Page);
		}
		if (p->GreaterPage != 0) ReleaseTree(file_d, p->GreaterPage, IsClose);
	}
	if ((Page != IndexRoot) || IsClose)
		GetXFile(file_d)->ReleasePage(p.get(), Page);
	else {
		p->Clean(); //FillChar(p.get(), XPageSize, 0);
		p->IsLeaf = true;
		GetXFile(file_d)->WrPage(p.get(), Page);
	}
	//ReleaseStore(p);
}

void XWKey::OneRecIdx(FileD* file_d, KeyFldD* KF, int N, void* record)
{
	Open(file_d, KF, true, false);
	Insert(file_d, N, true, record);
	NR++;
}

void XWKey::InsertAtNr(FileD* file_d, int I, int RecNr, void* record)
{
	XString x;
	x.PackKF(file_d, KFlds, record);
	NR++;
	NrToPath(file_d, I);
	InsertOnPath(file_d, x, RecNr);
}

int XWKey::InsertGetNr(FileD* file_d, int RecNr, void* record)
{
	XString x; int n;
	NR++; x.PackKF(file_d, KFlds, record);
	Search(file_d, x, true, n);
	int result = PathToNr(file_d);
	InsertOnPath(file_d, x, RecNr);
	return result;
}

void XWKey::DeleteAtNr(FileD* file_d, int I)
{
	NrToPath(file_d, I);
	DeleteOnPath(file_d);
	NR--;
}

void XWKey::AddToRecNr(FileD* file_d, int RecNr, short Dif)
{
	if (NRecs() == 0) return;
	NrToPath(file_d, 1);
	auto p = std::make_unique<XPage>();
	size_t item = XPath[XPathN].I;
	do {
		GetXFile(file_d)->RdPage(p.get(), XPath[XPathN].Page);
		short n = p->NItems - XPath[XPathN].I + 1;
		while (n > 0) {
			XItem* x = p->GetItem(XPath[XPathN].I++);
			int nn = x->GetN();
			if (nn >= RecNr) {
				x->PutN(nn + Dif);
			}
			n--;
		}
		GetXFile(file_d)->WrPage(p.get(), XPath[XPathN].Page);
		XPath[XPathN].Page = p->GreaterPage;
		XPath[XPathN].I = 1;
	} while (XPath[XPathN].Page != 0);
	//ReleaseStore(p);
}
