#include "XWorkFile.h"

#include "../cppfand/XFile.h"
#include "../cppfand/access.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"


XWorkFile::XWorkFile(XScan* AScan, XKey* AK)
{
	Scan = AScan;
	CFile = Scan->FD;
	KD = AK;
	XF = AK->XF();
}

XWorkFile::XWorkFile(XScan* AScan, std::vector<XKey*>& AK)
{
	Scan = AScan;
	CFile = Scan->FD;
	KD = AK.empty() ? nullptr : AK[0];
	XF = AK.empty() ? nullptr : AK[0]->XF();
}

void XWorkFile::Main(char Typ)
{
	XPP = new XPage();
	NxtXPage = XF->NewPage(XPP);
	MsgWritten = false;
	bool frst = true;
	// for all keys defined in #K
	while (KD != nullptr) {
		PX = new XXPage();
		PX->Reset(this);
		PX->IsLeaf = true;
		XKey* k = Scan->Key;
		KeyFldD* kf = KD->KFlds;
		if (Scan->Kind == 1 &&
#ifdef FandSQL
			!Scan->FD->IsSQLFile &&
#endif
			(Scan->Bool == nullptr
				&& (kf == nullptr || EquKFlds(k->KFlds, kf)))) {
			CopyIndex(k, kf, Typ);
		}
		else {
			if (frst) {
				frst = false;
			}
			else {
				Scan->SeekRec(0);
			}
			Reset(KD->KFlds, sizeof(XXPage) * 9, Typ, Scan->NRecs);
			SortMerge();
		}
		FinishIndex();
		delete PX; PX = nullptr;
		KD = KD->Chain;
	}
	XF->ReleasePage(XPP, NxtXPage);
	delete XPP;	XPP = nullptr;
}

void XWorkFile::CopyIndex(XKey* K, KeyFldD* KF, char Typ)
{
	WRec* r = nullptr;
	XPage* p = nullptr;
	longint page; WORD n; longint i;
	XItem* x = nullptr;
	WORD* xofs = (WORD*)x;

	r = (WRec*)GetStore(sizeof(WRec));
	r->X.S = "";
	p = (XPage*)GetStore(XPageSize);
	K->NrToPath(1);
	page = XPath[XPathN].Page;
	RunMsgOn(Typ, K->NRecs());
	i = 0;
	while (page != 0) {
		K->XF()->RdPage(p, page);
		x = (XItem*)(&p->A);
		n = p->NItems;
		while (n > 0) {
			r->PutN(x->GetN());
			// TODO: zbavit se Next() - TOTO NEBUDE FUNGOVAT!!!
			if (KF == nullptr) x = x->Next();
			else *xofs = x->UpdStr(&r->X.S);
			Output(r);
			n--;
		}
		i += p->NItems;
		RunMsgN(i);
		page = p->GreaterPage;
	}
	RunMsgOff();
}

bool XWorkFile::GetCRec()
{
	auto result = false;
	Scan->GetRec();
	result = !Scan->eof;
	RecNr = Scan->RecNr;
	IRec = Scan->IRec;
	return result;
}

void XWorkFile::Output(WRec* R)
{
	PX->AddToLeaf(R, KD);
}

void XWorkFile::FinishIndex()
{
	longint sum = 0, n = 0; XXPage* p = PX; XXPage* p1 = nullptr;
label1:
	sum = sum + p->Sum;
	p->ClearRest();
	p->GreaterPage = n;
	p1 = p->Chain;
	if (p1 == nullptr) n = KD->IndexRoot;
	else n = NxtXPage;

	// kopie XXPage do XPage a jeji zapis;
	auto xp = std::make_unique<XPage>();
	xp->IsLeaf = p->IsLeaf;
	xp->GreaterPage = p->GreaterPage;
	xp->NItems = p->NItems;
	memcpy(xp->A, p->A, sizeof(p->A)); // p->A ma 1017B, xp->A ma 1024B
	XF->WrPage(xp.get(), n, false);

	p = p1;
	if (p != nullptr) {
		NxtXPage = XF->NewPage(XPP);
		goto label1;
	}
	if (KD->InWork) ((XWKey*)KD)->NR = sum;
	else ((XFile*)XF)->NRecs = sum;
}