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
	delete xPage; xPage = nullptr;
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
				std::string s = r->X.S;
				//x->UpdStr(&r->X.S);
				s = x->GetKey(s);
				r->X.S = s;
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
	longint sum = 0;
	longint n = 0;
	XXPage* p = xxPage;

	while (true) {
		sum += p->Sum;
		p->ClearRest();
		p->GreaterPage = n;
		XXPage* p1 = p->Chain;
		if (p1 == nullptr) {
			n = xKey->IndexRoot;
		}
		else {
			n = nextXPage;
		}

		xwFile->WrPage(p, n);

		p = p1;
		if (p != nullptr) {
			nextXPage = xwFile->NewPage(xPage);
			continue;
		}

		break;
	}

	if (xKey->InWork) {
		xKey->NR = sum;
	}
	else {
		((XFile*)xwFile)->NRecs = sum;
	}
}