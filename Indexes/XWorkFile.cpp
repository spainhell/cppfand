#include "XWorkFile.h"

#include "../cppfand/XFile.h"
#include "../cppfand/access.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"


XWorkFile::XWorkFile(XScan* AScan, XKey* AK)
{
	xScan = AScan;
	CFile = xScan->FD;
	xKey = AK;
	xwFile = AK->GetXFile();
}

void XWorkFile::Main(char Typ)
{
	xPage = new XPage();
	nextXPage = xwFile->NewPage(xPage);
	msgWritten = false;
	bool frst = true;
	// for all keys defined in #K
	while (xKey != nullptr) {
		xxPage = new XXPage();
		xxPage->Reset(this);
		xxPage->IsLeaf = true;
		XKey* k = xScan->Key;
		KeyFldD* kf = xKey->KFlds;
		if (xScan->Kind == 1 &&
#ifdef FandSQL
			!xScan->FD->IsSQLFile &&
#endif
			(xScan->Bool == nullptr && (kf == nullptr || EquKFlds(k->KFlds, kf)))) {
			CopyIndex(k, kf, Typ);
		}
		else {
			if (frst) {
				frst = false;
			}
			else {
				xScan->SeekRec(0);
			}
			Reset(xKey->KFlds, sizeof(XXPage) * 9, Typ, xScan->NRecs);
			SortMerge();
		}
		FinishIndex();
		delete xxPage; xxPage = nullptr;
		xKey = xKey->Chain;
	}
	xwFile->ReleasePage(xPage, nextXPage);
	delete xPage;	xPage = nullptr;
}

void XWorkFile::CopyIndex(XKey* K, KeyFldD* KF, char Typ)
{
	
	WRec* r = new WRec();
	// r->X.S = ""; pstring is always "" at the beginning
	XPage* p = new XPage();

	K->NrToPath(1);
	longint page = XPath[XPathN].Page;
	RunMsgOn(Typ, K->NRecs());
	longint count = 0;

	while (page != 0) {
		K->GetXFile()->RdPage(p, page);
		for (size_t i = 1; i <= p->NItems; i++) {
			XItem* x = p->GetItem(i);
			r->PutN(x->GetN());
			if (KF == nullptr) {
				// x = x->Next();
			}
			else {
				x->UpdStr(&r->X.S);
			}
			Output(r);
		}
		count += p->NItems;
		RunMsgN(count);
		page = p->GreaterPage;
	}
	RunMsgOff();
}

bool XWorkFile::GetCRec()
{
	auto result = false;
	xScan->GetRec();
	result = !xScan->eof;
	RecNr = xScan->RecNr;
	IRec = xScan->IRec;
	return result;
}

void XWorkFile::Output(WRec* R)
{
	xxPage->AddToLeaf(R, xKey);
}

void XWorkFile::FinishIndex()
{
	longint sum = 0, n = 0; XXPage* p = xxPage; XXPage* p1 = nullptr;
label1:
	sum = sum + p->Sum;
	p->ClearRest();
	p->GreaterPage = n;
	p1 = p->Chain;
	if (p1 == nullptr) n = xKey->IndexRoot;
	else n = nextXPage;

	// kopie XXPage do XPage a jeji zapis;
	auto xp = std::make_unique<XPage>();
	xp->IsLeaf = p->IsLeaf;
	xp->GreaterPage = p->GreaterPage;
	xp->NItems = p->NItems;
	memcpy(xp->A, p->A, sizeof(p->A)); // p->A ma 1017B, xp->A ma 1024B
	xwFile->WrPage(xp.get(), n, false);

	p = p1;
	if (p != nullptr) {
		nextXPage = xwFile->NewPage(xPage);
		goto label1;
	}
	if (xKey->InWork) ((XWKey*)xKey)->NR = sum;
	else ((XFile*)xwFile)->NRecs = sum;
}